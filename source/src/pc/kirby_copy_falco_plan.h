#ifndef MELEE_PC_KIRBY_COPY_FALCO_PLAN_H
#define MELEE_PC_KIRBY_COPY_FALCO_PLAN_H
#include "kirby_copy_donkey_plan.h"
#include "item_article_plan.h"
/* Source ftKbCopyParts_Items: six-costume parts followed by two articles. */
static void MeleePc_KirbyCopyFalcoPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=68832||root!=956||!title_range(p,root,32)){p->error=-2;return;}
 kirby_copy_donkey_parts(p,root,6);
 uint32_t laser=title_pointer(p,root+24),blaster=title_pointer(p,root+28);
 if(laser==UINT32_MAX||blaster==UINT32_MAX){p->error=-1;return;}
 MeleePc_ItemArticlePlan(p,laser,55,2);
 MeleePc_ItemArticlePlan(p,blaster,75,9);
 /* Source DynamicModelDesc plus terminated descriptor list; detached metadata. */
 static const uint32_t fields[][2]={
 {2216,2152},{2220,0},{2224,0},{2228,0},{2232,2216},{2236,0},
 {28288,10120},{28292,0},{28296,0},{28300,0},{28304,28288},{28308,0}};
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++){
  uint32_t at=fields[i][0],target=fields[i][1];
  if(!title_range(p,at,4)||title_u32(p,at)!=target||
     !!p->relocs[at/4]!=!!target||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 }
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0])&&!p->error;i++)title_pointer(p,fields[i][0]);
}
#endif
