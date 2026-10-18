#ifndef MELEE_PC_FIGHTER_COMMON_SHAKE_H
#define MELEE_PC_FIGHTER_COMMON_SHAKE_H
/* Fighter_ShakeTable_t: Vec2* and signed count; consumer wraps an 8-bit
 * index at count. Both actual tables contain five source-consumed vectors. */
static void MeleePc_FighterCommonShakePlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,8)){p->error=-1;return;}
 uint32_t target=title_u32(p,root),count=title_u32(p,root+4);
 if(p->relocs[root/4]!=1 || p->relocs[(root+4)/4] ||
    (p->external_slots && (p->external_slots[root/4] || p->external_slots[(root+4)/4])) ||
    !count || count>255 || (target&3) || !title_range(p,target,count*8) ||
    (target<root+8 && root<target+count*8)){p->error=-1;return;}
 for(uint32_t off=0;off<count*8;off+=4){
  uint32_t at=target+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4]) ||
     (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_pointer(p,root);title_claim(p,root+4,4,4);title_claim(p,target,count*8,4);
}
#endif
