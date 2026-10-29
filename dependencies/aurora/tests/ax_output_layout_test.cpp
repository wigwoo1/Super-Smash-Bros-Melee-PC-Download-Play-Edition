#include "../lib/audio/ax_output_layout.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
using namespace aurora::audio;
struct Stream {
  int device=6,queue=0,ready=0;
  uint32_t source=2,rate=32000;
  uint64_t now=0;
  bool flushOk=true;
  unsigned flushes=0;
  std::vector<bool> results;
  std::vector<uint32_t> sets;
  int device_channels(){return device;}
  bool flush(){++flushes;return flushOk;}
  int queued(){return queue;}
  int available(){return ready;}
  uint64_t now_ms(){return now;}
  bool set_format(uint32_t channels,uint32_t hz){
    const unsigned index=sets.size();sets.push_back(channels);
    const bool ok=index>=results.size() || results[index];
    source=ok?channels:4;rate=hz;return ok;
  }
};
int main(){
  unsigned cases=0;
  for(uint32_t rate:{32000u,48000u}) {
    AxOutputLayout state;Stream io;
    assert(state.request(6));io.queue=16;
    assert(state.service(io,rate)==0 && state.channels==2 && state.pending);
    assert(io.sets.empty() && io.flushes==1);
    io.queue=0;io.ready=24;
    assert(state.service(io,rate)==0 && io.flushes==1);
    io.ready=0;
    assert(state.service(io,rate)==1 && state.channels==6 && !state.pending);
    assert(state.status==AURORA_AUDIO_OUTPUT_SURROUND && io.source==6 && io.rate==rate);
    assert(state.request(2));
    assert(state.service(io,rate)==1 && state.channels==2 && io.source==2);
    assert(state.status==AURORA_AUDIO_OUTPUT_STEREO);++cases;
  }
  for(int device:{0,1,2,4,8}) {
    AxOutputLayout state;Stream io;io.device=device;
    assert(state.request(6));
    assert(state.service(io,32000)==1 && state.channels==2 && state.requested==6);
    assert(state.status==(device==0?AURORA_AUDIO_OUTPUT_ERROR:AURORA_AUDIO_OUTPUT_UNAVAILABLE));
    assert(!io.flushes && io.sets.empty());++cases;
  }
  {
    AxOutputLayout state;Stream io;
    const auto before=state;
    assert(!state.request(4));assert(state.requested==before.requested && state.status==before.status);
    assert(state.service(io,32000)==1 && io.sets.empty());++cases;
  }
  {
    AxOutputLayout state;Stream io;io.results={false,true};
    assert(state.request(6));assert(state.service(io,32000)==1);
    assert(state.channels==2 && io.source==2 && state.status==AURORA_AUDIO_OUTPUT_ERROR);
    assert((io.sets==std::vector<uint32_t>{6,2}));++cases;
  }
  {
    AxOutputLayout state;Stream io;io.results={false,false};
    assert(state.request(6));assert(state.service(io,32000)==-1);
    assert(state.channels==2 && io.source==4 && !state.pending);
    assert(state.status==AURORA_AUDIO_OUTPUT_ERROR);++cases;
  }
  for(unsigned failure=0;failure<5;++failure) {
    AxOutputLayout state;Stream io;assert(state.request(6));
    if(failure==0)io.flushOk=false;
    if(failure==1)io.queue=-1;
    if(failure==2)io.ready=-1;
    if(failure==3){io.queue=1;assert(state.service(io,32000)==0);io.now=500;}
    if(failure==4){io.now=50;io.queue=1;assert(state.service(io,32000)==0);io.now=49;}
    assert(state.service(io,32000)==1 && state.channels==2 && io.source==2);
    assert(state.status==AURORA_AUDIO_OUTPUT_ERROR && !state.pending && io.sets.empty());++cases;
  }
  for(int device:{0,2}) {
    AxOutputLayout state;state.channels=state.requested=6;
    Stream io;io.source=6;io.device=device;
    state.device_changed(device);assert(state.pending);
    assert(state.service(io,32000)==1 && state.channels==2 && io.source==2);
    assert(state.requested==6 && state.status==(device==0?AURORA_AUDIO_OUTPUT_ERROR:AURORA_AUDIO_OUTPUT_UNAVAILABLE));++cases;
  }
  {
    AxOutputLayout state;Stream io;assert(state.request(6));
    assert(state.service(io,44100)==1 && state.status==AURORA_AUDIO_OUTPUT_ERROR);
    assert(io.sets.empty() && !io.flushes);++cases;
  }
  std::printf("AX_OUTPUT_LAYOUT_PASS cases=%u real_production_transaction=1 drain_before_format=1 rollback=1 terminal_unknown_format=1 device_fallback=1 bounded_stall=1 physical_device_simulated=1\n",cases);
}
