#ifndef MELEE_PC_ITEM_PIKACHU_ATTRIBUTES_H
#define MELEE_PC_ITEM_PIKACHU_ATTRIBUTES_H
#include "title_archive.h"
/* Source itCharItems.h: Thunder has three floats, ground TJolt four.
 * Air TJolt's opaque payload is not covered by this planner. */
static void MeleePc_ItemPikachuAttributesPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 if(p->error)return;
 unsigned size=kind==81?12:kind==89?16:0;
 if(!size){p->error=-2;return;}
 if((root&3)||!title_range(p,root,size)){p->error=-1;return;}
 for(unsigned off=0;off<size;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])||
     (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,size,4);
}
#endif
