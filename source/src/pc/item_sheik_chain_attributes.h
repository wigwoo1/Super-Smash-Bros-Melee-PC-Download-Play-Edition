#ifndef MELEE_PC_ITEM_SHEIK_CHAIN_ATTRIBUTES_H
#define MELEE_PC_ITEM_SHEIK_CHAIN_ATTRIBUTES_H
/* itSeakChain_Attrs: integer, finite floats, two opaque padding spans,
 * and two joint roots. Caller drains the shared graph queue before commit. */
static void MeleePc_ItemSheikChainAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3)||!title_range(p,root,108)){p->error=-1;return;}
 for(unsigned off=0;off<108;off+=4){
  uint32_t at=root+off;
  if(p->external_slots&&p->external_slots[at/4]){p->error=-1;return;}
  if(off>=100){
   uint32_t target=title_u32(p,at);
   if(!p->relocs[at/4]||(target&3)||!title_range(p,target,64)){p->error=-1;return;}
  }else{
   if(p->relocs[at/4]){p->error=-1;return;}
   unsigned padding=(off>=8&&off<16)||(off>=76&&off<84);
   if(off&&!padding&&(title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
  }
 }
 for(unsigned off=0;off<100;off+=4)
  title_claim(p,root+off,4,((off>=8&&off<16)||(off>=76&&off<84))?1:4);
 for(unsigned off=100;off<108;off+=4)
  title_enqueue(p,title_pointer(p,root+off),TITLE_HSD_Joint);
}
#endif
