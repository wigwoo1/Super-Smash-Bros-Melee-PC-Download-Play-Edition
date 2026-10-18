#ifndef MELEE_PC_ITEM_LINK_ATTRIBUTES_H
#define MELEE_PC_ITEM_LINK_ATTRIBUTES_H
/* itCharItems.h source layouts. Pointer fields are typed model/animation
 * roots, never scalar words. Caller must drain the shared graph queue before
 * committing an archive transaction. */
static void MeleePc_ItemLinkAttributesPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 uint32_t size=0,floats=0,pointers=0;
 switch(kind){
 case 60:size=100;floats=0x1fff8u;pointers=0x1fe0000u;break;
 case 62:size=96;floats=0x1ff7f7u;pointers=0xe00000u;break;
 case 64:size=48;floats=0x9ffu;pointers=0x600u;break;
 default:p->error=-2;return;
 }
 if(p->error||(root&3)||!title_range(p,root,size)){p->error=-1;return;}
 for(uint32_t off=0;off<size;off+=4){
  uint32_t at=root+off,mask=1u<<(off/4);
  if(pointers&mask)continue;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  if((floats&mask)&&(title_u32(p,at)&0x7f800000u)==0x7f800000u){p->error=-1;return;}
 }
 for(uint32_t off=0;off<size&&!p->error;off+=4){
  if(!(pointers&(1u<<(off/4)))){title_claim(p,root+off,4,4);continue;}
  unsigned type=TITLE_HSD_Joint;
  if(kind==60&&off>=76){
   const unsigned types[]={TITLE_HSD_AnimJoint,TITLE_HSD_MatAnimJoint,TITLE_HSD_ShapeAnimJoint};
   type=types[((off-76)/4)%3];
  }
  title_enqueue(p,title_pointer(p,root+off),type);
 }
}
#endif
