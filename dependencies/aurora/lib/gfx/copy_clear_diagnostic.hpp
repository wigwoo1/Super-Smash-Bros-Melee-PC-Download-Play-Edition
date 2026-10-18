#pragma once
#include "clear.hpp"
#include "../webgpu/gpu.hpp"
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace aurora::gfx::copy_clear_diagnostic {
struct Job {
  wgpu::Buffer readback;
  std::array<wgpu::Texture,16> color,depth;
  std::array<uint8_t,65536> expected{};
  std::atomic<int> status{0};
};
inline std::shared_ptr<Job> pending;
inline void drain() {
  if (!pending || !pending->status.load(std::memory_order_acquire)) return;
  bool equal=false;
  if (pending->status.load()==1) {
    const auto* actual=pending->readback.GetConstMappedRange(0,65536);
    equal=actual && std::memcmp(actual,pending->expected.data(),65536)==0;
    pending->readback.Unmap();
  }
  std::fprintf(stderr,"GPU_COPY_CLEAR_%s cases=16 compared_bytes=65536 scope=production_clear_draw_rgb_alpha_depth_scissor\n",equal?"PASS":"FAIL");
  pending.reset();
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame) {
  static bool started=false;
  if (started || !std::getenv("AURORA_DIAGNOSTIC_COPY_CLEAR")) return;
  started=true;
  const auto format=webgpu::g_graphicsConfig.surfaceConfiguration.format;
  if (format!=wgpu::TextureFormat::BGRA8Unorm && format!=wgpu::TextureFormat::RGBA8Unorm) {
    std::fprintf(stderr,"GPU_COPY_CLEAR_UNSUPPORTED_FORMAT\n"); return;
  }
  auto job=std::make_shared<Job>();
  const wgpu::BufferDescriptor buffer{.label="Copy clear synthetic readback",.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=65536};
  job->readback=webgpu::g_device.CreateBuffer(&buffer);
  wgpu::ShaderSourceWGSL probeShader{};
  probeShader.code=R"(
@vertex fn vs(@builtin(vertex_index) i:u32)->@builtin(position) vec4f {
  let p=array<vec2f,3>(vec2f(-1,1),vec2f(-1,-3),vec2f(3,1));
  return vec4f(p[i],0.5,1);
}
@fragment fn fs()->@location(0) vec4f {return vec4f(1,0,1,1);}
)";
  const wgpu::ShaderModuleDescriptor shaderDesc{.nextInChain=&probeShader,.label="Independent depth comparison probe"};
  const auto module=webgpu::g_device.CreateShaderModule(&shaderDesc);
  const wgpu::ColorTargetState probeColor{.format=format};
  const wgpu::FragmentState probeFragment{.module=module,.entryPoint="fs",.targetCount=1,.targets=&probeColor};
  const wgpu::DepthStencilState probeDepth{.format=webgpu::g_graphicsConfig.depthFormat,.depthWriteEnabled=false,.depthCompare=wgpu::CompareFunction::Less};
  const wgpu::RenderPipelineDescriptor probeDesc{.label="Independent depth comparison probe",.vertex={.module=module,.entryPoint="vs"},.primitive={.topology=wgpu::PrimitiveTopology::TriangleList},.depthStencil=&probeDepth,.fragment=&probeFragment};
  const auto probePipeline=webgpu::g_device.CreateRenderPipeline(&probeDesc);
  for (unsigned test=0;test<16;++test) {
    const bool rgb=(test&1)!=0,alpha=(test&2)!=0,clearDepth=(test&4)!=0,empty=(test&8)!=0;
    const wgpu::TextureDescriptor colorDesc{.label="Copy clear synthetic color",.usage=wgpu::TextureUsage::RenderAttachment|wgpu::TextureUsage::CopySrc,.dimension=wgpu::TextureDimension::e2D,.size={64,8,1},.format=format,.mipLevelCount=1,.sampleCount=1};
    job->color[test]=webgpu::g_device.CreateTexture(&colorDesc);
    auto depthDesc=colorDesc;depthDesc.label="Copy clear synthetic depth";depthDesc.format=webgpu::g_graphicsConfig.depthFormat;depthDesc.usage=wgpu::TextureUsage::RenderAttachment;
    job->depth[test]=webgpu::g_device.CreateTexture(&depthDesc);
    const wgpu::RenderPassColorAttachment color{.view=job->color[test].CreateView(),.loadOp=wgpu::LoadOp::Clear,.storeOp=wgpu::StoreOp::Store,.clearValue={17./255,34./255,51./255,68./255}};
    const wgpu::RenderPassDepthStencilAttachment depth{.view=job->depth[test].CreateView(),.depthLoadOp=wgpu::LoadOp::Clear,.depthStoreOp=wgpu::StoreOp::Store,.depthClearValue=0.75f};
    const wgpu::RenderPassDescriptor desc{.label="Production copy clear draw oracle",.colorAttachmentCount=1,.colorAttachments=&color,.depthStencilAttachment=&depth};
    const auto pass=cmd.BeginRenderPass(&desc);
    pass.SetPipeline(clear::create_pipeline(clear::PipelineConfig{.clearColor=rgb,.clearAlpha=alpha,.clearDepth=clearDepth}));
    clear::render_bound(clear::DrawData{.color={85./255,102./255,119./255,136./255},.depth=0.25f,.region={13,2,empty?0u:19u,3,false},.regionEnabled=true},pass,{64,8,1});
    pass.End();
    const wgpu::TexelCopyTextureInfo source{.texture=job->color[test]};
    const wgpu::TexelCopyBufferInfo destination{.layout={.offset=test*4096u,.bytesPerRow=256,.rowsPerImage=8},.buffer=job->readback};
    const wgpu::Extent3D size{64,8,1};cmd.CopyTextureToBuffer(&source,&destination,&size);
    for(unsigned y=0;y<8;++y) for(unsigned x=0;x<64;++x) {
      const bool inside=!empty && x>=13 && x<32 && y>=2 && y<5;
      std::array<uint8_t,4> value{17,34,51,68};
      if(inside && rgb) {value[0]=85;value[1]=102;value[2]=119;}
      if(inside && alpha) value[3]=136;
      if(format==wgpu::TextureFormat::BGRA8Unorm) std::swap(value[0],value[2]);
      std::memcpy(job->expected.data()+test*4096+(y*64+x)*4,value.data(),4);
      // z=0.5 passes against retained 0.75; fails against the cleared 0.25.
      if (!(inside && clearDepth)) value={255,0,255,255};
      std::memcpy(job->expected.data()+test*4096+2048+(y*64+x)*4,value.data(),4);
    }
    auto loadedColor=color;loadedColor.loadOp=wgpu::LoadOp::Load;
    auto loadedDepth=depth;loadedDepth.depthLoadOp=wgpu::LoadOp::Load;
    const wgpu::RenderPassDescriptor probePassDesc{.label="Verify retained and cleared depth",.colorAttachmentCount=1,.colorAttachments=&loadedColor,.depthStencilAttachment=&loadedDepth};
    const auto probePass=cmd.BeginRenderPass(&probePassDesc);
    probePass.SetPipeline(probePipeline);probePass.SetViewport(0,0,64,8,0,1);probePass.SetScissorRect(0,0,64,8);probePass.Draw(3);probePass.End();
    auto depthDestination=destination;depthDestination.layout.offset+=2048;
    cmd.CopyTextureToBuffer(&source,&depthDestination,&size);
  }
  pending=job;
  std::fprintf(stderr,"GPU_COPY_CLEAR_QUEUED cases=16 compared_bytes=65536\n");
  frame.afterSubmitCallbacks.emplace_back([job] {
    job->readback.MapAsync(wgpu::MapMode::Read,0,65536,wgpu::CallbackMode::AllowSpontaneous,
      [job](wgpu::MapAsyncStatus status,wgpu::StringView) {job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);});
  });
}
}
