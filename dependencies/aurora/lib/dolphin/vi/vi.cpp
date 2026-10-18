#include <dolphin/vi.h>

#include "../../window.hpp"
#include "aurora/math.hpp"
#include "vi_internal.hpp"
#include "acquire_wait.hpp"
#include "../../gx/draw_done.hpp"
#include "../../gx/gx.hpp"
#include "../../gfx/recording.hpp"
#include "../../internal.hpp"
#include <aurora/aurora.h>
#include <aurora/diagnostics.h>
#include <SDL3/SDL.h>
#include <vector>

#include <optional>
#include <cstdio>
#include <cstdlib>

static aurora::Module Log("aurora::vi");

namespace aurora::vi {
static void (*readyXfbRefresh)(void) = nullptr;
std::optional<GXRenderModeObj> g_renderMode;

Vec2<uint32_t> render_mode_size() noexcept {
  if (!g_renderMode) {
    return {640, 480};
  }
  return {g_renderMode->fbWidth, g_renderMode->efbHeight};
}

void configure(const GXRenderModeObj* rm) noexcept {
  const auto oldSize = render_mode_size();
  if (rm == nullptr) {
    g_renderMode.reset();
  } else {
    g_renderMode = *rm;
  }
  if (render_mode_size() != oldSize) {
    window::request_frame_buffer_resize();
  }
}

Vec2<uint32_t> configured_fb_size() noexcept {
  return render_mode_size();
}
bool pump(bool allowPre) noexcept {
  return presentation.pulse(SDL_GetTicksNS(),gx::draw_done::delivery_enabled(),allowPre);
}
bool present_source(uint64_t frame,gfx::TextureHandle& texture,std::shared_ptr<Presentation>& ticket){
  const bool trace = std::getenv("AURORA_DIAGNOSTIC_Z_FIFO") != nullptr;
  if (trace) std::fprintf(stderr,"Z_VI_SOURCE_PREPARE frame=%llu\n",static_cast<unsigned long long>(frame));
  if(!presentation.active)return false;
  // Only already-ready copies may be staged. Never wait, pump completion
  // callbacks, or rerun pre-retrace/game callbacks inside this recording.
  if (readyXfbRefresh && presentation.can_refresh_ready_xfb()) readyXfbRefresh();
  ticket=presentation.prepare(frame);
  const bool black=ticket?ticket->black:presentation.black;
  void* address=ticket?ticket->framebuffer:presentation.current;
  if (trace) std::fprintf(stderr,"Z_VI_SOURCE_SELECTED black=%u address=%p\n",unsigned(black),address);
  if(black){
    texture=gfx::new_render_texture(16,16,GX_TF_RGBA8,"Native VI black surface");
    if (trace) std::fputs("Z_VI_SOURCE_BLACK_CREATED\n",stderr);
    std::array<uint8_t,16*16*4> pixels{};
    for(size_t i=3;i<pixels.size();i+=4)pixels[i]=255;
    gfx::queue_texture_upload_data(pixels.data(),16*4,16,
      wgpu::TexelCopyTextureInfo{.texture=texture->texture},texture->size);
    if (trace) std::fputs("Z_VI_SOURCE_BLACK_UPLOADED\n",stderr);
  }else{
    const auto found=gx::g_gxState.copyTextures.find(address);
    if (trace) std::fputs("Z_VI_SOURCE_COPY_LOOKUP_END\n",stderr);
    if(found==gx::g_gxState.copyTextures.end() || !found->second.handle || !found->second.revision)return false;
    texture=found->second.handle;
  }
  return bool(texture);
}
bool shutdown() noexcept { return !presentation.active || presentation.shutdown(); }
} // namespace aurora::vi

extern "C" {
int aurora_set_ready_xfb_refresh(void (*callback)(void)) {
  // HSD installs callbacks during its startup recording. This pointer is
  // owner-only and is never captured by a published ticket or worker task.
  if (!aurora::gx::xfb::on_owner()) return 0;
  aurora::vi::readyXfbRefresh = callback;
  return 1;
}
void VIInit() {
  AURORA_ASSERT(aurora::gx::xfb::on_owner(),"VIInit requires native owner");
  // Original startup initializes VI before launch-button polling, then HSD
  // calls VIInit again. Preserve live presentation state on that repeated call.
  if (!aurora::vi::presentation.active)
    AURORA_ASSERT(aurora::vi::presentation.init(SDL_GetTicksNS()),"VIInit failed");
}
void VIConfigure(const GXRenderModeObj* rm) { aurora::vi::configure(rm); }
void VIConfigurePan(u16 xOrg, u16 yOrg, u16 width, u16 height) {
  (void)xOrg;
  (void)yOrg;
  (void)width;
  (void)height;
}
// This SDK query means a GameCube component cable is detected, not that
// the host GPU supports progressive presentation. Native graphics use host
// capability checks and have no console cable attachment.
u32 VIGetDTVStatus(){return 0;}
u32 VIGetTvFormat() { return 0; }
void VIFlush() { AURORA_ASSERT(aurora::vi::presentation.flush(),"VIFlush requires valid staged VI state"); }
void VISetNextFrameBuffer(void* framebuffer){
  // A GXCopyDisp allocation failure means this destination has no new
  // completed XFB. Consume only the matching destination failure, retain the
  // prior/current presentation (or startup black), and retry next game frame.
  const bool droppedCopy=aurora::gx::xfb::on_owner() &&
      aurora::gx::xfb::take_job_allocation_failure_for_buffer(framebuffer);
  const bool staged=aurora::vi::presentation.set_framebuffer(framebuffer);
  // The recovery exception covers one VI callback only. Leaving this armed
  // until some later callback happens to stage successfully can hide an
  // unrelated presentation-state failure forever (notably during startup).
  const bool failureGrace=aurora::gx::xfb::diagnosticVIFailureGrace;
  aurora::gx::xfb::diagnosticVIFailureGrace=false;
  if(!staged && std::getenv("AURORA_DIAGNOSTIC_XFB_ALLOC")){
    static unsigned rejectRecords;
    if(rejectRecords<16){
      const int slot=aurora::gx::xfb::find(framebuffer);
      const unsigned pins=slot>=0?aurora::gx::xfb::buffers[slot].pins:0;
      std::fprintf(stderr,
                   "AURORA_XFB_VI_STAGE_REJECT frame=%llu address=%p dropped_copy=%u grace=%u "
                   "current=%p draft=%p draft_pinned=%u inflight=%u state_failed=%u slot=%d pins=%u\n",
                   static_cast<unsigned long long>(aurora_diagnostic_recording_id()),framebuffer,
                   unsigned(droppedCopy),unsigned(failureGrace),aurora::vi::presentation.current,
                   aurora::vi::presentation.draft,unsigned(aurora::vi::presentation.draftPinned),
                   unsigned(bool(aurora::vi::presentation.inflight)),
                   unsigned(aurora::vi::presentation.failed),slot,pins);
      ++rejectRecords;
    }
  }
  if(!staged && (droppedCopy || failureGrace)){
    static uint64_t skippedFrames=0;
    ++skippedFrames;
    if(skippedFrames==1 || skippedFrames%120==0)
      std::fprintf(stderr,
                   "AURORA_XFB_FRAMEBUFFER_STAGE_SKIPPED allocation_failure=%llu frame=%llu\n",
                   static_cast<unsigned long long>(skippedFrames),
                   static_cast<unsigned long long>(aurora_diagnostic_recording_id()));
    return;
  }
  AURORA_ASSERT(staged,
                "VI framebuffer must be enrolled, completed and not pending presentation");
}
void VISetBlack(BOOL black){
  AURORA_ASSERT(aurora::vi::presentation.set_black(black!=0),"VI black staging requires valid native owner state");
}
void* VIGetCurrentFrameBuffer(){
  AURORA_ASSERT(aurora::gx::xfb::on_owner(),"VI framebuffer query requires owner");return aurora::vi::presentation.current;
}
void* VIGetNextFrameBuffer(){
  AURORA_ASSERT(aurora::gx::xfb::on_owner(),"VI framebuffer query requires owner");return aurora::vi::presentation.draft;
}
u32 VIGetRetraceCount(){
  AURORA_ASSERT(aurora::gx::xfb::on_owner(),"VI counter query requires owner");return aurora::vi::presentation.retraces;
}
VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback callback){
  AURORA_ASSERT(aurora::gx::xfb::on_owner() && aurora::vi::presentation.active,"VI callback requires initialized owner");
  auto old=aurora::vi::presentation.pre;aurora::vi::presentation.pre=callback;return old;
}
VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback callback){
  AURORA_ASSERT(aurora::gx::xfb::on_owner() && aurora::vi::presentation.active,"VI callback requires initialized owner");
  auto old=aurora::vi::presentation.post;aurora::vi::presentation.post=callback;return old;
}
void VIWaitForRetrace(){
  using namespace aurora;
  AURORA_ASSERT(gx::xfb::on_owner() && vi::presentation.active && gx::draw_done::delivery_enabled(),
                "VIWaitForRetrace requires initialized unmasked owner");
  auto wait_until=[&](const uint32_t target){
    const uint32_t before=vi::presentation.retraces;
    const uint64_t start=SDL_GetTicksNS();
    vi::AcquireWaitBudget budget;
    while(vi::presentation.retraces<target){
      const auto* event=aurora_update();
      for(;event && event->type!=AURORA_NONE;++event) {
        if(event->type==AURORA_EXIT) aurora_handle_exit();
      }
      AURORA_ASSERT(vi::pump(!gfx::detail::diagnostic_recording_id()),"VI presentation failed");
      if(!gfx::detail::diagnostic_recording_id() && vi::presentation.waiting && !vi::presentation.inflight){
        if(aurora_begin_frame())aurora_end_frame();
      }
      const auto& pending=vi::presentation.inflight;
      const uint64_t elapsed=SDL_GetTicksNS()-start;
      const auto decision=budget.evaluate(elapsed,vi::presentation.retraces>=target,
        pending && pending->phase.load(std::memory_order_acquire)==1 &&
          pending->presentStep.load(std::memory_order_acquire)==2,pending?pending->frame:0);
      if(decision==vi::AcquireWaitDecision::Ready)break;
      if(decision==vi::AcquireWaitDecision::GraceGranted)
        std::fprintf(stderr,"VI_ACQUIRE_GRACE frame=%llu before=%u target=%u current=%u elapsed_ns=%llu hard_limit_ns=%llu\n",
          (unsigned long long)budget.frame,before,target,vi::presentation.retraces,
          (unsigned long long)elapsed,(unsigned long long)vi::AcquireWaitBudget::hardNs);
      if(decision==vi::AcquireWaitDecision::TimedOut){
        uint64_t submitted=0,completed=0,failed=0,presented=0;
        aurora_diagnostic_gpu_counts(&submitted,&completed,&failed,&presented);
        Log.error("VI pending ticket: frame={} phase={} (0 queued,1 present encoding,2 finish,3 submit,4 result)",
          vi::presentation.inflight?vi::presentation.inflight->frame:0,
          vi::presentation.inflight?vi::presentation.inflight->phase.load(std::memory_order_acquire):0);
        Log.error("VI present step={} (0 callback start,1 surface lock,2 texture acquire,3 texture view,4 resample,5 bind group,6 blit,7 ImGui,8 finish)",
          vi::presentation.inflight?vi::presentation.inflight->presentStep.load(std::memory_order_acquire):0);
        Log.error("VI retrace timeout: before={} target={} current={} recording={} waiting={} inflight={} result={} flushed={} enabled={} next_slot={} submitted={} completed={} failed={} presented={}",
          before,target,vi::presentation.retraces,gfx::detail::diagnostic_recording_id(),vi::presentation.waiting,
          bool(vi::presentation.inflight),vi::presentation.inflight?vi::presentation.inflight->result.load(std::memory_order_acquire):0,
          vi::presentation.flushed,gx::draw_done::delivery_enabled(),vi::presentation.nextSlot,submitted,completed,failed,presented);
        if(std::getenv("AURORA_DIAGNOSTIC_VI_TIMING") && vi::presentation.inflight){
          const auto& ticket=*vi::presentation.inflight;
          const uint64_t now=SDL_GetTicksNS();
          const uint64_t finishStart=ticket.encoderFinishStartedNs.load(std::memory_order_acquire);
          const uint64_t finishDone=ticket.encoderFinishCompletedNs.load(std::memory_order_acquire);
          const uint64_t submitStart=ticket.queueSubmitStartedNs.load(std::memory_order_acquire);
          const uint64_t submitDone=ticket.queueSubmitCompletedNs.load(std::memory_order_acquire);
          const uint64_t callbacksDone=ticket.queueCallbacksRegisteredNs.load(std::memory_order_acquire);
          const uint64_t presentStart=ticket.surfacePresentStartedNs.load(std::memory_order_acquire);
          const uint64_t presentDone=ticket.surfacePresentCompletedNs.load(std::memory_order_acquire);
          const auto elapsed=[](uint64_t began,uint64_t ended,uint64_t current){
            return began ? (ended>=began && ended ? ended : current)-began : uint64_t{0};
          };
          Log.error("VI work timing: wait_ns={} finish_ns={} finish_pending={} submit_ns={} submit_pending={} callbacks_ns={} callbacks_pending={} present_ns={} present_pending={}",
            now-start,elapsed(finishStart,finishDone,now),finishStart && !finishDone,
            elapsed(submitStart,submitDone,now),submitStart && !submitDone,
            elapsed(submitDone,callbacksDone,now),submitDone && !callbacksDone,
            elapsed(presentStart,presentDone,now),presentStart && !presentDone);
        }
        AURORA_ASSERT(false,"VI wait timed out; inspect retrace/presentation state");
      }
      SDL_DelayNS(100000);
    }
  };
  if(window::has_pending_display_preset()){
    const bool traceDisplay=std::getenv("MELEE_DIAGNOSTIC_DISPLAY_PRESET_TRACE")!=nullptr;
    const auto displayTrace=[&](const char* stage){
      if(!traceDisplay)return;
      const int currentSlot=gx::xfb::find(vi::presentation.current);
      const int draftSlot=gx::xfb::find(vi::presentation.draft);
      std::fprintf(stderr,
        "NATIVE_DISPLAY_BOUNDARY stage=%s retraces=%u waiting=%u inflight=%u result=%u current=%p current_slot=%d current_pins=%u draft=%p draft_slot=%d draft_pins=%u draft_pinned=%u\n",
        stage,vi::presentation.retraces,unsigned(vi::presentation.waiting),
        unsigned(bool(vi::presentation.inflight)),
        vi::presentation.inflight?vi::presentation.inflight->result.load(std::memory_order_acquire):0,
        vi::presentation.current,currentSlot,currentSlot>=0?gx::xfb::buffers[currentSlot].pins:0u,
        vi::presentation.draft,draftSlot,draftSlot>=0?gx::xfb::buffers[draftSlot].pins:0u,
        unsigned(vi::presentation.draftPinned));
      std::fflush(stderr);
    };
    displayTrace("start");
    // A window-mode change recreates the presentation target. Retire any frame
    // already in flight and present at least one black frame before resizing.
    VISetBlack(1);
    displayTrace("set-black");
    VIFlush();
    displayTrace("flush-black");
    wait_until(vi::presentation.retraces+2);
    displayTrace("black-retraces-complete");
    AURORA_ASSERT(aurora_apply_display_preset_at_boundary(),
                  "Display preset could not be applied at a black VI boundary");
    displayTrace("resize-complete");
    // At initial VI startup there may not yet be an enrolled XFB to flush.
    // Leave the startup surface black until the first real framebuffer is
    // staged; a running menu has a current/next XFB and can unblack now.
    if(VIGetCurrentFrameBuffer() || VIGetNextFrameBuffer()){
      VISetBlack(0);
      displayTrace("set-nonblack");
      VIFlush();
      displayTrace("flush-nonblack");
    }
  }
  wait_until(vi::presentation.retraces+1);
}

void VISetWindowTitle(const char* title) { aurora::window::set_title(title); }
void VISetWindowFullscreen(bool fullscreen) { aurora::window::set_fullscreen(fullscreen); }
bool VIGetWindowFullscreen() { return aurora::window::get_fullscreen(); }
void VISetWindowSize(uint32_t width, uint32_t height) { aurora::window::set_window_size(width, height); }
void VISetWindowPosition(uint32_t x, uint32_t y) { aurora::window::set_window_position(x, y); }
void VICenterWindow() { aurora::window::center_window(); }
void VISetFrameBufferScale(float scale) { aurora::window::set_frame_buffer_scale(scale); }
}
