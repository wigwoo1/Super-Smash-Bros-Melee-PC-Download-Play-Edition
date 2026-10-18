#ifndef MELEE_PC_ITEM_COMMON_PARAMETERS_H
#define MELEE_PC_ITEM_COMMON_PARAMETERS_H
/* ItemCommonData (it/types.h), not ItemAttr or per-kind attributes.
 * Preserve byte/padding members at 0x48, 0xE4 and 0xEC. */
static void MeleePc_ItemCommonParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 static const unsigned short floats[]={0x4c,0x54,0x58,0x5c,0x60,0x68,0x6c,
  0x70,0x74,0x78,0x7c,0x80,0x84,0x88,0x8c,0x90,0x94,0x98,0x9c,0xa0,
  0xa4,0xa8,0xac,0xb0,0xb8,0xbc,0xc0,0xc4,0xc8,0xcc,0xd0,0xd4,0xe0,
  0xe8,0xf0,0xf4,0xf8,0x144,0x14c,0x150,0x154,0x158,0x15c};
 if(p->error || (root&3) || !title_range(p,root,0x160)){p->error=-1;return;}
 for(uint32_t at=0;at<0x160;at+=4){
  if(p->relocs[(root+at)/4] || (p->external_slots && p->external_slots[(root+at)/4])){p->error=-1;return;}
 }
 for(unsigned i=0;i<sizeof(floats)/sizeof(*floats);i++)
  if((title_u32(p,root+floats[i])&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 for(uint32_t at=0;at<0x160;at+=4)
  if(!title_claim(p,root+at,4,(at==0x48 || at==0xe4 || at==0xec)?1:4))return;
}
/* it_804D6D40_t: one s32 followed by six f32 values. */
static void MeleePc_ItemSecondaryParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,28)){p->error=-1;return;}
 for(uint32_t at=0;at<28;at+=4){
  if(p->relocs[(root+at)/4] || (p->external_slots && p->external_slots[(root+at)/4]) ||
     (at && (title_u32(p,root+at)&0x7f800000U)==0x7f800000U)){
   p->error=-1;return;
  }
 }
 title_claim(p,root,28,4);
}
#endif
