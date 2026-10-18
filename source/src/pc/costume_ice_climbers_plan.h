#ifndef MELEE_PC_COSTUME_ICE_CLIMBERS_PLAN_H
#define MELEE_PC_COSTUME_ICE_CLIMBERS_PLAN_H
/* Exact audited roots; outer dispatcher validates source-selected symbols. */
static void MeleePc_CostumeIceClimbersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size==346848 && root==62248){title_enqueue(p,62248,TITLE_HSD_Joint);title_enqueue(p,247936,TITLE_HSD_MatAnimJoint);return;}
 if(p->size==346944 && root==62344){title_enqueue(p,62344,TITLE_HSD_Joint);title_enqueue(p,248032,TITLE_HSD_MatAnimJoint);return;}
 if(p->size==346976 && root==62376){title_enqueue(p,62376,TITLE_HSD_Joint);title_enqueue(p,248064,TITLE_HSD_MatAnimJoint);return;}
 if(p->size==346912 && root==62312){title_enqueue(p,62312,TITLE_HSD_Joint);title_enqueue(p,248000,TITLE_HSD_MatAnimJoint);return;}
 p->error=-2;
}
#endif
