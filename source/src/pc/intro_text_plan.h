#ifndef MELEE_PC_INTRO_TEXT_PLAN_H
#define MELEE_PC_INTRO_TEXT_PLAN_H
#include "stage_stadium_sis.h"
/* SIS_IntroData: default-font streams only. Slot0 is a zero-length custom
 * image table at body-end; preserve the original pointer without dereference.
 * This exception applies to this exact profile only. */
static void MeleePc_IntroTextPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=288||root!=0||!title_range(p,0,32)){p->error=-2;return;}
 static const uint32_t targets[]={288,262,32,66,98,124,172,213};
 for(unsigned i=0;i<8;i++){
  unsigned at=i*4;
  if(p->relocs[i]!=1||(p->external_slots&&p->external_slots[i])||title_u32(p,at)!=targets[i]){p->error=-2;return;}
  if(!title_claim(p,at,4,4))return;
  p->relocs[i]=2;
 }
 for(unsigned i=2;i<8&&!p->error;i++){
  uint32_t at=targets[i],end=i<7?targets[i+1]:targets[1];int ended=0;
  while(at<end&&!p->error){
   unsigned op=p->bytes[32+at],size=1;
   if(op>=32){
    size=2;if(end-at<2){p->error=-1;return;}
    unsigned code=title_u16(p,at);
    if(code<0x2000||code>=0x2120){p->error=-2;return;}
   }else if(op==8||op==9||op>26){p->error=-2;return;}
   else if(op==5)size=3;
   else if(op==6||op==7||op==10||op==14)size=5;
   else if(op==12)size=4;
   if(size>end-at){p->error=-1;return;}
   stadium_sis_bytes(p,at,size);at+=size;
   if(!op){ended=1;break;}
  }
  if(!ended||at!=end){p->error=-1;return;}
 }
 if(!p->error)stadium_sis_bytes(p,262,26);
}
#endif
