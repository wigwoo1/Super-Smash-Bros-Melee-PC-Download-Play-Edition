#ifndef MELEE_PC_FIGHTER_CPU_ATTACKS_H
#define MELEE_PC_FIGHTER_CPU_ATTACKS_H
/* ftcpuattack.c ftCo_AttackEntry:36 bytes; terminal cmd==0 uses one word.
 * Caller proves character prefix and separately converts selected scripts. */
static void MeleePc_FighterCpuAttackListPlan(MeleePc_TitlePlan* p,uint32_t at,uint32_t scripts){
 if(at==UINT32_MAX || p->error)return;
 if(at&3){p->error=-1;return;}
 unsigned count=0;
 for(;;){
  if(!title_range(p,at,4))return;
  uint32_t cmd=title_u32(p,at),size=cmd?36:4;
  if(cmd && (cmd>=scripts || ++count>32)){p->error=-1;return;}
  if(!title_range(p,at,size))return;
  for(unsigned off=0;off<size;off+=4){
   uint32_t slot=at+off;
   if(p->relocs[slot/4] || (p->external_slots && p->external_slots[slot/4]) ||
      (off>=8 && off<=24 && (title_u32(p,slot)&0x7f800000U)==0x7f800000U)){p->error=-1;return;}
  }
  title_claim(p,at,size,4);
  if(p->error || !cmd)return;
  at+=36;
 }
}
static void MeleePc_FighterCpuAttackTablePlan(MeleePc_TitlePlan* p,uint32_t root,uint32_t characters,uint32_t scripts){
 if(p->error)return;
 if((root&3) || characters>p->size/4 || !title_range(p,root,characters*4)){p->error=-1;return;}
 for(uint32_t i=0;i<characters && !p->error;i++)MeleePc_FighterCpuAttackListPlan(p,title_pointer(p,root+i*4),scripts);
}
#endif
