#ifndef MELEE_PC_FIGHTER_ANIMATION_GRAPHS_H
#define MELEE_PC_FIGHTER_ANIMATION_GRAPHS_H
#include "title_archive.h"
/* Topological check permits shared DAG nodes, rejects child/next cycles.
 * Uses scratch storage and original offsets, before any byte conversion. */
static void fighter_animation_acyclic(MeleePc_TitlePlan* p){
 size_t capacity=(p->size+3)/4,head=0,tail=0,nodes=0,visited=0;
 uint32_t* incoming=calloc(capacity,sizeof(*incoming));
 uint32_t* queue=malloc(capacity*sizeof(*queue));
 if(!incoming||!queue){p->error=-1;goto done;}
 for(uint32_t i=0;i<capacity;i++)if(p->seen[i]){
  nodes++;const MeleePc_TitleType* layout=title_types+p->seen[i];
  for(unsigned j=0;j<layout->pointer_count;j++){
   const MeleePc_TitlePointer* field=layout->pointers+j;
   if(field->target<=0)continue;
   uint32_t slot=i*4+field->offset;
   if(!p->relocs[slot/4]||(p->external_slots&&p->external_slots[slot/4]))continue;
   uint32_t target=title_u32(p,slot);
   if((target&3)||target>=p->size||p->seen[target/4]!=field->target){p->error=-1;goto done;}
   incoming[target/4]++;
  }
 }
 for(uint32_t i=0;i<capacity;i++)if(p->seen[i]&&!incoming[i])queue[tail++]=i;
 while(head<tail){
  uint32_t i=queue[head++];visited++;
  const MeleePc_TitleType* layout=title_types+p->seen[i];
  for(unsigned j=0;j<layout->pointer_count;j++){
   const MeleePc_TitlePointer* field=layout->pointers+j;
   if(field->target<=0)continue;
   uint32_t slot=i*4+field->offset;
   if(!p->relocs[slot/4]||(p->external_slots&&p->external_slots[slot/4]))continue;
   uint32_t target=title_u32(p,slot)/4;
   if(--incoming[target]==0)queue[tail++]=target;
  }
 }
 if(visited!=nodes)p->error=-1;
done:
 free(incoming);free(queue);
}
/* Verified part-animation graphs: AnimJoint/AObj/FObj and packed streams.
 * Other animation descriptor kinds require their own source validation. */
static void MeleePc_FighterAnimationGraphsPlan(MeleePc_TitlePlan* p,
                                              const uint32_t* roots,unsigned count){
 if(p->error)return;
 if(count&&!roots){p->error=-1;return;}
 for(unsigned i=0;i<count;i++)title_enqueue(p,roots[i],TITLE_HSD_AnimJoint);
 while(p->head<p->count&&!p->error){
  MeleePc_TitleNode node=p->nodes[p->head++];unsigned type=node.type;
  if(type!=TITLE_HSD_AnimJoint&&type!=TITLE_HSD_AObjDesc&&type!=TITLE__HSD_FObjDesc){p->error=-2;return;}
  const MeleePc_TitleType* layout=title_types+type;
  for(unsigned i=0;i<layout->scalar_count;i++){
   unsigned off=layout->scalars[i].offset,pointer=0;
   for(unsigned j=0;j<layout->pointer_count;j++)if(layout->pointers[j].offset==off)pointer=1;
   if(!pointer&&(p->relocs[(node.at+off)/4]||
      (p->external_slots&&p->external_slots[(node.at+off)/4]))){p->error=-1;return;}
  }
  if(type!=TITLE_HSD_AnimJoint){
   uint32_t bits=title_u32(p,node.at+(type==TITLE_HSD_AObjDesc?4:8));
   if((bits&0x7f800000U)==0x7f800000U){p->error=-1;return;}
  }
  title_walk(p,node);
 }
 if(!p->error)fighter_animation_acyclic(p);
}
#endif
