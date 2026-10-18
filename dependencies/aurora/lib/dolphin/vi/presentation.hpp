#pragma once
#include "../../gx/xfb.hpp"
#include <atomic>
#include <memory>
#include <cstdint>
#include <SDL3/SDL.h>

namespace aurora::vi {
struct Presentation {
  uint64_t frame=0,sourceFrame=0;
  uint64_t selectedNs=0, preparedNs=0;
  std::atomic<uint64_t> resultNs{0};
  std::atomic<uint64_t> workerEntryNs{0}, acquireStartNs{0}, acquireEndNs{0};
  void* framebuffer=nullptr;
  bool black=true, changed=false;
  uint32_t count=0;
  uint64_t revision=0;
  std::atomic<unsigned> result{0}; // 0 pending, 1 accepted by presentation API, 2 failed.
  std::atomic<unsigned> phase{0}; // 0 queued, 1 present encoding, 2 finish, 3 submit, 4 present result.
  std::atomic<unsigned> presentStep{0}; // Encoding observation only; never controls presentation.
  // Optional timing probes for a blocked VI wait. These are diagnostic-only
  // when AURORA_DIAGNOSTIC_VI_TIMING is set; they never affect ticket state.
  std::atomic<uint64_t> encoderFinishStartedNs{0}, encoderFinishCompletedNs{0};
  std::atomic<uint64_t> queueSubmitStartedNs{0}, queueSubmitCompletedNs{0};
  std::atomic<uint64_t> queueCallbacksRegisteredNs{0};
  std::atomic<uint64_t> surfacePresentStartedNs{0}, surfacePresentCompletedNs{0};
};
struct PresentationState {
  bool active=false, failed=false, waiting=false, flushed=false, draftPinned=false;
  bool black=true, draftBlack=true;
  // Native GPU scene: VI callbacks are compatibility-clock ticks, not XFB
  // presentation acknowledgements. The last accepted legacy XFB stays pinned.
  bool snapshotClock=false;
  void* current=nullptr;
  void* draft=nullptr;
  uint64_t epoch=0, lastClock=0, nextSlot=1;
  uint32_t retraces=0;
  uint64_t flushRevision=0, selectedNs=0;
  void (*pre)(uint32_t)=nullptr;
  void (*post)(uint32_t)=nullptr;
  std::shared_ptr<Presentation> inflight;
  bool can_refresh_ready_xfb() const {
    return gx::xfb::on_owner() && active && !failed && waiting && !inflight &&
           !flushed && !draftPinned && !black && !draftBlack && current;
  }
  bool init(uint64_t now){
    if(!gx::xfb::on_owner() || active)return false;
    *this={};active=true;epoch=lastClock=now;return true;
  }
  bool set_framebuffer(void* address){
    if(!gx::xfb::on_owner() || !active || failed || inflight || !gx::xfb::pin(address))return false;
    snapshotClock=false; // A real legacy source change requires API acceptance.
    if(draftPinned && !gx::xfb::unpin(draft))return false;
    draft=address;draftPinned=true;
    if(std::getenv("AURORA_DIAGNOSTIC_VI_TIMELINE"))selectedNs=SDL_GetTicksNS();
    return true;
  }
  bool set_black(bool value){
    if(!gx::xfb::on_owner() || !active || failed)return false;
    if(value!=draftBlack)snapshotClock=false;
    draftBlack=value;return true;
  }
  bool flush(){
    if(!gx::xfb::on_owner() || !active || failed || (!draftBlack && !draft) ||
       flushRevision==UINT64_MAX)return false;
    snapshotClock=false;
    ++flushRevision;flushed=true;return true;
  }
  bool set_snapshot_clock(bool enabled){
    if(!gx::xfb::on_owner() || !active || failed)return false;
    if(!enabled){snapshotClock=false;return true;}
    if(snapshotClock)return true;
    // Never reclassify an unpublished/changed XFB or GPU-pending ticket as a
    // clock tick. HSD must drain CPU copies before requesting this handoff.
    if(inflight || flushed || draftPinned || black || draftBlack || !current)return false;
    snapshotClock=true;
    if(waiting){
      // Only an unchanged pre-retrace remains; pair its post callback without
      // promoting, acknowledging or unpinning any legacy framebuffer.
      waiting=false;++retraces;if(post)post(retraces);
    }
    return snapshotClock;
  }
  bool pulse(uint64_t now,bool enabled,bool allowPre){
    if(!gx::xfb::on_owner() || failed || now<lastClock)return false;
    if(!active)return true;
    lastClock=now;
    if(snapshotClock){
      if(inflight || draftPinned || flushed){failed=true;return false;}
      if(enabled && allowPre){
        const uint64_t elapsed=now-epoch;
        const uint64_t slot=elapsed/1000000000*60+(elapsed%1000000000)*60/1000000000;
        if(slot>=nextSlot){
          nextSlot=slot+1;waiting=true;
          if(pre)pre(retraces+1);
          // A callback may request a real source/black/mode change. Preserve
          // that pending transaction and return to ordinary presentation.
          if(!snapshotClock)return true;
          waiting=false;++retraces;if(post)post(retraces);
        }
      }
      return true;
    }
    if(inflight && enabled){
      const auto result=inflight->result.load(std::memory_order_acquire);
      if(result==2){failed=true;return false;}
      if(result==1){
        auto ticket=std::move(inflight);
        if(ticket->preparedNs){
          static unsigned records=0;
          if(records<8192)std::fprintf(stderr,
            "NATIVE_VI_TIMELINE record=%u frame=%llu source=%llu changed=%u selected_ns=%llu prepared_ns=%llu result_ns=%llu retired_ns=%llu finish_start_ns=%llu finish_end_ns=%llu submit_start_ns=%llu submit_end_ns=%llu callbacks_ns=%llu present_start_ns=%llu present_end_ns=%llu worker_ns=%llu acquire_start_ns=%llu acquire_end_ns=%llu\n",
            ++records,(unsigned long long)ticket->frame,(unsigned long long)ticket->sourceFrame,
            unsigned(ticket->changed),(unsigned long long)ticket->selectedNs,
            (unsigned long long)ticket->preparedNs,(unsigned long long)ticket->resultNs.load(std::memory_order_acquire),
            (unsigned long long)SDL_GetTicksNS(),
            (unsigned long long)ticket->encoderFinishStartedNs.load(std::memory_order_acquire),
            (unsigned long long)ticket->encoderFinishCompletedNs.load(std::memory_order_acquire),
            (unsigned long long)ticket->queueSubmitStartedNs.load(std::memory_order_acquire),
            (unsigned long long)ticket->queueSubmitCompletedNs.load(std::memory_order_acquire),
            (unsigned long long)ticket->queueCallbacksRegisteredNs.load(std::memory_order_acquire),
            (unsigned long long)ticket->surfacePresentStartedNs.load(std::memory_order_acquire),
            (unsigned long long)ticket->surfacePresentCompletedNs.load(std::memory_order_acquire),
            (unsigned long long)ticket->workerEntryNs.load(std::memory_order_acquire),
            (unsigned long long)ticket->acquireStartNs.load(std::memory_order_acquire),
            (unsigned long long)ticket->acquireEndNs.load(std::memory_order_acquire));
        }
        if(ticket->changed){
          if(draftPinned){
            if(current && !gx::xfb::unpin(current)){
              if(std::getenv("AURORA_DIAGNOSTIC_XFB_PIN_FAILURES")){
                const int oldSlot=gx::xfb::find(current),newSlot=gx::xfb::find(ticket->framebuffer);
                std::fprintf(stderr,
                  "AURORA_VI_RETIRE_UNPIN_FAILURE frame=%llu ticket_framebuffer=%p current=%p draft=%p "
                  "old_slot=%d old_pins=%u new_slot=%d new_pins=%u draft_pinned=%u revision=%llu\n",
                  static_cast<unsigned long long>(ticket->frame),ticket->framebuffer,current,draft,oldSlot,
                  oldSlot>=0?gx::xfb::buffers[oldSlot].pins:0u,newSlot,
                  newSlot>=0?gx::xfb::buffers[newSlot].pins:0u,unsigned(draftPinned),
                  static_cast<unsigned long long>(ticket->revision));
                std::fflush(stderr);
              }
              failed=true;return false;
            }
            current=ticket->framebuffer;draftPinned=false;
          }else if(ticket->framebuffer!=current){failed=true;return false;}
          black=ticket->black;
          // A newer black-only flush belongs to the next presentation. The
          // immutable accepted ticket must not consume that staged update.
          if(ticket->revision==flushRevision)flushed=false;
        }
        retraces=ticket->count;waiting=false;
        if(post)post(retraces);
      }
    }
    if(enabled && allowPre && !waiting && !inflight){
      const uint64_t elapsed=now-epoch;
      const uint64_t slot=elapsed/1000000000*60+(elapsed%1000000000)*60/1000000000;
      if(slot>=nextSlot){
        nextSlot=slot+1;waiting=true;
        if(pre)pre(retraces+1);
      }
    }
    return true;
  }
  std::shared_ptr<Presentation> prepare(uint64_t frame){
    if(!gx::xfb::on_owner() || !active || failed || snapshotClock || !waiting || inflight || !frame)return {};
    auto ticket=std::make_shared<Presentation>();ticket->frame=frame;ticket->count=retraces+1;
    ticket->changed=flushed;ticket->framebuffer=flushed?draft:current;
    ticket->revision=flushRevision;
    ticket->black=flushed?draftBlack:black;
    if(!ticket->black && !ticket->framebuffer)return {};
    if(!ticket->black){const int slot=gx::xfb::find(ticket->framebuffer);if(slot>=0)ticket->sourceFrame=gx::xfb::buffers[slot].contentFrame;}
    if(std::getenv("AURORA_DIAGNOSTIC_VI_TIMELINE")){
      ticket->selectedNs=flushed && draftPinned?selectedNs:0;
      ticket->preparedNs=SDL_GetTicksNS();
    }
    inflight=ticket;return ticket;
  }
  bool shutdown(){
    if(!gx::xfb::on_owner() || failed || inflight)return false;
    if(current && !gx::xfb::unpin(current))return false;
    if(draftPinned && !gx::xfb::unpin(draft))return false;
    *this={};return true;
  }
};
inline PresentationState presentation;
}
