#ifndef MELEE_PC_ITEM_MUSHROOM_ATTRIBUTES_H
#define MELEE_PC_ITEM_MUSHROOM_ATTRIBUTES_H
#include "item_dynamics.h"
/* Numeric velocity parameters followed by two growth/shrink animations. */
static void MeleePc_ItemMushroomAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,16)){p->error=-1;return;}
 item_dynamics_scalars(p,root,8,0);
 for(unsigned off=8;off<16 && !p->error;off+=4){
  uint32_t animation=item_dynamics_pointer(p,root+off);
  if(animation==UINT32_MAX){p->error=-1;return;}
  title_enqueue(p,animation,TITLE_HSD_AnimJoint);
 }
}
#endif
