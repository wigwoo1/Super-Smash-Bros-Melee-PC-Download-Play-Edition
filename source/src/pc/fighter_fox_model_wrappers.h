#ifndef MELEE_PC_FIGHTER_FOX_MODEL_WRAPPERS_H
#define MELEE_PC_FIGHTER_FOX_MODEL_WRAPPERS_H
#include "fighter_sound_tables.h"
/* Original article-joint-owned model descriptors and terminated model lists.
 * StaticModelDesc/DynamicModelDesc have the same layout when all animations
 * are NULL. Do not reinterpret the enclosing joint tail as an AnimJoint. */
static void MeleePc_FighterFoxModelWrappersPlan(MeleePc_TitlePlan* p){
 const uint32_t descriptors[]={82120,108192,165224};
 const uint32_t joints[]={82056,90024,165160};
 for(unsigned i=0;i<3&&!p->error;i++){
  uint32_t d=descriptors[i],joint,list=d+16;
  if(!fighter_sound_span(p,d,24)||!fighter_sound_pointer(p,d,&joint)||joint!=joints[i]){p->error=-1;return;}
  for(unsigned off=4;off<16;off+=4){
   uint32_t target;
   if(!fighter_sound_pointer(p,d+off,&target)||target!=UINT32_MAX){p->error=-1;return;}
  }
  uint32_t target,terminator;
  if(!fighter_sound_pointer(p,list,&target)||target!=d||
     !fighter_sound_pointer(p,list+4,&terminator)||terminator!=UINT32_MAX){p->error=-1;return;}
  for(unsigned off=0;off<24;off+=4)title_pointer(p,d+off);
  title_enqueue(p,joint,TITLE_HSD_Joint);
 }
}
#endif
