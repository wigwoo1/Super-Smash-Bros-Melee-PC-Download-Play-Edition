#ifndef MELEE_PC_COSTUME_MARS_PLAN_H
#define MELEE_PC_COSTUME_MARS_PLAN_H
static void MeleePc_CostumeMarsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=586800||root!=128872){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,416432,TITLE_HSD_MatAnimJoint);
}
#endif
