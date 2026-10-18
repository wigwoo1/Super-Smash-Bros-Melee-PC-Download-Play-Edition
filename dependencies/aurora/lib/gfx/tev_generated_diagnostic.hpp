#pragma once
#include "../gx/gx.hpp"
#include "../gx/shader_info.hpp"
#include "../webgpu/gpu.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
namespace aurora::gfx::tev_generated_diagnostic {
inline constexpr Module Log{"aurora::gfx::tev_generated_diagnostic"};
constexpr unsigned Cases=128, ReadbackSize=Cases*256;
struct Job {
  wgpu::Texture target;
  wgpu::Buffer vertices, uniforms, readback;
  std::array<std::array<unsigned char,4>,Cases> expected{};
  std::atomic<int> status{0};
};
inline std::shared_ptr<Job> pending;
inline void drain() {
  if(!pending || !pending->status.load(std::memory_order_acquire))return;
  unsigned mismatch=Cases;
  if(pending->status.load()==1) {
    const auto* bytes=static_cast<const unsigned char*>(pending->readback.GetConstMappedRange(0,ReadbackSize));
    if(bytes) {
      mismatch=0;
      for(unsigned i=0;i<Cases;++i) {
        if(std::memcmp(bytes+i*256,pending->expected[i].data(),4)) {
          if(mismatch<4)std::fprintf(stderr,"GPU_TEV_GENERATED_DIFF case=%u actual=%u,%u,%u,%u expected=%u,%u,%u,%u\n",i,bytes[i*256],bytes[i*256+1],bytes[i*256+2],bytes[i*256+3],pending->expected[i][0],pending->expected[i][1],pending->expected[i][2],pending->expected[i][3]);
          ++mismatch;
        }
      }
    }
    pending->readback.Unmap();
  }
  std::fprintf(stderr,"GPU_TEV_GENERATED_%s cases=128 pipelines=16 compared_bytes=512 mismatches=%u scope=complete_generated_shader_pixels\n",mismatch?"FAIL":"PASS",mismatch);
  pending.reset();
}
inline void encode(const wgpu::CommandEncoder& cmd,detail::FramePacket& frame) {
  static bool started=false;
  static unsigned step=0;
  static std::shared_ptr<Job> building;
  if(started || !std::getenv("AURORA_DIAGNOSTIC_TEV_GENERATED"))return;
  if(!building)building=std::make_shared<Job>();
  auto job=building;
  const auto pack=[](const std::array<int,4>& v,unsigned mode) {
    return (v[0]&255)+(mode>0?(v[1]&255)*256:0)+(mode>1?(v[2]&255)*65536:0);
  };
  const auto regular=[](int a,int b,int c,int d,bool sub,int bias,unsigned scale) {
    a&=255;b&=255;c&=255;const int factor=c+(c>>7);
    const int weighted=a*(256-factor)+b*factor;
    const int multiplier=scale==3?1:1<<scale;
    const int lerp=(weighted*multiplier+(scale==3?0:sub?127:128))>>8;
    int result=(d+bias)*multiplier+(sub?-lerp:lerp);
    if(scale==3)result>>=1;
    return std::clamp(result,-1024,1023);
  };
  std::array<gx::ShaderConfig,16> configs{};
  for(unsigned variant=0;variant<16;++variant) {
    auto& config=configs[variant];config.vtxStride=12;
    config.attrs[GX_VA_POS]={.attrType=GX_DIRECT,.cnt=3,.compType=GX_F32,.offset=0,.stride=12,.frac=0,.le=false};
    config.tevStageCount=variant<8?1:2;
    auto& stage=config.tevStages[0];
    stage.colorPass={GX_CC_C0,GX_CC_C1,GX_CC_C2,GX_CC_ZERO};
    stage.alphaPass={GX_CA_APREV,GX_CA_A0,GX_CA_A1,GX_CA_A2};
    stage.colorOp.op=stage.alphaOp.op=static_cast<GXTevOp>(underlying(GX_TEV_COMP_R8_GT)+variant%8);
    stage.colorOp.clamp=stage.alphaOp.clamp=false;
    stage.alphaOp.outReg=variant<8?GX_TEVPREV:GX_TEVREG0;
    if(variant>=8) {
      auto& next=config.tevStages[1];
      next.colorPass={GX_CC_A0,GX_CC_CPREV,GX_CC_HALF,GX_CC_C1};
      next.alphaPass={GX_CA_APREV,GX_CA_A0,GX_CA_A1,GX_CA_A2};
      next.colorOp.clamp=next.alphaOp.clamp=false;
      next.alphaOp.op=GX_TEV_SUB;next.alphaOp.bias=GX_TB_ADDHALF;next.alphaOp.scale=GX_CS_SCALE_2;
    }
    const auto info=gx::build_shader_info(config);
    // Every fixture genuinely reads all four initial registers. Before the
    // component-aware analysis, initial PREV alpha was omitted here.
    AURORA_ASSERT(info.loadsTevReg.all(),"Generated TEV fixture missing initial register");
  }
  constexpr unsigned UniformStride=2048;
  std::array<float,(UniformStride*Cases)/4> uniformBytes{};
  for(unsigned i=0;i<Cases;++i) {
    const unsigned variant=i/8,sample=i%8,mode=(variant%8)/2;
    const bool eq=variant&1;
    std::array<std::array<int,4>,4> regs{};
    regs[0]={13,29,47,int(sample*31)-128};
    regs[1]={int(sample*19)+32,128-int(sample*13),int(sample*7)+64,int(sample*23)-64};
    regs[2]=regs[1];regs[2][sample%3]+=(sample%4==0?0:sample%2==0?1:-1);
    regs[2][3]=int(sample*29)+33;regs[3]={192,96,32,int(sample*47)-256};
    const auto old=regs;
    const auto test=[eq](int a,int b){return eq?a==b:a>b;};
    for(unsigned channel=0;channel<3;++channel) {
      const bool pass=mode==3?test(old[1][channel]&255,old[2][channel]&255):test(pack(old[1],mode),pack(old[2],mode));
      regs[0][channel]=pass?(old[3][channel]&255):0;
    }
    const bool alphaTest=mode==3?test(old[0][3]&255,old[1][3]&255):test(pack(old[1],mode),pack(old[2],mode));
    const int alpha=std::clamp(old[3][3]+(alphaTest?(old[2][3]&255):0),-1024,1023);
    if(variant<8)regs[0][3]=alpha;
    else {
      regs[1][3]=alpha;const auto previous=regs;
      for(unsigned channel=0;channel<3;++channel)
        regs[0][channel]=regular(previous[1][3],previous[0][channel],128,previous[2][channel],false,0,0);
      regs[0][3]=regular(previous[0][3],previous[1][3],previous[2][3],previous[3][3],true,128,1);
    }
    for(unsigned channel=0;channel<4;++channel)job->expected[i][channel]=static_cast<unsigned char>(regs[0][channel]&255);
    auto* uniform=uniformBytes.data()+i*(UniformStride/4);
    uniform[0]=uniform[1]=uniform[2]=uniform[3]=1.f;
    // Source-layout independent fixture: viewport16, projection64,
    // 20 position/texture matrices960, 10 normal matrices480, registers64.
    for(unsigned axis=0;axis<4;++axis)uniform[4+axis*5]=1.f;
    for(unsigned axis=0;axis<3;++axis) {uniform[20+axis*5]=1.f;uniform[260+axis*5]=1.f;}
    for(unsigned reg=0;reg<4;++reg)for(unsigned channel=0;channel<4;++channel)
      uniform[380+reg*4+channel]=float(old[reg][channel])/255.f;
  }
  const std::array<float,9> vertices{-1.f,-1.f,0.f,3.f,-1.f,0.f,-1.f,3.f,0.f};
  std::array<uint32_t,9> vertexWords{};
  for(unsigned i=0;i<vertices.size();++i) {
    const auto bits=std::bit_cast<uint32_t>(vertices[i]);
    // Direct GX display-list float attributes are big-endian.
    vertexWords[i]=((bits&255u)<<24)|((bits&65280u)<<8)|((bits>>8)&65280u)|(bits>>24);
  }
  if(step==0) {
  const wgpu::BufferDescriptor vertexDesc{.usage=wgpu::BufferUsage::Storage,.size=sizeof(vertices),.mappedAtCreation=true};
  job->vertices=webgpu::g_device.CreateBuffer(&vertexDesc);std::memcpy(job->vertices.GetMappedRange(),vertexWords.data(),sizeof(vertexWords));job->vertices.Unmap();
  const wgpu::BufferDescriptor uniformDesc{.usage=wgpu::BufferUsage::Uniform,.size=sizeof(uniformBytes),.mappedAtCreation=true};
  job->uniforms=webgpu::g_device.CreateBuffer(&uniformDesc);std::memcpy(job->uniforms.GetMappedRange(),uniformBytes.data(),sizeof(uniformBytes));job->uniforms.Unmap();
  const wgpu::BufferDescriptor readDesc{.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=ReadbackSize};
  job->readback=webgpu::g_device.CreateBuffer(&readDesc);
  const wgpu::TextureDescriptor targetDesc{.usage=wgpu::TextureUsage::RenderAttachment|wgpu::TextureUsage::CopySrc,.dimension=wgpu::TextureDimension::e2D,.size={1,Cases,1},.format=wgpu::TextureFormat::RGBA8Unorm,.mipLevelCount=1,.sampleCount=1};
  job->target=webgpu::g_device.CreateTexture(&targetDesc);
  }
  const std::array inputEntries{
    wgpu::BindGroupLayoutEntry{.binding=0,.visibility=wgpu::ShaderStage::Vertex,.buffer={.type=wgpu::BufferBindingType::ReadOnlyStorage}},
    wgpu::BindGroupLayoutEntry{.binding=1,.visibility=wgpu::ShaderStage::Vertex,.buffer={.type=wgpu::BufferBindingType::ReadOnlyStorage}}};
  const wgpu::BindGroupLayoutDescriptor inputDesc{.entryCount=inputEntries.size(),.entries=inputEntries.data()};
  const auto inputLayout=webgpu::g_device.CreateBindGroupLayout(&inputDesc);
  const wgpu::BindGroupLayoutEntry uniformEntry{.binding=0,.visibility=wgpu::ShaderStage::Vertex|wgpu::ShaderStage::Fragment,.buffer={.type=wgpu::BufferBindingType::Uniform,.hasDynamicOffset=true,.minBindingSize=UniformStride}};
  const wgpu::BindGroupLayoutDescriptor uniformLayoutDesc{.entryCount=1,.entries=&uniformEntry};
  const auto uniformLayout=webgpu::g_device.CreateBindGroupLayout(&uniformLayoutDesc);
  const std::array layouts{inputLayout,uniformLayout};
  const wgpu::PipelineLayoutDescriptor layoutDesc{.bindGroupLayoutCount=layouts.size(),.bindGroupLayouts=layouts.data(),.immediateSize=sizeof(gx::DrawImmediateData)};
  const auto layout=webgpu::g_device.CreatePipelineLayout(&layoutDesc);
  const std::array vertexBindings{wgpu::BindGroupEntry{.binding=0,.buffer=job->vertices,.size=sizeof(vertices)},wgpu::BindGroupEntry{.binding=1,.buffer=job->vertices,.size=sizeof(vertices)}};
  const wgpu::BindGroupDescriptor verticesDesc{.layout=inputLayout,.entryCount=vertexBindings.size(),.entries=vertexBindings.data()};
  const auto vertexBind=webgpu::g_device.CreateBindGroup(&verticesDesc);
  const wgpu::BindGroupEntry uniformBinding{.binding=0,.buffer=job->uniforms,.size=UniformStride};
  const wgpu::BindGroupDescriptor uniformBindDesc{.layout=uniformLayout,.entryCount=1,.entries=&uniformBinding};
  const auto uniformBind=webgpu::g_device.CreateBindGroup(&uniformBindDesc);
  const wgpu::ColorTargetState colorTarget{.format=wgpu::TextureFormat::RGBA8Unorm};
  std::array<wgpu::RenderPipeline,16> pipelines;
  for(unsigned variant=step;variant<step+1;++variant) {
    const auto sourceText=gx::build_shader_source(configs[variant]);
    wgpu::ShaderSourceWGSL wgsl{};wgsl.code=sourceText.c_str();
    const wgpu::ShaderModuleDescriptor moduleDesc{.nextInChain=&wgsl,.label="Complete generated TEV shader"};
    const auto module=webgpu::g_device.CreateShaderModule(&moduleDesc);
    const wgpu::FragmentState fragment{.module=module,.entryPoint="fs_main",.targetCount=1,.targets=&colorTarget};
    const wgpu::RenderPipelineDescriptor pipelineDesc{.label="Complete generated TEV pipeline",.layout=layout,.vertex={.module=module,.entryPoint="vs_main"},.primitive={.topology=wgpu::PrimitiveTopology::TriangleList},.fragment=&fragment};
    pipelines[variant]=webgpu::g_device.CreateRenderPipeline(&pipelineDesc);
  }
  const wgpu::RenderPassColorAttachment attachment{.view=job->target.CreateView(),.loadOp=step==0?wgpu::LoadOp::Clear:wgpu::LoadOp::Load,.storeOp=wgpu::StoreOp::Store};
  const wgpu::RenderPassDescriptor passDesc{.colorAttachmentCount=1,.colorAttachments=&attachment};
  const auto pass=cmd.BeginRenderPass(&passDesc);gx::DrawImmediateData immediate{};
  pass.SetPipeline(pipelines[step]);pass.SetBindGroup(0,vertexBind);pass.SetImmediates(0,&immediate,sizeof(immediate));
  for(unsigned i=step*8;i<(step+1)*8;++i) {
    const uint32_t offset=i*UniformStride;
    pass.SetPipeline(pipelines[i/8]);pass.SetBindGroup(1,uniformBind,1,&offset);
    pass.SetViewport(0,float(i),1,1,0,1);pass.SetScissorRect(0,i,1,1);pass.Draw(3);
  }
  pass.End();
  std::fprintf(stderr,"GPU_TEV_GENERATED_PIPELINE variant=%u stages=%u\n",step,configs[step].tevStageCount);
  if(++step<16)return;
  started=true;
  const wgpu::TexelCopyTextureInfo textureSource{.texture=job->target};
  const wgpu::TexelCopyBufferInfo destination{.layout={.bytesPerRow=256,.rowsPerImage=Cases},.buffer=job->readback};
  const wgpu::Extent3D extent{1,Cases,1};cmd.CopyTextureToBuffer(&textureSource,&destination,&extent);
  pending=job;
  building.reset();
  frame.afterSubmitCallbacks.emplace_back([job]{job->readback.MapAsync(wgpu::MapMode::Read,0,ReadbackSize,wgpu::CallbackMode::AllowSpontaneous,[job](wgpu::MapAsyncStatus status,wgpu::StringView){job->status.store(status==wgpu::MapAsyncStatus::Success?1:-1,std::memory_order_release);});});
  std::fprintf(stderr,"GPU_TEV_GENERATED_QUEUED cases=128 pipelines=16 compared_bytes=512\n");
}
}
