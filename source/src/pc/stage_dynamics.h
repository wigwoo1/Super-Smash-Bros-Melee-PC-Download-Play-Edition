#ifndef MELEE_PC_STAGE_DYNAMICS_H
#define MELEE_PC_STAGE_DYNAMICS_H
#include "item_dynamics.h"
/* DynamicsTemplate consumed by grLib_801C9B20/lb_80011710. */
static void MeleePc_StageDynamicsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,20)){p->error=-1;return;}
 item_dynamics_scalars(p,root+4,16,4);
 uint32_t count=title_u32(p,root+4),params=item_dynamics_pointer(p,root);
 if(count>p->size/60 || (count && params==UINT32_MAX)){p->error=-1;return;}
 if(count)item_dynamics_scalars(p,params,count*60,0);
}
#endif
