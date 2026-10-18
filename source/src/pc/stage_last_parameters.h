#ifndef MELEE_PC_STAGE_LAST_PARAMETERS_H
#define MELEE_PC_STAGE_LAST_PARAMETERS_H
#include "stage_material_commands.h"
/* Source grLast_YakumonoParam: four ColorOverlay command stream pointers. */
static void MeleePc_StageLastParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,16)){p->error=-1;return;}
 uint32_t streams[4];
 for(unsigned i=0;i<4&&!p->error;i++){
  if(p->external_slots&&p->external_slots[(root+i*4)/4]){p->error=-2;return;}
  streams[i]=title_pointer(p,root+i*4);
 }
 MeleePc_ColorStreamsPlan(p,streams,4);
}
#endif
