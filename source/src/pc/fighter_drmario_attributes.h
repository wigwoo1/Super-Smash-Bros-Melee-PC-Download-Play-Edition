#ifndef MELEE_PC_FIGHTER_DRMARIO_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_DRMARIO_ATTRIBUTES_H
#include "title_archive.h"
/* Original ftMario_DatAttrs:32 scalar words, seven integer fields including DrMario overlay x4/xC, byte behavior and padding tail. */
static void MeleePc_FighterDrMarioAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,132)){p->error=-1;return;}
 for(unsigned off=0;off<128;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned integer=off==4||off==12||off==20||off==80||off==92||off==96||off==100;
  if(!integer&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 if(p->relocs[(root+128)/4]||(p->external_slots&&p->external_slots[(root+128)/4])){p->error=-1;return;}
 title_claim(p,root,128,4);
 title_claim(p,root+128,4,1);
}
#endif
