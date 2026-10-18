#ifndef MELEE_PC_BACKGROUND_FLASH_PLAN_H
#define MELEE_PC_BACKGROUND_FLASH_PLAN_H
#include "item_color_commands.h"
/* Exact LbBf profile: source 8-byte descriptors, complete serialized extent. */
static void MeleePc_BackgroundFlashPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=712||root!=584){p->error=-2;return;}
 MeleePc_ItemColorCommandsPlan(p,root,16);
 for(uint32_t i=0;i<(p->size+3)/4&&!p->error;i++)if(p->relocs[i]&&p->relocs[i]!=2)p->error=-2;
}
#endif
