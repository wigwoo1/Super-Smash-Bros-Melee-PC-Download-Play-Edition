#pragma once
#include "../gx/texcoord_projection_wgsl.hpp"
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
namespace aurora::gfx::texcoord_projection_diagnostic {
struct Job {
  wgpu::Buffer input,output,readback;
  std::array<uint32_t,2048> expected{};
  std::atomic<int> status{0};
};
inline std::shared_ptr<Job> pending;
inline void drain() {
  if(!pending || !pending->status.load(std::memory_order_acquire)) return;
  bool equal=false;
  if(pending->status.load()==1) {
    const auto* data=pending->readback.GetConstMappedRange(0,8192);
    equal=data && std::memcmp(data,pending->expected.data(),8192)==0;
    pending->readback.Unmap();
  }
  std::fprintf(stderr,"GPU_TEXCOORD_PROJECTION_%s cases=128 compared_bytes=8192 scope=production_wgsl_arithmetic_only\n",equal?"PASS":"FAIL");pending.reset();
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame) {
  static bool started=false;
  if(started || !std::getenv("AURORA_DIAGNOSTIC_TEXCOORD_PROJECTION")) return;
  started=true;auto job=std::make_shared<Job>();
  std::array<float,1024> input{};
  for(uint32_t i=0;i<128;++i) {
    const float x=float(int(i%17)-8)*0.5f;
    const float y=float(int((i*7)%23)-11)*0.5f;
    const float q=std::array<float,4>{0.f,-0.f,2.f,-2.f}[i%4];
    input[i*8]=x;input[i*8+1]=y;input[i*8+2]=q;
    const float vx=q==0.f?std::max(-1.f,std::min(1.f,x*0.5f)):x;
    const float vy=q==0.f?std::max(-1.f,std::min(1.f,y*0.5f)):y;
    // Independently compute the vertex finish and fragment projection outputs.
    const float px=q==0.f?x:x/q, py=q==0.f?y:y/q;
    job->expected[i*16]=std::bit_cast<uint32_t>(vx);
    job->expected[i*16+1]=std::bit_cast<uint32_t>(vy);
    job->expected[i*16+2]=std::bit_cast<uint32_t>(px);
    job->expected[i*16+3]=std::bit_cast<uint32_t>(py);
    const float width=float(8u << (i%5)), height=float(16u << ((i/5)%4));
    const float sx=width*std::array<float,4>{0.25f,1.f,2.f,8.f}[i%4];
    const float sy=height*std::array<float,4>{0.5f,1.f,4.f,1.f}[(i/4)%4];
    input[i*8+4]=sx;input[i*8+5]=sy;input[i*8+6]=width;input[i*8+7]=height;
    job->expected[i*16+4]=std::bit_cast<uint32_t>((px*sx)/width);
    job->expected[i*16+5]=std::bit_cast<uint32_t>((py*sy)/height);
    job->expected[i*16+6]=std::bit_cast<uint32_t>((x*sx)/width);
    job->expected[i*16+7]=std::bit_cast<uint32_t>((y*sy)/height);
    const float tx=i%3==2?x:x*0.5f+y*2.f+q*4.f+8.f;
    const float ty=i%3==2?y:x*-2.f+y*0.25f+q*8.f+16.f;
    const float tz=i%3==2?q:x*4.f+y*-8.f+q*0.5f+2.f;
    const std::array<float,8> transformed{tx,ty,tz,1.f,
      tx*2.f+ty*4.f+tz*8.f+16.f,tx*-4.f+ty*2.f+tz*0.5f+8.f,
      tx*0.25f+ty*-2.f+tz*4.f+32.f,1.f};
    for(unsigned k=0;k<8;++k)job->expected[i*16+8+k]=std::bit_cast<uint32_t>(transformed[k]);
  }
  const wgpu::BufferDescriptor inDesc{.label="Texcoord projection input",.usage=wgpu::BufferUsage::Storage,.size=4096,.mappedAtCreation=true};
  job->input=webgpu::g_device.CreateBuffer(&inDesc);std::memcpy(job->input.GetMappedRange(0,4096),input.data(),4096);job->input.Unmap();
  const wgpu::BufferDescriptor outDesc{.label="Texcoord projection output",.usage=wgpu::BufferUsage::Storage|wgpu::BufferUsage::CopySrc,.size=8192};
  job->output=webgpu::g_device.CreateBuffer(&outDesc);
  const wgpu::BufferDescriptor readDesc{.label="Texcoord projection readback",.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=8192};
  job->readback=webgpu::g_device.CreateBuffer(&readDesc);
  std::string shader(gx::TexcoordProjectionWGSL);shader+=R"(
@group(0) @binding(0) var<storage,read> inputs: array<vec4f>;
@group(0) @binding(1) var<storage,read_write> outputs: array<vec4f>;
@compute @workgroup_size(64) fn main(@builtin(global_invocation_id) id:vec3u) {
  if(id.x>=128u) {return;}
  let coord=inputs[id.x*2u].xyz;
  let dimensions=inputs[id.x*2u+1u];
  let uv=gx_project_texcoord(coord);
  outputs[id.x*4u]=vec4f(gx_texgen_finish(coord).xy,uv);
  outputs[id.x*4u+1u]=vec4f(gx_scale_texcoord(uv,dimensions.xy,dimensions.zw),
                             gx_scale_texcoord(coord.xy,dimensions.xy,dimensions.zw));
  let matrix=mat3x4f(vec4f(0.5,2.0,4.0,8.0),vec4f(-2.0,0.25,8.0,16.0),vec4f(4.0,-8.0,0.5,2.0));
  let post=mat3x4f(vec4f(2.0,4.0,8.0,16.0),vec4f(-4.0,2.0,0.5,8.0),vec4f(0.25,-2.0,4.0,32.0));
  var ubuf: IndexedMatrixFixture;
  ubuf.postex_mtx[0]=matrix;
  ubuf.postex_mtx[19]=matrix;
  let in_texmtxidx0=select(select(0u,57u,id.x%3u==1u),60u,id.x%3u==2u);
  let tc0=vec4f(coord,1.0);
)";
  shader.insert(shader.find("@compute"), "struct IndexedMatrixFixture { postex_mtx: array<mat3x4f,20>, };\n");
  shader+=gx::indexed_texcoord_transform_wgsl(0);
  shader+=R"(
  let transformed=tc0_tmp;
  outputs[id.x*4u+2u]=vec4f(transformed,1.0);
  outputs[id.x*4u+3u]=vec4f(gx_transform_texcoord(vec4f(transformed,1.0),post),1.0);
})";
  wgpu::ShaderSourceWGSL source{};source.code=shader.c_str();
  const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&source,.label="Production Texcoord projection"};
  const auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
  const wgpu::ComputePipelineDescriptor pipelineDesc{.label="Production Texcoord projection",.compute={.module=module,.entryPoint="main"}};
  const auto pipeline=webgpu::g_device.CreateComputePipeline(&pipelineDesc);
  const std::array entries{wgpu::BindGroupEntry{.binding=0,.buffer=job->input,.size=4096},wgpu::BindGroupEntry{.binding=1,.buffer=job->output,.size=8192}};
  const wgpu::BindGroupDescriptor bindDesc{.layout=pipeline.GetBindGroupLayout(0),.entryCount=entries.size(),.entries=entries.data()};
  const auto bind=webgpu::g_device.CreateBindGroup(&bindDesc);
  const auto pass=cmd.BeginComputePass();pass.SetPipeline(pipeline);pass.SetBindGroup(0,bind);pass.DispatchWorkgroups(2);pass.End();
  cmd.CopyBufferToBuffer(job->output,0,job->readback,0,8192);pending=job;
  frame.afterSubmitCallbacks.emplace_back([job] {job->readback.MapAsync(wgpu::MapMode::Read,0,8192,wgpu::CallbackMode::AllowSpontaneous,[job](wgpu::MapAsyncStatus status,wgpu::StringView) {job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);});});
  std::fprintf(stderr,"GPU_TEXCOORD_PROJECTION_QUEUED cases=128 compared_bytes=8192\n");
}
}
