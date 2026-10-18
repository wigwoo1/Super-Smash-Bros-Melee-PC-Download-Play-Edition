#ifndef MELEE_PC_STAGE_ZEBES_PARAMETERS_H
#define MELEE_PC_STAGE_ZEBES_PARAMETERS_H
/* grZe_YakumonoParam in grzebes.c. Preserve its unused opaque bytes;
 * acid_hit points to the nine-word lbColl_80008D30_arg1 descriptor. */
static void MeleePc_StageZebesParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,400)){p->error=-1;return;}
 for(uint32_t at=0;at<400;at+=4){
  if(at!=44 && (p->relocs[(root+at)/4] ||
     (p->external_slots && p->external_slots[(root+at)/4]))){p->error=-1;return;}
 }
 title_claim(p,root,20,4);title_claim(p,root+20,24,1);
 title_claim(p,root+48,112,4);title_claim(p,root+160,240,2);
 for(uint32_t at=0;at<160;at+=4){
  if(at==16 || (at>=20 && at<48))continue;
  if((title_u32(p,root+at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 }
 /* This stage requires a local hazard descriptor, not a NULL import. */
 if(p->external_slots && p->external_slots[(root+44)/4]){p->error=-1;return;}
 uint32_t hazard=title_pointer(p,root+44);
 if(p->error || hazard==UINT32_MAX || (hazard&3) ||
    !title_range(p,hazard,36)){p->error=-1;return;}
 if(hazard<root+400 && root<hazard+36){p->error=-1;return;}
 for(uint32_t at=hazard;at<hazard+36;at+=4){
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4])){
   p->error=-1;return;
  }
 }
 title_claim(p,hazard,36,4);
}
#endif
