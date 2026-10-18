#ifndef MELEE_PC_EVENT_PLAN_H
#define MELEE_PC_EVENT_PLAN_H
/* Source-declared event records. Plan all fields before any commit. Width8
 * is local to the event transaction, preserving whole u64 item masks. */
static void event_scalar(MeleePc_TitlePlan* p,uint32_t at,uint32_t size,unsigned width){
 if(p->error || !title_range(p,at,size))return;
 for(uint32_t slot=at&~3U;slot<at+size;slot+=4)
  if(p->relocs[slot/4] || (p->external_slots&&p->external_slots[slot/4])){p->error=-2;return;}
 title_claim(p,at,size,width);
}
static int event_record(MeleePc_TitlePlan* p,uint32_t at,unsigned size){
 if(at==UINT32_MAX)return 0;
 if((at&3)||!title_range(p,at,size)){p->error=-1;return 0;}return !p->error;
}
static void event_player(MeleePc_TitlePlan* p,uint32_t at){
 if(!event_record(p,at,28))return;
 event_scalar(p,at,12,1);event_scalar(p,at+12,4,2);event_scalar(p,at+16,12,4);
}
static void event_init(MeleePc_TitlePlan* p,uint32_t at){
 if(!event_record(p,at,40)){if(at==UINT32_MAX)p->error=-1;return;}
 event_scalar(p,at,6,1);event_scalar(p,at+6,2,2);event_scalar(p,at+8,4,4);
 event_scalar(p,at+16,8,8);event_scalar(p,at+24,16,4);
}
static void event_bonus(MeleePc_TitlePlan* p,uint32_t at){
 if(!event_record(p,at,24))return;
 event_scalar(p,at,8,1);event_scalar(p,at+8,12,4);event_scalar(p,at+20,4,1);
}
static void event_rounds(MeleePc_TitlePlan* p,uint32_t at){
 if(!event_record(p,at,40))return;
 if(p->bytes[32+at]>7){p->error=-2;return;}
 event_scalar(p,at,1,1);event_scalar(p,at+2,14,2);
 for(unsigned i=0;i<6&&!p->error;i++)event_player(p,title_pointer(p,at+16+i*4));
}
static void event_special(MeleePc_TitlePlan* p,unsigned index,uint32_t at){
 if(at==UINT32_MAX)return;
 if(index==0)event_scalar(p,at,8,4);
 else if(index==4)event_scalar(p,at,4,4);
 else if(index==12)event_scalar(p,at,12,4);
 else if(index==13||index==25||index==46){
  for(unsigned i=0;i<33&&!p->error;i++){
   if(!title_range(p,at+i,1))return;
   unsigned value=p->bytes[32+at+i];if(value>33){p->error=-2;return;}
   event_scalar(p,at+i,1,1);if(value==33)return;
  }p->error=-2;
 }else if(index==36){
  for(unsigned i=0;i<30&&!p->error;i++){
   event_scalar(p,at+i*4,4,4);if(p->error)return;
   if(title_u32(p,at+i*4)==0xbf)return;
  }p->error=-2;
 }else if(index==43){
  if(!event_record(p,at,8))return;
  event_scalar(p,at,4,4);uint32_t player=title_pointer(p,at+4);
  if(player==UINT32_MAX)p->error=-1;else event_player(p,player);
 }else p->error=-2;
}
static void MeleePc_EventPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=9624||root!=9420||!title_range(p,root,51*4)){p->error=-2;return;}
 for(unsigned i=0;i<51&&!p->error;i++){
  uint32_t at=title_pointer(p,root+i*4);
  if(at==UINT32_MAX||!event_record(p,at,44)){p->error=-1;return;}
  if(p->bytes[32+at]>2||(p->bytes[33+at]>>5)>6){p->error=-2;return;}
  event_scalar(p,at,2,1);
  event_special(p,i,title_pointer(p,at+4));
  event_init(p,title_pointer(p,at+8));event_bonus(p,title_pointer(p,at+12));event_rounds(p,title_pointer(p,at+16));
  for(unsigned j=0;j<6&&!p->error;j++)event_player(p,title_pointer(p,at+20+j*4));
 }
}
#endif
