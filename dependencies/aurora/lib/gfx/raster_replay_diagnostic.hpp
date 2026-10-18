#pragma once
#include "../gx/gx.hpp"
#include "../webgpu/gpu.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <mutex>
#include <string>
#include <atomic>
#include <cstring>
#include <set>
#include <memory>
#include "frame_packet.hpp"
#include "resources.hpp"
#include "resource_cache.hpp"
#include "raster_reference.hpp"
#include "texture_sample_reference.hpp"
#include "occlusion_pair_diagnostic.hpp"
namespace aurora::gfx::raster_replay_diagnostic {
inline bool depthWriteReplay() { return std::getenv("AURORA_DIAGNOSTIC_RASTER_DEPTH_WRITE") != nullptr; }
inline bool depthReplay() { return std::getenv("AURORA_DIAGNOSTIC_RASTER_DEPTH_REJECT") != nullptr; }
inline bool stateReplay() { return std::getenv("AURORA_DIAGNOSTIC_RASTER_STATE") != nullptr; }
inline bool productionFormat() { return std::getenv("AURORA_DIAGNOSTIC_PRODUCTION_FORMAT") != nullptr; }
inline wgpu::TextureFormat colourFormat() { return productionFormat()?webgpu::g_graphicsConfig.surfaceConfiguration.format:wgpu::TextureFormat::RGBA32Float; }
inline std::mutex pipelineMutex;
inline std::map<uint64_t,wgpu::RenderPipeline> pipelines;
inline bool ready(uint64_t shader) { const std::lock_guard lock(pipelineMutex);return pipelines.contains(shader); }
// A separate, explicitly diagnostic raster pipeline. No change to production
// rendering state; depth, blend, cull and MSAA acceptance must be separate.
inline void compile() {
  static bool started=false;
  const char* directory=std::getenv("AURORA_DIAGNOSTIC_RASTER_REPLAY_DIRECTORY");
  if(started || !directory || !*directory || !std::getenv("AURORA_DIAGNOSTIC_DRAW_UV")) return;
  started=true;
  const std::array entries{
    wgpu::BindGroupLayoutEntry{.binding=0,.visibility=wgpu::ShaderStage::Vertex,.buffer={.type=wgpu::BufferBindingType::ReadOnlyStorage}},
    wgpu::BindGroupLayoutEntry{.binding=1,.visibility=wgpu::ShaderStage::Vertex,.buffer={.type=wgpu::BufferBindingType::ReadOnlyStorage}}};
  const wgpu::BindGroupLayoutDescriptor inputDesc{.entryCount=entries.size(),.entries=entries.data()};
  const auto input=webgpu::g_device.CreateBindGroupLayout(&inputDesc);
  const wgpu::BindGroupLayoutEntry uniform{.binding=0,.visibility=wgpu::ShaderStage::Vertex|wgpu::ShaderStage::Fragment,.buffer={.type=wgpu::BufferBindingType::Uniform,.hasDynamicOffset=true}};
  const wgpu::BindGroupLayoutDescriptor uniformDesc{.entryCount=1,.entries=&uniform};
  const auto uniforms=webgpu::g_device.CreateBindGroupLayout(&uniformDesc);
  const std::array layouts{input,uniforms,gx::diagnostic_texture_bind_group_layout()};
  const wgpu::PipelineLayoutDescriptor layoutDesc{.bindGroupLayoutCount=layouts.size(),.bindGroupLayouts=layouts.data(),.immediateSize=sizeof(gx::DrawImmediateData)};
  const auto layout=webgpu::g_device.CreatePipelineLayout(&layoutDesc);
  if(productionFormat() && colourFormat()!=wgpu::TextureFormat::RGBA8Unorm && colourFormat()!=wgpu::TextureFormat::BGRA8Unorm) {
    std::fprintf(stderr,"PRODUCTION_FORMAT_REFERENCE_REFUSED\n");return;
  }
  wgpu::ColorTargetState target{.format=colourFormat()};
  if(stateReplay()) target.writeMask=wgpu::ColorWriteMask::Red|wgpu::ColorWriteMask::Green|wgpu::ColorWriteMask::Blue;
  const wgpu::DepthStencilState depthState{.format=webgpu::g_graphicsConfig.depthFormat,.depthWriteEnabled=true,.depthCompare=wgpu::CompareFunction::GreaterEqual};
  std::error_code error;unsigned count=0;
  for(const auto& entry:std::filesystem::directory_iterator(directory,error)) {
    if(count>=2 || entry.path().extension()!=L".wgsl") continue;
    const auto name=entry.path().filename().string();
    std::ifstream file(entry.path(),std::ios::binary);
    std::string code((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    if(code.empty() || code.size()>1024*1024) {std::fprintf(stderr,"RASTER_REPLAY_SOURCE_FAIL file=%s\n",name.c_str());continue;}
    wgpu::ShaderSourceWGSL source{};source.code=code.c_str();
    const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&source,.label="Original draw diagnostic raster shader"};
    const auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
    const wgpu::FragmentState fragment{.module=module,.entryPoint="fs_main",.targetCount=1,.targets=&target};
    wgpu::RenderPipelineDescriptor descriptor{.label="Original draw diagnostic raster pipeline",.layout=layout,.vertex={.module=module,.entryPoint="vs_main"},.primitive={.topology=wgpu::PrimitiveTopology::TriangleList,.cullMode=wgpu::CullMode::None},.fragment=&fragment};
    if(stateReplay()) { descriptor.primitive.frontFace=wgpu::FrontFace::CW; descriptor.primitive.cullMode=wgpu::CullMode::Back; descriptor.depthStencil=&depthState; }
    webgpu::g_device.CreateRenderPipelineAsync(&descriptor,wgpu::CallbackMode::AllowSpontaneous,
      [name](wgpu::CreatePipelineAsyncStatus status,wgpu::RenderPipeline pipeline,wgpu::StringView) {
        if(status==wgpu::CreatePipelineAsyncStatus::Success) {
          const auto shader=std::strtoull(name.c_str()+3,nullptr,16);
          const std::lock_guard lock(pipelineMutex);pipelines.emplace(shader,std::move(pipeline));
        }
        std::fprintf(stderr,"RASTER_REPLAY_COMPILE_%s file=%s status=%u\n",status==wgpu::CreatePipelineAsyncStatus::Success?"PASS":"FAIL",name.c_str(),unsigned(status));
      });
    ++count;
  }
  std::fprintf(stderr,"RASTER_REPLAY_COMPILE_QUEUED shaders=%u directory_error=%u scope=compilation_only\n",count,unsigned(bool(error)));
}
inline std::set<uint64_t> queued, queuedFront, queuedBack;
inline std::map<uint64_t,unsigned> attempts;
inline constexpr unsigned TargetSize=256,ReadbackBytes=TargetSize*TargetSize*16;
struct PixelJob {
  uint64_t shader=0;
  bool culled=false, depthRejected=false, occlusion=false, wholeOnly=false;
  unsigned order=0;
  std::vector<std::array<double,4>> farColour;
  std::vector<double> farDepth;
  double depthMin=1,depthMax=0;
  wgpu::Texture target, depth;
  wgpu::Buffer readback;
  uint32_t readBytes=ReadbackBytes;
  uint32_t colourBytes=ReadbackBytes;
  wgpu::TextureFormat format=wgpu::TextureFormat::RGBA32Float;
  std::vector<double> expectedDepth;
  std::atomic<int> status{0};
  std::vector<unsigned> offsets;
  std::vector<std::array<double,4>> expected;
  std::array<double,2> uvMin{},uvMax{};
  double clipWRatio=0;
};
inline std::vector<std::shared_ptr<PixelJob>> jobs;
inline bool needs(uint64_t shader) {return std::getenv("AURORA_DIAGNOSTIC_RASTER_REPLAY_DISPATCH") && (stateReplay()?(!queuedFront.contains(shader) || (!depthReplay() && !queuedBack.contains(shader))):!queued.contains(shader)) && attempts[shader]<128;}
inline void drain() {
  for(auto it=jobs.begin();it!=jobs.end();) {
    const auto job=*it;const int status=job->status.load(std::memory_order_acquire);
    if(!status) {++it;continue;}
    unsigned mismatch=0,rejections=0,invalid=0,depthBad=0,farColourRejected=0,farDepthRejected=0;double maxError=0;
    const auto* values=status==1?static_cast<const float*>(job->readback.GetConstMappedRange(0,job->readBytes)):nullptr;
    if(!values) ++invalid;
    else {
      auto colour=[&](unsigned offset,unsigned channel)->double {
        if(job->format==wgpu::TextureFormat::RGBA32Float)return values[offset+channel];
        const auto* bytes=reinterpret_cast<const uint8_t*>(values);
        const unsigned mapped=job->format==wgpu::TextureFormat::BGRA8Unorm && channel<3?2-channel:channel;
        return double(bytes[offset+mapped])/255.;
      };
      if(job->occlusion) for(unsigned p=0;p<job->offsets.size();++p)
        std::fprintf(stderr,"REPLAY_PIXEL_REFERENCE shader=%llu order=%u pixel=%u x=%u y=%u actual_rgb=%.9g,%.9g,%.9g expected_rgb=%.9g,%.9g,%.9g\n",
          static_cast<unsigned long long>(job->shader),job->order,p,
          (job->offsets[p]/4)%TargetSize,(job->offsets[p]/4)/TargetSize,
          colour(job->offsets[p],0),colour(job->offsets[p],1),colour(job->offsets[p],2),
          job->expected[p][0],job->expected[p][1],job->expected[p][2]);
      for(unsigned p=0;p<job->offsets.size();++p) for(unsigned c=0;c<4;++c) {
        const double actual=colour(job->offsets[p],c),expected=job->expected[p][c];
        const double error=std::abs(actual-expected);maxError=std::max(maxError,error);
        if(job->occlusion && (!std::isfinite(actual) || error>2.0/255))
          std::fprintf(stderr,"REPLAY_COLOUR_MISMATCH shader=%llu order=%u pixel=%u x=%u y=%u channel=%u actual=%.9g expected=%.9g abs_error=%.9g tolerance=%.9g\n",
            static_cast<unsigned long long>(job->shader),job->order,p,
            (job->offsets[p]/4)%TargetSize,(job->offsets[p]/4)/TargetSize,c,actual,expected,error,2.0/255);
        if(!std::isfinite(actual) || (!(job->culled || job->depthRejected) && (actual<0 || actual>1))) ++invalid;
        if(!std::isfinite(actual) || error>((job->culled || job->depthRejected)?0:2.0/255)) ++mismatch;
        if(std::abs(actual-(expected+0.125))>2.0/255) ++rejections;
      }
      if(depthWriteReplay()) {
        const auto* depths=values+job->colourBytes/4;
        unsigned bad=0,negative=0;double maximum=0;
        for(unsigned p=0;p<job->offsets.size();++p) {
          const double actual=depths[job->offsets[p]/4],expected=job->expectedDepth[p];
          const double error=std::abs(actual-expected);maximum=std::max(maximum,error);
          const double tolerance=(job->culled || job->depthRejected)?0:2e-7;
          if(!std::isfinite(actual) || actual<0 || actual>1 || error>tolerance) ++bad;
          if(std::abs(actual-(expected+0.01))>tolerance) ++negative;
        }
        depthBad=bad;
        if(job->occlusion) for(unsigned p=0;p<job->offsets.size();++p) {
          double difference=0;for(unsigned c=0;c<3;++c)difference=std::max(difference,std::abs(colour(job->offsets[p],c)-job->farColour[p][c]));
          if(difference>2./255)++farColourRejected;
          if(std::abs(double(depths[job->offsets[p]/4])-job->farDepth[p])>2e-7)++farDepthRejected;
        }
        if(!job->occlusion) std::fprintf(stderr,"RASTER_REPLAY_DEPTH_WRITE_%s shader=%llu culled=%u pixels=%zu mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g scope=stored_depth_original_triangle\n",bad?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),unsigned(job->culled),job->offsets.size(),bad,negative,maximum);
      }
      job->readback.Unmap();
    }
    if(job->occlusion) {
      const bool failed=invalid || mismatch || depthBad || farColourRejected!=job->offsets.size() || farDepthRejected!=job->offsets.size();
      if(job->wholeOnly) {
        std::fprintf(stderr,"WHOLE_DRAW_PIXEL_%s shader=%llu pixels=%zu colour_mismatches=%u depth_mismatches=%u invalid=%u altered_colour_rejections=%u altered_depth_rejections=%u scope=complete_original_draw\n",failed?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),job->offsets.size(),mismatch,depthBad,invalid,farColourRejected,farDepthRejected);
        it=jobs.erase(it);continue;
      }
      std::fprintf(stderr,"OCCLUSION_PIXEL_%s shader=%llu order=%u pixels=%zu colour_mismatches=%u depth_mismatches=%u invalid=%u far_colour_rejections=%u far_depth_rejections=%u scope=original_pair_lod0\n",failed?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),job->order,job->offsets.size(),mismatch,depthBad,invalid,farColourRejected,farDepthRejected);
      it=jobs.erase(it);continue;
    }
    std::fprintf(stderr,"RASTER_REPLAY_PIXEL_%s shader=%llu pixels=%zu components=%zu invalid=%u mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g scope=isolated_lod0_triangle\n",invalid || mismatch?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),job->offsets.size(),job->offsets.size()*4,invalid,mismatch,rejections,maxError);
    if(stateReplay()) std::fprintf(stderr,"RASTER_REPLAY_STATE_PIXEL_%s shader=%llu culled=%u pixels=%zu mismatches=%u invalid=%u scope=pinned_original_triangle\n",invalid || mismatch?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),unsigned(job->culled),job->offsets.size(),mismatch,invalid);
    if(job->depthRejected) std::fprintf(stderr,"RASTER_REPLAY_DEPTH_REJECT_%s shader=%llu pixels=%zu depth_min=%.9g depth_max=%.9g clear_depth=1 mismatches=%u scope=original_front_triangle\n",invalid || mismatch?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),job->offsets.size(),job->depthMin,job->depthMax,mismatch);
    double colourRange=0;
    for(unsigned c=0;c<4;++c) {
      double low=job->expected.front()[c],high=low;
      for(const auto& pixel:job->expected) {low=std::min(low,pixel[c]);high=std::max(high,pixel[c]);}
      colourRange=std::max(colourRange,high-low);
    }
    std::fprintf(stderr,"RASTER_REPLAY_COVERAGE shader=%llu coordinate_range=%.9g expected_colour_range=%.9g clip_w_ratio=%.9g scope=sampled_triangle_only\n",static_cast<unsigned long long>(job->shader),std::max(job->uvMax[0]-job->uvMin[0],job->uvMax[1]-job->uvMin[1]),colourRange,job->clipWRatio);
    it=jobs.erase(it);
  }
}
inline void submit(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame,const detail::DiagnosticReplayDraw& snapshot,
                   const wgpu::RenderPipeline& pipeline,const wgpu::BindGroup& textureBind,
                   const std::shared_ptr<PixelJob>& job,const std::array<unsigned,2>& triangles,unsigned count) {
    const auto& draw=snapshot.draw;
    if(std::getenv("AURORA_DIAGNOSTIC_PRODUCTION_SAMPLING")) {
      const auto& sampler=snapshot.sampler;
      const size_t offset=(draw.diagnosticShader==0xdff4b88bull?1568:1520)+960+128,base=draw.uniformRange.offset;
      float bias=1;
      if(draw.uniformRange.size>=offset+16 && base+offset+16<=frame.uniforms.size())std::memcpy(&bias,frame.uniforms.data()+base+offset+8,4);
      if(snapshot.mipCount!=1 || sampler.lodMinClamp!=0 || sampler.lodMaxClamp!=0 || bias!=0 || sampler.magFilter!=wgpu::FilterMode::Linear || sampler.minFilter!=wgpu::FilterMode::Linear || sampler.maxAnisotropy!=1) {
        std::fprintf(stderr,"PRODUCTION_SAMPLING_REFERENCE_REFUSED shader=%llu\n",static_cast<unsigned long long>(draw.diagnosticShader));return;
      }
    }
    if(std::getenv("AURORA_DIAGNOSTIC_MIP_METADATA")) {
      const size_t offset=(draw.diagnosticShader==0xdff4b88bull?1568:1520)+960+128,base=draw.uniformRange.offset;
      if(draw.uniformRange.size>=offset+16 && base+offset+16<=frame.uniforms.size()) {
        float sizeBias[4];std::memcpy(sizeBias,frame.uniforms.data()+base+offset,16);
        const auto& sampler=snapshot.sampler;
        std::fprintf(stderr,"REPLAY_JOB_MIP_METADATA shader=%llu pair=%u order=%u culled=%u mips=%u width=%u height=%u mag=%u min=%u mip_filter=%u lod_min=%.9g lod_max=%.9g bias=%.9g aniso=%u scope=exact_submitted_binding\n",static_cast<unsigned long long>(draw.diagnosticShader),unsigned(job->occlusion),job->order,unsigned(job->culled),snapshot.mipCount,snapshot.width,snapshot.height,unsigned(sampler.magFilter),unsigned(sampler.minFilter),unsigned(sampler.mipmapFilter),double(sampler.lodMinClamp),double(sampler.lodMaxClamp),double(sizeBias[2]),unsigned(sampler.maxAnisotropy));
      }
    }
    job->format=colourFormat();
    if(productionFormat()) {
      job->colourBytes=TargetSize*TargetSize*4;job->readBytes=job->colourBytes;
      auto quantize=[](auto& pixels){for(auto& pixel:pixels)for(auto& channel:pixel)channel=std::floor(std::clamp(channel,0.,1.)*255.+.5)/255.;};
      quantize(job->expected);quantize(job->farColour);
      std::fprintf(stderr,"PRODUCTION_FORMAT_JOB shader=%llu pair=%u order=%u culled=%u format=%u colour_bytes=%u scope=original_binding_unorm_target\n",static_cast<unsigned long long>(draw.diagnosticShader),unsigned(job->occlusion),job->order,unsigned(job->culled),unsigned(job->format),job->colourBytes);
    }
    const wgpu::TextureDescriptor textureDesc{.usage=wgpu::TextureUsage::RenderAttachment|wgpu::TextureUsage::CopySrc,.dimension=wgpu::TextureDimension::e2D,.size={TargetSize,TargetSize,1},.format=job->format,.mipLevelCount=1,.sampleCount=1};
    job->target=webgpu::g_device.CreateTexture(&textureDesc);
    if(depthWriteReplay())job->readBytes+=TargetSize*TargetSize*4;
    const wgpu::BufferDescriptor readDesc{.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=job->readBytes};job->readback=webgpu::g_device.CreateBuffer(&readDesc);
    const auto& resources=detail::resources();
    const std::array inputs{wgpu::BindGroupEntry{.binding=0,.buffer=resources.vertexBuffer,.size=VertexBufferSize},wgpu::BindGroupEntry{.binding=1,.buffer=resources.storageBuffer,.size=StorageBufferSize}};
    const wgpu::BindGroupDescriptor inputDesc{.layout=pipeline.GetBindGroupLayout(0),.entryCount=inputs.size(),.entries=inputs.data()};const auto input=webgpu::g_device.CreateBindGroup(&inputDesc);
    const wgpu::BindGroupEntry uniform{.binding=0,.buffer=resources.uniformBuffer,.size=draw.uniformRange.size};
    const wgpu::BindGroupDescriptor uniformDesc{.layout=pipeline.GetBindGroupLayout(1),.entryCount=1,.entries=&uniform};const auto uniforms=webgpu::g_device.CreateBindGroup(&uniformDesc);
    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    if(stateReplay()) {
      const wgpu::TextureDescriptor depthDesc{.usage=wgpu::TextureUsage::RenderAttachment|(depthWriteReplay()?wgpu::TextureUsage::CopySrc:wgpu::TextureUsage::None),.dimension=wgpu::TextureDimension::e2D,.size={TargetSize,TargetSize,1},.format=webgpu::g_graphicsConfig.depthFormat,.mipLevelCount=1,.sampleCount=1};
      job->depth=webgpu::g_device.CreateTexture(&depthDesc);
      depthAttachment.view=job->depth.CreateView(); depthAttachment.depthLoadOp=wgpu::LoadOp::Clear; depthAttachment.depthStoreOp=wgpu::StoreOp::Store; depthAttachment.depthClearValue=depthReplay()?1:0;
    }
    const wgpu::RenderPassColorAttachment attachment{.view=job->target.CreateView(),.loadOp=wgpu::LoadOp::Clear,.storeOp=wgpu::StoreOp::Store,.clearValue={-1,-1,-1,stateReplay()?0.25:-1}};
    const wgpu::RenderPassDescriptor passDesc{.colorAttachmentCount=1,.colorAttachments=&attachment,.depthStencilAttachment=stateReplay()?&depthAttachment:nullptr};const auto pass=cmd.BeginRenderPass(&passDesc);
    pass.SetPipeline(pipeline);pass.SetBindGroup(0,input);pass.SetBindGroup(1,uniforms,1,&draw.uniformRange.offset);pass.SetBindGroup(2,textureBind);pass.SetImmediates(0,&draw.immediateData,sizeof(draw.immediateData));
    pass.SetViewport(0,0,TargetSize,TargetSize,0,1);pass.SetScissorRect(0,0,TargetSize,TargetSize);
    if(job->order==2) {pass.SetIndexBuffer(resources.indexBuffer,wgpu::IndexFormat::Uint16,draw.idxRange.offset,draw.idxRange.size);pass.DrawIndexed(draw.indexCount);}
    else for(unsigned n=0;n<count;++n) {
      if(draw.indexCount) {pass.SetIndexBuffer(resources.indexBuffer,wgpu::IndexFormat::Uint16,draw.idxRange.offset+triangles[n]*6,6);pass.DrawIndexed(3);} else pass.Draw(3);
    }
    pass.End();
    const wgpu::TexelCopyTextureInfo source{.texture=job->target};
    const wgpu::TexelCopyBufferInfo destination{.layout={.bytesPerRow=job->colourBytes/TargetSize,.rowsPerImage=TargetSize},.buffer=job->readback};const wgpu::Extent3D extent{TargetSize,TargetSize,1};cmd.CopyTextureToBuffer(&source,&destination,&extent);
    if(depthWriteReplay()) {
      const wgpu::TexelCopyTextureInfo depthSource{.texture=job->depth,.aspect=wgpu::TextureAspect::DepthOnly};
      const wgpu::TexelCopyBufferInfo depthDestination{.layout={.offset=job->colourBytes,.bytesPerRow=TargetSize*4,.rowsPerImage=TargetSize},.buffer=job->readback};
      cmd.CopyTextureToBuffer(&depthSource,&depthDestination,&extent);
    }
    frame.afterSubmitCallbacks.emplace_back([job]{job->readback.MapAsync(wgpu::MapMode::Read,0,job->readBytes,wgpu::CallbackMode::AllowSpontaneous,[job](wgpu::MapAsyncStatus status,wgpu::StringView){job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);});});
    jobs.push_back(job);
}
inline bool encodePair(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame,
                       const detail::DiagnosticReplayDraw& snapshot,const occlusion_pair_diagnostic::Pair& pair) {
  if(!std::getenv("AURORA_DIAGNOSTIC_OCCLUSION_GPU"))return true;
  if(!stateReplay() || !depthWriteReplay() || depthReplay() || webgpu::g_graphicsConfig.depthFormat!=wgpu::TextureFormat::Depth32Float)return false;
  const std::array<uint32_t,16> supported{1,1,1,3,2,0,4,5,15,1,0,UINT32_MAX,0,0,0,gx::GXPipelineConfigVersion};
  if(snapshot.state!=supported)return false;
  wgpu::RenderPipeline pipeline;
  {const std::lock_guard lock(pipelineMutex);auto it=pipelines.find(snapshot.draw.diagnosticShader);if(it==pipelines.end())return false;pipeline=it->second;}
  const auto textureBind=find_bind_group(snapshot.draw.bindGroups.textureBindGroup);if(!textureBind)return false;
  if(pair.wholeOnly) {
    auto job=std::make_shared<PixelJob>();job->shader=snapshot.draw.diagnosticShader;job->occlusion=true;job->wholeOnly=true;job->order=2;
    job->offsets=pair.offsets;job->expected=pair.nearestColour;job->expectedDepth=pair.nearestDepth;job->farColour=pair.farColour;job->farDepth=pair.farDepth;
    submit(cmd,frame,snapshot,pipeline,textureBind,job,{},0);
    std::fprintf(stderr,"WHOLE_DRAW_QUEUED shader=%llu indices=%u vertices=%u pixels=%zu scope=complete_original_indexed_draw vertex_stream_hash=%llu vertex_stream_bytes=%u uniform_hash=%llu uniform_bytes=%u texture_hash=%llu\n",static_cast<unsigned long long>(snapshot.draw.diagnosticShader),snapshot.draw.indexCount,snapshot.draw.vtxCount,pair.offsets.size(),static_cast<unsigned long long>(snapshot.vertexStreamHash),snapshot.vertexStreamBytes,static_cast<unsigned long long>(snapshot.uniformHash),snapshot.uniformBytes,static_cast<unsigned long long>(snapshot.textureHash));
    return true;
  }
  for(unsigned order=0;order<2;++order) {
    auto job=std::make_shared<PixelJob>();job->shader=snapshot.draw.diagnosticShader;job->occlusion=true;job->order=order;
    job->offsets=pair.offsets;job->expected=pair.nearestColour;job->expectedDepth=pair.nearestDepth;job->farColour=pair.farColour;job->farDepth=pair.farDepth;
    submit(cmd,frame,snapshot,pipeline,textureBind,job,order?std::array<unsigned,2>{pair.second,pair.first}:std::array<unsigned,2>{pair.first,pair.second},2);
  }
  if(std::getenv("AURORA_DIAGNOSTIC_WHOLE_DRAW") && pair.wholeDrawVisible) {
    auto job=std::make_shared<PixelJob>();job->shader=snapshot.draw.diagnosticShader;job->occlusion=true;job->order=2;
    job->offsets=pair.offsets;job->expected=pair.nearestColour;job->expectedDepth=pair.nearestDepth;job->farColour=pair.farColour;job->farDepth=pair.farDepth;
    submit(cmd,frame,snapshot,pipeline,textureBind,job,{},0);
    std::fprintf(stderr,"WHOLE_DRAW_QUEUED shader=%llu indices=%u vertices=%u pixels=%zu scope=complete_original_indexed_draw vertex_stream_hash=%llu vertex_stream_bytes=%u uniform_hash=%llu uniform_bytes=%u texture_hash=%llu\n",static_cast<unsigned long long>(snapshot.draw.diagnosticShader),snapshot.draw.indexCount,snapshot.draw.vtxCount,pair.offsets.size(),static_cast<unsigned long long>(snapshot.vertexStreamHash),snapshot.vertexStreamBytes,static_cast<unsigned long long>(snapshot.uniformHash),snapshot.uniformBytes,static_cast<unsigned long long>(snapshot.textureHash));
  }
  std::fprintf(stderr,"OCCLUSION_GPU_QUEUED shader=%llu first=%u second=%u orders=2 pixels=%zu scope=original_pair_buffers\n",static_cast<unsigned long long>(snapshot.draw.diagnosticShader),pair.first,pair.second,pair.offsets.size());return true;
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame,
                   const detail::DiagnosticReplayDraw& snapshot,unsigned vertices,
                   const std::array<double,1024>& positions,const std::array<float,768>& coords,
                   bool normal,const std::array<int,768>& first,int factor,
                   const std::array<double,1024>& material) {
  const auto& draw=snapshot.draw;
  if(depthWriteReplay() && webgpu::g_graphicsConfig.depthFormat!=wgpu::TextureFormat::Depth32Float) {
    std::fprintf(stderr,"RASTER_REPLAY_DEPTH_FORMAT_REFUSED\n");return;
  }
  if(!needs(draw.diagnosticShader) || !snapshot.pixels) return;
  if(stateReplay()) {
    const std::array<uint32_t,16> supported{1,1,1,3,2,0,4,5,15,1,0,UINT32_MAX,0,0,0,gx::GXPipelineConfigVersion};
    if(snapshot.state!=supported) {
      if(queued.insert(draw.diagnosticShader).second) std::fprintf(stderr,"RASTER_REPLAY_STATE_REFUSED shader=%llu scope=unsupported_captured_state\n",static_cast<unsigned long long>(draw.diagnosticShader));
      return;
    }
  }
  wgpu::RenderPipeline pipeline;
  {const std::lock_guard lock(pipelineMutex);auto it=pipelines.find(draw.diagnosticShader);if(it==pipelines.end())return;pipeline=it->second;}
  const unsigned attempt=++attempts[draw.diagnosticShader];
  const auto textureBind=find_bind_group(draw.bindGroups.textureBindGroup);if(!textureBind)return;
  const auto& sampler=snapshot.sampler;
  using texture_sample_reference::Wrap;
  auto wrap=[](wgpu::AddressMode mode,Wrap& result) {
    if(mode==wgpu::AddressMode::ClampToEdge)result=Wrap::Clamp;
    else if(mode==wgpu::AddressMode::Repeat)result=Wrap::Repeat;
    else if(mode==wgpu::AddressMode::MirrorRepeat)result=Wrap::Mirror;
    else return false;
    return true;
  };
  Wrap s{},t{};
  if(!wrap(sampler.addressModeU,s) || !wrap(sampler.addressModeV,t) || sampler.maxAnisotropy!=1 || sampler.lodMinClamp!=0 || sampler.magFilter!=sampler.minFilter)return;
  const auto filter=sampler.magFilter==wgpu::FilterMode::Linear?texture_sample_reference::Filter::Linear:texture_sample_reference::Filter::Nearest;
  const size_t scaleOffset=(normal?1568:1520)+960,base=draw.uniformRange.offset;
  if(draw.uniformRange.size<scaleOffset+144 || base+scaleOffset+144>frame.uniforms.size())return;
  float scales[2],dimensions[2];std::memcpy(scales,frame.uniforms.data()+base+scaleOffset,8);std::memcpy(dimensions,frame.uniforms.data()+base+scaleOffset+128,8);
  if(!std::isfinite(scales[0]) || !std::isfinite(scales[1]) || !std::isfinite(dimensions[0]) || !std::isfinite(dimensions[1]) || dimensions[0]<=0 || dimensions[1]<=0)return;
  auto byte=[](double value){return std::clamp(int(std::floor(value*255+0.5)),0,255);};
  const unsigned triangles=draw.indexCount?std::min(draw.indexCount/3,stateReplay()?64u:16u):1;
  for(unsigned triangle=0;triangle<triangles;++triangle) {
    unsigned index[3]{0,1,2};
    if(draw.indexCount) {
      const size_t at=size_t(draw.idxRange.offset)+triangle*6;
      if(triangle*6+6>draw.idxRange.size || at+6>frame.indices.size())return;
      uint16_t encoded[3];std::memcpy(encoded,frame.indices.data()+at,6);
      for(unsigned n=0;n<3;++n)index[n]=encoded[n];
    }
    if(index[0]>=vertices || index[1]>=vertices || index[2]>=vertices)continue;
    std::array<std::array<double,4>,3> clip{};
    for(unsigned n=0;n<3;++n) for(unsigned c=0;c<4;++c)clip[n][c]=positions[index[n]*4+c];
    bool clockwise=false;
    if(stateReplay() && !raster_reference::clockwise(clip,clockwise)) continue;
    if(depthReplay() && !clockwise) continue;
    if(stateReplay() && (clockwise?queuedFront:queuedBack).contains(draw.diagnosticShader)) continue;
    auto job=std::make_shared<PixelJob>();job->shader=draw.diagnosticShader;
    job->culled=stateReplay() && !clockwise;
    job->depthRejected=depthReplay();
    const double minW=std::min({clip[0][3],clip[1][3],clip[2][3]}),maxW=std::max({clip[0][3],clip[1][3],clip[2][3]});
    if(minW>0)job->clipWRatio=maxW/minW;
    bool valid=true;
    for(unsigned y=0;y<TargetSize && job->offsets.size()<16 && valid;++y) for(unsigned x=0;x<TargetSize && job->offsets.size()<16 && valid;++x) {
      std::array<double,3> weights{};double depth=0;
      if(!raster_reference::weights(clip,TargetSize,TargetSize,x+0.5,y+0.5,weights,depth))continue;
      if(depthReplay() && depth>=1-1e-5) continue;
      job->depthMin=std::min(job->depthMin,depth);job->depthMax=std::max(job->depthMax,depth);
      double coordinate[3]{},colour[4]{};
      for(unsigned n=0;n<3;++n) {
        for(unsigned c=0;c<(normal?3u:2u);++c)coordinate[c]+=weights[n]*coords[index[n]*(normal?3:2)+c];
        for(unsigned c=0;c<3;++c)colour[c]+=weights[n]*first[index[n]*3+c]/255.0;
        colour[3]+=weights[n]*material[index[n]*4+3];
      }
      const double q=normal && coordinate[2]!=0?coordinate[2]:1;
      std::array<double,4> sampled{};
      valid=texture_sample_reference::sample(snapshot.pixels->data(),snapshot.pixels->size(),snapshot.width,snapshot.height,
          coordinate[0]/q*scales[0]/dimensions[0],coordinate[1]/q*scales[1]/dimensions[1],s,t,filter,sampled);
      if(!valid)break;
      std::array<double,4> expected{};
      for(unsigned c=0;c<3;++c) {
        const int tex=byte(sampled[c]),a=normal?byte(colour[c]):0,b=normal?tex:byte(colour[c]);
        const int f=normal?factor:tex,expanded=f+f/128;
        expected[c]=double((a*(256-expanded)+b*expanded+128)/256)/255;
      }
      expected[3]=stateReplay()?0.25:double(byte(colour[3]))/255;
      if(job->culled || job->depthRejected) expected={-1,-1,-1,0.25};
      for(unsigned c=0;c<2;++c) {
        const double uv=coordinate[c]/q*scales[c]/dimensions[c];
        if(job->offsets.empty())job->uvMin[c]=job->uvMax[c]=uv;
        else {job->uvMin[c]=std::min(job->uvMin[c],uv);job->uvMax[c]=std::max(job->uvMax[c],uv);}
      }
      job->offsets.push_back((y*TargetSize+x)*4);job->expected.push_back(expected);
      job->expectedDepth.push_back(job->culled?0:job->depthRejected?1:depth);
    }
    if(!valid || job->offsets.size()<4)continue;
    if(stateReplay()) std::fprintf(stderr,"RASTER_REPLAY_WINDING shader=%llu triangle=%u clockwise=%u expected_culled=%u scope=original_projected_triangle\n",static_cast<unsigned long long>(draw.diagnosticShader),triangle,unsigned(clockwise),unsigned(!clockwise));

    submit(cmd,frame,snapshot,pipeline,textureBind,job,{triangle,0},1);
    queued.insert(draw.diagnosticShader);
    if(stateReplay()) (clockwise?queuedFront:queuedBack).insert(draw.diagnosticShader);
    if(stateReplay()) std::fprintf(stderr,"RASTER_REPLAY_STATE_APPLIED shader=%llu reversed_z=1 cull_back=1 rgb_only=1 depth_clear=%u alpha_clear=0.25 scope=pinned_single_triangle\n",static_cast<unsigned long long>(draw.diagnosticShader),unsigned(depthReplay()));
    std::fprintf(stderr,"RASTER_REPLAY_DRAW_QUEUED shader=%llu triangle=%u indexed=%u pixels=%zu scope=isolated_original_buffers\n",static_cast<unsigned long long>(draw.diagnosticShader),triangle,unsigned(draw.indexCount!=0),job->offsets.size());return;
  }
  if(attempt==128)std::fprintf(stderr,"RASTER_REPLAY_SELECTION_FAIL shader=%llu attempts=128 scope=no_eligible_original_triangle\n",static_cast<unsigned long long>(draw.diagnosticShader));
}
}
