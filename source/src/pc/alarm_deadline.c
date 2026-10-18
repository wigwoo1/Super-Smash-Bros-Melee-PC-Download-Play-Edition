#include "alarm_deadline.h"
/* Matches console periodic InsertAlarm: retain a future start, otherwise choose
 * the first boundary strictly after now. Returns false on invalid/overflow. */
int MeleePc_NextPeriodicDeadline(int64_t start, int64_t period, int64_t now,
                                 int64_t* deadline)
{
    int64_t delta;
    if (!deadline || start < 0 || now < 0 || period <= 0) return 0;
    if (start >= now) { *deadline = start; return 1; }
    delta = period - (now - start) % period;
    if (now > INT64_MAX - delta) return 0;
    *deadline = now + delta;
    return 1;
}

