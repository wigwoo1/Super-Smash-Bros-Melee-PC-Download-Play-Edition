#ifndef MELEE_PC_FIGHTER_FOX_PLAN_H
#define MELEE_PC_FIGHTER_FOX_PLAN_H
#include "fighter_attributes.h"
#include "fighter_falco_attributes.h"
#include "fighter_animation_tables.h"
#include "fighter_animation_maps.h"
#include "fighter_part_animations.h"
#include "fighter_model_parts.h"
#include "fighter_scalar_children.h"
#include "fighter_hurtboxes.h"
#include "fighter_wait_table.h"
#include "fighter_fox_dynamics.h"
#include "fighter_auxiliary_fields.h"
#include "item_article_plan.h"
#include "fighter_fox_model_wrappers.h"

/* Staged Fox profile; never enabled in production dispatch. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_FighterFoxPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 if(p->size!=244960||root!=39156){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==10)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterAttributesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterFalcoAttributesPlan(p,targets[1]);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterFalcoAttributesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterModelPartsPlan(p,targets[2],4);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterModelPartsPlan error=%d\n",p->error);return;}
#endif
 const uint32_t tables[]={targets[3],targets[5]},counts[]={327,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterAnimationTablesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterAnimationMapPlan(p,targets[4],327);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterAnimationMapPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterAnimationMapPlan error=%d\n",p->error);return;}
#endif
 const uint32_t part_counts[]={4,4,3,4,4};
 /* Run animation-only traversal before any model nodes are queued. */
 /* Source-connected parent forest passes independent disjointness/DAG audit. */
 const uint32_t parent_roots[]={44712,46948,49384,51820,54056,56292,58828,61364,62868,64372,65876,67568,69260,70952,72644,74336,76028,77720,79412};
 MeleePc_FighterAnimationGraphsPlan(p,parent_roots,19);
 /* Fox model wrappers remain under audit. */
 MeleePc_FighterPartAnimationsPlan(p,targets[7],5,part_counts);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterPartAnimationsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterShieldDescriptorPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterWaitTablePlan(p,targets[9],327);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterWaitTablePlan error=%d\n",p->error);return;}
#endif
 
 
 MeleePc_FighterFoxDynamicsPlan(p,targets[11]);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterFoxDynamicsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterHurtboxesPlan error=%d\n",p->error);return;}
#endif
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterSoundTablesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterScalarChildPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterPartIdsPlan(p,targets[21]);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterPartIdsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
#ifdef MELEE_PC_TITLE_TRACE
 if(p->error){fprintf(stderr,"FOX_PLAN_STAGE MeleePc_FighterScalarChildPlan error=%d\n",p->error);return;}
#endif
 if(p->error)return;
 title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
 if(!fighter_sound_span(p,targets[18],20)){p->error=-1;return;}
 const unsigned kinds[]={54,74,56,0,0},states[]={2,9,3,0,0};
 for(unsigned i=0;i<5&&!p->error;i++){
  uint32_t item;
  if(!fighter_sound_pointer(p,targets[18]+i*4,&item)){p->error=-1;return;}
  title_pointer(p,targets[18]+i*4);
  if(i==3){if(item!=UINT32_MAX)p->error=-1;continue;}
  if(i==4){
   /* Fox callbacks only consume slots0..2. Preserve the unused extra
    * target as opaque bytes, without guessing its type or allocation extent. */
   MeleePc_FighterFoxModelWrappersPlan(p);
   while(p->head<p->count&&!p->error){MeleePc_TitleNode node=p->nodes[p->head++];title_walk(p,node);}
   if(!p->error){
    if(item!=30224||!title_range(p,item,24)){p->error=-1;return;}
    for(unsigned off=0;off<24;off++)if(p->claims[item+off]){p->error=-1;return;}
    for(unsigned off=0;off<24;off+=4)
     if(p->relocs[(item+off)/4]||(p->external_slots&&p->external_slots[(item+off)/4])){p->error=-1;return;}
   }
   continue;
  }
  if(item==UINT32_MAX){p->error=-1;return;}
  MeleePc_ItemArticlePlan(p,item,kinds[i],states[i]);
#ifdef MELEE_PC_TITLE_TRACE
  fprintf(stderr,"FOX_ARTICLE slot=%u kind=%u error=%d\n",i,kinds[i],p->error);
#endif
 }

}
#endif
