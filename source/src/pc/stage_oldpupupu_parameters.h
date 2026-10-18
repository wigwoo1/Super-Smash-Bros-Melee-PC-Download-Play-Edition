#ifndef MELEE_PC_STAGE_OLDPUPUPU_PARAMETERS_H
#define MELEE_PC_STAGE_OLDPUPUPU_PARAMETERS_H
/* Source grOldpupupu_YakumonoParam: four s16, two int and nine f32. */
static void MeleePc_StageOldpupupuParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,52)){p->error=-1;return;}
 for(uint32_t off=0;off<52;off+=4)
  if(p->relocs[(root+off)/4]||(p->external_slots&&p->external_slots[(root+off)/4])){p->error=-1;return;}
 for(uint32_t off=16;off<52;off+=4)
  if((title_u32(p,root+off)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 title_claim(p,root,8,2);title_claim(p,root+8,44,4);
}
#endif
