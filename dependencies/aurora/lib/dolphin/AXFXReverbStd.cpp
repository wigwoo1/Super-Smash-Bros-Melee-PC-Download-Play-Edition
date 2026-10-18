#include <dolphin/axfx.h>
#include <aurora/audio.h>
#include "../internal.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <unordered_map>
static aurora::Module Log("aurora::axfx::reverb_std");
static_assert(sizeof(long)==4 && sizeof(float)==4);
namespace {
struct Allocation {std::array<void*,15> pointers{};unsigned count=0;void (*release)(void*)=nullptr;};
std::unordered_map<AXFX_REVERBSTD*,Allocation> effects;
bool valid(const AXFX_REVERBSTD& r){
  return std::isfinite(r.coloration) && r.coloration>=0 && r.coloration<=1 &&
    std::isfinite(r.time) && r.time>=0.01f && r.time<=10 &&
    std::isfinite(r.mix) && r.mix>=0 && r.mix<=1 &&
    std::isfinite(r.damping) && r.damping>=0 && r.damping<=1 &&
    std::isfinite(r.preDelay) && r.preDelay>=0 && r.preDelay<=0.1f &&
    static_cast<long>(r.preDelay*32000)!=1;
}
void release(Allocation& a){for(unsigned i=0;i<a.count;i++)a.release(a.pointers[i]);}
float* allocate(Allocation& a,size_t count){
  auto* p=static_cast<float*>(__AXFXAlloc(count*sizeof(float)));
  if(p){std::memset(p,0,count*sizeof(float));a.pointers[a.count++]=p;}return p;
}
bool line(Allocation& a,AXFX_REVSTD_DELAYLINE& d,long lag){
  d.length=(lag+2)*4;d.inputs=allocate(a,lag+2);d.inPoint=0;d.outPoint=8;d.lastOutput=0;
  return d.inputs!=nullptr;
}
// SDK cursors are byte offsets and feedback uses the previous lastOutput.
float step(AXFX_REVSTD_DELAYLINE& d,float input){
  d.inputs[d.inPoint/4]=input;const float output=d.inputs[d.outPoint/4];
  d.inPoint=(d.inPoint+4)%d.length;d.outPoint=(d.outPoint+4)%d.length;
  d.lastOutput=output;return output;
}
float allpass(AXFX_REVSTD_DELAYLINE& d,float input,float coefficient){
  const float old=d.lastOutput;const float stored=std::fma(coefficient,old,input);
  const float output=std::fma(-coefficient,stored,old);step(d,stored);return output;
}
long truncate_ppc(float sample){
  // fctiwz's indefinite result for nonfinite/out-of-range conversion.
  if(!std::isfinite(sample) || sample>=2147483648.0f || sample< -2147483648.0f)return std::numeric_limits<int32_t>::min();
  return static_cast<long>(sample);
}
}
extern "C" {
int AXFXReverbStdShutdown(AXFX_REVERBSTD* r){
  AURORA_ASSERT(aurora_ax_source_rate()!=0,"Reverb shutdown requires live audio owner");
  if(!r)return 0;
  if(auto it=effects.find(r);it!=effects.end()){release(it->second);effects.erase(it);}
  r->rv={};return 1;
}
int AXFXReverbStdInit(AXFX_REVERBSTD* r){
  AURORA_ASSERT(aurora_ax_source_rate()==32000,"SDK standard reverb requires 32kHz owner");
  if(!r || !valid(*r) || effects.contains(r))return 0;
  r->rv={};Allocation allocation;allocation.release=__AXFXFree;auto& v=r->rv;
  constexpr long lengths[4]={1789,1999,433,149};
  bool ok=true;
  for(unsigned channel=0;channel<3 && ok;channel++){
    for(unsigned i=0;i<2 && ok;i++){
      const auto index=channel*2+i;
      ok=line(allocation,v.C[index],lengths[i]) && line(allocation,v.AP[index],lengths[i+2]);
      v.combCoef[index]=std::pow(10.0f,(-3.0f*lengths[i])/(32000.0f*r->time));
    }
  }
  v.allPassCoeff=r->coloration;v.level=r->mix;
  v.damping=1.0f-std::fma(0.8f,std::max(r->damping,0.05f),0.05f);
  v.preDelayTime=static_cast<long>(32000.0f*r->preDelay);
  if(v.preDelayTime)for(unsigned i=0;i<3 && ok;i++){
    v.preDelayLine[i]=allocate(allocation,v.preDelayTime);v.preDelayPtr[i]=v.preDelayLine[i];ok=v.preDelayLine[i]!=nullptr;
  }
  if(!ok){release(allocation);v={};return 0;}
  effects.emplace(r,allocation);r->tempDisableFX=0;return 1;
}
int AXFXReverbStdSettings(AXFX_REVERBSTD* r){
  AURORA_ASSERT(aurora_ax_source_rate()==32000,"Reverb settings require 32kHz owner");
  if(!r || !valid(*r))return 0;
  AXFXReverbStdShutdown(r);return AXFXReverbStdInit(r);
}
void AXFXReverbStdCallback(AXFX_BUFFERUPDATE* b,AXFX_REVERBSTD* r){
  AURORA_ASSERT(aurora_ax_source_rate()==32000 && b && r && effects.contains(r),"Reverb callback requires initialized effect and 32kHz owner");
  AURORA_ASSERT(b->left && b->right && b->surround,"Reverb requires three buses");
  if(r->tempDisableFX)return;
  auto& v=r->rv;std::array<long*,3> buses{b->left,b->right,b->surround};
  const float wet=v.level*0.6f,dry=0.6f-wet;
  for(unsigned channel=0;channel<3;channel++)for(unsigned i=0;i<160;i++){
    const float original=static_cast<float>(buses[channel][i]);float input=original;
    if(v.preDelayTime){
      input=*v.preDelayPtr[channel];*v.preDelayPtr[channel]++=original;
      // Preserve the SDK assembly's wrap at the last allocation element;
      // its one-sample case is refused at initialization to avoid an overrun.
      if(v.preDelayPtr[channel]==v.preDelayLine[channel]+v.preDelayTime-1)v.preDelayPtr[channel]=v.preDelayLine[channel];
    }
    const auto base=channel*2;
    const float c0=step(v.C[base],std::fma(v.combCoef[base],v.C[base].lastOutput,input));
    const float c1=step(v.C[base+1],std::fma(v.combCoef[base+1],v.C[base+1].lastOutput,input));
    float sample=allpass(v.AP[base],c0+c1,v.allPassCoeff);
    sample=std::fma(v.damping,v.lpLastout[channel],sample*0.3f);v.lpLastout[channel]=sample;
    sample=allpass(v.AP[base+1],sample,v.allPassCoeff);
    buses[channel][i]=truncate_ppc(std::fma(wet,sample,dry*original));
  }
}
}
