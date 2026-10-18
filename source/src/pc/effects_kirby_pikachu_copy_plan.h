#ifndef MELEE_PC_EFFECTS_KIRBY_PIKACHU_COPY_PLAN_H
#define MELEE_PC_EFFECTS_KIRBY_PIKACHU_COPY_PLAN_H
#include "particle_bank.h"
/* Source efAlt uses hsd_8039EFAC bank36/particle36000; no model prefix. */
static void MeleePc_EffectsKirbyPikachuCopyPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=25152||root!=0||!title_range(p,0,32)){p->error=-2;return;}
 uint32_t cmd=title_pointer(p,0),tex=title_pointer(p,4);
 if(p->error||cmd!=32||tex!=480){p->error=-2;return;}
 for(unsigned at=8;at<32;at++)if(p->bytes[32+at]||p->relocs[at/4]){p->error=-2;return;}
 title_claim(p,8,24,1);
 MeleePc_ParticleBanksPlan(p,cmd,tex);
}
#endif
