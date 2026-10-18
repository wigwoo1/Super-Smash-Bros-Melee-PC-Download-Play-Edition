#pragma once
#include "../gx/z_texture_wgsl.hpp"
#include "../webgpu/gpu.hpp"
#include <array>
#include <atomic>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
namespace aurora::gfx::z_texture_diagnostic {
struct Job {
  wgpu::Buffer input,output,readback;
  std::array<uint32_t,512> expected{};
  std::atomic<int> status{0};
};
inline std::shared_ptr<Job> pending;
inline void drain() {
  if(!pending || !pending->status.load(std::memory_order_acquire)) return;
  bool equal=false;
  if(pending->status.load()==1) {
    const auto* data=pending->readback.GetConstMappedRange(0,2048);
    equal=data && std::memcmp(data,pending->expected.data(),2048)==0;
    pending->readback.Unmap();
  }
  std::fprintf(stderr,"GPU_Z_TEXTURE_%s cases=512 compared_bytes=2048 scope=production_wgsl_arithmetic_only\n",equal?"PASS":"FAIL");pending.reset();
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame) {
  static bool started=false;
  if(started || !std::getenv("AURORA_DIAGNOSTIC_Z_TEXTURE")) return;
  started=true;auto job=std::make_shared<Job>();
  std::array<uint32_t,4096> input{};
  for(uint32_t i=0;i<512;++i) {
    const std::array<uint32_t,4> rgba{(i*73+5)%256,(i*29+127)%256,(i*19+241)%256,(i*53+11)%256};
    const uint32_t format=i%3,op=(i/3)%3,bias=i%4==0?0:i%4==1?0xFFFFFF:i*977731u;
    const uint32_t raster=(i*7919u+1234567u)&0xFFFFFFu;
    auto* row=input.data()+i*8;for(unsigned c=0;c<4;++c)row[c]=rgba[c];
    row[4]=format;row[5]=op;row[6]=bias;row[7]=raster;
    uint64_t tex=rgba[3];if(format==1)tex=rgba[3]*256u+rgba[0];
    if(format==2)tex=rgba[0]*65536u+rgba[1]*256u+rgba[2];
    const uint32_t result=op==0?raster:uint32_t((tex+bias+(op==1?raster:0))%16777216ull);
    job->expected[i]=std::bit_cast<uint32_t>(float(result)/16777216.f);
  }
  const wgpu::BufferDescriptor inDesc{.label="Z texture independent oracle input",.usage=wgpu::BufferUsage::Storage,.size=16384,.mappedAtCreation=true};
  job->input=webgpu::g_device.CreateBuffer(&inDesc);std::memcpy(job->input.GetMappedRange(0,16384),input.data(),16384);job->input.Unmap();
  const wgpu::BufferDescriptor outDesc{.label="Z texture arithmetic output",.usage=wgpu::BufferUsage::Storage|wgpu::BufferUsage::CopySrc,.size=2048};
  job->output=webgpu::g_device.CreateBuffer(&outDesc);
  const wgpu::BufferDescriptor readDesc{.label="Z texture arithmetic readback",.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=2048};
  job->readback=webgpu::g_device.CreateBuffer(&readDesc);
  std::string shader(gx::ZTextureWGSL);shader+=R"(
@group(0) @binding(0) var<storage,read> inputs: array<u32>;
@group(0) @binding(1) var<storage,read_write> outputs: array<u32>;
@compute @workgroup_size(64) fn main(@builtin(global_invocation_id) id:vec3u) {
  if(id.x>=512u) {return;}
  let b=id.x*8u;
  let sample=vec4f(f32(inputs[b]),f32(inputs[b+1u]),f32(inputs[b+2u]),f32(inputs[b+3u]))/255.0;
  outputs[id.x]=bitcast<u32>(gx_z_texture(sample,inputs[b+4u],inputs[b+5u],inputs[b+6u],f32(inputs[b+7u])/16777216.0));
})";
  wgpu::ShaderSourceWGSL source{};source.code=shader.c_str();
  const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&source,.label="Production Z texture arithmetic"};
  const auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
  const wgpu::ComputePipelineDescriptor pipelineDesc{.label="Production Z texture arithmetic",.compute={.module=module,.entryPoint="main"}};
  const auto pipeline=webgpu::g_device.CreateComputePipeline(&pipelineDesc);
  const std::array entries{wgpu::BindGroupEntry{.binding=0,.buffer=job->input,.size=16384},wgpu::BindGroupEntry{.binding=1,.buffer=job->output,.size=2048}};
  const wgpu::BindGroupDescriptor bindDesc{.layout=pipeline.GetBindGroupLayout(0),.entryCount=entries.size(),.entries=entries.data()};
  const auto bind=webgpu::g_device.CreateBindGroup(&bindDesc);
  const auto pass=cmd.BeginComputePass();pass.SetPipeline(pipeline);pass.SetBindGroup(0,bind);pass.DispatchWorkgroups(8);pass.End();
  cmd.CopyBufferToBuffer(job->output,0,job->readback,0,2048);pending=job;
  frame.afterSubmitCallbacks.emplace_back([job] {job->readback.MapAsync(wgpu::MapMode::Read,0,2048,wgpu::CallbackMode::AllowSpontaneous,[job](wgpu::MapAsyncStatus status,wgpu::StringView) {job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);});});
  std::fprintf(stderr,"GPU_Z_TEXTURE_QUEUED cases=512 compared_bytes=2048\n");
}
}
