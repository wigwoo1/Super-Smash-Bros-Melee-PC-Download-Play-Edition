#ifndef MELEE_PC_KIRBY_COPY_SHEIK_PLAN_H
#define MELEE_PC_KIRBY_COPY_SHEIK_PLAN_H
#include "fighter_model_parts.h"
#include "fighter_dynamics.h"
#include "item_article_plan.h"
/* Source ftKbCopyHat_ItemsDynamics; hat loader explicitly selects row0. */
static void MeleePc_KirbyCopySheikPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=70488||root!=944||!title_range(p,root,24)){p->error=-2;return;}
 if(!fighter_sound_scalar(p,root+4)||title_u32(p,root+4)!=1){p->error=-2;return;}
 title_claim(p,root+4,4,4);
 /* Original sound0 parameter aliases hurtbox bone1. Native scalar wins;
  * runtime parser must retain pre-conversion bytes for packed sound reads. */
 if(title_u32(p,432)!=0x40000000 || title_u32(p,436)!=0 ||
    title_u32(p,440)!=1 || title_u32(p,444)!=0 ||
    title_u32(p,848)!=1 || title_u32(p,852)!=440 ||
    !p->relocs[852/4] || p->relocs[440/4]){p->error=-2;return;}
 p->sound_alias_word=440;p->sound_alias_enabled=1;
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
 uint32_t thrown=title_pointer(p,root+12),held=title_pointer(p,root+16),dyn=title_pointer(p,root+20);
 if(thrown==UINT32_MAX||held==UINT32_MAX||dyn==UINT32_MAX){p->error=-1;return;}
 /* Thrown needle callbacks use IDs0..4; held needle anim_id is-1 and
  * Item_80268E5C skips its asset-state/script table. Schema selection alone
  * does not change the original registered Kirby item kinds. */
 MeleePc_ItemArticlePlan(p,thrown,79,5);
 MeleePc_ItemArticlePlan(p,held,80,0);
 MeleePc_FighterDynamicsPlan(p,dyn,0);
 /* Exact unused held-state script pointer and detached model directories.
  * Convert pointer words, without inventing runtime ownership or consumers. */
 static const uint32_t fields[][2]={
{908,608},{66016,65672},{66020,0},{66024,0},{66028,0},{66032,66016},{66036,0},{70464,69736},{70468,0},{70472,0},{70476,0},{70480,70464},{70484,0}};
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++){
  uint32_t at=fields[i][0],target=fields[i][1];
  if(!title_range(p,at,4)||title_u32(p,at)!=target||
     !!p->relocs[at/4]!=!!target||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 }
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++)title_pointer(p,fields[i][0]);
}
#endif
