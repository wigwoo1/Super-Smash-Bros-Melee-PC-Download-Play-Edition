#ifndef MELEE_PC_FIGHTER_ZELDA_PLAN_H
#define MELEE_PC_FIGHTER_ZELDA_PLAN_H
#include "fighter_attributes.h"
#include "fighter_zelda_attributes.h"
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
/* Source-audited Zelda profile only. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_FighterZeldaPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 /* Explicit candidate auxiliary graphs independently checked against source
  * layouts. Full archive ownership/runtime acceptance remains pending. */
 if(p->size!=172672||root!=41248){p->error=-2;return;}
 const uint32_t auxiliary[]={49840,53144,56504,59864,63168,66472,69976,73480,76040,78600,81160};
 MeleePc_FighterAnimationGraphsPlan(p,auxiliary,11);
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==10)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
 MeleePc_FighterZeldaAttributesPlan(p,targets[1]);
 MeleePc_FighterModelPartsPlan(p,targets[2],5);
 const uint32_t tables[]={targets[3],targets[5]},counts[]={311,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
 MeleePc_FighterAnimationMapPlan(p,targets[4],311);
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
 const uint32_t part_counts[]={4,4,3};
 /* Run animation-only traversal before any model nodes are queued. */
 MeleePc_FighterPartAnimationsPlan(p,targets[7],3,part_counts);
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
 
 
 MeleePc_FighterWaitTablePlan(p,targets[9],311);
 MeleePc_FighterDynamicsPlan(p,targets[11],36);
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
 /* ft_8007C630 initializes both Fighter::x1614 entries from AbsorbDesc[2].
  * Zelda's camera child immediately follows this 40-byte array. */
 if(targets[14]!=31396 || targets[15]!=targets[14]+40){p->error=-1;return;}
 MeleePc_FighterScalarChildPlan(p,targets[14]+20,1);
 for(unsigned i=0;i<2&&!p->error;i++){
  uint32_t article=title_pointer(p,targets[18]+i*4);
  MeleePc_ItemArticlePlan(p,article,108+i,i?1:2);
 }
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
 MeleePc_FighterPartIdsPlan(p,targets[21]);
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
 if(p->error)return;
 title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
}
#endif
