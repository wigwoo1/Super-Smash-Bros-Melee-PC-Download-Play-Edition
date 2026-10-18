#ifndef MELEE_PC_CHARACTER_SELECT_PLAN_H
#define MELEE_PC_CHARACTER_SELECT_PLAN_H
/* mncharsel.h: four descriptors followed by nine inline StaticModelDesc. */
static void MeleePc_CharacterSelectPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error||!title_range(p,root,160))return;
 const unsigned heads[4]={TITLE_HSD_CameraDescPerspective,TITLE_HSD_LightDesc,TITLE_HSD_LightDesc,TITLE_HSD_FogDesc};
 const unsigned models[4]={TITLE_HSD_Joint,TITLE_HSD_AnimJoint,TITLE_HSD_MatAnimJoint,TITLE_HSD_ShapeAnimJoint};
 for(unsigned i=0;i<4&&!p->error;i++){
  uint32_t at=title_pointer(p,root+i*4);
  if(at!=UINT32_MAX)title_enqueue(p,at,heads[i]);
 }
 for(unsigned i=0;i<9&&!p->error;i++)for(unsigned j=0;j<4&&!p->error;j++){
  uint32_t at=title_pointer(p,root+16+i*16+j*4);
  if(at!=UINT32_MAX)title_enqueue(p,at,models[j]);
 }
}
#endif
