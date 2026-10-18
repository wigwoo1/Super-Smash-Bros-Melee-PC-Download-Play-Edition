#ifndef MELEE_PC_ALARM_CLOCK_H
#define MELEE_PC_ALARM_CLOCK_H
#include <stdint.h>
typedef struct { uint64_t origin_ns,last_ns;int64_t origin_ticks;uint32_t rate; } MeleePc_AlarmClock;
/* Host samples both origins once. Never derives ticks from local timezone. */
int MeleePc_AlarmClockInit(MeleePc_AlarmClock* clock,uint64_t origin_ns,int64_t origin_ticks,uint32_t rate);
/* Reversed time/overflow rejected without changing clock or output. */
int MeleePc_AlarmClockRead(MeleePc_AlarmClock* clock,uint64_t monotonic_ns,int64_t* ticks);
/* Convert an absolute OS-tick deadline into the host epoch, rounding upward.
 * Deadlines at/before the epoch map to origin_ns (already due). Read-only;
 * invalid state/deadline/overflow leaves output untouched. */
int MeleePc_AlarmClockDeadline(const MeleePc_AlarmClock* clock,int64_t ticks,uint64_t* ns);
#endif
