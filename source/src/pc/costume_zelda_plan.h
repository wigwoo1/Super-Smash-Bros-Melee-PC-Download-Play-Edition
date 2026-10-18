#ifndef MELEE_PC_COSTUME_ZELDA_PLAN_H
#define MELEE_PC_COSTUME_ZELDA_PLAN_H
/* Source-selected Zelda costumes; complete graph conversion before commit. */
static void MeleePc_CostumeZeldaPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 uint32_t material=0;
 if(p->size==338336 && root==108784)material=271384;
 if(p->size==342176 && root==108784)material=275224;
 if(p->size==343040 && root==108784)material=276088;
 if(!material){p->error=-2;return;}
 title_enqueue(p,root,TITLE_HSD_Joint);
 title_enqueue(p,material,TITLE_HSD_MatAnimJoint);
}
#endif
