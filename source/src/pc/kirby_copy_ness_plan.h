#ifndef MELEE_PC_KIRBY_COPY_NESS_PLAN_H
#define MELEE_PC_KIRBY_COPY_NESS_PLAN_H
#include "fighter_model_parts.h"
#include "fighter_dynamics.h"
#include "item_article_plan.h"
#include "item_ness_attributes.h"
static void MeleePc_CopyNessArticlePlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind,uint32_t count){
 if(p->error||(root&3)||!title_range(p,root,24)){p->error=-1;return;}
 uint32_t t[6];for(unsigned i=0;i<6&&!p->error;i++)t[i]=item_dynamics_pointer(p,root+i*4);
 if(p->error)return;
 if(t[0]!=UINT32_MAX)MeleePc_ItemArticleAttributesPlan(p,t[0]);
 if(t[1]!=UINT32_MAX&&(kind==66||kind==67||kind==68||kind==69||kind==78||kind==102))MeleePc_ItemNessAttributesPlan(p,t[1],kind);
 if(t[2]!=UINT32_MAX)MeleePc_ItemHurtbonesPlan(p,t[2]);
 if(count&&t[3]==UINT32_MAX){p->error=-1;return;}
 if(t[3]!=UINT32_MAX){MeleePc_ItemStateAnimationsPlan(p,t[3],count);
  for(unsigned i=0;i<count&&!p->error;i++){uint32_t command=title_pointer(p,t[3]+i*16+12);MeleePc_ItemCommandPlan(p,&command,1);}}
 if(t[4]!=UINT32_MAX)MeleePc_ItemModelDescriptorPlan(p,t[4]);
 if(t[5]!=UINT32_MAX)MeleePc_ItemDynamicsPlan(p,t[5]);
}
/* Source ftKbCopyHat_Items; hat loader explicitly selects row0. */
static void MeleePc_KirbyCopyNessPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=82000||root!=664||!title_range(p,root,20)){p->error=-2;return;}
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
 /* Source copied PK Flash and explosion use original Ness attribute/state schemas. */
 MeleePc_CopyNessArticlePlan(p,laser,68,3);
 MeleePc_CopyNessArticlePlan(p,blaster,78,1);
 /* Source DynamicModelDesc joint plus three nullable animation lists,
  * followed by a terminated descriptor list. Detached allocation metadata;
  * validate exact profile without installing unused scene objects. */
 static const uint32_t fields[][2]={{54308,51516},{54312,54108},{54316,0},{54320,30216},{54324,54308},{54328,0},{54332,0},{54336,54320},{54340,0},{81960,81308},{81964,0},{81968,81828},{81972,0},{81976,58280},{81980,81960},{81984,81968},{81988,0},{81992,81976},{81996,0}};
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++){
  uint32_t at=fields[i][0],target=fields[i][1];
  if(!title_range(p,at,4)||title_u32(p,at)!=target||
     !!p->relocs[at/4]!=!!target||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 }
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++)title_pointer(p,fields[i][0]);
}
#endif
