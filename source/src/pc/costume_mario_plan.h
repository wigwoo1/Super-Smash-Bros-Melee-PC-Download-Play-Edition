#ifndef MELEE_PC_COSTUME_MARIO_PLAN_H
#define MELEE_PC_COSTUME_MARIO_PLAN_H
static void MeleePc_CostumeMarioPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=467728||root!=103424){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,279952,TITLE_HSD_MatAnimJoint);
}
#endif
