#ifndef MELEE_PC_KIRBY_COPY_YOSHI_PLAN_H
#define MELEE_PC_KIRBY_COPY_YOSHI_PLAN_H
#include "fighter_model_parts.h"
#include "fighter_dynamics.h"
#include "item_article_plan.h"
/* Yoshi copy hat, accessory egg joint, four hat animations and Egg Lay.
 * Despite the old ftKbCopyHat_Yoshi::dynamics annotation, offset0xC is
 * cast to HSD_Joint in ftkirbyyoshiegg.c and passed to SetAccessory. */
static void MeleePc_KirbyCopyYoshiPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=109468||root!=276||!title_range(p,root,36)){p->error=-2;return;}
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
 title_enqueue(p,title_pointer(p,root+12),TITLE_HSD_Joint);
 for(unsigned i=0;i<4&&!p->error;i++)title_enqueue(p,title_pointer(p,root+16+i*4),TITLE_HSD_AnimJoint);
 /* All Egg Lay callback motion IDs are-1; no asset-state/script rows.
  * Original Kirby kind registration is unchanged;87 selects shared schema. */
 MeleePc_ItemArticlePlan(p,title_pointer(p,root+32),87,0);
 /* Exact detached alternate egg model and descriptor directory; typed
  * conversion does not install it as an extra runtime scene object. */
 static const uint32_t fields[][2]={{98080,89672},{98084,0},{98088,0},{98092,0},{109440,101032},{109444,0},{109448,0},{109452,0},{109456,98080},{109460,109440},{109464,0}};
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++){
  uint32_t at=fields[i][0],target=fields[i][1];
  if(!title_range(p,at,4)||title_u32(p,at)!=target||
     !!p->relocs[at/4]!=!!target||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 }
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++)title_pointer(p,fields[i][0]);
 title_enqueue(p,101032,TITLE_HSD_Joint);
}
#endif

