#include <SDL3/SDL_timer.h>
#include "encoding.hpp"
#include "draw_readiness_diagnostic.hpp"

#include "frame.hpp"

#include "clear.hpp"
#include "depth_peek.hpp"
#include "diagnostic_capture.hpp"
#include "pipeline_cache.hpp"
#include "tex_copy_conv.hpp"
#include "tex_palette_conv.hpp"
#include "../gx/gx.hpp"
#include "../gx/pipeline.hpp"
#ifdef AURORA_ENABLE_RMLUI
#include "../rmlui/pipeline.hpp"
#endif
#include "../webgpu/gpu.hpp"
#include "../webgpu/gpu_prof.hpp"
#include "staged_texture_diagnostic.hpp"
#include "storage_diagnostic.hpp"
#include "index_upload_diagnostic.hpp"
#include "uniform_diagnostic.hpp"
#include "vertex_fetch_diagnostic.hpp"
#include "copy_clear_diagnostic.hpp"
#include "z_texture_diagnostic.hpp"
#include "tev_regular_diagnostic.hpp"
#include "tev_compare_diagnostic.hpp"
#include "copy_quantize4_diagnostic.hpp"
#include "r4_copy_diagnostic.hpp"
#include "tev_generated_diagnostic.hpp"
#include "texcoord_projection_diagnostic.hpp"
#include "vertex_replay_diagnostic.hpp"
#include "original_mesh_replay.hpp"
#include "original_texture_replay.hpp"
#include "original_material_replay.hpp"
#include "raster_replay_diagnostic.hpp"
#include "depth_texture_diagnostic.hpp"

#include <algorithm>
#include <atomic>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <tracy/Tracy.hpp>

namespace aurora::gfx {
std::atomic<uint64_t> diagnosticReadyPasses{0}, diagnosticUnavailablePasses{0};
// Render-worker state, valid only between known GX draws in one pass/pipeline.
static bool textureBindingValid = false;
static BindGroupRef currentTextureBinding{};

using namespace detail;
using webgpu::g_device;
using webgpu::g_queue;

namespace {
constexpr Module Log{"aurora::gfx"};
PipelineRef g_currentPipeline;
bool g_passPipelineUnavailable = false;

void apply_viewport(const wgpu::RenderPassEncoder& pass, const Viewport& vp) {
  const float minDepth = gx::UseReversedZ ? 1.f - vp.zfar : vp.znear;
  const float maxDepth = gx::UseReversedZ ? 1.f - vp.znear : vp.zfar;
  pass.SetViewport(vp.left, vp.top, vp.width, vp.height, minDepth, maxDepth);
}

void apply_scissor(const wgpu::RenderPassEncoder& pass, const ClipRect& sc, const wgpu::Extent3D& size) {
  const auto x = std::clamp(static_cast<uint32_t>(sc.x), 0u, size.width);
  const auto y = std::clamp(static_cast<uint32_t>(sc.y), 0u, size.height);
  const auto w = std::clamp(static_cast<uint32_t>(sc.width), 0u, size.width - x);
  const auto h = std::clamp(static_cast<uint32_t>(sc.height), 0u, size.height - y);
  pass.SetScissorRect(x, y, w, h);
}

DrawContext make_draw_context(const RenderPass& passInfo) {
  auto& res = resources();
  return {
      .device = g_device,
      .queue = g_queue,
      .vertexBuffer = res.vertexBuffer,
      .indexBuffer = res.indexBuffer,
      .uniformBuffer = res.uniformBuffer,
      .storageBuffer = res.storageBuffer,
      .colorFormat = webgpu::g_graphicsConfig.surfaceConfiguration.format,
      .depthFormat = webgpu::g_graphicsConfig.depthFormat,
      .sampleCount = passInfo.msaaSamples,
      .targetWidth = passInfo.targetSize.width,
      .targetHeight = passInfo.targetSize.height,
  };
}

void render_custom_draw(const CustomDrawCommand& draw, const wgpu::RenderPassEncoder& pass,
                        const RenderPass& passInfo) {
  const auto drawType = find_runtime_draw_type(draw.type);
  if (!drawType) {
    // Unregistered between record and replay; the command is a no-op.
    return;
  }

  const auto context = make_draw_context(passInfo);
  drawType->draw(context, pass, draw.payload.data(), draw.payloadSize, drawType->userdata);
}

void execute_encoder_task(wgpu::CommandEncoder& cmd, FramePacket& frame, const EncoderTask& task) {
  const auto taskType = find_runtime_encoder_task_type(task.type);
  if (!taskType) {
    // Unregistered between record and encode; the task is a no-op.
    return;
  }

  auto& res = resources();
  const EncoderTaskContext context{
      .device = g_device,
      .queue = g_queue,
      .vertexBuffer = res.vertexBuffer,
      .indexBuffer = res.indexBuffer,
      .uniformBuffer = res.uniformBuffer,
      .storageBuffer = res.storageBuffer,
  };
  taskType->callback(context, cmd, task.payload.data(), task.payloadSize, taskType->userdata);
  if (taskType->afterSubmit != nullptr) {
    const auto payload = task.payload;
    const auto payloadSize = task.payloadSize;
    const auto callback = taskType->afterSubmit;
    const auto userdata = taskType->userdata;
    frame.afterSubmitCallbacks.emplace_back([payload, payloadSize, callback, userdata] {
      const EncoderTaskCompletionContext completionContext{
          .device = g_device,
          .queue = g_queue,
      };
      callback(completionContext, payload.data(), payloadSize, userdata);
    });
  }
}

void render_pass(const wgpu::RenderPassEncoder& pass, FramePacket& frame, RenderPass& passInfo) {
  ZoneScoped;
  g_currentPipeline = UINTPTR_MAX;
  textureBindingValid = false;
  g_passPipelineUnavailable = false;
#ifdef AURORA_GFX_DEBUG_GROUPS
  std::vector<std::string> lastDebugGroupStack;
#endif
  Viewport currentViewport{};
  ClipRect currentScissor{};
  bool hasViewport = false;
  bool hasScissor = false;

  // Bind bind group for the whole pass
  pass.SetBindGroup(0, resources().staticBindGroup);
  pass.SetBindGroup(2, gx::g_emptyTextureBindGroup);

  decltype(DrawCommand::encoder) previousDrawEncoder = nullptr;
  for (auto& cmd : passInfo.commands) {
#ifdef AURORA_GFX_DEBUG_GROUPS
    {
      size_t firstDiff = lastDebugGroupStack.size();
      for (size_t i = 0; i < lastDebugGroupStack.size(); ++i) {
        if (i >= cmd.debugGroupStack.size() || cmd.debugGroupStack[i] != lastDebugGroupStack[i]) {
          firstDiff = i;
          break;
        }
      }
      for (size_t i = firstDiff; i < lastDebugGroupStack.size(); ++i) {
        pass.PopDebugGroup();
      }
      for (size_t i = firstDiff; i < cmd.debugGroupStack.size(); ++i) {
        pass.PushDebugGroup(cmd.debugGroupStack[i].c_str());
      }
      lastDebugGroupStack = cmd.debugGroupStack;
    }
#endif
    switch (cmd.type) {
    case CommandType::SetViewport: {
      const auto& vp = cmd.data.setViewport;
      apply_viewport(pass, vp);
      currentViewport = vp;
      hasViewport = true;
    } break;
    case CommandType::SetScissor: {
      const auto& sc = cmd.data.setScissor;
      apply_scissor(pass, sc, passInfo.targetSize);
      currentScissor = sc;
      hasScissor = true;
    } break;
    case CommandType::Draw: {
      auto& draw = cmd.data.draw;
      if (draw.encoder != previousDrawEncoder) textureBindingValid = false;
      previousDrawEncoder = draw.encoder;
      if (draw.encoder != nullptr) {
        draw.encoder(draw.payload.data(), pass, passInfo);
      }
    } break;
    case CommandType::CustomDraw: {
      render_custom_draw(cmd.data.customDraw, pass, passInfo);
      g_currentPipeline = UINTPTR_MAX;
  textureBindingValid = false;
      pass.SetBindGroup(0, resources().staticBindGroup);
      pass.SetBindGroup(2, gx::g_emptyTextureBindGroup);
      if (hasViewport) {
        apply_viewport(pass, currentViewport);
      }
      if (hasScissor) {
        apply_scissor(pass, currentScissor, passInfo.targetSize);
      }
    } break;
    case CommandType::DebugMarker: {
#if defined(AURORA_GFX_DEBUG_GROUPS)
      pass.InsertDebugMarker(wgpu::StringView(frame.debugMarkers[cmd.data.debugMarkerIndex]));
#endif
    } break;
    }
  }

#ifdef AURORA_GFX_DEBUG_GROUPS
  for (size_t i = 0; i < lastDebugGroupStack.size(); ++i) {
    pass.PopDebugGroup();
  }
#endif
}

void render(wgpu::CommandEncoder& cmd, FramePacket& frame, RenderPass& passInfo, uint32_t passIndex) {
  ZoneScoped;
  if (!passInfo.sealed) {
    return;
  }

  const bool tracePass = std::getenv("AURORA_DIAGNOSTIC_WORKER_ITEMS") != nullptr;
  const uint64_t passStartNs = tracePass ? SDL_GetTicksNS() : 0;
  uint64_t readbackAllocationNs = 0;
  for (const auto& conv : passInfo.paletteConvs) {
    tex_palette_conv::run(cmd, conv);
  }
  vertex_replay_diagnostic::encode(cmd,frame,passInfo);
  if (passInfo.discardable) {
    // This pass has no effect and can be safely discarded (e.g. an empty EFB segment between two back-to-back pass
    // breaks, or an unresolved offscreen pass).
    return;
  }

  const std::array attachments{
      wgpu::RenderPassColorAttachment{
          .view = passInfo.colorView,
          .resolveTarget = passInfo.resolveView,
          .loadOp = passInfo.colorLoadOp != wgpu::LoadOp::Undefined
                        ? passInfo.colorLoadOp
                        : (passInfo.clearColor ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load),
          .storeOp = passInfo.colorStoreOp,
          .clearValue =
              {
                  .r = passInfo.clearColorValue.x(),
                  .g = passInfo.clearColorValue.y(),
                  .b = passInfo.clearColorValue.z(),
                  .a = passInfo.clearColorValue.w(),
              },
      },
  };
  wgpu::RenderPassDepthStencilAttachment depthStencilAttachment{};
  const wgpu::RenderPassDepthStencilAttachment* depthStencilAttachmentPtr = nullptr;
  if (passInfo.depthStencilView) {
    depthStencilAttachment = {
        .view = passInfo.depthStencilView,
        .depthLoadOp = passInfo.hasDepth ? (passInfo.depthLoadOp != wgpu::LoadOp::Undefined
                                                ? passInfo.depthLoadOp
                                                : (passInfo.clearDepth ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load))
                                         : wgpu::LoadOp::Undefined,
        .depthStoreOp = passInfo.hasDepth ? passInfo.depthStoreOp : wgpu::StoreOp::Undefined,
        .depthClearValue = passInfo.clearDepthValue,
        .stencilLoadOp = passInfo.hasStencil ? passInfo.stencilLoadOp : wgpu::LoadOp::Undefined,
        .stencilStoreOp = passInfo.hasStencil ? passInfo.stencilStoreOp : wgpu::StoreOp::Undefined,
        .stencilClearValue = passInfo.stencilClearValue,
    };
    depthStencilAttachmentPtr = &depthStencilAttachment;
  }
  const auto label = passInfo.label.empty() ? fmt::format("Render pass {}", passIndex)
                                            : fmt::format("{} {}", passInfo.label, passIndex);
  const wgpu::RenderPassDescriptor renderPassDescriptor{
      .label = label.c_str(),
      .colorAttachmentCount = attachments.size(),
      .colorAttachments = attachments.data(),
      .depthStencilAttachment = depthStencilAttachmentPtr,
      .timestampWrites = webgpu::gpu_prof::pass_writes(label),
  };

  auto pass = cmd.BeginRenderPass(&renderPassDescriptor);
  const uint64_t drawsStartNs = tracePass ? SDL_GetTicksNS() : 0;
  render_pass(pass, frame, passInfo);
  const uint64_t drawsEndNs = tracePass ? SDL_GetTicksNS() : 0;
  pass.End();
  if(!g_passPipelineUnavailable)original_mesh_replay::encode(cmd,frame,passInfo);
  if(!g_passPipelineUnavailable){
    original_texture::encode(cmd,frame,passInfo);
    original_material::encode(cmd,frame,passInfo);
  }
  else if(original_texture::enabled())for(const auto& s:passInfo.originalTextureDraws)
    std::fprintf(stderr,"ORIGINAL_TEXTURE_SKIPPED source=%llu sequence=%llu reason=production-pipeline-unavailable\n",(unsigned long long)s.source->id,(unsigned long long)s.sequence);
  if(passInfo.hasDraws) {
    if(g_passPipelineUnavailable) diagnosticUnavailablePasses.fetch_add(1);
    else diagnosticReadyPasses.fetch_add(1);
  }

  if (!std::getenv("AURORA_DIAGNOSTIC_CAPTURE_PRESENT_SOURCE") && !std::getenv("AURORA_DIAGNOSTIC_CAPTURE_COPY_TEXTURE") && passInfo.captureDepthSnapshot && passInfo.hasDraws && !g_passPipelineUnavailable) diagnostic_capture::encode(cmd, passInfo.copySourceTexture, frame.frameId);
  if (passInfo.captureDepthSnapshot) {
    depth_peek::encode_frame_snapshot(cmd, passInfo.copySourceDepthView, passInfo.targetSize, passInfo.msaaSamples);
  }

  if (passInfo.resolveTarget) {
    const auto& dstSize = passInfo.resolveTarget->size;
    const bool needsConversion = tex_copy_conv::needs_conversion(passInfo.resolveFormat);
    const bool needsScaling = dstSize.width != static_cast<uint32_t>(passInfo.resolveRect.width) ||
                              dstSize.height != static_cast<uint32_t>(passInfo.resolveRect.height);
    const bool isDepth = gx::is_depth_format(passInfo.resolveFormat);
    if (isDepth && passInfo.msaaSamples > 1) {
      Log.fatal("Depth tex copies from multisampled EFB targets are not supported");
    }
    const tex_copy_conv::ConvRequest convReq{
        .fmt = passInfo.resolveFormat,
        .srcView = isDepth ? passInfo.copySourceDepthView : passInfo.copySourceView,
        .uniformRange = passInfo.resolveUniformRange,
        .dst = passInfo.resolveTarget,
        .sampleFilter = needsScaling ? tex_copy_conv::SampleFilter::Linear : tex_copy_conv::SampleFilter::Nearest,
        .displayCopy = passInfo.displayCopy.enabled,
    };
    if (passInfo.displayCopy.enabled) {
      tex_copy_conv::display_copy(cmd, convReq);
    } else if (needsConversion) {
      tex_copy_conv::run(cmd, convReq);
    } else if (needsScaling) {
      tex_copy_conv::blit(cmd, convReq);
    } else {
      const webgpu::gpu_prof::Zone zone{cmd, "EFB copy"};
      const wgpu::TexelCopyTextureInfo src{
          .texture = passInfo.copySourceTexture,
          .origin =
              wgpu::Origin3D{
                  .x = static_cast<uint32_t>(passInfo.resolveRect.x),
                  .y = static_cast<uint32_t>(passInfo.resolveRect.y),
              },
      };
      const wgpu::TexelCopyTextureInfo dst{
          .texture = passInfo.resolveTarget->texture,
      };
      const wgpu::Extent3D size{
          .width = static_cast<uint32_t>(passInfo.resolveRect.width),
          .height = static_cast<uint32_t>(passInfo.resolveRect.height),
          .depthOrArrayLayers = 1,
      };
      cmd.CopyTextureToTexture(&src, &dst, &size);
    }
    if(passInfo.xfbCopy) {
      auto job=passInfo.xfbCopy;
      const auto& readbackTarget = passInfo.xfbReadbackTarget ? passInfo.xfbReadbackTarget : passInfo.resolveTarget;
      if (passInfo.xfbReadbackTarget) {
        tex_copy_conv::blit(cmd, tex_copy_conv::ConvRequest{
          .fmt = GX_TF_RGBA8, .srcView = passInfo.resolveTarget->sampleTextureView,
          .uniformRange = passInfo.xfbResampleUniformRange, .dst = readbackTarget,
          .sampleFilter = tex_copy_conv::SampleFilter::Linear});
      }
      const uint32_t width=readbackTarget->size.width,height=readbackTarget->size.height,pitch=(width*4+255)&~255u;
      const uint64_t bytes=uint64_t(pitch)*height;
      AURORA_ASSERT(job->rgba.size()==size_t(width)*height*4 && job->width==width && job->height==height && passInfo.displayCopy.enabled,
                    "Invalid enrolled XFB copy extent");
      const wgpu::BufferDescriptor descriptor{.label="Native XFB readback",
        .usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=bytes};
      const uint64_t allocationStartNs = tracePass ? SDL_GetTicksNS() : 0;
      const auto readback=webgpu::g_device.CreateBuffer(&descriptor);
      if(tracePass)readbackAllocationNs+=SDL_GetTicksNS()-allocationStartNs;
      const wgpu::TexelCopyTextureInfo source{.texture=readbackTarget->texture};
      const wgpu::TexelCopyBufferInfo destination{
        .layout={.offset=0,.bytesPerRow=pitch,.rowsPerImage=height},.buffer=readback};
      const wgpu::Extent3D extent{width,height,1};
      cmd.CopyTextureToBuffer(&source,&destination,&extent);
      const bool bgra=readbackTarget->format==wgpu::TextureFormat::BGRA8Unorm;
      frame.afterSubmitCallbacks.emplace_back([job,readback,width,height,pitch,bytes,bgra]{
        job->releaseReadback=[readback]{readback.Unmap();};
        readback.MapAsync(wgpu::MapMode::Read,0,bytes,wgpu::CallbackMode::AllowSpontaneous,
          [job,readback,width,height,pitch,bgra](wgpu::MapAsyncStatus status,wgpu::StringView){
            if(status!=wgpu::MapAsyncStatus::Success){job->failed.store(true,std::memory_order_release);return;}
            const auto* pixels=static_cast<const uint8_t*>(readback.GetConstMappedRange());
            if(!pixels){job->failed.store(true,std::memory_order_release);return;}
            for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x){
              const auto* source=pixels+size_t(y)*pitch+x*4;
              auto* destination=job->rgba.data()+(size_t(y)*width+x)*4;
              destination[0]=source[bgra?2:0];destination[1]=source[1];
              destination[2]=source[bgra?0:2];destination[3]=source[3];
            }
            job->mapped.store(true,std::memory_order_release);
          });
      });
    }
    if (!std::getenv("AURORA_DIAGNOSTIC_CAPTURE_PRESENT_SOURCE") && std::getenv("AURORA_DIAGNOSTIC_CAPTURE_COPY_TEXTURE") &&
        passInfo.hasDraws && !g_passPipelineUnavailable) {
      diagnostic_capture::encode(cmd, passInfo.resolveTarget->texture, frame.frameId);
    }
  }

  if (passInfo.snapshotColorDst) {
    const webgpu::gpu_prof::Zone zone{cmd, "Pass snapshot"};
    const wgpu::TexelCopyTextureInfo src{
        .texture = passInfo.copySourceTexture,
    };
    const wgpu::TexelCopyTextureInfo dst{
        .texture = passInfo.snapshotColorDst,
    };
    const wgpu::Extent3D size{
        .width = passInfo.targetSize.width,
        .height = passInfo.targetSize.height,
        .depthOrArrayLayers = 1,
    };
    cmd.CopyTextureToTexture(&src, &dst, &size);
  }
  if (passInfo.snapshotDepthDst) {
    tex_copy_conv::snapshot_depth(cmd, passInfo.copySourceDepthView, passInfo.msaaSamples, passInfo.snapshotDepthDst);
  }
  if(tracePass){
    static unsigned records=0;
    if(records<65536)std::fprintf(stderr,"NATIVE_RENDER_COST record=%u frame=%llu pass=%u total_ns=%llu draw_ns=%llu setup_ns=%llu readback_alloc_ns=%llu commands=%zu\n",
      ++records,(unsigned long long)frame.frameId,passIndex,(unsigned long long)(SDL_GetTicksNS()-passStartNs),
      (unsigned long long)(drawsEndNs-drawsStartNs),(unsigned long long)(drawsStartNs-passStartNs),
      (unsigned long long)readbackAllocationNs,passInfo.commands.size());
  }

}

constexpr uint64_t VertexStagingOffset = 0;
constexpr uint64_t UniformStagingOffset = VertexStagingOffset + VertexBufferSize;
constexpr uint64_t IndexStagingOffset = UniformStagingOffset + UniformBufferSize;
constexpr uint64_t StorageStagingOffset = IndexStagingOffset + IndexBufferSize;
constexpr uint64_t TextureUploadStagingOffset = StorageStagingOffset + StorageBufferSize;

constexpr uint32_t align_down_copy_offset(uint32_t value) noexcept { return value & ~3u; }

void copy_staging_buffer_range(wgpu::CommandEncoder& cmd, const FramePacket& frame, uint32_t& copied,
                               uint32_t highWater, uint64_t stagingOffset, const wgpu::Buffer& dst) {
  if (highWater <= copied) {
    return;
  }
  const uint32_t copyStart = align_down_copy_offset(copied);
  const uint32_t copyEnd = AURORA_ALIGN(highWater, 4);
  cmd.CopyBufferToBuffer(staging_buffer(frame.stagingBuffer), stagingOffset + copyStart, dst, copyStart,
                         copyEnd - copyStart);
  copied = highWater;
}

bool needs_staging_copy(const FramePacket& frame, const FrameOp& op) {
  const auto& highWater = op.highWater;
  if (highWater.verts > frame.copied.verts || highWater.uniforms > frame.copied.uniforms ||
      highWater.indices > frame.copied.indices || highWater.storage > frame.copied.storage) {
    return true;
  }
  if constexpr (UseTextureBuffer) {
    return op.textureUploads.size() > frame.copied.textureUploadCount;
  }
  return false;
}

void copy_staging_to_high_water(wgpu::CommandEncoder& cmd, FramePacket& frame, const FrameOp& op) {
  if (!needs_staging_copy(frame, op)) {
    return;
  }
  const webgpu::gpu_prof::Zone zone{cmd, "Staging copies"};
  const auto& highWater = op.highWater;
  auto& res = resources();
  vertex_fetch_diagnostic::encode(cmd, frame);
  copy_clear_diagnostic::encode(cmd, frame);
  z_texture_diagnostic::encode(cmd, frame);
  tev_regular_diagnostic::encode(cmd, frame);
  tev_compare_diagnostic::encode(cmd, frame);
  copy_quantize4_diagnostic::encode(cmd, frame);
  r4_copy_diagnostic::encode(cmd, frame);
  tev_generated_diagnostic::encode(cmd, frame);
  texcoord_projection_diagnostic::encode(cmd, frame);
  indexed_position_diagnostic::compile();
  vertex_replay_diagnostic::compile();
  raster_replay_diagnostic::compile();
  depth_texture_diagnostic::encode(cmd, frame);
  copy_staging_buffer_range(cmd, frame, frame.copied.verts, highWater.verts, VertexStagingOffset, res.vertexBuffer);
  const auto uniformStart = frame.copied.uniforms;
  copy_staging_buffer_range(cmd, frame, frame.copied.uniforms, highWater.uniforms, UniformStagingOffset,
                            res.uniformBuffer);
  uniform_diagnostic::encode(cmd, frame, res.uniformBuffer, AURORA_ALIGN(uniformStart, 4),
                            highWater.uniforms & ~3u, op.diagnosticUniforms);
  const auto indexStart=frame.copied.indices;
  copy_staging_buffer_range(cmd, frame, frame.copied.indices, highWater.indices, IndexStagingOffset, res.indexBuffer);
  index_upload_diagnostic::encode(cmd,frame,res.indexBuffer,AURORA_ALIGN(indexStart,4),
                                  highWater.indices&~3u,op.diagnosticIndices);
  const auto storageStart = frame.copied.storage;
  copy_staging_buffer_range(cmd, frame, frame.copied.storage, highWater.storage, StorageStagingOffset,
                            res.storageBuffer);
  storage_diagnostic::encode(cmd, frame, res.storageBuffer, AURORA_ALIGN(storageStart, 4),
                            highWater.storage & ~3u, op.diagnosticStorage);

  if constexpr (UseTextureBuffer) {
    for (size_t i = frame.copied.textureUploadCount; i < op.textureUploads.size(); ++i) {
      const auto& item = *op.textureUploads[i];
      const wgpu::TexelCopyBufferInfo buf{
          .layout =
              wgpu::TexelCopyBufferLayout{
                  .offset = item.buffer ? item.layout.offset : item.layout.offset + TextureUploadStagingOffset,
                  .bytesPerRow = AURORA_ALIGN(item.layout.bytesPerRow, 256),
                  .rowsPerImage = item.layout.rowsPerImage,
              },
          .buffer = item.buffer ? item.buffer : staging_buffer(frame.stagingBuffer),
      };
      cmd.CopyBufferToTexture(&buf, &item.tex, &item.size);
      staged_texture_diagnostic::encode(cmd, frame, item);
    }
    frame.copied.textureUpload = highWater.textureUpload;
    frame.copied.textureUploadCount = op.textureUploads.size();
  }
}
} // namespace

namespace detail {
void encode_op(wgpu::CommandEncoder& cmd, FramePacket& frame, const FrameOp& op) {
  draw_readiness_diagnostic::begin(frame.frameId);
  copy_staging_to_high_water(cmd, frame, op);
  switch (op.type) {
  case FrameOpType::RenderPass:
    if (op.renderPass != nullptr) {
      render(cmd, frame, *op.renderPass, op.index);
    }
    break;
  case FrameOpType::TextureCopy:
    if (op.textureCopy != nullptr) {
      const webgpu::gpu_prof::Zone zone{cmd, "Texture copy"};
      cmd.CopyTextureToTexture(&op.textureCopy->src, &op.textureCopy->dst, &op.textureCopy->size);
    }
    break;
  case FrameOpType::EncoderTask:
    if (op.encoderTask != nullptr) {
      execute_encoder_task(cmd, frame, *op.encoderTask);
    }
    break;
  }
}
} // namespace detail

void bind_texture_group(BindGroupRef ref, const wgpu::RenderPassEncoder& pass) {
  static const bool reuse = std::getenv("AURORA_DIAGNOSTIC_REUSE_TEXTURE_BINDINGS") != nullptr;
  static uint64_t calls=0, skipped=0;
  if(reuse && textureBindingValid && currentTextureBinding==ref) {
    ++skipped;
  } else {
    pass.SetBindGroup(2, find_bind_group(ref));
    currentTextureBinding=ref;
    textureBindingValid=true;
  }
  if(reuse && ++calls%512==0 && calls<=1048576)
    std::fprintf(stderr,"NATIVE_TEXTURE_BIND_REUSE calls=%llu skipped=%llu\n",
      (unsigned long long)calls,(unsigned long long)skipped);
}

bool bind_pipeline(PipelineRef ref, const wgpu::RenderPassEncoder& pass) {
  if (ref == g_currentPipeline) {
    return true;
  }
  textureBindingValid = false;
  wgpu::RenderPipeline pipeline;
  if (!get_pipeline(ref, pipeline)) {
    g_passPipelineUnavailable = true;
    return false;
  }
  pass.SetPipeline(pipeline);
  g_currentPipeline = ref;
  return true;
}
} // namespace aurora::gfx



extern "C" void aurora_diagnostic_draw_pass_counts(uint64_t* ready, uint64_t* unavailable) {
  if(ready) *ready = aurora::gfx::diagnosticReadyPasses.load();
  if(unavailable) *unavailable = aurora::gfx::diagnosticUnavailablePasses.load();
}
