#ifndef MELEE_PC_STAGE_VENOM_PARAMETERS_H
#define MELEE_PC_STAGE_VENOM_PARAMETERS_H
/* Source grVenom_YakumonoParam: seven floats, 28 opaque char bytes,
 * and one material-script pointer. Opaque bytes remain in authored order. */
static void MeleePc_StageVenomParametersPlan(MeleePc_TitlePlan* p,uint32_t root,uint32_t* script){
 if(p->error)return;
 if(!script||(root&3)||!title_range(p,root,60)){p->error=-1;return;}
 for(unsigned off=0;off<60;off+=4){
  uint32_t at=root+off;
  if(off==56){
   if(p->external_slots&&p->external_slots[at/4]){p->error=-1;return;}
   *script=title_pointer(p,at);continue;
  }
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  if((off>=20&&off<44)||off==48){title_claim(p,at,4,1);continue;}
  if((title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
  title_claim(p,at,4,4);
 }
}
#endif
