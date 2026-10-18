#pragma once
#include <cstdint>
#include <limits>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace aurora {
struct ThreadCpuSample { uint64_t nanoseconds=0; bool available=false; };
// Diagnostic current-thread CPU accounting, never a gameplay/wall clock.
// Unsupported platforms report unavailable; normal rendering does not use it.
inline ThreadCpuSample thread_cpu_sample() noexcept {
#ifdef _WIN32
  FILETIME created{},exited{},kernel{},user{};
  if(!GetThreadTimes(GetCurrentThread(),&created,&exited,&kernel,&user))return {};
  const uint64_t k=(uint64_t(kernel.dwHighDateTime)<<32)|kernel.dwLowDateTime;
  const uint64_t u=(uint64_t(user.dwHighDateTime)<<32)|user.dwLowDateTime;
  constexpr uint64_t max=std::numeric_limits<uint64_t>::max();
  if(k>max-u || k+u>max/100)return {};
  return {(k+u)*100,true};
#else
  return {};
#endif
}
} // namespace aurora
