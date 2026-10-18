#ifndef MELEE_PC_FIGHTER_PIKACHU_CANDIDATE_H
#define MELEE_PC_FIGHTER_PIKACHU_CANDIDATE_H
#include "fighter_pikachu_plan.h"
/* Exact original profile. Auxiliary words are never attached to live data.
 * Claim only DAT-declared pointer slots; unknown scalar bytes stay original.
 * Full independent byte/claim oracle is required before production dispatch. */
static void MeleePc_FighterPikachuCandidatePlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=207584||root!=38444){p->error=-2;return;}
 const uint32_t extra[]={58560};
 MeleePc_FighterAnimationGraphsPlan(p,extra,1);
 MeleePc_FighterPikachuPlan(p,root);
 while(p->head<p->count&&!p->error){MeleePc_TitleNode node=p->nodes[p->head++];title_walk(p,node);}
 /* Rejected overlapping AObj/MatAnim interpretations remain opaque. These
  * four-byte target markers establish no allocation extent or native type. */
 const uint32_t opaque[]={58812,58820,15680};
 for(unsigned i=0;i<3&&!p->error;i++){
  uint32_t at=opaque[i];
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4]))p->error=-2;
  else title_claim(p,at,4,1);
 }
 const uint32_t pointers[][2]={
  {15324,15032},
  {58828,58812},
  {83424,58560},
  {83432,58800},
  {83440,58820},
  {83448,50280},
  {83452,83424},
  {83456,83432},
  {83460,83440},
  {83464,83448},
  {144064,120456},
  {144072,121288},
  {144080,121432},
  {144088,88488},
  {144092,144064},
  {144096,144072},
  {144100,144080},
  {144104,144088}
 };
 for(unsigned i=0;i<18&&!p->error;i++){
  uint32_t at=pointers[i][0],target=pointers[i][1];
  if(p->relocs[at/4]!=1||(p->external_slots&&p->external_slots[at/4])||title_u32(p,at)!=target){p->error=-2;break;}
  for(unsigned j=0;j<4;j++)if(p->claims[at+j])p->error=-2;
  unsigned deferred=0;
  for(unsigned j=0;j<18;j++)if(target==pointers[j][0])deferred=1;
  if(!p->claims[target]&&!deferred)p->error=-2;
 }
 for(unsigned i=0;i<18&&!p->error;i++)title_pointer(p,pointers[i][0]);
 for(uint32_t i=0;i<(p->size+3)/4&&!p->error;i++)if(p->relocs[i]&&p->relocs[i]!=2)p->error=-2;
}
#endif
