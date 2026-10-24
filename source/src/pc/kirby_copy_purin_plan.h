#ifndef MELEE_PC_KIRBY_COPY_PURIN_PLAN_H
#define MELEE_PC_KIRBY_COPY_PURIN_PLAN_H
#include "kirby_copy_donkey_plan.h"
#include "fighter_dynamics.h"
/* Source ftKbCopyParts_Dynamics; original Kirby has six costume tables. */
static void MeleePc_KirbyCopyPurinPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=63968||root!=424||!title_range(p,root,28)){p->error=-2;return;}
 kirby_copy_donkey_parts(p,root,6);
 MeleePc_FighterDynamicsPlan(p,title_pointer(p,root+24),0);
}
#endif
