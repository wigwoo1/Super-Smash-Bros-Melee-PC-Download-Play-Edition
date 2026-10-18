#ifndef MELEE_PC_KIRBY_COPY_LINK_PLAN_H
#define MELEE_PC_KIRBY_COPY_LINK_PLAN_H
#include "fighter_model_parts.h"
#include "fighter_dynamics.h"
#include "item_article_plan.h"
/* Source ftKbCopyHat_ItemsDynamics; hat loader explicitly selects row0. */
static void MeleePc_KirbyCopyLinkPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=40320||root!=884||!title_range(p,root,24)){p->error=-2;return;}
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
 uint32_t arrow=title_pointer(p,root+12),bow=title_pointer(p,root+16),dyn=title_pointer(p,root+20);
 if(arrow==UINT32_MAX||bow==UINT32_MAX||dyn==UINT32_MAX){p->error=-1;return;}
 /* Shared original callbacks/layouts: copied arrow140 uses64; bow142 uses76.
  * This selects schema only; original registered game kinds stay unchanged. */
 MeleePc_ItemArticlePlan(p,arrow,64,1);
 MeleePc_ItemArticlePlan(p,bow,76,6);
 MeleePc_FighterDynamicsPlan(p,dyn,0);
 /* Private exact-profile descriptors: validate and convert allocation words,
  * never install unused objects. Public-root ownership remains unproven. */
 static const uint32_t fields[][2]={
 {692,17320},{696,19400},
 {16864,14728},{16868,0},{16872,0},{16876,0},{16880,16864},{16884,0},
 {18944,17320},{18948,0},{18952,0},{18956,0},{18960,18944},{18964,0},
 {21024,19400},{21028,0},{21032,0},{21036,0},{21040,21024},{21044,0},
 {40268,31792},{40272,33432},{40276,35176},{40280,36648},{40284,38288},{40288,40008},{40292,0},
 {40296,26664},{40300,40268},{40304,0},{40308,0},{40312,40296},{40316,0}};
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++){
  uint32_t at=fields[i][0],target=fields[i][1];
  if(!title_range(p,at,4)||title_u32(p,at)!=target||
     !!p->relocs[at/4]!=!!target||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 }
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++)title_pointer(p,fields[i][0]);
}
#endif
