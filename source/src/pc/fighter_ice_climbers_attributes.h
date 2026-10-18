#ifndef MELEE_PC_FIGHTER_ICE_CLIMBERS_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_ICE_CLIMBERS_ATTRIBUTES_H
#include "title_archive.h"
/* Shared ftIceClimberAttributes. Source byte arrays remain byte-exact. */
static void MeleePc_FighterIceClimbersAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,0x15C)){p->error=-1;return;}
 for(unsigned off=0;off<0x15C;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned opaque=off==0xCC||(off>=0xD4&&off<0x12C)||off>=0x150;
  unsigned integer=off==0x1C||off==0x68;
  if(!opaque&&!integer&&(title_u32(p,at)&0x7F800000U)==0x7F800000U){p->error=-1;return;}
 }
 for(unsigned off=0;off<0x15C;off+=4){
  unsigned opaque=off==0xCC||(off>=0xD4&&off<0x12C)||off>=0x150;
  title_claim(p,root+off,4,opaque?1:4);
 }
}
#endif
