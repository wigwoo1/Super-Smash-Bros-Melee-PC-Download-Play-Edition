#include "../lib/dolphin/vi/presentation.hpp"
#include <cassert>
#include <cstdio>
#include <SDL3/SDL.h>
using namespace aurora;
alignas(32) static uint8_t memory[2][64];
static unsigned before,after;
static bool changeSource;
static void pre(uint32_t count){
  assert(count==after+1);++before;
  if(changeSource){
    changeSource=false;
    assert(vi::presentation.set_framebuffer(memory[1]));
    assert(vi::presentation.set_black(false) && vi::presentation.flush());
  }
}
static void post(uint32_t count){assert(count==after+1);++after;}
static int other_thread(void*){assert(!vi::presentation.set_snapshot_clock(true));return 7;}
int main(){
  assert(gx::xfb::initialize());
  for(unsigned i=0;i<2;++i){
    assert(gx::xfb::register_buffer(memory[i],64,16,2));
    auto job=gx::xfb::begin(memory[i],i+1,16,2);assert(job);
    job->mapped.store(true);job->gpuDone.store(true);assert(gx::xfb::pump());
  }
  auto& s=vi::presentation;
  assert(!s.set_snapshot_clock(true) && s.init(0));s.pre=pre;s.post=post;
  assert(!s.set_snapshot_clock(true)); // Startup black/no accepted CPU source.
  assert(s.set_framebuffer(memory[0]) && s.set_black(false) && s.flush());
  assert(!s.set_snapshot_clock(true)); // Unpublished legacy changes.
  assert(s.pulse(16666667,true,true));auto ticket=s.prepare(3);assert(ticket);
  assert(!s.set_snapshot_clock(true)); // A pending API result may not be invented.
  ticket->result.store(1);assert(s.pulse(20000000,true,false));
  assert(s.current==memory[0] && !s.draftPinned && !s.inflight);
  assert(s.set_snapshot_clock(true) && s.snapshotClock);
  assert(s.pulse(33333334,true,true) && before==2 && after==2);
  assert(!s.waiting && !s.inflight && !s.prepare(4));
  assert(gx::xfb::buffers[0].pins==1 && gx::xfb::buffers[1].pins==0);
  assert(s.pulse(50000000,false,true) && before==2);
  assert(s.pulse(50000000,true,false) && before==2);
  assert(s.pulse(50000000,true,true) && before==3 && after==3);
  assert(s.pulse(1000000000,true,true) && before==4 && after==4);
  // Real source changes made by a compatibility callback restore ordinary VI
  // ownership: the old source cannot unpin until the new API result arrives.
  changeSource=true;assert(s.pulse(1016666667,true,true));
  assert(!s.snapshotClock && s.waiting && s.draftPinned && s.current==memory[0]);
  assert(gx::xfb::buffers[0].pins==1 && gx::xfb::buffers[1].pins==1);
  ticket=s.prepare(5);assert(ticket && ticket->framebuffer==memory[1]);
  assert(!s.set_snapshot_clock(true));ticket->result.store(1);
  assert(s.pulse(1020000000,true,false));
  assert(s.current==memory[1] && gx::xfb::buffers[0].pins==0 && gx::xfb::buffers[1].pins==1);
  // An unchanged, unpublished pre callback can be paired at the explicit clock
  // handoff, with no XFB promotion or new GPU/presentation acknowledgement.
  assert(s.pulse(1033333334,true,true) && s.waiting);
  assert(s.set_snapshot_clock(true) && before==after && !s.waiting && !s.inflight);
  assert(s.current==memory[1] && gx::xfb::buffers[1].pins==1);
  SDL_Thread* probe=SDL_CreateThread(other_thread,"snapshot-owner",nullptr);assert(probe);
  int result=0;SDL_WaitThread(probe,&result);assert(result==7);
  // Black transitions also restore the actual surface-acceptance path.
  assert(s.set_black(true) && s.flush() && !s.snapshotClock && !s.black);
  assert(!s.set_snapshot_clock(true));assert(s.pulse(1050000000,true,true));
  ticket=s.prepare(6);assert(ticket && ticket->black && !s.black);
  ticket->result.store(1);assert(s.pulse(1051000000,true,false) && s.black);
  assert(s.set_snapshot_clock(false) && s.shutdown());
  assert(gx::xfb::buffers[1].pins==0);
  puts("PASS: snapshot-scene handoff, retained CPU pins, independent clock callbacks, real VI transitions, owner gate and shutdown");
}
