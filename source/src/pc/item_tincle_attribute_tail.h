#ifndef MELEE_PC_ITEM_TINCLE_ATTRIBUTE_TAIL_H
#define MELEE_PC_ITEM_TINCLE_ATTRIBUTE_TAIL_H
/* Source-consumed tail only. Offset0 and its target remain unclaimed. */
static void MeleePc_ItemTincleAttributeTailPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,88)){p->error=-1;return;}
 for(uint32_t off=4;off<88;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
  unsigned integer=off==4 || off==8 || off==40 || off==44 || off==48 || off==60 || off==64;
  if(off<84 && !integer && (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root+4,80,4);title_claim(p,root+84,4,1);
}
#endif
