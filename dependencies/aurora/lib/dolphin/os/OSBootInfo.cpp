#include "internal.hpp"
#include <dolphin/os.h>

void AuroraFillBootInfo() {
  if (OSBaseAddress == 0) {
    return;
  }

  const auto info = static_cast<OSBootInfo*>(OSPhysicalToCached(0));

  info->memorySize = aurora::g_config.mem1Size;
  // The native runtime exposes its allocated MEM1 size as the simulated
  // console size too; it does not emulate a separate smaller memory window.
  *static_cast<u32*>(OSPhysicalToCached(0xf0)) = aurora::g_config.mem1Size;
}
