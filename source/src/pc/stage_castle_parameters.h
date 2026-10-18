#ifndef MELEE_PC_STAGE_CASTLE_PARAMETERS_H
#define MELEE_PC_STAGE_CASTLE_PARAMETERS_H
/* grCastle_YakumonoParam: mixed s16/f32 records and opaque padding.
 * x114 is a grMaterial command stream, not an animation descriptor. This
 * helper claims its pointer only; its packed grammar needs separate planning. */
static void MeleePc_StageCastleParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,324)){p->error=-1;return;}
 for(uint32_t off=0;off<324;off+=4)if(off!=276 &&
    (p->relocs[(root+off)/4] || (p->external_slots && p->external_slots[(root+off)/4]))){p->error=-1;return;}
 title_claim(p,root,16,2);
 title_claim(p,root+28,4,1);title_claim(p,root+70,2,1);
 title_claim(p,root+86,2,1);title_claim(p,root+90,2,1);title_claim(p,root+296,4,1);
 const uint32_t floats[]={16,20,24,32,36,40,44,48,52,56,60,72,76,80,272,280,284,288,292,308,312,316,320};
 for(unsigned i=0;i<sizeof(floats)/sizeof(floats[0]);i++){
  uint32_t at=root+floats[i];
  if((title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
  title_claim(p,at,4,4);
 }
 title_claim(p,root+64,6,2);title_claim(p,root+84,2,2);title_claim(p,root+88,2,2);
 for(unsigned i=0;i<9;i++){
  uint32_t at=root+92+i*20;title_claim(p,at,2,2);title_claim(p,at+2,2,1);
  for(unsigned j=4;j<20;j+=4){
   if((title_u32(p,at+j)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
   title_claim(p,at+j,4,4);
  }
 }
 title_claim(p,root+300,8,2);
 uint32_t script=title_pointer(p,root+276);
 if(script!=UINT32_MAX && ((script&3)||!title_range(p,script,4))){p->error=-1;return;}
}
#endif
