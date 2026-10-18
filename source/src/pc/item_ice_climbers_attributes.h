#ifndef MELEE_PC_ITEM_ICE_CLIMBERS_ATTRIBUTES_H
#define MELEE_PC_ITEM_ICE_CLIMBERS_ATTRIBUTES_H
#include "title_archive.h"
#include <melee/it/forward.h>
/* itCharItems.h: padding is byte data; Belay joints require graph traversal
 * and external import binding before the archive can be used. */
static void MeleePc_ItemIceClimbersAttributesPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 uint32_t size=0,floats=0,opaque=0,pointers=0;
 if(p->error)return;
 switch(kind){
 case It_Kind_IceClimber_Ice:size=52;floats=0x6ff;opaque=0x100;break;
 case It_Kind_IceClimber_Blizzard:size=20;floats=0x1f;break;
 case It_Kind_IceClimber_GumStrings:size=44;floats=0x2c;opaque=0x10;pointers=0x600;break;
 default:p->error=-2;return;
 }
 if((root&3)||!title_range(p,root,size)){p->error=-1;return;}
 for(uint32_t off=0;off<size;off+=4){
  uint32_t at=root+off,mask=1u<<(off/4),v=title_u32(p,at);
  unsigned external=p->external_slots&&p->external_slots[at/4];
  if(pointers&mask){
   if(external){if(p->relocs[at/4]){p->error=-1;return;}continue;}
   if(p->relocs[at/4]){
    if((v&3)||!title_range(p,v,title_types[TITLE_HSD_Joint].size)){p->error=-1;return;}
   }else if(v){p->error=-1;return;}
  }else{
   if(p->relocs[at/4]||external){p->error=-1;return;}
   if((floats&mask)&&(v&0x7f800000u)==0x7f800000u){p->error=-1;return;}
  }
 }
 for(uint32_t off=0;off<size&&!p->error;off+=4){
  uint32_t mask=1u<<(off/4);
  if(pointers&mask)title_enqueue(p,title_pointer(p,root+off),TITLE_HSD_Joint);
  else title_claim(p,root+off,4,(opaque&mask)?1:4);
 }
}
#endif
