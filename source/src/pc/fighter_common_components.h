#ifndef MELEE_PC_FIGHTER_COMMON_COMPONENTS_H
#define MELEE_PC_FIGHTER_COMMON_COMPONENTS_H
#include "fighter_common_root.h"
#include "fighter_common_parameters.h"
#include "fighter_common_action_scalars.h"
#include "fighter_common_parts.h"
#include "fighter_common_part_events.h"
#include "fighter_color_commands.h"
#include "fighter_common_damage_shift.h"
#include "fighter_common_shake.h"
#include "fighter_common_modifiers.h"
#include "fighter_common_crowd.h"
#include "fighter_common_models.h"
#include "fighter_cpu_scripts.h"
#include "fighter_cpu_attacks.h"
#include "fighter_common_player_colors.h"
/* Composition preflight only. Unknown roots, nonstandard CPU entries and
 * complete relocation coverage must be resolved before production dispatch. */
static void MeleePc_FighterCommonComponentsPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t roots[23];if(MeleePc_FighterCommonRootPlan(p,root,roots))return;
 MeleePc_FighterCommonParametersPlan(p,roots[0]);
 for(unsigned i=1;i<=3;i++)MeleePc_FighterCommonActionScalarsPlan(p,roots[i],i);
 MeleePc_FighterCommonPartsPlan(p,roots[4]);
 MeleePc_FighterCommonPartEventsPlan(p,roots[5]);
 MeleePc_FighterColorCommandsPlan(p,roots[6],123);
 MeleePc_FighterColorCommandsPlan(p,roots[7],6);
 MeleePc_FighterCommonDamageShiftPlan(p,roots[9]);
 MeleePc_FighterCommonShakePlan(p,roots[10]);MeleePc_FighterCommonShakePlan(p,roots[11]);
 for(unsigned i=12;i<=15;i++)MeleePc_FighterCommonModifierPlan(p,roots[i],i);
 MeleePc_FighterCommonCrowdPlan(p,roots[21]);
 MeleePc_FighterCommonModelsPlan(p,roots);
 MeleePc_FighterCommonPlayerColorPlan(p,roots[18]);MeleePc_FighterCommonPlayerColorPlan(p,roots[19]);
 if(p->error)return;
 uint32_t ai=roots[22],fields[10];
 if((ai&3) || !title_range(p,ai,40)){p->error=-1;return;}
 for(unsigned i=0;i<10;i++)fields[i]=title_pointer(p,ai+i*4);
 MeleePc_FighterCpuScriptsPlan(p,fields[0],62);
 for(unsigned i=1;i<8;i++)MeleePc_FighterCpuAttackTablePlan(p,fields[i],32,62);
 for(unsigned i=8;i<10 && !p->error;i++){
  uint32_t at=fields[i],count=i==8?32:6;
  if(at==UINT32_MAX || (at&3) || !title_range(p,at,count*4)){p->error=-1;return;}
  for(unsigned j=0;j<count;j++){
   uint32_t slot=at+j*4;
   if(p->relocs[slot/4] || (p->external_slots && p->external_slots[slot/4]) || (title_u32(p,slot)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
  }
  title_claim(p,at,count*4,4);
 }
}
#endif
