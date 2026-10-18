#ifndef MELEE_PC_STAGE_KONGO_AUX_SCENES_H
#define MELEE_PC_STAGE_KONGO_AUX_SCENES_H
/* Original pinned source-compatible private scene descriptors. Convert
 * allocation contents without installing either unused scene. */
static void MeleePc_StageKongoAuxScenesPlan(MeleePc_TitlePlan* p){
 if(p->error)return;
 if(p->size!=458592){p->error=-2;return;}
 static const uint32_t fields[][2]={
 {269700,269644},
 {269704,0},
 {457696,457640},
 {457700,0},
 {269836,269816},
 {269840,0},
 {457840,457820},
 {457844,0},
 {269804,269796},
 {269808,269760},
 {269812,0},
 {457808,457800},
 {457812,457764},
 {457816,0},
};
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0]);i++){
  uint32_t at=fields[i][0],target=fields[i][1];
  if(!title_range(p,at,4))return;
  if(!!p->relocs[at/4]!=!!target ||
     (p->external_slots && p->external_slots[at/4]) ||
     title_u32(p,at)!=target){p->error=-2;return;}
 }
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0]) && !p->error;i++)title_pointer(p,fields[i][0]);
 title_enqueue(p,269644,TITLE_HSD_CameraDescPerspective);
 title_enqueue(p,457640,TITLE_HSD_CameraDescPerspective);
 title_enqueue(p,269760,TITLE_LightList);
 title_enqueue(p,269796,TITLE_LightList);
 title_enqueue(p,457764,TITLE_LightList);
 title_enqueue(p,457800,TITLE_LightList);
 title_enqueue(p,269816,TITLE_HSD_FogDesc);
 title_enqueue(p,457820,TITLE_HSD_FogDesc);
}
#endif
