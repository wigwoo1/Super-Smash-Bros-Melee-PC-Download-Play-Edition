#ifndef MELEE_PC_FIGHTER_PEACH_PLAN_H
#define MELEE_PC_FIGHTER_PEACH_PLAN_H
#include "fighter_attributes.h"
#include "fighter_peach_attributes.h"
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
#include "item_peach_attributes.h"
#include "fighter_dynamics.h"
#include "peach_staged_wrappers.h"
/* Exact Peach profile, registered by the guarded production dispatcher. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_PeachStagedArticlePlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind,uint32_t count){
 if(p->error||(root&3)||!title_range(p,root,24)){p->error=-1;return;}
 uint32_t t[6];for(unsigned i=0;i<6&&!p->error;i++)t[i]=item_dynamics_pointer(p,root+i*4);
 if(p->error)return;
 if(t[0]!=UINT32_MAX)MeleePc_ItemArticleAttributesPlan(p,t[0]);
 if(t[1]!=UINT32_MAX&&(kind==99||kind==111))MeleePc_ItemPeachAttributesPlan(p,t[1],kind);
 if(t[2]!=UINT32_MAX)MeleePc_ItemHurtbonesPlan(p,t[2]);
 if(count&&t[3]==UINT32_MAX){p->error=-1;return;}
 if(t[3]!=UINT32_MAX){MeleePc_ItemStateAnimationsPlan(p,t[3],count);
  for(unsigned i=0;i<count&&!p->error;i++){uint32_t command=title_pointer(p,t[3]+i*16+12);MeleePc_ItemCommandPlan(p,&command,1);}}
 if(t[4]!=UINT32_MAX)MeleePc_ItemModelDescriptorPlan(p,t[4]);
 if(t[5]!=UINT32_MAX)MeleePc_ItemDynamicsPlan(p,t[5]);
}
static void MeleePc_FighterPeachPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 if(p->size!=251072||root!=45948){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==10)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterAttributesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterPeachAttributesPlan(p,targets[1]);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterPeachAttributesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterModelPartsPlan(p,targets[2],5);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterModelPartsPlan error=%d\n",p->error);return;}
#endif
 const uint32_t tables[]={targets[3],targets[5]},counts[]={318,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterAnimationTablesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterAnimationMapPlan(p,targets[4],318);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterAnimationMapPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterAnimationMapPlan error=%d\n",p->error);return;}
#endif
 const uint32_t part_counts[]={4,4,3};
 /* Run animation-only traversal before any model nodes are queued. */
 const uint32_t parent_roots[]={54344,57624,61004,64384,67704,71028,74508,77988,80468,82948,85428};
 MeleePc_FighterAnimationGraphsPlan(p,parent_roots,11);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterAnimationGraphsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterPartAnimationsPlan(p,targets[7],3,part_counts);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterPartAnimationsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterShieldDescriptorPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterWaitTablePlan(p,targets[9],318);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterWaitTablePlan error=%d\n",p->error);return;}
#endif
 
 
 MeleePc_FighterDynamicsPlan(p,targets[11],86);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterEmptyDynamicsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterHurtboxesPlan error=%d\n",p->error);return;}
#endif
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
 /* ft_8007C630 consumes both x1614 entries. AbsorbDesc[2] spans
  * body33168..33207, immediately before the camera descriptor. */
 if(targets[14]!=33168||targets[15]!=33208){p->error=-2;return;}
 MeleePc_FighterScalarChildPlan(p,targets[14]+20,1);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterScalarChildPlan error=%d\n",p->error);return;}
#endif
 MeleePc_PeachStagedArticlePlan(p,title_pointer(p,targets[18]+0),98,2);
MeleePc_PeachStagedArticlePlan(p,title_pointer(p,targets[18]+4),99,3);
MeleePc_PeachStagedArticlePlan(p,title_pointer(p,targets[18]+8),103,2);
MeleePc_PeachStagedArticlePlan(p,title_pointer(p,targets[18]+12),104,2);
MeleePc_PeachStagedArticlePlan(p,title_pointer(p,targets[18]+16),111,1);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE ItemArticlePlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterSoundTablesPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterScalarChildPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterPartIdsPlan(p,targets[21]);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterPartIdsPlan error=%d\n",p->error);return;}
#endif
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
#ifdef MELEE_PEACH_PLAN_DIAGNOSTIC
 if(p->error){fprintf(stderr,"PEACH_PLAN_PHASE FighterScalarChildPlan error=%d\n",p->error);return;}
#endif
 MeleePc_PeachStagedWrappersPlan(p);
 if(p->error)return;
 title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
}
#endif


