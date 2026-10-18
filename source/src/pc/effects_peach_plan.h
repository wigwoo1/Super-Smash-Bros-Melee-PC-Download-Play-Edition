#ifndef MELEE_PC_EFFECTS_PEACH_PLAN_H
#define MELEE_PC_EFFECTS_PEACH_PLAN_H
/* Exact source-typed one-record Peach archive; null particle banks preserved. */
static void MeleePc_EffectsPeachPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=4936||root!=0||!title_range(p,0,32)){p->error=-2;return;}
 if(title_pointer(p,0)!=UINT32_MAX||title_pointer(p,4)!=UINT32_MAX){p->error=-2;return;}
 if(p->relocs[2]||(p->external_slots&&p->external_slots[2])||(title_u32(p,8)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 title_claim(p,8,4,4);
 uint32_t joint=title_pointer(p,12),anim=title_pointer(p,16);
 if(joint!=4616||anim!=4896||title_pointer(p,20)!=UINT32_MAX||title_pointer(p,24)!=UINT32_MAX){p->error=-2;return;}
 title_enqueue(p,joint,TITLE_HSD_Joint);title_enqueue(p,anim,TITLE_HSD_AnimJoint);
 for(unsigned at=28;at<32;at++)if(p->bytes[32+at]||p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
}
#endif
