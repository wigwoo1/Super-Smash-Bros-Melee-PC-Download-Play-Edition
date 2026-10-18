#ifndef MELEE_PC_FIGHTER_DRMARIO_PLAN_H
#define MELEE_PC_FIGHTER_DRMARIO_PLAN_H
#include "fighter_attributes.h"
#include "fighter_drmario_attributes.h"
#include "fighter_animation_tables.h"
#include "fighter_animation_maps.h"
#include "fighter_part_animations.h"
#include "fighter_model_parts.h"
#include "fighter_scalar_children.h"
#include "fighter_hurtboxes.h"
#include "fighter_wait_table.h"
#include "fighter_dynamics.h"
#include "fighter_auxiliary_fields.h"
#include "item_article_plan.h"
#include "fighter_drmario_model_wrappers.h"
/* Source-audited Dr. Mario candidate profile only. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_FighterDrMarioPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 /* Explicit audited auxiliary graphs; convert serialized data only. */
 if(p->size!=228384||root!=37192){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==10)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
 MeleePc_FighterDrMarioAttributesPlan(p,targets[1]);
 MeleePc_FighterModelPartsPlan(p,targets[2],5);
 const uint32_t tables[]={targets[3],targets[5]},counts[]={303,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
 MeleePc_FighterAnimationMapPlan(p,targets[4],303);
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
 const uint32_t auxiliary[]={45672,50248,54924,59600,64176,68752,73528,78304,82660,87016,91372};
 MeleePc_FighterAnimationGraphsPlan(p,auxiliary,11);
 const uint32_t part_counts[]={4,4,3};
 /* Run animation-only traversal before any model nodes are queued. */
 MeleePc_FighterPartAnimationsPlan(p,targets[7],3,part_counts);
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
 
 
 MeleePc_FighterWaitTablePlan(p,targets[9],303);
 MeleePc_FighterDynamicsPlan(p,targets[11],0);
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
 for(unsigned i=0;i<4;i++){
  uint32_t article=title_pointer(p,targets[18]+i*4);
  if(i==1||i==3){if(article==UINT32_MAX){p->error=-1;return;}MeleePc_ItemArticlePlan(p,article,i==1?49:84,i==1?6:2);}
  else if(article!=UINT32_MAX){p->error=-1;return;}
 }
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
 MeleePc_FighterPartIdsPlan(p,targets[21]);
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
 MeleePc_FighterDrMarioModelWrappersPlan(p);
 if(p->error)return;
 title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
 /* Independently audited serialized article model graphs; no live attachment. */
 title_enqueue(p,125320,TITLE_HSD_Joint);
 title_enqueue(p,137192,TITLE_HSD_Joint);
 title_enqueue(p,149480,TITLE_HSD_Joint);
 title_enqueue(p,131876,TITLE_HSD_MatAnimJoint);
 title_enqueue(p,143812,TITLE_HSD_MatAnimJoint);
 title_enqueue(p,150716,TITLE_HSD_MatAnimJoint);

}
#endif

