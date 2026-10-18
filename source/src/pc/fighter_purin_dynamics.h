#ifndef MELEE_PC_FIGHTER_PURIN_DYNAMICS_H
#define MELEE_PC_FIGHTER_PURIN_DYNAMICS_H
#include "fighter_dynamics.h"
/* ftCo_8009DC54 uses templates1..2 for costume2,3..4 for costume3.
 * Preserve the normal runtime dynamics count1; convert source-selected tail. */
static void MeleePc_FighterPurinDynamicsPlan(MeleePc_TitlePlan* p,uint32_t root){
 MeleePc_FighterDynamicsPlan(p,root,0);
 if(p->error)return;
 uint32_t bones=title_u32(p,root+4);
 if(title_u32(p,root)!=1||!title_range(p,bones,5*24)){p->error=-1;return;}
 for(unsigned i=1;i<5&&!p->error;i++){
  uint32_t at=bones+i*24;
  item_dynamics_scalars(p,at,4,4);item_dynamics_scalars(p,at+8,16,4);
  uint32_t params=item_dynamics_pointer(p,at+4),count=title_u32(p,at+8);
  if(count>p->size/60||(count&&params==UINT32_MAX)){p->error=-1;return;}
  if(count)item_dynamics_scalars(p,params,count*60,0);
 }
}
#endif
