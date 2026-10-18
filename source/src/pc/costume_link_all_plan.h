#ifndef MELEE_PC_COSTUME_LINK_ALL_PLAN_H
#define MELEE_PC_COSTUME_LINK_ALL_PLAN_H
static void MeleePc_CostumeLinkAllPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=484112||root!=125704){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,313936,TITLE_HSD_MatAnimJoint);
}
#endif
