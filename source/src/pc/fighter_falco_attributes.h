#ifndef MELEE_PC_FIGHTER_FALCO_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_FALCO_ATTRIBUTES_H
#include "title_archive.h"
/* ftFox_DatAttrs shared by Falco:52 scalar words, then ReflectDesc's
 * behavior byte and three padding bytes. Validate before claiming. */
static void MeleePc_FighterFalcoAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,0xD4)){p->error=-1;return;}
 for(unsigned off=0;off<0xD4;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned integer=off==0x1C||off==0x20||off==0x6C||off==0xA4||off==0xB0||off==0xB4;
  if(off<0xD0&&!integer&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,0xD0,4);
 title_claim(p,root+0xD0,4,1);
}
#endif
