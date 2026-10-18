#ifndef MELEE_PC_COSTUME_PEACH_PLAN_H
#define MELEE_PC_COSTUME_PEACH_PLAN_H
static void MeleePc_CostumePeachPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 uint32_t mat;
 if(p->size==801824 && root==127624)mat=439992;
 else if(p->size==756640 && root==134024)mat=427592;
 else if(p->size==798848 && root==127624)mat=437016;
 else if(p->size==799040 && root==127624)mat=437208;
 else if(p->size==798912 && root==127624)mat=437080;
 else {p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,mat,TITLE_HSD_MatAnimJoint);
}
#endif
