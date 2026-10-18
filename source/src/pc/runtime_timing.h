#ifndef MELEE_PC_RUNTIME_TIMING_H
#define MELEE_PC_RUNTIME_TIMING_H
#include <dolphin/os.h>
#include <stdlib.h>
/* Opt-in metadata only. No gameplay, callback, clock or pacing changes. */
static void MeleePc_ReportSlowPhase(const char* phase,OSTime start) {
    OSTime elapsed=OSGetTime()-start;
    if(elapsed>=OSMillisecondsToTicks(25) && getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") &&
       !getenv("MELEE_DIAGNOSTIC_QUIET_RUNTIME"))
        OSReport("NATIVE_SLOW_PHASE phase=%s ticks=%lld milliseconds=%lld\n",
                 phase,elapsed,OSTicksToMilliseconds(elapsed));
}
#endif
