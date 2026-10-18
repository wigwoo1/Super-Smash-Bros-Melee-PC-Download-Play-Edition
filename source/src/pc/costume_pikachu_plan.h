#ifndef MELEE_PC_COSTUME_PIKACHU_PLAN_H
#define MELEE_PC_COSTUME_PIKACHU_PLAN_H
static void MeleePc_CostumePikachuPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 uint32_t material;
 if(p->size==130944&&root==76680)material=98280;
 else if(p->size==162048&&root==81064)material=129384;
 else if(p->size==153248&&root==82536)material=120584;
 else if(p->size==155040&&root==84328)material=122376; else {p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,material,TITLE_HSD_MatAnimJoint);
}
#endif
