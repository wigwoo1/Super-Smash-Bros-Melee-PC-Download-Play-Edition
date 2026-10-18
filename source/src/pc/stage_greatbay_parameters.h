#ifndef MELEE_PC_STAGE_GREATBAY_PARAMETERS_H
#define MELEE_PC_STAGE_GREATBAY_PARAMETERS_H
/* Source grGb_StageAttr164: mixed timers/weights and float movement fields. */
static void MeleePc_StageGreatBayParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,164)){p->error=-1;return;}
 for(uint32_t off=0;off<164;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
  if(((off>=4 && off<68) || (off>=76 && off<112) || off==120) &&
     (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,4,2);title_claim(p,root+4,64,4);
 title_claim(p,root+68,8,2);title_claim(p,root+76,36,4);
 title_claim(p,root+112,8,2);title_claim(p,root+120,4,4);
 title_claim(p,root+124,40,2);
}
#endif
