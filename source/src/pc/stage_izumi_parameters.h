#ifndef MELEE_PC_STAGE_IZUMI_PARAMETERS_H
#define MELEE_PC_STAGE_IZUMI_PARAMETERS_H
/* GrIz yakumono_param: grIzumi_YakumonoParam in grizumi.c; twenty f32
 * values and the s32 at+4. This is stage-specific, never a generic DAT swap. */
static void MeleePc_StageIzumiParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || !title_claim(p,root,84,4))return;
 for(uint32_t at=0;at<84 && !p->error;at+=4){
  if(p->relocs[(root+at)/4]){p->error=-1;return;}
  if(at!=4 && (title_u32(p,root+at)&0x7f800000U)==0x7f800000U)p->error=-1;
 }
}
#endif
