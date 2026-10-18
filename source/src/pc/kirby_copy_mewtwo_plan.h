#ifndef MELEE_PC_KIRBY_COPY_MEWTWO_PLAN_H
#define MELEE_PC_KIRBY_COPY_MEWTWO_PLAN_H
/* Source-bound Mewtwo copy parts, Shadow Ball and dynamics. */
#include <pc/kirby_copy_donkey_plan.h>
#include <pc/fighter_dynamics.h>
#include <pc/item_article_plan.h>
static void MeleePc_KirbyCopyMewtwoPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=100384||root!=1296||!title_range(p,root,32)){p->error=-2;return;}
 kirby_copy_donkey_parts(p,root,6);
 uint32_t article=title_pointer(p,root+24),dynamics=title_pointer(p,root+28);
 if(article==UINT32_MAX||dynamics==UINT32_MAX){p->error=-1;return;}
 /* Shared Shadow Ball callbacks use motion IDs 0..9 (18 state rows). */
 MeleePc_ItemArticlePlan(p,article,112,10);
 MeleePc_FighterDynamicsPlan(p,dynamics,0);
 /* Source DynamicModelDesc, two terminated animation lists and model list.
  * Detached profile: do not install it as a runtime scene object. */
 static const uint32_t fields[][2]={
 {44632,31760},{44636,42464},{44640,0},{44644,33120},{44648,44500},{44652,0},
 {44656,9160},{44660,44632},{44664,44644},{44668,0},{44672,44656},{44676,0}};
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++){
  uint32_t at=fields[i][0],target=fields[i][1];
  if(!title_range(p,at,4)||title_u32(p,at)!=target||
     !!p->relocs[at/4]!=!!target||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 }
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++)title_pointer(p,fields[i][0]);
 title_enqueue(p,9160,TITLE_HSD_Joint);
 title_enqueue(p,31760,TITLE_HSD_AnimJoint);
 title_enqueue(p,42464,TITLE_HSD_AnimJoint);
 title_enqueue(p,33120,TITLE_HSD_MatAnimJoint);
 title_enqueue(p,44500,TITLE_HSD_MatAnimJoint);
}

#endif
