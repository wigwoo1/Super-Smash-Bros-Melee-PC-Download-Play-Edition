#ifndef MELEE_PC_FIGHTER_PURIN_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_PURIN_ATTRIBUTES_H
#include "title_archive.h"
/* Source ftPurinAttributes: byte padding and unconsumed E8/EC remain intact. */
static void MeleePc_FighterPurinAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,256)){p->error=-1;return;}
 for(unsigned off=0;off<256;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned opaque=off==72||off==96||off==100||off==176||off==232||off==236||off==248||off==252,integer=off==20||off==28||off==32||off==36||off==40||off==44||off==48||off==52||off==56||off==112||off==156;
  if(!opaque&&!integer&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 for(unsigned off=0;off<256;off+=4){unsigned opaque=off==72||off==96||off==100||off==176||off==232||off==236||off==248||off==252;title_claim(p,root+off,4,opaque?1:4);}
}
#endif
