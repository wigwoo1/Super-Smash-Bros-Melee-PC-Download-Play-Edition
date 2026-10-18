#ifndef MELEE_PC_FIGHTER_WAIT_TABLE_H
#define MELEE_PC_FIGHTER_WAIT_TABLE_H
#include "fighter_sound_tables.h"
/* WaitStruct's union holds signed integer pairs here, never pointers.
 * Runtime rolls1..100 and stops on signed animation ID -1. */
static void MeleePc_FighterWaitTablePlan(MeleePc_TitlePlan* p,uint32_t root,
                                       uint32_t animation_count){
 if(p->error)return;
 if(!animation_count||(root&3)||!title_range(p,root,8)){p->error=-1;return;}
 uint32_t records=0,total=0;
 for(uint32_t at=root;;at+=8){
  if(!fighter_sound_span(p,at,8)||!fighter_sound_scalar(p,at)||
     !fighter_sound_scalar(p,at+4)){p->error=-1;return;}
  uint32_t id=title_u32(p,at),weight=title_u32(p,at+4);records++;
  if(id==UINT32_MAX)break;
  if(id>=animation_count||weight>100||total>100-weight){p->error=-1;return;}
  total+=weight;
 }
 if(total!=100){p->error=-1;return;}
 title_claim(p,root,records*8,4);
}
#endif
