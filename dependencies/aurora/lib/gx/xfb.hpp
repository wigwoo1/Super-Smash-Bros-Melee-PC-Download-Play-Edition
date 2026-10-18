#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <functional>
#include <new>
#include <span>
#include <vector>
#include <SDL3/SDL_thread.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace aurora::gx::xfb {
struct Job {
  uint64_t frame;
  uint32_t width=0,height=0; // Original enrolled CPU XFB extent, set by owner before publication.
  unsigned slot;
  std::atomic<bool> gpuDone{false}, mapped{false}, failed{false};
  std::span<uint8_t> rgba;
  std::function<void()> releaseReadback;
  Job(uint64_t id, unsigned index, uint32_t w, uint32_t h, std::span<uint8_t> storage)
      : frame(id), width(w), height(h), slot(index), rgba(storage) {}
  void prepare(uint64_t id, uint32_t w, uint32_t h, std::span<uint8_t> storage) noexcept {
    frame=id;width=w;height=h;rgba=storage;
    gpuDone.store(false,std::memory_order_relaxed);
    mapped.store(false,std::memory_order_relaxed);
    failed.store(false,std::memory_order_relaxed);
  }
};
struct Buffer { uint8_t* address=nullptr; size_t bytes=0; uint32_t width=0,height=0,revision=0,pins=0; uint64_t contentFrame=0; };
inline std::array<Buffer,3> buffers{};
// GXCopyDisp jobs are produced every presented game frame. Keep one bounded
// CPU readback payload per enrolled XFB slot rather than allocating ~1.2 MiB
// in every transient shared Job (especially important for the 32-bit build).
inline std::array<std::vector<uint8_t>,3> rgbaStorage{};
inline std::array<std::shared_ptr<Job>,3> jobs{};
// Recycle each completed job object when no asynchronous callback still owns it.
// This removes the normal per-frame make_shared allocation from the XFB path.
inline std::array<std::shared_ptr<Job>,3> jobPool{};
inline std::atomic<SDL_ThreadID> owner{0};
inline bool failed=false;
inline bool diagnosticJobFailureObserved=false;
inline bool diagnosticJobFailureRecoveryLogged=false;
inline bool diagnosticVIFailureGrace=false;
inline uint64_t diagnosticJobFailureFrame=0;
inline bool diagnosticJobPoolStatsEnabled=false;
inline bool diagnosticForceNewJobEachFrame=false;
inline uint64_t diagnosticJobPoolCreated=0,diagnosticJobPoolReused=0;
inline void* pendingJobAllocationFailureAddress=nullptr;
#ifdef AURORA_XFB_TESTING
inline bool failNextJobAllocationForTest=false;
#endif
inline bool on_owner(){return owner.load() && owner.load()==SDL_GetCurrentThreadID();}
inline bool diagnostic_fail_next_job_allocation(uint64_t frame){
  static uint64_t targetFrame=[](){
    const char* value=std::getenv("AURORA_DIAGNOSTIC_XFB_FAIL_NEXT_JOB");
    if(!value || !value[0])return uint64_t(0);
    char* end=nullptr;
    const unsigned long long parsed=std::strtoull(value,&end,10);
    return end && *end=='\0' && parsed ? uint64_t(parsed) : uint64_t(0);
  }();
  static bool pending=targetFrame!=0;
  if(!pending || (targetFrame>1 && frame<targetFrame))return false;
  pending=false;
  return true;
}
inline bool address_space_sampling_enabled(){
  static const bool enabled=[](){
    const char* value=std::getenv("AURORA_DIAGNOSTIC_XFB_MEMORY_SAMPLES");
    return value && value[0] && value[0]!='0';
  }();
  return enabled;
}
inline void log_address_space(const char* event,uint64_t frame){
#ifdef _WIN32
  MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);
  const BOOL memoryAvailable=GlobalMemoryStatusEx(&memory);
  SYSTEM_INFO system{};GetSystemInfo(&system);
  const uintptr_t maximum=reinterpret_cast<uintptr_t>(system.lpMaximumApplicationAddress);
  uintptr_t cursor=reinterpret_cast<uintptr_t>(system.lpMinimumApplicationAddress);
  uint64_t largestFree=0,totalFree=0;
  while(cursor<=maximum){
    MEMORY_BASIC_INFORMATION region{};
    if(!VirtualQuery(reinterpret_cast<const void*>(cursor),&region,sizeof(region)))break;
    if(region.State==MEM_FREE){
      largestFree=std::max<uint64_t>(largestFree,region.RegionSize);
      if(region.RegionSize<=UINT64_MAX-totalFree)totalFree+=region.RegionSize;
    }
    const uintptr_t base=reinterpret_cast<uintptr_t>(region.BaseAddress);
    if(region.RegionSize>UINTPTR_MAX-base)break;
    const uintptr_t next=base+region.RegionSize;
    if(next<=cursor)break;
    cursor=next;
  }
  std::fprintf(stderr,
               "AURORA_XFB_ADDRESS_SPACE event=%s frame=%llu memory_status=%u memory_load=%lu "
               "available_virtual=%llu available_pagefile=%llu largest_free_region=%llu "
               "total_free_regions=%llu application_limit=%llu\n",
               event,static_cast<unsigned long long>(frame),unsigned(memoryAvailable),
               memoryAvailable?memory.dwMemoryLoad:0ul,
               static_cast<unsigned long long>(memoryAvailable?memory.ullAvailVirtual:0),
               static_cast<unsigned long long>(memoryAvailable?memory.ullAvailPageFile:0),
               static_cast<unsigned long long>(largestFree),
               static_cast<unsigned long long>(totalFree),
               static_cast<unsigned long long>(maximum));
#else
  std::fprintf(stderr,"AURORA_XFB_ADDRESS_SPACE event=%s frame=%llu unsupported_platform\n",
               event,static_cast<unsigned long long>(frame));
#endif
}
inline bool initialize(){
  for(const auto& job:jobs)if(job)return false;
  jobs={};jobPool={};
  diagnosticJobPoolStatsEnabled=std::getenv("AURORA_DIAGNOSTIC_XFB_JOB_POOL_STATS")!=nullptr;
  diagnosticForceNewJobEachFrame=diagnosticJobPoolStatsEnabled &&
      std::getenv("AURORA_DIAGNOSTIC_XFB_FORCE_NEW_JOB")!=nullptr;
  diagnosticJobPoolCreated=diagnosticJobPoolReused=0;
  buffers={};for(auto& storage:rgbaStorage)std::vector<uint8_t>().swap(storage);
  diagnosticJobFailureObserved=false;diagnosticJobFailureRecoveryLogged=false;diagnosticVIFailureGrace=false;
  diagnosticJobFailureFrame=0;
  pendingJobAllocationFailureAddress=nullptr;
#ifdef AURORA_XFB_TESTING
  failNextJobAllocationForTest=false;
#endif
  failed=false;owner.store(SDL_GetCurrentThreadID());return true;
}
inline int find(const void* address){for(unsigned i=0;i<buffers.size();++i)if(buffers[i].address==address)return int(i);return -1;}
inline bool take_job_allocation_failure_for_buffer(void* address){
  if(!address || pendingJobAllocationFailureAddress!=address)return false;
  pendingJobAllocationFailureAddress=nullptr;
  return true;
}
inline bool register_buffer(void* address,size_t bytes,uint32_t width,uint32_t height){
  if(!on_owner() || !address || failed || (uintptr_t(address)&31) || !width || (width&15) ||
     width>640 || !height || height>480 || bytes<size_t(width)*height*2 || bytes>4*1024*1024)return false;
  const auto begin=uintptr_t(address);if(bytes>UINTPTR_MAX-begin)return false;
  for(const auto& b:buffers)if(b.address){const auto other=uintptr_t(b.address);
    if(begin<other+b.bytes && other<begin+bytes)return false;}
  for(unsigned i=0;i<buffers.size();++i)if(!buffers[i].address){
    try { rgbaStorage[i].resize(size_t(width)*height*4); }
    catch(const std::bad_alloc&) { return false; }
    buffers[i]={static_cast<uint8_t*>(address),bytes,width,height,0};return true;
  }
  return false;
}
inline bool unregister_buffer(void* address){
  if(!on_owner() || !address)return false;const int slot=find(address);
  if(slot<0 || jobs[slot] || buffers[slot].pins)return false;
  buffers[slot]={};std::vector<uint8_t>().swap(rgbaStorage[slot]);return true;
}
inline bool pin(void* address){
  if(!on_owner() || !address || failed)return false;const int slot=find(address);
  if(slot<0 || jobs[slot] || !buffers[slot].revision || buffers[slot].pins==UINT32_MAX)return false;
  ++buffers[slot].pins;return true;
}
inline bool unpin(void* address){
  if(!on_owner() || !address)return false;const int slot=find(address);
  if(slot<0 || !buffers[slot].pins){
    if(std::getenv("AURORA_DIAGNOSTIC_XFB_PIN_FAILURES")){
      std::fprintf(stderr,"AURORA_XFB_UNPIN_FAILURE address=%p slot=%d pins=%u owner=%u current_thread=%u\n",
                   address,slot,slot>=0?buffers[slot].pins:0u,unsigned(owner.load()),unsigned(SDL_GetCurrentThreadID()));
      std::fflush(stderr);
    }
    return false;
  }
  --buffers[slot].pins;return true;
}
inline std::shared_ptr<Job> begin(void* address,uint64_t frame,uint32_t width,uint32_t height,
                                  bool* allocationFailure=nullptr){
  if(allocationFailure)*allocationFailure=false;
  if(!on_owner() || !address || !frame || failed)return {};
  if(address_space_sampling_enabled() && frame%120==0)log_address_space("sample",frame);
  const int slot=find(address);if(slot<0 || jobs[slot] || buffers[slot].pins || buffers[slot].width!=width ||
    buffers[slot].height!=height || buffers[slot].revision==UINT32_MAX ||
    rgbaStorage[slot].size()!=size_t(width)*height*4)return {};
  try {
#ifdef AURORA_XFB_TESTING
    if(failNextJobAllocationForTest){failNextJobAllocationForTest=false;throw std::bad_alloc();}
#endif
    if(diagnostic_fail_next_job_allocation(frame)){
      diagnosticJobFailureObserved=true;diagnosticJobFailureFrame=frame;
      throw std::bad_alloc();
    }
    std::shared_ptr<Job> job;
    if(!diagnosticForceNewJobEachFrame && jobPool[slot] &&
       jobPool[slot].use_count()==1 && !jobPool[slot]->releaseReadback){
      job=jobPool[slot];
      job->prepare(frame,width,height,std::span<uint8_t>(rgbaStorage[slot]));
      if(diagnosticJobPoolStatsEnabled)++diagnosticJobPoolReused;
    }else{
      job=std::make_shared<Job>(frame,unsigned(slot),width,height,
                                std::span<uint8_t>(rgbaStorage[slot]));
      jobPool[slot]=job;
      if(diagnosticJobPoolStatsEnabled)++diagnosticJobPoolCreated;
    }
    jobs[slot]=job;
    if(diagnosticJobFailureObserved && !diagnosticJobFailureRecoveryLogged){
      std::fprintf(stderr,"AURORA_XFB_JOB_ALLOC_RECOVERED failed_frame=%llu recovered_frame=%llu\n",
                   static_cast<unsigned long long>(diagnosticJobFailureFrame),
                   static_cast<unsigned long long>(frame));
      diagnosticJobFailureRecoveryLogged=true;
    }
    return job;
  } catch (const std::bad_alloc&) {
    diagnosticJobFailureObserved=true;
    diagnosticVIFailureGrace=true;
    diagnosticJobFailureFrame=frame;
    pendingJobAllocationFailureAddress=address;
    if(allocationFailure)*allocationFailure=true;
    if(std::getenv("AURORA_DIAGNOSTIC_XFB_ALLOC")) {
      std::fprintf(stderr,
                   "AURORA_XFB_JOB_ALLOC_FAILURE frame=%llu slot=%d width=%u height=%u rgba_storage_bytes=%llu "
                   "revision=%u pins=%u active_jobs=%u\n",
                   static_cast<unsigned long long>(frame),slot,width,height,
                   static_cast<unsigned long long>(size_t(width)*size_t(height)*4),
                   buffers[slot].revision,buffers[slot].pins,
                   unsigned(bool(jobs[0]))+unsigned(bool(jobs[1]))+unsigned(bool(jobs[2])));
      for(unsigned i=0;i<buffers.size();++i) {
        const auto& b=buffers[i];const auto& active=jobs[i];
        std::fprintf(stderr,
                     "AURORA_XFB_SLOT index=%u enrolled=%u width=%u height=%u revision=%u pins=%u "
                     "job=%u use_count=%ld gpu_done=%u mapped=%u failed=%u\n",
                     i,unsigned(b.address!=nullptr),b.width,b.height,b.revision,b.pins,
                     unsigned(bool(active)),active?active.use_count():0L,
                     active?unsigned(active->gpuDone.load(std::memory_order_relaxed)):0,
                     active?unsigned(active->mapped.load(std::memory_order_relaxed)):0,
                     active?unsigned(active->failed.load(std::memory_order_relaxed)):0);
      }
      log_address_space("allocation_failure",frame);
      std::fflush(stderr);
    }
    // The caller may retain the previous XFB for this presentation. Do not
    // turn a transient host allocation failure into process termination.
    return {};
  }
}
// GPU callbacks only publish flags/data, never access the game's destination memory.
inline int clamp_byte(int x){return x<0?0:x>255?255:x;}
inline int luma(const uint8_t* p){return clamp_byte(((66*p[0]+129*p[1]+25*p[2]+128)>>8)+16);}
inline int chroma_u(const uint8_t* p){return ((-38*int(p[0])-74*int(p[1])+112*int(p[2])+128)>>8)+128;}
inline int chroma_v(const uint8_t* p){return ((112*int(p[0])-94*int(p[1])-18*int(p[2])+128)>>8)+128;}
// Native BT.601 limited-range YUYV representation. Hardware XFB bit-exactness is a separate gate.
inline void convert(const uint8_t* rgba,uint8_t* output,size_t pixels){
  for(size_t i=0;i<pixels;i+=2){const auto* a=rgba+i*4;const auto* b=a+4;
    output[i*2]=uint8_t(luma(a));output[i*2+1]=uint8_t(clamp_byte((chroma_u(a)+chroma_u(b)+1)/2));
    output[i*2+2]=uint8_t(luma(b));output[i*2+3]=uint8_t(clamp_byte((chroma_v(a)+chroma_v(b)+1)/2));}
}
inline bool pump(){
  if(!on_owner() || failed)return false;
  for(unsigned i=0;i<jobs.size();++i)if(auto job=jobs[i]){
    if(job->failed.load(std::memory_order_acquire)){failed=true;return false;}
    if(job->gpuDone.load(std::memory_order_acquire) && job->mapped.load(std::memory_order_acquire)){
      if(job->releaseReadback){job->releaseReadback();job->releaseReadback={};}
      auto& b=buffers[i];convert(job->rgba.data(),b.address,size_t(b.width)*b.height);
      ++b.revision;b.contentFrame=job->frame;jobs[i].reset();
    }
  }
  return true;
}
inline bool frame_ready(uint64_t frame){
  if(!on_owner() || failed)return false;
  for(const auto& job:jobs)if(job && job->frame==frame)return false;
  return true;
}
inline std::array<std::shared_ptr<Job>,3> frame_jobs(uint64_t frame){
  std::array<std::shared_ptr<Job>,3> result{};
  // Called by the owner before handing immutable handles to the worker.
  if(on_owner())for(size_t i=0;i<jobs.size();++i)if(jobs[i] && jobs[i]->frame==frame)result[i]=jobs[i];
  return result;
}
inline bool revision(void* address,uint32_t* value){
  if(!on_owner() || !address || !value || failed)return false;
  const int slot=find(address);if(slot<0 || jobs[slot])return false;*value=buffers[slot].revision;return true;
}
inline bool shutdown(){
  if(!on_owner() || !pump())return false;for(const auto& job:jobs)if(job)return false;
  if(diagnosticJobPoolStatsEnabled)std::fprintf(stderr,
      "AURORA_XFB_JOB_POOL created=%llu reused=%llu\n",
      static_cast<unsigned long long>(diagnosticJobPoolCreated),
      static_cast<unsigned long long>(diagnosticJobPoolReused));
  jobs={};jobPool={};buffers={};for(auto& storage:rgbaStorage)std::vector<uint8_t>().swap(storage);
  owner.store(0);return true;
}
}
