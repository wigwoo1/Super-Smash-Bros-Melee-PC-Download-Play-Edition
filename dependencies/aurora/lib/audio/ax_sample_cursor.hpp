#pragma once
#include "sample_cursor.hpp"
#include <dolphin/ax.h>
#include <bit>
namespace aurora::audio {
inline uint32_t ax_address(uint16_t hi,uint16_t lo){return (uint32_t(hi)<<16)|lo;}
// Translate host-order PB fields, not raw big-endian bank bytes. Playback state
// and resampler lookahead stay separate; this snapshot does not advance the PB.
inline bool ax_sample_cursor(const AXPB& pb,SampleCursor& output){
  SampleCursor next;
  switch(pb.addr.format){case 0:next.format=SampleFormat::DSP_ADPCM;break;case 10:next.format=SampleFormat::PCM16_BE;break;case 25:next.format=SampleFormat::PCM8;break;default:return false;}
  if(pb.state>1 || pb.addr.loopFlag>1)return false;
  next.current=ax_address(pb.addr.currentAddressHi,pb.addr.currentAddressLo);
  next.end=ax_address(pb.addr.endAddressHi,pb.addr.endAddressLo);
  next.loop=ax_address(pb.addr.loopAddressHi,pb.addr.loopAddressLo);
  next.running=pb.state==1;next.looping=pb.addr.loopFlag==1;
  if(!next.looping && next.current>next.end)return false;
  if(pb.addr.format==10 && (next.current>UINT32_MAX/2 || next.end>UINT32_MAX/2 ||
                            (next.looping && next.loop>UINT32_MAX/2)))return false;
  if(pb.addr.format==0){
    if((next.current&15)<2 || (next.end&15)<2 || (next.looping && (next.loop&15)<2) || pb.adpcm.pred_scale>=128 || (next.looping && pb.adpcmLoop.loop_pred_scale>=128))return false;
    next.predictorScale=pb.adpcm.pred_scale;next.loopPredictorScale=pb.adpcmLoop.loop_pred_scale;
    next.history={std::bit_cast<int16_t>(pb.adpcm.yn1),std::bit_cast<int16_t>(pb.adpcm.yn2)};
    next.loopHistory={std::bit_cast<int16_t>(pb.adpcmLoop.loop_yn1),std::bit_cast<int16_t>(pb.adpcmLoop.loop_yn2)};
    for(unsigned i=0;i<16;i++)next.coefficients[i]=std::bit_cast<int16_t>(pb.adpcm.a[i/2][i%2]);
  }
  output=next;return true;
}
}
