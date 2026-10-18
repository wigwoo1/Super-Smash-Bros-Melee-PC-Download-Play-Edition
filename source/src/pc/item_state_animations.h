#ifndef MELEE_PC_ITEM_STATE_ANIMATIONS_H
#define MELEE_PC_ITEM_STATE_ANIMATIONS_H
/* Caller supplies source-derived extent. Script pointer is claimed, but its
 * command payload requires a separate validated item-command planner. */
static void MeleePc_ItemStateAnimationsPlan(MeleePc_TitlePlan* p,uint32_t root,uint32_t count){
 if(p->error || (root&3) || count>p->size/16 || !title_range(p,root,count*16)){p->error=-1;return;}
 const unsigned types[]={TITLE_HSD_AnimJoint,TITLE_HSD_MatAnimJoint,TITLE_HSD_ShapeAnimJoint};
 for(uint32_t i=0;i<count && !p->error;i++){
  uint32_t at=root+i*16;
  for(unsigned field=0;field<3;field++)title_enqueue(p,title_pointer(p,at+field*4),types[field]);
  title_pointer(p,at+12);
 }
}
#endif
