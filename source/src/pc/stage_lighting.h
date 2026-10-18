#ifndef MELEE_PC_STAGE_LIGHTING_H
#define MELEE_PC_STAGE_LIGHTING_H
/* Original map-head count is the number of 32-bit table words, confirmed
 * across disc stages. Native ground lookup needs descriptor/flag entry count.
 * Return that count separately: enclosing archive commits it only after all
 * stage plans succeed. Do not mutate source bytes during validation. */
static void MeleePc_StageLightingPlan(MeleePc_TitlePlan* p,uint32_t root,uint32_t* entries){
 if(p->error || !entries || !title_range(p,root,48)){if(!entries)p->error=-1;return;}
 uint32_t table=title_pointer(p,root+24),words=title_u32(p,root+28);
 title_claim(p,root+28,4,4);
 if(p->error || (words&1) || words>p->size/4 || (words && table==UINT32_MAX)){p->error=-1;return;}
 uint32_t count=words/2;
 if(count && !title_range(p,table,count*8))return;
 /* No independently referenced record may begin inside a flag word or
  * the middle of this table. Pointer slots must alternate with byte flags. */
 for(uint32_t slot=0;slot+4<=p->size && !p->error;slot+=4)if(p->relocs[slot/4]){
  uint32_t target=title_u32(p,slot);
  if(count && target>table && target<table+count*8){p->error=-1;return;}
 }
 for(uint32_t i=0;i<count && !p->error;i++){
  uint32_t at=table+i*8;
  if(p->relocs[(at+4)/4]){p->error=-1;return;}
  uint32_t desc=title_pointer(p,at);
  if(desc==UINT32_MAX){
   /* lbArchive_InitializeDAT clears external references to NULL. Ground
    * override lookup only compares descriptor identity; it does not load
    * or dereference these absent imported descriptors. */
   if(p->external_slots && p->external_slots[at/4]){title_claim(p,at+4,4,1);continue;}
   p->error=-1;return;
  }
  if(!title_range(p,desc,title_types[TITLE_HSD_LightDesc].size))return;
  title_claim(p,at+4,4,1);title_enqueue(p,desc,TITLE_HSD_LightDesc);
 }
 if(!p->error)*entries=count;
}
#endif
