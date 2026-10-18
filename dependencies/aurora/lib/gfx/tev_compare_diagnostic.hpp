#pragma once
#include "../gx/tev_compare_wgsl.hpp"
#include "../webgpu/gpu.hpp"
#include <array>
#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
namespace aurora::gfx::tev_compare_diagnostic {
struct Job {
  wgpu::Buffer input,output,readback;
  std::array<int32_t,49152> expected{};
  std::atomic<int> status{0};
};
inline std::shared_ptr<Job> pending;
inline void drain() {
  if(!pending || !pending->status.load(std::memory_order_acquire)) return;
  bool equal=false;
  if(pending->status.load()==1) {
    const auto* data=pending->readback.GetConstMappedRange(0,196608);
    equal=data && std::memcmp(data,pending->expected.data(),196608)==0;
    pending->readback.Unmap();
  }
  std::fprintf(stderr,"GPU_TEV_COMPARE_%s cases=12288 compared_bytes=196608 scope=production_wgsl_arithmetic_only\n",equal?"PASS":"FAIL");pending.reset();
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame) {
  static bool started=false;
  if(started || !std::getenv("AURORA_DIAGNOSTIC_TEV_COMPARE")) return;
  started=true;auto job=std::make_shared<Job>();
  std::array<int32_t,98304> input{};
  const auto packed=[](int value,unsigned mode) {
    const int red=value&255,green=(value>>8)&255,blue=(value>>16)&255;
    return red+(mode>0?green*256:0)+(mode>1?blue*65536:0);
  };
  for(unsigned i=0;i<12288;++i) {
    const unsigned mode=(i/1536)/2;const bool eq=(i/1536)%2;
    const unsigned sample=i%1536;
    const int a=int((sample*7919u+0x123456u)&0xffffffu);
    // Include equal, single-component-different and opposite-order comparisons.
    const int b=sample%4==0?a:sample%4==1?a^1:sample%4==2?a^256:a^65536;
    const int aa=int(sample%256)-256,ba=sample%3==0?aa:aa+1;
    const int c=int((sample*17)%256)+256,d=int(sample%2048)-1024;
    auto* row=input.data()+i*8;row[0]=a;row[1]=b;row[2]=aa;row[3]=ba;
    row[4]=c;row[5]=d;row[6]=mode;row[7]=eq;
    const auto test=[eq](int av,int bv){return eq?av==bv:av>bv;};
    const bool alpha=mode==3?test(aa&255,ba&255):test(packed(a,mode),packed(b,mode));
    job->expected[i*4]=d+(alpha?(c&255):0);
    for(unsigned channel=0;channel<3;++channel) {
      const bool pass=mode==3?test((a>>(channel*8))&255,(b>>(channel*8))&255)
                             :test(packed(a,mode),packed(b,mode));
      job->expected[i*4+channel+1]=d+(pass?(c&255):0);
    }
  }
  const wgpu::BufferDescriptor inDesc{.label="TEV comparison independent oracle input",.usage=wgpu::BufferUsage::Storage,.size=393216,.mappedAtCreation=true};
  job->input=webgpu::g_device.CreateBuffer(&inDesc);std::memcpy(job->input.GetMappedRange(0,393216),input.data(),393216);job->input.Unmap();
  const wgpu::BufferDescriptor outDesc{.label="TEV comparison output",.usage=wgpu::BufferUsage::Storage|wgpu::BufferUsage::CopySrc,.size=196608};
  job->output=webgpu::g_device.CreateBuffer(&outDesc);
  const wgpu::BufferDescriptor readDesc{.label="TEV comparison readback",.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=196608};
  job->readback=webgpu::g_device.CreateBuffer(&readDesc);
  std::string shader(gx::TevCompareWGSL);shader+=R"(
@group(0) @binding(0) var<storage,read> inputs: array<i32>;
@group(0) @binding(1) var<storage,read_write> outputs: array<vec4i>;
@compute @workgroup_size(64) fn main(@builtin(global_invocation_id) id:vec3u) {
  if(id.x>=12288u) {return;}
  let base=id.x*8u;let ap=inputs[base];let bp=inputs[base+1u];
  let a=vec3f(f32(ap & 255),f32((ap >> 8u) & 255),f32((ap >> 16u) & 255))/255.0;
  let b=vec3f(f32(bp & 255),f32((bp >> 8u) & 255),f32((bp >> 16u) & 255))/255.0;
  let aa=f32(inputs[base+2u])/255.0;let ba=f32(inputs[base+3u])/255.0;
  let c=f32(inputs[base+4u])/255.0;let d=f32(inputs[base+5u])/255.0;
  let mode=u32(inputs[base+6u]);let eq=inputs[base+7u]!=0;
  let scalar=gx_tev_compare1(a,b,aa,ba,c,d,mode,eq);
  let color=gx_tev_compare3(a,b,vec3f(c),vec3f(d),mode,eq);
  outputs[id.x]=vec4i(round(vec4f(scalar,color)*255.0));
})";
  wgpu::ShaderSourceWGSL source{};source.code=shader.c_str();
  const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&source,.label="Production TEV comparison"};
  const auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
  const wgpu::ComputePipelineDescriptor pipelineDesc{.label="Production TEV comparison",.compute={.module=module,.entryPoint="main"}};
  const auto pipeline=webgpu::g_device.CreateComputePipeline(&pipelineDesc);
  const std::array entries{wgpu::BindGroupEntry{.binding=0,.buffer=job->input,.size=393216},wgpu::BindGroupEntry{.binding=1,.buffer=job->output,.size=196608}};
  const wgpu::BindGroupDescriptor bindDesc{.layout=pipeline.GetBindGroupLayout(0),.entryCount=entries.size(),.entries=entries.data()};
  const auto bind=webgpu::g_device.CreateBindGroup(&bindDesc);
  const auto pass=cmd.BeginComputePass();pass.SetPipeline(pipeline);pass.SetBindGroup(0,bind);pass.DispatchWorkgroups(192);pass.End();
  cmd.CopyBufferToBuffer(job->output,0,job->readback,0,196608);pending=job;
  frame.afterSubmitCallbacks.emplace_back([job] {job->readback.MapAsync(wgpu::MapMode::Read,0,196608,wgpu::CallbackMode::AllowSpontaneous,[job](wgpu::MapAsyncStatus status,wgpu::StringView) {job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);});});
  std::fprintf(stderr,"GPU_TEV_COMPARE_QUEUED cases=12288 compared_bytes=196608\n");
}
}
