#ifndef MELEE_PC_FIGHTER_AUXILIARY_FIELDS_H
#define MELEE_PC_FIGHTER_AUXILIARY_FIELDS_H
#include "fighter_sound_tables.h"
/* ftCo_8009F834 cycles five x54 integer part IDs. No pointer semantics. */
static void MeleePc_FighterPartIdsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(!fighter_sound_span(p,root,20)){p->error=-1;return;}
 for(unsigned i=0;i<5;i++)if(!fighter_sound_scalar(p,root+i*4)){p->error=-1;return;}
 title_claim(p,root,20,4);
}
/* ftData_x20: joint reference and finite scale; graph traversal is separate. */
static void MeleePc_FighterShieldDescriptorPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t joint;
 if(p->error)return;
 if(!fighter_sound_span(p,root,8)||!fighter_sound_scalar(p,root+4)||
    (title_u32(p,root+4)&0x7f800000U)==0x7f800000U||
    !fighter_sound_pointer(p,root,&joint)||joint==UINT32_MAX||
    (joint&3)||!title_range(p,joint,64)){p->error=-1;return;}
 title_claim(p,root,8,4);title_pointer(p,root);
}
#endif
