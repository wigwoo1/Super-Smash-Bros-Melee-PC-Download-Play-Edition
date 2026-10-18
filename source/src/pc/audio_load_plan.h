#ifndef MELEE_PC_AUDIO_LOAD_PLAN_H
#define MELEE_PC_AUDIO_LOAD_PLAN_H
/* lbaudio_ax.h lbl_804D6454_t: four int** variants, thirty ID lists,
 * terminated by the numeric 0x83d60 sentinel consumed by sound-test code. */
static void MeleePc_AudioLoadPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(!title_range(p,root,16))return;
 for(unsigned variant=0;variant<4&&!p->error;variant++){
  uint32_t table=title_pointer(p,root+variant*4);
  if(table==UINT32_MAX||(table&3)||!title_range(p,table,30*4)){p->error=-1;return;}
  for(unsigned index=0;index<30&&!p->error;index++){
   uint32_t at=title_pointer(p,table+index*4);int ended=0;
   if(at==UINT32_MAX||(at&3)){p->error=-1;return;}
   for(unsigned i=0;i<p->size/4&&!p->error;i++,at+=4){
    if(!title_range(p,at,4))return;
    if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
    if(!title_claim(p,at,4,4))return;
    uint32_t value=title_u32(p,at);
    if(value==0x83d60){ended=1;break;}
    if(value>0x83d60){p->error=-2;return;}
   }if(!ended)p->error=-1;
  }
 }
}
#endif
