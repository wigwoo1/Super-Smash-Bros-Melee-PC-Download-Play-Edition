#include <dolphin/axfx.h>
#include <aurora/audio.h>
#include "../internal.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <unordered_map>
static aurora::Module Log("aurora::axfx::reverb_hi");
static_assert(sizeof(long)==4 && sizeof(float)==4);
static_assert(sizeof(AXFX_REVHI_WORK)==0x1c4 && sizeof(AXFX_REVERBHI)==0x1e0);
namespace {
struct Allocation {std::array<void*,21> pointers{};unsigned count=0;void (*release)(void*)=nullptr;};
std::unordered_map<AXFX_REVERBHI*,Allocation> effects;
bool valid(const AXFX_REVERBHI& r){
  return std::isfinite(r.coloration) && r.coloration>=0 && r.coloration<=1 &&
    std::isfinite(r.time) && r.time>=0.01f && r.time<=10 &&
    std::isfinite(r.mix) && r.mix>=0 && r.mix<=1 &&
    std::isfinite(r.damping) && r.damping>=0 && r.damping<=1 &&
    std::isfinite(r.preDelay) && r.preDelay>=0 && r.preDelay<=0.1f &&
    static_cast<long>(r.preDelay*32000)!=1 && std::isfinite(r.crosstalk) && r.crosstalk>=0 && r.crosstalk<=1;
}
void release(Allocation& a){for(unsigned i=0;i<a.count;i++)a.release(a.pointers[i]);}
float* allocate(Allocation& a,size_t count){
  auto* p=static_cast<float*>(__AXFXAlloc(count*sizeof(float)));
  if(p){std::memset(p,0,count*sizeof(float));a.pointers[a.count++]=p;}return p;
}
bool line(Allocation& a,AXFX_REVHI_DELAYLINE& d,long lag){
  d.length=(lag+2)*4;d.inputs=allocate(a,lag+2);d.inPoint=0;d.outPoint=8;d.lastOutput=0;
  return d.inputs!=nullptr;
}
// SDK cursors are byte offsets and feedback uses the previous lastOutput.
float step(AXFX_REVHI_DELAYLINE& d,float input){
  d.inputs[d.inPoint/4]=input;const float output=d.inputs[d.outPoint/4];
  d.inPoint=(d.inPoint+4)%d.length;d.outPoint=(d.outPoint+4)%d.length;
  d.lastOutput=output;return output;
}
float allpass(AXFX_REVHI_DELAYLINE& d,float input,float coefficient){
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
void DoCrossTalk(long* left,long* right,float cross,float inverse){
  AURORA_ASSERT(aurora_ax_source_rate()==32000 && left && right,"Crosstalk requires 32kHz owner and buses");
  AURORA_ASSERT(std::isfinite(cross) && std::isfinite(inverse) && cross>=0 && cross<=1 && inverse>=0 && inverse<=1,"Invalid crosstalk coefficients");
  const float rightCross=cross*0.6f,rightInverse=inverse*0.6f;
  const auto nearest=[](float value)->long{
    if(!std::isfinite(value) || value>=2147483648.0f || value< -2147483648.0f)return std::numeric_limits<int32_t>::min();
    const double lower=std::floor(double(value)),fraction=double(value)-lower;
    return static_cast<long>(lower+(fraction>0.5 || (fraction==0.5 && std::fmod(lower,2.0)!=0)?1:0));
  };
  for(unsigned i=0;i<160;i++){
    const float l=static_cast<float>(left[i]),r=static_cast<float>(right[i]);
    // Paired-single products round before addition; the right coefficient row
    // has SDK0.6 gain. fctiw conversion uses nearest/even under default FPSCR.
    volatile float ll=l*inverse,lr=r*cross,rl=l*rightCross,rr=r*rightInverse;
    left[i]=nearest(ll+lr);right[i]=nearest(rl+rr);
  }
}
int AXFXReverbHiShutdown(AXFX_REVERBHI* r){
  AURORA_ASSERT(aurora_ax_source_rate()!=0,"Reverb shutdown requires live audio owner");
  if(!r)return 0;
  if(auto it=effects.find(r);it!=effects.end()){release(it->second);effects.erase(it);}
  r->rv={};return 1;
}
int AXFXReverbHiInit(AXFX_REVERBHI* r){
  AURORA_ASSERT(aurora_ax_source_rate()==32000,"SDK high-quality reverb requires 32kHz owner");
  if(!r || !valid(*r) || effects.contains(r))return 0;
  r->rv={};Allocation allocation;allocation.release=__AXFXFree;auto& v=r->rv;
  constexpr long lengths[8]={1789,1999,2333,433,149,47,73,67};
  bool ok=true;
  for(unsigned channel=0;channel<3 && ok;channel++){
    for(unsigned i=0;i<3 && ok;i++){
      const auto index=channel*3+i;
      ok=line(allocation,v.C[index],lengths[i]) && line(allocation,v.AP[index],i<2?lengths[i+3]:lengths[channel+5]);
      v.combCoef[index]=std::pow(10.0f,(-3.0f*lengths[i])/(32000.0f*r->time));
    }
  }
  v.allPassCoeff=r->coloration;v.level=r->mix;v.crosstalk=r->crosstalk;
  v.damping=1.0f-std::fma(0.8f,std::max(r->damping,0.05f),0.05f);
  v.preDelayTime=static_cast<long>(32000.0f*r->preDelay);
  if(v.preDelayTime)for(unsigned i=0;i<3 && ok;i++){
    v.preDelayLine[i]=allocate(allocation,v.preDelayTime);v.preDelayPtr[i]=v.preDelayLine[i];ok=v.preDelayLine[i]!=nullptr;
  }
  if(!ok){release(allocation);v={};return 0;}
  effects.emplace(r,allocation);r->tempDisableFX=0;return 1;
}
int AXFXReverbHiSettings(AXFX_REVERBHI* r){
  AURORA_ASSERT(aurora_ax_source_rate()==32000,"Reverb settings require 32kHz owner");
  if(!r || !valid(*r))return 0;
  AXFXReverbHiShutdown(r);return AXFXReverbHiInit(r);
}
void AXFXReverbHiCallback(AXFX_BUFFERUPDATE* b,AXFX_REVERBHI* r){
  AURORA_ASSERT(aurora_ax_source_rate()==32000 && b && r && effects.contains(r),"Reverb callback requires initialized effect and 32kHz owner");
  AURORA_ASSERT(b->left && b->right && b->surround,"Reverb requires three buses");
  if(r->tempDisableFX)return;
  auto& v=r->rv;std::array<long*,3> buses{b->left,b->right,b->surround};
  if(v.crosstalk!=0)DoCrossTalk(b->left,b->right,v.crosstalk*0.5f,1.0f-v.crosstalk*0.5f);
  const float wet=v.level*0.6f,dry=0.6f-wet;
  for(unsigned channel=0;channel<3;channel++)for(unsigned i=0;i<160;i++){
    const float original=static_cast<float>(buses[channel][i]);float input=original;
    if(v.preDelayTime){
      input=*v.preDelayPtr[channel];*v.preDelayPtr[channel]++=original;
      // Preserve the SDK assembly's wrap at the last allocation element;
      // its one-sample case is refused at initialization to avoid an overrun.
      if(v.preDelayPtr[channel]==v.preDelayLine[channel]+v.preDelayTime-1)v.preDelayPtr[channel]=v.preDelayLine[channel];
    }
    const auto base=channel*3;
    const float c0=step(v.C[base],std::fma(v.combCoef[base],v.C[base].lastOutput,input));
    const float c1=step(v.C[base+1],std::fma(v.combCoef[base+1],v.C[base+1].lastOutput,input));
    const float c2=step(v.C[base+2],std::fma(v.combCoef[base+2],v.C[base+2].lastOutput,input));
    float sample=allpass(v.AP[base],(c0+c1)+c2,v.allPassCoeff);
    sample=allpass(v.AP[base+1],sample,v.allPassCoeff);
    sample=std::fma(v.damping,v.lpLastout[channel],sample*0.3f);v.lpLastout[channel]=sample;
    sample=allpass(v.AP[base+2],sample,v.allPassCoeff);
    buses[channel][i]=truncate_ppc(std::fma(wet,sample,dry*original));
  }
}
}
