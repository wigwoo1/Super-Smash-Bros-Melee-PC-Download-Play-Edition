#ifndef MELEE_PC_FIGHTER_HURTBOXES_H
#define MELEE_PC_FIGHTER_HURTBOXES_H
#include "fighter_sound_tables.h"
/* ftData_x30 and source40-byte ftHurtboxInit. Runtime supports15 capsules. */
static void MeleePc_FighterHurtboxesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 uint32_t target;
 if(!fighter_sound_span(p,root,8)||!fighter_sound_scalar(p,root)){
  p->error=-1;return;
 }
 uint32_t count=title_u32(p,root);
 if(count>15||!fighter_sound_pointer(p,root+4,&target)||
    (count&&target==UINT32_MAX)||
    (target!=UINT32_MAX&&!fighter_sound_span(p,target,count*40))){p->error=-1;return;}
 for(uint32_t i=0;i<count;i++)for(unsigned off=0;off<40;off+=4){
  uint32_t at=target+i*40+off;
  if(!fighter_sound_scalar(p,at)||
     (off>=12&&(title_u32(p,at)&0x7f800000U)==0x7f800000U)){p->error=-1;return;}
 }
 title_claim(p,root,8,4);title_pointer(p,root+4);
 if(count)title_claim(p,target,count*40,4);
}
#endif
