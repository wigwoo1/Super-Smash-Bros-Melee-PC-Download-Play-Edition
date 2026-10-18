#pragma once
#include "texture_convert.hpp"
#include "../gx/z_texture_wgsl.hpp"
#include <array>
#include <atomic>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
namespace aurora::gfx::depth_texture_diagnostic {
constexpr unsigned Pixels=45, Words=Pixels*3*2, Bytes=Words*4;
struct Job {
  std::array<wgpu::Texture,3> textures;
  wgpu::Buffer output,readback;
  std::array<uint32_t,Words> expected{};
  std::atomic<int> status{0};
};
inline std::shared_ptr<Job> pending;
inline void drain() {
  if(!pending || !pending->status.load(std::memory_order_acquire))return;
  bool equal=false;
  if(pending->status.load()==1) {
    const auto* bytes=pending->readback.GetConstMappedRange(0,Bytes);
    equal=bytes && std::memcmp(bytes,pending->expected.data(),Bytes)==0;
    pending->readback.Unmap();
  }
  std::fprintf(stderr,"GPU_DEPTH_TEXTURE_%s pixels=135 compared_bytes=1080 scope=production_decode_gpu_load_and_depth_arithmetic\n",equal?"PASS":"FAIL");
  pending.reset();
  if(!equal)std::abort();
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame) {
  static bool started=false;
  if(started || !std::getenv("AURORA_DIAGNOSTIC_DEPTH_TEXTURE"))return;
  started=true;auto job=std::make_shared<Job>();
  const std::array<u32,3> formats{GX_TF_Z8,GX_TF_Z16,GX_TF_Z24X8};
  std::array<wgpu::TextureView,3> views;
  for(unsigned f=0;f<3;++f) {
    const unsigned bw=f==0?8:4,tiles=(9+bw-1)/bw,blockBytes=f==2?64:32;
    std::vector<uint8_t> raw(tiles*2*blockBytes);
    for(unsigned i=0;i<raw.size();++i)raw[i]=uint8_t((i*37+11)%256);
    auto decoded=convert_texture(formats[f],9,5,1,raw);
    const wgpu::TextureDescriptor td{.label="Depth texture decoded oracle",.usage=wgpu::TextureUsage::TextureBinding|wgpu::TextureUsage::CopyDst,
      .dimension=wgpu::TextureDimension::e2D,.size={9,5,1},.format=wgpu::TextureFormat::RGBA8Unorm,.mipLevelCount=1,.sampleCount=1};
    job->textures[f]=webgpu::g_device.CreateTexture(&td);views[f]=job->textures[f].CreateView();
    const wgpu::TexelCopyTextureInfo dest{.texture=job->textures[f]};
    const wgpu::TexelCopyBufferLayout layout{.bytesPerRow=36,.rowsPerImage=5};
    webgpu::g_queue.WriteTexture(&dest,decoded.data.data(),decoded.data.size(),&layout,&td.size);
    for(unsigned y=0;y<5;++y)for(unsigned x=0;x<9;++x) {
      const unsigned block=((y/4)*tiles+x/bw)*blockBytes,pixel=(y%4)*bw+x%bw;
      auto byte=[](unsigned address){return (address*37+11)%256;};
      std::array<uint32_t,4> rgba;
      if(f==0)rgba.fill(byte(block+pixel));
      else if(f==1)rgba={byte(block+pixel*2+1),byte(block+pixel*2+1),byte(block+pixel*2+1),byte(block+pixel*2)};
      else rgba={byte(block+pixel*2+1),byte(block+32+pixel*2),byte(block+33+pixel*2),byte(block+pixel*2)};
      const unsigned at=(f*Pixels+y*9+x)*2;
      job->expected[at]=rgba[0]|(rgba[1]<<8)|(rgba[2]<<16)|(rgba[3]<<24);
      const uint32_t depth=f==0?rgba[3]:f==1?rgba[3]*256+rgba[0]:rgba[0]*65536+rgba[1]*256+rgba[2];
      job->expected[at+1]=std::bit_cast<uint32_t>(float(depth)/16777216.f);
    }
  }
  const wgpu::BufferDescriptor od{.label="Depth texture GPU load output",.usage=wgpu::BufferUsage::Storage|wgpu::BufferUsage::CopySrc,.size=Bytes};
  job->output=webgpu::g_device.CreateBuffer(&od);
  const wgpu::BufferDescriptor rd{.label="Depth texture GPU load readback",.usage=wgpu::BufferUsage::MapRead|wgpu::BufferUsage::CopyDst,.size=Bytes};
  job->readback=webgpu::g_device.CreateBuffer(&rd);
  std::string shader(gx::ZTextureWGSL);shader+=R"(
@group(0) @binding(0) var z8:texture_2d<f32>;
@group(0) @binding(1) var z16:texture_2d<f32>;
@group(0) @binding(2) var z24:texture_2d<f32>;
@group(0) @binding(3) var<storage,read_write> output:array<u32>;
@compute @workgroup_size(64) fn main(@builtin(global_invocation_id) id:vec3u) {
 if(id.x>=135u){return;}
 let f=id.x/45u;let p=id.x%45u;let pos=vec2i(i32(p%9u),i32(p/9u));
 var sample:vec4f;
 if(f==0u){sample=textureLoad(z8,pos,0);}else if(f==1u){sample=textureLoad(z16,pos,0);}else{sample=textureLoad(z24,pos,0);}
 let b=vec4u(round(sample*255.0));
 output[id.x*2u]=b.r|(b.g<<8u)|(b.b<<16u)|(b.a<<24u);
 output[id.x*2u+1u]=bitcast<u32>(gx_z_texture(sample,f,2u,0u,0.0));
})";
  wgpu::ShaderSourceWGSL source{};source.code=shader.c_str();
  const wgpu::ShaderModuleDescriptor md{.nextInChain=&source,.label="Production depth texture decode GPU load"};
  const auto module=webgpu::g_device.CreateShaderModule(&md);
  const wgpu::ComputePipelineDescriptor pd{.label="Depth texture GPU oracle",.compute={.module=module,.entryPoint="main"}};
  const auto pipeline=webgpu::g_device.CreateComputePipeline(&pd);
  const std::array entries{wgpu::BindGroupEntry{.binding=0,.textureView=views[0]},wgpu::BindGroupEntry{.binding=1,.textureView=views[1]},
    wgpu::BindGroupEntry{.binding=2,.textureView=views[2]},wgpu::BindGroupEntry{.binding=3,.buffer=job->output,.size=Bytes}};
  const wgpu::BindGroupDescriptor bd{.layout=pipeline.GetBindGroupLayout(0),.entryCount=entries.size(),.entries=entries.data()};
  const auto bind=webgpu::g_device.CreateBindGroup(&bd);
  const auto pass=cmd.BeginComputePass();pass.SetPipeline(pipeline);pass.SetBindGroup(0,bind);pass.DispatchWorkgroups(3);pass.End();
  cmd.CopyBufferToBuffer(job->output,0,job->readback,0,Bytes);pending=job;
  frame.afterSubmitCallbacks.emplace_back([job]{job->readback.MapAsync(wgpu::MapMode::Read,0,Bytes,wgpu::CallbackMode::AllowSpontaneous,
    [job](wgpu::MapAsyncStatus status,wgpu::StringView){job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);});});
  std::fputs("GPU_DEPTH_TEXTURE_QUEUED pixels=135 compared_bytes=1080\n",stderr);
}
}
