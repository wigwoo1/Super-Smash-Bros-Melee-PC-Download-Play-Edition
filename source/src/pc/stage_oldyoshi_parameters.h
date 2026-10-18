#ifndef MELEE_PC_STAGE_OLDYOSHI_PARAMETERS_H
#define MELEE_PC_STAGE_OLDYOSHI_PARAMETERS_H
/* groldyoshi_yakumono_param_t: seven s16, three floats; preserve trailing ABI padding. */
static void MeleePc_StageOldyoshiParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,28)){p->error=-1;return;}
 for(uint32_t off=0;off<28;off+=4)
  if(p->relocs[(root+off)/4]||(p->external_slots&&p->external_slots[(root+off)/4])){p->error=-1;return;}
 for(uint32_t off=4;off<16;off+=4)
  if((title_u32(p,root+off)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 title_claim(p,root,4,2);title_claim(p,root+4,12,4);title_claim(p,root+16,10,2);
}
#endif
