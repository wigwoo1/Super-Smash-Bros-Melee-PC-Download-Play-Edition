#ifndef MELEE_PC_STAGE_ICEMT_PARAMETERS_H
#define MELEE_PC_STAGE_ICEMT_PARAMETERS_H
/* grIceMt consumers: preserve padding; generator records replace unused float tail.
 * Caller derives descriptor reachability from the map joint-pair registration IDs. */
static uint32_t MeleePc_IcemtShortList(MeleePc_TitlePlan* p,uint32_t at,int fields){
 if(at==UINT32_MAX||(at&1)){p->error=-1;return 0;}
 uint32_t count=0;
 while(!p->error){
  if(!title_range(p,at,2)||p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;break;}
  int value=(int16_t)title_u16(p,at);title_claim(p,at,2,2);at+=2;
  if(value==-1){if(count<(fields?5U:1U))p->error=-1;return count;}
  if(value<0||(fields&&value>=6)){p->error=-1;break;}count++;
 }
 return 0;
}
static void MeleePc_StageIcemtParametersPlan(MeleePc_TitlePlan* p,uint32_t root,const unsigned char* reachable){
 if(p->error)return;
 if((root&3)||!title_range(p,root,188)||!reachable){p->error=-1;return;}
 for(uint32_t off=0;off<188;off+=4)
  if(off!=172&&off!=176&&off!=180&&(p->relocs[(root+off)/4]||(p->external_slots&&p->external_slots[(root+off)/4]))){p->error=-1;return;}
 for(uint32_t off=8;off<=160;off+=4){
  if((off>=52&&off<60)||off==152)continue;
  if((title_u32(p,root+off)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
  title_claim(p,root+off,4,4);
 }
 title_claim(p,root,6,2);title_claim(p,root+52,8,2);
 title_claim(p,root+152,4,2);title_claim(p,root+164,6,2);title_claim(p,root+184,2,2);
 uint32_t lists[3];
 for(unsigned i=0;i<3&&!p->error;i++){
  uint32_t slot=root+172+i*4;
  if(p->external_slots&&p->external_slots[slot/4]){p->error=-1;return;}
  lists[i]=title_pointer(p,slot);
 }
 if(p->error)return;
 MeleePc_IcemtShortList(p,lists[0],1);
 uint32_t a=MeleePc_IcemtShortList(p,lists[1],0),b=MeleePc_IcemtShortList(p,lists[2],0);
 if(a!=b)p->error=-1;
 for(unsigned i=0;i<80&&!p->error;i++)if(reachable[i]){
  uint32_t at=root+188+i*4;
  if(!title_range(p,at,4)||p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  title_claim(p,at,2,2);title_claim(p,at+2,2,1);
 }
}
#endif
