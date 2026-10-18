#ifndef MELEE_PC_FIGHTER_EMPTY_DYNAMICS_H
#define MELEE_PC_FIGHTER_EMPTY_DYNAMICS_H
#include "title_archive.h"
/* Main Kirby archive has no dynamic bones, collision descriptors or set map.
 * Nonempty fighter/hat graphs need their own typed traversal. */
static void MeleePc_FighterEmptyDynamicsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,20)){p->error=-1;return;}
 for(unsigned off=0;off<20;off+=4){
  if(title_u32(p,root+off)||p->relocs[(root+off)/4]||
     (p->external_slots&&p->external_slots[(root+off)/4])){p->error=-1;return;}
 }
 for(unsigned off=0;off<20;off++)if(p->claims[root+off]&&p->claims[root+off]!=4){p->error=-1;return;}
 title_claim(p,root,20,4);
}
#endif
