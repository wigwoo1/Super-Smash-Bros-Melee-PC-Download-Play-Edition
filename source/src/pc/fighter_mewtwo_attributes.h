#ifndef MELEE_PC_FIGHTER_MEWTWO_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_MEWTWO_ATTRIBUTES_H
#include "title_archive.h"
/* Original 136-byte Mewtwo attributes; embedded ReflectDesc byte tail stays intact. */
static void MeleePc_FighterMewtwoAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,136)){p->error=-1;return;}
 for(unsigned off=0;off<136;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned integer=off==12||off==16||off==28||off==32||off==80||off==104;
  if(off!=60&&!integer&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,60,4);
 title_claim(p,root+60,4,1);
 title_claim(p,root+64,72,4);
}
#endif
