#ifndef MELEE_PC_COSTUME_MEWTWO_PLAN_H
#define MELEE_PC_COSTUME_MEWTWO_PLAN_H
static void MeleePc_CostumeMewtwoPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(root!=96560){p->error=-2;return;}
 uint32_t mat;
 if(p->size==259104)mat=192744;
 else if(p->size==250912)mat=192744;
 else {p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,mat,TITLE_HSD_MatAnimJoint);
}
#endif
