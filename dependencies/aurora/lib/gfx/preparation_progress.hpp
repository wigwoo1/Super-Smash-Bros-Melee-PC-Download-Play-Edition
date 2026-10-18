#pragma once
#include <cstdint>
#include <mutex>
namespace aurora::gfx::preparation_progress {
struct Snapshot { uint64_t generation=0; unsigned done=0,total=0; bool active=false; };
inline std::mutex mutex;
inline Snapshot state;
inline void begin(unsigned total) { std::lock_guard lock{mutex}; ++state.generation; state.done=0; state.total=total; state.active=total>0; }
inline void advance() { std::lock_guard lock{mutex}; if(state.active && state.done<state.total) ++state.done; }
inline void finish() { std::lock_guard lock{mutex}; state.active=false; }
inline Snapshot snapshot() { std::lock_guard lock{mutex}; return state; }
}
