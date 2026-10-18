#ifndef MELEE_PC_COSTUME_KIRBY_COPY_FALCO_PLAN_H
#define MELEE_PC_COSTUME_KIRBY_COPY_FALCO_PLAN_H
static void MeleePc_CostumeKirbyCopyFalcoPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=183792||root!=50740){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,115508,TITLE_HSD_MatAnimJoint);
}
#endif

