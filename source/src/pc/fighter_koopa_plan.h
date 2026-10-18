#ifndef MELEE_PC_FIGHTER_KOOPA_PLAN_H
#define MELEE_PC_FIGHTER_KOOPA_PLAN_H
#include "fighter_attributes.h"
#include "fighter_koopa_attributes.h"
#include "fighter_animation_tables.h"
#include "fighter_animation_maps.h"
#include "fighter_part_animations.h"
#include "fighter_model_parts.h"
#include "fighter_scalar_children.h"
#include "fighter_hurtboxes.h"
#include "fighter_wait_table.h"
#include "fighter_bone_dynamics.h"
#include "fighter_auxiliary_fields.h"
#include "item_article_plan.h"
/* Source-audited Koopa profile only. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_FighterKoopaPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 if(p->size!=177120||root!=39604){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==10)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
 MeleePc_FighterKoopaAttributesPlan(p,targets[1]);
 MeleePc_FighterModelPartsPlan(p,targets[2],4);
 const uint32_t tables[]={targets[3],targets[5]},counts[]={316,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
 MeleePc_FighterAnimationMapPlan(p,targets[4],316);
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
 const uint32_t part_counts[]={4,4,3};
 /* Run animation-only traversal before any model nodes are queued. */
 const uint32_t auxiliary_roots[]={47736,52416,57168,61920,66600,71280,76244,81208,85688,90168,94648};
 MeleePc_FighterAnimationGraphsPlan(p,auxiliary_roots,11);
 MeleePc_FighterPartAnimationsPlan(p,targets[7],3,part_counts);
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
 MeleePc_FighterWaitTablePlan(p,targets[9],316);
 
 
 MeleePc_FighterBoneDynamicsPlan(p,targets[11]);
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
 MeleePc_FighterSoundTablesPlan(p,targets[19]);
 MeleePc_FighterScalarChildPlan(p,targets[20],5);
 MeleePc_FighterPartIdsPlan(p,targets[21]);
 MeleePc_FighterScalarChildPlan(p,targets[22],6);
 if(p->error)return;
 title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
 title_enqueue(p,targets[23],TITLE_HSD_Joint);
 if(!fighter_sound_span(p,targets[18],4)){p->error=-1;return;}
 uint32_t item;
 if(!fighter_sound_pointer(p,targets[18],&item)||item==UINT32_MAX){p->error=-1;return;}
 title_pointer(p,targets[18]);
 MeleePc_ItemArticlePlan(p,item,100,1);
}
#endif
