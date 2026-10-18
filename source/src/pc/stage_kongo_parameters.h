#ifndef MELEE_PC_STAGE_KONGO_PARAMETERS_H
#define MELEE_PC_STAGE_KONGO_PARAMETERS_H
/* Source grKongo_YakumonoParam: 40 floats, eight s16, two s32 and
 * one material-command pointer. Traverse that command graph separately. */
static void MeleePc_StageKongoParametersPlan(MeleePc_TitlePlan* p,
                                            uint32_t root,uint32_t* script){
 if(p->error)return;
 if(!script||(root&3)||!title_range(p,root,188)){p->error=-1;return;}
 for(unsigned off=0;off<188;off+=4){
  uint32_t at=root+off;
  if(p->external_slots&&p->external_slots[at/4]){p->error=-1;return;}
  if(off==132){
   uint32_t target=title_u32(p,at);
   if(p->relocs[at/4]){
    if((target&3)||!title_range(p,target,4)){p->error=-1;return;}
   }else if(target){p->error=-1;return;}
  }else{
   if(p->relocs[at/4]){p->error=-1;return;}
   if(!(off>=68&&off<84)&&off!=100&&off!=104&&
      (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
  }
 }
 for(unsigned off=0;off<188;){
  unsigned width=(off>=68&&off<84)?2:4;
  if(off==132)*script=title_pointer(p,root+off);
  else title_claim(p,root+off,width,width);
  off+=width;
 }
}
#endif
