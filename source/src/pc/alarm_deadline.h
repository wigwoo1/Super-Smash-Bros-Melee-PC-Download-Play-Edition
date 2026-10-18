#ifndef MELEE_PC_ALARM_DEADLINE_H
#define MELEE_PC_ALARM_DEADLINE_H
#include <stdint.h>
int MeleePc_NextPeriodicDeadline(int64_t start, int64_t period, int64_t now,
                                 int64_t* deadline);
#endif
