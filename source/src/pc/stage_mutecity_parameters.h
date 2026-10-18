#ifndef MELEE_PC_STAGE_MUTECITY_PARAMETERS_H
#define MELEE_PC_STAGE_MUTECITY_PARAMETERS_H
#include "stage_material_commands.h"
/* grMc_YakumonoParam: two overlay scripts, two hit descriptors, opaque
 * pad10[28], then nine floats. Original source consumes each hit descriptor
 * as nine scalar words through lbColl_80008D30. */
static void MeleePc_StageMutecityParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,80)){p->error=-1;return;}
 for(uint32_t off=16;off<80;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
 }
 for(uint32_t off=44;off<80;off+=4)
  if((title_u32(p,root+off)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 for(unsigned i=0;i<4 && !p->error;i++){
  uint32_t target=title_pointer(p,root+i*4);
  if(target==UINT32_MAX || !target){p->error=-1;break;}
  if(i<2){MeleePc_StageMaterialCommandsPlan(p,target);continue;}
  if((target&3) || !title_range(p,target,36)){p->error=-1;break;}
  for(uint32_t off=0;off<36;off+=4){
   uint32_t at=target+off;
   if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;break;}
  }
  if(!p->error)title_claim(p,target,36,4);
 }
 if(!p->error){title_claim(p,root+16,28,1);title_claim(p,root+44,36,4);}
}
#endif
