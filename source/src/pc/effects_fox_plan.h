#ifndef MELEE_PC_EFFECTS_FOX_PLAN_H
#define MELEE_PC_EFFECTS_FOX_PLAN_H
#include "particle_bank.h"
static void MeleePc_EffectsFoxPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=121600||root!=0||!title_range(p,0,128)){p->error=-2;return;}
 uint32_t cmd=title_pointer(p,0),tex=title_pointer(p,4);
 if(p->error||cmd!=128||tex!=1472){p->error=-2;return;}
 static const unsigned types[]={TITLE_HSD_Joint,TITLE_HSD_AnimJoint,TITLE_HSD_MatAnimJoint,TITLE_HSD_ShapeAnimJoint};
 for(unsigned at=8;at<128;at+=20){
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
  uint32_t bits=title_u32(p,at);float lifetime;memcpy(&lifetime,&bits,4);
  if(!isfinite(lifetime)){p->error=-1;return;}
  title_claim(p,at,4,4);
  for(unsigned i=0;i<4&&!p->error;i++){
   uint32_t target=title_pointer(p,at+4+i*4);
   if(target!=UINT32_MAX)title_enqueue(p,target,types[i]);
  }
 }

 MeleePc_ParticleBanksPlan(p,cmd,tex);
}
#endif

