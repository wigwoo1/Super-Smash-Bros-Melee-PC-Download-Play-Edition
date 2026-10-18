#ifndef MELEE_PC_STAGE_COLLISION_H
#define MELEE_PC_STAGE_COLLISION_H
/* MapCollData/MapLine/MapJoint from melee/mp/types.h. Plan only: no mutation
 * until the enclosing stage archive has validated every required record. */
static void stage_collision_ranges(MeleePc_TitlePlan* p,uint32_t at,uint32_t lines){
 if(!title_claim(p,at,20,2))return;
 for(unsigned i=0;i<5 && !p->error;i++){
  int start=(int16_t)title_u16(p,at+i*4),count=(int16_t)title_u16(p,at+i*4+2);
  if(count<0 || (count && (start<0 || (uint32_t)start>lines || (uint32_t)count>lines-(uint32_t)start)))p->error=-1;
 }
}
static void MeleePc_StageCollisionPlan(MeleePc_TitlePlan* p,uint32_t root){
 /* The serialized record ends at joint_count (+0x28). The inferred
  * runtime x2C field is unused by mpLibLoad and may be the next allocation
  * on disc, as in GrZe.dat; never claim or swap that neighboring word. */
 if(p->error || !title_range(p,root,44))return;
 uint32_t verts=title_pointer(p,root),nv=title_u32(p,root+4);
 uint32_t lines=title_pointer(p,root+8),nl=title_u32(p,root+12);
 uint32_t joints=title_pointer(p,root+36),nj=title_u32(p,root+40);
 title_claim(p,root+4,4,4);title_claim(p,root+12,4,4);title_claim(p,root+40,4,4);
 if(nv>p->size/8 || nl>p->size/16 || nj>p->size/40 ||
    (nv && verts==UINT32_MAX) || (nl && lines==UINT32_MAX) || (nj && joints==UINT32_MAX)){p->error=-1;return;}
 stage_collision_ranges(p,root+16,nl);
 if(nv && !title_claim(p,verts,nv*8,4))return;
 if(nl && !title_claim(p,lines,nl*16,2))return;
 if(nj && !title_range(p,joints,nj*40))return;
 for(uint32_t i=0;i<nv*2 && !p->error;i++)
  if((title_u32(p,verts+i*4)&0x7f800000U)==0x7f800000U)p->error=-1;
 for(uint32_t i=0;i<nl && !p->error;i++){
  uint32_t at=lines+i*16;
  if(title_u16(p,at)>=nv || title_u16(p,at+2)>=nv){p->error=-1;break;}
  for(unsigned j=4;j<12;j+=2){int neighbor=(int16_t)title_u16(p,at+j);
   if(neighbor < -1 || (neighbor>=0 && (uint32_t)neighbor>=nl))p->error=-1;
  }
 }
 for(uint32_t i=0;i<nj && !p->error;i++){
  uint32_t at=joints+i*40;stage_collision_ranges(p,at,nl);
  title_claim(p,at+20,16,4);title_claim(p,at+36,4,2);
  for(unsigned j=20;j<36;j+=4)if((title_u32(p,at+j)&0x7f800000U)==0x7f800000U)p->error=-1;
  int start=(int16_t)title_u16(p,at+36),count=(int16_t)title_u16(p,at+38);
  if(count<0 || (count && (start<0 || (uint32_t)start>nv || (uint32_t)count>nv-(uint32_t)start)))p->error=-1;
 }
}
#endif
