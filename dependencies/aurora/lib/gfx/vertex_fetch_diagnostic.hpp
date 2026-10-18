#pragma once
#include "vertex_fetch_diagnostic_wgsl.hpp"
#include <array>
#include <atomic>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
namespace aurora::gfx::vertex_fetch_diagnostic {
struct Job {
  wgpu::Buffer input,vertexInput,output,readback;
  wgpu::Buffer renderReadback;
  wgpu::Texture renderTarget;
  std::array<uint32_t,1024> expected{};
  std::array<uint32_t,1024> renderExpected{};
  std::atomic<int> status{0};
  std::atomic<int> renderStatus{0};
};
inline std::shared_ptr<Job> pending;
inline void drain() {
  if (!pending) return;
  const int status=pending->status.load(std::memory_order_acquire);
  const int renderStatus=pending->renderStatus.load(std::memory_order_acquire);
  if (!status || !renderStatus) return;
  bool equal=false;
  if (status==1) {
    const void* data=pending->readback.GetConstMappedRange(0,4096);
    equal=data && std::memcmp(data,pending->expected.data(),4096)==0;
    pending->readback.Unmap();
  }
  std::fprintf(stderr,"GPU_VERTEX_FETCH_%s offsets=256 words=1024\n",equal?"PASS":"FAIL");
  bool renderEqual=false;
  if (renderStatus==1) {
    const void* data=pending->renderReadback.GetConstMappedRange(0,4096);
    renderEqual=data && std::memcmp(data,pending->renderExpected.data(),4096)==0;
    pending->renderReadback.Unmap();
  }
  std::fprintf(stderr,"GPU_VERTEX_RENDER_%s pixels=256 words=1024\n",renderEqual?"PASS":"FAIL");
  pending.reset();
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame) {
  static bool started=false;
  if (started || std::getenv("AURORA_DIAGNOSTIC_GPU_VERTEX_FETCH")==nullptr) return;
  started=true;
  std::fprintf(stderr,"GPU_VERTEX_FETCH_PROFILE signed_xyz=%u stride=%u fraction=%u\n",
               std::getenv("AURORA_DIAGNOSTIC_GPU_POSITION_FETCH") ? 1u : 0u,
               std::getenv("AURORA_DIAGNOSTIC_GPU_POSITION_FETCH") ? 6u : 4u,
               std::getenv("AURORA_DIAGNOSTIC_GPU_POSITION_FETCH") ? 6u : 14u);
  auto job=std::make_shared<Job>();
  const bool positionProfile = std::getenv("AURORA_DIAGNOSTIC_GPU_POSITION_FETCH") != nullptr;
  std::array<uint8_t,16384> bytes{};
  std::array<uint8_t,1024> vertexBytes{};
  for (unsigned i=0;i<bytes.size();++i) bytes[i]=uint8_t(i*73+(i>>8)*31+19);
  for (unsigned i=0;i<256;++i) {
    const unsigned index=(i*29+7)%256;
    vertexBytes[17+i*3]=uint8_t(index>>8);vertexBytes[18+i*3]=uint8_t(index);
  }
  for (unsigned i=0;i<256;++i) {
    const unsigned be=unsigned(bytes[i])*256+bytes[i+1];
    const unsigned le=unsigned(bytes[i+1])*256+bytes[i];
    const int signedBe=be>=32768?int(be)-65536:int(be);
    job->expected[i*4]=std::bit_cast<uint32_t>(float(be)/16384.f);
    job->expected[i*4+1]=std::bit_cast<uint32_t>(float(le)/32768.f);
    job->expected[i*4+2]=std::bit_cast<uint32_t>(float(signedBe)/16384.f);
    job->expected[i*4+3]=(uint32_t(bytes[i])<<24)|(uint32_t(bytes[i+1])<<16)|(uint32_t(bytes[i+2])<<8)|bytes[i+3];
    const unsigned offset=2048+(i%12)*1024+((i*29+7)%256)*(positionProfile ? 6 : 4);
    const unsigned indexedBe=unsigned(bytes[offset])*256+bytes[offset+1];
    const unsigned indexedLe=unsigned(bytes[offset+1])*256+bytes[offset];
    const int indexedSigned=indexedBe>=32768?int(indexedBe)-65536:int(indexedBe);
    job->renderExpected[i*4]=std::bit_cast<uint32_t>(float(indexedBe)/16384.f);
    job->renderExpected[i*4+1]=std::bit_cast<uint32_t>(float(indexedLe)/32768.f);
    job->renderExpected[i*4+2]=std::bit_cast<uint32_t>(float(indexedSigned)/16384.f);
    job->renderExpected[i*4+3]=std::bit_cast<uint32_t>(7.f);
    if (positionProfile) {
      for (unsigned c=0;c<3;++c) {
        const unsigned value=unsigned(bytes[offset+c*2])*256+bytes[offset+c*2+1];
        const int signedValue=value>=32768?int(value)-65536:int(value);
        job->renderExpected[i*4+c]=std::bit_cast<uint32_t>(float(signedValue)/64.f);
      }
    }
  }
  wgpu::BufferDescriptor bufferDesc{.label="GPU vertex fetch synthetic input",.usage=wgpu::BufferUsage::Storage,.size=bytes.size(),.mappedAtCreation=true};
  job->input=webgpu::g_device.CreateBuffer(&bufferDesc);
  std::memcpy(job->input.GetMappedRange(0,bytes.size()),bytes.data(),bytes.size());job->input.Unmap();
  bufferDesc={.label="GPU vertex index input",.usage=wgpu::BufferUsage::Storage,.size=vertexBytes.size(),.mappedAtCreation=true};
  job->vertexInput=webgpu::g_device.CreateBuffer(&bufferDesc);
  std::memcpy(job->vertexInput.GetMappedRange(0,vertexBytes.size()),vertexBytes.data(),vertexBytes.size());job->vertexInput.Unmap();
  bufferDesc={.label="GPU vertex fetch output",.usage=wgpu::BufferUsage::Storage|wgpu::BufferUsage::CopySrc,.size=4096};
  job->output=webgpu::g_device.CreateBuffer(&bufferDesc);
  bufferDesc={.label="GPU vertex fetch readback",.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=4096};
  job->readback=webgpu::g_device.CreateBuffer(&bufferDesc);
  job->renderReadback=webgpu::g_device.CreateBuffer(&bufferDesc);
  wgpu::ShaderSourceWGSL wgsl{};wgsl.code=positionProfile ? PositionShader : Shader;
  const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&wgsl,.label="Production vertex fetch diagnostic"};
  const auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
  const wgpu::ComputePipelineDescriptor pipelineDesc{.label="Production vertex fetch diagnostic",.compute={.module=module,.entryPoint="diagnostic_main"}};
  const auto pipeline=webgpu::g_device.CreateComputePipeline(&pipelineDesc);
  const std::array entries{
    wgpu::BindGroupEntry{.binding=1,.buffer=job->input,.size=bytes.size()},
    wgpu::BindGroupEntry{.binding=2,.buffer=job->output,.size=4096}};
  const wgpu::BindGroupDescriptor bindDesc{.layout=pipeline.GetBindGroupLayout(0),.entryCount=entries.size(),.entries=entries.data()};
  const auto bindings=webgpu::g_device.CreateBindGroup(&bindDesc);
  const auto pass=cmd.BeginComputePass();pass.SetPipeline(pipeline);pass.SetBindGroup(0,bindings);pass.DispatchWorkgroups(4);pass.End();
  cmd.CopyBufferToBuffer(job->output,0,job->readback,0,4096);
  const wgpu::TextureDescriptor targetDesc{.label="Vertex fetch render target",.usage=wgpu::TextureUsage::RenderAttachment|wgpu::TextureUsage::CopySrc,
    .dimension=wgpu::TextureDimension::e2D,.size={16,16,1},.format=wgpu::TextureFormat::RGBA32Float,.mipLevelCount=1,.sampleCount=1};
  job->renderTarget=webgpu::g_device.CreateTexture(&targetDesc);
  const wgpu::ColorTargetState colorTarget{.format=wgpu::TextureFormat::RGBA32Float};
  const wgpu::FragmentState fragment{.module=module,.entryPoint="diagnostic_fragment",.targetCount=1,.targets=&colorTarget};
  const std::array inputLayoutEntries{
    wgpu::BindGroupLayoutEntry{.binding=0,.visibility=wgpu::ShaderStage::Vertex,.buffer={.type=wgpu::BufferBindingType::ReadOnlyStorage}},
    wgpu::BindGroupLayoutEntry{.binding=1,.visibility=wgpu::ShaderStage::Vertex,.buffer={.type=wgpu::BufferBindingType::ReadOnlyStorage}}};
  const wgpu::BindGroupLayoutDescriptor inputLayoutDesc{.entryCount=inputLayoutEntries.size(),.entries=inputLayoutEntries.data()};
  const auto inputLayout=webgpu::g_device.CreateBindGroupLayout(&inputLayoutDesc);
  const wgpu::PipelineLayoutDescriptor renderLayoutDesc{.bindGroupLayoutCount=1,.bindGroupLayouts=&inputLayout,
    .immediateSize=sizeof(gx::DrawImmediateData)};
  const auto renderLayout=webgpu::g_device.CreatePipelineLayout(&renderLayoutDesc);
  const wgpu::RenderPipelineDescriptor renderDesc{.label="Production vertex fetch render diagnostic",
    .layout=renderLayout,.vertex={.module=module,.entryPoint="diagnostic_vertex"},.primitive={.topology=wgpu::PrimitiveTopology::TriangleList},.fragment=&fragment};
  const auto renderPipeline=webgpu::g_device.CreateRenderPipeline(&renderDesc);
  const std::array renderEntries{
    wgpu::BindGroupEntry{.binding=0,.buffer=job->vertexInput,.size=vertexBytes.size()},
    wgpu::BindGroupEntry{.binding=1,.buffer=job->input,.size=bytes.size()}};
  const wgpu::BindGroupDescriptor renderBindingsDesc{.layout=renderPipeline.GetBindGroupLayout(0),.entryCount=renderEntries.size(),.entries=renderEntries.data()};
  const auto renderBindings=webgpu::g_device.CreateBindGroup(&renderBindingsDesc);
  const wgpu::RenderPassColorAttachment attachment{.view=job->renderTarget.CreateView(),.loadOp=wgpu::LoadOp::Clear,.storeOp=wgpu::StoreOp::Store};
  const wgpu::RenderPassDescriptor renderPassDesc{.label="Vertex fetch render diagnostic",.colorAttachmentCount=1,.colorAttachments=&attachment};
  const auto renderPass=cmd.BeginRenderPass(&renderPassDesc);
  gx::DrawImmediateData immediates{.vtxStart=17,.currentPnMtx=7};
  for(unsigned i=0;i<12;++i) immediates.arrayStart[i]=2048+i*1024;
  renderPass.SetPipeline(renderPipeline);renderPass.SetBindGroup(0,renderBindings);
  renderPass.SetImmediates(0,&immediates,sizeof(immediates));renderPass.Draw(256*6);renderPass.End();
  const wgpu::TexelCopyTextureInfo texSource{.texture=job->renderTarget};
  const wgpu::TexelCopyBufferInfo texDestination{.layout={.offset=0,.bytesPerRow=256,.rowsPerImage=16},.buffer=job->renderReadback};
  const wgpu::Extent3D extent{16,16,1};
  cmd.CopyTextureToBuffer(&texSource,&texDestination,&extent);
  pending=job;
  std::fprintf(stderr,"GPU_VERTEX_FETCH_QUEUED offsets=256 words=1024\n");
  std::fprintf(stderr,"GPU_VERTEX_RENDER_QUEUED pixels=256 words=1024\n");
  frame.afterSubmitCallbacks.emplace_back([job] {
    job->readback.MapAsync(wgpu::MapMode::Read,0,4096,wgpu::CallbackMode::AllowSpontaneous,
      [job](wgpu::MapAsyncStatus status,wgpu::StringView) {
        job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);
      });
    job->renderReadback.MapAsync(wgpu::MapMode::Read,0,4096,wgpu::CallbackMode::AllowSpontaneous,
      [job](wgpu::MapAsyncStatus status,wgpu::StringView) {
        job->renderStatus.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);
      });
  });
}
}
