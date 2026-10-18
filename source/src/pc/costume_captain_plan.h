#ifndef MELEE_PC_COSTUME_CAPTAIN_PLAN_H
#define MELEE_PC_COSTUME_CAPTAIN_PLAN_H
static void MeleePc_CostumeCaptainPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(!((p->size==519232&&root==128488)||(p->size==519136&&root==128456)||(p->size==519200&&root==128520)||(p->size==519424&&root==128744)||(p->size==519168&&root==128488)||(p->size==519200&&root==128520)||(p->size==551968&&root==128520))){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
}
#endif
