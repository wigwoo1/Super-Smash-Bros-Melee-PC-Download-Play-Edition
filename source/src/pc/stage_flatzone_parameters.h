#ifndef MELEE_PC_STAGE_FLATZONE_PARAMETERS_H
#define MELEE_PC_STAGE_FLATZONE_PARAMETERS_H
/* Source grFlatzone_YakumonoParam: thirteen signed words and three floats.
 * unk14 and unk2C are divisors in original ground callbacks. */
static void MeleePc_StageFlatzoneParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,64)){p->error=-1;return;}
 for(uint32_t off=0;off<64;off+=4){
  if(p->relocs[(root+off)/4]||(p->external_slots&&p->external_slots[(root+off)/4])){p->error=-1;return;}
  if((off==36||off==40||off==56)&&(title_u32(p,root+off)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 if(!title_u32(p,root+20)||!title_u32(p,root+44)){p->error=-1;return;}
 title_claim(p,root,64,4);
}
#endif
