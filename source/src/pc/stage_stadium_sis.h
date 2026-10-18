#ifndef MELEE_PC_STAGE_STADIUM_SIS_H
#define MELEE_PC_STAGE_STADIUM_SIS_H
/* Stadium SIS table: atlas, byte metrics, twenty encoded streams. Original
 * hsd_3A76.c consumes BE glyph/halfword operands explicitly; preserve them.
 * Jump/call opcodes need separate pointer semantics and remain unsupported. */
static void stadium_sis_bytes(MeleePc_TitlePlan* p,uint32_t at,uint32_t size){
 if(!title_range(p,at,size))return;
 for(uint32_t slot=at&~3U;slot<at+size;slot+=4)
  if(p->relocs[slot/4] || (p->external_slots && p->external_slots[slot/4])){p->error=-2;return;}
 title_claim(p,at,size,1);
}
static void MeleePc_StageSisPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned entries){
 if(entries<3 || entries>4096){p->error=-1;return;}
 if(p->error || !title_range(p,root,entries*4))return;
 for(unsigned i=0;i<entries && !p->error;i++){
  if(title_pointer(p,root+i*4)==UINT32_MAX)p->error=-1;
 }
 if(p->error)return;
 if(title_u32(p,root+8)!=root+entries*4){p->error=-2;return;}
 uint32_t glyphs=0;
 for(unsigned i=2;i<entries && !p->error;i++){
  uint32_t at=title_u32(p,root+i*4);int ended=0;
  for(uint32_t steps=0;steps<p->size && !p->error;steps++){
   if(!title_range(p,at,1))return;
   unsigned opcode=p->bytes[32+at],size=1;
   if(opcode>=32){
    size=2;if(!title_range(p,at,size))return;
    unsigned code=title_u16(p,at);
    if(code>=0x4000 && code-0x4000+1>glyphs)glyphs=code-0x4000+1;
   }else if(opcode==8 || opcode==9 || opcode>26){p->error=-2;return;}
   else if(opcode==5)size=3;
   else if(opcode==6 || opcode==7 || opcode==10 || opcode==14)size=5;
   else if(opcode==12)size=4;
   stadium_sis_bytes(p,at,size);if(p->error)return;
   at+=size;if(!opcode){ended=1;break;}
  }
  if(!ended)p->error=-1;
 }
 if(!p->error && glyphs){
  if(glyphs>p->size/512){p->error=-1;return;}
  stadium_sis_bytes(p,title_u32(p,root),glyphs*512);
  stadium_sis_bytes(p,title_u32(p,root+4),glyphs*2);
 }
}
static void MeleePc_StageStadiumSisPlan(MeleePc_TitlePlan* p,uint32_t root){MeleePc_StageSisPlan(p,root,22);}
static void MeleePc_StageCorneriaSisPlan(MeleePc_TitlePlan* p,uint32_t root){MeleePc_StageSisPlan(p,root,53);}
#endif
