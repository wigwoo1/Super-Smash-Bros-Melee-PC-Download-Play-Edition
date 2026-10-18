#ifndef MELEE_PC_FIGHTER_PURIN_PLAN_H
#define MELEE_PC_FIGHTER_PURIN_PLAN_H
#include "fighter_attributes.h"
#include "fighter_purin_attributes.h"
#include "fighter_animation_tables.h"
#include "fighter_animation_maps.h"
#include "fighter_part_animations.h"
#include "fighter_model_parts.h"
#include "fighter_scalar_children.h"
#include "fighter_hurtboxes.h"
#include "fighter_wait_table.h"
#include "fighter_purin_dynamics.h"
#include "fighter_auxiliary_fields.h"
#include "fighter_purin_hat_visibility.h"
#include "item_article_plan.h"

/* Staged Purin component profile: missing graph coverage is refused by caller. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_FighterPurinPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 if(p->size!=107520||root!=37012){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==9)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
 MeleePc_FighterPurinAttributesPlan(p,targets[1]);
 MeleePc_FighterModelPartsPlan(p,targets[2],5);
 const uint32_t tables[]={targets[3],targets[5]},counts[]={327,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
 MeleePc_FighterAnimationMapPlan(p,targets[4],327);
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
 const uint32_t parent_roots[]={40420,41520,43720,45920,48120};
 MeleePc_FighterAnimationGraphsPlan(p,parent_roots,5);
 const uint32_t part_counts[]={2,3};
 MeleePc_FighterPartAnimationsPlan(p,targets[7],2,part_counts);
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
 MeleePc_FighterWaitTablePlan(p,targets[10],327);
 
 
 MeleePc_FighterPurinDynamicsPlan(p,targets[11]);
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
 MeleePc_FighterPartIdsPlan(p,targets[21]);
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
 MeleePc_FighterPurinHatVisibilityPlan(p,targets[18],5);
 if(p->error)return;
 title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
}
#endif


