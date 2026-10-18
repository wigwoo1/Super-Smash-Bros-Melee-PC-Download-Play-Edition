#ifndef MELEE_PC_ALARM_QUEUE_H
#define MELEE_PC_ALARM_QUEUE_H
#include <stdint.h>
#define MELEE_PC_ALARM_CAPACITY 64
typedef void (*MeleePc_AlarmCallback)(void*);
/* Owner-only, read-only capacity check. Must not schedule/cancel/pump alarms. */
typedef int (*MeleePc_AlarmReady)(void*);
typedef struct { uintptr_t key; int64_t fire,start,period; uint64_t order; MeleePc_AlarmCallback callback; void* userdata; int active; MeleePc_AlarmReady catch_up_ready; void* ready_data; } MeleePc_AlarmEntry;
typedef struct { MeleePc_AlarmEntry entries[MELEE_PC_ALARM_CAPACITY];uint64_t next_order;int pumping; } MeleePc_AlarmQueue;
/* All calls belong to one host owner thread. Not an OSAlarm ABI implementation. */
void MeleePc_AlarmQueueInit(MeleePc_AlarmQueue* queue);
int MeleePc_AlarmSchedule(MeleePc_AlarmQueue* queue,uintptr_t key,int64_t fire,int64_t period,MeleePc_AlarmCallback callback,void* userdata);
int MeleePc_AlarmCancel(MeleePc_AlarmQueue* queue,uintptr_t key);
/* Opt a live periodic alarm into retained debt, one callback per boundary.
 * False readiness retains its deadline and lets other alarms proceed.
 * NULL restores ordinary coalescing. Rescheduling also clears this policy. */
int MeleePc_AlarmSetCatchUp(MeleePc_AlarmQueue* queue,uintptr_t key,
                          MeleePc_AlarmReady ready,void* data);
/* Returns callbacks dispatched; -1 rejects invalid/reentrant pump. Bounded budget.
 * Ordinary periodic alarms skip missed boundaries, matching console scheduling.
 * Explicit catch-up policy retains boundaries but cannot reconstruct past input. */
int MeleePc_AlarmPump(MeleePc_AlarmQueue* queue,int64_t now,unsigned budget);
/* Returns 0 for empty/invalid queue without changing output. */
int MeleePc_AlarmNextDeadline(const MeleePc_AlarmQueue* queue,int64_t* ticks);
#endif

