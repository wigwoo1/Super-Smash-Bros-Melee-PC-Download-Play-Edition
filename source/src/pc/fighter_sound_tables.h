#ifndef MELEE_PC_FIGHTER_SOUND_TABLES_H
#define MELEE_PC_FIGHTER_SOUND_TABLES_H
#include "title_archive.h"
static int fighter_sound_span(MeleePc_TitlePlan* p,uint32_t at,uint32_t size){
 if((at&3)||!title_range(p,at,size))return 0;
 for(uint32_t off=0;off<size;off++)if(p->claims[at+off]&&p->claims[at+off]!=4)return 0;
 return 1;
}
static int fighter_sound_scalar(MeleePc_TitlePlan* p,uint32_t at){
 return !p->relocs[at/4] && !(p->external_slots&&p->external_slots[at/4]);
}
static int fighter_sound_pointer(MeleePc_TitlePlan* p,uint32_t at,uint32_t* target){
 if(p->external_slots&&p->external_slots[at/4])return 0;
 *target=title_u32(p,at);
 if(p->relocs[at/4])return *target<=p->size;
 if(*target)return 0;
 *target=UINT32_MAX;return 1;
}
/* FtSFX has three optional counted FtSFXArr pointers and eleven signed IDs. */
static void MeleePc_FighterSoundTablesPlan(MeleePc_TitlePlan* p,uint32_t root){
 static const unsigned offsets[]={0,28,32};
 uint32_t records[3],arrays[3],counts[3];
 if(p->error)return;
 if(!fighter_sound_span(p,root,56)){p->error=-1;return;}
 for(unsigned off=0;off<56;off+=4)
  if(off!=0&&off!=28&&off!=32&&!fighter_sound_scalar(p,root+off)){p->error=-1;return;}
 for(unsigned i=0;i<3;i++){
  if(!fighter_sound_pointer(p,root+offsets[i],&records[i])){p->error=-1;return;}
  if(records[i]==UINT32_MAX)continue;
  uint32_t at=records[i];
  if(!fighter_sound_span(p,at,8)||!fighter_sound_scalar(p,at)){p->error=-1;return;}
  counts[i]=title_u32(p,at);
  if(!counts[i]||counts[i]>p->size/4||!fighter_sound_pointer(p,at+4,&arrays[i])||
     arrays[i]==UINT32_MAX||!fighter_sound_span(p,arrays[i],counts[i]*4)){p->error=-1;return;}
  for(uint32_t k=0;k<counts[i];k++)if(!fighter_sound_scalar(p,arrays[i]+k*4)){p->error=-1;return;}
 }
 title_claim(p,root,56,4);
 for(unsigned i=0;i<3;i++){
  title_pointer(p,root+offsets[i]);
  if(records[i]==UINT32_MAX)continue;
  title_claim(p,records[i],8,4);title_pointer(p,records[i]+4);
  title_claim(p,arrays[i],counts[i]*4,4);
 }
}
#endif
