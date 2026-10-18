#ifndef MELEE_PC_FIGHTER_COMMON_ACTION_SCALARS_H
#define MELEE_PC_FIGHTER_COMMON_ACTION_SCALARS_H
/* ftswing.c indexes six item categories by five animation variants.
 * ft_0881.c staling loop consumes nine scalar weights. Item-throw source
 * covers 26 consecutive motion states, each three f32 values. */
static void MeleePc_FighterCommonActionScalarsPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned index){
 uint32_t size=index==1?312:index==2?120:index==3?36:0;
 if(p->error)return;
 if(!size || (root&3) || !title_range(p,root,size)){p->error=-1;return;}
 for(uint32_t off=0;off<size;off+=4){uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4]) ||
     (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,size,4);
}
#endif
