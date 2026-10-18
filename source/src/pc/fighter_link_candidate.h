#ifndef MELEE_PC_FIGHTER_LINK_CANDIDATE_H
#define MELEE_PC_FIGHTER_LINK_CANDIDATE_H
#include "fighter_link_plan.h"
/* Explicit original profile, independently validated for production. Auxiliary
 * roots are converted in place and never attached to live fighter data.
 * Only DAT-declared disconnected pointer words are claimed; opaque adjacent
 * scalar allocations remain untouched. Layout changes require a new profile. */
static void MeleePc_FighterLinkCandidatePlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=222400||root!=40668){p->error=-2;return;}
 const uint32_t animations[]={46568,49068,51668,54268,56768,59268,61968,64668,66452,68236,70020,114500,114660};
 MeleePc_FighterAnimationGraphsPlan(p,animations,13);
 MeleePc_FighterLinkPlan(p,root);
 while(p->head<p->count&&!p->error){MeleePc_TitleNode node=p->nodes[p->head++];title_walk(p,node);}
 /* Bow special payload has no source reader. Validate a pointer-target marker
  * only; do not infer its allocation extent or convert an unknown scalar. */
 if(!p->error){
  if(p->relocs[16436/4]||(p->external_slots&&p->external_slots[16436/4]))p->error=-2;
  else title_claim(p,16436,4,1);
 }
 const uint32_t pointers[][2]={
  {16056,114500},
  {40216,101704},
  {40220,101064},
  {40224,101876},
  {40228,101236},
  {40304,114500},
  {40316,105576},
  {40320,104008},
  {40324,102824},
  {40384,138888},
  {40388,140968},
  {90164,87880},
  {90168,89680},
  {90176,88220},
  {90180,90092},
  {90188,79976},
  {90192,90164},
  {90196,90176},
  {90204,90188},
  {100640,100184},
  {100644,100500},
  {100652,94408},
  {100656,100640},
  {101276,101236},
  {101284,101064},
  {101288,101276},
  {101916,101876},
  {101924,101704},
  {101928,101916},
  {101940,100652},
  {101944,101284},
  {101948,101924},
  {103552,102824},
  {105120,104008},
  {105640,105576},
  {114720,114500},
  {114724,114660},
  {114732,109320},
  {114736,114720},
  {114748,103552},
  {114752,105120},
  {114756,105640},
  {114760,114732},
  {119168,117032},
  {119184,119168},
  {138412,129936},
  {138416,131576},
  {138420,133320},
  {138424,134792},
  {138428,136432},
  {138432,138152},
  {138440,124808},
  {138444,138412},
  {138456,138440},
  {140512,138888},
  {140528,140512},
  {142592,140968},
  {142608,142592},
};
 for(unsigned i=0;i<58&&!p->error;i++){
  uint32_t at=pointers[i][0],target=pointers[i][1];
  if(p->relocs[at/4]!=1||(p->external_slots&&p->external_slots[at/4])||title_u32(p,at)!=target){p->error=-2;break;}
  for(unsigned j=0;j<4;j++)if(p->claims[at+j])p->error=-2;
  unsigned deferred=0;
  for(unsigned j=0;j<58;j++)if(target==pointers[j][0])deferred=1;
  if(!p->claims[target]&&!deferred)p->error=-2;
 }
 for(unsigned i=0;i<58&&!p->error;i++)title_pointer(p,pointers[i][0]);
 const uint32_t imports[][2]={
  {16060,40308u},
  {16064,40312u},
  {40308,4294967295u},
  {40312,4294967295u},
};
 for(unsigned i=0;i<4&&!p->error;i++){
  uint32_t at=imports[i][0];
  if(!p->external_slots||!p->external_slots[at/4]||p->relocs[at/4]||title_u32(p,at)!=imports[i][1]){p->error=-2;break;}
  title_pointer(p,at);
 }
 for(uint32_t i=0;i<(p->size+3)/4&&!p->error;i++)if(p->relocs[i]&&p->relocs[i]!=2)p->error=-2;
}
#endif
