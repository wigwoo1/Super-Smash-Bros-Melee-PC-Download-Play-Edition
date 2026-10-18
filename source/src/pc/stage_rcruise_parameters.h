#ifndef MELEE_PC_STAGE_RCRUISE_PARAMETERS_H
#define MELEE_PC_STAGE_RCRUISE_PARAMETERS_H
/* Source grRCruise_YakumonoParam: 18 four-byte fields, no pointers. */
static void MeleePc_StageRCruiseParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,72)){p->error=-1;return;}
 for(uint32_t off=0;off<72;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
  if((off<12 || (off>=44 && off<=56)) &&
     (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 /* Original stage logic uses these integer fields as modulo divisors. */
 if(!title_u32(p,root+20) || !title_u32(p,root+28)){p->error=-1;return;}
 title_claim(p,root,72,4);
}
#endif
