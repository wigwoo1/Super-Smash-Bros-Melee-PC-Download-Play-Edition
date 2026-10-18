#ifndef MELEE_PC_FIGHTER_DYNAMICS_H
#define MELEE_PC_FIGHTER_DYNAMICS_H
#include "item_dynamics.h"
/* Caller provides the source animation-map indexed prefix; no allocation-size inference. */
static void MeleePc_FighterDynamicsPlan(MeleePc_TitlePlan* p,uint32_t root,uint32_t map_count){
 if(p->error||(root&3)||!title_range(p,root,20)||map_count>256){p->error=-1;return;}
 MeleePc_ItemDynamicsPlan(p,root);
 if(p->error)return;
 uint32_t bones=title_u32(p,root),table=item_dynamics_pointer(p,root+16);
 if(p->error)return;
 if(table==UINT32_MAX){if(map_count)p->error=-1;return;}
 if((table&3)||!title_range(p,table,map_count*4)){p->error=-1;return;}
 for(uint32_t i=0;i<map_count&&!p->error;i++){
  uint32_t row=item_dynamics_pointer(p,table+i*4);
  if(row!=UINT32_MAX&&!p->error){
   if(bones>p->size/4){p->error=-1;return;}
   item_dynamics_scalars(p,row,bones*4,bones*4);
  }
 }
}
#endif
