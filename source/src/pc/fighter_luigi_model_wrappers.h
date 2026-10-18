#ifndef MELEE_PC_FIGHTER_LUIGI_MODEL_WRAPPERS_H
#define MELEE_PC_FIGHTER_LUIGI_MODEL_WRAPPERS_H
#include "fighter_sound_tables.h"
/* Source DynamicModelDesc and terminated lists; targets share the registered
 * Luigi fireball graph. Caller enforces the exact complete archive profile. */
static void MeleePc_FighterLuigiModelWrappersPlan(MeleePc_TitlePlan* p){
 const uint32_t slots[]={82292,82296,82300,82304,82308,82312,82316,82320,82324,82328};
 const uint32_t expected[]={82028,UINT32_MAX,82280,UINT32_MAX,62920,82292,82300,UINT32_MAX,82308,UINT32_MAX};
 if(p->error)return;
 for(unsigned i=0;i<10;i++){
  uint32_t target;
  if(!fighter_sound_pointer(p,slots[i],&target)||target!=expected[i]){p->error=-1;return;}
 }
 for(unsigned i=0;i<10;i++)title_pointer(p,slots[i]);
}
#endif
