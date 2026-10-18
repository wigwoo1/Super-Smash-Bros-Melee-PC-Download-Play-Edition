#ifndef MELEE_PC_ITEM_UNOWN_ATTRIBUTES_H
#define MELEE_PC_ITEM_UNOWN_ATTRIBUTES_H
#include "item_dynamics.h"
/* Parent: scale/velocity floats, then integer timers. Projectile: integer
 * lifetime, then float random ranges and multipliers. Both own 26 joints. */
static void MeleePc_ItemUnownAttributesPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 if(kind!=172 && kind!=199){p->error=-2;return;}
 if(p->error || (root&3) || !title_range(p,root,140)){p->error=-1;return;}
 uint32_t floats=kind==172?0x3fu:0x1feu;
 for(uint32_t i=0;i<9 && !p->error;i++)item_dynamics_scalars(p,root+i*4,4,floats&(1u<<i)?0:4);
 for(uint32_t i=0;i<26 && !p->error;i++){
  uint32_t joint=item_dynamics_pointer(p,root+36+i*4);
  if(joint==UINT32_MAX){p->error=-1;return;}
  title_enqueue(p,joint,TITLE_HSD_Joint);
 }
}
#endif
