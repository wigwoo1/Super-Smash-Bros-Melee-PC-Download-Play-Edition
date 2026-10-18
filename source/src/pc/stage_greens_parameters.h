#ifndef MELEE_PC_STAGE_GREENS_PARAMETERS_H
#define MELEE_PC_STAGE_GREENS_PARAMETERS_H
/* grGreens_YakumonoParam: source-defined mixed floats and integers, no pointers. */
static void MeleePc_StageGreensParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,124)){p->error=-1;return;}
 for(uint32_t off=0;off<124;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
  if((off==44 || off==48 || (off>=60 && off<=88) || off>=108) && (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 /* Original apple spawn cadence takes modulo x68. */
 if(!title_u32(p,root+104)){p->error=-1;return;}
 title_claim(p,root,124,4);
}
#endif
