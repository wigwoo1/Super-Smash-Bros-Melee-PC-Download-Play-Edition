#ifndef MELEE_PC_FIGHTER_SAMUS_PLAN_H
#define MELEE_PC_FIGHTER_SAMUS_PLAN_H
#include "fighter_attributes.h"
#include "fighter_samus_attributes.h"
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
#include "samus_staged_wrappers.h"
#include "item_samus_grapple_prefix.h"
#include "samus_grapple_local_fields.h"
#include "samus_residual_pointers.h"
/* Staged Samus profile only; production dispatch disabled. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_FighterSamusPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 if(p->size!=303808||root!=39548){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==9||i==10)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterAttributesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterSamusAttributesPlan(p,targets[1]);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterSamusAttributesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterModelPartsPlan(p,targets[2],5);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterModelPartsPlan error=%d\n",p->error);return;}
#endif
 const uint32_t tables[]={targets[3],targets[5]},counts[]={313,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterAnimationTablesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterAnimationMapPlan(p,targets[4],313);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterAnimationMapPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterAnimationMapPlan error=%d\n",p->error);return;}
#endif
 const uint32_t part_counts[]={4};
 /* Run animation-only traversal before any model nodes are queued. */
 const uint32_t parent_roots[]={44488,46888,49288,51688};
 MeleePc_FighterAnimationGraphsPlan(p,parent_roots,4);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterAnimationGraphsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterPartAnimationsPlan(p,targets[7],1,part_counts);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterPartAnimationsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterShieldDescriptorPlan error=%d\n",p->error);return;}
#endif
 /* Samus wait table absent in original root. */
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterWaitTablePlan error=%d\n",p->error);return;}
#endif
 
 
 MeleePc_FighterEmptyDynamicsPlan(p,targets[11]);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterEmptyDynamicsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterHurtboxesPlan error=%d\n",p->error);return;}
#endif
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterScalarChildPlan error=%d\n",p->error);return;}
#endif
 const unsigned article_kinds[]={93,94,95};
 const uint32_t article_states[]={2,9,4};
 for(unsigned i=0;i<3;i++)MeleePc_ItemArticlePlan(p,title_pointer(p,targets[18]+i*4),article_kinds[i],article_states[i]);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE ItemArticlePlan error=%d\n",p->error);return;}
#endif
 /* Grapple has zero state descriptors and pointer-bearing special attrs. */
 uint32_t grapple=title_pointer(p,targets[18]+12),g[6];
 for(unsigned i=0;i<6;i++)g[i]=title_pointer(p,grapple+i*4);
 if(g[0]!=16780||g[1]!=16912||g[2]!=UINT32_MAX||g[3]!=UINT32_MAX||g[4]!=39456||g[5]!=UINT32_MAX){p->error=-1;return;}
 MeleePc_ItemArticleAttributesPlan(p,g[0]);
 MeleePc_ItemSamusGrapplePrefixPlan(p,g[1]);
 MeleePc_SamusGrappleLocalFieldsPlan(p);
 MeleePc_ItemModelDescriptorPlan(p,g[4]);
 const uint32_t grapple_joints[]={105160,104168,97672,71496};
 for(unsigned i=0;i<4;i++)title_enqueue(p,grapple_joints[i],TITLE_HSD_Joint);
 const uint32_t grapple_anims[]={105440,104448,99816,88184,97228};
 const uint32_t grapple_mats[]={105696,104704,100072};
 for(unsigned i=0;i<5;i++)title_enqueue(p,grapple_anims[i],TITLE_HSD_AnimJoint);
 for(unsigned i=0;i<3;i++)title_enqueue(p,grapple_mats[i],TITLE_HSD_MatAnimJoint);
 /* Caller validated original external chains before invoking this plan. */
 if(!p->external_slots){p->error=-1;return;}
 for(uint32_t i=0;i<(p->size+3)/4;i++)if(p->external_slots[i])title_pointer(p,i*4);
 if(p->error)return;
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterSoundTablesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterScalarChildPlan(p,targets[14]+20,1);
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterScalarChildPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterPartIdsPlan(p,targets[21]);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterPartIdsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
#ifdef MELEE_SAMUS_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"SAMUS_PLAN_PHASE FighterScalarChildPlan error=%d\n",p->error);return;}
#endif
 /* Source UNK_SAMUS_S1 throw accessory, four standard throw animations. */
 uint32_t beam=title_pointer(p,targets[18]+16);
 if(beam!=39512){p->error=-1;return;}
 const uint32_t beam_expected[]={107688,39496,217816,219200};
 for(unsigned i=0;i<4;i++)if(title_pointer(p,beam+i*4)!=beam_expected[i]){p->error=-1;return;}
 const uint32_t throw_expected[]={174568,153828,191168,212868};
 for(unsigned i=0;i<4;i++)if(title_pointer(p,39496+i*4)!=throw_expected[i]){p->error=-1;return;}
 title_enqueue(p,107688,TITLE_HSD_Joint);
 title_enqueue(p,153828,TITLE_HSD_AnimJoint);
 title_enqueue(p,174568,TITLE_HSD_AnimJoint);
 title_enqueue(p,191168,TITLE_HSD_AnimJoint);
 title_enqueue(p,212868,TITLE_HSD_AnimJoint);
 title_enqueue(p,217816,TITLE_HSD_AnimJoint);
 title_enqueue(p,219200,TITLE_HSD_MatAnimJoint);
 MeleePc_SamusStagedWrappersPlan(p);
 MeleePc_SamusResidualPointersPlan(p);
 /* Detached archive graphs: conversion coverage only, not runtime ownership. */
 title_enqueue(p,56040,TITLE_HSD_Joint);
 title_enqueue(p,65164,TITLE_HSD_AnimJoint);
 title_enqueue(p,65560,TITLE_HSD_MatAnimJoint);
 title_enqueue(p,65616,TITLE_HSD_ShapeAnimJoint);
 title_enqueue(p,68648,TITLE_HSD_Joint);
 title_enqueue(p,70636,TITLE_HSD_AnimJoint);
 title_enqueue(p,70780,TITLE_HSD_MatAnimJoint);
 title_enqueue(p,70860,TITLE_HSD_ShapeAnimJoint);
 title_enqueue(p,220680,TITLE_HSD_Joint);
 title_enqueue(p,232064,TITLE_HSD_AnimJoint);
 title_enqueue(p,232272,TITLE_HSD_MatAnimJoint);
 title_enqueue(p,232392,TITLE_HSD_ShapeAnimJoint);

 if(p->error)return;
 title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
}
#endif


