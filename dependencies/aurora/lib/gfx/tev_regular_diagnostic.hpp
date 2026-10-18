#pragma once
#include "../gx/tev_regular_wgsl.hpp"
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
namespace aurora::gfx::tev_regular_diagnostic {
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
  std::fprintf(stderr,"GPU_TEV_REGULAR_%s cases=12288 compared_bytes=196608 scope=production_wgsl_arithmetic_only\n",equal?"PASS":"FAIL");pending.reset();
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame) {
  static bool started=false;
  if(started || !std::getenv("AURORA_DIAGNOSTIC_TEV_REGULAR")) return;
  started=true;auto job=std::make_shared<Job>();
  std::array<int32_t,98304> input{};
  const auto expected=[](int a,int b,int c,int d,bool sub,int bias,unsigned scale,bool clamped) {
    a&=255;b&=255;c&=255;
    const int multiplier=scale==3?1:1<<scale;
    const int weighted=a*(256-c-(c>>7))+b*(c+(c>>7));
    const int roundBias=scale==3?0:sub?127:128;
    const int mixed=(weighted*multiplier+roundBias)>>8;
    int result=(d+bias)*multiplier+(sub?-mixed:mixed);
    if(scale==3)result>>=1;
    return std::clamp(result,clamped?0:-1024,clamped?255:1023);
  };
  for(unsigned i=0;i<12288;++i) {
    const unsigned sample=i%256,mode=i/256;
    const bool clamped=mode%2,sub=mode/24;
    const unsigned scale=(mode/2)%4;
    const int bias=std::array<int,3>{0,128,-128}[(mode/8)%3];
    const int a=int(sample)+(sample%3==0?256:sample%3==1?-256:0);
    const int b=int((sample+2)%256),c=191;
    const int d=std::array<int,12>{-1024,-1023,-512,-128,-1,0,1,127,128,255,511,1023}[sample%12];
    auto* row=input.data()+i*8;row[0]=a;row[1]=b;row[2]=c;row[3]=d;
    row[4]=sub;row[5]=bias;row[6]=scale;row[7]=clamped;
    job->expected[i*4]=expected(a,b,c,d,sub,bias,scale,clamped);
    job->expected[i*4+1]=expected(a,b,c,d,sub,bias,scale,clamped);
    job->expected[i*4+2]=expected(b,a,c,d,sub,bias,scale,clamped);
    job->expected[i*4+3]=expected(c,a,b,d,sub,bias,scale,clamped);
  }
  const wgpu::BufferDescriptor inDesc{.label="Z texture independent oracle input",.usage=wgpu::BufferUsage::Storage,.size=393216,.mappedAtCreation=true};
  job->input=webgpu::g_device.CreateBuffer(&inDesc);std::memcpy(job->input.GetMappedRange(0,393216),input.data(),393216);job->input.Unmap();
  const wgpu::BufferDescriptor outDesc{.label="TEV regular arithmetic output",.usage=wgpu::BufferUsage::Storage|wgpu::BufferUsage::CopySrc,.size=196608};
  job->output=webgpu::g_device.CreateBuffer(&outDesc);
  const wgpu::BufferDescriptor readDesc{.label="TEV regular arithmetic readback",.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=196608};
  job->readback=webgpu::g_device.CreateBuffer(&readDesc);
  std::string shader(gx::TevRegularWGSL);shader+=R"(
@group(0) @binding(0) var<storage,read> inputs: array<i32>;
@group(0) @binding(1) var<storage,read_write> outputs: array<vec4i>;
@compute @workgroup_size(64) fn main(@builtin(global_invocation_id) id:vec3u) {
  if(id.x>=12288u) {return;}
  let base=id.x*8u;let a=f32(inputs[base])/255.0;let b=f32(inputs[base+1u])/255.0;
  let c=f32(inputs[base+2u])/255.0;let d=f32(inputs[base+3u])/255.0;
  let sub=inputs[base+4u]!=0;let bias=inputs[base+5u];let scale=u32(inputs[base+6u]);
  let low=select(-1024.0/255.0,0.0,inputs[base+7u]!=0);
  let high=select(1023.0/255.0,1.0,inputs[base+7u]!=0);
  let scalar=clamp(gx_tev_regular1(a,b,c,d,sub,bias,scale),low,high);
  let color=clamp(gx_tev_regular3(vec3f(a,b,c),vec3f(b,a,a),vec3f(c,c,b),vec3f(d),sub,bias,scale),vec3f(low),vec3f(high));
  outputs[id.x]=vec4i(round(vec4f(scalar,color)*255.0));
})";
  wgpu::ShaderSourceWGSL source{};source.code=shader.c_str();
  const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&source,.label="Production TEV regular arithmetic"};
  const auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
  const wgpu::ComputePipelineDescriptor pipelineDesc{.label="Production TEV regular arithmetic",.compute={.module=module,.entryPoint="main"}};
  const auto pipeline=webgpu::g_device.CreateComputePipeline(&pipelineDesc);
  const std::array entries{wgpu::BindGroupEntry{.binding=0,.buffer=job->input,.size=393216},wgpu::BindGroupEntry{.binding=1,.buffer=job->output,.size=196608}};
  const wgpu::BindGroupDescriptor bindDesc{.layout=pipeline.GetBindGroupLayout(0),.entryCount=entries.size(),.entries=entries.data()};
  const auto bind=webgpu::g_device.CreateBindGroup(&bindDesc);
  const auto pass=cmd.BeginComputePass();pass.SetPipeline(pipeline);pass.SetBindGroup(0,bind);pass.DispatchWorkgroups(192);pass.End();
  cmd.CopyBufferToBuffer(job->output,0,job->readback,0,196608);pending=job;
  frame.afterSubmitCallbacks.emplace_back([job] {job->readback.MapAsync(wgpu::MapMode::Read,0,196608,wgpu::CallbackMode::AllowSpontaneous,[job](wgpu::MapAsyncStatus status,wgpu::StringView) {job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);});});
  std::fprintf(stderr,"GPU_TEV_REGULAR_QUEUED cases=12288 compared_bytes=196608\n");
}
}
