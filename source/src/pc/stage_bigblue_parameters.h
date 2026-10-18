#ifndef MELEE_PC_STAGE_BIGBLUE_PARAMETERS_H
#define MELEE_PC_STAGE_BIGBLUE_PARAMETERS_H
/* Source grBb_YakumonoParam; opaque pad64 and pad114 remain byte-preserved. */
static void MeleePc_StageBigblueParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,324)){p->error=-1;return;}
 for(uint32_t off=0;off<324;off+=4){
  uint32_t at=root+off;
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
 }
 static const uint32_t floats[]={0,4,8,12,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,104,108,112,116,120,124,128,132,144,148,152,156,160,164,168,172,188,192,196,200,204,208,212,216,228,232,236,240,244,248,252,256,260,264,292,296,300,304,308,312,316,320};
 for(unsigned i=0;i<sizeof(floats)/sizeof(floats[0]);i++)
  if((title_u32(p,root+floats[i])&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 /* Original divisions: x28, xE4, xEC and the integer expression x18+1. */
 if((title_u32(p,root+0x28)&0x7fffffffU)==0 ||
    (title_u32(p,root+0xE4)&0x7fffffffU)==0 ||
    (title_u32(p,root+0xEC)&0x7fffffffU)==0 ||
    title_u32(p,root+0x18)==0xffffffffU){p->error=-1;return;}
 title_claim(p,root,0x64,4);title_claim(p,root+0x64,4,1);
 title_claim(p,root+0x68,0x114-0x68,4);title_claim(p,root+0x114,8,1);
 title_claim(p,root+0x11C,324-0x11C,4);
}
#endif
