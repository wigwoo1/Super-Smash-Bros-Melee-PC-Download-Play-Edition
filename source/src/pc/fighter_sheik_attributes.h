#ifndef MELEE_PC_FIGHTER_SHEIK_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_SHEIK_ATTRIBUTES_H
#include "title_archive.h"
/* Source ftSeakAttributes: 29 scalar words, integer fields at 0x38/0x50. */
static void MeleePc_FighterSheikAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,116)){p->error=-1;return;}
 for(unsigned off=0;off<116;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])||
     (off!=0x38&&off!=0x50&&(title_u32(p,at)&0x7f800000U)==0x7f800000U)){
   p->error=-1;return;
  }
 }
 for(unsigned off=0;off<116;off+=4)title_claim(p,root+off,4,4);
}
#endif
