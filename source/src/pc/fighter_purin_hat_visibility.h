#ifndef MELEE_PC_FIGHTER_PURIN_HAT_VISIBILITY_H
#define MELEE_PC_FIGHTER_PURIN_HAT_VISIBILITY_H
#include "fighter_model_parts.h"
/* ftPr_Init_8013C360 follows items[1], then the embedded FtPartsDesc at +4.
 * Hat joints come from separate costume archives, not this reserved word. */
static void MeleePc_FighterPurinHatVisibilityPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned costumes){
 uint32_t unused,hat,vis;
 if(p->error)return;
 if(!costumes||costumes>5||!fighter_sound_span(p,root,8)||
    !fighter_sound_pointer(p,root,&unused)||unused!=UINT32_MAX||
    !fighter_sound_pointer(p,root+4,&hat)||hat==UINT32_MAX||
    !fighter_sound_span(p,hat,12)||!fighter_sound_scalar(p,hat)||title_u32(p,hat)!=0||
    !fighter_sound_scalar(p,hat+4)||!fighter_sound_pointer(p,hat+8,&vis)||vis==UINT32_MAX){p->error=-1;return;}
 uint32_t models=title_u32(p,hat+4);
 if(models>11||!fighter_sound_span(p,vis,costumes*16)){p->error=-1;return;}
 title_pointer(p,root);title_pointer(p,root+4);title_claim(p,hat,12,4);title_pointer(p,hat+8);
 for(unsigned c=0;c<costumes;c++)for(unsigned v=0;v<4;v++){
  uint32_t at=vis+c*16+v*4,lookup;
  if(!fighter_sound_pointer(p,at,&lookup)){p->error=-1;return;}
  title_pointer(p,at);
  if(lookup!=UINT32_MAX)MeleePc_FighterVisibilityLookupPlan(p,lookup,models);
  if(p->error)return;
 }
}
#endif
