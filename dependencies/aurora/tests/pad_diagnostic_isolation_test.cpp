#include "../lib/pad_diagnostic_isolation.hpp"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main() {
  using aurora::input::PadDiagnosticIsolation;
  // An isolated-test flag on its own must never disable normal player input.
  for (bool requested : {false, true}) {
    auto normal = PadDiagnosticIsolation::configured(false, requested);
    assert(!normal.enabled && normal.accepts_keyboard());
    assert(normal.accepts_controller(0, 0));
    assert(normal.accepts_controller(3, 4));
    assert(normal.accepts_controller(4, 4));
  }
  auto legacyDiagnostic = PadDiagnosticIsolation::configured(true, false);
  assert(!legacyDiagnostic.enabled && legacyDiagnostic.accepts_keyboard());
  assert(legacyDiagnostic.accepts_controller(3, 4));
  auto isolated = PadDiagnosticIsolation::configured(true, true);
  assert(isolated.enabled && !isolated.accepts_keyboard());
  assert(!isolated.accepts_controller(0, 0)); // Before device creation: no ownership.
  assert(!isolated.accepts_controller(3, 0));
  assert(!isolated.accepts_controller(0, 4));
  assert(!isolated.accepts_controller(3, 4)); // Connected Xbox or other SDL device.
  assert(isolated.accepts_controller(4, 4)); // Only the exact diagnostic instance.
  assert(!isolated.accepts_controller(4, 5)); // Reassigned device does not retain access.
  puts("PASS: normal/legacy input retained, explicit two-flag opt-in, exact device ownership, precreation and keyboard isolation");
}
