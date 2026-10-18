#pragma once
#include <array>
#include <cstdint>
#include <dolphin/axfx.h>
namespace aurora::audio {
// SDK AXAux uses separate DSP write/read and CPU effect slots. A processed
// send returns two 5ms blocks later; callbacks must never modify the write slot.
struct AuxRing {
  using Block=std::array<std::array<long,160>,3>;
  std::array<Block,3> blocks{};
  unsigned write=0,read=1,cpu=2;
  void (*callback)(void*,void*)=nullptr;
  void* context=nullptr;
  void process(){
    if(callback){
      auto& b=blocks[cpu];AXFX_BUFFERUPDATE view{b[0].data(),b[1].data(),b[2].data()};
      callback(&view,context);
    }
    write=(write+1)%3;read=(read+1)%3;cpu=(cpu+1)%3;
  }
};
}
