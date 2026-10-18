#ifndef MELEE_PC_FIGHTER_COMMON_ROOT_H
#define MELEE_PC_FIGHTER_COMMON_ROOT_H
#include "title_archive.h"
/* Fighter_LoadCommonData copies these 23 pointer fields to typed globals.
 * This preflight does not infer target extents or convert their payloads. */
static int MeleePc_FighterCommonRootPlan(MeleePc_TitlePlan* p,uint32_t root,uint32_t targets[23]){
 if(p->error)return p->error;
 if((root&3) || !title_range(p,root,92)){p->error=-1;return p->error;}
 for(unsigned i=0;i<23 && !p->error;i++)targets[i]=title_pointer(p,root+i*4);
 return p->error;
}
#endif
