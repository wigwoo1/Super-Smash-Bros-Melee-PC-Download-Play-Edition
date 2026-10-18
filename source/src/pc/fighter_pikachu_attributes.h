#ifndef MELEE_PC_FIGHTER_PIKACHU_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_PIKACHU_ATTRIBUTES_H
#include "title_archive.h"
/* Source ftPikachuAttributes: 0xE0 scalar words plus 24-byte ftCollisionBox.
 * Integer/item kinds are bit-preserved; floating fields must be finite. */
static void MeleePc_FighterPikachuAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,248)){p->error=-1;return;}
 for(unsigned off=0;off<248;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned integer=off==20||off==24||off==92||off==96||off==160||off==168||off==212||off==216||off==220;
  if(!integer && (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,248,4);
}
#endif
