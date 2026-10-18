#include "recording.hpp"
#include <aurora/diagnostics.h>
#include "corneria_position_profiles.hpp"
#include "mutecity_position_profiles.hpp"
#include "original_mesh_lighting_reference.hpp"
#include "original_mesh_light_uniforms.hpp"
#include "original_mesh_texgen_reference.hpp"
#include "../gx/shader_info.hpp"

#include "encoding.hpp"
#include "frame.hpp"
#include "resource_cache.hpp"

#include "clear.hpp"
#include "pipeline_cache.hpp"
#include "preparation_progress.hpp"
#include "render_worker.hpp"
#include "tex_copy_conv.hpp"
#include "tex_palette_conv.hpp"
#include "texture.hpp"
#include "uniform_range_cache.hpp"
#include "../gx/fifo.hpp"
#include "../gx/gx.hpp"
#include "../gx/pipeline.hpp"
#include "../recording_diagnostics.hpp"
#ifdef AURORA_ENABLE_RMLUI
#include "../rmlui/pipeline.hpp"
#endif
#include "../window.hpp"

#include <array>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <magic_enum.hpp>
#include <tracy/Tracy.hpp>

namespace aurora::gfx {
using namespace detail;

namespace {
constexpr Module Log{"aurora::gfx"};

struct FrameRecorder {
  FramePacket* packet = nullptr;
  graphics_quality::DrawScope graphicsDrawScope;
  TextureHandle diagnosticSnapshot;
  std::vector<PipelineRef> preparingGxPipelines;
  absl::flat_hash_map<uint64_t, std::vector<Range>> uniformRangesByHash;
  size_t frameSlot = 0;
  uint32_t currentRenderPass = UINT32_MAX;
  uint32_t drawCallCount = 0;
  uint32_t mergedDrawCallCount = 0;
  bool inOffscreen = false;
  std::optional<RenderPass> suspendedEfbPass;
  Viewport suspendedEfbViewport;
  ClipRect suspendedEfbScissor;
  webgpu::TextureWithSampler offscreenColor;
  webgpu::TextureWithSampler offscreenDepth;
  Viewport cachedViewport;
  ClipRect cachedScissor;
#ifdef AURORA_GFX_DEBUG_GROUPS
  std::vector<std::string> debugGroupStack;
#endif

  [[nodiscard]] bool active() const noexcept { return packet != nullptr; }

  FramePacket& frame() const {
    CHECK(packet != nullptr, "No active recording session");
    return *packet;
  }

  RenderPassList& passes() const { return frame().renderPasses; }
};

FrameRecorder g_recorder;

bool diagnostic_uniform_dedup_enabled() {
  static const bool enabled = std::getenv("AURORA_DIAGNOSTIC_UNIFORM_DEDUP") != nullptr;
  return enabled;
}

struct UniformDedupProfile {
  uint64_t calls = 0;
  uint64_t hits = 0;
  uint64_t bytesSaved = 0;
} uniformDedupProfile;

void report_uniform_dedup_profile(size_t length, bool hit) {
  ++uniformDedupProfile.calls;
  if (hit) {
    ++uniformDedupProfile.hits;
    uniformDedupProfile.bytesSaved += length;
  }
  if (uniformDedupProfile.calls != 60) return;
  std::fprintf(stderr,
      "NATIVE_UNIFORM_DEDUP_PROFILE calls=%llu hits=%llu bytes_saved=%llu\n",
      static_cast<unsigned long long>(uniformDedupProfile.calls),
      static_cast<unsigned long long>(uniformDedupProfile.hits),
      static_cast<unsigned long long>(uniformDedupProfile.bytesSaved));
  uniformDedupProfile = {};
}

std::string pass_label(std::string_view kind) {
#ifdef AURORA_GFX_DEBUG_GROUPS
  if (!g_recorder.debugGroupStack.empty()) {
    return fmt::format("{} ({})", kind, g_recorder.debugGroupStack.back());
  }
#endif
  return std::string{kind};
}

void set_efb_targets(RenderPass& pass) {
  pass.colorView = webgpu::g_frameBuffer.view;
  pass.resolveView = webgpu::g_graphicsConfig.msaaSamples > 1 ? webgpu::g_frameBufferResolved.view : nullptr;
  pass.depthStencilView = webgpu::g_depthBuffer.view;
  pass.copySourceTexture =
      webgpu::g_graphicsConfig.msaaSamples > 1 ? webgpu::g_frameBufferResolved.texture : webgpu::g_frameBuffer.texture;
  pass.copySourceView =
      webgpu::g_graphicsConfig.msaaSamples > 1 ? webgpu::g_frameBufferResolved.view : webgpu::g_frameBuffer.view;
  pass.copySourceDepthView = webgpu::g_depthBuffer.view;
  pass.targetSize = webgpu::g_frameBuffer.size;
  pass.msaaSamples = webgpu::g_graphicsConfig.msaaSamples;
  pass.hasDepth = true;
  pass.hasStencil = false;
}

struct OffscreenCacheKey {
  uint32_t width;
  uint32_t height;

  bool operator==(const OffscreenCacheKey& rhs) const { return width == rhs.width && height == rhs.height; }
  template <typename H>
  friend H AbslHashValue(H h, const OffscreenCacheKey& key) {
    return H::combine(std::move(h), key.width, key.height);
  }
};
struct OffscreenCacheEntry {
  webgpu::TextureWithSampler color;
  webgpu::TextureWithSampler depth;
};
absl::flat_hash_map<OffscreenCacheKey, OffscreenCacheEntry> g_offscreenCache;

// Pooled destinations for the public resolve_pass API. Entries are recycled
// per frame slot: a slot is only re-acquired after the render worker has
// submitted its previous frame, and queue serialization orders the new frame's
// copies after the old frame's reads.
struct PassSnapshotEntry {
  webgpu::TextureWithSampler color;
  webgpu::TextureWithSampler depth; // R32Float raw depth
};
struct PassSnapshotPool {
  std::vector<PassSnapshotEntry> entries;
  size_t used = 0;
};
std::array<PassSnapshotPool, FrameSlotCount> g_passSnapshotPools;

PassSnapshotEntry& acquire_pass_snapshot(uint32_t width, uint32_t height, bool wantColor, bool wantDepth) {
  auto& pool = g_passSnapshotPools[g_recorder.frameSlot];
  if (pool.used == pool.entries.size()) {
    pool.entries.emplace_back();
  }
  auto& entry = pool.entries[pool.used++];
  const wgpu::Extent3D size{width, height, 1};
  if (wantColor && (!entry.color.texture || entry.color.size.width != width || entry.color.size.height != height ||
                    entry.color.format != webgpu::g_graphicsConfig.surfaceConfiguration.format)) {
    const auto format = webgpu::g_graphicsConfig.surfaceConfiguration.format;
    const wgpu::TextureDescriptor desc{
        .label = "Pass Snapshot Color",
        .usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding,
        .dimension = wgpu::TextureDimension::e2D,
        .size = size,
        .format = format,
        .mipLevelCount = 1,
        .sampleCount = 1,
    };
    auto texture = webgpu::g_device.CreateTexture(&desc);
    auto view = texture.CreateView();
    entry.color = webgpu::TextureWithSampler{
        .texture = std::move(texture),
        .view = std::move(view),
        .size = size,
        .format = format,
    };
  }
  if (wantDepth && (!entry.depth.texture || entry.depth.size.width != width || entry.depth.size.height != height)) {
    const wgpu::TextureDescriptor desc{
        .label = "Pass Snapshot Depth",
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
        .dimension = wgpu::TextureDimension::e2D,
        .size = size,
        .format = wgpu::TextureFormat::R32Float,
        .mipLevelCount = 1,
        .sampleCount = 1,
    };
    auto texture = webgpu::g_device.CreateTexture(&desc);
    auto view = texture.CreateView();
    entry.depth = webgpu::TextureWithSampler{
        .texture = std::move(texture),
        .view = std::move(view),
        .size = size,
        .format = wgpu::TextureFormat::R32Float,
    };
  }
  return entry;
}

FramePacket& current_frame_packet() { return g_recorder.frame(); }

RenderPassList& current_render_passes() { return g_recorder.passes(); }

StagingHighWater current_high_water(const FramePacket& frame) noexcept {
  return {
      .verts = static_cast<uint32_t>(frame.verts.size()),
      .uniforms = static_cast<uint32_t>(frame.uniforms.size()),
      .indices = static_cast<uint32_t>(frame.indices.size()),
      .storage = static_cast<uint32_t>(frame.storage.size()),
      .textureUpload = static_cast<uint32_t>(frame.textureUpload.size()),
      .textureUploadCount = frame.textureUploads.size(),
  };
}

FrameOp capture_frame_op(FramePacket& frame, FrameOpType type, uint32_t index) {
  FrameOp op{
      .type = type,
      .index = index,
      .renderPass =
          type == FrameOpType::RenderPass && index < frame.renderPasses.size() ? &frame.renderPasses[index] : nullptr,
      .textureCopy = type == FrameOpType::TextureCopy && index < frame.textureCopies.size()
                         ? &frame.textureCopies[index]
                         : nullptr,
      .encoderTask =
          type == FrameOpType::EncoderTask && index < frame.encoderTasks.size() ? &frame.encoderTasks[index] : nullptr,
      .highWater = current_high_water(frame),
  };
  op.textureUploads.reserve(op.highWater.textureUploadCount);
  if(std::getenv("AURORA_DIAGNOSTIC_INDEX_UPLOAD") && std::getenv("AURORA_DIAGNOSTIC_DRAW_UV")) {
    static unsigned snapshots=0;
    if(snapshots<64 && op.highWater.indices) {
      op.diagnosticIndices.assign(frame.indices.data(),frame.indices.data()+op.highWater.indices);
      ++snapshots;
    }
  }
  if (std::getenv("AURORA_DIAGNOSTIC_STORAGE_UPLOAD") != nullptr &&
      (!std::getenv("AURORA_DIAGNOSTIC_STORAGE_MATCH_ONLY") ||
       std::getenv("AURORA_DIAGNOSTIC_DRAW_UV"))) {
    static unsigned snapshots = 0;
    static uint64_t previousFrame = UINT64_MAX;
    static uint32_t previousStorage = 0;
    if (previousFrame != frame.frameId) { previousFrame = frame.frameId; previousStorage = 0; }
    if (snapshots < 128 && op.highWater.storage > previousStorage) {
      op.diagnosticStorage.assign(frame.storage.data(), frame.storage.data() + op.highWater.storage);
      ++snapshots;
    }
    previousStorage = op.highWater.storage;
  }
  if (std::getenv("AURORA_DIAGNOSTIC_UNIFORM_UPLOAD") &&
      std::getenv("AURORA_DIAGNOSTIC_TEXTURE_MATRICES")) {
    static unsigned snapshots = 0;
    static uint64_t previousFrame = UINT64_MAX;
    static uint32_t previousUniforms = 0;
    if (previousFrame != frame.frameId) { previousFrame = frame.frameId; previousUniforms = 0; }
    if (snapshots < 128 && op.highWater.uniforms > previousUniforms) {
      op.diagnosticUniforms.assign(frame.uniforms.data(), frame.uniforms.data() + op.highWater.uniforms);
      ++snapshots;
    }
    previousUniforms = op.highWater.uniforms;
  }
  for (size_t i = 0; i < op.highWater.textureUploadCount; ++i) {
    op.textureUploads.push_back(&frame.textureUploads[i]);
  }
  return op;
}

void seal_pass(FramePacket& frame, uint32_t passIndex) {
  if (passIndex >= frame.renderPasses.size()) {
    return;
  }
  auto& pass = frame.renderPasses[passIndex];
  if (pass.sealed) {
    return;
  }
  pass.sealed = true;
}

Range push(ByteBuffer& target, const uint8_t* data, size_t length, size_t alignment) {
  if (recording_diagnostics::enabled(recording_diagnostics::Startup,"MELEE_DIAGNOSTIC_GAME_STARTUP")) {
    const size_t begin = target.size();
    const size_t aligned = alignment ? AURORA_ALIGN(begin, alignment) : begin;
    if (aligned > target.capacity() || length > target.capacity() - aligned) {
      const auto& packet = *g_recorder.packet;
      const char* kind = &target == &packet.storage ? "storage" :
                         &target == &packet.verts ? "vertices" :
                         &target == &packet.indices ? "indices" :
                         &target == &packet.uniforms ? "uniforms" : "other";
      std::fprintf(stderr,"NATIVE_RECORDING_OVERFLOW buffer=%s used=%zu aligned=%zu capacity=%zu request=%zu source=%p\n",
                   kind,begin,aligned,target.capacity(),length,static_cast<const void*>(data));
    }
  }
  if (alignment != 0) {
    const size_t begin = target.size();
    const size_t alignedBegin = AURORA_ALIGN(begin, alignment);
    if (alignedBegin > begin) {
      target.append_zeroes(alignedBegin - begin);
    }
  }
  const auto begin = target.size();
  if (length > 0) {
    target.append(data, length);
  }
  return {static_cast<uint32_t>(begin), static_cast<uint32_t>(length)};
}

Range map(ByteBuffer& target, size_t length, size_t alignment) {
  if (alignment != 0) {
    const size_t begin = target.size();
    const size_t alignedBegin = AURORA_ALIGN(begin, alignment);
    if (alignedBegin > begin) {
      target.append_zeroes(alignedBegin - begin);
    }
  }
  auto begin = target.size();
  if (length > 0) {
    target.append_zeroes(length);
  }
  return {static_cast<uint32_t>(begin), static_cast<uint32_t>(length)};
}

// For our public API, warn instead of fatal-ing when called outside an active recording frame.
bool check_recording(const char* name) {
  if (!g_recorder.active())
    UNLIKELY {
      Log.warn("{}: called outside an active frame", name);
      return false;
    }
  return true;
}

void push_command(CommandType type, const Command::Data& data) {
  if (g_recorder.currentRenderPass == UINT32_MAX)
    UNLIKELY {
      Log.warn("Dropping command {}", magic_enum::enum_name(type));
      return;
    }
  auto& renderPass = current_render_passes()[g_recorder.currentRenderPass];
  AURORA_ASSERT(!renderPass.sealed, "Attempted to append command {} to sealed render pass {}",
                magic_enum::enum_name(type), g_recorder.currentRenderPass);
  if (type == CommandType::Draw || type == CommandType::CustomDraw) {
    renderPass.hasDraws = true;
  }
  renderPass.commands.push_back({
      .type = type,
#ifdef AURORA_GFX_DEBUG_GROUPS
      .debugGroupStack = g_recorder.debugGroupStack,
#endif
      .data = data,
  });
}

template <class T>
concept InlinePayload =
    std::is_trivially_copyable_v<T> && std::is_aggregate_v<T> && std::is_trivially_destructible_v<T>;

template <InlinePayload T>
T& inline_payload(void* ptr) noexcept {
  // C++20 equivalent of C++23's std::start_lifetime_as
  return *std::launder(static_cast<T*>(std::memmove(ptr, ptr, sizeof(T))));
}

template <auto Renderer, InlinePayload T>
void encode_draw(void* payload, const wgpu::RenderPassEncoder& pass, const RenderPass& passInfo) {
  const T& data = inline_payload<T>(payload);
  if constexpr (std::is_invocable_v<decltype(Renderer), const T&, const wgpu::RenderPassEncoder&,
                                    const wgpu::Extent3D&>) {
    Renderer(data, pass, passInfo.targetSize);
  } else {
    static_assert(std::is_invocable_v<decltype(Renderer), const T&, const wgpu::RenderPassEncoder&>);
    Renderer(data, pass);
  }
}

template <auto Renderer, InlinePayload T>
DrawCommand make_draw_command(const T& data) {
  static_assert(sizeof(T) <= InlineDrawPayloadSize);
  static_assert(alignof(T) <= alignof(std::max_align_t));
  DrawCommand command{.encoder = encode_draw<Renderer, T>};
  std::memcpy(command.payload.data(), &data, sizeof(data));
  return command;
}

void push_draw_command(DrawCommand data) {
  push_command(CommandType::Draw, Command::Data{.draw = data});
  ++g_recorder.drawCallCount;
}

OffscreenCacheEntry get_offscreen_textures(uint32_t width, uint32_t height) {
  OffscreenCacheKey key{width, height};
  if (const auto it = g_offscreenCache.find(key); it != g_offscreenCache.end()) {
    return it->second;
  }
  const auto colorFormat = webgpu::g_graphicsConfig.surfaceConfiguration.format;
  const wgpu::Extent3D size{width, height, 1};
  const wgpu::TextureDescriptor colorDesc{
      .label = "Offscreen Color",
      .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc |
               wgpu::TextureUsage::CopyDst,
      .dimension = wgpu::TextureDimension::e2D,
      .size = size,
      .format = colorFormat,
      .mipLevelCount = 1,
      .sampleCount = 1,
  };
  auto colorTexture = webgpu::g_device.CreateTexture(&colorDesc);
  auto colorView = colorTexture.CreateView();
  webgpu::TextureWithSampler color{
      .texture = std::move(colorTexture),
      .view = std::move(colorView),
      .size = size,
      .format = colorFormat,
  };
  const auto depthFormat = webgpu::g_graphicsConfig.depthFormat;
  const wgpu::TextureDescriptor depthDesc{
      .label = "Offscreen Depth",
      .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
      .dimension = wgpu::TextureDimension::e2D,
      .size = size,
      .format = depthFormat,
      .mipLevelCount = 1,
      .sampleCount = 1,
  };
  auto depthTexture = webgpu::g_device.CreateTexture(&depthDesc);
  auto depthView = depthTexture.CreateView();
  webgpu::TextureWithSampler depth{
      .texture = std::move(depthTexture),
      .view = std::move(depthView),
      .size = size,
      .format = depthFormat,
  };
  OffscreenCacheEntry entry{
      .color = std::move(color),
      .depth = std::move(depth),
  };
  auto [insertIt, _] = g_offscreenCache.emplace(key, std::move(entry));
  return insertIt->second;
}

void resume_efb_pass_loading(const RenderPass& prevPass) {
  RenderPass newPass{
      .label = pass_label("EFB"),
      .colorView = prevPass.colorView,
      .resolveView = prevPass.resolveView,
      .depthStencilView = prevPass.depthStencilView,
      .copySourceTexture = prevPass.copySourceTexture,
      .copySourceView = prevPass.copySourceView,
      .copySourceDepthView = prevPass.copySourceDepthView,
      .targetSize = prevPass.targetSize,
      .msaaSamples = prevPass.msaaSamples,
      .clearColor = false,
      .clearDepth = false,
      .hasDepth = prevPass.hasDepth,
      .hasStencil = prevPass.hasStencil,
  };
  newPass.commands.reserve(2048);
  current_render_passes().emplace_back(std::move(newPass));
  ++g_recorder.currentRenderPass;
  push_command(CommandType::SetViewport, Command::Data{.setViewport = g_recorder.cachedViewport});
  push_command(CommandType::SetScissor, Command::Data{.setScissor = g_recorder.cachedScissor});
}

void enqueue_op(FramePacket& frame, uint32_t opIndex) {
  if (opIndex >= frame.ops.size()) {
    return;
  }
  auto op = frame.ops[opIndex];
  render_worker::enqueue_encode_pass(frame.frameId, opIndex, [packet = &frame, opIndex, op = std::move(op)] {
    if (op.renderPass == nullptr && op.textureCopy == nullptr && op.encoderTask == nullptr) {
      return;
    }
    if(std::getenv("AURORA_DIAGNOSTIC_WORKER_ITEMS")){
      static unsigned records=0;
      if(records<131072)std::fprintf(stderr,"NATIVE_WORKER_OP record=%u frame=%llu pass=%u kind=%u task=%u\n",
        ++records,(unsigned long long)packet->frameId,opIndex,unsigned(op.type),op.encoderTask?unsigned(op.encoderTask->type):0u);
    }
    encode_op(packet->encoder, *packet, op);
  });
}

void enqueue_pass(FramePacket& frame, uint32_t passIndex) {
  if(std::getenv("AURORA_DIAGNOSTIC_PREPARE_GX_PASSES")) {
    preparation_progress::begin(static_cast<unsigned>(g_recorder.preparingGxPipelines.size()));
    for(const auto ref:g_recorder.preparingGxPipelines) {
      AURORA_ASSERT(prepare_pipeline(ref),"Required GX pass preparation failed");
      preparation_progress::advance();
    }
    preparation_progress::finish();
    if(!g_recorder.preparingGxPipelines.empty()) {
      static unsigned samples=0;
      if(samples++<1024) std::fprintf(stderr,"NATIVE_GX_PASS_PREPARED frame=%llu pass=%u pipelines=%zu before_enqueue=1\n",static_cast<unsigned long long>(frame.frameId),passIndex,g_recorder.preparingGxPipelines.size());
    }
    g_recorder.preparingGxPipelines.clear();
  }
  seal_pass(frame, passIndex);
  const auto opIndex = static_cast<uint32_t>(frame.ops.size());
  frame.ops.emplace_back(capture_frame_op(frame, FrameOpType::RenderPass, passIndex));
  enqueue_op(frame, opIndex);
}
} // namespace

namespace detail {

uint64_t diagnostic_recording_id() noexcept {
  return g_recorder.active() ? g_recorder.packet->frameId : 0;
}
const graphics_quality::FrameQuality* recording_graphics_quality() noexcept {
  if(graphics_quality::ProcessingContext::active())
    return g_recorder.active()?&g_recorder.graphicsDrawScope.current():nullptr;
  // Owner services/publishes see immutable frame settings, never mutable worker HUD state.
  // Unrelated/background readers must not inspect the recorder at all.
  if(!aurora_diagnostic_is_owner())return nullptr;
  return g_recorder.active()?&g_recorder.packet->graphicsQuality:nullptr;
}
bool recording_hud_graphics() noexcept {
  return graphics_quality::ProcessingContext::active() && g_recorder.active() && g_recorder.graphicsDrawScope.depth()!=0;
}
void trace_hud_graphics(const char* event) noexcept {
  if(!std::getenv("MELEE_DIAGNOSTIC_HUD_QUALITY"))return;
  static unsigned reports=0;if(reports++>=64)return;
  const auto& q=g_recorder.graphicsDrawScope.current();
  std::fprintf(stderr,"NATIVE_HUD_QUALITY event=%s frame=%llu depth=%u requested_texture=%u effective_texture=%u requested_lighting=%u effective_lighting=%u\n",
    event,static_cast<unsigned long long>(g_recorder.packet->frameId),g_recorder.graphicsDrawScope.depth(),
    q.requested.texturePercent,q.texturePercent,unsigned(q.requested.lightingEnabled),unsigned(q.lightingEnabled));
}
bool push_hud_graphics() noexcept {
  if(!g_recorder.graphicsDrawScope.push(graphics_quality::ProcessingContext::active(),g_recorder.active(),false))return false;
  trace_hud_graphics("push");return true;
}
bool pop_hud_graphics() noexcept {
  if(!g_recorder.graphicsDrawScope.pop(graphics_quality::ProcessingContext::active(),g_recorder.active(),false))return false;
  trace_hud_graphics("pop");return true;
}
bool diagnostic_select_snapshot(uint64_t frameId, TextureHandle texture) {
  if(!frameId || diagnostic_recording_id()!=frameId || !texture || g_recorder.diagnosticSnapshot) return false;
  g_recorder.diagnosticSnapshot=std::move(texture);
  return true;
}
TextureHandle diagnostic_selected_snapshot() { return g_recorder.active() ? g_recorder.diagnosticSnapshot : nullptr; }

void begin_recording(FramePacket& packet, size_t frameSlot) {
  CHECK(!g_recorder.active(), "A recording session is already active");
  packet.graphicsQuality=graphics_quality::scene_snapshot();
  g_recorder.graphicsDrawScope.begin(packet.graphicsQuality);
  g_recorder.packet = &packet;
  if(std::getenv("MELEE_DIAGNOSTIC_QUALITY_SCOPE")) {
    static std::optional<graphics_quality::FrameQuality> previous;
    static unsigned reports=0;
    const auto& q=packet.graphicsQuality;
    if((!previous || *previous!=q) && reports++<256)
      std::fprintf(stderr,"NATIVE_GRAPHICS_QUALITY_FRAME frame=%llu gameplay=%u requested_texture=%u effective_texture=%u requested_lighting=%u effective_lighting=%u\n",
        static_cast<unsigned long long>(packet.frameId),unsigned(q.gameplay),q.requested.texturePercent,
        q.texturePercent,unsigned(q.requested.lightingEnabled),unsigned(q.lightingEnabled));
    previous=q;
  }
  g_recorder.diagnosticSnapshot.reset();
  g_recorder.preparingGxPipelines.clear();
  g_recorder.uniformRangesByHash.clear();
  g_recorder.frameSlot = frameSlot;
  g_passSnapshotPools[frameSlot].used = 0;
  g_recorder.drawCallCount = 0;
  g_recorder.mergedDrawCallCount = 0;
  g_recorder.suspendedEfbPass.reset();

  current_render_passes().emplace_back();
  auto& pass = current_render_passes()[0];
  pass.label = pass_label("EFB");
  set_efb_targets(pass);
  pass.clearColorValue = gx::g_gxState.clearColor;
  pass.clearDepthValue = gx::clear_depth_value();
  g_recorder.currentRenderPass = 0;
  g_recorder.cachedViewport = gx::map_logical_viewport(gx::g_gxState.logicalViewport);
  g_recorder.cachedScissor = gx::map_logical_scissor(gx::g_gxState.logicalScissor);
  push_command(CommandType::SetViewport, Command::Data{.setViewport = g_recorder.cachedViewport});
  push_command(CommandType::SetScissor, Command::Data{.setScissor = g_recorder.cachedScissor});
}

RecordedFrame end_recording() {
  CHECK(g_recorder.active(), "No active recording session");
  CHECK(g_recorder.graphicsDrawScope.depth()==0, "Unbalanced HUD graphics scope");
  AURORA_ASSERT(!g_recorder.inOffscreen, "end_frame called while offscreen rendering is active");
  AURORA_ASSERT(g_recorder.currentRenderPass == UINT32_MAX,
                "end_frame called before finish finalized the current render pass");
  auto& frame = g_recorder.frame();
  frame.stats.drawCallCount = g_recorder.drawCallCount;
  frame.stats.mergedDrawCallCount = g_recorder.mergedDrawCallCount;
  frame.stats.lastVertSize = frame.verts.size();
  frame.stats.lastUniformSize = frame.uniforms.size();
  frame.stats.lastIndexSize = frame.indices.size();
  frame.stats.lastStorageSize = frame.storage.size();
  frame.stats.lastTextureUploadSize = frame.textureUpload.size();

  for (auto& array : gx::g_gxState.arrays) {
    array.cachedRange = {};
  }
#if defined(AURORA_GFX_DEBUG_GROUPS)
  if (!g_recorder.debugGroupStack.empty()) {
    for (auto& item : std::ranges::reverse_view(g_recorder.debugGroupStack)) {
      Log.warn("Debug group was not popped at end of frame: {}", item);
    }
    g_recorder.debugGroupStack.clear();
  }
#endif
  const RecordedFrame recorded{.packet = g_recorder.packet, .frameSlot = g_recorder.frameSlot};
  g_recorder.packet = nullptr;
  g_recorder.diagnosticSnapshot.reset();
  g_recorder.frameSlot = 0;
  return recorded;
}

void shutdown_recording() {
  for (auto& pool : g_passSnapshotPools) {
    pool = {};
  }
  g_recorder.currentRenderPass = UINT32_MAX;
  g_offscreenCache.clear();
  g_recorder.offscreenColor = {};
  g_recorder.offscreenDepth = {};
  g_recorder.suspendedEfbPass.reset();
  g_recorder.inOffscreen = false;
  g_recorder.graphicsDrawScope.begin({});
  g_recorder.packet = nullptr;
  g_recorder.frameSlot = 0;
}

void increment_merged_draw_count() noexcept {
  if (g_recorder.active()) {
    ++g_recorder.mergedDrawCallCount;
  }
}

} // namespace detail

void queue_texture_upload(TextureUpload upload) {
  if (g_recorder.currentRenderPass != UINT32_MAX) {
    AURORA_ASSERT(!current_render_passes()[g_recorder.currentRenderPass].sealed,
                  "Attempted to append texture upload to sealed render pass {}", g_recorder.currentRenderPass);
  }
  current_frame_packet().textureUploads.emplace_back(std::move(upload));
}

void queue_texture_upload_data(const uint8_t* data, uint32_t bytesPerRow, uint32_t rowsPerImage,
                               wgpu::TexelCopyTextureInfo tex, wgpu::Extent3D size) {
  const auto copyBytesPerRow = AURORA_ALIGN(bytesPerRow, 256);
  std::vector<uint8_t> expected;
  if (std::getenv("AURORA_DIAGNOSTIC_STAGED_TEXTURE_UPLOAD") &&
      tex.texture.GetFormat() == wgpu::TextureFormat::RGBA8Unorm)
    expected.assign(data, data + uint64_t(bytesPerRow) * rowsPerImage);
  auto& frame = current_frame_packet();
  if (frame.textureUpload.size() + copyBytesPerRow * rowsPerImage <= TextureUploadSize) {
    const auto range = push_texture_data(data, bytesPerRow, rowsPerImage);
    const wgpu::TexelCopyBufferLayout layout{
        .offset = range.offset,
        .bytesPerRow = bytesPerRow,
        .rowsPerImage = rowsPerImage,
    };
    TextureUpload upload{layout, std::move(tex), size};
    upload.diagnosticExpected = std::move(expected);
    queue_texture_upload(std::move(upload));
    return;
  }

  const uint64_t uploadSize = copyBytesPerRow * rowsPerImage;
  const wgpu::BufferDescriptor descriptor{
      .label = "Overflow Texture Upload Buffer",
      .usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc,
      .size = uploadSize,
      .mappedAtCreation = true,
  };
  auto buffer = webgpu::g_device.CreateBuffer(&descriptor);
  auto* dst = static_cast<uint8_t*>(buffer.GetMappedRange(0, uploadSize));
  for (uint32_t row = 0; row < rowsPerImage; ++row) {
    memcpy(dst, data, bytesPerRow);
    data += bytesPerRow;
    dst += copyBytesPerRow;
  }
  buffer.Unmap();

  const wgpu::TexelCopyBufferLayout layout{
      .offset = 0,
      .bytesPerRow = bytesPerRow,
      .rowsPerImage = rowsPerImage,
  };
  TextureUpload upload{layout, std::move(tex), size, std::move(buffer)};
  upload.diagnosticExpected = std::move(expected);
  queue_texture_upload(std::move(upload));
}

void queue_texture_copy(wgpu::TexelCopyTextureInfo src, wgpu::TexelCopyTextureInfo dst, wgpu::Extent3D size) {
  ZoneScoped;
  auto& frame = current_frame_packet();
  if (g_recorder.currentRenderPass != UINT32_MAX) {
    enqueue_pass(frame, g_recorder.currentRenderPass);
    g_recorder.currentRenderPass = UINT32_MAX;
  }

  const auto copyIndex = static_cast<uint32_t>(frame.textureCopies.size());
  frame.textureCopies.emplace_back(TextureCopy{
      .src = std::move(src),
      .dst = std::move(dst),
      .size = size,
  });
  const auto opIndex = static_cast<uint32_t>(frame.ops.size());
  frame.ops.emplace_back(capture_frame_op(frame, FrameOpType::TextureCopy, copyIndex));
  enqueue_op(frame, opIndex);
}

void begin_color_pass(const ColorPassDescriptor& desc) {
  ZoneScoped;
  auto& frame = current_frame_packet();
  if (g_recorder.currentRenderPass != UINT32_MAX) {
    enqueue_pass(frame, g_recorder.currentRenderPass);
  }

  RenderPass pass{
      .label = desc.label != nullptr ? desc.label : "",
      .colorView = desc.colorView,
      .resolveView = desc.resolveView,
      .depthStencilView = desc.depthStencilView,
      .targetSize = desc.targetSize,
      .msaaSamples = desc.sampleCount,
      .clearColorValue =
          {
              static_cast<float>(desc.clearColor.r),
              static_cast<float>(desc.clearColor.g),
              static_cast<float>(desc.clearColor.b),
              static_cast<float>(desc.clearColor.a),
          },
      .clearDepthValue = desc.depthClearValue,
      .colorLoadOp = desc.colorLoadOp,
      .colorStoreOp = desc.colorStoreOp,
      .depthLoadOp = desc.depthLoadOp,
      .depthStoreOp = desc.depthStoreOp,
      .stencilLoadOp = desc.stencilLoadOp,
      .stencilStoreOp = desc.stencilStoreOp,
      .stencilClearValue = desc.stencilClearValue,
      .clearColor = desc.colorLoadOp == wgpu::LoadOp::Clear,
      .clearDepth = desc.depthLoadOp == wgpu::LoadOp::Clear,
      .hasDepth = desc.hasDepth,
      .hasStencil = desc.hasStencil,
  };
  pass.commands.reserve(128);
  frame.renderPasses.emplace_back(std::move(pass));
  g_recorder.currentRenderPass = static_cast<uint32_t>(frame.renderPasses.size() - 1);

  g_recorder.cachedViewport = {
      0.f, 0.f, static_cast<float>(desc.targetSize.width), static_cast<float>(desc.targetSize.height), 0.f, 1.f};
  g_recorder.cachedScissor = {0, 0, static_cast<int32_t>(desc.targetSize.width),
                              static_cast<int32_t>(desc.targetSize.height)};
  push_command(CommandType::SetViewport, Command::Data{.setViewport = g_recorder.cachedViewport});
  push_command(CommandType::SetScissor, Command::Data{.setScissor = g_recorder.cachedScissor});
}

void end_color_pass() {
  ZoneScoped;
  if (g_recorder.currentRenderPass == UINT32_MAX) {
    return;
  }
  enqueue_pass(current_frame_packet(), g_recorder.currentRenderPass);
  g_recorder.currentRenderPass = UINT32_MAX;
}

template <>
gx::DrawData* get_last_draw_command() {
  if (g_recorder.currentRenderPass >= current_render_passes().size()) {
    return nullptr;
  }
  auto& last = current_render_passes()[g_recorder.currentRenderPass].commands.back();
  if (last.type != CommandType::Draw || last.data.draw.encoder != encode_draw<gx::render, gx::DrawData>) {
    return nullptr;
  }
  return &inline_payload<gx::DrawData>(last.data.draw.payload.data());
}

namespace detail {
void diagnostic_update_merged_draw(const gx::DrawData& draw) noexcept {
  if(gx::mesh_diagnostic::enabled()){
    auto& frame=current_frame_packet();
    if(g_recorder.currentRenderPass<frame.renderPasses.size())
      for(auto& snapshot:frame.renderPasses[g_recorder.currentRenderPass].originalMeshDraws)
        if(snapshot.draw.vertRange.offset==draw.vertRange.offset&&snapshot.draw.pipeline==draw.pipeline)snapshot.draw=draw;
  }
  if(!recording_diagnostics::enabled(recording_diagnostics::DrawUv,"AURORA_DIAGNOSTIC_DRAW_UV")) return;
  auto& frame=current_frame_packet();
  if(g_recorder.currentRenderPass>=frame.renderPasses.size()) return;
  for(auto& snapshot:frame.renderPasses[g_recorder.currentRenderPass].diagnosticReplayDraws) {
    if(snapshot.draw.vertRange.offset!=draw.vertRange.offset) continue;
    snapshot.draw=draw;
    ++snapshot.diagnosticMergedPrimitives;
    if(draw.vertRange.offset<=frame.verts.size() && draw.vertRange.size<=frame.verts.size()-draw.vertRange.offset) {
      snapshot.vertexStreamBytes=draw.vertRange.size;
      snapshot.vertexStreamHash=14695981039346656037ull;
      for(size_t i=0;i<draw.vertRange.size;++i)
        snapshot.vertexStreamHash=(snapshot.vertexStreamHash^frame.verts.data()[draw.vertRange.offset+i])*1099511628211ull;
    }
  }
  if(!std::getenv("AURORA_DIAGNOSTIC_DRAW_MERGE_AUDIT")) return;
  static unsigned samples=0;
  if(samples>=512) return;
  unsigned failures=0,maximum=0;
  if(draw.indexCount) {
    if(uint64_t(draw.indexCount)*2!=draw.idxRange.size || draw.indexCount%3 ||
       draw.idxRange.offset>frame.indices.size() || draw.idxRange.size>frame.indices.size()-draw.idxRange.offset) failures++;
    else for(unsigned i=0;i<draw.indexCount;++i) {
      uint16_t index;std::memcpy(&index,frame.indices.data()+draw.idxRange.offset+i*2,2);
      maximum=std::max(maximum,unsigned(index));
      if(index>=draw.vtxCount) failures++;
    }
  } else if(draw.vtxCount%3) failures++;
  std::fprintf(stderr,"FINAL_MERGED_INDEX_AUDIT sample=%u vertices=%u indices=%u bytes=%u maximum_index=%u failures=%u\n",
               ++samples,draw.vtxCount,draw.indexCount,draw.idxRange.size,maximum,failures);
}
bool diagnostic_uniform_ranges_equal(Range a, Range b) noexcept {
  const auto& bytes=current_frame_packet().uniforms;
  if(a.size!=b.size || a.offset>bytes.size() || b.offset>bytes.size() ||
     a.size>bytes.size()-a.offset || b.size>bytes.size()-b.offset) return false;
  return std::memcmp(bytes.data()+a.offset,bytes.data()+b.offset,a.size)==0;
}
}

Vec2<uint32_t> get_render_target_size() noexcept {
  if (g_recorder.currentRenderPass < current_render_passes().size()) {
    const auto& size = current_render_passes()[g_recorder.currentRenderPass].targetSize;
    return {size.width, size.height};
  }
  const auto windowSize = window::get_window_size();
  return {windowSize.fb_width, windowSize.fb_height};
}

void set_viewport(const Viewport& cmd) noexcept {
  if (cmd != g_recorder.cachedViewport) {
    push_command(CommandType::SetViewport, Command::Data{.setViewport = cmd});
    g_recorder.cachedViewport = cmd;
  }
}

void set_scissor(const ClipRect& cmd) noexcept {
  if (cmd != g_recorder.cachedScissor) {
    push_command(CommandType::SetScissor, Command::Data{.setScissor = cmd});
    g_recorder.cachedScissor = cmd;
  }
}

template <>
void push_draw_command(clear::DrawData data) {
  push_draw_command(make_draw_command<clear::render>(data));
}

template <>
PipelineRef pipeline_ref(const clear::PipelineConfig& config) {
  return find_pipeline(ShaderType::Clear, config, [=] { return create_pipeline(config); });
}

void resolve_pass_into(TextureHandle texture, ClipRect rect, bool clearColor, bool clearAlpha, bool clearDepth,
                       Vec4<float> clearColorValue, float clearDepthValue, GXTexFmt resolveFormat, DisplayCopyOptions displayCopy, std::shared_ptr<gx::xfb::Job> xfbCopy) {
  // Resolve current render pass
  auto& prevPass = current_render_passes()[g_recorder.currentRenderPass];
  prevPass.resolveTarget = std::move(texture);
  prevPass.resolveRect = rect;
  prevPass.resolveFormat = resolveFormat;
  prevPass.displayCopy = displayCopy;
  prevPass.xfbCopy = std::move(xfbCopy);
  if (prevPass.xfbCopy && (prevPass.resolveTarget->size.width != prevPass.xfbCopy->width ||
                          prevPass.resolveTarget->size.height != prevPass.xfbCopy->height)) {
    AURORA_ASSERT(prevPass.xfbCopy->width && prevPass.xfbCopy->height && displayCopy.enabled,
                  "Scaled XFB readback requires enrolled dimensions");
    prevPass.xfbReadbackTarget = new_render_texture(prevPass.xfbCopy->width, prevPass.xfbCopy->height,
                                                   GX_TF_RGBA8, "Logical XFB readback surface");
    prevPass.xfbResampleUniformRange = push_uniform(std::array<float, 4>{0.f, 0.f, 1.f, 1.f});
    if (std::getenv("AURORA_DIAGNOSTIC_RENDER_HEIGHT")) {
      static unsigned reported = 0;
      if (reported++ < 16) std::fprintf(stderr,
        "SCALED_XFB_COPY gpu_width=%u gpu_height=%u cpu_width=%u cpu_height=%u logical_row_step=%.9g\n",
        prevPass.resolveTarget->size.width, prevPass.resolveTarget->size.height,
        prevPass.xfbCopy->width, prevPass.xfbCopy->height, displayCopy.logicalRowStep);
    }
  }
  // Push UV transform uniform for tex_copy_conv (crop region in UV space)
  const auto srcW = static_cast<float>(prevPass.targetSize.width);
  const auto srcH = static_cast<float>(prevPass.targetSize.height);
  const std::array uvTransform{
      static_cast<float>(rect.x) / srcW,
      static_cast<float>(rect.y) / srcH,
      static_cast<float>(rect.width) / srcW,
      static_cast<float>(rect.height) / srcH,
  };
  if (displayCopy.enabled) {
    AURORA_ASSERT(resolveFormat == GX_TF_RGBA8 && prevPass.msaaSamples == 1,
                  "Display copy currently requires progressive non-AA RGB8 source");
    AURORA_ASSERT(displayCopy.logicalRowStep > 0.f && displayCopy.gammaReciprocal > 0.f,
                  "Invalid display-copy sampling options");
    const auto uniform = display_copy_uniform(displayCopy, float(rect.x), float(rect.y),
                                              float(rect.width), float(rect.height), srcW, srcH);
    prevPass.resolveUniformRange = push_uniform(uniform);
  } else {
    prevPass.resolveUniformRange = push_uniform(uvTransform);
  }
  enqueue_pass(current_frame_packet(), g_recorder.currentRenderPass);

  // Populate new render pass from previous
  const auto msaaSamples = prevPass.msaaSamples;
  const auto clearRegion = clear::copy_clear_region(rect.x, rect.y, rect.width, rect.height,
                                                   prevPass.targetSize.width, prevPass.targetSize.height);
  RenderPass newPass{
      .label = pass_label("EFB"),
      .colorView = prevPass.colorView,
      .resolveView = prevPass.resolveView,
      .depthStencilView = prevPass.depthStencilView,
      .copySourceTexture = prevPass.copySourceTexture,
      .copySourceView = prevPass.copySourceView,
      .copySourceDepthView = prevPass.copySourceDepthView,
      .targetSize = prevPass.targetSize,
      .msaaSamples = msaaSamples,
      .clearColorValue = clearColorValue,
      .clearDepthValue = clearDepthValue,
      .clearColor = clearColor && clearAlpha && clearRegion.coversTarget,
      .clearDepth = clearDepth && clearRegion.coversTarget,
      .hasDepth = prevPass.hasDepth,
      .hasStencil = prevPass.hasStencil,
  };
  newPass.commands.reserve(2048);
  current_render_passes().emplace_back(std::move(newPass));
  ++g_recorder.currentRenderPass;

  if ((clearColor && !newPass.clearColor) || (clearAlpha && !newPass.clearColor) ||
      (clearDepth && !newPass.clearDepth)) {
    // A partial copy clears only its rectangle; retain untouched EFB pixels.
    push_draw_command(clear::DrawData{
        .pipeline = pipeline_ref(clear::PipelineConfig{
            .msaaSamples = msaaSamples,
            .clearColor = clearColor && !newPass.clearColor,
            .clearAlpha = clearAlpha && !newPass.clearColor,
            .clearDepth = clearDepth && !newPass.clearDepth,
        }),
        .color =
            wgpu::Color{
                .r = clearColorValue.x(),
                .g = clearColorValue.y(),
                .b = clearColorValue.z(),
                .a = clearColorValue.w(),
            },
        .depth = clearDepthValue,
        .region = clearRegion,
        .regionEnabled = true,
    });
  }
  push_command(CommandType::SetViewport, Command::Data{.setViewport = g_recorder.cachedViewport});
  push_command(CommandType::SetScissor, Command::Data{.setScissor = g_recorder.cachedScissor});
}

void queue_palette_conv(tex_palette_conv::ConvRequest req) {
  auto& renderPass = current_render_passes()[g_recorder.currentRenderPass];
  AURORA_ASSERT(!renderPass.sealed, "Attempted to append palette conversion to sealed render pass {}",
                g_recorder.currentRenderPass);
  renderPass.paletteConvs.push_back(std::move(req));
}

bool is_offscreen() noexcept { return g_recorder.inOffscreen; }

uint32_t get_sample_count() noexcept {
  CHECK(g_recorder.currentRenderPass != UINT32_MAX, "get_sample_count called outside of a frame");
  return current_render_passes()[g_recorder.currentRenderPass].msaaSamples;
}

void clear_caches() noexcept {
  g_offscreenCache.clear();
  clear_bind_group_cache();
}

bool push_custom_draw(DrawTypeId type, const void* payload, size_t payloadSize) {
  if (type == InvalidDrawType) {
    Log.warn("push_custom_draw: invalid draw type");
    return false;
  }
  if (payloadSize > InlineDrawPayloadSize) {
    Log.warn("push_custom_draw: payload size {} exceeds inline payload size {}", payloadSize, InlineDrawPayloadSize);
    return false;
  }
  if (payloadSize > 0 && payload == nullptr) {
    Log.warn("push_custom_draw: non-zero payload size with null payload");
    return false;
  }
  if (!find_runtime_draw_type(type)) {
    Log.warn("push_custom_draw: unregistered draw type {:#x}", type);
    return false;
  }

  gx::fifo::drain();

  if (!g_recorder.active() || g_recorder.currentRenderPass == UINT32_MAX) {
    Log.warn("push_custom_draw: called outside an active render pass");
    return false;
  }

  CustomDrawCommand draw{};
  draw.type = type;
  draw.payloadSize = static_cast<uint32_t>(payloadSize);
  if (payloadSize > 0) {
    std::memcpy(draw.payload.data(), payload, payloadSize);
  }
  push_command(CommandType::CustomDraw, Command::Data{.customDraw = draw});
  ++g_recorder.drawCallCount;
  return true;
}

void begin_offscreen(uint32_t width, uint32_t height) {
  ZoneScoped;
  CHECK(g_recorder.currentRenderPass != UINT32_MAX, "begin_offscreen called outside of a frame");

  // If the current EFB pass has no resolve target, its output is unobservable.
  // Suspend it so that we can resume it after the offscreen pass.
  if (!g_recorder.inOffscreen) {
    auto& currentPass = current_render_passes()[g_recorder.currentRenderPass];
    if (!currentPass.resolveTarget) {
      g_recorder.suspendedEfbPass = std::move(currentPass);
      current_render_passes().pop_back();
      --g_recorder.currentRenderPass;
    } else {
      enqueue_pass(current_frame_packet(), g_recorder.currentRenderPass);
    }
    g_recorder.suspendedEfbViewport = g_recorder.cachedViewport;
    g_recorder.suspendedEfbScissor = g_recorder.cachedScissor;
  }

  // Create offscreen textures
  auto offscreenEntry = get_offscreen_textures(width, height);
  g_recorder.offscreenColor = std::move(offscreenEntry.color);
  g_recorder.offscreenDepth = std::move(offscreenEntry.depth);

  // Start a new pass with offscreen targets
  RenderPass newPass{
      .label = pass_label("Offscreen"),
      .colorView = g_recorder.offscreenColor.view,
      .depthStencilView = g_recorder.offscreenDepth.view,
      .copySourceTexture = g_recorder.offscreenColor.texture,
      .copySourceView = g_recorder.offscreenColor.view,
      .copySourceDepthView = g_recorder.offscreenDepth.view,
      .targetSize = {width, height, 1},
      .msaaSamples = 1,
      .clearColorValue = {0.f, 0.f, 0.f, 0.f},
      .clearDepthValue = gx::UseReversedZ ? 0.f : 1.f,
      .clearColor = true,
      .clearDepth = true,
      .hasDepth = true,
      .hasStencil = false,
  };
  current_render_passes().emplace_back(std::move(newPass));
  ++g_recorder.currentRenderPass;

  g_recorder.inOffscreen = true;

  g_recorder.cachedViewport = {0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f};
  g_recorder.cachedScissor = {0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height)};
  push_command(CommandType::SetViewport, Command::Data{.setViewport = g_recorder.cachedViewport});
  push_command(CommandType::SetScissor, Command::Data{.setScissor = g_recorder.cachedScissor});
}

void end_offscreen() {
  ZoneScoped;
  CHECK(g_recorder.inOffscreen, "end_offscreen called without begin_offscreen");

  // Mark current render pass as discardable if there is no consumer
  auto& offscreenPass = current_render_passes()[g_recorder.currentRenderPass];
  offscreenPass.discardable = !offscreenPass.has_consumer();

  enqueue_pass(current_frame_packet(), g_recorder.currentRenderPass);

  g_recorder.inOffscreen = false;
  g_recorder.offscreenColor = {};
  g_recorder.offscreenDepth = {};

  // Resume suspended EFB pass, or start a new one (load existing content)
  if (g_recorder.suspendedEfbPass) {
    current_render_passes().emplace_back(std::move(*g_recorder.suspendedEfbPass));
    g_recorder.suspendedEfbPass.reset();
  } else {
    auto& pass = current_render_passes().emplace_back();
    pass.label = pass_label("EFB");
    pass.clearColor = false;
    pass.clearDepth = false;
  }
  ++g_recorder.currentRenderPass;
  set_efb_targets(current_render_passes()[g_recorder.currentRenderPass]);

  g_recorder.cachedViewport = g_recorder.suspendedEfbViewport;
  g_recorder.cachedScissor = g_recorder.suspendedEfbScissor;
  push_command(CommandType::SetViewport, Command::Data{.setViewport = g_recorder.cachedViewport});
  push_command(CommandType::SetScissor, Command::Data{.setScissor = g_recorder.cachedScissor});
}

bool create_pass(uint32_t width, uint32_t height) {
  if (width == 0 || height == 0) {
    Log.warn("create_pass: invalid size {}x{}", width, height);
    return false;
  }

  gx::fifo::drain();

  if (!g_recorder.active() || g_recorder.currentRenderPass == UINT32_MAX) {
    Log.warn("create_pass: called outside an active render pass");
    return false;
  }
  if (g_recorder.inOffscreen) {
    Log.warn("create_pass: an offscreen pass is already active (nesting is unsupported)");
    return false;
  }

  begin_offscreen(width, height);
  return true;
}

bool resolve_pass(const ResolveDesc& desc, ResolvedTargets& out) {
  out = {};
  gx::fifo::drain();

  if (!g_recorder.active() || g_recorder.currentRenderPass == UINT32_MAX) {
    Log.warn("resolve_pass: called outside an active render pass");
    return false;
  }

  bool wantDepth = desc.depth;
  if (wantDepth && !tex_copy_conv::snapshot_depth_supported()) {
    Log.warn("resolve_pass: depth snapshots are unsupported on this device");
    wantDepth = false;
  }

  auto& prevPass = current_render_passes()[g_recorder.currentRenderPass];
  const uint32_t width = prevPass.targetSize.width;
  const uint32_t height = prevPass.targetSize.height;
  // Requesting no snapshots is a plain pass break (or offscreen close, discarding its output).
  if (desc.color || wantDepth) {
    auto& entry = acquire_pass_snapshot(width, height, desc.color, wantDepth);
    if (desc.color) {
      prevPass.snapshotColorDst = entry.color.texture;
      out.color = entry.color.view;
      out.colorFormat = entry.color.format;
    }
    if (wantDepth) {
      prevPass.snapshotDepthDst = entry.depth.view;
      out.depth = entry.depth.view;
    }
  }
  out.width = width;
  out.height = height;

  if (g_recorder.inOffscreen) {
    // Seal the offscreen pass and resume the EFB.
    end_offscreen();
    return true;
  }

  // Seal the current EFB pass and continue on a new one that loads the existing contents.
  // EFB writes persist into the loading continuation, so content keeps the pass alive even
  // without a consumer.
  prevPass.discardable = !prevPass.has_consumer() && !prevPass.has_content();
  enqueue_pass(current_frame_packet(), g_recorder.currentRenderPass);
  resume_efb_pass_loading(prevPass);
  return true;
}

bool push_encoder_task(EncoderTaskId type, const void* payload, size_t payloadSize) {
  if (type == InvalidEncoderTask) {
    Log.warn("push_encoder_task: invalid encoder task type");
    return false;
  }
  if (payloadSize > InlineDrawPayloadSize) {
    Log.warn("push_encoder_task: payload size {} exceeds inline payload size {}", payloadSize, InlineDrawPayloadSize);
    return false;
  }
  if (payloadSize > 0 && payload == nullptr) {
    Log.warn("push_encoder_task: non-zero payload size with null payload");
    return false;
  }
  if (!find_runtime_encoder_task_type(type)) {
    Log.warn("push_encoder_task: unregistered encoder task type {:#x}", type);
    return false;
  }

  gx::fifo::drain();

  if (!g_recorder.active() || g_recorder.currentRenderPass == UINT32_MAX) {
    Log.warn("push_encoder_task: called outside an active render pass");
    return false;
  }
  if (g_recorder.inOffscreen) {
    Log.warn("push_encoder_task: unsupported while an offscreen pass is active");
    return false;
  }

  // Seal the current EFB pass, record the task between it and a continuation
  // pass that loads the existing contents. EFB writes persist into the continuation, so
  // content keeps the sealed pass alive even without a consumer.
  auto& frame = current_frame_packet();
  auto& prevPass = current_render_passes()[g_recorder.currentRenderPass];
  prevPass.discardable = !prevPass.has_consumer() && !prevPass.has_content();
  enqueue_pass(frame, g_recorder.currentRenderPass);

  const auto taskIndex = static_cast<uint32_t>(frame.encoderTasks.size());
  auto& task = frame.encoderTasks.emplace_back(EncoderTask{.type = type});
  task.payloadSize = static_cast<uint32_t>(payloadSize);
  if (payloadSize > 0) {
    std::memcpy(task.payload.data(), payload, payloadSize);
  }
  const auto opIndex = static_cast<uint32_t>(frame.ops.size());
  frame.ops.emplace_back(capture_frame_op(frame, FrameOpType::EncoderTask, taskIndex));
  enqueue_op(frame, opIndex);

  resume_efb_pass_loading(prevPass);
  return true;
}

namespace detail {
void diagnostic_original_mesh_primitive(const gx::DrawData& draw,const gx::ShaderConfig& config,
    const uint8_t* bytes,uint32_t size,uint32_t firstVertex) noexcept {
  const auto textureMask=original_texture::enabled()?static_cast<uint32_t>(gx::build_shader_info(config).sampledTextures.to_ulong()):0;
  if(original_texture::enabled()&&textureMask&&g_recorder.currentRenderPass!=UINT32_MAX){
    // Recording/FIFO-owner state. Capture the actual draw's immutable binding,
    // not a later global texture state on the encoding worker.
    static std::map<std::pair<uint64_t,uint32_t>,std::pair<uint64_t,unsigned>> transitions;
    auto& target=current_frame_packet().renderPasses[g_recorder.currentRenderPass].originalTextureDraws;
    for(unsigned slot=0;slot<8;slot++)if(textureMask&(1u<<slot)){
      const auto& bound=gx::get_texture(static_cast<GXTexMapID>(slot));
      auto source=original_texture::find_object(bound.texObj.texObjId,bound.texObj.texDataVersion);if(!source)continue;
      const auto& m=source->metadata;auto& state=transitions[{m.bodyHash,m.textureOffset}];
      if(state.first==source->id||state.second>=12)continue;
      if(m.paletteBytes){const auto idx=unsigned(bound.texObj.tlut);if(idx>=gx::g_gxState.loadedTluts.size()){std::fprintf(stderr,"ORIGINAL_TEXTURE_RECORD_FAIL reason=palette-slot\n");continue;}const auto& palette=gx::g_gxState.loadedTluts[idx];if(palette.data!=m.paletteData||unsigned(palette.format)!=m.paletteFormat||palette.numEntries!=m.paletteEntries){std::fprintf(stderr,"ORIGINAL_TEXTURE_RECORD_FAIL reason=palette-binding\n");continue;}}
      if(bound.texObj.data!=m.data||!bound.ref||bound.ref->isReplacement||bound.ref->size.width!=m.width||bound.ref->size.height!=m.height||
         bound.texObj.min_lod()!=0||bound.texObj.max_lod()!=0||target.size()>=32){std::fprintf(stderr,"ORIGINAL_TEXTURE_RECORD_FAIL reason=binding-scope\n");continue;}
      state.first=source->id;++state.second;target.push_back({draw,std::move(source),slot,state.second});
      if(std::getenv("MELEE_DIAGNOSTIC_ORIGINAL_MATERIAL")){
        target.back().config=config;target.back().firstVertex=firstVertex;static std::set<uint64_t> reported;const uint64_t hash=xxh3_hash(config);
        if(reported.insert(hash).second){const auto info=gx::build_shader_info(config);
          std::fprintf(stderr,"ORIGINAL_MATERIAL_CONFIG shader=%llu stages=%u fog=%u indirect=%u textures=%u texcoords=%u registers=%u konst=%u channels=%u\n",(unsigned long long)hash,config.tevStageCount,config.fogType,config.numIndStages,(unsigned)info.sampledTextures.to_ulong(),(unsigned)info.sampledTexCoords.to_ulong(),(unsigned)info.loadsTevReg.to_ulong(),(unsigned)info.sampledKColors.to_ulong(),(unsigned)info.sampledColorChannels.to_ulong());
          for(unsigned n=0;n<config.tevStageCount;n++){const auto& s=config.tevStages[n];std::fprintf(stderr,"ORIGINAL_MATERIAL_STAGE shader=%llu stage=%u ca=%u cb=%u cc=%u cd=%u aa=%u ab=%u ac=%u ad=%u color_op=%u alpha_op=%u color_bias=%u alpha_bias=%u color_scale=%u alpha_scale=%u color_clamp=%u alpha_clamp=%u color_out=%u alpha_out=%u texture=%u coord=%u channel=%u\n",(unsigned long long)hash,n,s.colorPass.a,s.colorPass.b,s.colorPass.c,s.colorPass.d,s.alphaPass.a,s.alphaPass.b,s.alphaPass.c,s.alphaPass.d,s.colorOp.op,s.alphaOp.op,s.colorOp.bias,s.alphaOp.bias,s.colorOp.scale,s.alphaOp.scale,s.colorOp.clamp,s.alphaOp.clamp,s.colorOp.outReg,s.alphaOp.outReg,s.texMapId,s.texCoordId,s.channelId);}
        }
      }
      std::fprintf(stderr,"ORIGINAL_TEXTURE_DRAW source=%llu texture=%u image=%u slot=%u sequence=%u group=%llu\n",(unsigned long long)state.first,m.textureOffset,m.imageOffset,slot,state.second,(unsigned long long)draw.bindGroups.textureBindGroup);
    }
  }
  using namespace gx::mesh_diagnostic;
  if(!enabled())return;
  auto context=active.load(std::memory_order_acquire);if(!context)return;
  const auto primitive=context->cursor.fetch_add(1);const auto& source=context->source;
  auto fail=[&](const char* reason){std::fprintf(stderr,"MESH_GPU_RECORD_FAIL id=%llu primitive=%u reason=%s\n",(unsigned long long)context->id,primitive,reason);};
  if(primitive>=source.primitives){fail("primitive-count");return;}
  const auto& original=source.primitive[primitive];
  if(size!=original.bytes||std::memcmp(bytes,context->original.data()+original.offset,size)){fail("original-bytes");return;}
  const auto& attr=config.attrs[GX_VA_POS];
  if(config.lineMode||config.vtxStride!=source.stride||attr.offset!=source.positionOffset||attr.attrType!=source.positionStorage||
     attr.cnt!=source.positionComponents||attr.compType!=source.positionType||attr.frac!=source.positionFraction||
     (attr.attrType!=GX_DIRECT&&(attr.stride!=source.positionStride||attr.le))){fail("source-layout");return;}
  auto& frame=current_frame_packet();
  if(g_recorder.currentRenderPass>=frame.renderPasses.size()||draw.instanceCount!=1||
     firstVertex>draw.vtxCount||original.points>draw.vtxCount-firstVertex){fail("draw-extent");return;}
  auto& snapshots=frame.renderPasses[g_recorder.currentRenderPass].originalMeshDraws;
  if(snapshots.size()>=128){fail("snapshot-cap");return;}
  const size_t base=draw.uniformRange.offset;
  if(base>frame.uniforms.size()||draw.uniformRange.size>frame.uniforms.size()-base||draw.uniformRange.size<80+20*48){fail("uniform-extent");return;}
  Snapshot snapshot{.draw=draw,.config=config,.context=context,.primitive=primitive,.firstVertex=firstVertex,.points=original.points};
  for(unsigned i=0;i<8;i++)if(source.uvMask&(1u<<i)){
    const auto& uv=config.attrs[GX_VA_TEX0+i];const auto& ref=source.uv[i];
    if(uv.attrType!=ref.storage||uv.offset!=ref.offset||uv.cnt!=ref.components||uv.compType!=ref.type||uv.frac!=ref.fraction||
       (uv.attrType!=GX_DIRECT&&(uv.stride!=ref.stride||uv.le))){fail("original-uv-layout");return;}
  }
  if(source.texgenMask){
    const auto checked=texgen_uniforms(frame.uniforms.data()+base,draw.uniformRange.size,config,gx::build_shader_info(config),source);
    if(!checked.valid){std::fprintf(stderr,"MESH_TEXGEN_UNIFORM_FAIL id=%llu primitive=%u reason=%s\n",(unsigned long long)context->id,primitive,checked.reason);fail(checked.reason);return;}
    snapshot.texgenMask=checked.mask;
    if(checked.mask)std::fprintf(stderr,"MESH_TEXGEN_UNIFORM_PASS id=%llu primitive=%u actor=%u game_frame=%u mask=%u components=%u max_abs_error=%.9g\n",
      (unsigned long long)context->id,primitive,source.actor,source.frame,checked.mask,checked.components,checked.maximum);
    unsigned srtMask=0;for(unsigned i=0;i<8;i++)if((checked.mask&(1u<<i))&&source.texgen[i].sourceSrtBytes==36)srtMask|=1u<<i;
    if(srtMask)std::fprintf(stderr,"MESH_TEXTURE_SRT_PASS id=%llu primitive=%u actor=%u game_frame=%u mask=%u source_bytes=36\n",(unsigned long long)context->id,primitive,source.actor,source.frame,srtMask);
  }
  if(source.lightingReference){
    const auto info=gx::build_shader_info(config);
    const auto checked=light_uniforms(frame.uniforms.data()+base,draw.uniformRange.size,config,info,source);
    if(!checked.valid){std::fprintf(stderr,"MESH_LIGHT_UNIFORM_FAIL id=%llu primitive=%u reason=%s offset=%zu max_abs_error=%.9g\n",(unsigned long long)context->id,primitive,checked.reason,checked.firstOffset,checked.maximum);fail(checked.reason);return;}
    snapshot.colorLanes=color_usage(config,&info);
    for(unsigned coord=0;coord<gx::MaxTexCoord;coord++)if(info.sampledTexCoords.test(coord)&&config.tcgs[coord].type==GX_TG_SRTG)
      std::fprintf(stderr,"MESH_TEXGEN_COLOR id=%llu primitive=%u actor=%u game_frame=%u coord=%u channel=%u lanes=%u\n",
        (unsigned long long)context->id,primitive,source.actor,source.frame,coord,unsigned(config.tcgs[coord].src==GX_TG_COLOR1),snapshot.colorLanes);
    snapshot.colorMask=((snapshot.colorLanes&15)?1u:0u)|((snapshot.colorLanes&240)?2u:0u);
    for(unsigned i=0;i<2;i++){
      const auto& color=config.attrs[GX_VA_CLR0+i];
      if(color.attrType!=source.colorStorage[i]||(color.attrType!=GX_NONE&&
         (color.offset!=source.colorOffset[i]||color.compType!=source.colorType[i]||
          (color.attrType!=GX_DIRECT&&color.stride!=source.colorStride[i])))){fail("original-color-layout");return;}
    }
    std::fprintf(stderr,"MESH_LIGHT_UNIFORM_PASS id=%llu primitive=%u components=%u max_abs_error=%.9g color_mask=%u source_material_bytes=%u\n",(unsigned long long)context->id,primitive,checked.components,checked.maximum,snapshot.colorMask,source.sourceMaterialChecks);
  }
  float projection[16];std::memcpy(projection,frame.uniforms.data()+base+16,64);
  if(source.paletteMask){
    unsigned compared=0,mismatches=0,firstSlot=0,firstKind=0;double maximum=0;
    if(source.normalMask&&draw.uniformRange.size<80+20*48+10*48){fail("normal-uniform-extent");return;}
    for(unsigned slot=0;slot<10;slot++){
      if(source.paletteMask&(1u<<slot))for(unsigned n=0;n<12;n++){
        float actual;std::memcpy(&actual,frame.uniforms.data()+base+80+slot*48+n*4,4);
        const double expected=source.positionPalette[slot][n],error=std::abs(double(actual)-expected);
        ++compared;if(!std::isfinite(actual)||error>2e-4*(1+std::abs(expected))){if(!mismatches){firstSlot=slot;firstKind=0;}mismatches++;}
        if(std::isfinite(error))maximum=std::max(maximum,error);
      }
      if(source.normalMask&(1u<<slot))for(unsigned r=0;r<3;r++)for(unsigned c=0;c<3;c++){
        float actual;std::memcpy(&actual,frame.uniforms.data()+base+80+20*48+slot*48+r*16+c*4,4);
        const double expected=source.normalPalette[slot][r*3+c],error=std::abs(double(actual)-expected);
        ++compared;if(!std::isfinite(actual)||error>2e-4*(1+std::abs(expected))){if(!mismatches){firstSlot=slot;firstKind=1;}mismatches++;}
        if(std::isfinite(error))maximum=std::max(maximum,error);
      }
    }
    std::fprintf(stderr,"MESH_PALETTE_%s id=%llu primitive=%u actor=%u game_frame=%u kind=%u mask=%u normal_mask=%u bones=%u weights=%u bind_components=%u weighted_slots=%u right_slots=%u components=%u mismatches=%u first_slot=%u first_kind=%u max_abs_error=%.9g\n",
      mismatches?"FAIL":"PASS",(unsigned long long)context->id,primitive,source.actor,source.frame,source.paletteKind,
      source.paletteMask,source.normalMask,source.sourceBoneChecks,source.sourceWeightChecks,source.sourceBindComponents,source.weightedSlots,source.rightSlots,
      compared,mismatches,firstSlot,firstKind,maximum);
    if(mismatches){fail("original-palette-upload");return;}
  }
  if(source.normalComponents){
    const auto& normal=config.attrs[GX_VA_NRM];
    const unsigned fraction=source.normalType==GX_S8?6:source.normalType==GX_S16?14:0;
    if(normal.offset!=source.normalOffset||normal.attrType!=source.normalStorage||normal.cnt!=source.normalComponents||
       normal.compType!=source.normalType||normal.frac!=fraction||normal.nbt3!=bool(source.normalNbt3)||
       (normal.attrType!=GX_DIRECT&&(normal.stride!=source.normalStride||normal.le))){fail("original-normal-layout");return;}
  }
  for(uint32_t n=0;n<original.points;n++){
    const auto& point=original.point[n];
    for(unsigned i=0;i<8;i++)if((snapshot.texgenMask&(1u<<i))&&!texgen_reference(source.texgen[i],point,snapshot.expectedTexgens.data()+n*16+i*2)){
      fail("texgen-nonfinite-reference");return;
    }
    const uint32_t slot=point.selector==UINT32_MAX?draw.immediateData.currentPnMtx:point.selector/3;
    if(slot>=20){fail("palette-slot");return;}
    if(source.paletteMask&&(slot>=10||!(source.paletteMask&(1u<<slot)))){fail("original-palette-slot");return;}
    if(source.normalComponents){
      if(!(source.normalMask&(1u<<slot))){fail("original-normal-slot");return;}
      auto* directions=snapshot.expectedDirections.data()+n*16;
      const auto* normal=source.normalPalette[slot];
      for(unsigned slice=0;slice<source.normalComponents/3;slice++)for(unsigned row=0;row<3;row++){
        double value=0;for(unsigned c=0;c<3;c++)value+=normal[row*3+c]*point.directions[slice*3+c];
        if(!std::isfinite(value)){fail("original-normal-nonfinite");return;}
        directions[(slice==0?0:slice+1)*4+row]=value;
      }
      const double length2=directions[0]*directions[0]+directions[1]*directions[1]+directions[2]*directions[2];
      for(unsigned row=0;row<3;row++)directions[4+row]=length2>1e-10?directions[row]/std::sqrt(length2):directions[row];
    }
    snapshot.selectorMask|=1u<<slot;
    float model[12];std::memcpy(model,frame.uniforms.data()+base+80+slot*48,48);
    snapshot.matrixHash=fingerprint(model,sizeof(model),snapshot.matrixHash);
    double eye[4]{0,0,0,1};
    for(unsigned row=0;row<3;row++){
      if(source.paletteMask){const auto* reference=source.positionPalette[slot];eye[row]=reference[row*4+3]+point.position[0]*reference[row*4]+point.position[1]*reference[row*4+1]+point.position[2]*reference[row*4+2];}
      else eye[row]=double(model[row*4+3])+point.position[0]*model[row*4]+point.position[1]*model[row*4+1]+point.position[2]*model[row*4+2];
    }
    for(unsigned row=0;row<4;row++){
      double clip=0;for(unsigned k=0;k<4;k++)clip+=eye[k]*projection[row*4+k];
      if(!std::isfinite(clip)){fail("nonfinite-reference");return;}
      snapshot.expected[n*4+row]=clip;
    }
    for(unsigned channel=0;channel<2;channel++)if(snapshot.colorMask&(1u<<channel)){
      if(!lighting_reference(source,point,channel,eye,snapshot.expectedDirections.data()+n*16+4,snapshot.expectedColors.data()+n*8+channel*4,(snapshot.colorLanes>>(channel*4))&15)){
        fail("hsd-light-reference");return;
      }
    }
  }
  snapshot.expectedHash=fingerprint(snapshot.expected.data(),snapshot.points*4*sizeof(double));
  snapshots.push_back(std::move(snapshot));
}
}

void push_draw_command(gx::DrawData data, const std::array<uint32_t, 16>& diagnosticState) {

  if (data.diagnosticShader && g_recorder.currentRenderPass != UINT32_MAX) {
    auto& draws=current_frame_packet().renderPasses[g_recorder.currentRenderPass].diagnosticReplayDraws;
    bool selected=true;
    if(std::getenv("AURORA_DIAGNOSTIC_CORNERIA_POSITION_BATCH")) {
      unsigned sameShader=0;
      for(const auto& prior:draws) if(prior.draw.diagnosticShader==data.diagnosticShader) ++sameShader;
      selected=corneria_position_profiles::find(data.diagnosticShader)!=nullptr &&
               data.vtxCount>=8 && sameShader<4;
    }
    if(std::getenv("AURORA_DIAGNOSTIC_MUTECITY_POSITION_BATCH")) {
      unsigned sameShader=0;
      for(const auto& prior:draws) if(prior.draw.diagnosticShader==data.diagnosticShader) ++sameShader;
      selected=mutecity_position_profiles::find(data.diagnosticShader)!=nullptr &&
               data.vtxCount>=1 && sameShader<4;
    }
    if(const char* target=std::getenv("AURORA_DIAGNOSTIC_REPLAY_SHADER")) {
      char* end=nullptr;
      const auto shader=std::strtoull(target,&end,10);
      selected=end!=target && *end==0 && shader==data.diagnosticShader;
    }
    // Filter diagnostic snapshots before the existing bound, so late stage
    // draws can be investigated without increasing capture memory or changing
    // ordinary draw recording.
    if(selected && draws.size()<32) {
      DiagnosticReplayDraw snapshot{.draw=data, .state=diagnosticState};
      const auto fingerprint=[](const uint8_t* bytes,size_t length) {
        uint64_t hash=14695981039346656037ull;
        for(size_t i=0;i<length;++i)hash=(hash ^ bytes[i])*1099511628211ull;
        return hash;
      };
      const auto& uniforms=current_frame_packet().uniforms;
      if(data.uniformRange.size && data.uniformRange.offset<=uniforms.size() &&
         data.uniformRange.size<=uniforms.size()-data.uniformRange.offset) {
        snapshot.uniformBytes=data.uniformRange.size;
        snapshot.uniformHash=fingerprint(uniforms.data()+data.uniformRange.offset,data.uniformRange.size);
      }
      const auto& vertices=current_frame_packet().verts;
      if(data.vertRange.size && data.vertRange.offset<=vertices.size() &&
         data.vertRange.size<=vertices.size()-data.vertRange.offset) {
        snapshot.vertexStreamHash=14695981039346656037ull;
        snapshot.vertexStreamBytes=data.vertRange.size;
        for(size_t i=0;i<data.vertRange.size;++i)
          snapshot.vertexStreamHash=(snapshot.vertexStreamHash ^ vertices.data()[data.vertRange.offset+i])*1099511628211ull;
      }
      if(std::getenv("AURORA_DIAGNOSTIC_TEXTURE_SAMPLE_ORACLE")) {
        const auto& texture=gx::get_texture(GX_TEXMAP0);
        if(texture.ref) {
          snapshot.pixels=texture.ref->diagnosticBaseMip;
          if(snapshot.pixels && !snapshot.pixels->empty())
            snapshot.textureHash=fingerprint(snapshot.pixels->data(),snapshot.pixels->size());
          snapshot.width=texture.ref->size.width;
          snapshot.height=texture.ref->size.height;
          snapshot.mipCount=texture.ref->mipCount;
          snapshot.sampler=texture.get_descriptor();
        }
      }
      draws.push_back(std::move(snapshot));
    }
  }
  push_draw_command(make_draw_command<gx::render>(data));
}

template <>
void push_draw_command(gx::DrawData data) {
  push_draw_command(data, {});
}

#ifdef AURORA_ENABLE_RMLUI
template <>
void push_draw_command(rmlui::DrawData data) {
  push_draw_command(make_draw_command<rmlui::render>(data));
}
#endif

template <>
PipelineRef pipeline_ref(const gx::PipelineConfig& config) {
  const auto context=gx::capture_pipeline_context();
  const auto ref=find_pipeline(ShaderType::GX, config, [=] { return gx::create_pipeline_with_context(config,context); });
  if(std::getenv("AURORA_DIAGNOSTIC_PREPARE_GX_PASSES") &&
     std::find(g_recorder.preparingGxPipelines.begin(),g_recorder.preparingGxPipelines.end(),ref)==g_recorder.preparingGxPipelines.end()) {
    AURORA_ASSERT(g_recorder.preparingGxPipelines.size()<4096,"GX pass preparation exceeds bounded pipeline inventory");
    g_recorder.preparingGxPipelines.push_back(ref);
  }
  return ref;
}

#ifdef AURORA_ENABLE_RMLUI
template <>
PipelineRef pipeline_ref(const rmlui::PipelineConfig& config) {
  return find_pipeline(ShaderType::Rml, config, [=] { return rmlui::create_pipeline(config); });
}
#endif

void finish() {
  ZoneScoped;
  if (!g_recorder.active()) {
    return;
  }
  AURORA_ASSERT(!g_recorder.inOffscreen, "finish called while offscreen rendering is active");
  if (g_recorder.currentRenderPass != UINT32_MAX) {
    auto& frame = current_frame_packet();
    frame.uniforms.append_zeroes(gx::MaxUniformSize);
    auto& pass = frame.renderPasses[g_recorder.currentRenderPass];
    pass.captureDepthSnapshot = true;
    enqueue_pass(frame, g_recorder.currentRenderPass);
    g_recorder.currentRenderPass = UINT32_MAX;
  }
}

Range push_verts(const uint8_t* data, size_t length, size_t alignment) {
  ZoneScoped;
  if (!check_recording("push_verts")) {
    return {};
  }
  return push(current_frame_packet().verts, data, length, alignment);
}

Range push_indices(const uint8_t* data, size_t length, size_t alignment) {
  ZoneScoped;
  if (!check_recording("push_indices")) {
    return {};
  }
  return push(current_frame_packet().indices, data, length, alignment);
}

Range push_uniform(const uint8_t* data, size_t length) {
  ZoneScoped;
  if (!check_recording("push_uniform")) {
    return {};
  }
  if (diagnostic_uniform_dedup_enabled() && data && length != 0) {
    auto& bytes = current_frame_packet().uniforms;
    const auto hash = detail::uniform_range_hash(data, length);
    auto& candidates = g_recorder.uniformRangesByHash[hash];
    for (const auto range : candidates) {
      if (detail::uniform_range_matches(range, data, length, bytes.data(), bytes.size())) {
        report_uniform_dedup_profile(length, true);
        return range;
      }
    }
    const auto range = push(current_frame_packet().uniforms, data, length,
                            resources().limits.minUniformBufferOffsetAlignment);
    candidates.push_back(range);
    report_uniform_dedup_profile(length, false);
    return range;
  }
  return push(current_frame_packet().uniforms, data, length, resources().limits.minUniformBufferOffsetAlignment);
}

bool diagnostic_storage_range_matches(Range range, const uint8_t* source, size_t length) {
  const auto& storage = current_frame_packet().storage;
  if (!source || length > range.size || range.offset > storage.size() ||
      length > storage.size() - range.offset) {
    return false;
  }
  return std::memcmp(storage.data() + range.offset, source, length) == 0;
}

Range push_storage(const uint8_t* data, size_t length) {
  ZoneScoped;
  if (!check_recording("push_storage")) {
    return {};
  }
  return push(current_frame_packet().storage, data, length, resources().limits.minStorageBufferOffsetAlignment);
}

Range push_texture_data(const uint8_t* data, u32 bytesPerRow, u32 rowsPerImage) {
  // For CopyBufferToTexture, we need an alignment of 256 per row (see Dawn kTextureBytesPerRowAlignment)
  const auto copyBytesPerRow = AURORA_ALIGN(bytesPerRow, 256);
  const auto range = map(current_frame_packet().textureUpload, copyBytesPerRow * rowsPerImage, 0);
  u8* dst = current_frame_packet().textureUpload.data() + range.offset;
  for (u32 i = 0; i < rowsPerImage; ++i) {
    memcpy(dst, data, bytesPerRow);
    data += bytesPerRow;
    dst += copyBytesPerRow;
  }
  return range;
}

uint32_t align_uniform(uint32_t value) {
  return AURORA_ALIGN(value, detail::resources().limits.minUniformBufferOffsetAlignment);
}

void insert_debug_marker(std::string label) {
#if defined(AURORA_GFX_DEBUG_GROUPS)
  auto& markers = current_frame_packet().debugMarkers;
  const auto idx = markers.size();
  markers.emplace_back(std::move(label));
  push_command(CommandType::DebugMarker, {.debugMarkerIndex = idx});
#endif
}

void push_debug_group(std::string label) {
#if defined(AURORA_GFX_DEBUG_GROUPS)
  g_recorder.debugGroupStack.push_back(std::move(label));
#endif
}
} // namespace aurora::gfx

void push_debug_group(const char* label) {
#ifdef AURORA_GFX_DEBUG_GROUPS
  aurora::gfx::g_recorder.debugGroupStack.emplace_back(label);
#endif
}
void pop_debug_group() {
#ifdef AURORA_GFX_DEBUG_GROUPS
  if (aurora::gfx::g_recorder.debugGroupStack.empty()) {
    aurora::gfx::Log.error("Debug group stack underflowed!");
    return;
  }

  aurora::gfx::g_recorder.debugGroupStack.pop_back();
#endif
}
