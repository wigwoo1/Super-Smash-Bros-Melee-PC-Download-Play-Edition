#ifndef MELEE_PC_STAGE_ITEM_SCRIPTS_H
#define MELEE_PC_STAGE_ITEM_SCRIPTS_H
#include "item_command_plan.h"
#include "item_state_extents.h"
/* Ground_801C0784 installs ALDYakuAll entries into item state scripts.
 * Use the same packed item grammar; command words remain BE. */
static void stage_item_script(MeleePc_TitlePlan* p,uint32_t at){
 MeleePc_ItemCommandPlan(p,&at,1);
}
static void MeleePc_StageItemScriptsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,4)){p->error=-1;return;}
 if(title_u32(p,root) || p->relocs[root/4]){p->error=-2;return;}
 title_claim(p,root,4,4);
 /* Ground installs into kind160's source-consumed state prefix. The final
  * permitted list entry is a terminator, never a descriptor write. */
 uint32_t capacity=melee_pc_item_state_extents[160];
 for(uint32_t i=1;i<=capacity && !p->error;i++){
  uint32_t script=title_pointer(p,root+i*4);
  if(script==UINT32_MAX)return;
  if(i==capacity){p->error=-2;return;}
  stage_item_script(p,script);
 }
 if(!p->error)p->error=-1;
}
#endif
