#ifndef MELEE_PC_STAGE_KONGO_MODEL_WRAPPERS_H
#define MELEE_PC_STAGE_KONGO_MODEL_WRAPPERS_H
/* Pinned original DynamicModelDesc and terminated model lists. Source
 * children match live stage records or independently verified Klap/quake
 * descriptors. No scene model is installed by converting these wrappers. */
static void MeleePc_StageKongoModelWrappersPlan(MeleePc_TitlePlan* p){
 if(p->error)return;
 if(p->size!=458592){p->error=-2;return;}
 static const uint32_t fields[][2]={
  {91344,87080},
  {91348,91336},
  {91352,0},
  {91356,0},
  {94688,94408},
  {94692,94680},
  {94696,0},
  {94700,0},
  {96872,94704},
  {96876,96864},
  {96880,0},
  {96884,0},
  {186984,185832},
  {186988,0},
  {186992,0},
  {186996,0},
  {229828,219912},
  {229832,229812},
  {229836,229820},
  {229840,0},
  {232524,231176},
  {232528,232516},
  {232532,0},
  {232536,0},
  {234940,233608},
  {234944,234932},
  {234948,0},
  {234952,0},
  {237376,236040},
  {237380,237368},
  {237384,0},
  {237388,0},
  {240316,238728},
  {240320,240304},
  {240324,0},
  {240328,0},
  {243368,241640},
  {243372,243356},
  {243376,0},
  {243380,0},
  {304196,303556},
  {304200,0},
  {304204,0},
  {304208,0},
  {229844,91344},
  {229848,94688},
  {229852,96872},
  {229856,186984},
  {229860,229828},
  {229864,0},
  {243384,232524},
  {243388,234940},
  {243392,237376},
  {243396,240316},
  {243400,243368},
  {243404,0},
  {304212,304196},
  {304216,0},
  {302804,302788},
  {302808,0},
  {458584,458568},
  {458588,0},
};
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0]);i++){
  uint32_t at=fields[i][0],target=fields[i][1];
  if(!title_range(p,at,4))return;
  if(!!p->relocs[at/4]!=!!target ||
     (p->external_slots && p->external_slots[at/4]) ||
     title_u32(p,at)!=target){p->error=-2;return;}
 }
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0]) && !p->error;i++)
  title_pointer(p,fields[i][0]);
}
#endif
