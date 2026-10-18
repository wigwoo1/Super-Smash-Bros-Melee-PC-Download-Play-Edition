#ifndef MELEE_PC_FIGHTER_LINK_ATTRIBUTES_H
#define MELEE_PC_FIGHTER_LINK_ATTRIBUTES_H
/* Source ftLk_DatAttrs/SwordAttrs/AbsorbDesc. Common Link grab callbacks
 * read 94/9C/A0 as signed integer frame timings (original PPC lwz).
 * Sword byte flags/padding and C0 filler retain serialized byte order. */
static void MeleePc_FighterLinkAttributesPlan(MeleePc_TitlePlan* p,uint32_t root){
 static const unsigned floats[]={0,4,8,20,24,28,32,36,40,48,52,56,60,64,68,76,80,84,100,104,124,128,180,200,204,208,212,216};
 if(p->error)return;
 if((root&3)||!title_range(p,root,220)){p->error=-1;return;}
 for(unsigned off=0;off<220;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  for(unsigned i=0;i<sizeof(floats)/sizeof(*floats);i++)if(off==floats[i]&&
     (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 for(unsigned off=0;off<220;off+=4){
  unsigned opaque=(off>=108&&off<120)||off==192;
  title_claim(p,root+off,4,opaque?1:4);
 }
}
#endif
