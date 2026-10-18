#ifndef MELEE_PC_FIGHTER_COMMON_PARTS_H
#define MELEE_PC_FIGHTER_COMMON_PARTS_H
/* Source FighterPartsTable: two byte-array pointers and u32 parts_num.
 * Preserve source-counted joint map and explicitly consumed reverse prefix.
 * Unproven reverse suffix stays opaque and receives no scalar conversion. */
static void MeleePc_FighterCommonPartsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,34*4)){p->error=-1;return;}
 for(unsigned i=0;i<34 && !p->error;i++){
  uint32_t rec=title_pointer(p,root+i*4);
  if(rec==UINT32_MAX)continue;
  if((rec&3) || !title_range(p,rec,12)){p->error=-1;return;}
  uint32_t count=title_u32(p,rec+8);
  if(!count || count>140 || p->relocs[(rec+8)/4] || (p->external_slots && p->external_slots[(rec+8)/4])){p->error=-1;return;}
  uint32_t joint=title_pointer(p,rec),part=title_pointer(p,rec+4);title_claim(p,rec+8,4,4);
  if(joint==UINT32_MAX || part==UINT32_MAX || !title_range(p,joint,count) || !title_range(p,part,54)){p->error=-1;return;}
  for(unsigned k=0;k<2;k++){
   uint32_t start=k?part:joint,size=k?54:count;
   for(uint32_t at=start&~3U;at<start+size;at+=4)
    if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
   title_claim(p,start,size,1);
  }
 }
}
#endif
