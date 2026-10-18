#ifndef MELEE_PC_STAGE_CORNERIA_LASER_ATTRIBUTES_H
#define MELEE_PC_STAGE_CORNERIA_LASER_ATTRIBUTES_H
/* Source readers: ArwingLaserAttr is pointer + two floats; Great Fox is
 * four floats. The Arwing nested allocation is not a complete ItemAttr:
 * preserve its four source-declared flag/padding bytes and convert the
 * throw-speed word at +4. Unused tail stays opaque, never a full ItemAttr. */
static void MeleePc_StageCorneriaLaserFloat(MeleePc_TitlePlan* p,uint32_t at){
 if(p->error)return;
 if((at&3)||!title_range(p,at,4)||p->relocs[at/4]||
    (p->external_slots&&p->external_slots[at/4])||
    (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 title_claim(p,at,4,4);
}
static void MeleePc_StageCorneriaLaserAttributesPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 if(p->error)return;
 if((kind!=234&&kind!=235)||(root&3)||!title_range(p,root,kind==234?12:16)){p->error=-1;return;}
 if(kind==234){
  if(p->external_slots&&p->external_slots[root/4]){p->error=-1;return;}
  uint32_t nested=title_pointer(p,root);
  if(p->error||nested==UINT32_MAX||(nested&3)||!title_range(p,nested,8)){p->error=-1;return;}
  if(p->relocs[nested/4]||(p->external_slots&&p->external_slots[nested/4])){p->error=-1;return;}
  title_claim(p,nested,4,1);
  MeleePc_StageCorneriaLaserFloat(p,nested+4);
 }
 for(unsigned off=kind==234?4:0;off<(kind==234?12:16);off+=4)
  MeleePc_StageCorneriaLaserFloat(p,root+off);
}
#endif
