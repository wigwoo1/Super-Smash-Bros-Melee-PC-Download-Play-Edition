#ifndef MELEE_PC_STAGE_KRAID_PARAMETERS_H
#define MELEE_PC_STAGE_KRAID_PARAMETERS_H
/* Source grKraid_YakumonoParam: 7 mixed words, followed by six floats. */
static void MeleePc_StageKraidParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,52)){p->error=-1;return;}
 for(uint32_t off=0;off<52;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
  if((off==12 || off==16 || off>=28) && (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 /* grKraid_801FE6D8 divides by map_time_acl and map_rot_spd_min. */
 if(title_u32(p,root+8)==0 || (title_u32(p,root+12)&0x7fffffffU)==0){p->error=-1;return;}
 title_claim(p,root,52,4);
}
#endif
