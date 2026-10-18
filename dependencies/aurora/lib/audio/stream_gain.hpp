#pragma once
#include <cstdint>
namespace aurora::audio {
inline int16_t stream_gain(int16_t sample,uint8_t volume){
  const int32_t gain=volume+(volume>>7); // 0..256, full volume is unity.
  const int32_t value=int32_t(sample)*gain;
  return static_cast<int16_t>(value>=0?value/256:-((-value+255)/256));
}
}
