#ifndef MELEE_PC_ITEM_FOOD_ATTRIBUTES_H
#define MELEE_PC_ITEM_FOOD_ATTRIBUTES_H
#include "item_dynamics.h"
/* itFoods_Logic18_Spawned: count, joint, heal. it_8028F9D8 additionally reads
 * record.w and following record.x, requiring one trailing float. */
static void MeleePc_ItemFoodAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,4)){p->error=-1;return;}
 uint32_t count=title_u32(p,root);
 if(!count || count>INT32_MAX || p->size<4 || count>(p->size-4)/16 || !title_range(p,root,count*16+4)){p->error=-1;return;}
 for(uint32_t i=0;i<count && !p->error;i++){
  uint32_t at=root+i*16;
  item_dynamics_scalars(p,at,4,i?0:4);
  uint32_t joint=item_dynamics_pointer(p,at+4);
  if(joint==UINT32_MAX){p->error=-1;return;}
  title_enqueue(p,joint,TITLE_HSD_Joint);
  item_dynamics_scalars(p,at+8,4,4);
  item_dynamics_scalars(p,at+12,4,0);
 }
 item_dynamics_scalars(p,root+count*16,4,0);
}
#endif
