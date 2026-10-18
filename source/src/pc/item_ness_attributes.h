#ifndef MELEE_PC_ITEM_NESS_ATTRIBUTES_H
#define MELEE_PC_ITEM_NESS_ATTRIBUTES_H
#include "title_archive.h"
/* PKFire reads only x0/x4; the declared pillar scale is not its data.
 * Yo-yo pointers lead to typed model roots. Drain the queue before commit. */
static void MeleePc_ItemNessAttributesPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 uint32_t size=0,floats=0,pointers=0;
 switch(kind){
 case 66:size=8;floats=3;break;
 case 67:size=12;floats=7;break;
 case 68:size=44;floats=0x7ff;break;
 case 69:case 78:size=20;floats=0x1f;break;
 case 102:size=92;floats=0xfff8;pointers=0x700000;break;
 default:p->error=-2;return;
 }
 if(p->error||(root&3)||!title_range(p,root,size)){p->error=-1;return;}
 for(unsigned off=0;off<size;off+=4){
  uint32_t at=root+off,mask=1u<<(off/4);
  if(pointers&mask)continue;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  if((floats&mask)&&(title_u32(p,at)&0x7f800000u)==0x7f800000u){p->error=-1;return;}
 }
 for(unsigned off=0;off<size&&!p->error;off+=4){
  if(pointers&(1u<<(off/4)))title_enqueue(p,title_pointer(p,root+off),off==88?TITLE_HSD_MatAnimJoint:TITLE_HSD_Joint);
  else title_claim(p,root+off,4,4);
 }
}
#endif
