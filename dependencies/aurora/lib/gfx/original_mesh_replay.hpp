#pragma once
#include "frame_packet.hpp"
#include "resources.hpp"
#include "../webgpu/gpu.hpp"
#include "original_mesh_direction_shader.hpp"
#include <map>
#include <string>
#include <cstdio>
#include <cmath>
namespace aurora::gfx::original_mesh_replay {
struct ReplayPipeline {wgpu::ComputePipeline compute;uint32_t directionMask=0,colorMask=0;};
inline std::map<std::pair<uint64_t,uint32_t>,ReplayPipeline> pipelines;
inline ReplayPipeline pipeline(const gx::ShaderConfig& config,bool directions,uint32_t colors,uint32_t uv,uint32_t texgen){
  const auto hash=xxh3_hash(config);const auto key=std::make_pair(hash,uint32_t(directions)|(colors<<1)|(uv<<3)|(texgen<<11));
  auto found=pipelines.find(key);if(found!=pipelines.end())return found->second;
  if(pipelines.size()>=32){std::fprintf(stderr,"MESH_GPU_PIPELINE_FAIL reason=cap\n");return {};}
  auto source=gx::build_shader_source(config);
  const std::string vertex="@vertex\nfn vs_main(";
  auto at=source.find(vertex);if(at==std::string::npos||source.find("@builtin(instance_index)")!=std::string::npos){
    std::fprintf(stderr,"MESH_GPU_PIPELINE_FAIL reason=entry\n");return {};}
  source.erase(at,8);
  const std::string index="@builtin(vertex_index) ";at=source.find(index);
  if(at==std::string::npos){std::fprintf(stderr,"MESH_GPU_PIPELINE_FAIL reason=index\n");return {};}
  source.erase(at,index.size());
  uint32_t directionMask=0;
  if(uv&&!directions){std::fprintf(stderr,"MESH_GPU_PIPELINE_FAIL reason=uv-needs-direction-adapter\n");return {};}
  if(directions){auto observed=observe_directions(std::move(source),colors,uv,texgen);
    if(!observed.mask){std::fprintf(stderr,"MESH_GPU_PIPELINE_FAIL reason=direction-observer\n");return {};}
    source=std::move(observed.source);directionMask=observed.mask;
  }
  source+="\n@group(3) @binding(0) var<storage,read_write> mesh_result: array<vec4f>;\n"
    "@compute @workgroup_size(1) fn mesh_main(@builtin(global_invocation_id) id: vec3u){\n";
  if(uv){
    source+="let result=vs_main(id.x);let base=id.x*"+std::to_string(texgen?23:15)+"u;mesh_result[base]=result.pos;mesh_result[base+1u]=result.normalRaw;mesh_result[base+2u]=result.normalUnit;mesh_result[base+3u]=result.binormal;mesh_result[base+4u]=result.tangent;mesh_result[base+5u]=result.color0;mesh_result[base+6u]=result.color1;\n";
    for(unsigned i=0;i<8;i++)source+="mesh_result[base+"+std::to_string(7+i)+"u]=result.uv"+std::to_string(i)+";\n";
    if(texgen)for(unsigned i=0;i<8;i++)source+="mesh_result[base+"+std::to_string(15+i)+"u]=result.texgen"+std::to_string(i)+";\n";
    source+="}\n";
  }else source+=directions?(colors?"let result=vs_main(id.x);let base=id.x*7u;mesh_result[base]=result.pos;mesh_result[base+1u]=result.normalRaw;mesh_result[base+2u]=result.normalUnit;mesh_result[base+3u]=result.binormal;mesh_result[base+4u]=result.tangent;mesh_result[base+5u]=result.color0;mesh_result[base+6u]=result.color1;\n}\n":
    "let result=vs_main(id.x);let base=id.x*5u;mesh_result[base]=result.pos;mesh_result[base+1u]=result.normalRaw;mesh_result[base+2u]=result.normalUnit;mesh_result[base+3u]=result.binormal;mesh_result[base+4u]=result.tangent;\n}\n"):
    "mesh_result[id.x]=vs_main(id.x).pos;\n}\n";
  const std::array inputEntries{
    wgpu::BindGroupLayoutEntry{.binding=0,.visibility=wgpu::ShaderStage::Compute,.buffer={.type=wgpu::BufferBindingType::ReadOnlyStorage}},
    wgpu::BindGroupLayoutEntry{.binding=1,.visibility=wgpu::ShaderStage::Compute,.buffer={.type=wgpu::BufferBindingType::ReadOnlyStorage}}};
  const wgpu::BindGroupLayoutDescriptor inputDesc{.entryCount=inputEntries.size(),.entries=inputEntries.data()};
  const auto inputs=webgpu::g_device.CreateBindGroupLayout(&inputDesc);
  const wgpu::BindGroupLayoutEntry uniform{.binding=0,.visibility=wgpu::ShaderStage::Compute,.buffer={.type=wgpu::BufferBindingType::Uniform,.hasDynamicOffset=true}};
  const wgpu::BindGroupLayoutDescriptor uniformDesc{.entryCount=1,.entries=&uniform};
  const auto uniforms=webgpu::g_device.CreateBindGroupLayout(&uniformDesc);
  const wgpu::BindGroupLayoutDescriptor emptyDesc{};const auto empty=webgpu::g_device.CreateBindGroupLayout(&emptyDesc);
  const wgpu::BindGroupLayoutEntry output{.binding=0,.visibility=wgpu::ShaderStage::Compute,.buffer={.type=wgpu::BufferBindingType::Storage}};
  const wgpu::BindGroupLayoutDescriptor outputDesc{.entryCount=1,.entries=&output};const auto outputs=webgpu::g_device.CreateBindGroupLayout(&outputDesc);
  const std::array layouts{inputs,uniforms,empty,outputs};
  const wgpu::PipelineLayoutDescriptor layoutDesc{.bindGroupLayoutCount=layouts.size(),.bindGroupLayouts=layouts.data(),.immediateSize=sizeof(gx::DrawImmediateData)};
  const auto layout=webgpu::g_device.CreatePipelineLayout(&layoutDesc);
  wgpu::ShaderSourceWGSL wgsl{};wgsl.code=source.c_str();
  const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&wgsl,.label="Original fighter production vertex function replay"};
  auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
  const wgpu::ComputePipelineDescriptor descriptor{.layout=layout,.compute={.module=module,.entryPoint="mesh_main"}};
  auto compiled=webgpu::g_device.CreateComputePipeline(&descriptor);ReplayPipeline result{compiled,directionMask,colors};pipelines.emplace(key,result);
  std::fprintf(stderr,"MESH_GPU_PIPELINE shader=%llu ready=%u direction_mask=%u\n",(unsigned long long)hash,unsigned(bool(compiled)),directionMask);return result;
}
struct Job {
  wgpu::Buffer output,readback;
  std::array<double,AURORA_MESH_POINTS*4> expected;
  std::array<double,AURORA_MESH_POINTS*16> expectedDirections;
  std::array<double,AURORA_MESH_POINTS*8> expectedColors;
  std::array<double,AURORA_MESH_POINTS*16> expectedUv;
  uint32_t uvMask=0;
  std::array<double,AURORA_MESH_POINTS*16> expectedTexgens;
  uint32_t texgenMask=0;
  uint64_t id,packet,shader,geometryHash,matrixHash,expectedHash;
  uint32_t actor,gameFrame,nbt,primitive,points,mask,firstVertex,paletteMask,normalMask,directionMask,colorMask,colorLanes;
};
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame,const detail::RenderPass& pass){
  if(!gx::mesh_diagnostic::enabled())return;
  for(const auto& snapshot:pass.originalMeshDraws){
    const auto& draw=snapshot.draw;const auto& source=snapshot.context->source;
    auto compiled=pipeline(snapshot.config,source.normalComponents!=0,snapshot.colorMask,source.uvMask,snapshot.texgenMask);auto compute=compiled.compute;if(!compute)continue;
    auto job=std::make_shared<Job>();job->expected=snapshot.expected;job->id=snapshot.context->id;
    job->expectedDirections=snapshot.expectedDirections;job->directionMask=compiled.directionMask;
    job->expectedColors=snapshot.expectedColors;job->colorMask=compiled.colorMask;job->colorLanes=snapshot.colorLanes;
    job->uvMask=source.uvMask;
    job->texgenMask=snapshot.texgenMask;job->expectedTexgens=snapshot.expectedTexgens;
    for(unsigned n=0;n<snapshot.points;n++)for(unsigned i=0;i<8;i++)for(unsigned c=0;c<2;c++)
      job->expectedUv[n*16+i*2+c]=source.primitive[snapshot.primitive].point[n].uv[i][c];
    if(job->directionMask==15&&source.normalComponents!=9){std::fprintf(stderr,"MESH_GPU_PIPELINE_FAIL reason=nbt-source-missing\n");continue;}
    job->packet=frame.frameId;job->shader=xxh3_hash(snapshot.config);job->actor=source.actor;job->gameFrame=source.frame;
    job->geometryHash=snapshot.context->geometryHash;job->matrixHash=snapshot.matrixHash;job->expectedHash=snapshot.expectedHash;
    job->paletteMask=source.paletteMask;job->normalMask=source.normalMask;
    job->nbt=source.nbt;job->primitive=snapshot.primitive;job->points=snapshot.points;job->mask=snapshot.selectorMask;job->firstVertex=snapshot.firstVertex;
    const uint32_t size=job->points*(job->texgenMask?368:job->uvMask?240:job->colorMask?112:job->directionMask?80:16);
    const wgpu::BufferDescriptor outputDesc{.usage=wgpu::BufferUsage::Storage|wgpu::BufferUsage::CopySrc,.size=size};
    const wgpu::BufferDescriptor readDesc{.usage=wgpu::BufferUsage::MapRead|wgpu::BufferUsage::CopyDst,.size=size};
    job->output=webgpu::g_device.CreateBuffer(&outputDesc);job->readback=webgpu::g_device.CreateBuffer(&readDesc);
    const auto& resources=detail::resources();
    const std::array inputs{wgpu::BindGroupEntry{.binding=0,.buffer=resources.vertexBuffer,.size=VertexBufferSize},wgpu::BindGroupEntry{.binding=1,.buffer=resources.storageBuffer,.size=StorageBufferSize}};
    const wgpu::BindGroupDescriptor inputDesc{.layout=compute.GetBindGroupLayout(0),.entryCount=inputs.size(),.entries=inputs.data()};const auto input=webgpu::g_device.CreateBindGroup(&inputDesc);
    const wgpu::BindGroupEntry uniform{.binding=0,.buffer=resources.uniformBuffer,.size=draw.uniformRange.size};
    const wgpu::BindGroupDescriptor uniformDesc{.layout=compute.GetBindGroupLayout(1),.entryCount=1,.entries=&uniform};const auto uniforms=webgpu::g_device.CreateBindGroup(&uniformDesc);
    const wgpu::BindGroupDescriptor emptyDesc{.layout=compute.GetBindGroupLayout(2)};const auto empty=webgpu::g_device.CreateBindGroup(&emptyDesc);
    const wgpu::BindGroupEntry output{.binding=0,.buffer=job->output,.size=size};
    const wgpu::BindGroupDescriptor resultDesc{.layout=compute.GetBindGroupLayout(3),.entryCount=1,.entries=&output};const auto result=webgpu::g_device.CreateBindGroup(&resultDesc);
    auto immediates=draw.immediateData;
    // Equivalent contiguous vertex slice of the actual merged draw. Only the
    // diagnostic invocation's base changes;the production draw is untouched.
    immediates.vtxStart+=snapshot.firstVertex*snapshot.config.vtxStride;
    auto dispatch=cmd.BeginComputePass();dispatch.SetPipeline(compute);dispatch.SetBindGroup(0,input);
    dispatch.SetBindGroup(1,uniforms,1,&draw.uniformRange.offset);dispatch.SetBindGroup(2,empty);dispatch.SetBindGroup(3,result);
    dispatch.SetImmediates(0,&immediates,sizeof(immediates));dispatch.DispatchWorkgroups(job->points);dispatch.End();
    cmd.CopyBufferToBuffer(job->output,0,job->readback,0,size);
    std::fprintf(stderr,"MESH_GPU_QUEUED id=%llu primitive=%u actor=%u game_frame=%u packet=%llu nbt=%u points=%u mask=%u first_vertex=%u shader=%llu geometry_hash=%llu matrix_hash=%llu expected_hash=%llu\n",
      (unsigned long long)job->id,job->primitive,job->actor,job->gameFrame,(unsigned long long)job->packet,job->nbt,job->points,job->mask,job->firstVertex,(unsigned long long)job->shader,
      (unsigned long long)job->geometryHash,(unsigned long long)job->matrixHash,(unsigned long long)job->expectedHash);
    frame.afterSubmitCallbacks.emplace_back([job,size]{
      job->readback.MapAsync(wgpu::MapMode::Read,0,size,wgpu::CallbackMode::AllowSpontaneous,[job,size](wgpu::MapAsyncStatus status,wgpu::StringView){
        const auto* values=status==wgpu::MapAsyncStatus::Success?static_cast<const float*>(job->readback.GetConstMappedRange(0,size)):nullptr;
        unsigned mismatches=0,rejections=0,directionMismatches=0,directionRejections=0,directionComponents=0;double maximum=0,directionMaximum=0;
        const unsigned stride=job->texgenMask?92:job->uvMask?60:job->colorMask?28:job->directionMask?20:4;
        for(uint32_t n=0;n<job->points*4;n++){
          const double expected=job->expected[n],actual=values?values[(n/4)*stride+n%4]:NAN;
          const double error=std::abs(actual-expected),tolerance=2e-5*(1+std::abs(expected));
          if(!std::isfinite(actual)||error>tolerance)mismatches++;
          if(std::isfinite(error))maximum=std::max(maximum,error);
          if(std::isfinite(actual)&&std::abs(actual-(expected+.25*(1+std::abs(expected))))>tolerance)rejections++;
        }
        uint64_t gpuHash=14695981039346656037ull,directionGpuHash=gpuHash,directionExpectedHash=gpuHash;
        unsigned colorMismatches=0,colorComponents=0,colorRejections=0;double colorMaximum=0;
        uint64_t colorGpuHash=gpuHash,colorExpectedHash=gpuHash;
        unsigned uvMismatches=0,uvComponents=0,uvRejections=0;double uvMaximum=0;
        uint64_t uvGpuHash=gpuHash,uvExpectedHash=gpuHash;
        unsigned texgenMismatches=0,texgenComponents=0,texgenRejections=0;double texgenMaximum=0;
        uint64_t texgenGpuHash=gpuHash,texgenExpectedHash=gpuHash;
        for(unsigned point=0;point<job->points;point++){
          for(unsigned i=0;i<8;i++)if(job->texgenMask&(1u<<i))for(unsigned c=0;c<2;c++){
            const double expected=job->expectedTexgens[point*16+i*2+c];
            const float actual=values?values[point*stride+60+i*4+c]:NAN;
            const double error=std::abs(double(actual)-expected),tolerance=3e-6*(1+std::abs(expected));texgenComponents++;
            if(!std::isfinite(actual)||error>tolerance)texgenMismatches++;
            if(std::isfinite(error))texgenMaximum=std::max(texgenMaximum,error);
            if(std::isfinite(actual)&&std::abs(actual-(expected+.25*(1+std::abs(expected))))>tolerance)texgenRejections++;
            texgenGpuHash=gx::mesh_diagnostic::fingerprint(&actual,sizeof(actual),texgenGpuHash);
            texgenExpectedHash=gx::mesh_diagnostic::fingerprint(&expected,sizeof(expected),texgenExpectedHash);
          }
          for(unsigned i=0;i<8;i++)if(job->uvMask&(1u<<i))for(unsigned c=0;c<2;c++){
            const double expected=job->expectedUv[point*16+i*2+c];
            const float actual=values?values[point*stride+28+i*4+c]:NAN;
            const double error=std::abs(double(actual)-expected),tolerance=2e-6*(1+std::abs(expected));uvComponents++;
            if(!std::isfinite(actual)||error>tolerance)uvMismatches++;
            if(std::isfinite(error))uvMaximum=std::max(uvMaximum,error);
            if(std::isfinite(actual)&&std::abs(actual-(expected+.25*(1+std::abs(expected))))>tolerance)uvRejections++;
            uvGpuHash=gx::mesh_diagnostic::fingerprint(&actual,sizeof(actual),uvGpuHash);
            uvExpectedHash=gx::mesh_diagnostic::fingerprint(&expected,sizeof(expected),uvExpectedHash);
          }
          if(values)gpuHash=gx::mesh_diagnostic::fingerprint(values+point*stride,16,gpuHash);
          for(unsigned row=0;row<4;row++)if(job->directionMask&(1u<<row))for(unsigned c=0;c<4;c++){
            const double expected=job->expectedDirections[point*16+row*4+c];
            const float actual=values?values[point*stride+4+row*4+c]:NAN;
            const double error=std::abs(double(actual)-expected),tolerance=3e-5*(1+std::abs(expected));directionComponents++;
            if(!std::isfinite(actual)||error>tolerance)directionMismatches++;
            if(std::isfinite(error))directionMaximum=std::max(directionMaximum,error);
            if(std::isfinite(actual)&&std::abs(actual-(expected+.25*(1+std::abs(expected))))>tolerance)directionRejections++;
            directionGpuHash=gx::mesh_diagnostic::fingerprint(&actual,sizeof(actual),directionGpuHash);
            directionExpectedHash=gx::mesh_diagnostic::fingerprint(&expected,sizeof(expected),directionExpectedHash);
          }
          for(unsigned channel=0;channel<2;channel++)if(job->colorMask&(1u<<channel))for(unsigned c=0;c<4;c++)if(job->colorLanes&(1u<<(channel*4+c))){
            const double expected=job->expectedColors[point*8+channel*4+c];
            const float actual=values?values[point*stride+20+channel*4+c]:NAN;
            const double error=std::abs(double(actual)-expected);colorComponents++;
            if(!std::isfinite(actual)||error>.5/255.)colorMismatches++;
            if(std::isfinite(error))colorMaximum=std::max(colorMaximum,error);
            if(std::isfinite(actual)&&std::abs(actual-(expected+.25))>.5/255.)colorRejections++;
            colorGpuHash=gx::mesh_diagnostic::fingerprint(&actual,sizeof(actual),colorGpuHash);
            colorExpectedHash=gx::mesh_diagnostic::fingerprint(&expected,sizeof(expected),colorExpectedHash);
          }
        }
        if(values)job->readback.Unmap();
        std::fprintf(stderr,"MESH_GPU_%s id=%llu primitive=%u actor=%u game_frame=%u packet=%llu nbt=%u points=%u mask=%u first_vertex=%u mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g geometry_hash=%llu matrix_hash=%llu expected_hash=%llu gpu_hash=%llu hsd_reference=%u palette_mask=%u normal_mask=%u direction_mask=%u direction_components=%u direction_mismatches=%u direction_wrong_reference_rejections=%u direction_max_abs_error=%.9g direction_expected_hash=%llu direction_gpu_hash=%llu\n",
          (mismatches||directionMismatches||colorMismatches||uvMismatches||texgenMismatches)?"FAIL":"PASS",(unsigned long long)job->id,job->primitive,job->actor,job->gameFrame,(unsigned long long)job->packet,
          job->nbt,job->points,job->mask,job->firstVertex,mismatches,rejections,maximum,
          (unsigned long long)job->geometryHash,(unsigned long long)job->matrixHash,(unsigned long long)job->expectedHash,(unsigned long long)gpuHash,
          unsigned(job->paletteMask!=0),job->paletteMask,job->normalMask,job->directionMask,directionComponents,directionMismatches,directionRejections,directionMaximum,
          (unsigned long long)directionExpectedHash,(unsigned long long)directionGpuHash);
        if(job->colorMask)std::fprintf(stderr,"MESH_COLOR_%s id=%llu primitive=%u actor=%u game_frame=%u mask=%u lanes=%u components=%u mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g expected_hash=%llu gpu_hash=%llu\n",
          colorMismatches?"FAIL":"PASS",(unsigned long long)job->id,job->primitive,job->actor,job->gameFrame,job->colorMask,job->colorLanes,colorComponents,colorMismatches,colorRejections,colorMaximum,
          (unsigned long long)colorExpectedHash,(unsigned long long)colorGpuHash);
        if(job->uvMask)std::fprintf(stderr,"MESH_UV_%s id=%llu primitive=%u actor=%u game_frame=%u mask=%u components=%u mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g expected_hash=%llu gpu_hash=%llu\n",
          uvMismatches?"FAIL":"PASS",(unsigned long long)job->id,job->primitive,job->actor,job->gameFrame,job->uvMask,uvComponents,uvMismatches,uvRejections,uvMaximum,
          (unsigned long long)uvExpectedHash,(unsigned long long)uvGpuHash);
        if(job->texgenMask)std::fprintf(stderr,"MESH_TEXGEN_%s id=%llu primitive=%u actor=%u game_frame=%u mask=%u components=%u mismatches=%u wrong_reference_rejections=%u max_abs_error=%.9g expected_hash=%llu gpu_hash=%llu\n",
          texgenMismatches?"FAIL":"PASS",(unsigned long long)job->id,job->primitive,job->actor,job->gameFrame,job->texgenMask,texgenComponents,texgenMismatches,texgenRejections,texgenMaximum,
          (unsigned long long)texgenExpectedHash,(unsigned long long)texgenGpuHash);
      });
    });
  }
}
}
