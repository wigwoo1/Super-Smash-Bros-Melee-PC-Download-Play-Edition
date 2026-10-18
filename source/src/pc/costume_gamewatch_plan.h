#ifndef MELEE_PC_COSTUME_GAMEWATCH_PLAN_H
#define MELEE_PC_COSTUME_GAMEWATCH_PLAN_H
/* Source costume list uses the shared original GameWatch model. */
static void MeleePc_CostumeGameWatchPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(root!=102920||p->size!=108520){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
}
#endif
