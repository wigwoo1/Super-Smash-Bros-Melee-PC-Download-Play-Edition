#pragma once
#include <atomic>
#include <cstdint>
#include <cstdlib>

namespace aurora::recording_diagnostics {
enum Flag : uint32_t { Startup=1, DrawUv=2 };
inline uint32_t read_environment() noexcept {
  return (std::getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") ? Startup : 0u) |
         (std::getenv("AURORA_DIAGNOSTIC_DRAW_UV") ? DrawUv : 0u);
}
struct State {
  std::atomic<uint32_t> flags{read_environment()};
  const bool legacy=std::getenv("AURORA_DIAGNOSTIC_LEGACY_RECORDING_ENV") != nullptr;
};
inline State& state() noexcept { static State value; return value; }
// The existing scene-owner command refresh also publishes these flags. A
// recorder/FIFO worker reads one atomic word; it never locks the CRT environment
// on each append or merged draw. The legacy route is only for matched A/B tests.
inline void refresh() noexcept {
  state().flags.store(read_environment(),std::memory_order_release);
}
inline bool enabled(Flag flag,const char* name) noexcept {
  auto& value=state();
  return value.legacy ? std::getenv(name)!=nullptr :
      (value.flags.load(std::memory_order_acquire)&flag)!=0;
}
} // namespace aurora::recording_diagnostics
