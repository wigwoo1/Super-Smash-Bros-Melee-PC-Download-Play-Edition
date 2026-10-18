#ifndef MELEE_PC_FIGHTER_GAMEWATCH_CORE_PLAN_H
#define MELEE_PC_FIGHTER_GAMEWATCH_CORE_PLAN_H
#include "fighter_attributes.h"
#ifdef MELEE_GAMEWATCH_CORE_DIAGNOSTIC
#include <stdio.h>
#define GW_CORE_CHECK(label) do { if(p->error){fprintf(stderr,"GAMEWATCH_CORE_STAGE %s error=%d\n",label,p->error);return;} } while(0)
#else
#define GW_CORE_CHECK(label) do { if(p->error)return; } while(0)
#endif
#include "fighter_gamewatch_attributes.h"
#include "fighter_animation_tables.h"
#include "fighter_animation_maps.h"
#include "fighter_part_animations.h"
#include "fighter_model_parts.h"
#include "fighter_scalar_children.h"
#include "fighter_hurtboxes.h"
#include "fighter_wait_table.h"
#include "fighter_empty_dynamics.h"
#include "fighter_auxiliary_fields.h"
#include "item_article_plan.h"
#include "item_gamewatch_article_plan.h"
/* Staged original GameWatch profile. Auxiliary graphs remain pending. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_FighterGameWatchCorePlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 if(p->size!=238528||root!=40732){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||(i==10?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
 GW_CORE_CHECK("MeleePc_FighterAttributesPlan");
 MeleePc_FighterGameWatchAttributesPlan(p,targets[1]);
 GW_CORE_CHECK("MeleePc_FighterGameWatchAttributesPlan");
 MeleePc_FighterModelPartsPlan(p,targets[2],4);
 GW_CORE_CHECK("MeleePc_FighterModelPartsPlan");
 const uint32_t tables[]={targets[3],targets[5]},counts[]={323,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
 GW_CORE_CHECK("MeleePc_FighterAnimationTablesPlan");
 MeleePc_FighterAnimationMapPlan(p,targets[4],323);
 GW_CORE_CHECK("MeleePc_FighterAnimationMapPlan");
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
 GW_CORE_CHECK("MeleePc_FighterAnimationMapPlan");
 /* Third table ends at39984: that next pointer is referenced by article root40064+16, not the part animation allocation. */
 const uint32_t part_counts[]={4,4,3};
 /* Run animation-only traversal before any model nodes are queued. */
 MeleePc_FighterPartAnimationsPlan(p,targets[7],3,part_counts);
 GW_CORE_CHECK("MeleePc_FighterPartAnimationsPlan");
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
 GW_CORE_CHECK("MeleePc_FighterShieldDescriptorPlan");
 MeleePc_FighterWaitTablePlan(p,targets[9],323);
 GW_CORE_CHECK("MeleePc_FighterWaitTablePlan");
 
 
 MeleePc_FighterEmptyDynamicsPlan(p,targets[11]);
 GW_CORE_CHECK("MeleePc_FighterEmptyDynamicsPlan");
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
 GW_CORE_CHECK("MeleePc_FighterHurtboxesPlan");
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
 GW_CORE_CHECK("MeleePc_FighterSoundTablesPlan");
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
 GW_CORE_CHECK("MeleePc_FighterScalarChildPlan");
 MeleePc_FighterPartIdsPlan(p,targets[21]);
 GW_CORE_CHECK("MeleePc_FighterPartIdsPlan");
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
 GW_CORE_CHECK("MeleePc_FighterScalarChildPlan");
 if(p->error)return;
 title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
 /* ftGw_Init_OnLoad reads ten article roots and one FtPartsVisLookup table.
  * Article, attachment and visibility graphs are staged below; auxiliary
  * graphs must keep the complete archive coverage gate closed. */
 if(!fighter_sound_span(p,targets[18],44)){p->error=-1;return;}
 for(unsigned i=0;i<11&&!p->error;i++){
  uint32_t item;
  if(!fighter_sound_pointer(p,targets[18]+i*4,&item)||item==UINT32_MAX){p->error=-1;return;}
  title_pointer(p,targets[18]+i*4);
  if(i<10)MeleePc_ItemGameWatchArticlePlan(p,item,i);
  else MeleePc_FighterVisibilityLookupPlan(p,item,title_u32(p,targets[2]));
  GW_CORE_CHECK("GameWatch article children");
 }
}
#undef GW_CORE_CHECK
#endif
