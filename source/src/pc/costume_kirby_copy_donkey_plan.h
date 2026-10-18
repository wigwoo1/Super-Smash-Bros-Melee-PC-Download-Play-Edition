#ifndef MELEE_PC_COSTUME_KIRBY_COPY_DONKEY_PLAN_H
#define MELEE_PC_COSTUME_KIRBY_COPY_DONKEY_PLAN_H
static void MeleePc_CostumeKirbyCopyDonkeyPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=176976||root!=57876){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,108712,TITLE_HSD_MatAnimJoint);
}
#endif
