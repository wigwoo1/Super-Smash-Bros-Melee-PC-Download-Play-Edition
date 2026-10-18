#ifndef MELEE_PC_FIGHTER_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_ATTRIBUTES_H
#include "title_archive.h"
/* Source ftCo_DatAttrs and ftKb_DatAttrs. Only scalar components; no archive
 * registration until the fighter's remaining graphs are validated. */
static void MeleePc_FighterAttributesPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kirby){
 static const uint16_t common_int[]={88,152,160,164,364};
 static const uint16_t kirby_int[]={0,40,44,48,236,240,264,268,272,280,308,320,348,368,400,404,416,420,424,428,468,472,496,500,572,576,608,612,628,632,648,652,680,692,696,752,796,860,864,868,880,884,888,912,916,980,1004,1024,1028};
 if(p->error)return;
 if(kirby>1){p->error=-1;return;}
 uint32_t size=kirby?1060:388,words=kirby?1056:384;
 if((root&3) || !title_range(p,root,size)){p->error=-1;return;}
 const uint16_t* ints=kirby?kirby_int:common_int;
 size_t count=kirby?sizeof(kirby_int)/sizeof(*ints):sizeof(common_int)/sizeof(*ints);
 /* Complete validation before any component claims are changed. */
 for(uint32_t off=0;off<size;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
  if(off>=words || (kirby && off==52))continue;
  unsigned integer=0;for(size_t i=0;i<count;i++)if(off==ints[i])integer=1;
  if(!integer && (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 for(uint32_t off=0;off<size;){
  unsigned width=(off>=words)?1:(kirby && off==52)?2:(kirby && off==54)?1:4;
  uint32_t length=(kirby && off==54)?2:width;
  for(uint32_t i=0;i<length;i++)if(p->claims[root+off+i] && p->claims[root+off+i]!=width){p->error=-1;return;}
  off+=length;
 }
 if(kirby){title_claim(p,root,52,4);title_claim(p,root+52,2,2);title_claim(p,root+54,2,1);title_claim(p,root+56,1000,4);}
 else title_claim(p,root,384,4);
 title_claim(p,root+words,4,1);
}
#endif
