#pragma once
#include <atomic>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include "frame_packet.hpp"
#include "resources.hpp"
namespace aurora::gfx::indexed_position_diagnostic {
inline std::mutex mutex;
inline wgpu::RenderPipeline pipeline;
struct Job {wgpu::Texture texture;wgpu::Buffer readback;std::vector<double> expected;std::vector<bool> used;std::vector<uint8_t> indexExpected;unsigned width=64,indices=0,readbackBytes=0;std::atomic<int> status{0};};
inline std::shared_ptr<Job> pending;
inline bool submitted=false;
inline bool needs(uint64_t shader){return std::getenv("AURORA_DIAGNOSTIC_INDEXED_POSITION_SHADER") && shader==648529937ull && !submitted;}
inline void compile(){
  static bool started=false;const char* path=std::getenv("AURORA_DIAGNOSTIC_INDEXED_POSITION_SHADER");
  if(started || !path)return;started=true;
  std::ifstream file(path);std::string code((std::istreambuf_iterator<char>(file)),{});
  if(code.empty()){std::fprintf(stderr,"INDEXED_POSITION_SOURCE_FAIL\n");return;}
  wgpu::ShaderSourceWGSL source{};source.code=code.c_str();
  const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&source};
  const auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
  const auto& res=detail::resources();
  const std::array layouts{res.staticBindGroupLayout,res.uniformBindGroupLayout};
  const wgpu::PipelineLayoutDescriptor layoutDesc{.bindGroupLayoutCount=layouts.size(),.bindGroupLayouts=layouts.data(),.immediateSize=sizeof(gx::DrawImmediateData)};
  const auto layout=webgpu::g_device.CreatePipelineLayout(&layoutDesc);
  const wgpu::ColorTargetState target{.format=wgpu::TextureFormat::RGBA32Float};
  const wgpu::FragmentState fragment{.module=module,.entryPoint="indexed_probe_fragment",.targetCount=1,.targets=&target};
  const wgpu::RenderPipelineDescriptor desc{.label="Original indexed position point probe",.layout=layout,
      .vertex={.module=module,.entryPoint="indexed_probe_vertex"},.primitive={.topology=wgpu::PrimitiveTopology::PointList},.fragment=&fragment};
  webgpu::g_device.CreateRenderPipelineAsync(&desc,wgpu::CallbackMode::AllowSpontaneous,
    [](wgpu::CreatePipelineAsyncStatus status,wgpu::RenderPipeline value,wgpu::StringView){
      if(status==wgpu::CreatePipelineAsyncStatus::Success){std::lock_guard lock(mutex);pipeline=std::move(value);}
      std::fprintf(stderr,"INDEXED_POSITION_COMPILE_%s status=%u\n",status==wgpu::CreatePipelineAsyncStatus::Success?"PASS":"FAIL",unsigned(status));
    });
}
inline void drain(){
  if(!pending)return;const auto job=pending;int status=job->status.load(std::memory_order_acquire);if(!status)return;
  unsigned mismatch=0,components=0,rejected=0;double maximum=0;
  const float* pixels=status==1?static_cast<const float*>(job->readback.GetConstMappedRange(0,job->readbackBytes)):nullptr;
  for(unsigned vertex=0;vertex<job->used.size();++vertex)if(job->used[vertex])for(unsigned c=0;c<4;++c){
    double expected=job->expected[vertex*4+c],actual=pixels?pixels[vertex*4+c]:NAN;
    double tolerance=2e-5*(1+std::abs(expected)),error=std::abs(actual-expected);++components;
    if(!std::isfinite(actual) || error>tolerance)++mismatch;
    if(error>maximum)maximum=error;
    if(std::isfinite(actual) && std::abs(actual-(expected+0.25*(1+std::abs(expected))))>tolerance)++rejected;
  }
  if(!job->indexExpected.empty()) {
    uint64_t hash=14695981039346656037ull;unsigned failures=0;
    const auto* bytes=pixels?reinterpret_cast<const uint8_t*>(pixels)+job->width*16:nullptr;
    for(size_t i=0;i<job->indexExpected.size();++i){const uint8_t actual=bytes?bytes[i]:0;if(!bytes || actual!=job->indexExpected[i])++failures;hash=(hash^actual)*1099511628211ull;}
    const bool negative=bytes && bytes[0]!=(job->indexExpected[0]^1u);
    std::fprintf(stderr,"INDEXED_TOPOLOGY_GPU_%s bytes=%u fnv=%llu mismatches=%u negative_rejections=%u\n",!failures && negative?"PASS":"FAIL",unsigned(job->indexExpected.size()),static_cast<unsigned long long>(hash),failures,unsigned(negative));
  }
  if(status==1)job->readback.Unmap();
  std::fprintf(stderr,"INDEXED_POSITION_ORACLE_%s indices=%u components=%u mismatches=%u negative_rejections=%u max_abs_error=%.9g\n",
               !mismatch && rejected==components?"PASS":"FAIL",job->indices,components,mismatch,rejected,maximum);
  pending.reset();
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame,const gx::DrawData& draw,
                   unsigned vertices,const std::array<double,1024>& expected,unsigned mergedPrimitives){
  if(submitted || draw.diagnosticShader!=648529937ull || !draw.indexCount || vertices>draw.vtxCount || vertices>64)return;
  wgpu::RenderPipeline ready;{std::lock_guard lock(mutex);ready=pipeline;}if(!ready)return;
  const bool full=std::getenv("AURORA_DIAGNOSTIC_INDEXED_POSITION_FULL");
  if(full && (draw.idxRange.offset%4 || !mergedPrimitives || draw.vtxCount<=64 || draw.vtxCount>4096 || draw.indexCount>24576))return;
  const unsigned count=full?draw.indexCount:std::min(draw.indexCount,64u);
  if(draw.idxRange.offset>frame.indices.size() || uint64_t(count)*2>frame.indices.size()-draw.idxRange.offset)return;
  auto job=std::make_shared<Job>();job->indices=count;job->width=full?4096:64;
  if(full) {
    vertices=draw.vtxCount;job->expected.resize(vertices*4);
    const size_t base=draw.uniformRange.offset,matrixOffset=80+size_t(draw.immediateData.currentPnMtx)*48;
    if(draw.immediateData.currentPnMtx>=20 || draw.uniformRange.size<matrixOffset+48 ||
       base>frame.uniforms.size() || matrixOffset+48>frame.uniforms.size()-base)return;
    float model[12],projection[16];std::memcpy(model,frame.uniforms.data()+base+matrixOffset,48);
    std::memcpy(projection,frame.uniforms.data()+base+16,64);
    for(unsigned v=0;v<vertices;++v) {
      const size_t at=size_t(draw.immediateData.vtxStart)+v*8;
      if(at>frame.verts.size() || 2>frame.verts.size()-at)return;
      const auto* packed=frame.verts.data()+at;
      const unsigned index=(unsigned(packed[0])<<8)|unsigned(packed[1]);
      const size_t positionAt=size_t(draw.immediateData.arrayStart[0])+size_t(index)*6;
      if(positionAt>frame.storage.size() || 6>frame.storage.size()-positionAt)return;
      const auto* bytes=frame.storage.data()+positionAt;double position[4]{0,0,0,1};
      for(unsigned c=0;c<3;++c){const unsigned value=(unsigned(bytes[c*2])<<8)|unsigned(bytes[c*2+1]);position[c]=(value>=32768?int(value)-65536:int(value))/64.0;}
      double transformed[4]{0,0,0,1};
      for(unsigned c=0;c<3;++c)for(unsigned k=0;k<4;++k)transformed[c]+=position[k]*model[c*4+k];
      for(unsigned c=0;c<4;++c){double value=0;for(unsigned k=0;k<4;++k)value+=transformed[k]*projection[c*4+k];if(!std::isfinite(value))return;job->expected[v*4+c]=value;}
    }
  } else job->expected.assign(expected.begin(),expected.begin()+vertices*4);
  job->used.resize(vertices,false);
  for(unsigned i=0;i<count;++i){uint16_t index;std::memcpy(&index,frame.indices.data()+draw.idxRange.offset+i*2,2);if(index>=vertices)return;job->used[index]=true;}
  job->readbackBytes=job->width*16;
  if(full) {
    job->indexExpected.assign(frame.indices.data()+draw.idxRange.offset,frame.indices.data()+draw.idxRange.offset+count*2);
    job->readbackBytes+=(count*2+3)&~3u;
    uint64_t hash=14695981039346656037ull;
    if(draw.vertRange.offset>frame.verts.size() || draw.vertRange.size>frame.verts.size()-draw.vertRange.offset)return;
    for(size_t i=0;i<draw.vertRange.size;++i)hash=(hash^frame.verts.data()[draw.vertRange.offset+i])*1099511628211ull;
    std::fprintf(stderr,"INDEXED_TOPOLOGY_VERTEX bytes=%u fnv=%llu\n",draw.vertRange.size,static_cast<unsigned long long>(hash));
  }
  const wgpu::TextureDescriptor td{.usage=wgpu::TextureUsage::RenderAttachment|wgpu::TextureUsage::CopySrc,
      .size={job->width,1,1},.format=wgpu::TextureFormat::RGBA32Float};job->texture=webgpu::g_device.CreateTexture(&td);
  const wgpu::BufferDescriptor bd{.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=job->readbackBytes};job->readback=webgpu::g_device.CreateBuffer(&bd);
  const wgpu::RenderPassColorAttachment color{.view=job->texture.CreateView(),.loadOp=wgpu::LoadOp::Clear,.storeOp=wgpu::StoreOp::Store,.clearValue={-9999,-9999,-9999,-9999}};
  const wgpu::RenderPassDescriptor pd{.colorAttachmentCount=1,.colorAttachments=&color};auto pass=cmd.BeginRenderPass(&pd);
  const auto& res=detail::resources();pass.SetPipeline(ready);pass.SetBindGroup(0,res.staticBindGroup);
  pass.SetBindGroup(1,res.uniformBindGroup,1,&draw.uniformRange.offset);pass.SetImmediates(0,&draw.immediateData,sizeof(draw.immediateData));
  pass.SetIndexBuffer(res.indexBuffer,wgpu::IndexFormat::Uint16,draw.idxRange.offset,draw.idxRange.size);
  pass.DrawIndexed(count);pass.End();
  const wgpu::TexelCopyTextureInfo src{.texture=job->texture};const wgpu::TexelCopyBufferInfo dst{.layout={.bytesPerRow=job->width*16,.rowsPerImage=1},.buffer=job->readback};
  const wgpu::Extent3D size{job->width,1,1};cmd.CopyTextureToBuffer(&src,&dst,&size);
  if(full)cmd.CopyBufferToBuffer(res.indexBuffer,draw.idxRange.offset,job->readback,job->width*16,(count*2+3)&~3u);
  frame.afterSubmitCallbacks.emplace_back([job]{job->readback.MapAsync(wgpu::MapMode::Read,0,job->readbackBytes,wgpu::CallbackMode::AllowSpontaneous,
    [job](wgpu::MapAsyncStatus status,wgpu::StringView){job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);});});
  pending=job;submitted=true;std::fprintf(stderr,"INDEXED_POSITION_QUEUED shader=648529937 vertices=%u indices=%u bytes=%u merged_primitives=%u full=%u\n",vertices,count,job->width*16,mergedPrimitives,unsigned(full));
}
}
