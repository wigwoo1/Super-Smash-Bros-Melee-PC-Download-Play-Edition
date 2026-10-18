#ifndef MELEE_PC_FIGHTER_BONE_DYNAMICS_H
#define MELEE_PC_FIGHTER_BONE_DYNAMICS_H
#include "item_dynamics.h"
/* Source ftDynamics bone-only profile. Collision/set-map extensions must be
 * NULL; this helper does not silently discard unsupported extensions. */
static void MeleePc_FighterBoneDynamicsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error||(root&3)||!title_range(p,root,20)){p->error=-1;return;}
 for(unsigned off=8;off<20;off+=4){
  if(title_u32(p,root+off)||p->relocs[(root+off)/4]||
     (p->external_slots&&p->external_slots[(root+off)/4])){p->error=-2;return;}
 }
 MeleePc_ItemDynamicsPlan(p,root);
 title_claim(p,root+16,4,4);
}
#endif
