#pragma once
#include <dolphin/ax.h>
#include <SDL3/SDL_thread.h>
#include <array>
#include <cstdint>
#include <limits>
namespace aurora::audio {
struct VoicePool {
  std::array<AXVPB,AX_MAX_VOICES> voices{};
  std::array<bool,AX_MAX_VOICES> used{},reserved{};
  std::array<uint64_t,AX_MAX_VOICES> age{};
  SDL_ThreadID owner=0;uint64_t serial=0;
  bool initialize(){if(owner)return false;owner=SDL_GetCurrentThreadID();return owner!=0;}
  bool on_owner() const{return owner && owner==SDL_GetCurrentThreadID();}
  int index(const AXVPB* voice) const{
    for(unsigned i=0;i<voices.size();i++)if(voice==&voices[i])return i;return -1;
  }
  bool owns(const AXVPB* voice) const{const int i=index(voice);return on_owner() && i>=0 && used[i] && !reserved[i];}
  AXVPB* acquire(uint32_t priority,void(*callback)(void*),uint32_t context){
    if(!on_owner() || priority<1 || priority>=AX_PRIORITY_STACKS || serial==UINT64_MAX)return nullptr;
    int candidate=-1;
    for(unsigned i=0;i<voices.size();i++)if(!used[i] && !reserved[i]){candidate=i;break;}
    if(candidate<0){
      for(unsigned i=0;i<voices.size();i++)if(!reserved[i] && uint32_t(voices[i].priority)<priority &&
          (candidate<0 || voices[i].priority<voices[candidate].priority ||
           (voices[i].priority==voices[candidate].priority && age[i]<age[candidate])))candidate=i;
      if(candidate<0)return nullptr;
      // Keep the old ABI data visible to the owner's drop callback; prevent
      // nested allocations from reusing this slot until that callback returns.
      reserved[candidate]=true;used[candidate]=false;
      if(voices[candidate].callback)voices[candidate].callback(&voices[candidate]);
      reserved[candidate]=false;
    }
    auto& voice=voices[candidate];voice={};voice.index=candidate;voice.priority=priority;
    voice.callback=callback;voice.userContext=context;used[candidate]=true;age[candidate]=++serial;
    return &voice;
  }
  bool release(AXVPB* voice){if(!owns(voice))return false;used[index(voice)]=false;voice->pb.state=0;return true;}
  bool set_priority(AXVPB* voice,uint32_t priority){
    if(!owns(voice) || priority<1 || priority>=AX_PRIORITY_STACKS)return false;
    voice->priority=priority;return true;
  }
  bool shutdown(){
    if(!on_owner())return false;
    for(bool r:reserved)if(r)return false;
    *this={};return true;
  }
};
}
