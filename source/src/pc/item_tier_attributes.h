#ifndef MELEE_PC_ITEM_TIER_ATTRIBUTES_H
#define MELEE_PC_ITEM_TIER_ATTRIBUTES_H
#include "item_dynamics.h"
/* it_2E5A_Attrs: 15 base floats and three 44-byte model/animation tiers.
 * xD84_value is an integer, not a script pointer. Item_80268D34 consumes only
 * the three animation slots when this entry is viewed as ItemStateDesc. */
static void MeleePc_ItemTierAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error||(root&3)||!title_range(p,root,192)){p->error=-1;return;}
 item_dynamics_scalars(p,root,60,0);
 const unsigned types[]={TITLE_HSD_Joint,TITLE_HSD_AnimJoint,TITLE_HSD_MatAnimJoint,TITLE_HSD_ShapeAnimJoint};
 for(uint32_t i=0;i<3 && !p->error;i++){
  uint32_t at=root+60+i*44;
  for(unsigned j=0;j<4 && !p->error;j++){
   uint32_t target=item_dynamics_pointer(p,at+j*4);
   if(j==0 && target==UINT32_MAX){p->error=-1;return;}
   title_enqueue(p,target,types[j]);
  }
  item_dynamics_scalars(p,at+16,28,8);
 }
}
#endif
