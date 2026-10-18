#ifndef MELEE_PC_ITEM_COLOR_COMMANDS_H
#define MELEE_PC_ITEM_COLOR_COMMANDS_H
/* Caller supplies a separately validated descriptor count. CommandInfo
 * words become host scalars; GXColor payloads retain their byte order.
 * Grammar follows lbcommand.c and lb_013B.c. This plans bytes only. */
static void MeleePc_ColorStreamsPlan(MeleePc_TitlePlan* p,const uint32_t* roots,uint32_t count){
 if(p->error)return;
 if((count && !roots) || count>p->size/4){p->error=-1;return;}
 size_t capacity=(size_t)p->size/4+count+1,head=0,tail=0;
 uint32_t* queue=malloc(capacity*sizeof(*queue));
 unsigned char* marks=calloc((p->size+3)/4,1);
 if(!queue || !marks){p->error=-1;goto done;}
 for(uint32_t i=0;i<count;i++)if(roots[i]!=UINT32_MAX)queue[tail++]=roots[i];
 while(head<tail && !p->error){
  uint32_t at=queue[head++];
  while(!p->error){
   if((at&3) || !title_range(p,at,4)){p->error=-1;break;}
   if(marks[at/4]){if(marks[at/4]!=1)p->error=-1;break;}
   unsigned opcode=title_u32(p,at)>>26,size=4,rgba=0;
   if(opcode>20){p->error=-2;break;}
   if(opcode==5 || opcode==7)size=8;
   if(opcode==13 || opcode==14 || opcode==18 || opcode==19){size=8;rgba=4;}
   if(opcode==15){size=8;rgba=4;}
   if(!title_range(p,at,size))break;
   for(unsigned off=0;off<size;off+=4){
    uint32_t slot=at+off;
    if(marks[slot/4] || p->claims[slot] ||
       (p->external_slots && p->external_slots[slot/4]) ||
       (p->relocs[slot/4] && !((opcode==5 || opcode==7) && off==4))){p->error=-1;break;}
   }
   if(p->error)break;
   for(unsigned off=0;off<size;off+=4){
    marks[(at+off)/4]=off?2:1;
    title_claim(p,at+off,4,rgba && off==rgba?1:4);
   }
   if(opcode==5 || opcode==7){
    if(!p->relocs[(at+4)/4]){p->error=-1;break;}
    uint32_t target=title_pointer(p,at+4);
    if(p->error || target==UINT32_MAX || tail==capacity){p->error=-1;break;}
    queue[tail++]=target;
   }
   if(opcode==0 || opcode==6 || opcode==7 || opcode==10)break;
   at+=size;
  }
 }
done:
 free(marks);free(queue);
}
static void MeleePc_ItemColorCommandsPlan(MeleePc_TitlePlan* p,uint32_t root,uint32_t count){
 if(p->error)return;
 if((root&3) || count>p->size/8 || !title_range(p,root,count*8)){p->error=-1;return;}
 uint32_t* roots=malloc((count?count:1)*sizeof(*roots));
 if(!roots){p->error=-1;return;}
 for(uint32_t i=0;i<count && !p->error;i++){
  uint32_t slot=root+i*8;
  if(p->relocs[(slot+4)/4] || (p->external_slots &&
     (p->external_slots[slot/4] || p->external_slots[(slot+4)/4]))){p->error=-1;break;}
  roots[i]=title_pointer(p,slot);title_claim(p,slot+4,4,1);
 }
 MeleePc_ColorStreamsPlan(p,roots,count);free(roots);
}
#endif

