#include "gx.hpp"
#include <aurora/diagnostics.h>
#include "__gx.h"

#include "../../gfx/tex_copy_conv.hpp"
#include "../../gfx/texture.hpp"
#include "../../gfx/recording.hpp"
#include "../../gfx/render_snapshot.hpp"
#include "../../window.hpp"
#include "../../gfx/clear.hpp"
#include "../../webgpu/gpu.hpp"
#include "../../gx/texture.hpp"
#include "../vi/vi_internal.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {
struct XfbCopyProfile {
  bool checked = false;
  bool enabled = false;
  unsigned calls = 0;
  uint64_t fifoNs = 0, fifoMaxNs = 0;
  uint64_t encodeNs = 0, encodeMaxNs = 0;
} xfbCopyProfile;

struct XfbCopyStageProfile {
  unsigned calls = 0;
  uint64_t setupNs = 0, alphaNs = 0, resolveNs = 0, publishNs = 0;
} xfbCopyStageProfile;

struct XfbCopyStageTrace {
  bool checked = false;
  bool enabled = false;
  unsigned calls = 0;
} xfbCopyStageTrace;

uint64_t monotonic_ns() {
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
}

bool xfb_copy_profile_enabled() {
  if (!xfbCopyProfile.checked) {
    xfbCopyProfile.enabled = std::getenv("MELEE_DIAGNOSTIC_PACING_PROFILE") != nullptr;
    xfbCopyProfile.checked = true;
  }
  return xfbCopyProfile.enabled;
}

void record_xfb_copy_profile(uint64_t fifoNs, uint64_t encodeNs) {
  if (!xfb_copy_profile_enabled()) return;
  ++xfbCopyProfile.calls;
  xfbCopyProfile.fifoNs += fifoNs;
  xfbCopyProfile.fifoMaxNs = std::max(xfbCopyProfile.fifoMaxNs, fifoNs);
  xfbCopyProfile.encodeNs += encodeNs;
  xfbCopyProfile.encodeMaxNs = std::max(xfbCopyProfile.encodeMaxNs, encodeNs);
  if (xfbCopyProfile.calls != 60) return;
  std::fprintf(stderr,
      "NATIVE_XFB_COPY_PROFILE calls=%u fifo_total_us=%llu fifo_avg_us=%llu fifo_max_us=%llu "
      "encode_total_us=%llu encode_avg_us=%llu encode_max_us=%llu\n",
      xfbCopyProfile.calls,
      static_cast<unsigned long long>(xfbCopyProfile.fifoNs / 1000),
      static_cast<unsigned long long>(xfbCopyProfile.fifoNs / 1000 / xfbCopyProfile.calls),
      static_cast<unsigned long long>(xfbCopyProfile.fifoMaxNs / 1000),
      static_cast<unsigned long long>(xfbCopyProfile.encodeNs / 1000),
      static_cast<unsigned long long>(xfbCopyProfile.encodeNs / 1000 / xfbCopyProfile.calls),
      static_cast<unsigned long long>(xfbCopyProfile.encodeMaxNs / 1000));
  xfbCopyProfile.calls = 0;
  xfbCopyProfile.fifoNs = xfbCopyProfile.fifoMaxNs = 0;
  xfbCopyProfile.encodeNs = xfbCopyProfile.encodeMaxNs = 0;
}

void record_xfb_copy_stage_profile(uint64_t setupNs, uint64_t alphaNs,
                                   uint64_t resolveNs, uint64_t publishNs,
                                   uint32_t width, uint32_t height) {
  if (!xfbCopyStageTrace.checked) {
    xfbCopyStageTrace.enabled = std::getenv("AURORA_DIAGNOSTIC_XFB_COPY_STAGE_TRACE") != nullptr;
    xfbCopyStageTrace.checked = true;
  }
  if (xfbCopyStageTrace.enabled && xfbCopyStageTrace.calls < 12) {
    ++xfbCopyStageTrace.calls;
    std::fprintf(stderr,
        "NATIVE_XFB_COPY_STAGE call=%u width=%u height=%u setup_us=%llu "
        "alpha_us=%llu resolve_us=%llu publish_us=%llu\n",
        xfbCopyStageTrace.calls, width, height,
        static_cast<unsigned long long>(setupNs / 1000),
        static_cast<unsigned long long>(alphaNs / 1000),
        static_cast<unsigned long long>(resolveNs / 1000),
        static_cast<unsigned long long>(publishNs / 1000));
  }
  ++xfbCopyStageProfile.calls;
  xfbCopyStageProfile.setupNs += setupNs;
  xfbCopyStageProfile.alphaNs += alphaNs;
  xfbCopyStageProfile.resolveNs += resolveNs;
  xfbCopyStageProfile.publishNs += publishNs;
  if (xfbCopyStageProfile.calls != 60) return;
  std::fprintf(stderr,
      "NATIVE_XFB_COPY_STAGE_PROFILE calls=%u setup_avg_us=%llu alpha_avg_us=%llu "
      "resolve_avg_us=%llu publish_avg_us=%llu\n",
      xfbCopyStageProfile.calls,
      static_cast<unsigned long long>(xfbCopyStageProfile.setupNs / 1000 / xfbCopyStageProfile.calls),
      static_cast<unsigned long long>(xfbCopyStageProfile.alphaNs / 1000 / xfbCopyStageProfile.calls),
      static_cast<unsigned long long>(xfbCopyStageProfile.resolveNs / 1000 / xfbCopyStageProfile.calls),
      static_cast<unsigned long long>(xfbCopyStageProfile.publishNs / 1000 / xfbCopyStageProfile.calls));
  xfbCopyStageProfile = {};
}

// SDK display-copy register shadow. Actual XFB copy execution is still pending.
struct DisplayCopyState {
  u32 source = 0, size = 0, stride = 0, scale = 256, control = 0;
} displayCopy;
aurora::Vec2<uint32_t> scale_copy_dst(u32 logicalWidth, u32 logicalHeight) {
  if (g_gxState.viewportPolicy == AURORA_VIEWPORT_NATIVE) {
    return {logicalWidth, logicalHeight};
  }

  const auto [logicalFbWidth, logicalFbHeight] = aurora::gx::logical_fb_size();
  const auto [targetWidth, targetHeight] = aurora::gfx::get_render_target_size();
  if (logicalFbWidth == 0 || logicalFbHeight == 0 || targetWidth == 0 || targetHeight == 0) {
    return {logicalWidth, logicalHeight};
  }

  const float scaleX = static_cast<float>(targetWidth) / static_cast<float>(logicalFbWidth);
  const float scaleY = static_cast<float>(targetHeight) / static_cast<float>(logicalFbHeight);
  const auto scaledWidth = std::max<u32>(static_cast<u32>(std::lround(static_cast<float>(logicalWidth) * scaleX)), 1);
  const auto scaledHeight = std::max<u32>(static_cast<u32>(std::lround(static_cast<float>(logicalHeight) * scaleY)), 1);
  return {scaledWidth, scaledHeight};
}
} // namespace

namespace aurora::gx {
static void copy_tex_impl(const void* dest, GXBool clear, gfx::DisplayCopyOptions display = {}, std::shared_ptr<xfb::Job> job = {}) noexcept {
  const bool profileStages = job && xfb_copy_profile_enabled();
  uint64_t stageStart = profileStages ? monotonic_ns() : 0;
  const auto rect = map_logical_scissor(g_gxState.texCopySrc);
  const auto [dstWidth, dstHeight] = scale_copy_dst(g_gxState.texCopyDstWidth, g_gxState.texCopyDstHeight);
  const auto texCopyFmt = g_gxState.texCopyFmt;
  if(!job && texCopyFmt==GX_CTF_R4 && std::getenv("MELEE_DIAGNOSTIC_SHADOW_RESOLUTION")) {
    static unsigned lastWidth;
    if(lastWidth!=g_gxState.texCopyDstWidth) {
      lastWidth=g_gxState.texCopyDstWidth;
      const auto target=gfx::get_render_target_size();
      std::fprintf(stderr,"NATIVE_SHADOW_COPY_TARGET logical=%u mapped_width=%u mapped_height=%u dst_width=%u dst_height=%u target_width=%u target_height=%u offscreen=%u\n",
          g_gxState.texCopyDstWidth,rect.width,rect.height,dstWidth,dstHeight,target.x,target.y,unsigned(gfx::is_offscreen()));
    }
  }
  if (!job && texCopyFmt == GX_TF_RGB565 &&
      std::getenv("MELEE_DIAGNOSTIC_REFRACTION_CLOAK")) {
    static unsigned records=0;
    if (++records<=16) {
      const auto target=gfx::get_render_target_size();
      std::fprintf(stderr,"REFRACTION_COPY_REQUEST count=%u logical_x=%d logical_y=%d logical_width=%d logical_height=%d mapped_x=%d mapped_y=%d mapped_width=%u mapped_height=%u dst_width=%u dst_height=%u target_width=%u target_height=%u policy=%u offscreen=%u\n",
                   records,g_gxState.texCopySrc.x,g_gxState.texCopySrc.y,g_gxState.texCopySrc.width,g_gxState.texCopySrc.height,
                   rect.x,rect.y,rect.width,rect.height,dstWidth,dstHeight,target.x,target.y,
                   unsigned(g_gxState.viewportPolicy),unsigned(gfx::is_offscreen()));
    }
  }


  const GXState::CopyTextureKey key{
      .dest = dest,
      .width = dstWidth,
      .height = dstHeight,
      .format = texCopyFmt,
  };
  auto it = g_gxState.copyTextureCache.find(key);
  if (it == g_gxState.copyTextureCache.end()) {
    gfx::TextureHandle handle;
    if (gfx::tex_copy_conv::needs_conversion(texCopyFmt)) {
      handle = gfx::new_conv_texture(dstWidth, dstHeight, texCopyFmt, "Copy Conv Texture");
    } else {
      // Configure the texture swizzle to use alpha 1.0 if targeting RGB565 or EFB doesn't have alpha
      const auto fmt = texCopyFmt == GX_TF_RGB565 || g_gxState.pixelFmt == GX_PF_RGB8_Z24 ||
                               g_gxState.pixelFmt == GX_PF_RGB565_Z16
                           ? GX_TF_RGB565
                           : GX_TF_RGBA8;
      handle = gfx::new_render_texture(dstWidth, dstHeight, fmt, "Resolved Texture");
    }
    it = g_gxState.copyTextureCache.emplace(key, GXState::CopyTextureRef{.handle = handle, .revision = 0}).first;
  }
  auto& handle = it->second;
  const uint64_t setupNs = profileStages ? monotonic_ns() - stageStart : 0;

  stageStart = profileStages ? monotonic_ns() : 0;
  if (g_gxState.alphaUpdate && g_gxState.dstAlpha != UINT32_MAX) {
    if (!clear) {
      // TODO: figure out the right behavior here.
      // should the copy have a specific alpha value but the EFB remains untouched?
    }
    // Overwrite alpha before resolving
    gfx::push_draw_command(gfx::clear::DrawData{
        .pipeline = gfx::pipeline_ref(gfx::clear::PipelineConfig{
            .clearColor = false,
            .clearAlpha = true,
            .clearDepth = false,
        }),
        .color = wgpu::Color{0.f, 0.f, 0.f, g_gxState.dstAlpha / 255.f},
    });
  }
  const uint64_t alphaNs = profileStages ? monotonic_ns() - stageStart : 0;

  stageStart = profileStages ? monotonic_ns() : 0;
  const auto clearColor = clear && g_gxState.colorUpdate;
  const auto clearAlpha = clear && g_gxState.alphaUpdate;
  const auto clearDepth = clear && g_gxState.depthUpdate;
  gfx::resolve_pass_into(handle.handle, rect, clearColor, clearAlpha, clearDepth, g_gxState.clearColor,
                         clear_depth_value(), texCopyFmt, display, std::move(job));
  const uint64_t resolveNs = profileStages ? monotonic_ns() - stageStart : 0;
  stageStart = profileStages ? monotonic_ns() : 0;
  ++handle.revision;
  if (std::getenv("AURORA_DIAGNOSTIC_TEXTURE_CONTENT") &&
      std::getenv("AURORA_DIAGNOSTIC_DRAW_UV") && texCopyFmt == GX_CTF_R4) {
    static unsigned records = 0;
    if (records++ < 128) {
      std::fprintf(stderr,"R4_COPY_REQUEST destination=%zx revision=%llu x=%d y=%d width=%u height=%u dst_width=%u dst_height=%u clear=%u\n",
          reinterpret_cast<size_t>(dest),static_cast<unsigned long long>(handle.revision),
          rect.x,rect.y,rect.width,rect.height,dstWidth,dstHeight,unsigned(clear));
    } else if (records == 129) std::fprintf(stderr,"R4_COPY_REQUEST_LIMIT records=128\n");
  }
  g_gxState.copyTextures[dest] = handle;
  texture::invalidate_bindings();
  if (profileStages)
    record_xfb_copy_stage_profile(setupNs, alphaNs, resolveNs, monotonic_ns() - stageStart,
                                  dstWidth, dstHeight);
}
void copy_tex(const void* dest, GXBool clear) noexcept { copy_tex_impl(dest, clear); }
} // namespace aurora::gx

extern "C" {
// Diagnostic bridge for validating the real filter pass before GXCopyDisp/VI integration.
// It produces a native virtual display surface, never CPU XFB bytes.
int aurora_diagnostic_filtered_snapshot(const void* destination, uint64_t frame_id) {
  if (!aurora_diagnostic_is_owner() || !destination || !frame_id ||
      frame_id != aurora_diagnostic_recording_id()) return 0;
  aurora::gx::fifo::drain();
  if (g_gxState.pixelFmt != GX_PF_RGB8_Z24 || displayCopy.scale != 256 ||
      ((displayCopy.control >> 7) & 3u) == 3u) return 0;
  for (unsigned i = 1; i <= 4; ++i)
    if ((g_gxState.bpRegCache[i] & 0xffffff) != 0x666666) return 0;
  const u32 c0 = g_gxState.bpRegCache[0x53], c1 = g_gxState.bpRegCache[0x54];
  aurora::gfx::DisplayCopyOptions options;
  const auto logicalSize = aurora::gx::logical_fb_size();
  const auto targetSize = aurora::gfx::get_render_target_size();
  if (!logicalSize.y || !targetSize.y) return 0;
  options.logicalRowStep = float(targetSize.y) / float(logicalSize.y);
  options.enabled = true;
  options.clampTop = (displayCopy.control & 1) != 0;
  options.clampBottom = (displayCopy.control & 2) != 0;
  const unsigned gamma = (displayCopy.control >> 7) & 3;
  options.gammaReciprocal = gamma == 0 ? 1.f : gamma == 1 ? 1.f / 1.7f : 1.f / 2.2f;
  options.rowWeights = aurora::gfx::display_copy_weights(c0, c1);
  // Diagnostic currently uses the host's explicit GXSetTexCopySrc/Dst crop.
  // Preserve SDK copy dimensions; no inferred XFB size or VI success.
  if (g_gxState.texCopyFmt != GX_TF_RGBA8) return 0;
  aurora::gx::copy_tex_impl(destination, GX_FALSE, options);
  return 1;
}
int aurora_diagnostic_blank_snapshot(const void* destination, uint64_t frame_id) {
  if(!aurora_diagnostic_is_owner() || !destination || !frame_id ||
     frame_id!=aurora_diagnostic_recording_id()) return 0;
  aurora::gx::fifo::drain();
  if(g_gxState.copyTextures.contains(destination)) return 0;
  const auto format=aurora::webgpu::g_graphicsConfig.surfaceConfiguration.format;
  if(format!=wgpu::TextureFormat::RGBA8Unorm && format!=wgpu::TextureFormat::BGRA8Unorm) return 0;
  auto texture=aurora::gfx::new_render_texture(640,480,GX_TF_RGBA8,"Blank native snapshot");
  if(!texture || !texture->texture || !texture->sampleTextureView) return 0;
  std::vector<uint8_t> pixels(640*480*4,0);
  for(size_t i=3;i<pixels.size();i+=4) pixels[i]=255;
  aurora::gfx::queue_texture_upload_data(pixels.data(),640*4,480,
                                       wgpu::TexelCopyTextureInfo{.texture=texture->texture},texture->size);
  g_gxState.copyTextureCache.emplace(aurora::gx::GXState::CopyTextureKey{
      .dest=destination,.width=640,.height=480,.format=GX_TF_RGBA8},
      aurora::gx::GXState::CopyTextureRef{.handle=texture,.revision=1});
  g_gxState.copyTextures[destination]={.handle=std::move(texture),.revision=1};
  return 1;
}
int aurora_diagnostic_present_snapshot(const void* destination, uint64_t frame_id) {
  if(!aurora_diagnostic_is_owner() || !destination || !frame_id ||
     frame_id!=aurora_diagnostic_recording_id()) return 0;
  aurora::gx::fifo::drain();
  const auto found=g_gxState.copyTextures.find(destination);
  if(found==g_gxState.copyTextures.end() || !found->second.handle ||
     !found->second.handle->texture || !found->second.handle->sampleTextureView ||
     (found->second.handle->format!=wgpu::TextureFormat::RGBA8Unorm && found->second.handle->format!=wgpu::TextureFormat::BGRA8Unorm)) return 0;
  return aurora::gfx::detail::diagnostic_select_snapshot(frame_id,found->second.handle);
}
int aurora_diagnostic_snapshot_info(const void* destination, uint32_t* width,
                                    uint32_t* height, uint32_t* revision) {
  if(!aurora_diagnostic_is_owner() || !destination || !width || !height || !revision) return 0;
  aurora::gx::fifo::drain();
  const auto found = g_gxState.copyTextures.find(destination);
  if(found == g_gxState.copyTextures.end() || !found->second.handle ||
     !found->second.handle->texture || !found->second.handle->sampleTextureView) return 0;
  *width = found->second.handle->size.width;
  *height = found->second.handle->size.height;
  *revision = found->second.revision;
  return 1;
}
GXRenderModeObj GXNtsc480IntDf = {
    VI_TVMODE_NTSC_INT,
    640,
    480,
    480,
    40,
    0,
    640,
    480,
    VI_XFBMODE_DF,
    0,
    0,
    {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6},
    {8, 8, 10, 12, 10, 8, 8},
};
GXRenderModeObj GXNtsc480Int = {
    VI_TVMODE_NTSC_INT,
    640,
    480,
    480,
    40,
    0,
    640,
    480,
    VI_XFBMODE_DF,
    0,
    0,
    {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6},
    {0, 0, 21, 22, 21, 0, 0},
};
GXRenderModeObj GXNtsc480Prog = {
    VI_TVMODE_NTSC_PROG,
    640,
    480,
    480,
    40,
    0,
    640,
    480,
    VI_XFBMODE_SF,
    0,
    0,
    {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6},
    {0, 0, 21, 22, 21, 0, 0},
};
GXRenderModeObj GXPal528IntDf = {
    VI_TVMODE_PAL_INT,
    704,
    528,
    480,
    40,
    0,
    640,
    480,
    VI_XFBMODE_DF,
    0,
    0,
    {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6},
    {8, 8, 10, 12, 10, 8, 8},
};
GXRenderModeObj GXMpal480IntDf = {
    VI_TVMODE_PAL_INT,
    640,
    480,
    480,
    40,
    0,
    640,
    480,
    VI_XFBMODE_DF,
    0,
    0,
    {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6},
    {8, 8, 10, 12, 10, 8, 8},
};

void GXAdjustForOverscan(GXRenderModeObj* rmin, GXRenderModeObj* rmout, u16 hor, u16 ver) {
  *rmout = *rmin;
  const auto size = aurora::window::get_window_size();
  rmout->fbWidth = size.fb_width;
  rmout->efbHeight = size.fb_height;
  rmout->xfbHeight = size.fb_height;
}

void GXSetDispCopySrc(u16 left, u16 top, u16 wd, u16 ht) {
  AURORA_ASSERT(wd && ht && wd <= 1024 && ht <= 1024 && left <= 1023 && top <= 1023,
                "Invalid display-copy source dimensions");
  displayCopy.source = 0x49000000 | left | (u32(top) << 10);
  displayCopy.size = 0x4a000000 | (wd - 1) | (u32(ht - 1) << 10);
  GX_WRITE_RAS_REG(displayCopy.source);
  GX_WRITE_RAS_REG(displayCopy.size);
}

void GXSetTexCopySrc(u16 left, u16 top, u16 wd, u16 ht) {
  GX_WRITE_AURORA(GX_AURORA_LOAD_COPY_SRC);
  GX_WRITE_U32(left);
  GX_WRITE_U32(top);
  GX_WRITE_U32(wd);
  GX_WRITE_U32(ht);
}

void GXSetDispCopyDst(u16 wd, u16 ht) {
  (void)ht; // SDK destination register represents stride, not height.
  AURORA_ASSERT(wd && !(wd & 15) && wd <= 16368, "Invalid display-copy destination width");
  displayCopy.stride = 0x4d000000 | (wd >> 4);
  GX_WRITE_RAS_REG(displayCopy.stride);
}

void GXSetTexCopyDst(u16 wd, u16 ht, GXTexFmt fmt, GXBool mipmap) {
  GX_WRITE_AURORA(GX_AURORA_LOAD_COPY_DST);
  GX_WRITE_U32(wd);
  GX_WRITE_U32(ht);
  GX_WRITE_U32(fmt);
  GX_WRITE_U8(mipmap != GX_FALSE);
}

// TODO GXSetDispCopyFrame2Field
void GXSetCopyClamp(GXFBClamp clamp) {
  // SDK setter shadows both copy triggers; it does not submit a copy.
  const u32 bits = u32(clamp) & 3u;
  displayCopy.control = (displayCopy.control & ~3u) | bits;
  __gx->cpTex = (__gx->cpTex & ~3u) | bits;
}

u32 GXSetDispCopyYScale(f32 vscale) {
  AURORA_ASSERT(std::isfinite(vscale) && vscale >= 1.f && vscale <= 256.f,
                "Unsupported display-copy vertical scale");
  displayCopy.scale = u32(256.f / vscale) & 0x1ff;
  GX_WRITE_RAS_REG(0x4e000000 | displayCopy.scale);
  const u32 height = ((displayCopy.size >> 10) & 0x3ff) + 1;
  return u32(height * (256.f / float(displayCopy.scale)));
}

int aurora_diagnostic_display_copy_regs(uint32_t* source, uint32_t* size,
                                       uint32_t* stride, uint32_t* scale) {
  if(!aurora_diagnostic_is_owner() || !source || !size || !stride || !scale) return 0;
  aurora::gx::fifo::drain();
  *source = g_gxState.bpRegCache[0x49] & 0xffffff;
  *size = g_gxState.bpRegCache[0x4a] & 0xffffff;
  *stride = g_gxState.bpRegCache[0x4d] & 0xffffff;
  *scale = g_gxState.bpRegCache[0x4e] & 0xffffff;
  return 1;
}

int aurora_diagnostic_copy_controls(uint32_t* display, uint32_t* texture, uint32_t* decoded) {
  if (!aurora_diagnostic_is_owner() || !display || !texture || !decoded) return 0;
  aurora::gx::fifo::drain();
  *display = displayCopy.control;
  *texture = __gx->cpTex;
  *decoded = g_gxState.bpRegCache[0x52] & 0xffffff;
  return 1;
}

int aurora_diagnostic_copy_filter_regs(uint32_t* registers) {
  if (!aurora_diagnostic_is_owner() || !registers) return 0;
  aurora::gx::fifo::drain();
  for (unsigned i = 0; i < 4; ++i) registers[i] = g_gxState.bpRegCache[i + 1] & 0xffffff;
  registers[4] = g_gxState.bpRegCache[0x53] & 0xffffff;
  registers[5] = g_gxState.bpRegCache[0x54] & 0xffffff;
  return 1;
}

void GXSetCopyClear(GXColor color, u32 depth) {
  // BP 0x4F: clear color R + A
  u32 reg0 = 0;
  SET_REG_FIELD(0, reg0, 8, 0, color.r);
  SET_REG_FIELD(0, reg0, 8, 8, color.a);
  SET_REG_FIELD(0, reg0, 8, 24, 0x4F);
  GX_WRITE_RAS_REG(reg0);

  // BP 0x50: clear color B + G
  u32 reg1 = 0;
  SET_REG_FIELD(0, reg1, 8, 0, color.b);
  SET_REG_FIELD(0, reg1, 8, 8, color.g);
  SET_REG_FIELD(0, reg1, 8, 24, 0x50);
  GX_WRITE_RAS_REG(reg1);

  // BP 0x51: clear Z (24-bit)
  u32 reg2 = 0;
  SET_REG_FIELD(0, reg2, 24, 0, depth);
  SET_REG_FIELD(0, reg2, 8, 24, 0x51);
  GX_WRITE_RAS_REG(reg2);
  __gx->bpSent = 1;
}

void GXSetCopyFilter(GXBool aa, u8 sample_pattern[12][2], GXBool vf, u8 vfilter[7]) {
  // SDK register packing only; native copy filtering still needs execution.
  for (u32 reg = 0; reg < 4; ++reg) {
    u32 packed = (reg + 1) << 24;
    if (aa) {
      for (u32 sample = 0; sample < 3; ++sample) {
        packed |= (u32(sample_pattern[reg * 3 + sample][0]) & 15u) << (sample * 8);
        packed |= (u32(sample_pattern[reg * 3 + sample][1]) & 15u) << (sample * 8 + 4);
      }
    } else {
      packed |= 0x666666;
    }
    GX_WRITE_RAS_REG(packed);
  }
  u32 coeff0 = 0x53000000, coeff1 = 0x54000000;
  if (vf) {
    for (u32 tap = 0; tap < 4; ++tap) coeff0 |= (u32(vfilter[tap]) & 63u) << (tap * 6);
    for (u32 tap = 0; tap < 3; ++tap) coeff1 |= (u32(vfilter[tap + 4]) & 63u) << (tap * 6);
  } else {
    coeff0 |= (21u << 12) | (22u << 18);
    coeff1 |= 21u;
  }
  GX_WRITE_RAS_REG(coeff0);
  GX_WRITE_RAS_REG(coeff1);
  __gx->bpSent = 0;
}

void GXSetDispCopyGamma(GXGamma gamma) {
  // SDK control state only; native gamma-correct copy execution is unfinished.
  displayCopy.control = (displayCopy.control & ~(3u << 7)) | ((u32(gamma) & 3u) << 7);
}

void GXCopyDisp(void* dest, GXBool clear) {
  const bool trace = std::getenv("AURORA_DIAGNOSTIC_Z_FIFO") != nullptr;
  const bool profile = xfb_copy_profile_enabled();
  uint64_t fifoNs = 0;
  uint64_t fifoStart = 0;
  uint64_t profileStart = 0;
  if (trace) std::fputs("Z_DISPLAY_COPY_BEGIN\n", stderr);
  AURORA_ASSERT(aurora::gx::xfb::on_owner() && dest && aurora_diagnostic_recording_id(),
                "GXCopyDisp requires enrolled live storage and an active native frame");
  fifoStart = profile ? monotonic_ns() : 0;
  aurora::gx::fifo::drain();
  if (profile) {
    const uint64_t drainedAt = monotonic_ns();
    fifoNs = drainedAt - fifoStart;
    profileStart = drainedAt;
  }
  if (trace) std::fprintf(stderr, "Z_DISPLAY_COPY_DRAINED pixel_format=%u scale=%u samples=%u\n",
                          unsigned(g_gxState.pixelFmt), unsigned(displayCopy.scale),
                          unsigned(aurora::gfx::get_sample_count()));
  AURORA_ASSERT(g_gxState.pixelFmt==GX_PF_RGB8_Z24 && displayCopy.scale==256 &&
                ((displayCopy.control>>7)&3u)!=3u && aurora::gfx::get_sample_count()==1,
                "GXCopyDisp currently requires progressive non-AA RGB8 and unit Y scale");
  for(unsigned i=1;i<=4;++i)AURORA_ASSERT((g_gxState.bpRegCache[i]&0xffffff)==0x666666,
                                       "GXCopyDisp AA sample resolve is unfinished");
  const uint32_t width=(displayCopy.size&1023)+1,height=((displayCopy.size>>10)&1023)+1;
  const uint32_t stride=(displayCopy.stride&1023)*16;
  const auto logical=aurora::gx::logical_fb_size();
  const auto target=aurora::gfx::get_render_target_size();
  AURORA_ASSERT(stride==width && logical.x && logical.y && target.x && target.y,
                "GXCopyDisp requires packed stride and nonzero framebuffer dimensions");
  bool allocationFailure=false;
  const bool gpuSnapshot=aurora::gfx::render_snapshot::destination(dest,aurora_diagnostic_recording_id());
  auto job=gpuSnapshot ? std::shared_ptr<aurora::gx::xfb::Job>{} :
      aurora::gx::xfb::begin(dest,aurora_diagnostic_recording_id(),width,height,&allocationFailure);
  if (trace) std::fprintf(stderr,"Z_DISPLAY_COPY_JOB_RESULT allocation_failure=%u\n",unsigned(allocationFailure));
  if(allocationFailure){
    static uint64_t droppedFrames=0;
    ++droppedFrames;
    if(droppedFrames==1 || droppedFrames%120==0)
      std::fprintf(stderr,"AURORA_XFB_PRESENTATION_DROPPED allocation_failure=%llu frame=%llu\n",
                   static_cast<unsigned long long>(droppedFrames),
                   static_cast<unsigned long long>(aurora_diagnostic_recording_id()));
  }
  // Keep executing the resolve/clear path without an XFB readback job after a
  // transient allocation failure. This preserves EFB clear semantics and the
  // last presented CPU XFB instead of aborting or leaving the EFB uncleared.
  AURORA_ASSERT(job || allocationFailure || gpuSnapshot,"GXCopyDisp destination is not enrolled, mismatched or still in use");
  aurora::gfx::DisplayCopyOptions options;
  options.enabled=true;options.logicalRowStep=float(target.y)/float(logical.y);
  options.clampTop=(displayCopy.control&1)!=0;
  options.clampBottom=(displayCopy.control&2)!=0;
  const unsigned gamma=(displayCopy.control>>7)&3;
  options.gammaReciprocal=gamma==0?1.f:gamma==1?1.f/1.7f:1.f/2.2f;
  options.rowWeights=aurora::gfx::display_copy_weights(g_gxState.bpRegCache[0x53],g_gxState.bpRegCache[0x54]);
  const auto savedSource=g_gxState.texCopySrc;
  const auto savedWidth=g_gxState.texCopyDstWidth,savedHeight=g_gxState.texCopyDstHeight;
  const auto savedFormat=g_gxState.texCopyFmt;
  g_gxState.texCopySrc={int(displayCopy.source&1023),int((displayCopy.source>>10)&1023),int(width),int(height)};
  g_gxState.texCopyDstWidth=width;g_gxState.texCopyDstHeight=height;g_gxState.texCopyFmt=GX_TF_RGBA8;
  aurora::gx::copy_tex_impl(dest,clear,options,std::move(job));
  if(gpuSnapshot) {
    const auto found=g_gxState.copyTextures.find(dest);
    AURORA_ASSERT(found!=g_gxState.copyTextures.end() &&
      aurora::gfx::render_snapshot::attach(dest,aurora_diagnostic_recording_id(),found->second.handle),
      "GPU display snapshot attachment failed");
  }
  if (profile) record_xfb_copy_profile(fifoNs, monotonic_ns() - profileStart);
  if (trace) std::fputs("Z_DISPLAY_COPY_RECORDED\n", stderr);
  g_gxState.texCopySrc=savedSource;g_gxState.texCopyDstWidth=savedWidth;
  g_gxState.texCopyDstHeight=savedHeight;g_gxState.texCopyFmt=savedFormat;
}

void GXCopyTex(void* dest, GXBool clear) {
  GX_WRITE_AURORA(GX_AURORA_LOAD_COPY_DEST);
  GX_WRITE_U64(reinterpret_cast<u64>(dest));

  SET_REG_FIELD(0, __gx->cpTex, 1, 11, clear != GX_FALSE);
  SET_REG_FIELD(0, __gx->cpTex, 1, 14, 0);
  SET_REG_FIELD(0, __gx->cpTex, 8, 24, 0x52);
  GX_WRITE_RAS_REG(__gx->cpTex);
}

// TODO GXGetYScaleFactor
// TODO GXGetNumXfbLines
// TODO GXClearBoundingBox
// TODO GXReadBoundingBox
}





