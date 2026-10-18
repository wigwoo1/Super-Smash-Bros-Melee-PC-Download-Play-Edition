#ifndef MELEE_PC_STAGE_YORSTER_PARAMETERS_H
#define MELEE_PC_STAGE_YORSTER_PARAMETERS_H
/* YorsterParams: four floats and four integers, no pointers. */
static void MeleePc_StageYorsterParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,32)){p->error=-1;return;}
 for(uint32_t off=0;off<32;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
  if((off<16) && (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 /* Source block timing divides by x04 and x0C. */
 if(!(title_u32(p,root+4)&0x7fffffffU) || !(title_u32(p,root+12)&0x7fffffffU)){p->error=-1;return;}
 title_claim(p,root,32,4);
}
#endif
