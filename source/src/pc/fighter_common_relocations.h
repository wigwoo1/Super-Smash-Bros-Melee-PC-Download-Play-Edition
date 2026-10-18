#ifndef MELEE_PC_FIGHTER_COMMON_RELOCATIONS_H
#define MELEE_PC_FIGHTER_COMMON_RELOCATIONS_H
/* Finalize declared disconnected pointer words without assigning a type to
 * opaque target scalars. x44 root pointer is only assigned by source loader;
 * no payload consumer found. Its target remains unclaimed and unchanged. */
static int MeleePc_FighterCommonRelocationsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || p->head!=p->count)return p->error?p->error:-2;
 if(!title_range(p,root,92))return p->error;
 uint32_t unused_slot=root+68;
 for(uint32_t i=0;i<(p->size+3)/4;i++)if(p->relocs[i]){
  uint32_t slot=i*4;if(!title_range(p,slot,4))return p->error;
  uint32_t target=title_u32(p,slot);if(target>=p->size)return -1;
  if(p->relocs[i]==1){
   if(target&3)return -2;
   for(unsigned j=0;j<4;j++)if(p->claims[slot+j])return -2;
   for(size_t j=0;j<p->count;j++){
    uint32_t start=p->nodes[j].at,size=title_types[p->nodes[j].type].size;
    if(slot<start+size && start<slot+4)return -2;
   }
  }else if(p->relocs[i]==2){
   unsigned deferred=!(target&3) && p->relocs[target/4]==1;
   if(!p->claims[target] && !deferred && slot!=unused_slot)return -2;
  }else return -2;
  for(uint32_t j=0;j<(p->size+3)/4;j++)if(p->relocs[j]==1 && target>j*4 && target-j*4<4)return -2;
 }
 for(uint32_t i=0;i<(p->size+3)/4;i++)if(p->relocs[i]==1){title_claim(p,i*4,4,4);p->relocs[i]=2;}
 return p->error;
}
#endif
