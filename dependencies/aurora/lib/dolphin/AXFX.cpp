#include <dolphin/axfx.h>
#include <aurora/audio.h>
#include "../internal.hpp"
#include <array>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

static aurora::Module Log("aurora::axfx");
static_assert(sizeof(long)==4,"AXFX buffers require 32-bit SDK samples");
namespace {
struct DelayAllocation { std::array<long*,3> buffers{}; void (*release)(void*)=nullptr; };
std::unordered_map<AXFX_DELAY*,DelayAllocation> delays;
bool parameters(const AXFX_DELAY& d) {
  for(unsigned i=0;i<3;i++)
    if(d.delay[i]<=5 || d.delay[i]>60000 || d.feedback[i]>100 || d.output[i]>100)return false;
  return true;
}
// Match the SDK's low 32-bit product followed by a signed arithmetic shift.
long multiply(long sample,u32 gain) {
  const auto low=static_cast<uint32_t>(static_cast<uint64_t>(static_cast<uint32_t>(sample))*gain);
  return std::bit_cast<int32_t>(low)>>7;
}
long add(long a,long b) { return std::bit_cast<int32_t>(static_cast<uint32_t>(a)+static_cast<uint32_t>(b)); }
}
extern "C" {
void* AXFXAllocFunction(size_t size){return std::malloc(size);}
void AXFXFreeFunction(void* p){std::free(p);}
void* (*__AXFXAlloc)(size_t)=AXFXAllocFunction;
void (*__AXFXFree)(void*)=AXFXFreeFunction;
void AXFXSetHooks(void* (*allocate)(size_t),void (*release)(void*)) {
  AURORA_ASSERT(aurora_ax_source_rate()!=0,"AXFX hooks require live audio owner");
  AURORA_ASSERT(allocate && release,"AXFX requires paired allocation hooks");
  __AXFXAlloc=allocate;__AXFXFree=release;
}
int AXFXDelayShutdown(AXFX_DELAY* d) {
  AURORA_ASSERT(aurora_ax_source_rate()!=0,"AXFX shutdown requires live audio owner");
  if(!d)return 0;
  if(auto it=delays.find(d);it!=delays.end()) {
    for(auto p:it->second.buffers)if(p)it->second.release(p);
    delays.erase(it);
  }
  d->left=d->right=d->sur=nullptr;return 1;
}
int AXFXDelaySettings(AXFX_DELAY* d) {
  AURORA_ASSERT(aurora_ax_source_rate()==32000,"SDK AXFX delay settings require 32kHz audio owner");
  if(!d || !parameters(*d))return 0;
  // As in the SDK, settings replace the old delay history. Allocation failure
  // leaves the effect inactive and reports failure; never return fake success.
  AXFXDelayShutdown(d);
  DelayAllocation allocation;allocation.release=__AXFXFree;
  for(unsigned i=0;i<3;i++) {
    d->currentSize[i]=((d->delay[i]-5)*32+159)/160;
    d->currentPos[i]=0;
    d->currentFeedback[i]=(d->feedback[i]*128)/100;
    d->currentOutput[i]=(d->output[i]*128)/100;
    const size_t bytes=static_cast<size_t>(d->currentSize[i])*160*sizeof(long);
    allocation.buffers[i]=static_cast<long*>(__AXFXAlloc(bytes));
    if(!allocation.buffers[i]) {
      for(auto p:allocation.buffers)if(p)allocation.release(p);
      return 0;
    }
    std::memset(allocation.buffers[i],0,bytes);
  }
  delays.emplace(d,allocation);
  d->left=allocation.buffers[0];d->right=allocation.buffers[1];d->sur=allocation.buffers[2];return 1;
}
int AXFXDelayInit(AXFX_DELAY* d) {
  AURORA_ASSERT(aurora_ax_source_rate()==32000,"SDK AXFX delay init requires 32kHz audio owner");
  if(!d || delays.contains(d))return 0;
  return AXFXDelaySettings(d);
}
void AXFXDelayCallback(AXFX_BUFFERUPDATE* buffers,AXFX_DELAY* d) {
  AURORA_ASSERT(buffers && d && delays.contains(d),"AXFX delay callback requires initialized effect");
  AURORA_ASSERT(buffers->left && buffers->right && buffers->surround,"AXFX delay requires three sample buses");
  AURORA_ASSERT(aurora_ax_source_rate()==32000,"SDK AXFX delay requires 160 samples at 32kHz");
  auto& allocation=delays.at(d);
  std::array<long*,3> samples{buffers->left,buffers->right,buffers->surround};
  for(unsigned channel=0;channel<3;channel++) {
    AURORA_ASSERT(d->currentPos[channel]<d->currentSize[channel],"AXFX delay cursor out of bounds");
    auto* history=allocation.buffers[channel]+d->currentPos[channel]*160;
    for(unsigned i=0;i<160;i++) {
      const auto old=history[i];history[i]=add(samples[channel][i],multiply(old,d->currentFeedback[channel]));
      samples[channel][i]=multiply(old,d->currentOutput[channel]);
    }
    d->currentPos[channel]=(d->currentPos[channel]+1)%d->currentSize[channel];
  }
}
}
