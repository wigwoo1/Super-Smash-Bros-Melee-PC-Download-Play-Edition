#ifndef MELEE_PC_FIGHTER_ANIMATION_TABLES_H
#define MELEE_PC_FIGHTER_ANIMATION_TABLES_H
#include "title_archive.h"
/* ftAction_803C0870: opcodes10..58. Packed bits stay BE; only declared
 * call/goto pointer words become native. Runtime packed readers pending. */
static void MeleePc_FighterActionStreamsPlan(MeleePc_TitlePlan* p,const uint32_t* roots,uint32_t count){
 static const unsigned char sizes[]={5,5,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,1,1,7,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,3,2,1,4};
 if(p->error)return;
 size_t capacity=(p->size+3)/4,head=0,tail=0;
 unsigned char* seen=calloc(capacity,1);uint32_t* queue=malloc(capacity*sizeof(*queue));
 if(!seen || !queue || (count && !roots)){p->error=-1;goto finish;}
 for(uint32_t i=0;i<count;i++){
  uint32_t at=roots[i];if(at==UINT32_MAX)continue;
  if((at&3) || !title_range(p,at,4)){p->error=-1;goto finish;}
  if(!seen[at/4]){seen[at/4]=1;queue[tail++]=at;}
 }
 while(head<tail && !p->error){
  uint32_t at=queue[head++];if(seen[at/4]==3){p->error=-1;break;}seen[at/4]=2;
  unsigned op=title_u32(p,at)>>26;
  if(op>58){p->error=-2;break;}
  unsigned n=(op==5 || op==7)?8:op<10?4:sizes[op-10]*4;
  if(!title_range(p,at,n))break;
  for(uint32_t off=4;off<n;off+=4){if(seen[(at+off)/4]){p->error=-1;break;}seen[(at+off)/4]=3;}
  for(uint32_t off=0;off<n && !p->error;off+=4){
   uint32_t slot=at+off;
   if(p->external_slots && p->external_slots[slot/4]){p->error=-1;break;}
   if((op==5 || op==7) && off==4)continue;
   if(p->relocs[slot/4]){p->error=-1;break;}title_claim(p,slot,4,1);
  }
  uint32_t targets[2];unsigned target_count=0;
  if((op==5 || op==7) && !p->error){
   /* Command_07 sets the cursor to its operand; NULL ends the interpreter.
    * Retail Pichu contains this unrelocated zero goto. It is a typed NULL
    * pointer, not an offset-zero branch or an unknown command fallback. */
   if(op==7 && !p->relocs[(at+4)/4] && !title_u32(p,at+4))
    title_pointer(p,at+4);
   else {
    if(!p->relocs[(at+4)/4]){p->error=-1;break;}
    targets[target_count++]=title_pointer(p,at+4);
   }
  }
  if(op!=0 && op!=6 && op!=7)targets[target_count++]=at+n;
  for(unsigned i=0;i<target_count && !p->error;i++){
   uint32_t target=targets[i];
   if(target==UINT32_MAX || (target&3) || !title_range(p,target,4) || seen[target/4]==3){p->error=-1;break;}
   if(!seen[target/4]){if(tail==capacity){p->error=-1;break;}seen[target/4]=1;queue[tail++]=target;}
  }
 }
finish:free(queue);free(seen);
}
/* Caller supplies counts verified from ftData_Table_Unk0/ftData_UnkIntPairs. */
static void MeleePc_FighterAnimationTablesPlan(MeleePc_TitlePlan* p,const uint32_t tables[2],const uint32_t counts[2]){
 if(p->error)return;
 if(!tables || !counts || counts[0]>p->size/24 || counts[1]>p->size/24){p->error=-1;return;}
 uint32_t count=counts[0]+counts[1],used=0;
 uint32_t* roots=malloc((count?count:1)*sizeof(*roots));if(!roots){p->error=-1;return;}
 for(unsigned t=0;t<2 && !p->error;t++){
  uint32_t root=tables[t];if(root&3){p->error=-1;break;}if(!title_range(p,root,counts[t]*24))break;
  for(uint32_t i=0;i<counts[t] && !p->error;i++){
   uint32_t at=root+i*24;
   if(p->external_slots && (p->external_slots[at/4] || p->external_slots[(at+12)/4])){p->error=-1;break;}
   for(unsigned off=4;off<24;off+=4)if(off!=12 && (p->relocs[(at+off)/4] || (p->external_slots && p->external_slots[(at+off)/4])))p->error=-1;
   if(p->error)break;
   if(title_u32(p,at+8)>(t?0xb000U:0x8000U) || title_u32(p,at+20)){p->error=-1;break;}
   uint32_t string=title_pointer(p,at);roots[used++]=title_pointer(p,at+12);
   title_claim(p,at+4,8,4);title_claim(p,at+16,8,4);
   if(string!=UINT32_MAX && !p->error){uint32_t end=string;while(end<p->size && p->bytes[32+end])++end;
    if(end==p->size){p->error=-1;break;}title_claim(p,string,end-string+1,1);}
  }
 }
 if(!p->error)MeleePc_FighterActionStreamsPlan(p,roots,used);
 free(roots);
}
#endif
