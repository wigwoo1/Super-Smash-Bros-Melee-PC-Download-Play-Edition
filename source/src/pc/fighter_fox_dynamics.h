#ifndef MELEE_PC_FIGHTER_FOX_DYNAMICS_H
#define MELEE_PC_FIGHTER_FOX_DYNAMICS_H
#include "fighter_sound_tables.h"
/* Audited PlFx ftDynamics/BoneDynamicsTemplate/60-byte parameter/AbsorbDesc.
 * Scratch claims only: caller commits bytes after successful complete planning. */
static void MeleePc_FighterFoxDynamicsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=244960||root!=10620){p->error=-2;return;}
 const uint32_t starts[]={10620,10576,10336,10600},sizes[]={20,24,240,20};
 for(unsigned i=0;i<4;i++){
  if(!fighter_sound_span(p,starts[i],sizes[i])){p->error=-1;return;}
  for(uint32_t off=0;off<sizes[i];off+=4){
   uint32_t at=starts[i]+off;
   int pointer=(i==0&&(off==4||off==12))||(i==1&&off==4);
   if(pointer){uint32_t target;if(!fighter_sound_pointer(p,at,&target)||target==UINT32_MAX){p->error=-1;return;}}
   else if(!fighter_sound_scalar(p,at)){p->error=-1;return;}
   int floating=(i==1&&off>=12)||i==2||(i==3&&off>=4);
   if(floating&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
  }
 }
 if(title_u32(p,root)!=1||title_u32(p,root+4)!=10576||title_u32(p,root+8)!=1||
    title_u32(p,root+12)!=10600||title_u32(p,root+16)||
    title_u32(p,10576)!=17||title_u32(p,10580)!=10336||title_u32(p,10584)!=4||
    title_u32(p,10600)!=41){p->error=-1;return;}
 for(unsigned i=0;i<4;i++)title_claim(p,starts[i],sizes[i],4);
 title_pointer(p,root+4);title_pointer(p,root+12);title_pointer(p,10580);
}
#endif
