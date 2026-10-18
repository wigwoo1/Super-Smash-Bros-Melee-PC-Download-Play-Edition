#ifndef MELEE_PC_FIGHTER_LUIGI_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_LUIGI_ATTRIBUTES_H
#include "title_archive.h"
/* Original ftLuigi_DatAttrs:32 scalar words, five integer fields, byte behavior and padding tail. */
static void MeleePc_FighterLuigiAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,152)){p->error=-1;return;}
 for(unsigned off=0;off<152;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned integer=off==136||off==148;
  if(!integer&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,152,4);
}
#endif

