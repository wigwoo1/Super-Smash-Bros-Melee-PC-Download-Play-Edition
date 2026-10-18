#ifndef MELEE_PC_FIGHTER_LUIGI_PLAN_H
#define MELEE_PC_FIGHTER_LUIGI_PLAN_H
#include "fighter_attributes.h"
#include "fighter_luigi_attributes.h"
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
#include "fighter_luigi_model_wrappers.h"
/* Staged Luigi profile only; production dispatch disabled. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_FighterLuigiPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 if(p->size!=161728||root!=36488){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==10)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterAttributesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterLuigiAttributesPlan(p,targets[1]);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterLuigiAttributesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterModelPartsPlan(p,targets[2],4);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterModelPartsPlan error=%d\n",p->error);return;}
#endif
 const uint32_t tables[]={targets[3],targets[5]},counts[]={312,16};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterAnimationTablesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterAnimationMapPlan(p,targets[4],312);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterAnimationMapPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterAnimationMapPlan(p,targets[6],16);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterAnimationMapPlan error=%d\n",p->error);return;}
#endif
 const uint32_t part_counts[]={4,4,3};
 /* Run animation-only traversal before any model nodes are queued. */
 const uint32_t parent_roots[]={41456,43620,45884,48148,50312,52476,54840,57204,58468,59732,60996};
 MeleePc_FighterAnimationGraphsPlan(p,parent_roots,11);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterAnimationGraphsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterPartAnimationsPlan(p,targets[7],3,part_counts);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterPartAnimationsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterShieldDescriptorPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterWaitTablePlan(p,targets[9],312);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterWaitTablePlan error=%d\n",p->error);return;}
#endif
 
 
 MeleePc_FighterEmptyDynamicsPlan(p,targets[11]);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterEmptyDynamicsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterHurtboxesPlan error=%d\n",p->error);return;}
#endif
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterScalarChildPlan error=%d\n",p->error);return;}
#endif
 MeleePc_ItemArticlePlan(p,title_pointer(p,targets[18]),105,1);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE ItemArticlePlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterSoundTablesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterScalarChildPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterPartIdsPlan(p,targets[21]);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterPartIdsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
#ifdef MELEE_LUIGI_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"LUIGI_PLAN_PHASE FighterScalarChildPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterLuigiModelWrappersPlan(p);
 if(p->error)return;
 title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
}
#endif


