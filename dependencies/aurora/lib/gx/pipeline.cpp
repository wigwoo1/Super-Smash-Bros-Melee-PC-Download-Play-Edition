#include "pipeline.hpp"
#include "../gfx/draw_readiness_diagnostic.hpp"

#include "../gfx/encoding.hpp"
#include "../gfx/resources.hpp"
#include "../gfx/pipeline_cache.hpp"
#include "../gfx/resource_cache.hpp"

#include "gx_fmt.hpp"
#include "shader_info.hpp"

#include <tracy/Tracy.hpp>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <set>
#include <tuple>
#include <map>

namespace aurora::gx {
namespace {
constexpr Module Log{"aurora::gx::pipeline"};
struct DestinationAlphaPipelines {wgpu::RenderPipeline color,alpha;};
std::mutex destinationAlphaMutex;
std::map<gfx::PipelineRef,DestinationAlphaPipelines> destinationAlphaPipelines;
DestinationAlphaPipelines destination_alpha_pair(gfx::PipelineRef ref){
  const std::lock_guard lock{destinationAlphaMutex};
  const auto found=destinationAlphaPipelines.find(ref);return found==destinationAlphaPipelines.end()?DestinationAlphaPipelines{}:found->second;
}
}
void clear_destination_alpha_pipelines(){const std::lock_guard lock{destinationAlphaMutex};destinationAlphaPipelines.clear();}

wgpu::RenderPipeline create_pipeline(const PipelineConfig& config) {
  return create_pipeline_with_context(config,capture_pipeline_context());
}
wgpu::RenderPipeline create_pipeline_with_context(const PipelineConfig& config,const PipelineBuildContext& context) {
  ZoneScoped;
  const bool timing=std::getenv("AURORA_DIAGNOSTIC_PIPELINE_TIMING") != nullptr;
  const auto start=std::chrono::steady_clock::now();
  // Serialize WGSL/module preparation, including debug sets and capture files.
  static std::mutex shaderPreparation;
  const bool overrideAlpha=config.alphaUpdate&&config.dstAlpha!=UINT32_MAX;
  auto colorConfig=config;
  if(overrideAlpha){colorConfig.alphaUpdate=false;colorConfig.depthUpdate=false;colorConfig.dstAlpha=UINT32_MAX;}
  wgpu::ShaderModule shader;
  { std::lock_guard lock{shaderPreparation}; shader=build_shader(colorConfig.shaderConfig); }
  const auto moduleReady=std::chrono::steady_clock::now();
  if(gfx::draw_readiness_diagnostic::enabled())
    std::fprintf(stderr,"DRAW_READINESS_COMPILE pipeline=%llu shader=%llu\n",static_cast<unsigned long long>(xxh3_hash(config,static_cast<HashType>(gfx::ShaderType::GX))),static_cast<unsigned long long>(xxh3_hash(config.shaderConfig)));
  const auto label =
      fmt::format("GX Pipeline {:x} shader {:x}", xxh3_hash(config, static_cast<HashType>(gfx::ShaderType::GX)),
                  xxh3_hash(config.shaderConfig));
  auto pipeline=build_pipeline_with_context(colorConfig, {}, shader, label.c_str(),context);
  if(overrideAlpha){
    auto alphaConfig=config;alphaConfig.colorUpdate=false;alphaConfig.blendMode=GX_BM_NONE;
    alphaConfig.blendFacSrc=GX_BL_ONE;alphaConfig.blendFacDst=GX_BL_ZERO;
    alphaConfig.shaderConfig.forceOpaqueAlpha=true;
    wgpu::ShaderModule alphaShader;
    {std::lock_guard lock{shaderPreparation};alphaShader=build_shader(alphaConfig.shaderConfig);}
    auto alpha=build_pipeline_with_context(alphaConfig,{},alphaShader,"GX destination-alpha coverage",context);
    AURORA_ASSERT(bool(pipeline)&&bool(alpha),"Destination-alpha pipeline pair unavailable");
    const std::lock_guard lock{destinationAlphaMutex};
    destinationAlphaPipelines.insert_or_assign(xxh3_hash(config,static_cast<HashType>(gfx::ShaderType::GX)),DestinationAlphaPipelines{pipeline,alpha});
  }
  if(timing) {
    static std::atomic<unsigned> samples{0};
    const auto sample=samples.fetch_add(1);
    if(sample<1024) std::fprintf(stderr,"NATIVE_PIPELINE_CREATE sample=%u pipeline=%llu shader_module_ms=%.3f render_pipeline_ms=%.3f\n",sample+1,static_cast<unsigned long long>(xxh3_hash(config,static_cast<HashType>(gfx::ShaderType::GX))),std::chrono::duration<double,std::milli>(moduleReady-start).count(),std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-moduleReady).count());
  }
  return pipeline;
}

void render(const DrawData& data, const wgpu::RenderPassEncoder& pass) {
  const bool ready=gfx::bind_pipeline(data.pipeline, pass);
  gfx::draw_readiness_diagnostic::record(data,ready);
  if (!ready) {
    return;
  }

  const auto& resources = gfx::detail::resources();
  pass.SetImmediates(0, &data.immediateData, sizeof(data.immediateData));
  const std::array offsets{data.uniformRange.offset};
  pass.SetBindGroup(1, resources.uniformBindGroup, offsets.size(), offsets.data());
  if (std::getenv("AURORA_DIAGNOSTIC_UNIFORM_UPLOAD") &&
      std::getenv("AURORA_DIAGNOSTIC_TEXTURE_MATRICES")) {
    // Encoding owner only. Observe the offset passed to the actual bind call.
    static std::set<uint64_t> seen;
    static bool limitReported = false;
    const auto key = data.diagnosticUniformShader;
    if (seen.size() < 2048 && seen.insert(key).second) {
      std::fprintf(stderr,
          "DRAW_UNIFORM_BOUND shader=%llx pipeline=%llu offset=%u bytes=%u binding_bytes=%u alignment=%u\n",
          static_cast<unsigned long long>(data.diagnosticUniformShader),
          static_cast<unsigned long long>(data.pipeline), offsets[0], data.uniformRange.size,
          MaxUniformSize, resources.limits.minUniformBufferOffsetAlignment);
    } else if (seen.size() >= 2048 && !limitReported) {
      std::fprintf(stderr, "DRAW_UNIFORM_BOUND_LIMIT records=2048\n");
      limitReported = true;
    }
  }
  if (data.bindGroups.textureBindGroup) {
    gfx::bind_texture_group(data.bindGroups.textureBindGroup, pass);
  }
  pass.SetIndexBuffer(resources.indexBuffer, wgpu::IndexFormat::Uint16, data.idxRange.offset, data.idxRange.size);
  if (data.dstAlpha != UINT32_MAX) {
    const wgpu::Color color{0.f, 0.f, 0.f, data.dstAlpha / 255.f};
    pass.SetBlendConstant(&color);
    const auto pair=destination_alpha_pair(data.pipeline);
    AURORA_ASSERT(bool(pair.color)&&bool(pair.alpha),"Missing destination-alpha pipeline pair");
    const auto count=data.indexCount?data.indexCount:data.vtxCount;
    AURORA_ASSERT(count%3==0,"Destination-alpha draw is not a triangle list");
    // Preserve primitive ordering:later overlapping triangles/instances must
    // see the alpha written by earlier ones. The color pass leaves depth
    // unchanged;the identical coverage/discard pass performs the original
    // depth test/write,so LESS draws do not reject their own alpha update.
    for(uint32_t instance=0;instance<data.instanceCount;instance++)for(uint32_t first=0;first<count;first+=3){
      pass.SetPipeline(pair.color);
      if(data.indexCount)pass.DrawIndexed(3,1,first,0,instance);else pass.Draw(3,1,first,instance);
      pass.SetPipeline(pair.alpha);
      if(data.indexCount)pass.DrawIndexed(3,1,first,0,instance);else pass.Draw(3,1,first,instance);
    }
    // gfx::bind_pipeline caches the primary identity. Restore it even for a
    // following normal draw that reuses that identity and skips SetPipeline.
    pass.SetPipeline(pair.color);
    static std::atomic<unsigned> reported{0};
    if(std::getenv("AURORA_DIAGNOSTIC_DST_ALPHA")&&reported.fetch_add(1)<16)
      std::fprintf(stderr,"DESTINATION_ALPHA_PAIRED triangles=%u instances=%u alpha=%u indexed=%u\n",count/3,data.instanceCount,data.dstAlpha,unsigned(data.indexCount!=0));
    return;
  }
  if (data.indexCount == 0) {
    pass.Draw(data.vtxCount, data.instanceCount);
  } else {
    pass.DrawIndexed(data.indexCount, data.instanceCount);
  }
}

} // namespace aurora::gx
