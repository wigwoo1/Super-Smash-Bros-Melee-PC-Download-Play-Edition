#ifndef MELEE_PC_COSTUME_FOX_PLAN_H
#define MELEE_PC_COSTUME_FOX_PLAN_H
static void MeleePc_CostumeFoxPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=355712||root!=116328){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,338440,TITLE_HSD_MatAnimJoint);
}
#endif

