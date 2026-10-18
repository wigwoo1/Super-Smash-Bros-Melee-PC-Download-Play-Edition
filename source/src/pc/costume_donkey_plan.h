#ifndef MELEE_PC_COSTUME_DONKEY_PLAN_H
#define MELEE_PC_COSTUME_DONKEY_PLAN_H
static void MeleePc_CostumeDonkeyPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=635984||root!=112360){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,517840,TITLE_HSD_MatAnimJoint);
}
#endif
