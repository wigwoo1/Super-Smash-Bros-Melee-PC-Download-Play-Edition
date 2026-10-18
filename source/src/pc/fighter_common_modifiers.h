#ifndef MELEE_PC_FIGHTER_COMMON_MODIFIERS_H
#define MELEE_PC_FIGHTER_COMMON_MODIFIERS_H
/* Source Fighter_804D6524_t/6520_t/651C_t/6518_t: all f32.
 * kind is the ftLoadCommonData pointer-field index, not a fighter kind. */
static void MeleePc_FighterCommonModifierPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 uint32_t size=kind==12?156:kind==13?60:kind==14?36:kind==15?8:0;
 if(p->error)return;
 if(!size || (root&3) || !title_range(p,root,size)){p->error=-1;return;}
 for(uint32_t off=0;off<size;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4]) ||
     (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 title_claim(p,root,size,4);
}
#endif
