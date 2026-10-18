#ifndef MELEE_PC_FIGHTER_MEWTWO_MODEL_WRAPPERS_H
#define MELEE_PC_FIGHTER_MEWTWO_MODEL_WRAPPERS_H
#include "fighter_sound_tables.h"
/* Source DynamicModelDesc and terminated lists; targets share the registered
 * Mewtwo ShadowBall graph. Caller enforces the exact complete archive profile. */
static void MeleePc_FighterMewtwoModelWrappersPlan(MeleePc_TitlePlan* p){
 const uint32_t slots[]={106744,106748,106752,106756,106760,106764,106768,106772,106776,106780,106784,106788};
 const uint32_t expected[]={93872,104576,UINT32_MAX,95232,106612,UINT32_MAX,71272,106744,106756,UINT32_MAX,106768,UINT32_MAX};
 if(p->error)return;
 for(unsigned i=0;i<12;i++){
  uint32_t target;
  if(!fighter_sound_pointer(p,slots[i],&target)||target!=expected[i]){p->error=-1;return;}
 }
 for(unsigned i=0;i<12;i++)title_pointer(p,slots[i]);
}
#endif
