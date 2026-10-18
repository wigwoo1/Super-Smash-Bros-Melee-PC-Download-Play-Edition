#ifndef MELEE_PC_COSTUME_LUIGI_PLAN_H
#define MELEE_PC_COSTUME_LUIGI_PLAN_H
static void MeleePc_CostumeLuigiPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(root!=104752){p->error=-2;return;}
 uint32_t mat;
 if(p->size==367584)mat=274680;
 else if(p->size==481552)mat=293776;
 else {p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,mat,TITLE_HSD_MatAnimJoint);
}
#endif
