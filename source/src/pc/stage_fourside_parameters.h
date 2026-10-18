#ifndef MELEE_PC_STAGE_FOURSIDE_PARAMETERS_H
#define MELEE_PC_STAGE_FOURSIDE_PARAMETERS_H
/* Source grFourside_YakumonoParam: 17 words, 3 u16 fields, 2 padding bytes. */
static void MeleePc_StageFoursideParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,76)){p->error=-1;return;}
 for(uint32_t off=0;off<76;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
 }
 static const uint32_t floats[]={28,32,36,40,44,48,56};
 for(unsigned i=0;i<sizeof(floats)/sizeof(floats[0]);i++)
  if((title_u32(p,root+floats[i])&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 title_claim(p,root,68,4);title_claim(p,root+68,6,2);title_claim(p,root+74,2,1);
}
#endif
