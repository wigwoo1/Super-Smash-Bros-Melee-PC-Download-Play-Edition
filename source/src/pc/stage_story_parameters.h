#ifndef MELEE_PC_STAGE_STORY_PARAMETERS_H
#define MELEE_PC_STAGE_STORY_PARAMETERS_H
/* grStory_YakumonoParam: three floats followed by float vpos[6]. */
static void MeleePc_StageStoryParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,36)){p->error=-1;return;}
 for(uint32_t off=0;off<36;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4]) ||
     (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,36,4);
}
#endif
