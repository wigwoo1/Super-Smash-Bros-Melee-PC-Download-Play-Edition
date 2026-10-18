#ifndef MELEE_PC_ITEM_COMMAND_PLAN_H
#define MELEE_PC_ITEM_COMMAND_PLAN_H
/* Packed command words remain BE; only actual relocated branch pointer words
 * become native. Runtime handlers must use explicit packed readers. */
static void MeleePc_ItemCommandPlan(MeleePc_TitlePlan* p,const uint32_t* roots,uint32_t count){
 if(p->error)return;
 size_t capacity=(p->size+3)/4,head=0,tail=0;
 unsigned char* seen=calloc(capacity,1);uint32_t* queue=malloc(capacity*sizeof(*queue));
 if(!seen || !queue){p->error=-1;goto finish;}
 for(uint32_t i=0;i<count;i++){
  uint32_t at=roots[i];if(at==UINT32_MAX)continue;
  if((at&3) || !title_range(p,at,4)){p->error=-1;goto finish;}
  if(!seen[at/4]){seen[at/4]=1;queue[tail++]=at;}
 }
 while(head<tail && !p->error){
  uint32_t at=queue[head++];if(seen[at/4]==3){p->error=-1;break;}seen[at/4]=2;
  unsigned op=title_u32(p,at)>>26,n=op==11?24:op==10?20:(op==5 || op==7)?8:4;
  if(op>25){p->error=-2;break;}
  if(op==16){unsigned sub=(title_u32(p,at)>>18)&255;
   if(sub!=0 && sub!=1 && sub!=2 && sub!=10 && sub!=11){p->error=-2;break;}n=12;}
  if(!title_range(p,at,n))break;
  for(uint32_t off=4;off<n;off+=4){
   if(seen[(at+off)/4]){p->error=-1;break;}seen[(at+off)/4]=3;
  }
  for(uint32_t off=0;off<n && !p->error;off+=4){
   uint32_t slot=at+off;
   if(p->external_slots && p->external_slots[slot/4]){p->error=-2;break;}
   if((op==5 || op==7) && off==4)continue;
   if(p->relocs[slot/4]){p->error=-1;break;}title_claim(p,slot,4,1);
  }
  uint32_t targets[2];unsigned target_count=0;
  if((op==5 || op==7) && !p->error){
   if(!p->relocs[(at+4)/4]){p->error=-1;break;}
   targets[target_count++]=title_pointer(p,at+4);
  }
  if(op!=0 && op!=6 && op!=7)targets[target_count++]=at+n;
  for(unsigned i=0;i<target_count && !p->error;i++){
   uint32_t target=targets[i];
   if(target==UINT32_MAX || (target&3) || !title_range(p,target,4) || seen[target/4]==3){p->error=-1;break;}
   if(!seen[target/4]){if(tail==capacity){p->error=-1;break;}seen[target/4]=1;queue[tail++]=target;}
  }
 }
finish:
 free(queue);free(seen);
}
#endif
