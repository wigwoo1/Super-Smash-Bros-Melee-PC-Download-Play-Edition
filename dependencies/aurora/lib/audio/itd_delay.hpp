#pragma once
#include <array>
#include <cstdint>
#include <dolphin/ax.h>
namespace aurora::audio {
// Native bounded interaural delay: Melee's pan table uses 0..31 samples.
// Target changes interpolate over 160 source-output frames. This transition
// policy is native behavior; exact original DSP transition fidelity is pending.
struct ItdDelay {
  struct Channel {int32_t delayQ16=0;uint16_t target=0,remaining=0;};
  std::array<int16_t,32> history{};uint32_t head=0;Channel left,right;bool ready=false;
  static void retarget(Channel& c,uint16_t target){if(c.target!=target){c.target=target;c.remaining=160;}}
  int16_t read(Channel& c){
    const unsigned whole=static_cast<uint32_t>(c.delayQ16)>>16;
    const unsigned fraction=static_cast<uint32_t>(c.delayQ16)&65535;
    const int64_t a=history[(head+32-whole)%32],b=history[(head+31-whole)%32];
    const int64_t weighted=a*(65536-fraction)+b*fraction;
    const int16_t result=static_cast<int16_t>(weighted>=0?weighted/65536:-((-weighted+65535)/65536));
    if(c.remaining){c.delayQ16+=(int32_t(c.target)*65536-c.delayQ16)/c.remaining;--c.remaining;}
    return result;
  }
  bool apply(AXPBITD& pb,bool reset,int16_t sample,int16_t& l,int16_t& r){
    if(pb.flag!=1 || pb.shiftL>31 || pb.shiftR>31 || pb.targetShiftL>31 || pb.targetShiftR>31)return false;
    if(!ready || reset){*this={};ready=true;left.delayQ16=pb.shiftL*65536;right.delayQ16=pb.shiftR*65536;left.target=pb.shiftL;right.target=pb.shiftR;}
    retarget(left,pb.targetShiftL);retarget(right,pb.targetShiftR);
    history[head]=sample;l=read(left);r=read(right);head=(head+1)%32;
    pb.shiftL=static_cast<uint16_t>(left.delayQ16>>16);pb.shiftR=static_cast<uint16_t>(right.delayQ16>>16);return true;
  }
};
}
