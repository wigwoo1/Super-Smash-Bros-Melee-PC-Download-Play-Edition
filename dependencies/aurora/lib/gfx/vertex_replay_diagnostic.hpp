#pragma once
#include "corneria_position_profiles.hpp"
#include "mutecity_position_profiles.hpp"
#include "indexed_position_diagnostic.hpp"
#include "../gx/gx.hpp"
#include "../webgpu/gpu.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <map>
#include <mutex>
#include <set>
#include <atomic>
#include <cmath>
#include <memory>
#include <algorithm>
#include <cstring>
#include "resources.hpp"
#include "frame_packet.hpp"
#include "resource_cache.hpp"
#include "texture_sample_reference.hpp"
#include "raster_replay_diagnostic.hpp"
#include "occlusion_pair_diagnostic.hpp"
namespace aurora::gfx::vertex_replay_diagnostic {
inline std::mutex pipelineMutex;
inline std::map<uint64_t,wgpu::ComputePipeline> pipelines;
struct Job { wgpu::Buffer output,readback;uint64_t shader;uint32_t vertices,bytes,stride=8;std::atomic<int> status{0};bool hasUvOracle=false,hasNormalOracle=false,hasFragmentOracle=false,hasTextureOracle=false,hasMaterialOracle=false,hasPositionOracle=false;std::array<float,768> uvExpected{};std::array<float,1024> fragmentExpected{};std::array<double,1024> colourExpected{},materialExpected{},positionExpected{};std::array<int,768> materialFirst{};int materialFactor=0; };
inline std::vector<std::shared_ptr<Job>> jobs;
// Compilation and explicitly opted-in original-buffer replay.
inline void compile() {
  static bool started=false;
  const char* directory=std::getenv("AURORA_DIAGNOSTIC_VERTEX_REPLAY_DIRECTORY");
  if(started || !directory || !*directory || !std::getenv("AURORA_DIAGNOSTIC_DRAW_UV")) return;
  started=true;
  const std::array inputs{
    wgpu::BindGroupLayoutEntry{.binding=0,.visibility=wgpu::ShaderStage::Compute,.buffer={.type=wgpu::BufferBindingType::ReadOnlyStorage}},
    wgpu::BindGroupLayoutEntry{.binding=1,.visibility=wgpu::ShaderStage::Compute,.buffer={.type=wgpu::BufferBindingType::ReadOnlyStorage}}};
  const wgpu::BindGroupLayoutDescriptor inputDesc{.entryCount=inputs.size(),.entries=inputs.data()};
  const auto input=webgpu::g_device.CreateBindGroupLayout(&inputDesc);
  const wgpu::BindGroupLayoutEntry uniform{.binding=0,.visibility=wgpu::ShaderStage::Compute,.buffer={.type=wgpu::BufferBindingType::Uniform,.hasDynamicOffset=true}};
  const wgpu::BindGroupLayoutDescriptor uniformDesc{.entryCount=1,.entries=&uniform};
  const auto uniforms=webgpu::g_device.CreateBindGroupLayout(&uniformDesc);
  const wgpu::BindGroupLayoutDescriptor emptyDesc{};
  const auto empty=std::getenv("AURORA_DIAGNOSTIC_TEXTURE_REPLAY")?gx::diagnostic_texture_bind_group_layout():webgpu::g_device.CreateBindGroupLayout(&emptyDesc);
  const wgpu::BindGroupLayoutEntry output{.binding=0,.visibility=wgpu::ShaderStage::Compute,.buffer={.type=wgpu::BufferBindingType::Storage}};
  const wgpu::BindGroupLayoutDescriptor outputDesc{.entryCount=1,.entries=&output};
  const auto outputs=webgpu::g_device.CreateBindGroupLayout(&outputDesc);
  const std::array layouts{input,uniforms,empty,outputs};
  const wgpu::PipelineLayoutDescriptor layoutDesc{.bindGroupLayoutCount=layouts.size(),.bindGroupLayouts=layouts.data(),.immediateSize=sizeof(gx::DrawImmediateData)};
  const auto layout=webgpu::g_device.CreatePipelineLayout(&layoutDesc);
  std::error_code error;unsigned count=0;
  for(const auto& entry:std::filesystem::directory_iterator(directory,error)) {
    if(count>=(std::getenv("AURORA_DIAGNOSTIC_MUTECITY_POSITION_BATCH")?23u:6u) || entry.path().extension()!=L".wgsl") continue;
    const auto name=entry.path().filename().string();
    std::ifstream file(entry.path(),std::ios::binary);
    std::string code((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    if(code.empty() || code.size()>1024*1024) {std::fprintf(stderr,"VERTEX_REPLAY_SOURCE_FAIL file=%s\n",name.c_str());continue;}
    wgpu::ShaderSourceWGSL source{};source.code=code.c_str();
    const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&source,.label="Production vertex replay compilation"};
    const auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
    const wgpu::ComputePipelineDescriptor descriptor{.label="Production vertex replay compilation",.layout=layout,.compute={.module=module,.entryPoint="replay_main"}};
    webgpu::g_device.CreateComputePipelineAsync(&descriptor,wgpu::CallbackMode::AllowSpontaneous,
      [name](wgpu::CreatePipelineAsyncStatus status,wgpu::ComputePipeline pipeline,wgpu::StringView) {
        if(status==wgpu::CreatePipelineAsyncStatus::Success) {
          const auto shader=std::strtoull(name.c_str()+3,nullptr,16);
          const std::lock_guard lock(pipelineMutex);pipelines.emplace(shader,std::move(pipeline));
        }
        std::fprintf(stderr,"VERTEX_REPLAY_COMPILE_%s file=%s status=%u\n",status==wgpu::CreatePipelineAsyncStatus::Success?"PASS":"FAIL",name.c_str(),unsigned(status));
      });
    ++count;
  }
  std::fprintf(stderr,"VERTEX_REPLAY_COMPILE_QUEUED shaders=%u directory_error=%u scope=compilation_only\n",count,unsigned(bool(error)));
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame,const detail::RenderPass& renderPass) {
  if(!std::getenv("AURORA_DIAGNOSTIC_VERTEX_REPLAY_DISPATCH")) return;
  indexed_position_diagnostic::compile();indexed_position_diagnostic::drain();
  raster_replay_diagnostic::drain();
  for(auto it=jobs.begin();it!=jobs.end();) {
    const auto job=*it;const int status=job->status.load(std::memory_order_acquire);
    if(!status) {++it;continue;}
    unsigned nonfinite=0;uint64_t hash=14695981039346656037ull;
    const auto* values=status==1?static_cast<const float*>(job->readback.GetConstMappedRange(0,job->bytes)):nullptr;
    if(values) {
      for(unsigned n=0;n<job->bytes/4;++n) if(!std::isfinite(values[n])) ++nonfinite;
      const auto* bytes=reinterpret_cast<const uint8_t*>(values);
      for(unsigned n=0;n<job->bytes;++n) hash=(hash^bytes[n])*1099511628211ull;
      if(job->hasUvOracle || job->hasNormalOracle) {
        const unsigned components=job->hasNormalOracle?3:2;
        unsigned mismatches=0,wrongReferenceRejections=0;double maximumError=0;
        for(unsigned vertex=0;vertex<job->vertices;++vertex) for(unsigned component=0;component<components;++component) {
          const double actual=values[vertex*job->stride+4+component],expected=job->uvExpected[vertex*components+component];
          const double tolerance=2e-5*(1+std::abs(expected));
          const double error=std::abs(actual-expected);maximumError=std::max(maximumError,error);
          if(!std::isfinite(actual) || !std::isfinite(expected) || error>tolerance) ++mismatches;
          if(std::abs(actual-(expected+0.125))>tolerance) ++wrongReferenceRejections;
        }
        std::fprintf(stderr,"VERTEX_REPLAY_%s_ORACLE_%s shader=%llu vertices=%u components=%u mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g scope=one_%s_path\n",job->hasNormalOracle?"NORMAL":"UV",mismatches?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),job->vertices,job->vertices*components,mismatches,wrongReferenceRejections,maximumError,job->hasNormalOracle?"normal":"uv");
      }
      if(job->hasFragmentOracle) {
        unsigned mismatches=0,rejections=0;double maximumError=0;
        for(unsigned vertex=0;vertex<job->vertices;++vertex) for(unsigned component=0;component<4;++component) {
          const double actual=values[vertex*job->stride+8+component],expected=job->fragmentExpected[vertex*4+component];
          const double error=std::abs(actual-expected),tolerance=2e-5*(1+std::abs(expected));maximumError=std::max(maximumError,error);
          if(!std::isfinite(actual) || !std::isfinite(expected) || error>tolerance) ++mismatches;
          if(std::abs(actual-(expected+0.125))>tolerance) ++rejections;
        }
        std::fprintf(stderr,"FRAGMENT_COORD_ORACLE_%s shader=%llu vertices=%u components=%u mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g scope=vertex_outputs_only\n",mismatches?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),job->vertices,job->vertices*4,mismatches,rejections,maximumError);
      }
      if(job->stride>=16) {
        unsigned invalid=0,nonzero=0;
        for(unsigned vertex=0;vertex<job->vertices;++vertex) for(unsigned component=0;component<4;++component) {
          const float value=values[vertex*job->stride+12+component];
          if(!std::isfinite(value) || value<0 || value>1) ++invalid;
          if(value!=0) ++nonzero;
        }
        std::fprintf(stderr,"TEXTURE_REPLAY_READBACK_%s shader=%llu vertices=%u colour_components=%u invalid=%u nonzero=%u scope=lod0_access_only\n",invalid?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),job->vertices,job->vertices*4,invalid,nonzero);
      }
      if(job->hasTextureOracle) {
        unsigned mismatches=0,rejections=0;double maxError=0;
        // Hardware bilinear weights may be quantized; tolerance is bounded to
        // two RGBA8 code points. Large deliberately incorrect references must fail.
        constexpr double tolerance=2.0/255.0;
        for(unsigned vertex=0;vertex<job->vertices;++vertex) for(unsigned c=0;c<4;++c) {
          const double actual=values[vertex*job->stride+12+c],expected=job->colourExpected[vertex*4+c];
          const double error=std::abs(actual-expected);maxError=std::max(maxError,error);
          if(!std::isfinite(actual) || !std::isfinite(expected) || error>tolerance) ++mismatches;
          if(std::abs(actual-(expected+0.125))>tolerance) ++rejections;
        }
        std::fprintf(stderr,"TEXTURE_SAMPLE_ORACLE_%s shader=%llu vertices=%u components=%u mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g tolerance=%.9g scope=rgba8_lod0_vertex_samples\n",mismatches?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),job->vertices,job->vertices*4,mismatches,rejections,maxError,tolerance);
      }
      if(job->hasMaterialOracle) {
        unsigned mismatches=0,rejections=0;double maxError=0;
        constexpr double tolerance=2.0/255.0;
        for(unsigned vertex=0;vertex<job->vertices;++vertex) for(unsigned c=0;c<4;++c) {
          const double actual=values[vertex*job->stride+16+c],expected=job->materialExpected[vertex*4+c];
          const double error=std::abs(actual-expected);maxError=std::max(maxError,error);
          if(!std::isfinite(actual) || !std::isfinite(expected) || error>tolerance) ++mismatches;
          if(std::abs(actual-(expected+0.125))>tolerance) ++rejections;
        }
        std::fprintf(stderr,"MATERIAL_REPLAY_ORACLE_%s shader=%llu vertices=%u components=%u mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g scope=lod0_tev_at_vertices\n",mismatches?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),job->vertices,job->vertices*4,mismatches,rejections,maxError);
        if(std::getenv("AURORA_DIAGNOSTIC_MATERIAL_ARITHMETIC_ORACLE")) {
          unsigned arithmeticMismatch=0,negativeRejections=0;double arithmeticError=0;
          constexpr double arithmeticTolerance=2e-6;
          for(unsigned vertex=0;vertex<job->vertices;++vertex) for(unsigned c=0;c<4;++c) {
            double expected=job->materialExpected[vertex*4+3];
            if(c<3) {
              const float sampled=values[vertex*job->stride+12+c];
              if(!std::isfinite(sampled) || sampled<0 || sampled>1) {++arithmeticMismatch;continue;}
              // Same GPU sampled input, independent integer interpolation.
              const int tex=std::clamp(int(std::floor(double(sampled)*255+0.5)),0,255);
              const int first=job->materialFirst[vertex*3+c];
              const int factor=job->hasUvOracle?tex:job->materialFactor;
              const int expanded=factor+factor/128;
              const int a=job->hasUvOracle?0:first,b=job->hasUvOracle?first:tex;
              expected=double((a*(256-expanded)+b*expanded+128)/256)/255.0;
            }
            const double actual=values[vertex*job->stride+16+c],error=std::abs(actual-expected);
            arithmeticError=std::max(arithmeticError,error);
            if(!std::isfinite(actual) || error>arithmeticTolerance) ++arithmeticMismatch;
            if(std::abs(actual-(expected+0.125))>arithmeticTolerance) ++negativeRejections;
          }
          std::fprintf(stderr,"MATERIAL_ARITHMETIC_ORACLE_%s shader=%llu components=%u mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g scope=same_gpu_sample_input\n",arithmeticMismatch?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),job->vertices*4,arithmeticMismatch,negativeRejections,arithmeticError);
        }
      }
      if(job->hasPositionOracle) {
        unsigned mismatches=0,rejections=0;double maxError=0;
        for(unsigned vertex=0;vertex<job->vertices;++vertex) for(unsigned c=0;c<4;++c) {
          const double actual=values[vertex*job->stride+c],expected=job->positionExpected[vertex*4+c];
          const double error=std::abs(actual-expected),tolerance=2e-5*(1+std::abs(expected));
          maxError=std::max(maxError,error);
          if(!std::isfinite(actual) || !std::isfinite(expected) || error>tolerance) ++mismatches;
          if(std::abs(actual-(expected+0.25*(1+std::abs(expected))))>tolerance) ++rejections;
        }
        std::fprintf(stderr,"POSITION_PROJECTION_ORACLE_%s shader=%llu vertices=%u components=%u mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g scope=clip_space_vertices\n",mismatches?"FAIL":"PASS",static_cast<unsigned long long>(job->shader),job->vertices,job->vertices*4,mismatches,rejections,maxError);
      }
      job->readback.Unmap();
    }
    std::fprintf(stderr,"VERTEX_REPLAY_READBACK_%s shader=%llu vertices=%u bytes=%u nonfinite=%u hash=%llu scope=dispatch_only\n",values?"PASS":"FAIL",static_cast<unsigned long long>(job->shader),job->vertices,job->bytes,nonfinite,static_cast<unsigned long long>(hash));
    it=jobs.erase(it);
  }
  static std::set<uint64_t> dispatched;
  for(const auto& snapshot:renderPass.diagnosticReplayDraws) {
    const auto& draw=snapshot.draw;
    const bool computeDone=dispatched.contains(draw.diagnosticShader);
    const bool indexedNeeded=indexed_position_diagnostic::needs(draw.diagnosticShader);
    if((dispatched.size()>=(std::getenv("AURORA_DIAGNOSTIC_MUTECITY_POSITION_BATCH")?23u:6u) && !indexedNeeded) || !draw.vtxCount || draw.instanceCount!=1 || (computeDone && !indexedNeeded && !raster_replay_diagnostic::needs(draw.diagnosticShader) && (!std::getenv("AURORA_DIAGNOSTIC_OCCLUSION_SURVEY") || occlusion_pair_diagnostic::finished(draw.diagnosticShader) || occlusion_pair_diagnostic::attempts[draw.diagnosticShader]>=128))) continue;
    wgpu::ComputePipeline pipeline;
    {const std::lock_guard lock(pipelineMutex);auto it=pipelines.find(draw.diagnosticShader);if(it==pipelines.end())continue;pipeline=it->second;}
    if(std::getenv("AURORA_DIAGNOSTIC_MIP_METADATA")) {
      static std::set<uint64_t> recorded;
      const bool normal=draw.diagnosticShader==0xdff4b88bull,uv=draw.diagnosticShader==0x3f73e0adull;
      const size_t offset=(normal?1568:1520)+960+128,base=draw.uniformRange.offset;
      if((normal || uv) && draw.uniformRange.size>=offset+16 && base+offset+16<=frame.uniforms.size() && recorded.insert(draw.diagnosticShader).second) {
        float dimensionsBias[4];std::memcpy(dimensionsBias,frame.uniforms.data()+base+offset,16);
        const auto& sampler=snapshot.sampler;
        std::fprintf(stderr,"ORIGINAL_MIP_METADATA shader=%llu mips=%u width=%u height=%u mag=%u min=%u mip_filter=%u lod_min=%.9g lod_max=%.9g bias=%.9g aniso=%u scope=captured_original_binding\n",static_cast<unsigned long long>(draw.diagnosticShader),snapshot.mipCount,snapshot.width,snapshot.height,unsigned(sampler.magFilter),unsigned(sampler.minFilter),unsigned(sampler.mipmapFilter),double(sampler.lodMinClamp),double(sampler.lodMaxClamp),double(dimensionsBias[2]),unsigned(sampler.maxAnisotropy));
      }
    }
    if(std::getenv("AURORA_DIAGNOSTIC_CORNERIA_POSITION_BATCH") && draw.vtxCount<8) continue;
    auto job=std::make_shared<Job>();job->shader=draw.diagnosticShader;job->vertices=std::min(draw.vtxCount,(std::getenv("AURORA_DIAGNOSTIC_CORNERIA_POSITION_BATCH") || std::getenv("AURORA_DIAGNOSTIC_MUTECITY_POSITION_BATCH"))?64u:(computeDone && raster_replay_diagnostic::stateReplay())?(std::getenv("AURORA_DIAGNOSTIC_WHOLE_DRAW")?256u:64u):8u);job->stride=std::getenv("AURORA_DIAGNOSTIC_MATERIAL_REPLAY")?20:std::getenv("AURORA_DIAGNOSTIC_TEXTURE_REPLAY")?16:std::getenv("AURORA_DIAGNOSTIC_FRAGMENT_COORD_ORACLE")?12:8;job->bytes=job->vertices*job->stride*4;
    if(std::getenv("AURORA_DIAGNOSTIC_POSITION_PROJECTION_ORACLE")) {
      const bool uv=job->shader==0x3f73e0adull,normal=job->shader==0xdff4b88bull;
      // Corneria profile independently inspected in generated production WGSL:
      // BE16 position index at byte0, packed stride8, S16 XYZ stride6/fraction6,
      // static current position matrix. Other stage layouts remain unverified.
      const bool corneria=job->shader==648529937ull;
      const auto* profile=std::getenv("AURORA_DIAGNOSTIC_CORNERIA_POSITION_BATCH")?corneria_position_profiles::find(job->shader):nullptr;
      const auto* muteProfile=std::getenv("AURORA_DIAGNOSTIC_MUTECITY_POSITION_BATCH")?mutecity_position_profiles::find(job->shader):nullptr;
      const unsigned indexWidth=muteProfile?muteProfile->indexWidth:2;
      const unsigned arrayStride=muteProfile?muteProfile->arrayStride:6;
      const unsigned packedStride=muteProfile?muteProfile->stride:profile?profile->stride:(uv || corneria)?8:4;
      const double divisor=muteProfile?double(1u<<muteProfile->fraction):profile?double(1u<<profile->fraction):corneria?64.0:256.0;
      const size_t base=draw.uniformRange.offset,matrixOffset=80+size_t(draw.immediateData.currentPnMtx)*48;
      bool valid=(uv || normal || corneria || profile || muteProfile) && draw.immediateData.currentPnMtx<20 &&
          draw.uniformRange.size>=matrixOffset+48 && base+matrixOffset+48<=frame.uniforms.size();
      float model[12]{},projection[16]{};
      if(valid) {std::memcpy(model,frame.uniforms.data()+base+matrixOffset,48);std::memcpy(projection,frame.uniforms.data()+base+16,64);}
      for(float value:model) valid=valid && std::isfinite(value);
      for(float value:projection) valid=valid && std::isfinite(value);
      for(unsigned vertex=0;valid && vertex<job->vertices;++vertex) {
        const size_t indexAt=size_t(draw.immediateData.vtxStart)+vertex*packedStride;
        if(indexAt+indexWidth>frame.verts.size()) {valid=false;break;}
        const auto* indexBytes=frame.verts.data()+indexAt;
        const unsigned index=indexWidth==1?unsigned(indexBytes[0]):(unsigned(indexBytes[0])<<8)|unsigned(indexBytes[1]);
        const size_t positionAt=size_t(draw.immediateData.arrayStart[0])+size_t(index)*arrayStride;
        if(positionAt+arrayStride>frame.storage.size()) {valid=false;break;}
        const auto* bytes=frame.storage.data()+positionAt;double position[4]{0,0,0,1};
        for(unsigned c=0;c<3;++c) {
          if(muteProfile && muteProfile->floats) {
            const uint32_t encoded=(uint32_t(bytes[c*4])<<24)|(uint32_t(bytes[c*4+1])<<16)|
                                   (uint32_t(bytes[c*4+2])<<8)|uint32_t(bytes[c*4+3]);
            float decoded;std::memcpy(&decoded,&encoded,4);position[c]=decoded;
          } else {
            const unsigned encoded=(unsigned(bytes[c*2])<<8)|unsigned(bytes[c*2+1]);
            position[c]=(encoded>=32768?int(encoded)-65536:int(encoded))/divisor;
          }
        }
        double transformed[4]{0,0,0,1};
        for(unsigned c=0;c<3;++c) for(unsigned k=0;k<4;++k) transformed[c]+=position[k]*model[c*4+k];
        for(unsigned c=0;c<4;++c) {
          double result=0;for(unsigned k=0;k<4;++k) result+=transformed[k]*projection[c*4+k];
          job->positionExpected[vertex*4+c]=result;valid=valid && std::isfinite(result);
        }
      }
      job->hasPositionOracle=valid;
      if(!valid) std::fprintf(stderr,"POSITION_PROJECTION_ORACLE_INPUT_FAIL shader=%llu\n",static_cast<unsigned long long>(job->shader));
    }
    // Narrow, pinned original Battlefield profile: BE16 UV indices at +6 in
    // an eight-byte vertex, U16 fraction14 UV array, identity pre-transform,
    // postmtx[0]. Layout is independently derived from the captured WGSL types:
    // viewport16 + projection64 + position/texture20*48 + normal10*48.
    if(job->shader==0x3f73e0adull && std::getenv("AURORA_DIAGNOSTIC_VERTEX_REPLAY_UV_ORACLE")) {
      constexpr size_t postOffset=16+64+20*48+10*48;
      const size_t matrixAt=size_t(draw.uniformRange.offset)+postOffset;
      bool valid=draw.uniformRange.size>=postOffset+48 && matrixAt+48<=frame.uniforms.size();
      float matrix[12]{};if(valid) std::memcpy(matrix,frame.uniforms.data()+matrixAt,sizeof(matrix));
      for(unsigned n=0;n<12;++n) valid=valid && std::isfinite(matrix[n]);
      for(unsigned vertex=0;valid && vertex<job->vertices;++vertex) {
        const size_t indexAt=size_t(draw.immediateData.vtxStart)+vertex*8+6;
        if(indexAt+2>frame.verts.size()) {valid=false;break;}
        const auto* indexBytes=frame.verts.data()+indexAt;
        const unsigned index=(unsigned(indexBytes[0])<<8)|unsigned(indexBytes[1]);
        const size_t uvAt=size_t(draw.immediateData.arrayStart[4])+size_t(index)*4;
        if(uvAt+4>frame.storage.size()) {valid=false;break;}
        const auto* uvBytes=frame.storage.data()+uvAt;
        const double s=((unsigned(uvBytes[0])<<8)|unsigned(uvBytes[1]))/16384.0;
        const double t=((unsigned(uvBytes[2])<<8)|unsigned(uvBytes[3]))/16384.0;
        double result[3];for(unsigned row=0;row<3;++row)
          result[row]=s*matrix[row*4]+t*matrix[row*4+1]+matrix[row*4+2]+matrix[row*4+3];
        if(result[2]==0.0) {result[0]=std::clamp(result[0]*0.5,-1.0,1.0);result[1]=std::clamp(result[1]*0.5,-1.0,1.0);}
        for(unsigned component=0;component<2;++component) job->uvExpected[vertex*2+component]=float(result[component]);
      }
      job->hasUvOracle=valid;
      if(!valid) std::fprintf(stderr,"VERTEX_REPLAY_UV_ORACLE_INPUT_FAIL shader=%llu\n",static_cast<unsigned long long>(job->shader));
    }
    if(job->shader==0xdff4b88bull && std::getenv("AURORA_DIAGNOSTIC_VERTEX_REPLAY_NORMAL_ORACLE")) {
      // Pinned BE16 normal-index profile: signed frac14, pre-matrix slot10,
      // normalize, post slot0. Three vec4 color fields precede post matrices.
      constexpr size_t preOffset=16+64+10*48,postOffset=16+64+20*48+10*48+3*16;
      const size_t base=draw.uniformRange.offset;
      bool valid=draw.uniformRange.size>=postOffset+48 && base+postOffset+48<=frame.uniforms.size();
      float pre[12]{},post[12]{};
      if(valid) {std::memcpy(pre,frame.uniforms.data()+base+preOffset,48);std::memcpy(post,frame.uniforms.data()+base+postOffset,48);}
      for(unsigned n=0;n<12;++n) valid=valid && std::isfinite(pre[n]) && std::isfinite(post[n]);
      for(unsigned vertex=0;valid && vertex<job->vertices;++vertex) {
        const size_t indexAt=size_t(draw.immediateData.vtxStart)+vertex*4+2;
        if(indexAt+2>frame.verts.size()) {valid=false;break;}
        const auto* indices=frame.verts.data()+indexAt;
        const unsigned index=(unsigned(indices[0])<<8)|unsigned(indices[1]);
        const size_t normalAt=size_t(draw.immediateData.arrayStart[1])+size_t(index)*6;
        if(normalAt+6>frame.storage.size()) {valid=false;break;}
        const auto* bytes=frame.storage.data()+normalAt;double normal[3];
        for(unsigned component=0;component<3;++component) {
          const unsigned encoded=(unsigned(bytes[component*2])<<8)|unsigned(bytes[component*2+1]);
          normal[component]=(encoded>=32768?int(encoded)-65536:int(encoded))/16384.0;
        }
        double transformed[3];for(unsigned row=0;row<3;++row)
          transformed[row]=normal[0]*pre[row*4]+normal[1]*pre[row*4+1]+normal[2]*pre[row*4+2]+pre[row*4+3];
        const double length=std::sqrt(transformed[0]*transformed[0]+transformed[1]*transformed[1]+transformed[2]*transformed[2]);
        if(!std::isfinite(length) || length==0) {valid=false;break;}
        for(double& value:transformed) value/=length;
        double result[3];for(unsigned row=0;row<3;++row)
          result[row]=transformed[0]*post[row*4]+transformed[1]*post[row*4+1]+transformed[2]*post[row*4+2]+post[row*4+3];
        if(result[2]==0) {result[0]=std::clamp(result[0]*0.5,-1.0,1.0);result[1]=std::clamp(result[1]*0.5,-1.0,1.0);}
        for(unsigned component=0;component<3;++component) {job->uvExpected[vertex*3+component]=float(result[component]);valid=valid && std::isfinite(job->uvExpected[vertex*3+component]);}
      }
      job->hasNormalOracle=valid;
      if(!valid) std::fprintf(stderr,"VERTEX_REPLAY_NORMAL_ORACLE_INPUT_FAIL shader=%llu\n",static_cast<unsigned long long>(job->shader));
    }
    if(job->stride>=12 && (job->hasUvOracle || job->hasNormalOracle)) {
      const size_t scaleOffset=(job->hasNormalOracle?1568:1520)+20*48;
      const size_t sizeOffset=scaleOffset+8*16,base=draw.uniformRange.offset;
      bool valid=draw.uniformRange.size>=sizeOffset+16 && base+sizeOffset+16<=frame.uniforms.size();
      float scales[4]{},dimensions[4]{};
      if(valid) {std::memcpy(scales,frame.uniforms.data()+base+scaleOffset,16);std::memcpy(dimensions,frame.uniforms.data()+base+sizeOffset,16);}
      valid=valid && std::isfinite(scales[0]) && std::isfinite(scales[1]) && std::isfinite(dimensions[0]) && std::isfinite(dimensions[1]) && dimensions[0]>0 && dimensions[1]>0;
      const unsigned count=job->hasNormalOracle?3:2;
      for(unsigned vertex=0;valid && vertex<job->vertices;++vertex) {
        const double q=count==3 && job->uvExpected[vertex*count+2]!=0?job->uvExpected[vertex*count+2]:1.0;
        for(unsigned component=0;component<2;++component) {
          const double projected=job->uvExpected[vertex*count+component]/q;
          job->fragmentExpected[vertex*4+component]=float(projected);
          job->fragmentExpected[vertex*4+component+2]=float(projected*scales[component]/dimensions[component]);
          valid=valid && std::isfinite(job->fragmentExpected[vertex*4+component]) && std::isfinite(job->fragmentExpected[vertex*4+component+2]);
        }
      }
      job->hasFragmentOracle=valid;
      if(!valid) std::fprintf(stderr,"FRAGMENT_COORD_ORACLE_INPUT_FAIL shader=%llu\n",static_cast<unsigned long long>(job->shader));
    }
    if(std::getenv("AURORA_DIAGNOSTIC_TEXTURE_SAMPLE_ORACLE")) {
      const auto& sampler=snapshot.sampler;
      auto wrap=[](wgpu::AddressMode mode,texture_sample_reference::Wrap& out) {
        switch(mode) {
        case wgpu::AddressMode::ClampToEdge:out=texture_sample_reference::Wrap::Clamp;return true;
        case wgpu::AddressMode::Repeat:out=texture_sample_reference::Wrap::Repeat;return true;
        case wgpu::AddressMode::MirrorRepeat:out=texture_sample_reference::Wrap::Mirror;return true;
        default:return false;
        }
      };
      texture_sample_reference::Wrap s{},t{};
      bool valid=job->stride>=16 && job->hasFragmentOracle && snapshot.pixels &&
          sampler.lodMinClamp==0 && sampler.lodMaxClamp>=0 && sampler.maxAnisotropy==1 &&
          sampler.magFilter==sampler.minFilter &&
          (sampler.magFilter==wgpu::FilterMode::Nearest || sampler.magFilter==wgpu::FilterMode::Linear) &&
          wrap(sampler.addressModeU,s) && wrap(sampler.addressModeV,t);
      const auto filter=sampler.magFilter==wgpu::FilterMode::Linear?texture_sample_reference::Filter::Linear:texture_sample_reference::Filter::Nearest;
      for(unsigned vertex=0;valid && vertex<job->vertices;++vertex) {
        std::array<double,4> colour{};
        valid=texture_sample_reference::sample(snapshot.pixels->data(),snapshot.pixels->size(),snapshot.width,snapshot.height,
            job->fragmentExpected[vertex*4+2],job->fragmentExpected[vertex*4+3],s,t,filter,colour);
        for(unsigned c=0;c<4;++c) job->colourExpected[vertex*4+c]=colour[c];
      }
      job->hasTextureOracle=valid;
      if(!computeDone || !valid) std::fprintf(stderr,"TEXTURE_SAMPLE_ORACLE_INPUT_%s shader=%llu width=%u height=%u pixels=%u mag=%u min=%u lod_min=%.9g aniso=%u\n",valid?"PASS":"FAIL",static_cast<unsigned long long>(job->shader),snapshot.width,snapshot.height,unsigned(bool(snapshot.pixels)),unsigned(sampler.magFilter),unsigned(sampler.minFilter),double(sampler.lodMinClamp),unsigned(sampler.maxAnisotropy));
    }
    if(job->stride==20) {
      // Independently reduced formulas for these two pinned original materials:
      // UV: vertex RGB multiplied by texture RGB; vertex alpha passed through.
      // Normal: konst0 mixed with texture using konst1 alpha; register0 alpha.
      auto byte=[](double value) {return std::clamp(int(std::floor(value*255.0+0.5)),0,255);};
      bool valid=job->hasTextureOracle;
      float constants[12]{};
      if(job->hasNormalOracle) {
        constexpr size_t offset=16+64+20*48+10*48;
        const size_t at=size_t(draw.uniformRange.offset)+offset;
        valid=valid && draw.uniformRange.size>=offset+48 && at+48<=frame.uniforms.size();
        if(valid) std::memcpy(constants,frame.uniforms.data()+at,48);
        for(float value:constants) valid=valid && std::isfinite(value) && value>=0 && value<=1;
      }
      for(unsigned vertex=0;valid && vertex<job->vertices;++vertex) {
        const size_t at=size_t(draw.immediateData.vtxStart)+vertex*8+2;
        if(job->hasUvOracle && at+4>frame.verts.size()) {valid=false;break;}
        for(unsigned c=0;c<3;++c) {
          const int tex=byte(job->colourExpected[vertex*4+c]);
          const int factor=job->hasUvOracle?tex:byte(constants[11]);
          const int a=job->hasUvOracle?0:byte(constants[4+c]);
          const int b=job->hasUvOracle?frame.verts.data()[at+c]:tex;
          job->materialFirst[vertex*3+c]=job->hasUvOracle?b:a;
          job->materialFactor=byte(constants[11]);
          const int expanded=factor+factor/128;
          const int blended=(a*(256-expanded)+b*expanded+128)/256;
          job->materialExpected[vertex*4+c]=double(std::clamp(blended,0,255))/255.0;
        }
        job->materialExpected[vertex*4+3]=job->hasUvOracle?double(frame.verts.data()[at+3])/255.0:double(byte(constants[3]))/255.0;
      }
      job->hasMaterialOracle=valid;
      if(!valid) std::fprintf(stderr,"MATERIAL_REPLAY_ORACLE_INPUT_FAIL shader=%llu\n",static_cast<unsigned long long>(job->shader));
    }
    if(job->hasPositionOracle && job->hasMaterialOracle && (!std::getenv("AURORA_DIAGNOSTIC_OCCLUSION_GPU") || raster_replay_diagnostic::ready(draw.diagnosticShader))) occlusion_pair_diagnostic::survey(frame,snapshot,job->vertices,job->positionExpected,job->uvExpected,job->hasNormalOracle,job->materialFirst,job->materialFactor,[&](const auto& pair){return raster_replay_diagnostic::encodePair(cmd,frame,snapshot,pair);});
    if(job->hasPositionOracle && job->hasMaterialOracle) raster_replay_diagnostic::encode(cmd,frame,snapshot,job->vertices,job->positionExpected,job->uvExpected,job->hasNormalOracle,job->materialFirst,job->materialFactor,job->materialExpected);
    if(job->hasPositionOracle && std::getenv("AURORA_DIAGNOSTIC_INDEXED_POSITION_SHADER"))
      indexed_position_diagnostic::encode(cmd,frame,draw,job->vertices,job->positionExpected,snapshot.diagnosticMergedPrimitives);
    if(computeDone)continue;
    const wgpu::BufferDescriptor outputDesc{.usage=wgpu::BufferUsage::Storage|wgpu::BufferUsage::CopySrc,.size=job->bytes};
    job->output=webgpu::g_device.CreateBuffer(&outputDesc);
    const wgpu::BufferDescriptor readDesc{.usage=wgpu::BufferUsage::MapRead|wgpu::BufferUsage::CopyDst,.size=job->bytes};
    job->readback=webgpu::g_device.CreateBuffer(&readDesc);
    const auto& resources=detail::resources();
    const std::array inputs{wgpu::BindGroupEntry{.binding=0,.buffer=resources.vertexBuffer,.size=VertexBufferSize},wgpu::BindGroupEntry{.binding=1,.buffer=resources.storageBuffer,.size=StorageBufferSize}};
    const wgpu::BindGroupDescriptor inputDesc{.layout=pipeline.GetBindGroupLayout(0),.entryCount=inputs.size(),.entries=inputs.data()};
    const auto input=webgpu::g_device.CreateBindGroup(&inputDesc);
    const wgpu::BindGroupEntry uniform{.binding=0,.buffer=resources.uniformBuffer,.size=draw.uniformRange.size};
    const wgpu::BindGroupDescriptor uniformDesc{.layout=pipeline.GetBindGroupLayout(1),.entryCount=1,.entries=&uniform};
    const auto uniforms=webgpu::g_device.CreateBindGroup(&uniformDesc);
    const wgpu::BindGroupDescriptor emptyDesc{.layout=pipeline.GetBindGroupLayout(2)};
    const auto empty=job->stride>=16?find_bind_group(draw.bindGroups.textureBindGroup):webgpu::g_device.CreateBindGroup(&emptyDesc);
    if(job->stride>=16 && !empty) {std::fprintf(stderr,"TEXTURE_REPLAY_BINDING_FAIL shader=%llu\n",static_cast<unsigned long long>(job->shader));continue;}
    const wgpu::BindGroupEntry output{.binding=0,.buffer=job->output,.size=job->bytes};
    const wgpu::BindGroupDescriptor bindDesc{.layout=pipeline.GetBindGroupLayout(3),.entryCount=1,.entries=&output};
    const auto bind=webgpu::g_device.CreateBindGroup(&bindDesc);
    const auto pass=cmd.BeginComputePass();pass.SetPipeline(pipeline);pass.SetBindGroup(0,input);pass.SetBindGroup(1,uniforms,1,&draw.uniformRange.offset);pass.SetBindGroup(2,empty);pass.SetBindGroup(3,bind);pass.SetImmediates(0,&draw.immediateData,sizeof(draw.immediateData));pass.DispatchWorkgroups(job->vertices);pass.End();
    cmd.CopyBufferToBuffer(job->output,0,job->readback,0,job->bytes);
    frame.afterSubmitCallbacks.emplace_back([job]{job->readback.MapAsync(wgpu::MapMode::Read,0,job->bytes,wgpu::CallbackMode::AllowSpontaneous,[job](wgpu::MapAsyncStatus status,wgpu::StringView){job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);});});
    jobs.push_back(job);dispatched.insert(draw.diagnosticShader);
    if(job->stride>=16) std::fprintf(stderr,"TEXTURE_REPLAY_ORIGINAL_BINDING shader=%llu resolved=1\n",static_cast<unsigned long long>(job->shader));
    std::fprintf(stderr,"VERTEX_REPLAY_DISPATCH_QUEUED shader=%llu vertices=%u bytes=%u uniform_offset=%u uniform_bytes=%u vertex_offset=%u\n",static_cast<unsigned long long>(draw.diagnosticShader),job->vertices,job->bytes,draw.uniformRange.offset,draw.uniformRange.size,draw.immediateData.vtxStart);
  }
}
}
