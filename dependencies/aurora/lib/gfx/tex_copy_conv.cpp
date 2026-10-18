#include "tex_copy_conv.hpp"

#include "resources.hpp"
#include "recording.hpp"
#include "diagnostic_capture.hpp"
#include <aurora/diagnostics.h>
#include <cstring>

#include "../internal.hpp"
#include "../gx/gx.hpp"
#include "../webgpu/gpu.hpp"
#include "../webgpu/gpu_prof.hpp"
#include "texture.hpp"
#include "../gx/gx_fmt.hpp"

#include <absl/container/flat_hash_map.h>

#include "texture_convert.hpp"

using namespace std::string_literals;

namespace aurora::gfx::tex_copy_conv {
static Module Log("aurora::gfx::tex_copy_conv");

using webgpu::g_device;

static constexpr std::string_view ShaderPreamble = R"(
@group(0) @binding(0) var src_samp: sampler;
@group(0) @binding(1) var src: texture_2d<f32>;

struct UVTransform {
    offset: vec2f,
    scale: vec2f,
};
@group(0) @binding(2) var<uniform> uv_xf: UVTransform;

struct VertexOutput {
    @builtin(position) pos: vec4f,
    @location(0) uv: vec2f,
};

var<private> positions: array<vec2f, 3> = array(
    vec2f(-1.0, 1.0),
    vec2f(-1.0, -3.0),
    vec2f(3.0, 1.0),
);
var<private> uvs: array<vec2f, 3> = array(
    vec2f(0.0, 0.0),
    vec2f(0.0, 2.0),
    vec2f(2.0, 0.0),
);

@vertex fn vs_main(@builtin(vertex_index) vi: u32) -> VertexOutput {
    var out: VertexOutput;
    out.pos = vec4f(positions[vi], 0.0, 1.0);
    out.uv = uvs[vi] * uv_xf.scale + uv_xf.offset;
    return out;
}

fn intensity(rgb: vec3f) -> f32 {
    // ITU-R BT.601 luma coefficients
    return dot(rgb, vec3f(0.257, 0.504, 0.098)) + 16.0 / 255.0;
}

fn quantize4(v: f32) -> f32 {
    // Truncate the 8-bit component to its high nibble, then expand for sampling.
    // Multiplying normalized values by 16 shifts thresholds on filtered copies.
    return floor(v * 255.0 / 16.0) / 15.0;
}
)"sv;

std::string_view diagnostic_quantize4_wgsl() noexcept {
  const auto start = ShaderPreamble.find("fn quantize4(");
  const auto end = ShaderPreamble.find('}', start);
  return ShaderPreamble.substr(start, end - start + 1);
}

static const std::string DepthShaderPreamble = R"(
@group(0) @binding(0) var src: texture_depth_2d;

struct UVTransform {
    offset: vec2f,
    scale: vec2f,
};
@group(0) @binding(1) var<uniform> uv_xf: UVTransform;

struct VertexOutput {
    @builtin(position) pos: vec4f,
    @location(0) uv: vec2f,
};

var<private> positions: array<vec2f, 3> = array(
    vec2f(-1.0, 1.0),
    vec2f(-1.0, -3.0),
    vec2f(3.0, 1.0),
);
var<private> uvs: array<vec2f, 3> = array(
    vec2f(0.0, 0.0),
    vec2f(0.0, 2.0),
    vec2f(2.0, 0.0),
);

@vertex fn vs_main(@builtin(vertex_index) vi: u32) -> VertexOutput {
    var out: VertexOutput;
    out.pos = vec4f(positions[vi], 0.0, 1.0);
    out.uv = uvs[vi] * uv_xf.scale + uv_xf.offset;
    return out;
}
)"s + (gx::UseReversedZ ? R"(
fn gx_z24(uv: vec2f) -> u32 {
    let texSize = vec2i(textureDimensions(src));
    let coord = clamp(vec2i(floor(uv * vec2f(texSize))), vec2i(0), texSize - vec2i(1));
    let depth = textureLoad(src, coord, 0);
    return min(u32(clamp(1.0 - depth, 0.0, 1.0) * 16777215.0 + 0.5), 0x00ffffffu);
}
)"s
                        : R"(
fn gx_z24(uv: vec2f) -> u32 {
    let texSize = vec2i(textureDimensions(src));
    let coord = clamp(vec2i(floor(uv * vec2f(texSize))), vec2i(0), texSize - vec2i(1));
    let depth = textureLoad(src, coord, 0);
    return min(u32(clamp(depth, 0.0, 1.0) * 16777215.0 + 0.5), 0x00ffffffu);
}
)"s);

// Passthrough blit (for scaling)
static constexpr std::string_view FragPassthrough = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return textureSample(src, src_samp, in.uv);
}
)"sv;

// Independently implemented progressive display filter. Seven SDK taps collapse
// to the preceding/current/following EFB rows for a non-AA source. RGB uses
// integer six-bit weights; the supported RGB8 EFB has opaque alpha.
static constexpr std::string_view FragDisplayCopy = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let y0 = clamp(in.uv.y - uv_xf.rows.x, uv_xf.rows.y, uv_xf.rows.z);
    let y1 = clamp(in.uv.y, uv_xf.rows.y, uv_xf.rows.z);
    let y2 = clamp(in.uv.y + uv_xf.rows.x, uv_xf.rows.y, uv_xf.rows.z);
    let a = textureSampleLevel(src, src_samp, vec2f(in.uv.x, y0), 0.0);
    let b = textureSampleLevel(src, src_samp, vec2f(in.uv.x, y1), 0.0);
    let c = textureSampleLevel(src, src_samp, vec2f(in.uv.x, y2), 0.0);
    let sum = vec3u(a.rgb * 255.0) * uv_xf.weights.x
            + vec3u(b.rgb * 255.0) * uv_xf.weights.y
            + vec3u(c.rgb * 255.0) * uv_xf.weights.z;
    let filtered = min((sum >> vec3u(6u)) & vec3u(511u), vec3u(255u));
    let corrected = round(pow(vec3f(filtered) / 255.0, vec3f(uv_xf.rows.w)) * 255.0);
    return vec4f(corrected / 255.0, 1.0);
}
)"sv;

// GX_TF_I4: 4-bit intensity -> R8Unorm (quantized)
static constexpr std::string_view FragI4 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let rgb = textureSample(src, src_samp, in.uv).rgb;
    let i = quantize4(intensity(rgb));
    return vec4f(i, i, i, i);
}
)"sv;

// GX_TF_I8: 8-bit intensity -> R8Unorm
static constexpr std::string_view FragI8 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let rgb = textureSample(src, src_samp, in.uv).rgb;
    let i = intensity(rgb);
    return vec4f(i, i, i, i);
}
)"sv;

// GX_TF_IA4: 4-bit intensity + 4-bit alpha -> RG8Unorm
static constexpr std::string_view FragIA4 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let c = textureSample(src, src_samp, in.uv);
    let i = quantize4(intensity(c.rgb));
    let a = quantize4(c.a);
    return vec4f(i, i, i, a);
}
)"sv;

// GX_TF_IA8: 8-bit intensity + 8-bit alpha -> RG8Unorm
static constexpr std::string_view FragIA8 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let c = textureSample(src, src_samp, in.uv);
    let i = intensity(c.rgb);
    return vec4f(i, i, i, c.a);
}
)"sv;

// GX_TF_RGB565: Blit alpha to 1.0
static constexpr std::string_view FragRGB565 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let c = textureSample(src, src_samp, in.uv);
    return vec4f(c.rgb, 1.0);
}
)"sv;

// GX_CTF_R4: 4-bit red -> R8Unorm
static constexpr std::string_view FragR4 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let r = quantize4(textureSample(src, src_samp, in.uv).r);
    return vec4f(r, r, r, r);
}
)"sv;

// GX_CTF_RA4: 4-bit red + 4-bit alpha -> RG8Unorm
static constexpr std::string_view FragRA4 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let c = textureSample(src, src_samp, in.uv);
    let r = quantize4(c.r);
    return vec4f(r, r, r, quantize4(c.a));
}
)"sv;

// GX_CTF_RA8: 8-bit red + 8-bit alpha -> RG8Unorm
static constexpr std::string_view FragRA8 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let c = textureSample(src, src_samp, in.uv);
    return vec4f(c.r, c.r, c.r, c.a);
}
)"sv;

// GX_CTF_A8: 8-bit alpha -> R8Unorm
static constexpr std::string_view FragA8 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let a = textureSample(src, src_samp, in.uv).a;
    return vec4f(a, a, a, a);
}
)"sv;

// GX_CTF_R8: 8-bit red -> R8Unorm
static constexpr std::string_view FragR8 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let r = textureSample(src, src_samp, in.uv).r;
    return vec4f(r, r, r, r);
}
)"sv;

// GX_CTF_G8: 8-bit green -> R8Unorm
static constexpr std::string_view FragG8 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let g = textureSample(src, src_samp, in.uv).g;
    return vec4f(g, g, g, g);
}
)"sv;

// GX_CTF_B8: 8-bit blue -> R8Unorm
static constexpr std::string_view FragB8 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let b = textureSample(src, src_samp, in.uv).b;
    return vec4f(b, b, b, b);
}
)"sv;

// GX_CTF_RG8: 8-bit red + 8-bit green -> RG8Unorm
static constexpr std::string_view FragRG8 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let c = textureSample(src, src_samp, in.uv);
    return vec4f(c.r, c.r, c.r, c.g);
}
)"sv;

// GX_CTF_GB8: 8-bit green + 8-bit blue -> RG8Unorm
static constexpr std::string_view FragGB8 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let c = textureSample(src, src_samp, in.uv);
    return vec4f(c.g, c.g, c.g, c.b);
}
)"sv;

// GX_TF_Z8: Upper 8-bits depth -> I8
static constexpr std::string_view FragZ8 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let z8 = f32((gx_z24(in.uv) >> 16u) & 0xFFu) / 255.0;
    return vec4f(z8, z8, z8, z8);
}
)"sv;

// GX_TF_Z16: Upper 16-bits depth -> IA8
static constexpr std::string_view FragZ16 = R"(
@fragment fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let z16 = gx_z24(in.uv) >> 8u;
    let i = f32((z16 >> 8u) & 0xFFu) / 255.0;
    let a = f32(z16 & 0xFFu) / 255.0;
    return vec4f(i, i, i, a);
}
)"sv;

// Depth -> R32Float (no scaling)
static constexpr std::string_view DepthSnapshotShader = R"(
@group(0) @binding(0) var src: texture_depth_2d;

var<private> positions: array<vec2f, 3> = array(
    vec2f(-1.0, 1.0),
    vec2f(-1.0, -3.0),
    vec2f(3.0, 1.0),
);

@vertex fn vs_main(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {
    return vec4f(positions[vi], 0.0, 1.0);
}

@fragment fn fs_main(@builtin(position) pos: vec4f) -> @location(0) vec4f {
    let depth = textureLoad(src, vec2i(pos.xy), 0);
    return vec4f(depth, 0.0, 0.0, 1.0);
}
)"sv;

static constexpr std::string_view DepthSnapshotShaderMS = R"(
@group(0) @binding(0) var src: texture_depth_multisampled_2d;

var<private> positions: array<vec2f, 3> = array(
    vec2f(-1.0, 1.0),
    vec2f(-1.0, -3.0),
    vec2f(3.0, 1.0),
);

@vertex fn vs_main(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {
    return vec4f(positions[vi], 0.0, 1.0);
}

@fragment fn fs_main(@builtin(position) pos: vec4f) -> @location(0) vec4f {
    let depth = textureLoad(src, vec2i(pos.xy), 0);
    return vec4f(depth, 0.0, 0.0, 1.0);
}
)"sv;

struct ConvPipeline {
  GXTexFmt fmt;
  std::string_view fragShader;
  wgpu::TextureFormat outputFormat;
  const char* label;
};

static constexpr std::array ConvPipelines{
    ConvPipeline{GX_TF_I4, FragI4, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv I4"},
    ConvPipeline{GX_TF_I8, FragI8, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv I8"},
    ConvPipeline{GX_TF_IA4, FragIA4, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv IA4"},
    ConvPipeline{GX_TF_IA8, FragIA8, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv IA8"},
    ConvPipeline{GX_TF_RGB565, FragRGB565, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv RGB565"},
    ConvPipeline{GX_CTF_R4, FragR4, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv R4"},
    ConvPipeline{GX_CTF_RA4, FragRA4, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv RA4"},
    ConvPipeline{GX_CTF_RA8, FragRA8, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv RA8"},
    ConvPipeline{GX_CTF_A8, FragA8, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv A8"},
    ConvPipeline{GX_CTF_R8, FragR8, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv R8"},
    ConvPipeline{GX_CTF_G8, FragG8, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv G8"},
    ConvPipeline{GX_CTF_B8, FragB8, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv B8"},
    ConvPipeline{GX_CTF_RG8, FragRG8, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv RG8"},
    ConvPipeline{GX_CTF_GB8, FragGB8, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv GB8"},
};

static constexpr std::array DepthConvPipelines{
    ConvPipeline{GX_TF_Z8, FragZ8, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv Z8"},
    ConvPipeline{GX_TF_Z16, FragZ16, wgpu::TextureFormat::RGBA8Unorm, "TexCopyConv Z16"},
};

static wgpu::BindGroupLayout g_bindGroupLayout;
static wgpu::BindGroupLayout g_depthBindGroupLayout;
static wgpu::Sampler g_nearestSampler;
static wgpu::Sampler g_linearSampler;
static absl::flat_hash_map<GXTexFmt, wgpu::RenderPipeline> g_pipelines;
static wgpu::RenderPipeline g_blitPipeline;
static wgpu::RenderPipeline g_displayCopyPipeline;
static wgpu::BindGroupLayout g_depthSnapshotBindGroupLayout;
static wgpu::BindGroupLayout g_depthSnapshotBindGroupLayoutMS;
static wgpu::RenderPipeline g_depthSnapshotPipeline;
static wgpu::RenderPipeline g_depthSnapshotPipelineMS;

static wgpu::BindGroupLayout create_depth_snapshot_layout(bool multisampled, const char* label) {
  const std::array entries{
      wgpu::BindGroupLayoutEntry{
          .binding = 0,
          .visibility = wgpu::ShaderStage::Fragment,
          .texture =
              wgpu::TextureBindingLayout{
                  .sampleType = wgpu::TextureSampleType::Depth,
                  .viewDimension = wgpu::TextureViewDimension::e2D,
                  .multisampled = multisampled,
              },
      },
  };
  const wgpu::BindGroupLayoutDescriptor descriptor{
      .label = label,
      .entryCount = entries.size(),
      .entries = entries.data(),
  };
  return g_device.CreateBindGroupLayout(&descriptor);
}

static wgpu::RenderPipeline create_depth_snapshot_pipeline(const std::string_view shaderSource,
                                                           const wgpu::BindGroupLayout& bindGroupLayout,
                                                           const char* label) {
  const std::string source{shaderSource};
  const wgpu::ShaderSourceWGSL wgslSource{wgpu::ShaderSourceWGSL::Init{
      .code = source.c_str(),
  }};
  const wgpu::ShaderModuleDescriptor moduleDescriptor{
      .nextInChain = &wgslSource,
      .label = label,
  };
  const auto module = g_device.CreateShaderModule(&moduleDescriptor);

  const std::array colorTargets{wgpu::ColorTargetState{
      .format = wgpu::TextureFormat::R32Float,
  }};
  const wgpu::FragmentState fragmentState{
      .module = module,
      .entryPoint = "fs_main",
      .targetCount = colorTargets.size(),
      .targets = colorTargets.data(),
  };
  const wgpu::PipelineLayoutDescriptor layoutDescriptor{
      .bindGroupLayoutCount = 1,
      .bindGroupLayouts = &bindGroupLayout,
  };
  const auto pipelineLayout = g_device.CreatePipelineLayout(&layoutDescriptor);
  const wgpu::RenderPipelineDescriptor pipelineDescriptor{
      .label = label,
      .layout = pipelineLayout,
      .vertex =
          wgpu::VertexState{
              .module = module,
              .entryPoint = "vs_main",
          },
      .primitive =
          wgpu::PrimitiveState{
              .topology = wgpu::PrimitiveTopology::TriangleList,
          },
      .fragment = &fragmentState,
  };
  return g_device.CreateRenderPipeline(&pipelineDescriptor);
}

static wgpu::RenderPipeline create_pipeline(const ConvPipeline& conv, const std::string_view shaderPreamble,
                                            const wgpu::BindGroupLayout& bindGroupLayout) {
  std::string shaderSource;
  shaderSource.reserve(shaderPreamble.size() + conv.fragShader.size());
  shaderSource += shaderPreamble;
  shaderSource += conv.fragShader;

  const wgpu::ShaderSourceWGSL wgslSource{wgpu::ShaderSourceWGSL::Init{
      .code = shaderSource.c_str(),
  }};
  const wgpu::ShaderModuleDescriptor moduleDescriptor{
      .nextInChain = &wgslSource,
      .label = conv.label,
  };
  const auto module = g_device.CreateShaderModule(&moduleDescriptor);

  const std::array colorTargets{wgpu::ColorTargetState{
      .format = conv.outputFormat,
  }};
  const wgpu::FragmentState fragmentState{
      .module = module,
      .entryPoint = "fs_main",
      .targetCount = colorTargets.size(),
      .targets = colorTargets.data(),
  };

  const wgpu::PipelineLayoutDescriptor layoutDescriptor{
      .bindGroupLayoutCount = 1,
      .bindGroupLayouts = &bindGroupLayout,
  };
  const auto pipelineLayout = g_device.CreatePipelineLayout(&layoutDescriptor);

  const wgpu::RenderPipelineDescriptor pipelineDescriptor{
      .label = conv.label,
      .layout = pipelineLayout,
      .vertex =
          wgpu::VertexState{
              .module = module,
              .entryPoint = "vs_main",
          },
      .primitive =
          wgpu::PrimitiveState{
              .topology = wgpu::PrimitiveTopology::TriangleList,
          },
      .fragment = &fragmentState,
  };
  return g_device.CreateRenderPipeline(&pipelineDescriptor);
}

bool needs_conversion(const GXTexFmt fmt) { return g_pipelines.contains(fmt); }

void initialize() {
  static constexpr std::array bindGroupLayoutEntries{
      wgpu::BindGroupLayoutEntry{
          .binding = 0,
          .visibility = wgpu::ShaderStage::Fragment,
          .sampler =
              wgpu::SamplerBindingLayout{
                  .type = wgpu::SamplerBindingType::Filtering,
              },
      },
      wgpu::BindGroupLayoutEntry{
          .binding = 1,
          .visibility = wgpu::ShaderStage::Fragment,
          .texture =
              wgpu::TextureBindingLayout{
                  .sampleType = wgpu::TextureSampleType::Float,
                  .viewDimension = wgpu::TextureViewDimension::e2D,
              },
      },
      wgpu::BindGroupLayoutEntry{
          .binding = 2,
          .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
          .buffer =
              wgpu::BufferBindingLayout{
                  .type = wgpu::BufferBindingType::Uniform,
              },
      },
  };
  static constexpr wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor{
      .label = "TexCopyConv Bind Group Layout",
      .entryCount = bindGroupLayoutEntries.size(),
      .entries = bindGroupLayoutEntries.data(),
  };
  g_bindGroupLayout = g_device.CreateBindGroupLayout(&bindGroupLayoutDescriptor);

  static constexpr std::array depthBindGroupLayoutEntries{
      wgpu::BindGroupLayoutEntry{
          .binding = 0,
          .visibility = wgpu::ShaderStage::Fragment,
          .texture =
              wgpu::TextureBindingLayout{
                  .sampleType = wgpu::TextureSampleType::Depth,
                  .viewDimension = wgpu::TextureViewDimension::e2D,
              },
      },
      wgpu::BindGroupLayoutEntry{
          .binding = 1,
          .visibility = wgpu::ShaderStage::Vertex,
          .buffer =
              wgpu::BufferBindingLayout{
                  .type = wgpu::BufferBindingType::Uniform,
              },
      },
  };
  static constexpr wgpu::BindGroupLayoutDescriptor depthBindGroupLayoutDescriptor{
      .label = "TexCopyConv Depth Bind Group Layout",
      .entryCount = depthBindGroupLayoutEntries.size(),
      .entries = depthBindGroupLayoutEntries.data(),
  };
  g_depthBindGroupLayout = g_device.CreateBindGroupLayout(&depthBindGroupLayoutDescriptor);

  g_blitPipeline = create_pipeline(
      {GX_TF_RGBA8, FragPassthrough, webgpu::g_graphicsConfig.surfaceConfiguration.format, "TexCopyConv Blit"},
      ShaderPreamble, g_bindGroupLayout);
  std::string displayPreamble{ShaderPreamble};
  const std::string uvStruct = "    scale: vec2f,\n";
  const auto uvPosition = displayPreamble.find(uvStruct);
  AURORA_ASSERT(uvPosition != std::string::npos, "Display-copy uniform layout missing");
  displayPreamble.insert(uvPosition + uvStruct.size(), "    rows: vec4f,\n    weights: vec4u,\n");
  g_displayCopyPipeline = create_pipeline(
      ConvPipeline{.fmt = GX_TF_RGBA8, .fragShader = FragDisplayCopy,
                   .outputFormat = webgpu::g_graphicsConfig.surfaceConfiguration.format, .label = "Display Copy RGB8"},
      displayPreamble, g_bindGroupLayout);
  for (const auto& conv : ConvPipelines) {
    g_pipelines[conv.fmt] = create_pipeline(conv, ShaderPreamble, g_bindGroupLayout);
    if (conv.outputFormat != to_wgpu(conv.fmt)) {
      Log.fatal("Output format mismatch for {}", conv.fmt);
    }
  }
  // Skip depth copies in compatibility mode
  if (webgpu::g_hasCoreFeatures) {
    for (const auto& conv : DepthConvPipelines) {
      g_pipelines[conv.fmt] = create_pipeline(conv, DepthShaderPreamble, g_depthBindGroupLayout);
      if (conv.outputFormat != to_wgpu(conv.fmt)) {
        Log.fatal("Output format mismatch for {}", conv.fmt);
      }
    }
    g_depthSnapshotBindGroupLayout = create_depth_snapshot_layout(false, "Depth Snapshot Bind Group Layout");
    g_depthSnapshotBindGroupLayoutMS = create_depth_snapshot_layout(true, "Depth Snapshot MS Bind Group Layout");
    g_depthSnapshotPipeline =
        create_depth_snapshot_pipeline(DepthSnapshotShader, g_depthSnapshotBindGroupLayout, "Depth Snapshot");
    g_depthSnapshotPipelineMS =
        create_depth_snapshot_pipeline(DepthSnapshotShaderMS, g_depthSnapshotBindGroupLayoutMS, "Depth Snapshot MS");
  }

  static constexpr wgpu::SamplerDescriptor nearestSamplerDescriptor{
      .label = "TexCopyConv Nearest Sampler",
      .magFilter = wgpu::FilterMode::Nearest,
      .minFilter = wgpu::FilterMode::Nearest,
  };
  g_nearestSampler = g_device.CreateSampler(&nearestSamplerDescriptor);

  static constexpr wgpu::SamplerDescriptor linearSamplerDescriptor{
      .label = "TexCopyConv Linear Sampler",
      .magFilter = wgpu::FilterMode::Linear,
      .minFilter = wgpu::FilterMode::Linear,
  };
  g_linearSampler = g_device.CreateSampler(&linearSamplerDescriptor);
}

void shutdown() {
  g_pipelines.clear();
  g_blitPipeline = {};
  g_displayCopyPipeline = {};
  g_bindGroupLayout = {};
  g_depthBindGroupLayout = {};
  g_nearestSampler = {};
  g_linearSampler = {};
  g_depthSnapshotBindGroupLayout = {};
  g_depthSnapshotBindGroupLayoutMS = {};
  g_depthSnapshotPipeline = {};
  g_depthSnapshotPipelineMS = {};
}

static void execute(const wgpu::CommandEncoder& cmd, const ConvRequest& req, const wgpu::RenderPipeline& pipeline,
                    const wgpu::Buffer* diagnosticUniform = nullptr) {
  if (!pipeline) {
    return;
  }
  wgpu::BindGroup bindGroup;
  if (gx::is_depth_format(req.fmt)) {
    // Skip depth copies in compatibility mode
    if (!webgpu::g_hasCoreFeatures) {
      return;
    }
    const std::array bindGroupEntries{
        wgpu::BindGroupEntry{
            .binding = 0,
            .textureView = req.srcView,
        },
        wgpu::BindGroupEntry{
            .binding = 1,
            .buffer = diagnosticUniform ? *diagnosticUniform : detail::resources().uniformBuffer,
            .offset = req.uniformRange.offset,
            .size = req.uniformRange.size,
        },
    };
    const wgpu::BindGroupDescriptor bindGroupDescriptor{
        .layout = g_depthBindGroupLayout,
        .entryCount = bindGroupEntries.size(),
        .entries = bindGroupEntries.data(),
    };
    bindGroup = g_device.CreateBindGroup(&bindGroupDescriptor);
  } else {
    const auto& sampler = req.sampleFilter == SampleFilter::Linear ? g_linearSampler : g_nearestSampler;
    const std::array bindGroupEntries{
        wgpu::BindGroupEntry{
            .binding = 0,
            .sampler = sampler,
        },
        wgpu::BindGroupEntry{
            .binding = 1,
            .textureView = req.srcView,
        },
        wgpu::BindGroupEntry{
            .binding = 2,
            .buffer = diagnosticUniform ? *diagnosticUniform : detail::resources().uniformBuffer,
            .offset = req.uniformRange.offset,
            .size = req.uniformRange.size,
        },
    };
    const wgpu::BindGroupDescriptor bindGroupDescriptor{
        .layout = g_bindGroupLayout,
        .entryCount = bindGroupEntries.size(),
        .entries = bindGroupEntries.data(),
    };
    bindGroup = g_device.CreateBindGroup(&bindGroupDescriptor);
  }

  const std::array colorAttachments{
      wgpu::RenderPassColorAttachment{
          .view = req.dst->attachmentTextureView,
          .loadOp = wgpu::LoadOp::Clear,
          .storeOp = wgpu::StoreOp::Store,
          .clearValue = {0.0, 0.0, 0.0, 0.0},
      },
  };
  const wgpu::RenderPassDescriptor renderPassDescriptor{
      .label = "TexCopyConv Pass",
      .colorAttachmentCount = colorAttachments.size(),
      .colorAttachments = colorAttachments.data(),
      .timestampWrites = webgpu::gpu_prof::pass_writes("EFB copy convert"),
  };
  const auto pass = cmd.BeginRenderPass(&renderPassDescriptor);
  pass.SetPipeline(pipeline);
  pass.SetBindGroup(0, bindGroup);
  pass.Draw(3);
  pass.End();
}

void run(const wgpu::CommandEncoder& cmd, const ConvRequest& req) {
  const auto it = g_pipelines.find(req.fmt);
  if (it == g_pipelines.end()) {
    Log.fatal("No copy conversion pipeline for format {}", static_cast<int>(req.fmt));
  }
  execute(cmd, req, it->second);
}

void blit(const wgpu::CommandEncoder& cmd, const ConvRequest& req) { execute(cmd, req, g_blitPipeline); }
void diagnostic_run_r4(const wgpu::CommandEncoder& cmd, const ConvRequest& req, const wgpu::Buffer& uniform) {
  AURORA_ASSERT(std::getenv("AURORA_DIAGNOSTIC_R4_COPY_PIXELS") && req.fmt == GX_CTF_R4 &&
                req.uniformRange.offset == 0 && req.uniformRange.size == 16 && uniform,
                "Invalid diagnostic R4 copy request");
  execute(cmd, req, g_pipelines.at(GX_CTF_R4), &uniform);
}

void display_copy(const wgpu::CommandEncoder& cmd, const ConvRequest& req) {
  AURORA_ASSERT(req.displayCopy && req.fmt == GX_TF_RGBA8 && req.uniformRange.size == 48 &&
                    req.dst->format == webgpu::g_graphicsConfig.surfaceConfiguration.format && g_displayCopyPipeline,
                "Invalid progressive display-copy request");
  execute(cmd, req, g_displayCopyPipeline);
}

bool snapshot_depth_supported() noexcept { return static_cast<bool>(g_depthSnapshotPipeline); }

void snapshot_depth(const wgpu::CommandEncoder& cmd, const wgpu::TextureView& srcDepth, uint32_t msaaSamples,
                    const wgpu::TextureView& dst) {
  const bool multisampled = msaaSamples > 1;
  const auto& pipeline = multisampled ? g_depthSnapshotPipelineMS : g_depthSnapshotPipeline;
  if (!pipeline) {
    return;
  }
  const std::array bindGroupEntries{
      wgpu::BindGroupEntry{
          .binding = 0,
          .textureView = srcDepth,
      },
  };
  const wgpu::BindGroupDescriptor bindGroupDescriptor{
      .layout = multisampled ? g_depthSnapshotBindGroupLayoutMS : g_depthSnapshotBindGroupLayout,
      .entryCount = bindGroupEntries.size(),
      .entries = bindGroupEntries.data(),
  };
  const auto bindGroup = g_device.CreateBindGroup(&bindGroupDescriptor);

  const std::array colorAttachments{
      wgpu::RenderPassColorAttachment{
          .view = dst,
          .loadOp = wgpu::LoadOp::Clear,
          .storeOp = wgpu::StoreOp::Store,
          .clearValue = {0.0, 0.0, 0.0, 0.0},
      },
  };
  const wgpu::RenderPassDescriptor renderPassDescriptor{
      .label = "Depth Snapshot Pass",
      .colorAttachmentCount = colorAttachments.size(),
      .colorAttachments = colorAttachments.data(),
      .timestampWrites = webgpu::gpu_prof::pass_writes("Depth snapshot"),
  };
  const auto pass = cmd.BeginRenderPass(&renderPassDescriptor);
  pass.SetPipeline(pipeline);
  pass.SetBindGroup(0, bindGroup);
  pass.Draw(3);
  pass.End();
}

namespace {
struct PatternCopy {
  TextureHandle source;
  TextureHandle atlas;
  std::array<TextureHandle, 16> destinations;
  std::array<Range, 16> uniforms;
  uint64_t frameId;
  bool resample = false;
  std::array<TextureHandle,16> filtered;
  std::array<Range,16> filterUniforms;
};
void encode_pattern(const EncoderTaskContext&, const wgpu::CommandEncoder& cmd,
                    const void* payload, size_t length, void*) {
  AURORA_ASSERT(length == sizeof(PatternCopy*), "Invalid pattern payload");
  PatternCopy* pattern;
  std::memcpy(&pattern, payload, sizeof(pattern));
  for (unsigned i = 0; i < 16; ++i) {
    if (pattern->resample) {
      if (pattern->filtered[i]) display_copy(cmd,ConvRequest{.fmt=GX_TF_RGBA8,
        .srcView=pattern->source->sampleTextureView,.uniformRange=pattern->filterUniforms[i],
        .dst=pattern->filtered[i],.sampleFilter=SampleFilter::Linear,.displayCopy=true});
      blit(cmd, ConvRequest{.fmt=GX_TF_RGBA8, .srcView=(pattern->filtered[i]?pattern->filtered[i]:pattern->source)->sampleTextureView,
           .uniformRange=pattern->uniforms[i], .dst=pattern->destinations[i],
           .sampleFilter=SampleFilter::Linear});
    } else display_copy(cmd, ConvRequest{.fmt = GX_TF_RGBA8,
                  .srcView = pattern->source->sampleTextureView,
                  .uniformRange = pattern->uniforms[i], .dst = pattern->destinations[i],
                  .sampleFilter = SampleFilter::Nearest, .displayCopy = true});
    const wgpu::TexelCopyTextureInfo source{.texture = pattern->destinations[i]->texture};
    const wgpu::TexelCopyTextureInfo destination{
        .texture = pattern->atlas->texture, .origin = {.x = i * 16, .y = 0}};
    const wgpu::Extent3D extent{16, 16, 1};
    cmd.CopyTextureToTexture(&source, &destination, &extent);
  }
  diagnostic_capture::encode(cmd, pattern->atlas->texture, pattern->frameId);
}
void submitted_pattern(const EncoderTaskCompletionContext&, const void* payload, size_t, void*) {
  PatternCopy* pattern;
  std::memcpy(&pattern, payload, sizeof(pattern));
  delete pattern; // Submitted command buffer and readback retain their GPU resources.
}
}

int diagnostic_pattern(uint64_t frameId) {
  if (!aurora_diagnostic_is_owner() || frameId < 20 ||
      frameId != aurora_diagnostic_recording_id()) return 0;
  static bool requested = false; // Isolated one-shot process diagnostic, not runtime restart support.
  if (requested) return 0;
  auto pattern = std::make_unique<PatternCopy>();
  pattern->frameId = frameId;
  const char* resample = std::getenv("AURORA_DIAGNOSTIC_SCALED_XFB_PATTERN");
  if (resample) {
    if (std::strcmp(resample, "8") && std::strcmp(resample, "24")) return 0;
    const unsigned size = unsigned(std::strtoul(resample, nullptr, 10));
    pattern->resample = true;
    pattern->source = new_render_texture(size, size, GX_TF_RGBA8, "Synthetic scaled XFB source");
    pattern->atlas = new_render_texture(256, 16, GX_TF_RGBA8, "Scaled XFB pixel atlas");
    const bool bgra = pattern->source->format == wgpu::TextureFormat::BGRA8Unorm;
    if (!bgra && pattern->source->format != wgpu::TextureFormat::RGBA8Unorm) return 0;
    std::vector<uint8_t> pixels(size * size * 4);
    for (unsigned y=0; y<size; ++y) for (unsigned x=0; x<size; ++x) {
      const unsigned at=(y*size+x)*4;
      const uint8_t r=(x*37+y*11)&255, g=(x*13+y*53)&255, b=(x*71+y*7)&255;
      pixels[at]=bgra?b:r; pixels[at+1]=g; pixels[at+2]=bgra?r:b; pixels[at+3]=255;
    }
    queue_texture_upload_data(pixels.data(),size*4,size,
      wgpu::TexelCopyTextureInfo{.texture=pattern->source->texture},pattern->source->size);
    for (unsigned i=0;i<16;++i) {
      pattern->destinations[i]=new_render_texture(16,16,GX_TF_RGBA8,"Synthetic logical XFB");
      pattern->uniforms[i]=push_uniform(std::array<float,4>{0,0,1,1});
      if (std::getenv("AURORA_DIAGNOSTIC_SCALED_XFB_FILTER")) {
        DisplayCopyOptions options; options.enabled=true; options.clampTop=options.clampBottom=true;
        options.rowWeights={16,32,16}; options.logicalRowStep=float(size)/16.f;
        options.gammaReciprocal=i%3==1?1.f/1.7f:i%3==2?1.f/2.2f:1.f;
        pattern->filtered[i]=new_render_texture(size,size,GX_TF_RGBA8,"Filtered scaled XFB");
        pattern->filterUniforms[i]=push_uniform(display_copy_uniform(options,0,0,float(size),float(size),float(size),float(size)));
      }
    }
    const auto task=register_encoder_task_type(EncoderTaskDescriptor{
      .label="Scaled XFB pixel oracle",.callback=encode_pattern,.afterSubmit=submitted_pattern});
    PatternCopy* pointer=pattern.get();
    if (!push_encoder_task(task,&pointer,sizeof(pointer))) { unregister_encoder_task_type(task); return 0; }
    requested=true; pattern.release();
    std::fprintf(stderr,"SCALED_XFB_PATTERN source=%u target=16 filter=linear shader=production_blit\n",size);
    return 1;
  }
  pattern->source = new_render_texture(16, 32, GX_TF_RGBA8, "Display filter pattern");
  pattern->atlas = new_render_texture(256, 16, GX_TF_RGBA8, "Display filter readback atlas");
  const bool bgra = pattern->source->format == wgpu::TextureFormat::BGRA8Unorm;
  if (!bgra && pattern->source->format != wgpu::TextureFormat::RGBA8Unorm) return 0;
  std::array<uint8_t, 16 * 32 * 4> pixels{};
  for (unsigned y = 0; y < 32; ++y) for (unsigned x = 0; x < 16; ++x) {
    const unsigned offset = (y * 16 + x) * 4;
    const uint8_t r = x == 0 ? (y & 1 ? 255 : 0) : (y * 37 + x * 11) & 255;
    const uint8_t g = (y * 19 + x * 53) & 255;
    const uint8_t b = (y * 71 + x * 7) & 255;
    pixels[offset] = bgra ? b : r;
    pixels[offset + 1] = g;
    pixels[offset + 2] = bgra ? r : b;
    pixels[offset + 3] = (y * 9 + x * 3) & 255; // RGB8 shader must output opaque alpha.
  }
  queue_texture_upload_data(pixels.data(), 16 * 4, 32,
      wgpu::TexelCopyTextureInfo{.texture = pattern->source->texture}, pattern->source->size);
  for (unsigned i = 0; i < 16; ++i) {
    DisplayCopyOptions options;
    options.enabled = true;
    std::array<uint32_t, 7> taps{0, 0, 21, 22, 21, 0, 0};
    if (i < 7) { taps.fill(0); taps[i] = 63; }
    else if (i == 8 || i == 12) taps = {0, 16, 16, 16, 0, 16, 0};
    else if (i == 9) taps.fill(63);
    else if (i == 10) { taps = {63, 0, 0, 0, 0, 0, 0}; options.clampTop = true; }
    else if (i == 11) { taps = {0, 0, 0, 0, 0, 0, 63}; options.clampBottom = true; }
    else if (i == 15) taps.fill(0);
    uint32_t first = 0, second = 0;
    for (unsigned tap = 0; tap < 4; ++tap) first |= taps[tap] << (tap * 6);
    for (unsigned tap = 4; tap < 7; ++tap) second |= taps[tap] << ((tap - 4) * 6);
    options.rowWeights = display_copy_weights(first, second);
    if (i == 12) options.clampTop = options.clampBottom = true;
    if (i == 13) options.gammaReciprocal = 1.f / 1.7f;
    if (i == 14) options.gammaReciprocal = 1.f / 2.2f;
    pattern->destinations[i] = new_render_texture(16, 16, GX_TF_RGBA8, "Display filter case");
    pattern->uniforms[i] = push_uniform(display_copy_uniform(options, 0, 8, 16, 16, 16, 32));
  }
  const auto task = register_encoder_task_type(EncoderTaskDescriptor{
      .label = "Display filter pixel tests", .callback = encode_pattern, .afterSubmit = submitted_pattern});
  PatternCopy* pointer = pattern.get();
  if (!push_encoder_task(task, &pointer, sizeof(pointer))) {
    unregister_encoder_task_type(task);
    return 0;
  }
  requested = true;
  pattern.release();
  return 1;
}

} // namespace aurora::gfx::tex_copy_conv

extern "C" int aurora_diagnostic_display_filter_pattern(uint64_t frameId) {
  return aurora::gfx::tex_copy_conv::diagnostic_pattern(frameId);
}
