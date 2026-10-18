#ifndef MELEE_PC_COSTUME_SAMUS_PLAN_H
#define MELEE_PC_COSTUME_SAMUS_PLAN_H
static void MeleePc_CostumeSamusPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(root!=164488||(p->size!=395392&&p->size!=401536)){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
}
#endif
