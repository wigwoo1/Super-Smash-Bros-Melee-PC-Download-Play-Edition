#ifndef MELEE_PC_FIGHTER_MARIO_MODEL_WRAPPERS_H
#define MELEE_PC_FIGHTER_MARIO_MODEL_WRAPPERS_H
#include "fighter_sound_tables.h"
/* Pinned auxiliary DynamicModelDesc/list structures from the original
 * Mario archive. The caller must enforce the complete archive profile. */
static void MeleePc_FighterMarioModelWrappersPlan(MeleePc_TitlePlan* p){
 const uint32_t slots[]={84108,84112,84116,84120,84124,84128,
  84084,84088,84092,84096,84100,84104,
  101780,101784,101788,101792,101796,101800,
  101768,101772,101776};
 const uint32_t expected[]={64616,84084,84092,84100,84108,UINT32_MAX,
  83724,UINT32_MAX,84044,UINT32_MAX,84072,UINT32_MAX,
  92180,101768,UINT32_MAX,UINT32_MAX,101780,UINT32_MAX,
  97784,101328,UINT32_MAX};
 if(p->error)return;
 for(unsigned i=0;i<sizeof(slots)/sizeof(slots[0]);i++){
  uint32_t target;
  if(!fighter_sound_pointer(p,slots[i],&target)||target!=expected[i]){
   p->error=-1;return;
  }
 }
 for(unsigned i=0;i<sizeof(slots)/sizeof(slots[0]);i++)title_pointer(p,slots[i]);
 title_enqueue(p,84072,TITLE_HSD_ShapeAnimJoint);
 /* Detached serialized animation records: source-schema graph and byte
  * oracle verified; runtime ownership remains a separate acceptance gate. */
 title_enqueue(p,84176,TITLE_HSD_AnimJoint);
 title_enqueue(p,84440,TITLE_HSD_AnimJoint);
 title_enqueue(p,84464,TITLE__HSD_MatAnim);
 title_enqueue(p,84496,TITLE_HSD_MatAnimJoint);
 title_enqueue(p,84160,TITLE_HSD_AObjDesc);
 title_enqueue(p,84424,TITLE_HSD_AObjDesc);
 title_enqueue(p,84140,TITLE__HSD_FObjDesc);
 title_enqueue(p,84404,TITLE__HSD_FObjDesc);
}
#endif

