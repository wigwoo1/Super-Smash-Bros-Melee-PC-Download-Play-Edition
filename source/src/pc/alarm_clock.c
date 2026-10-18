#include "alarm_clock.h"
#include "tick_clock.h"
#include <limits.h>
int MeleePc_AlarmClockDeadline(const MeleePc_AlarmClock* clock,int64_t ticks,uint64_t* ns){
    uint64_t elapsed=0;
    if(!clock || !ns || !clock->rate || clock->origin_ticks<0 || ticks<0 || clock->last_ns<clock->origin_ns)return 0;
    if(ticks>clock->origin_ticks && !MeleePc_TicksToNanosecondsCeil(ticks-clock->origin_ticks,clock->rate,&elapsed))return 0;
    if(elapsed>UINT64_MAX-clock->origin_ns)return 0;
    *ns=clock->origin_ns+elapsed;return 1;
}
int MeleePc_AlarmClockInit(MeleePc_AlarmClock* clock,uint64_t ns,int64_t ticks,uint32_t rate){
    if(!clock || ticks<0 || !rate)return 0;
    MeleePc_AlarmClock next={ns,ns,ticks,rate};*clock=next;return 1;
}
int MeleePc_AlarmClockRead(MeleePc_AlarmClock* clock,uint64_t ns,int64_t* ticks){
    int64_t elapsed;if(!clock || !ticks || !clock->rate || clock->origin_ticks<0 || ns<clock->last_ns || ns<clock->origin_ns)return 0;
    if(!MeleePc_NanosecondsToTicks(ns-clock->origin_ns,clock->rate,&elapsed) || elapsed>INT64_MAX-clock->origin_ticks)return 0;
    *ticks=clock->origin_ticks+elapsed;clock->last_ns=ns;return 1;
}
