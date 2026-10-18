#ifndef MELEE_PC_COSTUME_KIRBY_COPY_GAMEWATCH_PLAN_H
#define MELEE_PC_COSTUME_KIRBY_COPY_GAMEWATCH_PLAN_H
static void MeleePc_CostumeKirbyCopyGameWatchPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=102816||root!=63924){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,77664,TITLE_HSD_MatAnimJoint);
}
#endif

