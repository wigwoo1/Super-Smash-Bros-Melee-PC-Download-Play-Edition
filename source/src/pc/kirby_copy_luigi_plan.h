#ifndef MELEE_PC_KIRBY_COPY_LUIGI_PLAN_H
#define MELEE_PC_KIRBY_COPY_LUIGI_PLAN_H
#include "fighter_model_parts.h"
#include "fighter_dynamics.h"
#include "item_article_plan.h"
/* Source ftKbCopyHat_Item; hat loader explicitly selects row0. */
static void MeleePc_KirbyCopyLuigiPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=48188||root!=296||!title_range(p,root,16)){p->error=-2;return;}
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
 uint32_t article=title_pointer(p,root+12);
 if(article==UINT32_MAX){p->error=-1;return;}
 /* Original Kirby fireball registration shares Luigi's kind132 schema and animation0. */
 MeleePc_ItemArticlePlan(p,article,132,1);
 /* Source DynamicModelDesc and terminated anim/model lists, detached from public hat. */
 static const uint32_t fields[][2]={
 {48148,47884},{48152,0},{48156,48136},{48160,0},
 {48164,28776},{48168,48148},{48172,48156},{48176,0},
 {48180,48164},{48184,0}};
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++){
  uint32_t at=fields[i][0],target=fields[i][1];
  if(!title_range(p,at,4)||title_u32(p,at)!=target||
     !!p->relocs[at/4]!=!!target||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 }
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++)title_pointer(p,fields[i][0]);

}
#endif
