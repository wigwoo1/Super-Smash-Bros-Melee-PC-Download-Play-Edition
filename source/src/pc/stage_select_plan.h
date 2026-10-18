#ifndef MELEE_PC_STAGE_SELECT_PLAN_H
#define MELEE_PC_STAGE_SELECT_PLAN_H
/* mnstagesel.h: four descriptors followed by twelve inline StaticModelDesc. */
static void MeleePc_StageSelectPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error||!title_range(p,root,208))return;
 /* Exact original profiles have an extra duplicate pointer after each
  * counted shape array. Validate and convert it without extending count. */
 const unsigned* tails=0;
 static const unsigned tails_0[4]={491168,491340,492448,492644};
 if(p->size==668524)tails=tails_0;
 static const unsigned tails_1[4]={448928,449100,450208,450404};
 if(p->size==624236)tails=tails_1;
 if(!tails){p->error=-2;return;}
 for(unsigned i=0;i<4&&!p->error;++i){
  unsigned slot=tails[i];
  if(!title_range(p,slot-4,8) || !p->relocs[slot/4] ||
     !p->relocs[(slot-4)/4] || title_u32(p,slot)!=title_u32(p,slot-4)){p->error=-1;return;}
  title_pointer(p,slot);
 }
 const unsigned heads[4]={TITLE_HSD_CameraDescPerspective,TITLE_HSD_LightDesc,TITLE_HSD_LightDesc,TITLE_HSD_FogDesc};
 const unsigned models[4]={TITLE_HSD_Joint,TITLE_HSD_AnimJoint,TITLE_HSD_MatAnimJoint,TITLE_HSD_ShapeAnimJoint};
 for(unsigned i=0;i<4&&!p->error;i++){
  uint32_t at=title_pointer(p,root+i*4);
  if(at!=UINT32_MAX)title_enqueue(p,at,heads[i]);
 }
 for(unsigned i=0;i<12&&!p->error;i++)for(unsigned j=0;j<4&&!p->error;j++){
  uint32_t at=title_pointer(p,root+16+i*16+j*4);
  if(at!=UINT32_MAX)title_enqueue(p,at,models[j]);
 }
}
#endif
