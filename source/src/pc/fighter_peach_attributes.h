#ifndef MELEE_PC_FIGHTER_PEACH_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_PEACH_ATTRIBUTES_H
#include "title_archive.h"
/* Source ftPe_DatAttrs: 192-byte ftPe_DatAttrs with inline item table and AbsorbDesc; all fields are 32-bit. */
static void MeleePc_FighterPeachAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,192)){p->error=-1;return;}
 for(unsigned off=0;off<192;off+=4){uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}}
 static const unsigned floats[]={0,4,8,12,52,56,60,64,68,72,76,80,84,88,92,96,100,104,108,112,116,120,124,128,132,136,140,148,152,156,160,164,168,176,180,184,188};
 for(unsigned i=0;i<sizeof(floats)/sizeof(*floats);++i)
  if((title_u32(p,root+floats[i])&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 title_claim(p,root,192,4);
}
#endif
