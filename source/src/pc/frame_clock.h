#ifndef MELEE_PC_FRAME_CLOCK_H
#define MELEE_PC_FRAME_CLOCK_H
#include <stdint.h>
typedef struct {
    uint32_t presentation_hz;
    uint64_t simulation_phase;
    uint64_t presentation_phase;
} MeleePc_FrameClock;
/* Counts due steps without sleeping, invoking callbacks or discarding debt.
 * Caller supplies monotonic elapsed nanoseconds and processes all simulation steps.
 * Native presentation uses this phase arithmetic; gameplay steps remain owned
 * by the retained-debt PAD timer, so its simulation output is not a second timer. */
int MeleePc_FrameClockInit(MeleePc_FrameClock* clock, uint32_t presentation_hz);
int MeleePc_FrameClockAdvance(MeleePc_FrameClock* clock, uint64_t elapsed_ns,
                             uint64_t* simulation_steps, uint64_t* presentations);
typedef struct {
    MeleePc_FrameClock phase;
    uint64_t elapsed_ns, opportunities, pending, presented, missed;
} MeleePc_PresentationClock;
/* Absolute elapsed time since scene-clock initialization. Poll retains due
 * presentation opportunities while simulation/input has priority. Finish
 * coalesces every missed picture, including time spent rendering; it never
 * advances/discards gameplay debt and never bursts stale frames to catch up. */
int MeleePc_PresentationClockInit(MeleePc_PresentationClock*,uint32_t hz);
int MeleePc_PresentationClockPoll(MeleePc_PresentationClock*,uint64_t elapsed_ns);
int MeleePc_PresentationClockFinish(MeleePc_PresentationClock*,uint64_t elapsed_ns,int presented);
#endif
