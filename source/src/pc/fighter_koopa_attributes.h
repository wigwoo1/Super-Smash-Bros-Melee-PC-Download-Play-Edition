#ifndef MELEE_PC_FIGHTER_KOOPA_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_KOOPA_ATTRIBUTES_H
#include "title_archive.h"
/* Source ftKoopaAttributes: forty scalar words, four integer fields.
 * Validate the entire component before claiming any conversion bytes. */
static void MeleePc_FighterKoopaAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,160)){p->error=-1;return;}
 for(unsigned off=0;off<160;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  if(off!=4&&off!=32&&off!=44&&off!=80&&
     (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,160,4);
}
#endif
