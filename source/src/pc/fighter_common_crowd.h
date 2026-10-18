#ifndef MELEE_PC_FIGHTER_COMMON_CROWD_H
#define MELEE_PC_FIGHTER_COMMON_CROWD_H
/* CrowdConfig: 12 f32 and five s32 fields; no pointers. */
static void MeleePc_FighterCommonCrowdPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,68)){p->error=-1;return;}
 for(uint32_t off=0;off<68;off+=4){
  uint32_t at=root+off;unsigned integer=(off>=28 && off<=40) || off==60;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4]) ||
     (!integer && (title_u32(p,at)&0x7f800000U)==0x7f800000U)){p->error=-1;return;}
 }
 title_claim(p,root,68,4);
}
#endif
