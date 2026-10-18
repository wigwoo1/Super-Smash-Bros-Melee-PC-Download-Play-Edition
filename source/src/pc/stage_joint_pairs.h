#ifndef MELEE_PC_STAGE_JOINT_PAIRS_H
#define MELEE_PC_STAGE_JOINT_PAIRS_H
/* map_head unk0: Ground_801C34AC_entry in ground.c. The pairs are s16
 * model-joint index / stage_info.x280 index. Preserve source index semantics. */
static void MeleePc_StageJointPairsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || !title_range(p,root,48))return;
 uint32_t table=title_pointer(p,root),count=title_u32(p,root+4);
 title_claim(p,root+4,4,4);
 if(count>p->size/12 || (count && table==UINT32_MAX)){p->error=-1;return;}
 if(count && !title_range(p,table,count*12))return;
 for(uint32_t i=0;i<count && !p->error;i++){
  uint32_t at=table+i*12,joint=title_pointer(p,at),pairs=title_pointer(p,at+4),n=title_u32(p,at+8);
  title_claim(p,at+8,4,4);
  if(joint==UINT32_MAX || n>p->size/4 || (n && pairs==UINT32_MAX)){p->error=-1;return;}
  title_enqueue(p,joint,TITLE_HSD_Joint);
  if(n && !title_claim(p,pairs,n*4,2))return;
  for(uint32_t j=0;j<n && !p->error;j++){
   int model_index=(int16_t)title_u16(p,pairs+j*4),stage_index=(int16_t)title_u16(p,pairs+j*4+2);
   if(model_index<0 || stage_index<0 || stage_index>=261)p->error=-1;
  }
 }
}
#endif
