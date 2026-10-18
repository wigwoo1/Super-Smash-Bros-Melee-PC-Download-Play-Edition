#ifndef MELEE_PC_STAGE_KONGO_OPAQUE_KLAP_H
#define MELEE_PC_STAGE_KONGO_OPAQUE_KLAP_H
/* Original DAT declares one pointer inside the Klap special allocation.
 * Preserve every other byte. The source special void* and unused Klap
 * getters do not establish a payload type or either allocation's extent. */
static void MeleePc_StageKongoOpaqueKlapPlan(MeleePc_TitlePlan* p){
 if(p->error)return;
 if(p->size!=458592 || !title_range(p,303200,8) ||
    !title_range(p,303176,24) || !title_range(p,302964,4)){
  if(!p->error)p->error=-2;return;
 }
 static const uint32_t pointers[][2]={
  {303204,303176},{303180,302964},{302964,302944}};
 if(title_u32(p,303200)!=218){p->error=-2;return;}
 for(unsigned i=0;i<3;i++){
  uint32_t at=pointers[i][0];
  if(!p->relocs[at/4] || (p->external_slots && p->external_slots[at/4]) ||
     title_u32(p,at)!=pointers[i][1]){p->error=-2;return;}
 }
 title_pointer(p,302964);
}
#endif
