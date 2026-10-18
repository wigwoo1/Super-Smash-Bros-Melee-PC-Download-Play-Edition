#ifndef MELEE_PC_FIGHTER_PART_ANIMATIONS_H
#define MELEE_PC_FIGHTER_PART_ANIMATIONS_H
#include "fighter_animation_graphs.h"
/* Counts come from verified per-fighter extents, never runtime capacity5. */
static void MeleePc_FighterPartAnimationsPlan(MeleePc_TitlePlan* p,uint32_t root,
 unsigned count,const uint32_t* animation_counts){
 if(p->error)return;
 if(!count||count>5||!animation_counts||(root&3)||!title_range(p,root,count*4)){p->error=-1;return;}
 size_t capacity=p->size/4;unsigned n=0;
 uint32_t* roots=malloc(capacity*sizeof(*roots));
 if(!roots){p->error=-1;return;}
 for(unsigned i=0;i<count&&!p->error;i++){
  if(p->external_slots&&p->external_slots[(root+i*4)/4]){p->error=-1;break;}
  uint32_t at=title_pointer(p,root+i*4);
  if(at==UINT32_MAX||(at&3)||!title_range(p,at,12)||p->relocs[at/4]||
     (p->external_slots&&p->external_slots[at/4])){p->error=-1;break;}
  unsigned parts=title_u16(p,at+2);title_claim(p,at,4,2);
  if(p->external_slots&&(p->external_slots[(at+4)/4]||p->external_slots[(at+8)/4])){p->error=-1;break;}
  uint32_t bytes=title_pointer(p,at+4),table=title_pointer(p,at+8),animations=animation_counts[i];
  if(!animations||animations>capacity||table==UINT32_MAX||(table&3)||
     !title_range(p,table,animations*4)||!parts||bytes==UINT32_MAX||!title_range(p,bytes,parts)){p->error=-1;break;}
  for(uint32_t slot=bytes&~3U;slot<bytes+parts;slot+=4)
   if(p->relocs[slot/4]||(p->external_slots&&p->external_slots[slot/4]))p->error=-1;
  if(p->error)break;
  title_claim(p,bytes,parts,1);
  for(unsigned j=0;j<animations&&!p->error;j++){
   uint32_t animation=title_pointer(p,table+j*4);
   if(animation!=UINT32_MAX){if(n==capacity){p->error=-1;break;}roots[n++]=animation;}
  }
 }
 if(!p->error)MeleePc_FighterAnimationGraphsPlan(p,roots,n);
 free(roots);
}
#endif
