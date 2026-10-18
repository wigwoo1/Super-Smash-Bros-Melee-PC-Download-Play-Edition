#ifndef MELEE_PC_TICK_CLOCK_H
#define MELEE_PC_TICK_CLOCK_H
#include <stdint.h>
/* Elapsed monotonic time only. No wall-clock epoch or timezone adjustment.
 * Returns 0 on invalid arguments/overflow without changing output. */
int MeleePc_NanosecondsToTicks(uint64_t elapsed_ns,uint32_t ticks_per_second,int64_t* ticks);
int MeleePc_TicksToNanosecondsCeil(int64_t ticks,uint32_t ticks_per_second,uint64_t* nanoseconds);
#endif

