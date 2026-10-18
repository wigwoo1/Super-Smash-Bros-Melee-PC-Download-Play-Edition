#pragma once
#include <cstdint>

namespace aurora::input {
// Explicit diagnostic ownership only. Ordinary play accepts all assigned devices.
struct PadDiagnosticIsolation {
  bool enabled;
  static constexpr PadDiagnosticIsolation configured(bool sdlDiagnostic, bool requested) {
    return {sdlDiagnostic && requested};
  }
  constexpr bool accepts_keyboard() const { return !enabled; }
  constexpr bool accepts_controller(std::uint32_t instance, std::uint32_t ownedInstance) const {
    return !enabled || (ownedInstance != 0 && instance == ownedInstance);
  }
};
} // namespace aurora::input
