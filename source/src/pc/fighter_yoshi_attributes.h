#ifndef MELEE_PC_FIGHTER_YOSHI_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_YOSHI_ATTRIBUTES_H
#include "title_archive.h"
/* ftYs_DatAttrs defines EC..113 as ten Egg Throw floats, despite padding
 * in the alternate attribute view. Only 12C..137 remains opaque. */
static void MeleePc_FighterYoshiAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,312)){p->error=-1;return;}
 for(unsigned off=0;off<312;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned opaque=off>=300,integer=off==0||off==56||off==72||off==76||off==80||off==164||off==220;
  if(!opaque&&!integer&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 for(unsigned off=0;off<312;off+=4){unsigned opaque=off>=300;title_claim(p,root+off,4,opaque?1:4);}
}
#endif
