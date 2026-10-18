#ifndef MELEE_PC_ITEM_WSTAR_ATTRIBUTES_H
#define MELEE_PC_ITEM_WSTAR_ATTRIBUTES_H
#include "item_dynamics.h"
/* it_80294364 consumes the count-sized inline animation/sound array. */
static void MeleePc_ItemWstarAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,40)){p->error=-1;return;}
 item_dynamics_scalars(p,root,36,0);
 item_dynamics_scalars(p,root+36,4,4);
 uint32_t count=title_u32(p,root+36);
 if(count<2 || count>8 || !title_range(p,root+40,count*8)){p->error=-1;return;}
 for(uint32_t i=0;i<count && !p->error;i++){
  uint32_t at=root+40+i*8;
  uint32_t joint=item_dynamics_pointer(p,at);
  if(joint==UINT32_MAX){p->error=-1;return;}
  title_enqueue(p,joint,TITLE_HSD_AnimJoint);
  item_dynamics_scalars(p,at+4,4,4);
 }
}
#endif
