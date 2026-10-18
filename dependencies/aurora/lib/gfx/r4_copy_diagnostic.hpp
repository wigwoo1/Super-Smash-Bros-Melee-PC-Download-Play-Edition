#pragma once
#include "tex_copy_conv.hpp"
#include "texture.hpp"
#include "../webgpu/gpu.hpp"
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
namespace aurora::gfx::r4_copy_diagnostic {
constexpr unsigned Bytes=5120;
struct Job {
  std::array<TextureHandle,2> source,destination;
  std::array<wgpu::Buffer,2> uploads;
  wgpu::Buffer uniform,readback;
  std::array<uint8_t,Bytes> expected{};
  std::atomic<int> status{0};
};
inline std::shared_ptr<Job> pending;
inline void drain() {
  if(!pending || !pending->status.load(std::memory_order_acquire)) return;
  bool equal=false; unsigned mismatches=0;
  if(pending->status.load()==1) {
    const auto* actual=static_cast<const uint8_t*>(pending->readback.GetConstMappedRange(0,Bytes));
    if(actual) for(unsigned i=0;i<Bytes;++i) mismatches+=actual[i]!=pending->expected[i];
    equal=actual && mismatches==0; pending->readback.Unmap();
  }
  std::fprintf(stderr,"GPU_R4_COPY_PIXELS_%s nearest_pixels=256 linear_pixels=1024 compared_bytes=%u mismatches=%u scope=production_r4_render_copy_synthetic\n",equal?"PASS":"FAIL",Bytes,mismatches);
  pending.reset();
}
inline TextureHandle texture(unsigned w,unsigned h,bool destination) {
  const wgpu::TextureDescriptor desc{.label="Synthetic R4 copy texture",.usage=destination?
    wgpu::TextureUsage::RenderAttachment|wgpu::TextureUsage::CopySrc:
    wgpu::TextureUsage::TextureBinding|wgpu::TextureUsage::CopyDst,
    .dimension=wgpu::TextureDimension::e2D,.size={w,h,1},.format=wgpu::TextureFormat::RGBA8Unorm,.mipLevelCount=1,.sampleCount=1};
  auto tex=webgpu::g_device.CreateTexture(&desc);auto view=tex.CreateView();
  return std::make_shared<TextureRef>(tex,destination?wgpu::TextureView{}:view,
    destination?view:wgpu::TextureView{},wgpu::Extent3D{w,h,1},wgpu::TextureFormat::RGBA8Unorm,1,destination?GX_CTF_R4:GX_TF_RGBA8_PC);
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame) {
  static bool started=false;
  if(started || !std::getenv("AURORA_DIAGNOSTIC_R4_COPY_PIXELS"))return;
  started=true;auto job=std::make_shared<Job>();
  const wgpu::BufferDescriptor uniformDesc{.label="R4 copy identity UV",.usage=wgpu::BufferUsage::Uniform,.size=16,.mappedAtCreation=true};
  job->uniform=webgpu::g_device.CreateBuffer(&uniformDesc);
  const std::array<float,4> uv{0,0,1,1};std::memcpy(job->uniform.GetMappedRange(0,16),uv.data(),16);job->uniform.Unmap();
  const wgpu::BufferDescriptor readDesc{.label="R4 copy independent oracle",.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=Bytes};
  job->readback=webgpu::g_device.CreateBuffer(&readDesc);
  for(unsigned test=0;test<2;++test) {
    const unsigned dw=64,dh=test?16:4,sw=test?128:64,sh=test?32:4;
    std::vector<uint8_t> pixels(sw*sh*4);
    for(unsigned y=0;y<dh;++y)for(unsigned x=0;x<dw;++x) {
      const unsigned index=y*dw+x;const unsigned sum=std::min(index,1020u);
      // Deliberately asymmetric 2x2 blocks exercise both dimensions of filtering.
      unsigned remaining=sum;
      for(unsigned p=0;p<(test?4u:1u);++p) {
        const unsigned red=test?std::min(remaining,255u):index;
        if(test)remaining-=red;
        const unsigned sx=test?x*2+(p&1):x,sy=test?y*2+(p>>1):y;
        const unsigned at=(sy*sw+sx)*4;
        pixels[at]=red;pixels[at+1]=255-red;pixels[at+2]=17;pixels[at+3]=85;
      }
      const uint8_t value=(test?sum/64:index>>4)*17;
      const unsigned out=(test?1024:0)+(y*64+x)*4;
      for(unsigned c=0;c<4;++c)job->expected[out+c]=value;
    }
    job->source[test]=texture(sw,sh,false);job->destination[test]=texture(dw,dh,true);
    const wgpu::BufferDescriptor uploadDesc{.label="R4 copy synthetic upload",.usage=wgpu::BufferUsage::CopySrc,.size=pixels.size(),.mappedAtCreation=true};
    job->uploads[test]=webgpu::g_device.CreateBuffer(&uploadDesc);
    std::memcpy(job->uploads[test].GetMappedRange(0,pixels.size()),pixels.data(),pixels.size());job->uploads[test].Unmap();
    const wgpu::TexelCopyBufferInfo upload{.layout={.bytesPerRow=sw*4,.rowsPerImage=sh},.buffer=job->uploads[test]};
    const wgpu::TexelCopyTextureInfo dst{.texture=job->source[test]->texture};
    const wgpu::Extent3D sourceSize{sw,sh,1};cmd.CopyBufferToTexture(&upload,&dst,&sourceSize);
    tex_copy_conv::diagnostic_run_r4(cmd,tex_copy_conv::ConvRequest{.fmt=GX_CTF_R4,
      .srcView=job->source[test]->sampleTextureView,.uniformRange={0,16},.dst=job->destination[test],
      .sampleFilter=test?tex_copy_conv::SampleFilter::Linear:tex_copy_conv::SampleFilter::Nearest},job->uniform);
    const wgpu::TexelCopyTextureInfo from{.texture=job->destination[test]->texture};
    const wgpu::TexelCopyBufferInfo to{.layout={.offset=test?1024u:0u,.bytesPerRow=256,.rowsPerImage=dh},.buffer=job->readback};
    const wgpu::Extent3D outputSize{dw,dh,1};cmd.CopyTextureToBuffer(&from,&to,&outputSize);
  }
  pending=job;
  frame.afterSubmitCallbacks.emplace_back([job] {job->readback.MapAsync(wgpu::MapMode::Read,0,Bytes,wgpu::CallbackMode::AllowSpontaneous,
    [job](wgpu::MapAsyncStatus status,wgpu::StringView) {job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);});});
  std::fprintf(stderr,"GPU_R4_COPY_PIXELS_QUEUED compared_bytes=%u\n",Bytes);
}
}
