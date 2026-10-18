#ifndef MELEE_PC_STAGE_PARAMETERS_H
#define MELEE_PC_STAGE_PARAMETERS_H
/* GroundParam/StageParam from melee/gr/types.h. GXColor and padding bytes
 * stay byte ordered. The enclosing archive commits only after all plans pass. */
static void MeleePc_StageParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || !title_range(p,root,0xdc))return;
 title_claim(p,root,4,4);title_claim(p,root+4,2,2);title_claim(p,root+6,2,1);
 title_claim(p,root+8,4,2);title_claim(p,root+12,32,4);
 title_claim(p,root+0x2c,2,1);title_claim(p,root+0x2e,2,2);
 title_claim(p,root+0x30,56,4);title_claim(p,root+0x68,72,2);
 uint32_t rows=title_pointer(p,root+0xb0),count=title_u32(p,root+0xb4);
 title_claim(p,root+0xb4,4,4);title_claim(p,root+0xb8,36,1);
 static const unsigned char floats[]={0,0x18,0x1c,0x20,0x24,0x28,0x3c,0x40,0x44,0x48,0x50,0x54,0x58,0x5c,0x60,0x64};
 for(unsigned i=0;i<sizeof(floats) && !p->error;i++)
  if((title_u32(p,root+floats[i])&0x7f800000U)==0x7f800000U)p->error=-1;
 if(count>p->size/100 || (count && rows==UINT32_MAX)){p->error=-1;return;}
 if(count && !title_range(p,rows,count*100))return;
 for(uint32_t i=0;i<count && !p->error;i++){
  title_claim(p,rows+i*100,20,4);title_claim(p,rows+i*100+20,80,2);
 }
}
#endif
