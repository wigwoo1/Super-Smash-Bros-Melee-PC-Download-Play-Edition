#ifndef MELEE_PC_FIGHTER_COMMON_PLAYER_COLORS_H
#define MELEE_PC_FIGHTER_COMMON_PLAYER_COLORS_H
/* Player_GetUnk45 CPU branch returns4; consumers read RGB[0..2] at index*4.
 * Claim minimum19 bytes, not presumed20byte allocation/padding. Human index
 * validation remains runtime work. GX bytes keep original byte order. */
static void MeleePc_FighterCommonPlayerColorPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(root==UINT32_MAX || !title_range(p,root,19)){p->error=-1;return;}
 for(uint32_t slot=root/4;slot<(root+22)/4;slot++){
  if(p->relocs[slot] || (p->external_slots && p->external_slots[slot])){p->error=-1;return;}
 }
 title_claim(p,root,19,1);
}
#endif
