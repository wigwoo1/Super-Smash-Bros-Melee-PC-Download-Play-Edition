#ifndef MELEE_PC_FIGHTER_FALCO_PLAN_H
#define MELEE_PC_FIGHTER_FALCO_PLAN_H
#include "fighter_attributes.h"
#include "fighter_falco_attributes.h"
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
#include "fighter_falco_model_wrappers.h"
/* Source-audited Falco profile only. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_FighterFalcoPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 if(p->size!=223104||root!=39384){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==10)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
 MeleePc_FighterFalcoAttributesPlan(p,targets[1]);
 MeleePc_FighterModelPartsPlan(p,targets[2],4);
 const uint32_t tables[]={targets[3],targets[5]},counts[]={327,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
 MeleePc_FighterAnimationMapPlan(p,targets[4],327);
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
 const uint32_t part_counts[]={4,4,3,4,4};
 /* Run animation-only traversal before any model nodes are queued. */
 const uint32_t parent_roots[]={44552,46668,48884,51100,53216,55332,57648,59964,61348,62732,64116,65756,67396,69036,70676,72260,73844,75428,77012};
 MeleePc_FighterAnimationGraphsPlan(p,parent_roots,19);
 MeleePc_FighterPartAnimationsPlan(p,targets[7],5,part_counts);
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
 MeleePc_FighterWaitTablePlan(p,targets[9],327);
 
 
 MeleePc_FighterEmptyDynamicsPlan(p,targets[11]);
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
 MeleePc_FighterPartIdsPlan(p,targets[21]);
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
 if(p->error)return;
 title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
 if(!fighter_sound_span(p,targets[18],20)){p->error=-1;return;}
 const unsigned kinds[]={55,75,0,57,0},states[]={2,9,0,3,0};
 for(unsigned i=0;i<5&&!p->error;i++){
  uint32_t item;
  if(!fighter_sound_pointer(p,targets[18]+i*4,&item)){p->error=-1;return;}
  title_pointer(p,targets[18]+i*4);
  if(i==2||i==4){if(item!=UINT32_MAX)p->error=-2;continue;}
  if(item==UINT32_MAX){p->error=-1;return;}
  MeleePc_ItemArticlePlan(p,item,kinds[i],states[i]);
#ifdef MELEE_PC_TITLE_TRACE
  fprintf(stderr,"FALCO_ARTICLE slot=%u kind=%u error=%d\n",i,kinds[i],p->error);
#endif
 }
 MeleePc_FighterFalcoModelWrappersPlan(p);
}
#endif
