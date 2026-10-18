#ifndef MELEE_PC_ITEM_TOOLS_ATTRIBUTES_H
#define MELEE_PC_ITEM_TOOLS_ATTRIBUTES_H
#include "item_dynamics.h"
/* itToolsAttributes header plus five itToolsMotionAttrs. Flat Zone spawns
 * HSD_Randi(5); callbacks retain that index, with animation IDs index and index+5. */
static void MeleePc_ItemToolsAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,156)){p->error=-1;return;}
 item_dynamics_scalars(p,root,12,0);
 item_dynamics_scalars(p,root+12,4,4);
 item_dynamics_scalars(p,root+16,140,0);
}
#endif
