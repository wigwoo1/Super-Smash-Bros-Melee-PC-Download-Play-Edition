#ifndef MELEE_PC_FIGHTER_LINK_DYNAMICS_H
#define MELEE_PC_FIGHTER_LINK_DYNAMICS_H
#include "fighter_bone_dynamics.h"
/* Link's ftDynamics has BoneDynamicsTemplate records and no collision or
 * per-animation set map. Shared first16-byte layout matches ItemDynamics;
 * absent extensions are verified rather than silently ignored. */
static void MeleePc_FighterLinkDynamicsPlan(MeleePc_TitlePlan* p,uint32_t root){
 MeleePc_FighterBoneDynamicsPlan(p,root);
}
#endif
