#ifndef MELEE_PC_ITEM_HURTBONES_H
#define MELEE_PC_ITEM_HURTBONES_H
/* ItHurtBoneList/ItHurtBoneDesc from it/types.h: count/pointer followed
 * by bone ID, two Vec3 endpoints and scale per descriptor. */
static void MeleePc_ItemHurtbonesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,8)){p->error=-1;return;}
 if(p->relocs[root/4] || (p->external_slots &&
    (p->external_slots[root/4] || p->external_slots[(root+4)/4]))){p->error=-1;return;}
 uint32_t count=title_u32(p,root),descs=title_pointer(p,root+4);
 title_claim(p,root,4,4);
 if(p->error || count>p->size/32 || (count && descs==UINT32_MAX)){
  p->error=-1;return;
 }
 if(!count)return;
 if((descs&3) || !title_range(p,descs,count*32)){p->error=-1;return;}
 if(descs<root+8 && root<descs+count*32){p->error=-1;return;}
 for(uint32_t i=0;i<count*32;i+=4){
  uint32_t at=descs+i;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4]) ||
     ((i%32) && (title_u32(p,at)&0x7f800000U)==0x7f800000U)){
   p->error=-1;return;
  }
 }
 title_claim(p,descs,count*32,4);
}
#endif
