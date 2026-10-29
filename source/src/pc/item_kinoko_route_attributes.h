#ifndef MELEE_PC_ITEM_KINOKO_ROUTE_ATTRIBUTES_H
#define MELEE_PC_ITEM_KINOKO_ROUTE_ATTRIBUTES_H
#include "item_dynamics.h"
/* itCommonItems.h defines Nokonoko and Patapata layouts. Shell callbacks in
 * itzgshell.c consume x4..x44 as floats. Their unused x0 is nevertheless a
 * DAT-declared pointer, despite the local C declaration naming it float.
 * Preserve its unconsumed target rather than invent a scalar schema. The
 * generic x0 getters are restricted to Rabbit Hood, Metal Box and Star.
 * Only the exact GrNKr profile below enables these stage article kinds. */
static void MeleePc_ItemKinokoRouteAttributesPlan(MeleePc_TitlePlan* p,
 uint32_t root,unsigned kind){
 if(p->error || (root&3)){p->error=-1;return;}
 uint32_t size=kind==211?12:kind==212?64:(kind==219 || kind==220)?72:0;
 if(!size){p->error=-2;return;}
 if(!title_range(p,root,size))return;
 uint32_t nested=item_dynamics_pointer(p,root);
 if(p->error || nested==UINT32_MAX || (nested&3) || !title_range(p,nested,8)){
  p->error=-1;return;
 }
 if(kind==211){
  item_dynamics_scalars(p,nested,8,4);
  item_dynamics_scalars(p,root+4,8,0);
 }else if(kind==212){
  /* The nested first four bytes are declared padding, not an integer. */
  if(p->relocs[nested/4] || (p->external_slots && p->external_slots[nested/4])){
   p->error=-2;return;
  }
  title_claim(p,nested,4,1);item_dynamics_scalars(p,nested+4,4,0);
  for(uint32_t off=4;off<64 && !p->error;off+=4){
   unsigned integer=off==16 || off==28 || off==36 || off==44;
   item_dynamics_scalars(p,root+off,4,integer?4:0);
  }
 }else{
  /* No shell callback reads through x0. Claim just the validated target
   * byte as opaque ownership, leaving the allocation's bytes unchanged. */
  title_claim(p,nested,1,1);
  item_dynamics_scalars(p,root+4,68,0);
 }
}
#endif
