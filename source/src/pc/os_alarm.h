#ifndef MELEE_PC_OS_ALARM_H
#define MELEE_PC_OS_ALARM_H
#include <dolphin/os/OSAlarm.h>
typedef OSTime (*MeleePc_AlarmTimeProvider)(void*);
/* Install once a stable monotonic OS-tick provider before OSInitAlarm. Owner thread only. Negative or regressing samples abort before dispatch. */
int MeleePc_InstallAlarmClock(MeleePc_AlarmTimeProvider provider,void* userdata);
/* Forwards caller context verbatim; does not emulate PPC register state.
 * Caller must validate native handler context requirements before integration. */
int MeleePc_DispatchAlarms(OSContext* context,unsigned budget);
/* Native owner-only extension; does not change the console OSAlarm ABI.
 * Capacity callback is read-only. NULL restores ordinary coalescing. */
int MeleePc_SetAlarmCatchUp(OSAlarm* alarm,int (*ready)(void*),void* data);
/* Owner-only read of the earliest queued OS-tick deadline. Returns zero for
 * no alarm, invalid output, uninitialized backend or wrong owner; leaves output
 * untouched. Does not sample the clock or dispatch, even while callbacks mask.
 * Host must combine this deadline with input/render wakeups and service gates. */
int MeleePc_GetAlarmDeadline(OSTime* deadline);
#endif


