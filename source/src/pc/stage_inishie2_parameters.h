#ifndef MELEE_PC_STAGE_INISHIE2_PARAMETERS_H
#define MELEE_PC_STAGE_INISHIE2_PARAMETERS_H
/* grInishie2_YakumonoParam: twelve signed shorts and thirteen floats. */
static void MeleePc_StageInishie2ParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,76)){p->error=-1;return;}
 for(uint32_t off=0;off<76;off+=4)
  if(p->relocs[(root+off)/4] || (p->external_slots && p->external_slots[(root+off)/4])){p->error=-1;return;}
 for(uint32_t off=20;off<72;off+=4)
  if((title_u32(p,root+off)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 /* test_random forwards nonzero ranges to HSD_Randi; zero is permitted. */
 static const uint32_t ranges[]={2,6,10,14,74};
 for(unsigned i=0;i<sizeof(ranges)/sizeof(ranges[0]);i++)
  if(title_u16(p,root+ranges[i])&0x8000U){p->error=-1;return;}
 title_claim(p,root,20,2);title_claim(p,root+20,52,4);title_claim(p,root+72,4,2);
}
#endif
