#ifndef MELEE_PC_FIGHTER_NESS_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_NESS_ATTRIBUTES_H
#include "title_archive.h"
/* Source ftNessAttributes: AbsorbDesc and ReflectDesc included; preserve byte behavior/padding. */
static void MeleePc_FighterNessAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,220)){p->error=-1;return;}
 for(unsigned off=0;off<220;off+=4){uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}}
 static const unsigned floats[]={16,20,24,28,32,36,40,44,48,52,56,60,76,80,84,88,92,96,100,104,108,112,116,120,124,128,136,140,144,148,156,160,164,168,172,176,180,192,196,200,204,208,212};
 for(unsigned i=0;i<sizeof(floats)/sizeof(*floats);++i)
  if((title_u32(p,root+floats[i])&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 title_claim(p,root,216,4);title_claim(p,root+216,4,1);
}
#endif
