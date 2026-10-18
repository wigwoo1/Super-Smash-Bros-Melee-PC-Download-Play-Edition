#ifndef MELEE_PC_COSTUME_KOOPA_PLAN_H
#define MELEE_PC_COSTUME_KOOPA_PLAN_H
static void MeleePc_CostumeKoopaPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=509328||root!=165640){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,485232,TITLE_HSD_MatAnimJoint);
}
#endif
