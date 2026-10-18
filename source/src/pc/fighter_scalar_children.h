#ifndef MELEE_PC_FIGHTER_SCALAR_CHILDREN_H
#define MELEE_PC_FIGHTER_SCALAR_CHILDREN_H
#include "title_archive.h"
/* ftData children: scale, absorb, camera, pickup, ledge, Vec2, byte/float
 * settings. Components only; whole fighter registration remains separate. */
static void MeleePc_FighterScalarChildPlan(MeleePc_TitlePlan* p,
                                         uint32_t root,unsigned kind){
 static const uint8_t sizes[]={8,20,24,48,28,8,28};
 if(p->error)return;
 if(kind>=sizeof(sizes)){p->error=-1;return;}
 unsigned size=sizes[kind];
 if((root&3) || !title_range(p,root,size)){p->error=-1;return;}
 for(unsigned off=0;off<size;off+=4){
  unsigned floating=kind==0?off==4:kind==1?off>=4:
                    kind==4?off>=12:kind==6?(off==4||off==12||off==24):1;
  if(p->relocs[(root+off)/4] ||
     (p->external_slots && p->external_slots[(root+off)/4]) ||
     (floating && (title_u32(p,root+off)&0x7f800000U)==0x7f800000U)){
   p->error=-1;return;
  }
 }
 /* Preflight every claim so errors never partially claim a component. */
 for(unsigned off=0;off<size;off++){
  unsigned width=(kind==4&&off<12)?2:
   (kind==6&&!(off>=4&&off<8)&&!(off>=12&&off<16)&&off<24)?1:4;
  if(p->claims[root+off] && p->claims[root+off]!=width){p->error=-1;return;}
 }
 for(unsigned off=0;off<size;){
  unsigned width=(kind==4&&off<12)?2:
   (kind==6&&!(off>=4&&off<8)&&!(off>=12&&off<16)&&off<24)?1:4;
  title_claim(p,root+off,width,width);off+=width;
 }
}
#endif
