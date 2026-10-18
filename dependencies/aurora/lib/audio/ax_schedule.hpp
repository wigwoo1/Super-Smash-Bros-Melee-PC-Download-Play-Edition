#pragma once
#include <cstdint>
namespace aurora::audio {
struct AxSchedule {
  static constexpr uint64_t blockNs=5000000;
  uint64_t consumedThrough=0;
  bool initialize(uint64_t now){consumedThrough=now;return now<=UINT64_MAX-blockNs;}
  bool due(uint64_t now,uint32_t& count) const{
    if(now<consumedThrough)return false;
    const auto blocks=(now-consumedThrough)/blockNs;
    // Host stalls/backpressure leave every sample tick pending. Bound work,
    // not elapsed time: commit alone advances the source audio timeline.
    count=static_cast<uint32_t>(blocks>20?20:blocks);return true;
  }
  bool commit(){if(consumedThrough>UINT64_MAX-blockNs)return false;consumedThrough+=blockNs;return true;}
};
}
