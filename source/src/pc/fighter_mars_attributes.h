#ifndef MELEE_PC_FIGHTER_MARS_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_MARS_ATTRIBUTES_H
#include "title_archive.h"
/* MarsAttributes + AbsorbDesc + SwordAttrs. Sword flags/padding remain bytes.
 * Validate the complete component before recording any conversion claims. */
static void MeleePc_FighterMarsAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,0x98)){p->error=-1;return;}
 for(unsigned off=0;off<0x98;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned opaque=off>=0x80&&off<0x8C;
  unsigned integer=off==0||off==4||off==8||off==0x64||off==0x8C;
  if(!opaque&&!integer&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 for(unsigned off=0;off<0x98;off+=4)
  title_claim(p,root+off,4,off>=0x80&&off<0x8C?1:4);
}
#endif
