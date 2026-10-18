#pragma once
#include "ax_sample_cursor.hpp"
#include "ax_src_coefficients.hpp"
namespace aurora::audio {
struct AxFirState {std::array<int16_t,4> history{};uint32_t phase=0;};
// Consume only the integer source advance, then convolve oldest-to-newest.
// A zero ratio reuses history without reading ARAM or changing the cursor.
inline bool ax_fir_frame(SampleCursor& cursor,AxFirState& fir,uint32_t ratio,
                         uint16_t bank,SampleRead read,void* context,int16_t& output){
  if(bank>=3 || ratio>4*65536 || fir.phase>=65536 || !read)return false;
  auto next=cursor;auto state=fir;
  const uint32_t position=state.phase+ratio;
  const unsigned consume=position>>16;state.phase=position&65535;
  for(unsigned i=0;i<consume;++i){
    int16_t sample=0;
    if(next.running && !pull_sample(next,read,context,sample))return false;
    for(unsigned j=0;j<3;++j)state.history[j]=state.history[j+1];
    state.history[3]=sample;
  }
  const auto& weights=ax_src_coefficients[bank][state.phase>>9];
  int64_t total=0;for(unsigned i=0;i<4;++i)total+=int64_t(state.history[i])*weights[i];
  // Define arithmetic-shift rounding for negative values without relying on
  // implementation-defined signed right shifts.
  const int64_t scaled=total>=0?total/32768:-((-total+32767)/32768);
  output=static_cast<int16_t>(std::clamp(scaled,int64_t{-32768},int64_t{32767}));
  cursor=next;fir=state;return true;
}
}
