#ifndef MELEE_PC_COSTUME_FALCO_PLAN_H
#define MELEE_PC_COSTUME_FALCO_PLAN_H
static void MeleePc_CostumeFalcoPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=237728||root!=93864){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,220536,TITLE_HSD_MatAnimJoint);
}
#endif

