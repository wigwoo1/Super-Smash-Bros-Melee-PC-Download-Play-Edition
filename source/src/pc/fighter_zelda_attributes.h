#ifndef MELEE_PC_FIGHTER_ZELDA_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_ZELDA_ATTRIBUTES_H
#include "title_archive.h"
/* ftZelda_DatAttrs and ReflectDesc: the final behavior byte/padding stay bytes. */
static void MeleePc_FighterZeldaAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,0xA8)){p->error=-1;return;}
 for(unsigned off=0;off<0xA8;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned integer=off==4||off==0x10||off==0x14||off==0x18||off==0x1C||
      off==0x28||off==0x30||off==0x48||off==0x60||off==0x84||off==0x88;
  if(off<0xA4&&!integer&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 for(unsigned off=0;off<0xA8;off+=4)title_claim(p,root+off,4,off==0xA4?1:4);
}
#endif
