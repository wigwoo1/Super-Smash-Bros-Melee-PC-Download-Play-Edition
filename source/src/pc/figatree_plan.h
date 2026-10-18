#ifndef MELEE_PC_FIGATREE_PLAN_H
#define MELEE_PC_FIGATREE_PLAN_H
/* Source FigaTree/FigaTrack layouts. Packed FObj samples remain bytes. */
static void MeleePc_FigaTreePlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,20)){p->error=-1;return;}
 for(unsigned i=0;i<3;i++)if(p->relocs[(root+i*4)/4] || (p->external_slots&&p->external_slots[(root+i*4)/4])){p->error=-1;return;}
 uint32_t bits=title_u32(p,root+8);float frames;memcpy(&frames,&bits,4);
 if(!isfinite(frames)||frames<0){p->error=-1;return;}
 title_claim(p,root,12,4);
 for(unsigned i=12;i<=16;i+=4)if(!p->relocs[(root+i)/4]||(p->external_slots&&p->external_slots[(root+i)/4])){p->error=-1;return;}
 uint32_t nodes=title_pointer(p,root+12),tracks=title_pointer(p,root+16),total=0,cursor=nodes;
 if(p->error || nodes>=p->size || tracks>=p->size || (tracks&3)){p->error=-1;return;}
 for(;;){
  if(!title_range(p,cursor,1))return;
  int count=(int8_t)p->bytes[32+cursor];title_claim(p,cursor++,1,1);
  if(count==-1)break;
  if(count<0 || (unsigned)count>p->size/12-total){p->error=-1;return;}
  total+=(unsigned)count;
 }
 if(!total || !title_range(p,tracks,total*12)){p->error=-1;return;}
 for(uint32_t j=0;j<total&&!p->error;j++){
  uint32_t at=tracks+j*12;
  for(unsigned k=0;k<8;k++)if(p->relocs[(at+k)/4] || (p->external_slots&&p->external_slots[(at+k)/4])){p->error=-1;return;}
  uint32_t length=title_u16(p,at);
  title_claim(p,at,4,2);title_claim(p,at+4,4,1);
  if(!p->relocs[(at+8)/4] || (p->external_slots&&p->external_slots[(at+8)/4])){p->error=-1;return;}
  uint32_t payload=title_pointer(p,at+8);
  if(!length || !title_range(p,payload,length)) {p->error=-1;return;}
  title_claim(p,payload,length,1);
 }
}
#endif
