#ifndef MELEE_PC_FIGHTER_SHEIK_PLAN_H
#define MELEE_PC_FIGHTER_SHEIK_PLAN_H
#include "fighter_attributes.h"
#include "fighter_sheik_attributes.h"
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
/* Source-audited Sheik profile only. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_FighterSheikPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 if(p->size!=156352||root!=38052){p->error=-2;return;}
 /* Exact original needle sound0 parameter word aliases hurtbox bone1.
  * Sheik parser must attach immutable original bytes before runtime use. */
 if(title_u32(p,14852)!=0x40000000 || title_u32(p,14856)!=0 ||
    title_u32(p,14860)!=1 || title_u32(p,14864)!=0 ||
    title_u32(p,37824)!=1 || title_u32(p,37828)!=14860 ||
    !p->relocs[37828/4] || p->relocs[14860/4]){p->error=-1;return;}
 p->sound_alias_word=14860;p->sound_alias_enabled=1;
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==10)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
 MeleePc_FighterSheikAttributesPlan(p,targets[1]);
 MeleePc_FighterModelPartsPlan(p,targets[2],5);
 const uint32_t tables[]={targets[3],targets[5]},counts[]={317,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
 MeleePc_FighterAnimationMapPlan(p,targets[4],317);
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
 const uint32_t part_counts[]={4,4};
 /* Run animation-only traversal before any model nodes are queued. */
 MeleePc_FighterPartAnimationsPlan(p,targets[7],2,part_counts);
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
 
 
 MeleePc_FighterWaitTablePlan(p,targets[9],317);
 MeleePc_FighterDynamicsPlan(p,targets[11],0);
 MeleePc_FighterHurtboxesPlan(p,targets[12]);
 for(unsigned i=0;i<5;i++)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
 /* ft_8007C630 initializes both Fighter::x1614 entries from AbsorbDesc[2].
  * Sheik's camera child immediately follows this 40-byte array. */
 if(targets[14]!=29224 || targets[15]!=targets[14]+40){p->error=-1;return;}
 MeleePc_FighterScalarChildPlan(p,targets[14]+20,1);
 const unsigned item_kinds[]={79,80,85,97},state_counts[]={5,1,1,0};
 for(unsigned i=0;i<4&&!p->error;i++){
  uint32_t article=title_pointer(p,targets[18]+i*4);
  MeleePc_ItemArticlePlan(p,article,item_kinds[i],state_counts[i]);
 }
 /* The side-special callback selects items[4] or items[5] as HSD_Joint**
  * and passes element2 to the transition animation routines. These are
  * source-used joint subtrees, not registered item/article descriptors. */
 for(unsigned i=4;i<6&&!p->error;i++){
  uint32_t at=targets[18]+i*4;
  uint32_t joint=title_pointer(p,at);
  if(joint==UINT32_MAX||(joint&3)||!title_range(p,joint,64)){p->error=-1;break;}
  title_enqueue(p,joint,TITLE_HSD_Joint);
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
