#pragma once
#include <atomic>
#include <cstdint>
#include <cstdlib>

namespace aurora::binding_diagnostics {
enum Flag : uint32_t { HdMip=1, Content=2, Bindings=4, DrawUv=8 };
inline uint32_t read_environment() noexcept {
  return (std::getenv("MELEE_DIAGNOSTIC_HD_MIP_TRACE") ? HdMip : 0u) |
         (std::getenv("AURORA_DIAGNOSTIC_TEXTURE_CONTENT") ? Content : 0u) |
         (std::getenv("AURORA_DIAGNOSTIC_TEXTURE_BINDINGS") ? Bindings : 0u) |
         (std::getenv("AURORA_DIAGNOSTIC_DRAW_UV") ? DrawUv : 0u);
}
struct State {
  std::atomic<uint32_t> flags{read_environment()};
  const bool legacy=std::getenv("AURORA_DIAGNOSTIC_LEGACY_BINDING_ENV") != nullptr;
};
inline State& state() noexcept { static State value;return value; }
// Existing command-flag refresh publishes dynamic diagnostic changes from the
// scene owner. A draw reads one coherent snapshot without environment locking.
inline void refresh() noexcept { state().flags.store(read_environment(),std::memory_order_release); }
struct Snapshot {
  uint32_t flags;
  bool legacy;
  bool enabled(Flag flag,const char* name) const noexcept {
    // Retain the previous lookup behavior solely for matched performance tests.
    return legacy ? std::getenv(name) != nullptr : (flags & flag) != 0;
  }
};
inline Snapshot snapshot() noexcept {
  auto& value=state();return {value.flags.load(std::memory_order_acquire),value.legacy};
}
} // namespace aurora::binding_diagnostics
