#ifndef MELEE_PC_ITEM_SAMUS_GRAPPLE_PREFIX_H
#define MELEE_PC_ITEM_SAMUS_GRAPPLE_PREFIX_H
#include "title_archive.h"
/* itSamusGrappleAttributes scalar prefix through x60 only. Pointer/import fields stay unclaimed. */
static void MeleePc_ItemSamusGrapplePrefixPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,100)){p->error=-1;return;}
 for(unsigned off=0;off<100;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned integer=off==12||off==52;
  if(!integer&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,100,4);
}
#endif
