#ifndef MELEE_PC_KIRBY_COPY_FOX_PLAN_H
#define MELEE_PC_KIRBY_COPY_FOX_PLAN_H
#include "fighter_model_parts.h"
#include "fighter_dynamics.h"
#include "item_article_plan.h"
/* Source ftKbCopyHat_Items; hat loader explicitly selects row0. */
static void MeleePc_KirbyCopyFoxPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=75416||root!=832||!title_range(p,root,20)){p->error=-2;return;}
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
 uint32_t laser=title_pointer(p,root+12),blaster=title_pointer(p,root+16);
 if(laser==UINT32_MAX||blaster==UINT32_MAX){p->error=-1;return;}
 /* Original copied kinds136/138 share laser54 and blaster74 callback schemas. */
 MeleePc_ItemArticlePlan(p,laser,54,2);
 MeleePc_ItemArticlePlan(p,blaster,74,9);
 /* Source DynamicModelDesc joint plus three nullable animation lists,
  * followed by a terminated descriptor list. Detached allocation metadata;
  * validate exact profile without installing unused scene objects. */
 static const uint32_t fields[][2]={
 {49320,49256},{49324,0},{49328,0},{49332,0},{49336,49320},{49340,0},
 {75392,57224},{75396,0},{75400,0},{75404,0},{75408,75392},{75412,0}};
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++){
  uint32_t at=fields[i][0],target=fields[i][1];
  if(!title_range(p,at,4)||title_u32(p,at)!=target||
     !!p->relocs[at/4]!=!!target||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 }
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++)title_pointer(p,fields[i][0]);
}
#endif
