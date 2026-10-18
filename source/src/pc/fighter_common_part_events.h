#ifndef MELEE_PC_FIGHTER_COMMON_PART_EVENTS_H
#define MELEE_PC_FIGHTER_COMMON_PART_EVENTS_H
/* 33 fighter entries plus shared/Taro animation entry at index33. Fighter_804D6540_t is pointer/count;
 * each source record has four u8 fields. ftParts forms a 32-bit bitmask. */
static void MeleePc_FighterCommonPartEventsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,34*4)){p->error=-1;return;}
 for(unsigned i=0;i<34 && !p->error;i++){
  uint32_t rec=title_pointer(p,root+i*4);
  if(rec==UINT32_MAX)continue;
  if((rec&3) || !title_range(p,rec,8)){p->error=-1;return;}
  uint32_t count=title_u32(p,rec+4);
  if(count>32 || p->relocs[(rec+4)/4] || (p->external_slots && p->external_slots[(rec+4)/4])){p->error=-1;return;}
  uint32_t payload=title_pointer(p,rec);title_claim(p,rec+4,4,4);
  if(!count)continue;
  if(payload==UINT32_MAX || !title_range(p,payload,count*4)){p->error=-1;return;}
  for(uint32_t at=payload&~3U;at<payload+count*4;at+=4)
   if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
  title_claim(p,payload,count*4,1);
 }
}
#endif
