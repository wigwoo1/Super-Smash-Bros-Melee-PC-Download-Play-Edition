#ifndef MELEE_PC_COIN_GET_SCENE_PLAN_H
#define MELEE_PC_COIN_GET_SCENE_PLAN_H
/* Original source-derived scene wrapper arrays, both language profiles. */
static void MeleePc_CoinGetScenePlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=10364||root!=10348){p->error=-2;return;}
 title_pointer(p,10096);
 title_pointer(p,10100);
 title_pointer(p,10104);
 title_pointer(p,10108);
 title_pointer(p,10112);
 title_pointer(p,10116);
 title_pointer(p,10120);
 title_pointer(p,10124);
 title_pointer(p,10224);
 title_pointer(p,10228);
 title_pointer(p,10336);
 title_pointer(p,10340);
 title_pointer(p,10344);
 title_pointer(p,10348);
 title_pointer(p,10352);
 title_pointer(p,10356);
 title_pointer(p,10360);
 title_enqueue(p,10168,TITLE_HSD_CameraDescPerspective);
 title_enqueue(p,10328,TITLE_LightList);
 title_enqueue(p,10292,TITLE_LightList);
 title_enqueue(p,5704,TITLE_HSD_Joint);
 title_enqueue(p,9656,TITLE_HSD_AnimJoint);
 title_enqueue(p,10252,TITLE_HSD_LightPointDesc);
}
#endif
