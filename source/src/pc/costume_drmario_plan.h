#ifndef MELEE_PC_COSTUME_DRMARIO_PLAN_H
#define MELEE_PC_COSTUME_DRMARIO_PLAN_H
static void MeleePc_CostumeDrMarioPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=476816||root!=103144){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,288736,TITLE_HSD_MatAnimJoint);
}
#endif
