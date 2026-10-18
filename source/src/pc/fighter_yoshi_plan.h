#ifndef MELEE_PC_FIGHTER_YOSHI_PLAN_H
#define MELEE_PC_FIGHTER_YOSHI_PLAN_H
#include "fighter_attributes.h"
#include "fighter_yoshi_attributes.h"
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

/* Source-audited Yoshi component profile: caller separately validates exact isolated pointer/zero islands and refuses missing graph coverage. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_FighterYoshiPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 if(p->size!=271232||root!=37656){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==9||i==10)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
 MeleePc_FighterYoshiAttributesPlan(p,targets[1]);
 MeleePc_FighterModelPartsPlan(p,targets[2],6);
 const uint32_t tables[]={targets[3],targets[5]},counts[]={314,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
 MeleePc_FighterAnimationMapPlan(p,targets[4],314);
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
 const uint32_t parent_roots[]={38560,40784,43084,45384,47584,49784,52184,54584,56368,58152,59936,61736,63536,65336,67136,68936,70736,72536,74336};
 MeleePc_FighterAnimationGraphsPlan(p,parent_roots,19);
 const uint32_t part_counts[]={4,4,3,4,4};
 MeleePc_FighterPartAnimationsPlan(p,targets[7],5,part_counts);
 if(!fighter_sound_scalar(p,targets[8])||title_u32(p,targets[8])!=0){p->error=-1;return;}
 title_claim(p,targets[8],4,4);
 MeleePc_FighterDynamicsPlan(p,targets[11],0);
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
 /* ft_8007C630 initializes both Fighter.x1614[2] absorb capsules. */
 MeleePc_FighterScalarChildPlan(p,targets[14]+20,1);
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
 MeleePc_FighterPartIdsPlan(p,targets[21]);
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
 const unsigned item_kinds[]={86,88,87};
 const uint32_t item_states[]={2,1,0};
 for(unsigned i=0;i<3&&!p->error;i++){
  uint32_t article=title_pointer(p,targets[18]+i*4);
  if(article==UINT32_MAX){p->error=-1;return;}
  MeleePc_ItemArticlePlan(p,article,item_kinds[i],item_states[i]);
#ifdef MELEE_YOSHI_PLAN_DIAGNOSTIC
  fprintf(stderr,"YOSHI_PLAN_ARTICLE index=%u kind=%u error=%d\n",i,item_kinds[i],p->error);
#endif
 }
 if(p->error)return;

 /* ftYs_SpecialN_8012CDD4 supplies this accessory joint to
  * ftCommon_SetAccessory in ftCo_YoshiEgg; it is not an ItemArticle. */
 title_enqueue(p,title_pointer(p,targets[18]+12),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
}
#endif


