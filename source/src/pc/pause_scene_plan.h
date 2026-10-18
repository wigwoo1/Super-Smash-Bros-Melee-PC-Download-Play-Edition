#ifndef MELEE_PC_PAUSE_SCENE_PLAN_H
#define MELEE_PC_PAUSE_SCENE_PLAN_H
/* Original source-derived scene wrapper arrays, both language profiles. */
static void MeleePc_PauseScenePlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=32104||root!=32088){p->error=-2;return;}
 title_pointer(p,31808);
 title_pointer(p,31812);
 title_pointer(p,31816);
 title_pointer(p,31820);
 title_pointer(p,31824);
 title_pointer(p,31828);
 title_pointer(p,31832);
 title_pointer(p,31836);
 title_pointer(p,31936);
 title_pointer(p,31940);
 title_pointer(p,32048);
 title_pointer(p,32052);
 title_pointer(p,32056);
 title_pointer(p,32080);
 title_pointer(p,32084);
 title_pointer(p,32088);
 title_pointer(p,32092);
 title_pointer(p,32096);
 title_pointer(p,32100);
 title_enqueue(p,31880,TITLE_HSD_CameraDescPerspective);
 title_enqueue(p,32040,TITLE_LightList);
 title_enqueue(p,32004,TITLE_LightList);
 title_enqueue(p,32060,TITLE_HSD_FogDesc);
 title_enqueue(p,8488,TITLE_HSD_Joint);
 title_enqueue(p,29932,TITLE_HSD_MatAnimJoint);
 title_enqueue(p,31964,TITLE_HSD_LightPointDesc);
}
#endif
