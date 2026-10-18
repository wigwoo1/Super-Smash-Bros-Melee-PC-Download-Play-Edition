#ifndef MELEE_PC_FIGHTER_PICHU_PLAN_H
#define MELEE_PC_FIGHTER_PICHU_PLAN_H
#include "fighter_attributes.h"
#include "fighter_pikachu_attributes.h"
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
#ifdef MELEE_PICHU_PLAN_TRACE
#define PICHU_CHECK(label) do { if(p->error){fprintf(stderr,"PICHU_PLAN phase=%s error=%d\n",label,p->error);return;} } while(0)
#else
#define PICHU_CHECK(label) do { if(p->error)return; } while(0)
#endif
/* Staged exact Pichu profile. Caller owns scratch validation/commit. Production
 * admission still requires independent full-file oracle and parser tests. */
static void MeleePc_FighterPichuPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 if(p->size!=232960||root!=38400){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==10)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
 PICHU_CHECK("MeleePc_FighterAttributesPlan");
 MeleePc_FighterPikachuAttributesPlan(p,targets[1]);
 PICHU_CHECK("MeleePc_FighterPikachuAttributesPlan");
 MeleePc_FighterModelPartsPlan(p,targets[2],4);
 PICHU_CHECK("MeleePc_FighterModelPartsPlan");
 const uint32_t tables[]={targets[3],targets[5]},counts[]={320,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
 PICHU_CHECK("MeleePc_FighterAnimationTablesPlan");
 /* Unreferenced retail goto still targets a source-counted action command. */
 if(title_u32(p,15000)!=0x1c000000 || !p->relocs[15004/4] ||
    title_u32(p,15004)!=14708 || p->claims[14708]!=1){p->error=-1;return;}
 const uint32_t unused_goto=15000;
 MeleePc_FighterActionStreamsPlan(p,&unused_goto,1);
 PICHU_CHECK("unused goto");
 MeleePc_FighterAnimationMapPlan(p,targets[4],320);
 PICHU_CHECK("MeleePc_FighterAnimationMapPlan");
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
 PICHU_CHECK("MeleePc_FighterAnimationMapPlan");
 const uint32_t part_counts[]={4,3};
 /* Run animation-only traversal before any model nodes are queued. */
 const uint32_t parent_roots[]={41540,42560,43680,44800,45820,46840,47860,57472};
 MeleePc_FighterAnimationGraphsPlan(p,parent_roots,8);
 PICHU_CHECK("MeleePc_FighterAnimationGraphsPlan");
 MeleePc_FighterPartAnimationsPlan(p,targets[7],2,part_counts);
 PICHU_CHECK("MeleePc_FighterPartAnimationsPlan");
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
 PICHU_CHECK("MeleePc_FighterShieldDescriptorPlan");
 MeleePc_FighterWaitTablePlan(p,targets[9],320);
 PICHU_CHECK("MeleePc_FighterWaitTablePlan");
 MeleePc_FighterEmptyDynamicsPlan(p,targets[11]);
 PICHU_CHECK("MeleePc_FighterEmptyDynamicsPlan");
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
 PICHU_CHECK("MeleePc_FighterHurtboxesPlan");
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
 PICHU_CHECK("MeleePc_FighterSoundTablesPlan");
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
 PICHU_CHECK("MeleePc_FighterScalarChildPlan");
 MeleePc_FighterPartIdsPlan(p,targets[21]);
 PICHU_CHECK("MeleePc_FighterPartIdsPlan");
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
 PICHU_CHECK("MeleePc_FighterScalarChildPlan");
 if(p->error)return;
 title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
 /* Explicit disjoint unexported ShapeAnimJoint/DObj tree; not attached. */
 title_enqueue(p,120344,TITLE_HSD_ShapeAnimJoint);
 if(!fighter_sound_span(p,targets[18],12)){p->error=-1;return;}
 /* Pichu shares these exact item callback/attribute layouts with Pikachu.
  * Planner kind selects the layout only; original 82/91/92 values stay intact. */
 const unsigned kinds[]={81,89,90},states[]={1,2,1};
 if(title_u32(p,targets[1]+220)!=82 || title_u32(p,targets[1]+20)!=91 || title_u32(p,targets[1]+24)!=92){p->error=-1;return;}
 for(unsigned i=0;i<3&&!p->error;i++){
  uint32_t item;
  if(!fighter_sound_pointer(p,targets[18]+i*4,&item)||item==UINT32_MAX){p->error=-1;return;}
  title_pointer(p,targets[18]+i*4);
  MeleePc_ItemArticlePlan(p,item,kinds[i],states[i]);
  PICHU_CHECK("MeleePc_ItemArticlePlan");
 }
 while(p->head<p->count && !p->error)title_walk(p,p->nodes[p->head++]);
 PICHU_CHECK("complete typed graph");
 /* Unexported source DynamicModelDesc directories. Original descriptor
  * order is Joint, AnimJoint**, MatAnimJoint**, ShapeAnimJoint**. Reuse the
  * same typed array walker as Training scenes; never attach these lists. */
 title_array(p,82376,TITLE_DynamicModelDesc,0,1);
 title_array(p,143016,TITLE_DynamicModelDesc,0,1);
 while(p->head<p->count && !p->error)title_walk(p,p->nodes[p->head++]);
}
#undef PICHU_CHECK
#endif
