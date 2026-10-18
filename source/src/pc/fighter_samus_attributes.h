#ifndef MELEE_PC_FIGHTER_SAMUS_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_SAMUS_ATTRIBUTES_H
#include "title_archive.h"
/* ftSs_DatAttrs through xCC. Unknown xD0 stays unclaimed; not registered. */
static void MeleePc_FighterSamusAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,208)){p->error=-1;return;}
 for(unsigned off=0;off<208;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned integer=off==32||(off>=156&&off<=200);
  if(!integer&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,208,4);
}
#endif
