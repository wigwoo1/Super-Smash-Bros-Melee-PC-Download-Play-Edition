#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <algorithm>

namespace aurora::audio {
struct ADPCMHistory { int16_t recent=0, older=0; };
// Ordinary Nintendo DSP blocks: one predictor/scale byte and fourteen nibbles.
// Voice address/loop semantics are deliberately handled by the voice layer.
inline bool decode_adpcm_block(std::span<const uint8_t> bytes,
    const std::array<int16_t,16>& coefficients, ADPCMHistory& history,
    std::span<int16_t> output) {
  if(bytes.size()!=8 || output.size()<14 || (bytes[0]>>4)>=8)return false;
  std::array<int16_t,14> decoded{};
  auto next=history;
  const unsigned predictor=bytes[0]>>4;
  const int64_t scale=int64_t{1}<<(bytes[0]&15);
  for(unsigned i=0;i<14;++i){
    const unsigned packed=bytes[1+i/2];
    int residual=(i&1)?packed&15:packed>>4;
    if(residual>=8)residual-=16;
    const int64_t prediction=int64_t(coefficients[2*predictor])*next.recent+
                             int64_t(coefficients[2*predictor+1])*next.older+1024;
    // Arithmetic right-shift semantics without signed-shift portability assumptions.
    const int64_t rounded=prediction>=0?prediction/2048:-((-prediction+2047)/2048);
    const int64_t sample=std::clamp(residual*scale+rounded,int64_t{-32768},int64_t{32767});
    decoded[i]=static_cast<int16_t>(sample);
    next.older=next.recent;next.recent=decoded[i];
  }
  std::copy(decoded.begin(),decoded.end(),output.begin());history=next;return true;
}
inline bool read_pcm8(std::span<const uint8_t> bytes,size_t index,int16_t& output){
  if(index>=bytes.size())return false;
  const int sample=bytes[index]<128?bytes[index]:int(bytes[index])-256;
  output=static_cast<int16_t>(sample*256);return true;
}
inline bool read_pcm16be(std::span<const uint8_t> bytes,size_t index,int16_t& output){
  if(index>=bytes.size()/2)return false;
  const unsigned raw=(unsigned(bytes[index*2])<<8)|bytes[index*2+1];
  output=static_cast<int16_t>(raw<32768?int(raw):int(raw)-65536);return true;
}
}
