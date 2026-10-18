#ifndef MELEE_PC_COSTUME_KIRBY_PLAN_H
#define MELEE_PC_COSTUME_KIRBY_PLAN_H
static void MeleePc_CostumeKirbyPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=272944||root!=125448){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,204688,TITLE_HSD_MatAnimJoint);
}
#endif
