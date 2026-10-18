#ifndef MELEE_PC_STAGE_OLDKONGO_PARAMETERS_H
#define MELEE_PC_STAGE_OLDKONGO_PARAMETERS_H
#include "stage_material_commands.h"
/* grOldKongo_YakumonoParam and its source grMaterial_801C9604 overlay stream. */
static void MeleePc_StageOldkongoParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,112)){p->error=-1;return;}
 for(uint32_t off=0;off<108;off+=4)
  if(p->relocs[(root+off)/4]||(p->external_slots&&p->external_slots[(root+off)/4])){p->error=-1;return;}
 for(uint32_t off=4;off<108;off+=4)
  if((off<44||off==68||off==72)&&(title_u32(p,root+off)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 title_claim(p,root,4,2);title_claim(p,root+4,40,4);title_claim(p,root+44,16,2);title_claim(p,root+60,48,4);
 uint32_t stream=title_pointer(p,root+108);
 MeleePc_StageMaterialCommandsPlan(p,stream);
}
#endif
