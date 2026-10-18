#ifndef MELEE_PC_FIGHTER_CAPTAIN_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_CAPTAIN_ATTRIBUTES_H
#include "title_archive.h"
/* Original ftCaptain_DatAttrs:35 scalar words, three integer fields. */
static void MeleePc_FighterCaptainAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,0x8C)){p->error=-1;return;}
 for(unsigned off=0;off<0x8C;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned integer=off==0x64||off==0x6C||off==0x78;
  if(!integer&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,0x8C,4);
}
#endif
