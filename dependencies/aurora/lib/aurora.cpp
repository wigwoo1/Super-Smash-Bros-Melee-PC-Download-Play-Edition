#include <aurora/aurora.h>
#include <aurora/diagnostics.h>
#include "owner_service.hpp"
#include "graphics_quality_scope.hpp"
#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <array>
#include <mutex>

#ifdef AURORA_ENABLE_GX
#include "gfx/resources.hpp"
#include "gfx/preparation_progress.hpp"
#include "gfx/frame.hpp"
#include "gfx/recording.hpp"
#include "gfx/render_snapshot.hpp"
#include "gfx/diagnostic_capture.hpp"
#include "gfx/render_worker.hpp"
#include "gx/command_processor.hpp"
#include "gx/fifo.hpp"
#include <dolphin/gx/GXCommandList.h>
#include <dolphin/gx/GXAurora.h>
#include "gx/gx.hpp"
#include "gx/draw_done.hpp"
#include "gx/xfb.hpp"
#include "dolphin/vi/vi_internal.hpp"
#include "gx/texture.hpp"
#include <aurora/texture.hpp>
#include "io.hpp"
#include "imgui.hpp"
#include "webgpu/gpu.hpp"
#include "webgpu/gpu_prof.hpp"
#include <webgpu/webgpu_cpp.h>
#endif

#ifdef AURORA_ENABLE_RMLUI
#include "rmlui.hpp"
#endif

#include "input.hpp"
#include "internal.hpp"
#include "thread.hpp"
#include "window.hpp"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <magic_enum.hpp>

#include "system_info.hpp"
#include "tracy/Tracy.hpp"

namespace {
std::atomic<SDL_ThreadID> diagnosticOwner{0};
aurora::OwnerService ownerService;
void (*hostExitHandler)(void) = nullptr;
bool diagnosticUiPrepared = false;
bool diagnosticFramesStarted = false;
bool diagnostic_on_owner() {
  const auto owner = diagnosticOwner.load();
  return owner && owner == SDL_GetCurrentThreadID();
}
std::atomic<uint32_t> texturePackRequested{0}, texturePackApplied{0};
std::atomic<bool> hdTexturePackAvailable{false};
#ifdef AURORA_ENABLE_GX
std::filesystem::path hdTexturePackPath;
aurora::texture::ReplacementGroup hdTexturePackGroup;

void service_texture_pack() {
  if (!diagnostic_on_owner() || aurora::gfx::detail::diagnostic_recording_id()) return;
  const auto requested = texturePackRequested.load();
  if (requested == texturePackApplied.load()) return;
  if (requested == 0) {
    aurora::texture::unregister_replacements(hdTexturePackGroup);
    hdTexturePackGroup.registrations.clear();
    texturePackApplied.store(0);
  } else {
    auto group = aurora::texture::load_replacement_directory(hdTexturePackPath);
    if (group.registrations.empty()) {
      hdTexturePackAvailable.store(false);
      texturePackRequested.store(0);
    } else {
      hdTexturePackGroup = std::move(group);
      texturePackApplied.store(1);
    }
  }
  std::fprintf(stderr, "NATIVE_TEXTURE_PACK applied=%u registrations=%zu\n",
               texturePackApplied.load(), hdTexturePackGroup.registrations.size());
}
uint32_t snapshot_ack_delay_ms() {
  static const uint32_t delay=[] {
    const char* value=std::getenv("AURORA_DIAGNOSTIC_SNAPSHOT_ACK_DELAY_MS");
    if(!value || !*value)return 0u;
    char* end=nullptr;const auto parsed=std::strtoul(value,&end,10);
    return end && *end=='\0' && parsed<=100 ? uint32_t(parsed) : 0u;
  }();
  return delay;
}
uint64_t snapshot_trace_limit() {
  static const uint64_t limit=[] {
    const char* value=std::getenv("AURORA_DIAGNOSTIC_SNAPSHOT_TRACE_LIMIT");
    if(!value || !*value)return uint64_t(64);
    char* end=nullptr;const auto parsed=std::strtoul(value,&end,10);
    return end && *end=='\0' && parsed>=64 && parsed<=1024 ? uint64_t(parsed) : uint64_t(64);
  }();
  return limit;
}
#endif
std::atomic<uint64_t> diagnosticGeneration{1};
struct DiagnosticAck {uint64_t generation,id;uint32_t kind,status;};
std::array<DiagnosticAck,256> diagnosticAcks;std::mutex diagnosticAckMutex;size_t diagnosticAckHead=0,diagnosticAckCount=0;uint64_t diagnosticAckDrops=0;
bool diagnosticAckFailed = false;
void diagnostic_ack(uint64_t generation,uint64_t id,uint32_t kind,uint32_t status){std::lock_guard lock(diagnosticAckMutex);diagnosticAckFailed |= status != 0;if(diagnosticAckCount==diagnosticAcks.size()){diagnosticAckDrops++;return;}diagnosticAcks[(diagnosticAckHead+diagnosticAckCount++)%diagnosticAcks.size()]={generation,id,kind,status};}
std::atomic<uint64_t> diagnosticSubmitted{0}, diagnosticCompleted{0}, diagnosticFailed{0}, diagnosticPresented{0};
}
extern "C" int aurora_set_texture_pack(uint32_t pack) {
  if (!diagnostic_on_owner() || pack > 1 || (pack == 1 && !hdTexturePackAvailable.load())) return 0;
  texturePackRequested.store(pack);
  return 1;
}
extern "C" uint32_t aurora_get_texture_pack(void) { return texturePackApplied.load(); }
extern "C" int aurora_is_hd_texture_pack_available(void) { return hdTexturePackAvailable.load() ? 1 : 0; }
extern "C" int aurora_is_texture_pack_pending(void) { return texturePackRequested.load() != texturePackApplied.load(); }
extern "C" void aurora_diagnostic_gpu_counts(uint64_t* submitted,uint64_t* completed,uint64_t* failed,uint64_t* presented) {
  if(submitted)*submitted=diagnosticSubmitted.load(); if(completed)*completed=diagnosticCompleted.load();
  if(failed)*failed=diagnosticFailed.load(); if(presented)*presented=diagnosticPresented.load();
}
extern "C" int aurora_diagnostic_is_owner(void) { return diagnostic_on_owner(); }
extern "C" int aurora_set_owner_service(int (*provider)(void)) {return ownerService.install(diagnostic_on_owner(),provider);}
extern "C" int aurora_service_owner(void) {
  constexpr aurora::Module Log{"aurora::preparation"};
  if(!diagnostic_on_owner()) return 0;
#ifdef AURORA_ENABLE_GX
  if(std::getenv("AURORA_DIAGNOSTIC_PREPARE_GX_PASSES")) {
    SDL_PumpEvents(); // Leave queued input intact for normal event delivery.
    static bool showing=false; static std::string originalTitle;
    static uint64_t lastUpdate=0; static unsigned samples=0,restoreSamples=0;
    const auto progress=aurora::gfx::preparation_progress::snapshot();
    static bool preparationCloseQueued=false;
    if(!preparationCloseQueued && progress.active && std::getenv("AURORA_DIAGNOSTIC_CLOSE_DURING_PREPARATION")) {
      uint32_t foreground=0,background=0,compiling=0,failed=0;
      if(aurora_diagnostic_pipeline_work(&foreground,&background,&compiling,&failed) && compiling>0) {
        SDL_Event event{};event.type=SDL_EVENT_QUIT;
        AURORA_ASSERT(SDL_PushEvent(&event),"Preparation close event could not be queued");
        preparationCloseQueued=true;
        std::fprintf(stderr,"NATIVE_PREPARATION_CLOSE_QUEUED foreground=%u background=%u compiling=%u failed=%u owner=1\n",foreground,background,compiling,failed);
      }
    }
    auto* window=aurora::window::get_sdl_window();
    const uint64_t now=SDL_GetTicksNS();
    if(window && progress.active && (!showing || now-lastUpdate>=50000000ULL)) {
      if(!showing) originalTitle=SDL_GetWindowTitle(window);
      showing=true;lastUpdate=now;
      const auto title=std::string("Preparing graphics: ")+std::to_string(progress.done)+"/"+std::to_string(progress.total);
      aurora::window::set_title(title.c_str());
      const bool verified=title==SDL_GetWindowTitle(window);
      if(samples++<128) std::fprintf(stderr,"NATIVE_PREPARATION_UI active=1 done=%u total=%u owner=1 events_pumped=1 title_verified=%u\n",progress.done,progress.total,unsigned(verified));
    } else if(window && showing && !progress.active) {
      aurora::window::set_title(originalTitle.c_str());showing=false;
      if(restoreSamples++<128) std::fprintf(stderr,"NATIVE_PREPARATION_UI active=0 owner=1 title_restored=%u\n",unsigned(originalTitle==SDL_GetWindowTitle(window)));
    }
  }
#endif
  return ownerService.pump(true);
}
extern "C" int aurora_set_exit_handler(void (*handler)(void)) {
  if (!diagnostic_on_owner() || !handler || hostExitHandler) return 0;
  hostExitHandler=handler; return 1;
}
extern "C" int aurora_set_controls_available(const bool available) {
  if (!diagnostic_on_owner()) return 0;
  aurora::imgui::set_controls_available(available);
  return 1;
}

extern "C" int aurora_set_fps_counter_enabled(int enabled) {
  if (!diagnostic_on_owner() || (enabled != 0 && enabled != 1)) return 0;
  aurora::imgui::set_fps_counter_enabled(enabled != 0);
  return 1;
}
extern "C" int aurora_get_fps_counter_enabled(void) {
  return aurora::imgui::fps_counter_enabled() ? 1 : 0;
}

extern "C" int aurora_set_render_height(const uint32_t height) {
  if (!diagnostic_on_owner()) return 0;
  return aurora::window::try_set_render_height(height) ? 1 : 0;
}
extern "C" uint32_t aurora_get_render_height(void) {
  return aurora::window::render_height();
}
static std::atomic<bool> fighterShadows{true};
static std::atomic<uint32_t> fighterShadowResolution{256};
static std::atomic<uint32_t> textureQuality{100};
static std::atomic<bool> lightingEnabled{true};
static std::atomic<bool> lowPolyFighters{false};
static aurora::graphics_quality::SceneScope graphicsSceneScope;
static thread_local uint32_t hudGraphicsQueuedDepth=0;
namespace aurora::graphics_quality {
FrameQuality scene_snapshot() noexcept {
  return graphicsSceneScope.snapshot({textureQuality.load(),lightingEnabled.load()});
}
}
static aurora::graphics_quality::FrameQuality effective_graphics_quality() {
#ifdef AURORA_ENABLE_GX
  if(const auto* frame=aurora::gfx::detail::recording_graphics_quality())return *frame;
#endif
  return aurora::graphics_quality::scene_snapshot();
}
extern "C" int aurora_set_gameplay_graphics(int gameplay) {
  bool idle=true;
#ifdef AURORA_ENABLE_GX
  if(diagnostic_on_owner())idle=!aurora::gfx::detail::diagnostic_recording_id() && !aurora::gx::fifo::in_display_list();
#endif
  return graphicsSceneScope.set(diagnostic_on_owner(),idle,gameplay)?1:0;
}
extern "C" int aurora_push_hud_graphics(void) {
#ifdef AURORA_ENABLE_GX
  if(!diagnostic_on_owner() || aurora::gx::fifo::in_display_list() ||
     !aurora::gfx::detail::diagnostic_recording_id() || hudGraphicsQueuedDepth==8)return 0;
  ++hudGraphicsQueuedDepth;
  aurora::gx::fifo::write_u8(GX_AURORA);
  aurora::gx::fifo::write_u16(GX_AURORA_HUD_GRAPHICS_PUSH);
  aurora::gx::fifo::publish();return 1;
#else
  return 0;
#endif
}
extern "C" int aurora_pop_hud_graphics(void) {
#ifdef AURORA_ENABLE_GX
  if(!diagnostic_on_owner() || aurora::gx::fifo::in_display_list() ||
     !aurora::gfx::detail::diagnostic_recording_id() || hudGraphicsQueuedDepth==0)return 0;
  --hudGraphicsQueuedDepth;
  aurora::gx::fifo::write_u8(GX_AURORA);
  aurora::gx::fifo::write_u16(GX_AURORA_HUD_GRAPHICS_POP);
  aurora::gx::fifo::publish();return 1;
#else
  return 0;
#endif
}
extern "C" uint32_t aurora_get_effective_texture_quality(void) {
  return effective_graphics_quality().texturePercent;
}
extern "C" int aurora_get_effective_lighting_enabled(void) {
  return effective_graphics_quality().lightingEnabled?1:0;
}
extern "C" int aurora_set_low_poly_fighters(int enabled) {
  if (!diagnostic_on_owner() || (enabled!=0 && enabled!=1)) return 0;
  lowPolyFighters.store(enabled!=0);return 1;
}
extern "C" int aurora_get_low_poly_fighters(void) { return lowPolyFighters.load()?1:0; }
extern "C" int aurora_set_lighting_enabled(int enabled) {
  if (!diagnostic_on_owner() || (enabled!=0 && enabled!=1)) return 0;
  lightingEnabled.store(enabled!=0);return 1;
}
extern "C" int aurora_get_lighting_enabled(void) { return lightingEnabled.load()?1:0; }
extern "C" int aurora_set_texture_quality(const uint32_t percent) {
  if (!diagnostic_on_owner() || (percent != 100 && percent != 50 && percent != 25)) return 0;
  textureQuality.store(percent); return 1;
}
extern "C" uint32_t aurora_get_texture_quality(void) { return textureQuality.load(); }
extern "C" int aurora_set_fighter_shadow_resolution(const uint32_t size) {
  if (!diagnostic_on_owner() || (size!=64 && size!=128 && size!=256)) return 0;
  fighterShadowResolution.store(size);return 1;
}
extern "C" uint32_t aurora_get_fighter_shadow_resolution(void) {
  return fighterShadowResolution.load();
}
extern "C" int aurora_set_fighter_shadows(const int enabled) {
  if (!diagnostic_on_owner() || (enabled!=0 && enabled!=1)) return 0;
  fighterShadows.store(enabled);
  return 1;
}
extern "C" int aurora_get_fighter_shadows(void) {
  return fighterShadows.load() ? 1 : 0;
}

extern "C" int aurora_diagnostic_render_target(uint32_t* width,uint32_t* height,int* offscreen) {
#ifdef AURORA_ENABLE_GX
  if (!diagnostic_on_owner() || !width || !height || !offscreen ||
      !aurora::gfx::detail::diagnostic_recording_id()) return 0;
  /* Camera code asks before writing its projection. Consume earlier target
   * transitions so this cannot report the previous shadow/copy pass. Never
   * drain while authoring a display list. */
  if (aurora::gx::fifo::in_display_list()) return 0;
  aurora::gx::fifo::drain();
  const auto size=aurora::gfx::get_render_target_size();
  if (!size.x || !size.y) return 0;
  *width=size.x; *height=size.y; *offscreen=aurora::gfx::is_offscreen()?1:0;
  return 1;
#else
  return 0;
#endif
}

extern "C" float aurora_get_render_aspect(void) {
  if (!diagnostic_on_owner() || !aurora::window::get_sdl_window()) return 0.f;
  const auto size=aurora::window::get_window_size();
  return size.fb_height ? float(size.fb_width)/size.fb_height : 0.f;
}
extern "C" int aurora_set_display_preset(uint32_t preset) {
  if (!diagnostic_on_owner() || preset>3) return 0;
  return aurora::window::request_display_preset(preset) ? 1 : 0;
}
extern "C" uint32_t aurora_get_display_preset(void) {
  return aurora::window::display_preset();
}
extern "C" int aurora_has_pending_display_preset(void) {
  return aurora::window::has_pending_display_preset() ? 1 : 0;
}
extern "C" int aurora_apply_display_preset_at_boundary(void) {
  if (!diagnostic_on_owner()) return 0;
  return aurora::window::apply_pending_display_preset_at_boundary() ? 1 : 0;
}
extern "C" int aurora_get_widescreen_enabled(void) {
  return aurora::window::display_preset()!=0;
}

extern "C" void aurora_handle_exit(void) {
  constexpr aurora::Module Log{"aurora::exit"};
  AURORA_ASSERT(diagnostic_on_owner() && hostExitHandler,"Exit requires installed owner handler");
  hostExitHandler();
  AURORA_ASSERT(false,"Terminal exit handler returned");
}
extern "C" int aurora_set_gx_callback_gate(int (*provider)(void)) {
#ifdef AURORA_ENABLE_GX
  return aurora::gx::draw_done::install_gate(provider);
#else
  (void)provider;return 0;
#endif
}
#ifdef AURORA_ENABLE_GX
bool aurora::gx::draw_done::wait(uint32_t timeoutMs) {
  if(!on_owner() || timeoutMs>2000 || !webgpu::g_instance || !webgpu::g_device)return false;
  const uint64_t activeFrame=gfx::detail::diagnostic_recording_id(),start=SDL_GetTicksNS();
  const int initialPending=pending(activeFrame);
  unsigned pollsAccepted=0,pollsRejected=0;
  for(;;){
    const int status=pending(activeFrame);
    if(status<0 || (status>0 && !delivery_enabled()) || !xfb::pump() || !pump())return false;
    if(pending(activeFrame)==0){
      if(std::getenv("AURORA_DIAGNOSTIC_DRAW_DONE_WAIT_TRACE")) std::fprintf(stderr,"DRAW_DONE_WAIT initial_pending=%d polls_accepted=%u polls_rejected=%u elapsed_ns=%llu drained=1\n",initialPending,pollsAccepted,pollsRejected,static_cast<unsigned long long>(SDL_GetTicksNS()-start));
      return true;
    }
    if(SDL_GetTicksNS()-start>=uint64_t(timeoutMs)*1000000)return false;
    // A full render queue must not hide the completion wait's timeout.
    // Rejected event polls are retried on the next owner iteration.
    if(gfx::render_worker::try_enqueue_work([]{webgpu::g_instance.ProcessEvents();}))++pollsAccepted;
    else ++pollsRejected;
    SDL_DelayNS(100000);
  }
}
#endif
extern "C" int aurora_register_xfb(void* destination,size_t bytes,uint32_t width,uint32_t height) {
#ifdef AURORA_ENABLE_GX
  return aurora::gx::xfb::register_buffer(destination,bytes,width,height);
#else
  return 0;
#endif
}
extern "C" int aurora_diagnostic_queue_quit(void) {
  if(!aurora_diagnostic_is_owner())return 0;
  SDL_Event event{};event.type=SDL_EVENT_QUIT;
  return SDL_PushEvent(&event)?1:0;
}
extern "C" int aurora_diagnostic_queue_window_close(void) {
  if(!aurora_diagnostic_is_owner())return 0;
  SDL_Event event{};event.type=SDL_EVENT_WINDOW_CLOSE_REQUESTED;
  event.window.windowID=SDL_GetWindowID(aurora::window::get_sdl_window());
  if(!event.window.windowID)return 0;
  return SDL_PushEvent(&event)?1:0;
}
extern "C" int aurora_unregister_xfb(void* destination) {
#ifdef AURORA_ENABLE_GX
  return aurora::gx::xfb::unregister_buffer(destination);
#else
  return 0;
#endif
}
extern "C" int aurora_xfb_revision(void* destination,uint32_t* revision) {
#ifdef AURORA_ENABLE_GX
  return aurora::gx::xfb::revision(destination,revision);
#else
  return 0;
#endif
}
extern "C" int aurora_diagnostic_prepare_ui(void) {
  if (!diagnostic_on_owner() || diagnosticUiPrepared || diagnosticFramesStarted || aurora::gfx::detail::diagnostic_recording_id() ||
      !aurora::window::is_presentable() || aurora::window::is_paused()) return 0;
  aurora::imgui::new_frame(aurora::window::get_window_size());
  (void)aurora::imgui::freeze();
  diagnosticUiPrepared = true;
  return 1;
}
extern "C" uint64_t aurora_diagnostic_recording_id(void) {
  return diagnostic_on_owner() ? aurora::gfx::detail::diagnostic_recording_id() : 0;
}
extern "C" int aurora_diagnostic_gpu_ack(uint64_t* generation,uint64_t* id,uint32_t* kind,uint32_t* status){if(!diagnostic_on_owner()||!generation||!id||!kind||!status)return 0;std::lock_guard lock(diagnosticAckMutex);if(!diagnosticAckCount)return 0;auto ack=diagnosticAcks[diagnosticAckHead];diagnosticAckHead=(diagnosticAckHead+1)%diagnosticAcks.size();diagnosticAckCount--;*generation=ack.generation;*id=ack.id;*kind=ack.kind;*status=ack.status;return 1;}
extern "C" void aurora_diagnostic_gpu_poll(){
if(!diagnostic_on_owner())return;
#ifdef AURORA_ENABLE_GX
aurora::gfx::render_worker::enqueue_work([]{if(aurora::webgpu::g_instance)aurora::webgpu::g_instance.ProcessEvents();});
#endif
}
extern "C" uint64_t aurora_diagnostic_gpu_ack_drops(){std::lock_guard lock(diagnosticAckMutex);return diagnosticAckDrops;}
extern "C" int aurora_diagnostic_gpu_drain(uint32_t timeoutMs) {
#ifdef AURORA_ENABLE_GX
  if (!diagnostic_on_owner() || !std::getenv("AURORA_DIAGNOSTIC_GPU_COMPLETION") || timeoutMs > 2000 ||
      !aurora::webgpu::g_instance || !aurora::webgpu::g_device) return 0;
  const uint64_t start = SDL_GetTicksNS();
  for (;;) {
    if (diagnosticFailed.load() || aurora_diagnostic_gpu_ack_drops()) return 0;
    if (aurora::gfx::diagnostic_quiescent()) {
      if (diagnosticCompleted.load() == diagnosticSubmitted.load()) return 1;
      if (SDL_GetTicksNS() - start >= uint64_t(timeoutMs) * 1000000) return 0;
      aurora_diagnostic_gpu_poll();
    }
    if (SDL_GetTicksNS() - start >= uint64_t(timeoutMs) * 1000000) return 0;
    SDL_Delay(1);
  }
#else
  return 0;
#endif
}
extern "C" int aurora_diagnostic_gpu_reset(uint64_t generation){
if(!diagnostic_on_owner())return 0;
#ifdef AURORA_ENABLE_GX
if(!aurora::gfx::diagnostic_quiescent())return 0;
#endif
std::lock_guard lock(diagnosticAckMutex);if(generation<=diagnosticGeneration.load() || diagnosticAckCount || diagnosticAckDrops || diagnosticAckFailed || diagnosticFailed.load() || diagnosticCompleted.load()!=diagnosticSubmitted.load())return 0;diagnosticSubmitted.store(0);diagnosticCompleted.store(0);diagnosticPresented.store(0);diagnosticGeneration.store(generation);return 1;}
namespace aurora {
AuroraConfig g_config;
uint32_t g_sdlCustomEventsStart;
char g_gameName[4];

namespace {
constexpr Module Log{"aurora"};

#ifdef AURORA_ENABLE_GX
// GPU
using webgpu::g_device;
using webgpu::g_queue;
using webgpu::g_surface;

uint32_t clamp_scissor_coord(double value, uint32_t maximum) noexcept {
  if (!std::isfinite(value)) {
    return 0;
  }
  return static_cast<uint32_t>(std::clamp(value, 0.0, static_cast<double>(maximum)));
}

void set_present_viewport(const wgpu::RenderPassEncoder& pass, const gfx::Viewport& viewport, uint32_t surfaceWidth,
                          uint32_t surfaceHeight) noexcept {
  pass.SetViewport(viewport.left, viewport.top, viewport.width, viewport.height, viewport.znear, viewport.zfar);
  const auto scissorX = clamp_scissor_coord(std::floor(viewport.left), surfaceWidth);
  const auto scissorY = clamp_scissor_coord(std::floor(viewport.top), surfaceHeight);
  const auto scissorRight = clamp_scissor_coord(std::ceil(viewport.left + viewport.width), surfaceWidth);
  const auto scissorBottom = clamp_scissor_coord(std::ceil(viewport.top + viewport.height), surfaceHeight);
  pass.SetScissorRect(scissorX, scissorY, scissorRight - scissorX, scissorBottom - scissorY);
}
#endif

#ifdef AURORA_ENABLE_GX
constexpr std::array PreferredBackendOrder{
#ifdef ENABLE_BACKEND_WEBGPU
    BACKEND_WEBGPU,
#endif
#ifdef DAWN_ENABLE_BACKEND_D3D12
    BACKEND_D3D12,
#endif
#ifdef DAWN_ENABLE_BACKEND_METAL
    BACKEND_METAL,
#endif
#ifdef DAWN_ENABLE_BACKEND_VULKAN
    BACKEND_VULKAN,
#endif
#ifdef DAWN_ENABLE_BACKEND_D3D11
    BACKEND_D3D11,
#endif
// #ifdef DAWN_ENABLE_BACKEND_DESKTOP_GL
//     BACKEND_OPENGL,
// #endif
#ifdef DAWN_ENABLE_BACKEND_OPENGLES
    BACKEND_OPENGLES,
#endif
#ifdef DAWN_ENABLE_BACKEND_NULL
    BACKEND_NULL,
#endif
};
#else
constexpr std::array<AuroraBackend, 0> PreferredBackendOrder{};
#endif

bool g_initialFrame = false;

AuroraInfo initialize(int argc, char* argv[], const AuroraConfig& config) noexcept {
  diagnosticUiPrepared = false;
  diagnosticFramesStarted = false;
  diagnosticOwner.store(SDL_GetCurrentThreadID());
  window::reset_display_preset();
  fighterShadows.store(true);
  fighterShadowResolution.store(256);
  textureQuality.store(100);
  graphicsSceneScope.reset();
  hudGraphicsQueuedDepth=0;
  lightingEnabled.store(true);
  lowPolyFighters.store(false);
#ifdef AURORA_ENABLE_GX
  gx::draw_done::initialize();
  if(!gx::xfb::initialize())std::abort();
  if(!gfx::render_snapshot::initialize())std::abort();
  gx::draw_done::readyProvider=gx::xfb::frame_ready;
#endif
  g_config = config;
  Log.info("Aurora initializing");
  log_system_information();
  if (g_config.appName == nullptr) {
    g_config.appName = "Aurora";
  } else {
    g_config.appName = strdup(g_config.appName);
  }
  if (g_config.sessionOnlyPreferences) {
    g_config.userPath = nullptr;
  } else if (g_config.userPath == nullptr) {
    g_config.userPath = SDL_GetPrefPath(nullptr, g_config.appName);
    if (g_config.userPath == nullptr) {
      Log.fatal("Unable to initialize user path: {}", SDL_GetError());
      std::abort();
    }
  } else {
    g_config.userPath = strdup(g_config.userPath);
  }
  if (g_config.cachePath == nullptr) {
    g_config.cachePath = SDL_GetPrefPath(nullptr, g_config.appName);
    if (g_config.cachePath == nullptr) {
      Log.fatal("Unable to initialize cache path: {}", SDL_GetError());
      std::abort();
    }
  } else {
    g_config.cachePath = strdup(g_config.cachePath);
  }
  if (g_config.resourcesPath == nullptr) {
    g_config.resourcesPath = SDL_GetBasePath();
  } else {
    g_config.resourcesPath = strdup(g_config.resourcesPath);
  }
  if (!g_config.appName || (!g_config.sessionOnlyPreferences && !g_config.userPath) || !g_config.cachePath || !g_config.resourcesPath) {
    Log.fatal("Unable to allocate required runtime paths");
    std::abort();
  }
  if (g_config.msaa == 0) {
    g_config.msaa = 1;
  }
  if (g_config.maxTextureAnisotropy == 0) {
    g_config.maxTextureAnisotropy = 16;
  }
  AURORA_ASSERT(window::initialize(), "Error initializing window");

  g_sdlCustomEventsStart = SDL_RegisterEvents(2);
  AURORA_ASSERT(g_sdlCustomEventsStart, "Failed to allocate user events: {}", SDL_GetError());
  AURORA_ASSERT(window::initialize_event_watch(), "Error initializing SDL event watch");

#ifdef AURORA_ENABLE_GX
  /* Attempt to create a window using the calling application's desired backend */
  AuroraBackend selectedBackend = config.desiredBackend;
  bool windowCreated = false;
  if (selectedBackend != BACKEND_AUTO && window::create_window(selectedBackend)) {
    if (webgpu::initialize(selectedBackend, config.allowCpuAdapter)) {
      windowCreated = true;
    } else {
      window::destroy_window();
    }
  }

  if (!windowCreated) {
    for (const auto backendType : PreferredBackendOrder) {
      selectedBackend = backendType;
      if (!window::create_window(selectedBackend)) {
        continue;
      }
      if (webgpu::initialize(selectedBackend, config.allowCpuAdapter)) {
        windowCreated = true;
        break;
      } else {
        window::destroy_window();
      }
    }
  }

  AURORA_ASSERT(windowCreated, "Error creating window: {}", SDL_GetError());

  // Initialize SDL_Renderer for ImGui when we can't use a Dawn backend
  if (webgpu::g_backendType == wgpu::BackendType::Null) {
    AURORA_ASSERT(window::create_renderer(), "Failed to initialize SDL renderer: {}", SDL_GetError());
  }
#else
  AuroraBackend selectedBackend = BACKEND_NULL;
  AURORA_ASSERT(window::create_window(BACKEND_NULL), "Error creating window: {}", SDL_GetError());
  AURORA_ASSERT(window::create_renderer(), "Failed to initialize SDL renderer: {}", SDL_GetError());
#endif

  window::show_window();
  thread::set_current({
      .name = "Main thread",
      .affinity = thread::Affinity::SharedCache,
  });

#ifdef AURORA_ENABLE_GX
  gfx::initialize();
  gx::fifo::init();
  imgui::create_context();
#endif
  const auto size = window::get_window_size();
  Log.info("Using framebuffer size {}x{} scale {}", size.fb_width, size.fb_height, size.scale);
#ifdef AURORA_ENABLE_GX
  if (g_config.imGuiInitCallback != nullptr) {
    g_config.imGuiInitCallback(&size);
  }
  imgui::initialize();
#endif

#ifdef AURORA_ENABLE_RMLUI
  rmlui::initialize(size);
#endif

  g_initialFrame = true;
  g_config.desiredBackend = selectedBackend;
  return {
      .backend = selectedBackend,
      .userPath = g_config.userPath,
      .cachePath = g_config.cachePath,
      .window = window::get_sdl_window(),
      .windowSize = size,
  };
}

void shutdown() noexcept {
  ownerService.reset();
  hostExitHandler=nullptr;
  const auto trace = [](const char* stage) {
    if (std::getenv("AURORA_DIAGNOSTIC_COMPONENT_SHUTDOWN")) {
      std::fprintf(stderr, "COMPONENT_SHUTDOWN_STAGE %s\n", stage);
      std::fflush(stderr);
    }
  };
#ifdef AURORA_ENABLE_GX
  if(std::getenv("AURORA_DIAGNOSTIC_COMPONENT_SHUTDOWN")) std::fprintf(stderr,"DRAW_DONE_TEARDOWN_ENTRY pending=%d\n",gx::draw_done::pending(0));
  trace("fifo");
  gx::fifo::shutdown();
  trace("worker-sync");
  gfx::render_worker::synchronize();
  trace("render-snapshot-drain");
  AURORA_ASSERT(!gfx::render_snapshot::active, "Cannot shut down an open snapshot recording");
  const auto snapshotDrainStart = SDL_GetTicksNS();
  while (!gfx::render_snapshot::pool.drained()) {
    AURORA_ASSERT(SDL_GetTicksNS() - snapshotDrainStart < 2000000000ULL,
                  "GPU snapshot acknowledgements did not drain");
    gfx::render_worker::enqueue_work([] { webgpu::g_instance.ProcessEvents(); });
    gfx::render_worker::synchronize();
    SDL_Delay(1);
  }
  const auto& snapshots = gfx::render_snapshot::pool;
  if (snapshots.acquired || std::getenv("AURORA_DIAGNOSTIC_SNAPSHOT_TRACE"))
    std::fprintf(stderr, "NATIVE_RENDER_SNAPSHOT_POOL acquired=%llu submitted=%llu retired=%llu cancelled=%llu deferred=%llu failed=%u high_water=%zu\n",
      (unsigned long long)snapshots.acquired, (unsigned long long)snapshots.submitted,
      (unsigned long long)snapshots.retired, (unsigned long long)snapshots.cancelled,
      (unsigned long long)snapshots.deferred, unsigned(snapshots.has_failed()), snapshots.high_water);
  trace("xfb-vi");
  if (!gx::xfb::pump()) std::abort();
  trace("draw-done-pump");
  if (!gx::draw_done::pump()) std::abort();
  trace("draw-done-shutdown");
  // Retire accepted presentation tickets while the callback owner/gate is
  // still live. Do not schedule another retrace during teardown.
  trace("vi-retire-pump");
  if (!vi::pump(false)) std::abort();
  trace("draw-done-drain");
  const int drawDonePending=gx::draw_done::pending(0);
  const bool drawDoneWaited=gx::draw_done::wait(2000);
  if(std::getenv("AURORA_DIAGNOSTIC_COMPONENT_SHUTDOWN")) std::fprintf(stderr,"DRAW_DONE_TEARDOWN_DRAIN initial_pending=%d waited=%u final_pending=%d\n",drawDonePending,unsigned(drawDoneWaited),gx::draw_done::pending(0));
  if(!drawDoneWaited) std::abort();
  const bool drawDoneDrained=gx::draw_done::shutdown();
  if(std::getenv("AURORA_DIAGNOSTIC_COMPONENT_SHUTDOWN")) std::fprintf(stderr,"DRAW_DONE_SHUTDOWN_CALL returned=%u\n",unsigned(drawDoneDrained));
  if (!drawDoneDrained) std::abort();
  trace("vi-shutdown");
  if (!vi::shutdown()) std::abort();
  trace("xfb-shutdown");
  if (!gx::xfb::shutdown()) std::abort();
#ifdef AURORA_ENABLE_RMLUI
  rmlui::shutdown();
#endif
  trace("imgui");
  imgui::shutdown();
  trace("gfx");
  gfx::shutdown();
  trace("webgpu");
  webgpu::shutdown();
#endif
  trace("input");
  input::shutdown();
  trace("window");
  window::shutdown();
  trace("complete");
  diagnosticOwner.store(0);
}

const AuroraEvent* update() noexcept {
  ZoneScoped;
  AURORA_ASSERT(aurora_service_owner(),"Native owner service failed");
#ifdef AURORA_ENABLE_GX
  service_texture_pack();
  if (!gfx::detail::diagnostic_recording_id() && (!gx::xfb::pump() || !gx::draw_done::pump()))std::abort();
  if(!vi::pump(!gfx::detail::diagnostic_recording_id()))std::abort();
#endif
  if (g_initialFrame) {
    g_initialFrame = false;
    input::initialize();
  }
#ifdef AURORA_ENABLE_GX
  gx::update();
#endif
  return window::poll_events();
}

extern "C" int aurora_service_wait_events(void) {
  if (!diagnostic_on_owner()) return 0;
  const auto* event = aurora::window::poll_events();
  for (; event && event->type != AURORA_NONE; ++event) {
    if (event->type == AURORA_EXIT) aurora_handle_exit();
  }
  return 1;
}
extern "C" int aurora_service_input_wait(void) {
  if(!aurora_service_wait_events())return 0;
  if (!aurora_service_owner()) return 0;
  SDL_Delay(1);
  return 1;
}

bool begin_frame() noexcept {
  ZoneScoped;
#ifdef AURORA_ENABLE_GX
  service_texture_pack();
  // Isolate completed-copy delivery from the pending-XFB helper's extra present.
  // Callbacks stay on the owner, before any new recording or VI ticket begins.
  if (std::getenv("AURORA_DIAGNOSTIC_PUMP_BEFORE_FRAME")) {
    AURORA_ASSERT(gx::xfb::on_owner() && !gfx::detail::diagnostic_recording_id(),
                  "Pre-frame completion delivery requires an idle owner");
    if (!gx::xfb::pump() || !gx::draw_done::pump()) std::abort();
  }
  if(!vi::pump(true))std::abort();
  const auto setupStart = SDL_GetTicksNS();
  {
    if (!window::is_presentable()) {
      webgpu::release_surface();
      return false;
    }
    if (window::is_paused()) {
      return false;
    }
    if (!g_surface) {
      webgpu::refresh_surface(true);
      if (!g_surface) {
        return false;
      }
    }
  }

  const auto windowReady = SDL_GetTicksNS();
  imgui::new_frame(window::get_window_size());
  const auto imguiReady = SDL_GetTicksNS();
  if (!gfx::begin_frame()) {
    return false;
  }
  diagnosticFramesStarted = true;
  const auto gfxReady = SDL_GetTicksNS();
  gx::fifo::begin_frame();
  if (std::getenv("AURORA_DIAGNOSTIC_FRAME_WAIT")) {
    const auto fifoReady = SDL_GetTicksNS();
    std::fprintf(stderr, "FRAME_SETUP window_ns=%llu imgui_ns=%llu gfx_ns=%llu fifo_ns=%llu\n",
                 static_cast<unsigned long long>(windowReady-setupStart),
                 static_cast<unsigned long long>(imguiReady-windowReady),
                 static_cast<unsigned long long>(gfxReady-imguiReady),
                 static_cast<unsigned long long>(fifoReady-gfxReady));
  }
#endif
  return true;
}

void end_frame() noexcept {
  ZoneScoped;
#ifdef AURORA_ENABLE_GX
  const bool trace = std::getenv("AURORA_DIAGNOSTIC_Z_FIFO") != nullptr;
  if (trace) std::fputs("Z_FRAME_END_BEGIN\n", stderr);
  gx::fifo::drain();
  if (trace) std::fputs("Z_FRAME_END_FIFO_DRAINED\n", stderr);
  gx::fifo::end_frame();
  if (trace) std::fputs("Z_FRAME_END_FIFO_ENDED\n", stderr);
  gx::texture::end_frame();
  if (trace) std::fputs("Z_FRAME_END_TEXTURES_ENDED\n", stderr);
  if(std::getenv("AURORA_DIAGNOSTIC_PREPARE_GX_PASSES")) gfx::finish();
  std::shared_ptr<vi::Presentation> viTicket;
  gfx::TextureHandle viSource;
  auto snapshotTicket = gfx::render_snapshot::publish(gfx::detail::diagnostic_recording_id());
  const auto snapshotAckDelay = snapshot_ack_delay_ms();
  if (trace) std::fputs("Z_FRAME_END_VI_SOURCE_BEGIN\n", stderr);
  if(!snapshotTicket && vi::presentation.active &&
     !vi::present_source(gfx::detail::diagnostic_recording_id(),viSource,viTicket))std::abort();
  if (trace) std::fputs("Z_FRAME_END_VI_SOURCE_END\n", stderr);
  gfx::finish();
  if (trace) std::fputs("Z_FRAME_END_RECORDING_FINISHED\n", stderr);
  auto imguiDrawData = imgui::freeze();
  if (trace) std::fputs("Z_FRAME_END_UI_FROZEN\n", stderr);

  // Capture this frame's source handles and dimensions before asynchronous work.
  auto presentSource = webgpu::present_source();
  if(const auto snapshot=gfx::detail::diagnostic_selected_snapshot()) {
    presentSource.texture=snapshot->texture;
    presentSource.view=snapshot->sampleTextureView;
    presentSource.size=snapshot->size;
    presentSource.format=snapshot->format;
  }
  auto xfbJobs=gx::xfb::frame_jobs(gfx::detail::diagnostic_recording_id());
  if(snapshotTicket) {
    for(const auto& job:xfbJobs) AURORA_ASSERT(!job, "Snapshot frame must not contain CPU XFB jobs");
    const auto& snapshot=snapshotTicket->resource;
    presentSource.texture=snapshot->texture;presentSource.view=snapshot->sampleTextureView;
    presentSource.size=snapshot->size;presentSource.format=snapshot->format;
    if(std::getenv("AURORA_DIAGNOSTIC_SNAPSHOT_TRACE") && snapshotTicket->id<=snapshot_trace_limit())
      std::fprintf(stderr,"NATIVE_RENDER_SNAPSHOT frame=%llu id=%llu generation=%llu slot=%zu width=%u height=%u vi_ticket=0 xfb_jobs=0\n",
        (unsigned long long)snapshotTicket->frame,(unsigned long long)snapshotTicket->id,
        (unsigned long long)snapshotTicket->generation,snapshotTicket->slot,
        snapshot->size.width,snapshot->size.height);
  }
  if(viSource){
    presentSource.texture=viSource->texture;presentSource.view=viSource->sampleTextureView;
    presentSource.size=viSource->size;presentSource.format=viSource->format;
  }
  const auto viewport = webgpu::calculate_present_viewport(webgpu::g_graphicsConfig.surfaceConfiguration.width,
                                                           webgpu::g_graphicsConfig.surfaceConfiguration.height,
                                                           presentSource.size.width, presentSource.size.height);

  wgpu::BindGroup rmlBindGroup;
  bool rmlOverlay = false;
#if AURORA_ENABLE_RMLUI
  if (rmlui::is_initialized()) {
    auto rmlFrame = rmlui::record_frame(viewport);
    rmlBindGroup = std::move(rmlFrame.bindGroup);
    rmlOverlay = rmlFrame.overlay;
  }
#endif

  gfx::end_frame([rmlBindGroup = std::move(rmlBindGroup), rmlOverlay, viewport, presentSource,
                  imguiDrawData = std::move(imguiDrawData), xfbJobs=std::move(xfbJobs),viTicket=std::move(viTicket), snapshotTicket=std::move(snapshotTicket), snapshotAckDelay](
                     uint64_t recordedFrameId, wgpu::CommandEncoder& encoder, std::vector<gfx::AfterSubmitCallback> afterSubmitCallbacks) {
    if(viTicket && viTicket->preparedNs)viTicket->workerEntryNs.store(SDL_GetTicksNS(),std::memory_order_release);
    if(viTicket)viTicket->phase.store(1,std::memory_order_release);
    wgpu::Texture currentTexture;
    wgpu::TextureView currentView;
    auto surfaceStatus = wgpu::SurfaceGetCurrentTextureStatus::Error;
    {
      if(viTicket)viTicket->presentStep.store(1,std::memory_order_release);
      window::SurfaceLock surfaceLock;
      if (window::is_presentable() && g_surface) {
        ZoneScopedN("Acquire texture");
        wgpu::SurfaceTexture surfaceTexture;
        if(viTicket)viTicket->presentStep.store(2,std::memory_order_release);
        if(viTicket && viTicket->preparedNs)viTicket->acquireStartNs.store(SDL_GetTicksNS(),std::memory_order_release);
        // One-shot diagnostic delay at the acquisition boundary. It must not
        // acknowledge the ticket or advance VI while the render worker waits.
        if(viTicket && std::getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")){
          static bool injected=false;
          const char* text=std::getenv("AURORA_DIAGNOSTIC_VI_ACQUIRE_DELAY_MS");
          if(!injected && text && recordedFrameId>=126){
            char* end=nullptr;const auto delay=std::strtoul(text,&end,10);
            if(end!=text && !*end && delay>0 && delay<=4000){
              injected=true;
              std::fprintf(stderr,"VI_ACQUIRE_DELAY_BEGIN frame=%llu milliseconds=%lu\n",(unsigned long long)recordedFrameId,delay);
              SDL_Delay(static_cast<Uint32>(delay));
              std::fprintf(stderr,"VI_ACQUIRE_DELAY_END frame=%llu\n",(unsigned long long)recordedFrameId);
            }
          }
        }
        g_surface.GetCurrentTexture(&surfaceTexture);
        if(viTicket && viTicket->preparedNs)viTicket->acquireEndNs.store(SDL_GetTicksNS(),std::memory_order_release);
        if(viTicket)viTicket->presentStep.store(3,std::memory_order_release);
        surfaceStatus = surfaceTexture.status;
        if (surfaceStatus == wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal) {
          currentTexture = std::move(surfaceTexture.texture);
          currentView = currentTexture.CreateView();
        }
      }
    }

    const bool canPresent = currentTexture && currentView;
    if (canPresent) {
      wgpu::BindGroup presentBindGroup;
      if (rmlBindGroup && !rmlOverlay) {
        presentBindGroup = rmlBindGroup;
      } else {
        if(viTicket)viTicket->presentStep.store(4,std::memory_order_release);
        const auto& resampledSource = webgpu::resample_present_source(encoder, viewport, presentSource);
        if(std::getenv("AURORA_DIAGNOSTIC_CAPTURE_PRESENT_SOURCE") &&
           (!std::getenv("AURORA_DIAGNOSTIC_CAPTURE_REQUIRE_GPU_SNAPSHOT") || snapshotTicket))
          gfx::diagnostic_capture::encode(encoder, presentSource.texture, recordedFrameId,
            bool(viTicket),viTicket?viTicket->black:false,viTicket?viTicket->frame:0,viTicket?viTicket->sourceFrame:0);
        if(viTicket)viTicket->presentStep.store(5,std::memory_order_release);
        presentBindGroup = webgpu::create_copy_bind_group(resampledSource);
      }
      {
        if(viTicket)viTicket->presentStep.store(6,std::memory_order_release);
        const std::array attachments{
            wgpu::RenderPassColorAttachment{
                .view = currentView,
                .loadOp = wgpu::LoadOp::Clear,
                .storeOp = wgpu::StoreOp::Store,
            },
        };
        const wgpu::RenderPassDescriptor renderPassDescriptor{
            .label = "EFB copy render pass",
            .colorAttachmentCount = attachments.size(),
            .colorAttachments = attachments.data(),
            .timestampWrites = webgpu::gpu_prof::pass_writes("Present blit"),
        };
        const auto pass = encoder.BeginRenderPass(&renderPassDescriptor);
        // Copy EFB -> XFB (swapchain)
        pass.SetPipeline(webgpu::g_CopyPipeline);
        pass.SetBindGroup(0, presentBindGroup, 0, nullptr);
        set_present_viewport(pass, viewport, webgpu::g_graphicsConfig.surfaceConfiguration.width,
                             webgpu::g_graphicsConfig.surfaceConfiguration.height);

        pass.Draw(3);
        if (rmlBindGroup && rmlOverlay) {
          pass.SetPipeline(webgpu::g_CopyPremultipliedAlphaPipeline);
          pass.SetBindGroup(0, rmlBindGroup, 0, nullptr);
          pass.Draw(3);
        }
        pass.End();
      }
      if (std::getenv("AURORA_DIAGNOSTIC_CAPTURE_COMPOSITED_PRESENT")) {
        // Re-render the same final compositor pass into a readable offscreen
        // texture. Surface textures are not guaranteed to support CopySrc, so
        // read back this exact-size diagnostic twin rather than the swapchain.
        static wgpu::Texture diagnosticPresentTexture;
        static uint32_t diagnosticPresentWidth = 0;
        static uint32_t diagnosticPresentHeight = 0;
        static wgpu::TextureFormat diagnosticPresentFormat = wgpu::TextureFormat::Undefined;
        const auto width = webgpu::g_graphicsConfig.surfaceConfiguration.width;
        const auto height = webgpu::g_graphicsConfig.surfaceConfiguration.height;
        const auto format = webgpu::g_graphicsConfig.surfaceConfiguration.format;
        if (!diagnosticPresentTexture || diagnosticPresentWidth != width || diagnosticPresentHeight != height ||
            diagnosticPresentFormat != format) {
          diagnosticPresentTexture = {};
          const wgpu::TextureDescriptor descriptor{
              .label = "Diagnostic composited presentation capture",
              .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc,
              .dimension = wgpu::TextureDimension::e2D,
              .size = {.width = width, .height = height, .depthOrArrayLayers = 1},
              .format = format,
              .mipLevelCount = 1,
              .sampleCount = 1,
          };
          diagnosticPresentTexture = webgpu::g_device.CreateTexture(&descriptor);
          diagnosticPresentWidth = width;
          diagnosticPresentHeight = height;
          diagnosticPresentFormat = format;
        }
        const auto diagnosticPresentView = diagnosticPresentTexture.CreateView();
        const std::array captureAttachments{
            wgpu::RenderPassColorAttachment{
                .view = diagnosticPresentView,
                .loadOp = wgpu::LoadOp::Clear,
                .storeOp = wgpu::StoreOp::Store,
            },
        };
        const wgpu::RenderPassDescriptor captureDescriptor{
            .label = "Diagnostic composited presentation pass",
            .colorAttachmentCount = captureAttachments.size(),
            .colorAttachments = captureAttachments.data(),
        };
        {
          const auto capturePass = encoder.BeginRenderPass(&captureDescriptor);
          capturePass.SetPipeline(webgpu::g_CopyPipeline);
          capturePass.SetBindGroup(0, presentBindGroup, 0, nullptr);
          set_present_viewport(capturePass, viewport, width, height);
          capturePass.Draw(3);
          if (rmlBindGroup && rmlOverlay) {
            capturePass.SetPipeline(webgpu::g_CopyPremultipliedAlphaPipeline);
            capturePass.SetBindGroup(0, rmlBindGroup, 0, nullptr);
            capturePass.Draw(3);
          }
          capturePass.End();
        }
        {
          const std::array overlayAttachments{
              wgpu::RenderPassColorAttachment{
                  .view = diagnosticPresentView,
                  .loadOp = wgpu::LoadOp::Load,
                  .storeOp = wgpu::StoreOp::Store,
              },
          };
          const wgpu::RenderPassDescriptor overlayDescriptor{
              .label = "Diagnostic composited ImGui pass",
              .colorAttachmentCount = overlayAttachments.size(),
              .colorAttachments = overlayAttachments.data(),
          };
          const auto capturePass = encoder.BeginRenderPass(&overlayDescriptor);
          capturePass.SetViewport(0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f);
          imgui::render(capturePass, imguiDrawData);
          capturePass.End();
        }
        gfx::diagnostic_capture::encode(encoder, diagnosticPresentTexture, recordedFrameId,
            bool(viTicket), viTicket ? viTicket->black : false, viTicket ? viTicket->frame : 0,
            viTicket ? viTicket->sourceFrame : 0, true);
      }
      {
        if(viTicket)viTicket->presentStep.store(7,std::memory_order_release);
        const std::array attachments{
            wgpu::RenderPassColorAttachment{
                .view = currentView,
                .loadOp = wgpu::LoadOp::Load,
                .storeOp = wgpu::StoreOp::Store,
            },
        };
        const wgpu::RenderPassDescriptor renderPassDescriptor{
            .label = "ImGui render pass",
            .colorAttachmentCount = attachments.size(),
            .colorAttachments = attachments.data(),
            .timestampWrites = webgpu::gpu_prof::pass_writes("ImGui"),
        };
        const auto pass = encoder.BeginRenderPass(&renderPassDescriptor);
        pass.SetViewport(0.f, 0.f, static_cast<float>(webgpu::g_graphicsConfig.surfaceConfiguration.width),
                         static_cast<float>(webgpu::g_graphicsConfig.surfaceConfiguration.height), 0.f, 1.f);
        imgui::render(pass, imguiDrawData);
        pass.End();
      }
    } else {
      Log.info("Skipping present; window not presentable");
    }
    webgpu::gpu_prof::frame_end(encoder);
    if(viTicket)viTicket->presentStep.store(8,std::memory_order_release);
    const wgpu::CommandBufferDescriptor cmdBufDescriptor{.label = "Redraw command buffer"};
    if(viTicket)viTicket->phase.store(2,std::memory_order_release);
    const bool traceViTiming=viTicket && std::getenv("AURORA_DIAGNOSTIC_VI_TIMING");
    if(traceViTiming)viTicket->encoderFinishStartedNs.store(SDL_GetTicksNS(),std::memory_order_release);
    // Fault-injection hook to prove the timeout report can identify a Finish
    // stall. It is inert unless both diagnostic timing and a bounded delay are
    // explicitly requested; it does not change normal presentation behavior.
    if(traceViTiming)if(const char* delayText=std::getenv("AURORA_DIAGNOSTIC_VI_FINISH_DELAY_MS")){
      char* end=nullptr;
      const auto delayMs=std::strtoul(delayText,&end,10);
      if(end!=delayText && end && *end=='\0' && delayMs>0 && delayMs<=10000)
        SDL_DelayNS(static_cast<uint64_t>(delayMs)*1000000ULL);
    }
    const auto buffer = encoder.Finish(&cmdBufDescriptor);
    if(traceViTiming)viTicket->encoderFinishCompletedNs.store(SDL_GetTicksNS(),std::memory_order_release);
    uint64_t diagnosticSubmissionId=0;const uint64_t diagnosticEpoch=diagnosticGeneration.load();
    {
      ZoneScopedN("Queue Submit");
      if(traceViTiming)viTicket->queueSubmitStartedNs.store(SDL_GetTicksNS(),std::memory_order_release);
      g_queue.Submit(1, &buffer);
      if(traceViTiming)viTicket->queueSubmitCompletedNs.store(SDL_GetTicksNS(),std::memory_order_release);
      if(viTicket)viTicket->phase.store(3,std::memory_order_release);
      g_queue.OnSubmittedWorkDone(wgpu::CallbackMode::AllowSpontaneous,
        [id=recordedFrameId,xfbJobs,snapshotTicket,snapshotAckDelay](wgpu::QueueWorkDoneStatus status,wgpu::StringView){
          const bool success=status==wgpu::QueueWorkDoneStatus::Success;
          for(const auto& job:xfbJobs)if(job){if(!success)job->failed.store(true,std::memory_order_release);else job->gpuDone.store(true,std::memory_order_release);}
          gx::draw_done::complete(id,success);
          // Bounded fault injection only; normal execution never sleeps here.
          if(snapshotTicket && snapshotAckDelay)SDL_Delay(snapshotAckDelay);
          if(snapshotTicket && !snapshotTicket->acknowledge(snapshotTicket->generation,snapshotTicket->id,true,success))std::abort();
        });
      if (std::getenv("AURORA_DIAGNOSTIC_GPU_COMPLETION")) {
        diagnosticSubmissionId=recordedFrameId;diagnostic_ack(diagnosticEpoch,diagnosticSubmissionId,0,0);diagnosticSubmitted.fetch_add(1);
        g_queue.OnSubmittedWorkDone(wgpu::CallbackMode::AllowSpontaneous,
          [epoch=diagnosticEpoch,id=diagnosticSubmissionId](wgpu::QueueWorkDoneStatus status, wgpu::StringView) {
            diagnostic_ack(epoch,id,1,status==wgpu::QueueWorkDoneStatus::Success?0:1);
            if(status==wgpu::QueueWorkDoneStatus::Success) diagnosticCompleted.fetch_add(1);
            else diagnosticFailed.fetch_add(1);
          });
      }
      if(traceViTiming)viTicket->queueCallbacksRegisteredNs.store(SDL_GetTicksNS(),std::memory_order_release);
    }
    webgpu::gpu_prof::after_submit();
    bool diagnosticPresentAccepted = false;
    if (canPresent && g_surface) {
      ZoneScopedN("Present");
      wgpu::ConvertibleStatus status = wgpu::Status::Error;
      {
        window::SurfaceLock surfaceLock;
        // Explicit diagnostic omission, not a simulated hardware/device failure.
        const bool omitDiagnosticPresent = diagnosticSubmissionId &&
            std::getenv("AURORA_DIAGNOSTIC_SKIP_PRESENT");
        if (window::is_presentable() && !omitDiagnosticPresent) {
          if(traceViTiming)viTicket->surfacePresentStartedNs.store(SDL_GetTicksNS(),std::memory_order_release);
          status = g_surface.Present();
          if(traceViTiming)viTicket->surfacePresentCompletedNs.store(SDL_GetTicksNS(),std::memory_order_release);
        }
      }
      if (status) {
        diagnosticPresentAccepted = true;
        if(diagnosticSubmissionId){diagnostic_ack(diagnosticEpoch,diagnosticSubmissionId,2,0);diagnosticPresented.fetch_add(1);}
        gfx::after_present();
      } else {
        Log.warn("Surface present failed");
        webgpu::release_surface();
      }
    } else if (g_surface) {
      switch (surfaceStatus) {
      case wgpu::SurfaceGetCurrentTextureStatus::Timeout:
        Log.warn("Surface texture acquisition timed out");
        break;
      case wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal:
      case wgpu::SurfaceGetCurrentTextureStatus::Outdated:
        Log.info("Surface texture is {}, reconfiguring swapchain", magic_enum::enum_name(surfaceStatus));
        window::push_custom_event(window::CustomEvent::RefreshSurface);
        break;
      case wgpu::SurfaceGetCurrentTextureStatus::Lost:
        Log.warn("Surface texture is {}, releasing surface", magic_enum::enum_name(surfaceStatus));
        webgpu::release_surface();
        break;
      case wgpu::SurfaceGetCurrentTextureStatus::Error:
        Log.warn("Surface texture is {}, dropping surface", magic_enum::enum_name(surfaceStatus));
        g_surface = {};
        break;
      default:
        if (!window::is_presentable()) {
          webgpu::release_surface();
        } else {
          Log.error("Failed to get surface texture: {}", magic_enum::enum_name(surfaceStatus));
        }
        break;
      }
    }
    if(viTicket && viTicket->preparedNs)viTicket->resultNs.store(SDL_GetTicksNS(),std::memory_order_release);
    if(viTicket){viTicket->result.store(diagnosticPresentAccepted?1:2,std::memory_order_release);viTicket->phase.store(4,std::memory_order_release);}
    if(snapshotTicket && !snapshotTicket->acknowledge(snapshotTicket->generation,snapshotTicket->id,false,diagnosticPresentAccepted))std::abort();
    // Unavailable or failed presentation must not permit ticket retirement.
    if (diagnosticSubmissionId && !diagnosticPresentAccepted) {
      diagnostic_ack(diagnosticEpoch, diagnosticSubmissionId, 2, 1);
    }
    for (auto& callback : afterSubmitCallbacks) {
      if (callback) {
        callback();
      }
    }
    gfx::after_submit();

    TracyPlotConfig("aurora: lastVertSize", tracy::PlotFormatType::Memory, false, true, 0);
    TracyPlotConfig("aurora: lastUniformSize", tracy::PlotFormatType::Memory, false, true, 0);
    TracyPlotConfig("aurora: lastIndexSize", tracy::PlotFormatType::Memory, false, true, 0);
    TracyPlotConfig("aurora: lastStorageSize", tracy::PlotFormatType::Memory, false, true, 0);
    TracyPlotConfig("aurora: lastTextureUploadSize", tracy::PlotFormatType::Memory, false, true, 0);

    const auto& stats = gfx::detail::resources().stats;
    TracyPlot("aurora: queuedPipelines", static_cast<int64_t>(stats.queuedPipelines));
    TracyPlot("aurora: createdPipelines", static_cast<int64_t>(stats.createdPipelines));
    TracyPlot("aurora: drawCallCount", static_cast<int64_t>(stats.drawCallCount));
    TracyPlot("aurora: mergedDrawCallCount", static_cast<int64_t>(stats.mergedDrawCallCount));
    TracyPlot("aurora: lastVertSize", static_cast<int64_t>(stats.lastVertSize));
    TracyPlot("aurora: lastUniformSize", static_cast<int64_t>(stats.lastUniformSize));
    TracyPlot("aurora: lastIndexSize", static_cast<int64_t>(stats.lastIndexSize));
    TracyPlot("aurora: lastStorageSize", static_cast<int64_t>(stats.lastStorageSize));
    TracyPlot("aurora: lastTextureUploadSize", static_cast<int64_t>(stats.lastTextureUploadSize));
  });

#endif
}
} // namespace
} // namespace aurora

// C API bindings
AuroraInfo aurora_initialize(int argc, char* argv[], const AuroraConfig* config) {
  const auto info = aurora::initialize(argc, argv, *config);
#ifdef AURORA_ENABLE_GX
  texturePackRequested.store(0);
  texturePackApplied.store(0);
  hdTexturePackGroup.registrations.clear();
  auto packRoot = aurora::io::fs_path_from_string(aurora::g_config.resourcesPath) / "content" / "hd";
  if (const char* path = SDL_getenv("MELEE_HD_TEXTURE_PACK_DIR"); path && *path)
    packRoot = aurora::io::fs_path_from_string(path);
  const char* diagnosticPath = SDL_getenv("MELEE_DIAGNOSTIC_HD_PACK_DIR");
  if (diagnosticPath && *diagnosticPath)
    packRoot = aurora::io::fs_path_from_string(diagnosticPath).parent_path();
  hdTexturePackPath = packRoot / "textures";
  std::error_code error;
  const bool installed = std::filesystem::is_regular_file(packRoot / "manifest.json", error) && !error &&
                         std::filesystem::is_directory(hdTexturePackPath, error) && !error;
  hdTexturePackAvailable.store(installed);
  if (diagnosticPath && *diagnosticPath && installed) {
    texturePackRequested.store(1);
    service_texture_pack();
    std::fprintf(stderr, "NATIVE_HD_PACK registered=%zu diagnostic=1\n", hdTexturePackGroup.registrations.size());
  }
#endif
  return info;
}
void aurora_shutdown() {
  aurora::shutdown();
#ifdef AURORA_ENABLE_GX
  hdTexturePackGroup.registrations.clear();
  hdTexturePackPath.clear();
#endif
  texturePackApplied.store(0);
  texturePackRequested.store(0);
  hdTexturePackAvailable.store(false);
}
const AuroraEvent* aurora_update() { return aurora::update(); }
int aurora_service_frame_events(void) {
  if(!diagnostic_on_owner())return 0;
#ifdef AURORA_ENABLE_GX
  if(aurora::gfx::detail::diagnostic_recording_id())return 0;
#endif
  const AuroraEvent* event=aurora::update();
  for(;event && event->type!=AURORA_NONE;++event)
    if(event->type==AURORA_EXIT)aurora_handle_exit();
  return 1;
}
bool aurora_begin_frame() { return aurora::begin_frame(); }
bool aurora_set_render_snapshot_scene(bool enabled) {
#ifdef AURORA_ENABLE_GX
  using namespace aurora;
  AURORA_ASSERT(gx::xfb::on_owner() && !gfx::detail::diagnostic_recording_id() && !gfx::render_snapshot::active,
                "Snapshot scene handoff requires an idle owner");
  if(!enabled)return vi::presentation.set_snapshot_clock(false);
  if(!gx::xfb::pump() || !gx::draw_done::pump() || !vi::pump(false))std::abort();
  if(gx::draw_done::pending(0)!=0)return false;
  for(const auto& job:gx::xfb::jobs)if(job)return false;
  const bool wasEnabled=vi::presentation.snapshotClock;
  const bool enabledNow=vi::presentation.set_snapshot_clock(true);
  if(enabledNow && !wasEnabled && std::getenv("AURORA_DIAGNOSTIC_SNAPSHOT_TRACE"))
    std::fprintf(stderr,"NATIVE_SNAPSHOT_SCENE enabled=1 cpu_xfb_jobs=0 vi_inflight=0 framebuffer_promoted=0\n");
  return enabledNow;
#else
  return false;
#endif
}
void* aurora_begin_render_snapshot_frame(void) {
#ifdef AURORA_ENABLE_GX
  using namespace aurora;
  AURORA_ASSERT(gx::xfb::on_owner() && !gfx::detail::diagnostic_recording_id() && !gfx::render_snapshot::active,
                "GPU snapshot acquisition requires idle owner");
  // Extra pictures must not bypass an intentional black/fade boundary.
  if(vi::presentation.active && (vi::presentation.black ||
      (vi::presentation.flushed && vi::presentation.draftBlack)))return nullptr;
  auto ticket=gfx::render_snapshot::pool.acquire();
  if(!ticket) {
    // A full pool cannot reach begin_frame's GPU event processing. Poll on the
    // existing GPU owner so real completion callbacks can release their ticket
    // references. Coalesce requests and never block or manufacture an ack.
    static std::atomic<bool> pollQueued{false};
    bool expected=false;
    if(webgpu::g_instance && !gfx::render_snapshot::pool.has_failed() &&
       pollQueued.compare_exchange_strong(expected,true,std::memory_order_acq_rel)) {
      if(!gfx::render_worker::try_enqueue_work([] {
          webgpu::g_instance.ProcessEvents();
          pollQueued.store(false,std::memory_order_release);
        }))pollQueued.store(false,std::memory_order_release);
      else if(std::getenv("AURORA_DIAGNOSTIC_SNAPSHOT_TRACE")) {
        static unsigned records=0;
        if(records++<16)std::fprintf(stderr,"NATIVE_SNAPSHOT_EVENT_POLL queued=1\n");
      }
    }
    return nullptr;
  }
  if(!begin_frame()) {
    AURORA_ASSERT(gfx::render_snapshot::pool.cancel_unencoded(ticket),"Unencoded snapshot cancellation failed");
    return nullptr;
  }
  ticket->frame=gfx::detail::diagnostic_recording_id();
  void* token=const_cast<void*>(gfx::render_snapshot::pool.token(ticket));
  gfx::render_snapshot::active=std::move(ticket);
  return token;
#else
  return nullptr;
#endif
}
bool aurora_begin_scene_frame(void) {
#ifdef AURORA_ENABLE_GX
  using namespace aurora;
  AURORA_ASSERT(gx::xfb::on_owner() && !gfx::detail::diagnostic_recording_id(),
                "Scene acquisition requires idle owner recording");
  if (!gx::xfb::pump() || !gx::draw_done::pump()) std::abort();
  // begin_frame performs VI selection. Inspect that selection afterward so
  // a retrace crossing during acquisition cannot be missed by a preflight.
  if (!begin_frame()) return false;
  auto& state = vi::presentation;
  if(std::getenv("AURORA_DIAGNOSTIC_VI_TIMELINE")) {
    static unsigned records=0;
    if(records<8192)std::fprintf(stderr,
      "NATIVE_VI_SCENE_GATE record=%u active=%u waiting=%u inflight=%u flushed=%u pinned=%u black=%u\n",
      ++records,unsigned(state.active),unsigned(state.waiting),unsigned(bool(state.inflight)),
      unsigned(state.flushed),unsigned(state.draftPinned),unsigned(state.draftBlack));
  }
  if (!state.active || !state.waiting || state.inflight || !state.flushed ||
      !state.draftPinned || state.draftBlack) return true;
  const auto frame = gfx::detail::diagnostic_recording_id();
  // The existing end_frame path freezes the source and immutable ticket,
  // retaining all pin and API-acceptance retirement rules. No game callback.
  end_frame();
  static unsigned records = 0;
  if (records < 1024) {
    std::fprintf(stderr,"NATIVE_PENDING_XFB_PRESENT record=%u frame=%llu scene_draw=0\n",
                 ++records,static_cast<unsigned long long>(frame));
  }
  // At most one extra submission per acquisition; no recursive servicing.
  return begin_frame();
#else
  return aurora::begin_frame();
#endif
}
void aurora_end_frame() { aurora::end_frame(); }
AuroraBackend aurora_get_backend() { return aurora::g_config.desiredBackend; }
const AuroraBackend* aurora_get_available_backends(size_t* count) {
  if (count != nullptr) {
    *count = aurora::PreferredBackendOrder.size();
  }
  return aurora::PreferredBackendOrder.data();
}
void aurora_set_log_level(AuroraLogLevel level) { aurora::g_config.logLevel = level; }
void aurora_set_pause_on_focus_lost(bool value) { aurora::g_config.pauseOnFocusLost = value; }
void aurora_set_background_input(bool value) {
  aurora::g_config.allowJoystickBackgroundEvents = value;
  aurora::window::set_background_input(value);
}
void aurora_set_resampler(AuroraSampler sampler) {
#ifdef AURORA_ENABLE_GX
  aurora::webgpu::set_resampler(sampler);
#else
  (void)sampler;
#endif
}







