#ifndef MELEE_PC_STAGE_CORNERIA_PARAMETERS_H
#define MELEE_PC_STAGE_CORNERIA_PARAMETERS_H
/* grCorneria_YakumonoParam: 23 floats, four signed words, one material
 * script pointer, and 28 opaque padding bytes. Script traversal is separate. */
static void MeleePc_StageCorneriaParametersPlan(MeleePc_TitlePlan* p,uint32_t root,uint32_t* script){
 if(p->error)return;
 if(!script||(root&3)||!title_range(p,root,140)){p->error=-1;return;}
 for(unsigned off=0;off<140;off+=4){
  uint32_t at=root+off;
  if(off==132){
   if(p->external_slots&&p->external_slots[at/4]){p->error=-1;return;}
   *script=title_pointer(p,at);continue;
  }
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  if((off>=80&&off<104)||off==108){title_claim(p,at,4,1);continue;}
  if((off<80||off==104||off==112||off==136)&&
     (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
  title_claim(p,at,4,4);
 }
}
#endif
