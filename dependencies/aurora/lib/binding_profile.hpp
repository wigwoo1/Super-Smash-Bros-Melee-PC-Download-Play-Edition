#pragma once
#include "binding_diagnostics.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace aurora::binding_profile {
// Observational, opt-in counters local to the calling thread. The FIFO worker
// reports its own completed calls; initialization/presentation-thread work is
// not mixed in. Timings nest: build contains sampler and bind-group lookups.
enum Phase { Resolve, Build, Sampler, BindGroup, Count };
struct Sample { uint64_t calls=0, ns=0; };
struct Counters {
  std::array<Sample,Count> phases{};
  uint64_t emptyBuilds=0, samplerHits=0, samplerMisses=0;
  uint64_t bindHits=0, bindMisses=0;
};
inline thread_local Counters counters;
inline bool enabled() noexcept {
  static const bool value=std::getenv("AURORA_DIAGNOSTIC_BINDING_PROFILE") != nullptr &&
                          std::getenv("MELEE_DIAGNOSTIC_PACING_PROFILE") != nullptr;
  return value;
}
inline uint64_t now() noexcept {
  return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
}
inline uint64_t begin() noexcept { return enabled() ? now() : 0; }
inline void finish(Phase phase,uint64_t start) noexcept {
  if(!enabled())return;
  auto& sample=counters.phases[phase];
  sample.ns+=now()-start;++sample.calls;
}
inline void cache_result(bool sampler,bool hit) noexcept {
  if(!enabled())return;
  if(sampler)++(hit?counters.samplerHits:counters.samplerMisses);
  else ++(hit?counters.bindHits:counters.bindMisses);
}
inline void empty_build() noexcept { if(enabled())++counters.emptyBuilds; }
inline void report(uint64_t outerBindingsNs) noexcept {
  if(!enabled())return;
  const auto& c=counters;
  std::fprintf(stderr,"NATIVE_BINDING_PROFILE outer_ns=%llu env_lookup_legacy=%u",(unsigned long long)outerBindingsNs,
               unsigned(binding_diagnostics::snapshot().legacy));
  static constexpr const char* names[]={"resolve","build","sampler","bind_group"};
  for(unsigned i=0;i<Count;++i)
    std::fprintf(stderr," %s_calls=%llu %s_ns=%llu",names[i],(unsigned long long)c.phases[i].calls,
                 names[i],(unsigned long long)c.phases[i].ns);
  std::fprintf(stderr," empty_builds=%llu sampler_hits=%llu sampler_misses=%llu bind_hits=%llu bind_misses=%llu\n",
      (unsigned long long)c.emptyBuilds,(unsigned long long)c.samplerHits,(unsigned long long)c.samplerMisses,
      (unsigned long long)c.bindHits,(unsigned long long)c.bindMisses);
  counters={};
}
} // namespace aurora::binding_profile
