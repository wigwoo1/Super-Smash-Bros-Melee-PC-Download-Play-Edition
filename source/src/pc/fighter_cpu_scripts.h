#ifndef MELEE_PC_FIGHTER_CPU_SCRIPTS_H
#define MELEE_PC_FIGHTER_CPU_SCRIPTS_H
/* Caller proves the required pointer prefix. Bytecode follows the actual
 * ftCo_800B4880 copy loop, including its advanced-byte second comparison. */
static void MeleePc_FighterCpuScriptsPlan(MeleePc_TitlePlan* p,uint32_t root,uint32_t count){
 if(p->error)return;
 if((root&3) || count>p->size/4 || !title_range(p,root,count*4)){p->error=-1;return;}
 for(uint32_t i=0;i<count && !p->error;i++){
  uint32_t at=title_pointer(p,root+4*i);
  if(p->error || at==UINT32_MAX)continue;
  uint32_t start=at;
  for(;;){
   if(!title_range(p,at,1))break;
   unsigned value=p->bytes[32+at];
   if(value==127){++at;break;}
   if(value>191){++at;if(!title_range(p,at,1))break;value=p->bytes[32+at];}
   if(value>127){++at;if(!title_range(p,at,1))break;}
   ++at;
  }
  if(p->error)break;
  for(uint32_t slot=start/4;slot<((at+3)/4);slot++){
   if(p->relocs[slot] || (p->external_slots && p->external_slots[slot])){p->error=-1;break;}
  }
  if(!p->error)title_claim(p,start,at-start,1);
 }
}
#endif
