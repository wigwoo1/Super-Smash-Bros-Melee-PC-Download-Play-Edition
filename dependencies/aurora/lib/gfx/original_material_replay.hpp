#pragma once
#include "original_material_reference.hpp"
#include "original_texture_replay.hpp"
#include "resources.hpp"
namespace aurora::gfx::original_material {
struct Pipeline {wgpu::RenderPipeline render;wgpu::ComputePipeline capture;};
inline std::map<uint64_t,Pipeline> pipelines;
inline Pipeline pipeline(const gx::ShaderConfig& config,unsigned slot){
  const auto key=xxh3_hash(config);if(auto it=pipelines.find(key);it!=pipelines.end())return it->second;
  const auto info=gx::build_shader_info(config);
  if(!valid(config,slot)||info.sampledTextures.count()!=1||!info.sampledTextures[slot]||info.sampledTexCoords.count()!=1||pipelines.size()>=16){std::fprintf(stderr,"ORIGINAL_MATERIAL_GPU_FAIL reason=config-scope\n");return {};}
  unsigned coord=0;while(!info.sampledTexCoords[coord])++coord;
  if(config.tcgs[coord].type!=GX_TG_MTX2x4){std::fprintf(stderr,"ORIGINAL_MATERIAL_GPU_FAIL reason=texgen-scope\n");return {};}
  auto code=gx::build_shader_source(config);const std::string entry="@vertex\nfn vs_main(",index="@builtin(vertex_index) ";
  auto at=code.find(entry);if(at==std::string::npos||code.find("@builtin(instance_index)")!=std::string::npos){std::fprintf(stderr,"ORIGINAL_MATERIAL_GPU_FAIL reason=vertex-entry\n");return {};}
  // Only adapt the vertex entry;the complete production fs_main stays verbatim.
  const auto originalFragment=code.substr(code.find("@fragment"));code.erase(at,8);at=code.find(index);if(at==std::string::npos)return {};code.erase(at,index.size());
  if(code.substr(code.find("@fragment"))!=originalFragment)return {};
  code+="\n@group(3) @binding(0) var<storage,read_write> material_colors: array<vec4f>;\n"
    "@compute @workgroup_size(1) fn material_capture(){let result=vs_main(0u);";
  for(unsigned n=0;n<2;n++)code+="material_colors["+std::to_string(n)+"]="+(info.sampledColorChannels[n]?"result.cc"+std::to_string(n):"vec4f(0.0)")+";";
  code+="}\n@vertex fn material_vs(@builtin(vertex_index) id:u32)->VertexOutput {var out=vs_main(0u);"
    "let p=array<vec2f,3>(vec2f(-1.0,-1.0),vec2f(3.0,-1.0),vec2f(-1.0,3.0));let xy=p[id];out.pos=vec4f(xy,0.5,1.0);"
    "let uv=vec2f(xy.x*0.5+0.5,0.5-xy.y*0.5);out.tex"+std::to_string(coord)+"_uv=uv*ubuf.tex"+std::to_string(slot)+"_size_bias.xy/ubuf.texcoord_scale["+std::to_string(coord)+"].xy;return out;}\n";
  const auto vertexCompute=wgpu::ShaderStage::Vertex|wgpu::ShaderStage::Compute;
  const std::array inputEntries{
    wgpu::BindGroupLayoutEntry{.binding=0,.visibility=vertexCompute,.buffer={.type=wgpu::BufferBindingType::ReadOnlyStorage}},
    wgpu::BindGroupLayoutEntry{.binding=1,.visibility=vertexCompute,.buffer={.type=wgpu::BufferBindingType::ReadOnlyStorage}}};
  const wgpu::BindGroupLayoutDescriptor inputDesc{.entryCount=inputEntries.size(),.entries=inputEntries.data()};const auto inputs=webgpu::g_device.CreateBindGroupLayout(&inputDesc);
  const wgpu::BindGroupLayoutEntry uniform{.binding=0,.visibility=vertexCompute|wgpu::ShaderStage::Fragment,.buffer={.type=wgpu::BufferBindingType::Uniform,.hasDynamicOffset=true}};
  const wgpu::BindGroupLayoutDescriptor uniformDesc{.entryCount=1,.entries=&uniform};const auto uniformLayout=webgpu::g_device.CreateBindGroupLayout(&uniformDesc);
  const wgpu::BindGroupLayoutEntry output{.binding=0,.visibility=wgpu::ShaderStage::Compute,.buffer={.type=wgpu::BufferBindingType::Storage}};
  const wgpu::BindGroupLayoutDescriptor outputDesc{.entryCount=1,.entries=&output};const auto outputs=webgpu::g_device.CreateBindGroupLayout(&outputDesc);
  const std::array layouts{inputs,uniformLayout,gx::diagnostic_texture_bind_group_layout(),outputs};
  const wgpu::PipelineLayoutDescriptor layoutDesc{.bindGroupLayoutCount=layouts.size(),.bindGroupLayouts=layouts.data(),.immediateSize=sizeof(gx::DrawImmediateData)};const auto layout=webgpu::g_device.CreatePipelineLayout(&layoutDesc);
  wgpu::ShaderSourceWGSL source{};source.code=code.c_str();const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&source,.label="Original material complete fragment replay"};const auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
  const wgpu::ColorTargetState target{.format=wgpu::TextureFormat::RGBA8Unorm};const wgpu::FragmentState fragment{.module=module,.entryPoint="fs_main",.targetCount=1,.targets=&target};
  const wgpu::RenderPipelineDescriptor renderDesc{.layout=layout,.vertex={.module=module,.entryPoint="material_vs"},.primitive={.topology=wgpu::PrimitiveTopology::TriangleList},.fragment=&fragment};
  const wgpu::ComputePipelineDescriptor computeDesc{.layout=layout,.compute={.module=module,.entryPoint="material_capture"}};
  Pipeline result{webgpu::g_device.CreateRenderPipeline(&renderDesc),webgpu::g_device.CreateComputePipeline(&computeDesc)};pipelines.emplace(key,result);return result;
}
struct Job {
  original_texture::Snapshot snapshot;Inputs input;std::vector<uint8_t> decoded;
  wgpu::Texture target;wgpu::Buffer colors,readback;uint64_t packet=0;unsigned rowBytes=0;
};
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame,const detail::RenderPass& pass){
  if(!original_texture::enabled()||!std::getenv("MELEE_DIAGNOSTIC_ORIGINAL_MATERIAL"))return;
  for(const auto& s:pass.originalTextureDraws){
    const auto& m=s.source->metadata;const auto& draw=s.draw;const auto info=gx::build_shader_info(s.config);auto job=std::make_shared<Job>();job->snapshot=s;job->packet=frame.frameId;job->rowBytes=(m.width*4+255)&~255u;
    if(draw.uniformRange.offset>frame.uniforms.size()||draw.uniformRange.size>frame.uniforms.size()-draw.uniformRange.offset||!uniforms(std::span(frame.uniforms.data()+draw.uniformRange.offset,draw.uniformRange.size),s.config,info,job->input)||
       !original_texture::decode_original(s.source->bytes,s.source->palette,m.format,m.paletteFormat,m.width,m.height,job->decoded)||draw.instanceCount!=1||s.firstVertex>=draw.vtxCount){std::fprintf(stderr,"ORIGINAL_MATERIAL_GPU_FAIL reason=source-inputs\n");continue;}
    const auto compiled=pipeline(s.config,s.slot);const auto texture=find_bind_group(draw.bindGroups.textureBindGroup);if(!compiled.render||!compiled.capture||!texture){std::fprintf(stderr,"ORIGINAL_MATERIAL_GPU_FAIL reason=actual-bindings\n");continue;}
    const auto& res=detail::resources();const auto& compute=compiled.capture;
    const std::array bindings{wgpu::BindGroupEntry{.binding=0,.buffer=res.vertexBuffer,.size=VertexBufferSize},wgpu::BindGroupEntry{.binding=1,.buffer=res.storageBuffer,.size=StorageBufferSize}};
    const wgpu::BindGroupDescriptor inputDesc{.layout=compute.GetBindGroupLayout(0),.entryCount=bindings.size(),.entries=bindings.data()};const auto input=webgpu::g_device.CreateBindGroup(&inputDesc);
    const wgpu::BindGroupEntry uniform{.binding=0,.buffer=res.uniformBuffer,.size=draw.uniformRange.size};const wgpu::BindGroupDescriptor uniformDesc{.layout=compute.GetBindGroupLayout(1),.entryCount=1,.entries=&uniform};const auto uniformGroup=webgpu::g_device.CreateBindGroup(&uniformDesc);
    const wgpu::BufferDescriptor colorsDesc{.usage=wgpu::BufferUsage::Storage|wgpu::BufferUsage::CopySrc,.size=32};job->colors=webgpu::g_device.CreateBuffer(&colorsDesc);
    const wgpu::BindGroupEntry output{.binding=0,.buffer=job->colors,.size=32};const wgpu::BindGroupDescriptor outputDesc{.layout=compute.GetBindGroupLayout(3),.entryCount=1,.entries=&output};const auto outputGroup=webgpu::g_device.CreateBindGroup(&outputDesc);
    const uint64_t readSize=256+uint64_t(job->rowBytes)*m.height;const wgpu::BufferDescriptor readDesc{.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=readSize};job->readback=webgpu::g_device.CreateBuffer(&readDesc);
    const wgpu::TextureDescriptor targetDesc{.usage=wgpu::TextureUsage::RenderAttachment|wgpu::TextureUsage::CopySrc,.dimension=wgpu::TextureDimension::e2D,.size={m.width,m.height,1},.format=wgpu::TextureFormat::RGBA8Unorm,.mipLevelCount=1,.sampleCount=1};job->target=webgpu::g_device.CreateTexture(&targetDesc);
    auto immediates=draw.immediateData;immediates.vtxStart+=s.firstVertex*s.config.vtxStride;
    auto dispatch=cmd.BeginComputePass();dispatch.SetPipeline(compute);dispatch.SetBindGroup(0,input);dispatch.SetBindGroup(1,uniformGroup,1,&draw.uniformRange.offset);dispatch.SetBindGroup(2,texture);dispatch.SetBindGroup(3,outputGroup);dispatch.SetImmediates(0,&immediates,sizeof(immediates));dispatch.DispatchWorkgroups(1);dispatch.End();cmd.CopyBufferToBuffer(job->colors,0,job->readback,0,32);
    const wgpu::RenderPassColorAttachment attachment{.view=job->target.CreateView(),.loadOp=wgpu::LoadOp::Clear,.storeOp=wgpu::StoreOp::Store,.clearValue={13./255,29./255,47./255,61./255}};const wgpu::RenderPassDescriptor renderDesc{.colorAttachmentCount=1,.colorAttachments=&attachment};auto render=cmd.BeginRenderPass(&renderDesc);
    render.SetPipeline(compiled.render);render.SetBindGroup(0,input);render.SetBindGroup(1,uniformGroup,1,&draw.uniformRange.offset);render.SetBindGroup(2,texture);render.SetBindGroup(3,outputGroup);render.SetImmediates(0,&immediates,sizeof(immediates));render.Draw(3);render.End();
    const wgpu::TexelCopyTextureInfo from{.texture=job->target};const wgpu::TexelCopyBufferInfo to{.layout={.offset=256,.bytesPerRow=job->rowBytes,.rowsPerImage=m.height},.buffer=job->readback};const wgpu::Extent3D extent{m.width,m.height,1};cmd.CopyTextureToBuffer(&from,&to,&extent);
    std::fprintf(stderr,"ORIGINAL_MATERIAL_GPU_QUEUED source=%llu texture=%u image=%u sequence=%llu packet=%llu first_vertex=%u pixels=%u\n",(unsigned long long)s.source->id,m.textureOffset,m.imageOffset,(unsigned long long)s.sequence,(unsigned long long)job->packet,s.firstVertex,m.width*m.height);
    frame.afterSubmitCallbacks.emplace_back([job,readSize]{job->readback.MapAsync(wgpu::MapMode::Read,0,readSize,wgpu::CallbackMode::AllowSpontaneous,[job,readSize](wgpu::MapAsyncStatus status,wgpu::StringView){
      const auto& s=job->snapshot;const auto& m=s.source->metadata;const auto* bytes=status==wgpu::MapAsyncStatus::Success?static_cast<const uint8_t*>(job->readback.GetConstMappedRange(0,readSize)):nullptr;
      unsigned mismatches=0,acceptedPixels=0,discardedPixels=0,components=0,maxError=0;std::vector<uint8_t> expected(m.width*m.height*4),actual(expected.size());
      if(!bytes)++mismatches;
      if(bytes){for(unsigned n=0;n<8;n++){float f;std::memcpy(&f,bytes+n*4,4);if(!std::isfinite(f)||f<0||f>1)++mismatches;job->input.raster[n/4][n%4]=std::isfinite(f)?int(std::round(double(f)*255)):0;}
        for(unsigned y=0;y<m.height;y++)for(unsigned x=0;x<m.width;x++){
          const unsigned at=(y*m.width+x)*4;Color texture{};for(unsigned c=0;c<4;c++)texture[c]=job->decoded[at+c];Color result{};bool accepted=false;
          if(!evaluate(s.config,s.slot,job->input,texture,result,accepted))++mismatches;
          if(accepted)++acceptedPixels;else {++discardedPixels;result={13,29,47,61};}
          for(unsigned c=0;c<4;c++){expected[at+c]=uint8_t(result[c]);actual[at+c]=bytes[256+y*job->rowBytes+x*4+c];const unsigned error=unsigned(std::abs(int(actual[at+c])-result[c]));maxError=std::max(maxError,error);++components;if(error)++mismatches;}
        }
      }
      const auto expectedHash=original_texture::hash(expected.data(),expected.size()),actualHash=original_texture::hash(actual.data(),actual.size());if(bytes)job->readback.Unmap();
      std::fprintf(stderr,"ORIGINAL_MATERIAL_GPU_%s source=%llu texture=%u image=%u sequence=%llu packet=%llu first_vertex=%u pixels=%u components=%u accepted=%u discarded=%u mismatches=%u max_byte_error=%u expected_hash=%llu actual_hash=%llu\n",mismatches?"FAIL":"PASS",(unsigned long long)s.source->id,m.textureOffset,m.imageOffset,(unsigned long long)s.sequence,(unsigned long long)job->packet,s.firstVertex,m.width*m.height,components,acceptedPixels,discardedPixels,mismatches,maxError,(unsigned long long)expectedHash,(unsigned long long)actualHash);
    });});
  }
}
}
