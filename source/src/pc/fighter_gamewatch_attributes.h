#ifndef MELEE_PC_FIGHTER_GAMEWATCH_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_GAMEWATCH_ATTRIBUTES_H
#include "title_archive.h"
/* Source ftGameWatchAttributes: 0x94 bytes. GXColor[4] and outline at
 * 0x04..0x17 remain byte arrays; Judge flags and absorption bone are integers.
 * Staged helper only: no production GameWatch archive dispatch yet. */
static void MeleePc_FighterGameWatchAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,0x94)){p->error=-1;return;}
 for(unsigned off=0;off<0x94;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned color=off>=4&&off<0x18;
  unsigned integer=(off>=0x34&&off<0x58)||off==0x80;
  if(!color&&!integer&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,4,4);
 title_claim(p,root+4,20,1);
 title_claim(p,root+0x18,0x7c,4);
}
#endif
