#pragma once
#include "sample_decode.hpp"
#include <limits>
#include <dolphin/ar.h>

namespace aurora::audio {
enum class SampleFormat { DSP_ADPCM, PCM8, PCM16_BE };
// Addresses are nibble, byte or 16-bit word indices respectively. End is inclusive.
struct SampleCursor {
  SampleFormat format=SampleFormat::DSP_ADPCM;
  uint32_t current=0,end=0,loop=0;
  bool running=false,looping=false;
  uint8_t predictorScale=0,loopPredictorScale=0;
  ADPCMHistory history{},loopHistory{};
  std::array<int16_t,16> coefficients{};
};
using SampleRead=bool(*)(void*,uint32_t,uint8_t*,uint32_t);
// One sample is transactional: refused reads do not advance or modify output.
inline bool pull_sample(SampleCursor& cursor,SampleRead read,void* context,int16_t& output){
  /* End is an inclusive trigger, not an upper bound on the loop target.
   * Stream rings can jump above it until the owner updates the next end.
   * Every physical access still goes through the bounded read provider. */
  if(!read || !cursor.running || (!cursor.looping && cursor.current>cursor.end))return false;
  auto next=cursor;int16_t sample;
  switch(next.format){
  case SampleFormat::PCM8:{
    uint8_t byte;if(!read(context,next.current,&byte,1))return false;
    sample=static_cast<int16_t>((byte<128?int(byte):int(byte)-256)*256);break;
  }
  case SampleFormat::PCM16_BE:{
    if(next.current>UINT32_MAX/2)return false;
    uint8_t bytes[2];if(!read(context,next.current*2,bytes,2))return false;
    const int raw=(int(bytes[0])<<8)|bytes[1];
    sample=static_cast<int16_t>(raw<32768?raw:raw-65536);break;
  }
  case SampleFormat::DSP_ADPCM:{
    // Ordinary frames only; end/loop positions in a header require hardware
    // accelerator edge semantics and are explicitly refused here.
    if((next.end&15)<2 || (next.looping && (next.loop&15)<2))return false;
    if((next.current&15)==0){
      uint8_t header;if(!read(context,next.current/2,&header,1))return false;
      if(header>=128 || next.current>UINT32_MAX-2 ||
         (next.current<=next.end && next.end-next.current<2))return false;
      next.predictorScale=header;next.current+=2;
    }else if((next.current&15)==1)return false;
    if(next.predictorScale>=128)return false;
    uint8_t packed;if(!read(context,next.current/2,&packed,1))return false;
    int residual=(next.current&1)?packed&15:packed>>4;
    if(residual>=8)residual-=16;
    const unsigned predictor=next.predictorScale>>4;
    const int64_t prediction=int64_t(next.coefficients[2*predictor])*next.history.recent+
      int64_t(next.coefficients[2*predictor+1])*next.history.older+1024;
    const int64_t rounded=prediction>=0?prediction/2048:-((-prediction+2047)/2048);
    sample=static_cast<int16_t>(std::clamp(residual*(int64_t{1}<<(next.predictorScale&15))+rounded,
                                        int64_t{-32768},int64_t{32767}));
    next.history.older=next.history.recent;next.history.recent=sample;break;
  }
  default:return false;
  }
  if(next.current==next.end){
    if(next.looping){next.current=next.loop;next.history=next.loopHistory;next.predictorScale=next.loopPredictorScale;}
    else next.running=false;
  }else {if(next.current==UINT32_MAX)return false;++next.current;}
  cursor=next;output=sample;return true;
}
inline bool read_aram_sample_bytes(void*,uint32_t offset,uint8_t* output,uint32_t bytes){
  return ARReadStorage(offset,output,bytes)!=0;
}
inline bool pull_aram_sample(SampleCursor& cursor,int16_t& output){
  return pull_sample(cursor,read_aram_sample_bytes,nullptr,output);
}
}
