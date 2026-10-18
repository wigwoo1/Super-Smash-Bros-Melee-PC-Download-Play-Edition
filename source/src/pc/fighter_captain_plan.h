#ifndef MELEE_PC_FIGHTER_CAPTAIN_PLAN_H
#define MELEE_PC_FIGHTER_CAPTAIN_PLAN_H
#include "fighter_attributes.h"
#include "fighter_captain_attributes.h"
#include "fighter_animation_tables.h"
#include "fighter_animation_maps.h"
#include "fighter_part_animations.h"
#include "fighter_model_parts.h"
#include "fighter_scalar_children.h"
#include "fighter_hurtboxes.h"
#include "fighter_wait_table.h"
#include "fighter_empty_dynamics.h"
#include "fighter_auxiliary_fields.h"
/* Staged Captain profile only; production dispatch disabled. Caller owns scratch plan and commits
 * conversion only after full graph traversal and relocation coverage checks. */
static void MeleePc_FighterCaptainPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t targets[24];
 if(p->error)return;
 if(p->size!=150880||root!=39428){p->error=-2;return;}
 if(!fighter_sound_span(p,root,96)){p->error=-1;return;}
 for(unsigned i=0;i<24;i++){
  if(!fighter_sound_pointer(p,root+i*4,&targets[i])||((i==10||i==18)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
  title_pointer(p,root+i*4);
 }
 MeleePc_FighterAttributesPlan(p,targets[0],0);
 MeleePc_FighterCaptainAttributesPlan(p,targets[1]);
 MeleePc_FighterModelPartsPlan(p,targets[2],6);
 const uint32_t tables[]={targets[3],targets[5]},counts[]={318,14};
 MeleePc_FighterAnimationTablesPlan(p,tables,counts);
 /* Two exact unused goto records target an already source-counted stream.
  * Convert their pointer words without exposing new gameplay motions. */
 const uint32_t unused_roots[]={18596,18608};
 for(unsigned i=0;i<2;i++){
  uint32_t at=unused_roots[i];
  if(!title_range(p,at,8)||title_u32(p,at)!=0x1c000000U||
     title_u32(p,at+4)!=18496||!p->relocs[(at+4)/4]||p->claims[18496]!=1){p->error=-1;return;}
 }
 MeleePc_FighterActionStreamsPlan(p,unused_roots,2);

 MeleePc_FighterAnimationMapPlan(p,targets[4],318);
 MeleePc_FighterAnimationMapPlan(p,targets[6],14);
 const uint32_t part_counts[]={4,4,3};
 /* Run animation-only traversal before any model nodes are queued. */
 const uint32_t parent_roots[]={44584,46844,49304,51764,54024,56284,58844,61404,62752,64100,65448};
 MeleePc_FighterAnimationGraphsPlan(p,parent_roots,11);
 MeleePc_FighterPartAnimationsPlan(p,targets[7],3,part_counts);
 MeleePc_FighterShieldDescriptorPlan(p,targets[8]);
 MeleePc_FighterWaitTablePlan(p,targets[9],318);
 
 
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
}
#endif
