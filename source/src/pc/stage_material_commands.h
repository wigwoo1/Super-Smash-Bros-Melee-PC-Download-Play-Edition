#ifndef MELEE_PC_STAGE_MATERIAL_COMMANDS_H
#define MELEE_PC_STAGE_MATERIAL_COMMANDS_H
#include "item_color_commands.h"
/* Stage material overlays share the color command grammar. Custom callbacks
 * (opcode21+) stay unsupported until their extent and semantics are proven. */
static void MeleePc_StageMaterialCommandsPlan(MeleePc_TitlePlan* p,uint32_t root){
 MeleePc_ColorStreamsPlan(p,&root,1);
}
#endif
