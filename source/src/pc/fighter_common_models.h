#ifndef MELEE_PC_FIGHTER_COMMON_MODELS_H
#define MELEE_PC_FIGHTER_COMMON_MODELS_H
/* ftLoadCommonData: x20 holds the source rebirth Joint/AnimJoint pair.
 * x40 and x50 are direct HSD_Joint roots. Other roots are planned separately. */
static void MeleePc_FighterCommonModelsPlan(MeleePc_TitlePlan* p,const uint32_t* roots){
 if(p->error)return;
 if(!roots){p->error=-1;return;}
 uint32_t pair=roots[8];
 if(pair==UINT32_MAX || (pair&3) || !title_range(p,pair,8)){p->error=-1;return;}
 title_enqueue(p,title_pointer(p,pair),TITLE_HSD_Joint);
 title_enqueue(p,title_pointer(p,pair+4),TITLE_HSD_AnimJoint);
 title_enqueue(p,roots[16],TITLE_HSD_Joint);
 title_enqueue(p,roots[20],TITLE_HSD_Joint);
 while(p->head<p->count && !p->error){MeleePc_TitleNode node=p->nodes[p->head++];title_walk(p,node);}
}
#endif
