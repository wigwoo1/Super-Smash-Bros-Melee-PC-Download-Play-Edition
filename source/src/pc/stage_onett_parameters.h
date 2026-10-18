#ifndef MELEE_PC_STAGE_ONETT_PARAMETERS_H
#define MELEE_PC_STAGE_ONETT_PARAMETERS_H
/* grOnett_StageParam: 26 source-defined f32 fields, no pointers. */
static void MeleePc_StageOnettParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,104)){p->error=-1;return;}
 for(uint32_t off=0;off<104;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4]) ||
     (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,104,4);
}
#endif
