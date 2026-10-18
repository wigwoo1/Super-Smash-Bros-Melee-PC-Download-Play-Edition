#ifndef MELEE_PC_STAGE_INISHIE1_PARAMETERS_H
#define MELEE_PC_STAGE_INISHIE1_PARAMETERS_H
/* Source grInishie1_YakumonoParam: eighteen floats, five s16 and one u16.
 * Signedness does not change the two-byte endian conversion. */
static void MeleePc_StageInishie1ParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,84)){p->error=-1;return;}
 for(uint32_t off=0;off<84;off+=4)
  if(p->relocs[(root+off)/4]||(p->external_slots&&p->external_slots[(root+off)/4])){p->error=-1;return;}
 for(uint32_t off=0;off<84;off+=4)
  if((off<20||off>=32)&&(title_u32(p,root+off)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 title_claim(p,root,20,4);title_claim(p,root+20,12,2);title_claim(p,root+32,52,4);
}
#endif
