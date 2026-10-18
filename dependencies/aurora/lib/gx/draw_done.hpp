#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <mutex>
#include <SDL3/SDL_thread.h>

namespace aurora::gx::draw_done {
struct Request { uint64_t frame=0; void (*callback)()=nullptr; bool complete=false; uint64_t order=0; };
inline std::array<Request,64> requests{};
inline std::mutex mutex;
inline std::atomic<SDL_ThreadID> owner{0};
inline bool failed=false;
inline uint64_t nextOrder=0;
inline int (*gate)()=nullptr;
inline bool (*readyProvider)(uint64_t)=nullptr;
inline bool on_owner(){return owner.load()!=0 && owner.load()==SDL_GetCurrentThreadID();}
inline bool install_gate(int (*provider)()){
  if(!on_owner() || !provider || gate || nextOrder)return false;
  gate=provider;return true;
}
inline bool delivery_enabled(){return on_owner() && gate && gate()!=0;}
// Owner-only query: -1 failed/unsubmitted active request, 0 drained, 1 pending.
inline int pending(uint64_t activeFrame){
  if(!on_owner())return -1;
  std::lock_guard lock(mutex);if(failed)return -1;
  bool any=false;for(const auto& item:requests)if(item.frame){if(item.frame==activeFrame)return -1;any=true;}
  return any?1:0;
}
bool wait(uint32_t timeoutMs);
inline void initialize(){std::lock_guard lock(mutex);for(const auto& request:requests)if(request.frame)std::abort();failed=false;nextOrder=0;gate=nullptr;readyProvider=nullptr;owner.store(SDL_GetCurrentThreadID());}
inline bool request(uint64_t frame,void (*callback)()){
  if(!on_owner() || !gate || !frame)return false;
  std::lock_guard lock(mutex);if(failed || nextOrder==UINT64_MAX)return false;
  for(auto& item:requests)if(!item.frame){item={frame,callback,false,nextOrder++};return true;}
  return false;
}
// May run on a spontaneous Dawn callback thread: never calls game code.
inline void complete(uint64_t frame,bool success){
  std::lock_guard lock(mutex);
  for(auto& item:requests)if(item.frame==frame){if(success)item.complete=true;else failed=true;}
}
inline bool pump(){
  if(!on_owner())return false;
  for(;;){void (*callback)()=nullptr;bool found=false;
    if(!delivery_enabled()){std::lock_guard lock(mutex);return !failed;}
    {std::lock_guard lock(mutex);if(failed)return false;
      // Preserve request order even if asynchronous completions arrive out of order.
      size_t earliest=requests.size();
      for(size_t i=0;i<requests.size();i++)if(requests[i].frame && (earliest==requests.size() || requests[i].order<requests[earliest].order))earliest=i;
      if(earliest==requests.size() || !requests[earliest].complete ||
        (readyProvider && !readyProvider(requests[earliest].frame)))return true;
      callback=requests[earliest].callback;requests[earliest]={};found=true;
    }
    if(found && callback)callback();
  }
}
inline bool shutdown(){
  const bool trace=std::getenv("AURORA_DIAGNOSTIC_COMPONENT_SHUTDOWN")!=nullptr;
  if(trace) std::fputs("DRAW_DONE_SHUTDOWN enter=1\n",stderr);
  if(!on_owner()){if(trace)std::fputs("DRAW_DONE_SHUTDOWN owner=0 return=0\n",stderr);return false;}
  if(trace) std::fputs("DRAW_DONE_SHUTDOWN mutex_wait=1\n",stderr);
  std::lock_guard lock(mutex);
  if(trace) std::fputs("DRAW_DONE_SHUTDOWN mutex_acquired=1\n",stderr);
  unsigned pendingCount=0;
  for(const auto& item:requests) if(item.frame) {
    ++pendingCount;
    if(trace) std::fprintf(stderr,"DRAW_DONE_SHUTDOWN_REQUEST frame=%llu complete=%u order=%llu callback=%u\n",static_cast<unsigned long long>(item.frame),unsigned(item.complete),static_cast<unsigned long long>(item.order),unsigned(item.callback!=nullptr));
  }
  if(trace) std::fprintf(stderr,"DRAW_DONE_SHUTDOWN pending=%u failed=%u return=%u\n",pendingCount,unsigned(failed),unsigned(pendingCount==0 && !failed));
  if(pendingCount)return false;
  owner.store(0);return !failed;
}
}
