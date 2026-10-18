#pragma once
#include "sample_cursor.hpp"
#include <span>

namespace aurora::audio {
struct MixerVoice {
  SampleCursor cursor;
  uint32_t ratio=65536,phase=0; // Source samples per output sample, 16.16.
  uint16_t left=32768,right=32768;
  int16_t current=0,next=0;
  bool primed=false,tail=false,active=true,failed=false;
};
inline bool prime_voice(MixerVoice& v,SampleRead read,void* context){
  if(v.ratio>8*65536 || v.phase>=65536 || v.left>32768 || v.right>32768)return false;
  if(v.primed)return true;
  if(!pull_sample(v.cursor,read,context,v.current))return false;
  if(!v.cursor.running){v.next=v.current;v.tail=true;}
  else if(!pull_sample(v.cursor,read,context,v.next))return false;
  v.primed=true;return true;
}
// Caller owns voice state. No game callbacks run from an audio device thread.
// Linear SRC/direct stereo buses only; AX envelopes/aux/ITD are later layers.
inline bool mix_stereo_frame(std::span<MixerVoice> voices,SampleRead read,void* context,
                            int16_t& left,int16_t& right){
  if(voices.size()>64 || !read)return false;
  int64_t l=0,r=0;
  for(auto& v:voices){
    if(!v.active)continue;
    if(!prime_voice(v,read,context)){v.failed=true;v.active=false;return false;}
    const int64_t value=(int64_t(v.current)*(65536-v.phase)+int64_t(v.next)*v.phase)/65536;
    l+=value*v.left/32768;r+=value*v.right/32768;
    v.phase+=v.ratio;
    while(v.phase>=65536 && v.active){
      v.phase-=65536;
      if(v.tail){v.active=false;break;}
      v.current=v.next;
      if(!v.cursor.running){v.next=v.current;v.tail=true;}
      else if(!pull_sample(v.cursor,read,context,v.next)){v.failed=true;v.active=false;return false;}
    }
  }
  left=static_cast<int16_t>(std::clamp(l,int64_t{-32768},int64_t{32767}));
  right=static_cast<int16_t>(std::clamp(r,int64_t{-32768},int64_t{32767}));return true;
}
}
