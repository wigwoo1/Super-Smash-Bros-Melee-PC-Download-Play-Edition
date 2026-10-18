#ifndef MELEE_PC_COSTUME_NESS_PLAN_H
#define MELEE_PC_COSTUME_NESS_PLAN_H
static void MeleePc_CostumeNessPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 uint32_t mat;
 if(p->size==367456 && root==110800)mat=333912;
 else if(p->size==369344 && root==112616)mat=335800;
 else if(p->size==369376 && root==112648)mat=335832;
 else if(p->size==369376 && root==112648)mat=335832;
 else {p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,mat,TITLE_HSD_MatAnimJoint);
}
#endif
