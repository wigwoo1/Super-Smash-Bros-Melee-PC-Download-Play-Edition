#ifndef MELEE_PC_FIGHTER_POPO_ISOLATED_POINTERS_H
#define MELEE_PC_FIGHTER_POPO_ISOLATED_POINTERS_H
/* Exact original-profile preservation policy. Descriptor ownership is not
 * guessed: only declared pointer words change endian; zero bytes stay opaque.
 * Caller separately validates public roots and full archive coverage. */
static void MeleePc_FighterPopoIsolatedPointersPlan(MeleePc_TitlePlan* p){
 const uint32_t spans[][2]={{36784,36792},{59348,59392},{59688,59712}};
 const uint32_t pointers[][2]={{36784,59624},{36788,59624},{59348,59308},{59356,56552},
  {59360,59348},{59372,59356},{59688,59624},{59704,59688}};
 if(p->error)return;
 if(p->size!=101664){p->error=-2;return;}
 for(unsigned i=0;i<3;i++)for(uint32_t at=spans[i][0];at<spans[i][1];at+=4){
  unsigned pointer=0;uint32_t expected=0;
  for(unsigned j=0;j<8;j++)if(pointers[j][0]==at){pointer=1;expected=pointers[j][1];}
  if(title_u32(p,at)!=expected||!!p->relocs[at/4]!=pointer||
     (p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  for(unsigned b=0;b<4;b++)if(p->claims[at+b]){p->error=-1;return;}
 }
 for(uint32_t at=0;at<p->size;at+=4)if(p->relocs[at/4]){
  uint32_t target=title_u32(p,at);unsigned slot_inside=0,target_inside=0;
  for(unsigned i=0;i<3;i++){
   slot_inside|=at>=spans[i][0]&&at<spans[i][1];
   target_inside|=target>=spans[i][0]&&target<spans[i][1];
  }
  if(target_inside&&!slot_inside){p->error=-1;return;}
 }
 for(unsigned i=0;i<3;i++)for(uint32_t at=spans[i][0];at<spans[i][1];at+=4){
  if(p->relocs[at/4])title_pointer(p,at);
  else title_claim(p,at,4,1);
 }
}
#endif
