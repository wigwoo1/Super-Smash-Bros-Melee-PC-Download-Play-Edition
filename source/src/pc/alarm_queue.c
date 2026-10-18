#include "alarm_queue.h"
#include "alarm_deadline.h"
#include <string.h>
#include <limits.h>
void MeleePc_AlarmQueueInit(MeleePc_AlarmQueue* q){if(q)memset(q,0,sizeof(*q));}
int MeleePc_AlarmSchedule(MeleePc_AlarmQueue* q,uintptr_t key,int64_t fire,int64_t period,MeleePc_AlarmCallback cb,void* data){
    int slot=-1;if(!q || !key || fire<0 || period<0 || !cb || q->next_order==UINT64_MAX)return 0;
    for(unsigned i=0;i<MELEE_PC_ALARM_CAPACITY;i++)if(q->entries[i].active && q->entries[i].key==key){slot=(int)i;break;}
    if(slot<0)for(unsigned i=0;i<MELEE_PC_ALARM_CAPACITY;i++)if(!q->entries[i].active){slot=(int)i;break;}
    if(slot<0)return 0;
    MeleePc_AlarmEntry entry={key,fire,fire,period,q->next_order++,cb,data,1,NULL,NULL};q->entries[slot]=entry;return 1;
}
int MeleePc_AlarmCancel(MeleePc_AlarmQueue* q,uintptr_t key){
    if(!q || !key)return 0;for(unsigned i=0;i<MELEE_PC_ALARM_CAPACITY;i++)if(q->entries[i].active && q->entries[i].key==key){q->entries[i].active=0;return 1;}return 0;
}
int MeleePc_AlarmSetCatchUp(MeleePc_AlarmQueue* q,uintptr_t key,MeleePc_AlarmReady ready,void* data){
    if(!q || !key || q->pumping)return 0;
    for(unsigned i=0;i<MELEE_PC_ALARM_CAPACITY;i++){
        MeleePc_AlarmEntry* e=&q->entries[i];
        if(e->active && e->key==key && e->period>0){e->catch_up_ready=ready;e->ready_data=data;return 1;}
    }return 0;
}
int MeleePc_AlarmPump(MeleePc_AlarmQueue* q,int64_t now,unsigned budget){
    unsigned count=0;if(!q || now<0 || q->pumping || budget>INT_MAX)return -1;q->pumping=1;
    while(count<budget){int slot=-1;
      for(unsigned i=0;i<MELEE_PC_ALARM_CAPACITY;i++){MeleePc_AlarmEntry* e=&q->entries[i];if(!e->active || e->fire>now)continue;
        if(e->catch_up_ready && !e->catch_up_ready(e->ready_data))continue;
        if(slot<0 || e->fire<q->entries[slot].fire || (e->fire==q->entries[slot].fire && e->order<q->entries[slot].order))slot=(int)i;
      }if(slot<0)break;
      MeleePc_AlarmEntry e=q->entries[slot];q->entries[slot].active=0;
      if(e.period && e.catch_up_ready){
        if(e.fire<=INT64_MAX-e.period){q->entries[slot].fire=e.fire+e.period;q->entries[slot].active=1;}
      }else if(e.period){int64_t next;if(MeleePc_NextPeriodicDeadline(e.start,e.period,now,&next)){
          if(next<=now){if(now>INT64_MAX-e.period)next=-1;else next=now+e.period;}
          if(next>=0){q->entries[slot].fire=next;q->entries[slot].active=1;}
      }}
      e.callback(e.userdata);count++;
    }q->pumping=0;return (int)count;
}
int MeleePc_AlarmNextDeadline(const MeleePc_AlarmQueue* q,int64_t* ticks){
    int found=0;int64_t next=0;if(!q || !ticks)return 0;
    for(unsigned i=0;i<MELEE_PC_ALARM_CAPACITY;i++)if(q->entries[i].active && (!found || q->entries[i].fire<next)){next=q->entries[i].fire;found=1;}
    if(found)*ticks=next;return found;
}
