#ifndef MELEE_PC_KIRBY_COPY_PEACH_PLAN_H
#define MELEE_PC_KIRBY_COPY_PEACH_PLAN_H
#include "fighter_model_parts.h"
#include "fighter_dynamics.h"
#include "fighter_peach_plan.h"
/* Source ftKbCopyHat_Items; hat loader explicitly selects row0. */
static void MeleePc_KirbyCopyPeachPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=35932||root!=512||!title_range(p,root,20)){p->error=-2;return;}
 if(!fighter_sound_scalar(p,root+4)||title_u32(p,root+4)!=1){p->error=-2;return;}
 title_claim(p,root+4,4,4);
 title_enqueue(p,title_pointer(p,root),TITLE_HSD_Joint);
 uint32_t vis=title_pointer(p,root+8);
 if(vis==UINT32_MAX||!title_range(p,vis,16)){p->error=-1;return;}
 for(unsigned v=0;v<4&&!p->error;v++){
  uint32_t lookup=title_pointer(p,vis+v*4);
  if(lookup==UINT32_MAX)continue;
  if(!title_range(p,lookup,8)||!fighter_sound_scalar(p,lookup)){p->error=-1;return;}
  uint32_t count=title_u32(p,lookup),groups=title_pointer(p,lookup+4);
  title_claim(p,lookup,4,4);
  if(count>p->size/8||(count&&groups==UINT32_MAX)||
     (groups!=UINT32_MAX&&!title_range(p,groups,count*8))){p->error=-1;return;}
  for(uint32_t i=0;i<count&&!p->error;i++){
   uint32_t at=groups+i*8;
   if(!fighter_sound_scalar(p,at)){p->error=-1;return;}
   uint32_t n=title_u32(p,at),indices=title_pointer(p,at+4);
   title_claim(p,at,4,4);
   if(n>p->size||(n&&indices==UINT32_MAX)||
      (indices!=UINT32_MAX&&!fighter_parts_payload(p,indices,n,1))){p->error=-1;return;}
  }
 }
 uint32_t toad=title_pointer(p,root+12),spore=title_pointer(p,root+16);
 if(toad==UINT32_MAX||spore==UINT32_MAX){p->error=-1;return;}
 /* Original Kirby registration uses the shared Peach Toad/spore callbacks (2/1 states). */
 MeleePc_PeachStagedArticlePlan(p,toad,104,2);
 MeleePc_PeachStagedArticlePlan(p,spore,111,1);
 /* Retail auxiliary directory: sc/types.h DynamicModelDesc with terminated
  * animation lists. Convert its graph, but do not install extra scene objects. */
 title_array(p,35924,TITLE_DynamicModelDesc,0,1);
}
#endif
