#ifndef MELEE_PC_FIGHTER_ICE_CLIMBERS_CORE_PLAN_H
#define MELEE_PC_FIGHTER_ICE_CLIMBERS_CORE_PLAN_H
#include "fighter_attributes.h"
#include "fighter_ice_climbers_attributes.h"
#include "fighter_model_parts.h"
#include "fighter_animation_tables.h"
#include "fighter_animation_maps.h"
#include "fighter_auxiliary_fields.h"
#include "fighter_part_animations.h"
#include "fighter_scalar_children.h"
#include "fighter_hurtboxes.h"
#include "fighter_dynamics.h"
#include "fighter_popo_isolated_pointers.h"
#include "item_article_plan.h"
/* Caller must reject remaining relocations and discard the scratch plan on
 * failure before any archive commit. Runtime gameplay is a separate gate. */
static void MeleePc_FighterIceClimbersCorePlan(MeleePc_TitlePlan* p,uint32_t root,unsigned nana){
 if(p->error)return;
 if(nana>1||p->size!=(nana?87680:101664)||root!=(nana?28036:36844)){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 uint32_t targets[24];
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 for(unsigned i=0;i<7;i++)if(targets[i]==UINT32_MAX){p->error=-1;return;}
 MeleePc_FighterAttributesPlan(p,targets[0],0);
 MeleePc_FighterIceClimbersAttributesPlan(p,targets[1]);
 MeleePc_FighterModelPartsPlan(p,targets[2],4);
 const uint32_t tables[]={targets[3],targets[5]},counts[]={321,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
 /* Exact audited isolated call/end islands target a known action stream.
  * These have no incoming local references in the supported profiles. */
 const uint32_t call_roots[]={nana?17384:28300,nana?17396:28312};
 for(unsigned i=0;i<2;i++){
  uint32_t at=call_roots[i];
  if(!title_range(p,at,12)||title_u32(p,at)!=(5u<<26)||
     title_u32(p,at+4)!=(nana?4244u:12720u)||title_u32(p,at+8)||
     p->relocs[at/4]||!p->relocs[(at+4)/4]||p->relocs[(at+8)/4]){p->error=-1;return;}
 }
 MeleePc_FighterActionStreamsPlan(p,call_roots,2);
 MeleePc_FighterAnimationMapPlan(p,targets[4],321);
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
 /* Independently walked source-connected parents; all four costumes share
  * the verified hierarchy. Packed FObj streams retain original byte order. */
 const uint32_t popo_heads[]={40440,41764,43188,44612,45936,47260,48784,50308,51376,52444,53512};
 const uint32_t nana_heads[]={31640,32964,34388,35812,37136,38460,39984,41508,42576,43644,44712};
 MeleePc_FighterAnimationGraphsPlan(p,nana?nana_heads:popo_heads,11);
 const uint32_t part_counts[]={4,4,3};
 MeleePc_FighterPartAnimationsPlan(p,targets[7],3,part_counts);
 if(targets[8]==UINT32_MAX||targets[23]==UINT32_MAX){p->error=-1;return;}
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
 if(p->error)return;
 title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
 MeleePc_FighterDynamicsPlan(p,targets[11],0);
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
 /* ft_8007C630/ft_8007C6DC consume x38 for both x1614[2] entries.
  * Each AbsorbDesc is 20 bytes; the second must also use native scalars. */
 if(targets[14]==UINT32_MAX || !title_range(p,targets[14],40)){p->error=-1;return;}
 MeleePc_FighterScalarChildPlan(p,targets[14]+20,1);
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
 MeleePc_FighterPartIdsPlan(p,targets[21]);
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
 if(p->error)return;
 if(targets[18]==UINT32_MAX||!fighter_sound_span(p,targets[18],12)){p->error=-1;return;}
 const unsigned kinds[]={It_Kind_IceClimber_Ice,It_Kind_IceClimber_Blizzard,It_Kind_IceClimber_GumStrings};
 const unsigned animation_counts[]={1,1,0};
 for(unsigned i=0;i<3&&!p->error;i++){
  uint32_t article=title_pointer(p,targets[18]+i*4);
  if(article==UINT32_MAX){p->error=-1;return;}
  MeleePc_ItemArticlePlan(p,article,kinds[i],animation_counts[i]);
 }
 if(!nana)MeleePc_FighterPopoIsolatedPointersPlan(p);
 /* Validated serialized import-chain links also need native byte order,
  * including unused article-animation entries. Never traverse them as joints.
  * ArchiveLocateExtern owns binding after archive parse. */
 if(!p->error && p->external_slots)
  for(uint32_t at=0;at+4<=p->size;at+=4)
   if(p->external_slots[at/4])title_pointer(p,at);
}
#endif
