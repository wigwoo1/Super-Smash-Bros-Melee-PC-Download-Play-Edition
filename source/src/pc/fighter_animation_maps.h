#ifndef MELEE_PC_FIGHTER_ANIMATION_MAPS_H
#define MELEE_PC_FIGHTER_ANIMATION_MAPS_H
#include "title_archive.h"
/* ftData x10/x18 point to source-counted u8[count][2], not halfwords. */
static void MeleePc_FighterAnimationMapPlan(MeleePc_TitlePlan* p,uint32_t root,
                                          uint32_t count){
 if(p->error)return;
 if(!count||count>p->size/2||!title_range(p,root,count*2)){p->error=-1;return;}
 uint32_t size=count*2;
 for(uint32_t at=root&~3U;at<root+size;at+=4)
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
 for(uint32_t off=0;off<size;off++)if(p->claims[root+off]&&p->claims[root+off]!=1){p->error=-1;return;}
 title_claim(p,root,size,1);
}
#endif
