#ifndef MELEE_PC_COSTUME_EMBLEM_PLAN_H
#define MELEE_PC_COSTUME_EMBLEM_PLAN_H
static void MeleePc_CostumeEmblemPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=651616||root!=130216){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,509368,TITLE_HSD_MatAnimJoint);
}
#endif
