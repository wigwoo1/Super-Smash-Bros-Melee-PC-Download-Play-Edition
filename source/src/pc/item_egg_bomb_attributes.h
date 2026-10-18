#ifndef MELEE_PC_ITEM_EGG_BOMB_ATTRIBUTES_H
#define MELEE_PC_ITEM_EGG_BOMB_ATTRIBUTES_H
#include "item_dynamics.h"
static void MeleePc_ItemEggBombAttributesPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 if(p->error || (root&3)){p->error=-1;return;}
 if(kind==3){
  if(!title_range(p,root,8))return;
  item_dynamics_scalars(p,root,8,8);
  uint32_t maximum=title_u32(p,root+4);
  if(!maximum || maximum>INT32_MAX)p->error=-1;
 }else if(kind==6)item_dynamics_scalars(p,root,40,0);
 else p->error=-2;
}
#endif
