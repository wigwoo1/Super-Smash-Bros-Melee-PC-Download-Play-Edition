#ifndef MELEE_PC_ITEM_WHISPY_ATTRIBUTES_H
#define MELEE_PC_ITEM_WHISPY_ATTRIBUTES_H
/* itWhispyAppleAttributes: source reads x4/x8 integers and x14/x18 floats.
 * Preserve opaque byte arrays x0[4] and xC[8]. */
static void MeleePc_ItemWhispyAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,28)){p->error=-1;return;}
 for(uint32_t off=0;off<28;off+=4){
  if((off && p->relocs[(root+off)/4]) || (p->external_slots && p->external_slots[(root+off)/4])){p->error=-1;return;}
  if(off>=20 && (title_u32(p,root+off)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 /* Retail x0 is a relocation slot despite the opaque source byte array.
  * Convert only pointer metadata; its unused pointee remains opaque. */
 uint32_t opaque=title_pointer(p,root);
 /* Mark only the validated address byte of the source-unused opaque target.
  * No allocation extent or scalar type is inferred, and no target byte swaps. */
 if(opaque!=UINT32_MAX)title_claim(p,opaque,1,1);
 title_claim(p,root+4,8,4);
 title_claim(p,root+12,8,1);title_claim(p,root+20,8,4);
}
#endif
