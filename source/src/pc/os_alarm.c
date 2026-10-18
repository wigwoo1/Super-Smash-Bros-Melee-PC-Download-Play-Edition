#include "os_alarm.h"
#include "alarm_queue.h"
#include "callback_gate.h"
#include "alarm_deadline.h"
#include <stdlib.h>
static void require(int condition){if(!condition)abort();}
#include <limits.h>
#include <string.h>
static MeleePc_AlarmQueue queue;
static MeleePc_AlarmTimeProvider provider;
static void* provider_data;
static OSContext* dispatch_context;
static int initialized;
static OSTime last_tick;
static int clock_sampled;
int MeleePc_SetAlarmCatchUp(OSAlarm* alarm,int (*ready)(void*),void* data){
    return MeleePc_IsCallbackOwner() && initialized && alarm &&
        MeleePc_AlarmSetCatchUp(&queue,(uintptr_t)alarm,ready,data);
}
int MeleePc_GetAlarmDeadline(OSTime* deadline){
    if(!MeleePc_IsCallbackOwner() || !initialized || !deadline)return 0;
    return MeleePc_AlarmNextDeadline(&queue,deadline);
}
static OSTime now(void){require(MeleePc_IsCallbackOwner());require(provider!=NULL);OSTime tick=provider(provider_data);require(tick>=0 && (!clock_sampled || tick>=last_tick));last_tick=tick;clock_sampled=1;return tick;}
int MeleePc_InstallAlarmClock(MeleePc_AlarmTimeProvider p,void* data){if(!MeleePc_IsCallbackOwner() || !p || provider || initialized)return 0;provider=p;provider_data=data;return 1;}
void OSInitAlarm(void){require(MeleePc_IsCallbackOwner());require(provider!=NULL);if(!initialized){MeleePc_AlarmQueueInit(&queue);initialized=1;}}
void OSCreateAlarm(OSAlarm* alarm){require(MeleePc_IsCallbackOwner());require(alarm!=NULL);if(initialized)for(unsigned i=0;i<MELEE_PC_ALARM_CAPACITY;i++)require(!queue.entries[i].active || queue.entries[i].key!=(uintptr_t)alarm);memset(alarm,0,sizeof(*alarm));}
static void dispatch(void* data){
    OSAlarm* alarm=data;OSAlarmHandler handler=alarm->handler;
    alarm->handler=NULL;
    if(alarm->period){for(unsigned i=0;i<MELEE_PC_ALARM_CAPACITY;i++)if(queue.entries[i].active && queue.entries[i].key==(uintptr_t)alarm){alarm->fire=queue.entries[i].fire;alarm->handler=handler;break;}}
    handler(alarm,dispatch_context);
}
static void set(OSAlarm* alarm,OSTime fire,OSTime start,OSTime period,OSAlarmHandler handler){
    require(MeleePc_IsCallbackOwner());
    require(initialized && alarm && handler && fire>=0 && period>=0);
    require(MeleePc_AlarmSchedule(&queue,(uintptr_t)alarm,fire,period,dispatch,alarm));
    /* Queue periodic phase starts at first fire: same boundaries as original start. */
    alarm->handler=handler;alarm->fire=fire;alarm->period=period;alarm->start=start;alarm->prev=alarm->next=NULL;
}
void OSSetAbsAlarm(OSAlarm* alarm,OSTime time,OSAlarmHandler handler){set(alarm,time,0,0,handler);}
void OSSetAlarm(OSAlarm* alarm,OSTime delay,OSAlarmHandler handler){OSTime tick=now();require(delay>=0 && tick<=INT64_MAX-delay);set(alarm,tick+delay,0,0,handler);}
void OSSetPeriodicAlarm(OSAlarm* alarm,OSTime start,OSTime period,OSAlarmHandler handler){int64_t fire;require(MeleePc_NextPeriodicDeadline(start,period,now(),&fire));set(alarm,fire,start,period,handler);}
void OSCancelAlarm(OSAlarm* alarm){require(MeleePc_IsCallbackOwner());require(initialized && alarm);MeleePc_AlarmCancel(&queue,(uintptr_t)alarm);alarm->handler=NULL;alarm->prev=alarm->next=NULL;}
int MeleePc_DispatchAlarms(OSContext* context,unsigned budget){require(MeleePc_IsCallbackOwner());if(!initialized || queue.pumping || budget>INT_MAX)return -1;if(!MeleePc_CallbacksEnabled())return 0;dispatch_context=context;OSTime tick=now();unsigned total=0;while(total<budget && MeleePc_CallbacksEnabled()){int count=MeleePc_AlarmPump(&queue,tick,1);if(count<=0)break;total+=(unsigned)count;}dispatch_context=NULL;return (int)total;}
BOOL OSCheckAlarmQueue(void){
    if(!MeleePc_IsCallbackOwner() || !initialized)return FALSE;
    for(unsigned i=0;i<MELEE_PC_ALARM_CAPACITY;i++)if(queue.entries[i].active){
      MeleePc_AlarmEntry* e=&queue.entries[i];if(!e->key || !e->callback || e->fire<0 || e->period<0)return FALSE;
      for(unsigned j=i+1;j<MELEE_PC_ALARM_CAPACITY;j++)if(queue.entries[j].active && queue.entries[j].key==e->key)return FALSE;
    }return TRUE;
}






