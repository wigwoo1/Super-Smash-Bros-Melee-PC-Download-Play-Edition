#ifndef MELEE_PC_FIGHTER_COMMON_DAMAGE_SHIFT_H
#define MELEE_PC_FIGHTER_COMMON_DAMAGE_SHIFT_H
#include "fighter_common_shake.h"
/* DamageFall selects0/1/2; even words are Vec2 pointers and odd words
 * are counts cast to u8, despite the source root's Vec2** declaration. */
static void MeleePc_FighterCommonDamageShiftPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,24)){p->error=-1;return;}
 for(unsigned i=0;i<3 && !p->error;i++)MeleePc_FighterCommonShakePlan(p,root+i*8);
}
#endif
