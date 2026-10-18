#ifndef MELEE_PC_STAGE_STADIUM_PARAMETERS_H
#define MELEE_PC_STAGE_STADIUM_PARAMETERS_H
/* Original private grPStadium_YakumonoParam in grpstadium.c: seven ints,
 * RGB bytes/padding, ten u32 words, five s16 values and tail padding. */
static void MeleePc_StageStadiumParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,84)){if(root&3)p->error=-1;return;}
 for(uint32_t at=root;at<root+84;at+=4){
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){p->error=-1;return;}
 }
 title_claim(p,root,28,4);title_claim(p,root+28,4,1);
 title_claim(p,root+32,40,4);title_claim(p,root+72,10,2);
 title_claim(p,root+82,2,1);
}
#endif
