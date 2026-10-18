#ifndef MELEE_PC_FIGHTER_DISCONNECTED_POINTERS_H
#define MELEE_PC_FIGHTER_DISCONNECTED_POINTERS_H
#include "title_archive.h"
/* Only after exhaustive source-typed root traversal. Unreachable allocations
 * retain opaque scalar/payload bytes; DAT itself identifies pointer words.
 * No guessed allocation types and no mutation until every refusal check passes. */
static int MeleePc_FighterDisconnectedPointersPlan(MeleePc_TitlePlan* p){
 if(p->error||p->head!=p->count)return p->error?p->error:-2;
 unsigned char* rooted=calloc(p->size,1);
 if(!rooted)return -1;
 int error=0;
 for(uint32_t i=0;i<(p->size+3)/4&&!error;i++)if(p->seen[i]){
  unsigned type=p->seen[i];
  if(type>=sizeof(title_types)/sizeof(title_types[0])||!title_range(p,i*4,title_types[type].size)){error=-1;break;}
  memset(rooted+i*4,1,title_types[type].size);
 }
 for(uint32_t i=0;i<(p->size+3)/4&&!error;i++)if(p->relocs[i]){
  uint32_t slot=i*4;
  if(!title_range(p,slot,4)){error=-1;break;}
  uint32_t target=title_u32(p,slot);
  if(target>=p->size){error=-1;break;}
  if(p->relocs[i]==1){
   if(target&3){error=-2;break;}
   for(unsigned j=0;j<4;j++)if(p->claims[slot+j]||rooted[slot+j])error=-2;
  }else if(!p->claims[target])error=-2;
  for(unsigned j=1;j<4&&!error;j++)if(target>=j&&p->relocs[(target-j)/4]==1&&((target-j)&3)==0)error=-2;
 }
 free(rooted);
 if(error||p->error)return error?error:p->error;
 for(uint32_t i=0;i<(p->size+3)/4;i++)if(p->relocs[i]==1){
  title_claim(p,i*4,4,4);p->relocs[i]=2;
 }
 return p->error;
}
#endif
