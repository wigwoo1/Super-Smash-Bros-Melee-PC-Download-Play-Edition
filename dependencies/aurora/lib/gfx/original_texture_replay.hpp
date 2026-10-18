#pragma once
#include "frame_packet.hpp"
#include "resource_cache.hpp"
#include "original_texture_reference.hpp"
#include "../webgpu/gpu.hpp"
#include <tuple>
#include <algorithm>
#include <string>
#include <cmath>
namespace aurora::gfx::original_texture {
inline std::map<std::tuple<unsigned,unsigned,unsigned>,wgpu::ComputePipeline> pipelines;
inline wgpu::ComputePipeline pipeline(unsigned slot,unsigned width,unsigned height){
  auto key=std::make_tuple(slot,width,height);if(auto it=pipelines.find(key);it!=pipelines.end())return it->second;
  if(!std::getenv("AURORA_DIAGNOSTIC_TEXTURE_REPLAY")||pipelines.size()>=16){std::fprintf(stderr,"ORIGINAL_TEXTURE_GPU_FAIL reason=pipeline-scope\n");return {};}
  std::string code="@group(2) @binding("+std::to_string(slot*2)+") var image: texture_2d<f32>;\n@group(2) @binding("+std::to_string(slot*2+1)+") var image_sampler: sampler;\n@group(3) @binding(0) var<storage,read_write> result: array<vec4f>;\n";
  code+="@compute @workgroup_size(8,8) fn main(@builtin(global_invocation_id) id: vec3u){if(id.x>="+std::to_string(width)+"u||id.y>="+std::to_string(height)+"u){return;}let index=id.y*"+std::to_string(width)+"u+id.x;if(index==0u){result[0]=vec4f(vec2f(textureDimensions(image)),f32(textureNumLevels(image)),1.0);}result[1u+index*2u]=textureLoad(image,vec2i(id.xy),0);result[2u+index*2u]=textureSampleLevel(image,image_sampler,(vec2f(id.xy)+vec2f(0.5))/vec2f("+std::to_string(width)+".0,"+std::to_string(height)+".0),0.0);}";
  const wgpu::BindGroupLayoutDescriptor emptyDesc{};const auto empty=webgpu::g_device.CreateBindGroupLayout(&emptyDesc);
  const wgpu::BindGroupLayoutEntry output{.binding=0,.visibility=wgpu::ShaderStage::Compute,.buffer={.type=wgpu::BufferBindingType::Storage}};
  const wgpu::BindGroupLayoutDescriptor outputDesc{.entryCount=1,.entries=&output};const auto outputs=webgpu::g_device.CreateBindGroupLayout(&outputDesc);
  const std::array layouts{empty,empty,gx::diagnostic_texture_bind_group_layout(),outputs};const wgpu::PipelineLayoutDescriptor layoutDesc{.bindGroupLayoutCount=layouts.size(),.bindGroupLayouts=layouts.data()};const auto layout=webgpu::g_device.CreatePipelineLayout(&layoutDesc);
  wgpu::ShaderSourceWGSL source{};source.code=code.c_str();const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&source,.label="Original image actual bind-group sampling"};const auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
  const wgpu::ComputePipelineDescriptor descriptor{.layout=layout,.compute={.module=module,.entryPoint="main"}};auto compiled=webgpu::g_device.CreateComputePipeline(&descriptor);pipelines.emplace(key,compiled);return compiled;
}
struct Job {std::shared_ptr<const Source> source;std::vector<uint8_t> expected;wgpu::Buffer output,readback;uint64_t sequence=0,group=0,packet=0;unsigned slot=0;};
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame,const detail::RenderPass& pass){
  if(!enabled())return;
  for(const auto& snapshot:pass.originalTextureDraws){
    const auto& m=snapshot.source->metadata;auto job=std::make_shared<Job>();job->source=snapshot.source;job->sequence=snapshot.sequence;job->slot=snapshot.slot;job->group=snapshot.draw.bindGroups.textureBindGroup;job->packet=frame.frameId;
    if(!decode_original(snapshot.source->bytes,snapshot.source->palette,m.format,m.paletteFormat,m.width,m.height,job->expected)){std::fprintf(stderr,"ORIGINAL_TEXTURE_GPU_FAIL reason=reference\n");continue;}
    const auto bind=find_bind_group(snapshot.draw.bindGroups.textureBindGroup);const auto compute=pipeline(snapshot.slot,m.width,m.height);
    if(!bind||!compute){std::fprintf(stderr,"ORIGINAL_TEXTURE_GPU_FAIL reason=actual-binding\n");continue;}
    const uint64_t size=(uint64_t(m.width)*m.height*2+1)*16;
    const wgpu::BufferDescriptor outputDesc{.usage=wgpu::BufferUsage::Storage|wgpu::BufferUsage::CopySrc,.size=size};const wgpu::BufferDescriptor readDesc{.usage=wgpu::BufferUsage::MapRead|wgpu::BufferUsage::CopyDst,.size=size};job->output=webgpu::g_device.CreateBuffer(&outputDesc);job->readback=webgpu::g_device.CreateBuffer(&readDesc);
    const wgpu::BindGroupDescriptor emptyDesc{.layout=compute.GetBindGroupLayout(0)};const auto empty=webgpu::g_device.CreateBindGroup(&emptyDesc);
    const wgpu::BindGroupEntry output{.binding=0,.buffer=job->output,.size=size};const wgpu::BindGroupDescriptor resultDesc{.layout=compute.GetBindGroupLayout(3),.entryCount=1,.entries=&output};const auto result=webgpu::g_device.CreateBindGroup(&resultDesc);
    auto dispatch=cmd.BeginComputePass();dispatch.SetPipeline(compute);dispatch.SetBindGroup(0,empty);dispatch.SetBindGroup(1,empty);dispatch.SetBindGroup(2,bind);dispatch.SetBindGroup(3,result);dispatch.DispatchWorkgroups((m.width+7)/8,(m.height+7)/8);dispatch.End();cmd.CopyBufferToBuffer(job->output,0,job->readback,0,size);
    std::fprintf(stderr,"ORIGINAL_TEXTURE_GPU_QUEUED source=%llu texture=%u image=%u slot=%u sequence=%llu packet=%llu pixels=%u group=%llu\n",(unsigned long long)job->source->id,m.textureOffset,m.imageOffset,job->slot,(unsigned long long)job->sequence,(unsigned long long)job->packet,m.width*m.height,(unsigned long long)job->group);
    frame.afterSubmitCallbacks.emplace_back([job,size]{job->readback.MapAsync(wgpu::MapMode::Read,0,size,wgpu::CallbackMode::AllowSpontaneous,[job,size](wgpu::MapAsyncStatus status,wgpu::StringView){
      const auto& m=job->source->metadata;const auto* values=status==wgpu::MapAsyncStatus::Success?static_cast<const float*>(job->readback.GetConstMappedRange(0,size)):nullptr;
      unsigned mismatches=0,components=0,rejected=0;double maximum=0;std::vector<uint8_t> loaded(job->expected.size()),sampled(job->expected.size());
      if(!values||values[0]!=m.width||values[1]!=m.height||values[2]!=1||values[3]!=1)++mismatches;
      if(values)for(size_t n=0;n<job->expected.size();n++)for(unsigned path=0;path<2;path++){
        const float actual=values[4+(n/4)*8+path*4+n%4];const double expected=job->expected[n]/255.0,error=std::abs(actual-expected);++components;
        if(!std::isfinite(actual)||error>2e-6)++mismatches;if(std::isfinite(error))maximum=std::max(maximum,error);
        if(!std::isfinite(actual)||std::abs(actual-(expected+(expected>.5?-1:1)/255.0))>2e-6)++rejected;
        (path?sampled:loaded)[n]=std::isfinite(actual)?uint8_t(std::clamp(std::lround(actual*255),0l,255l)):0;
      }
      // Hash owned readback-derived bytes before Unmap;no mapped pointer escapes.
      const auto expectedHash=hash(job->expected.data(),job->expected.size()),loadHash=hash(loaded.data(),loaded.size()),sampleHash=hash(sampled.data(),sampled.size());
      if(values)job->readback.Unmap();
      std::fprintf(stderr,"ORIGINAL_TEXTURE_GPU_%s source=%llu texture=%u image=%u slot=%u sequence=%llu packet=%llu pixels=%u components=%u mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g expected_hash=%llu load_hash=%llu sample_hash=%llu group=%llu\n",mismatches?"FAIL":"PASS",(unsigned long long)job->source->id,m.textureOffset,m.imageOffset,job->slot,(unsigned long long)job->sequence,(unsigned long long)job->packet,m.width*m.height,components,mismatches,rejected,maximum,(unsigned long long)expectedHash,(unsigned long long)loadHash,(unsigned long long)sampleHash,(unsigned long long)job->group);
    });});
  }
}
}
