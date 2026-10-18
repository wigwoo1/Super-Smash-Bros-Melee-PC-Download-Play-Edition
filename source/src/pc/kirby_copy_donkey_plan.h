#ifndef MELEE_PC_KIRBY_COPY_DONKEY_PLAN_H
#define MELEE_PC_KIRBY_COPY_DONKEY_PLAN_H
#include "fighter_model_parts.h"
/* ftKbCopyParts: source six-costume visibility/texture tables, mask and model. */
static void kirby_copy_donkey_parts(MeleePc_TitlePlan* p,uint32_t root,unsigned costumes){
 uint32_t vis,tex;
 if(p->error)return;
 if(!costumes||costumes>p->size/16||!fighter_sound_span(p,root,16)||
    !title_range(p,root,24)||!fighter_sound_scalar(p,root)||
    !fighter_sound_scalar(p,root+8)||!fighter_sound_pointer(p,root+4,&vis)||
    !fighter_sound_pointer(p,root+12,&tex)||vis==UINT32_MAX||tex==UINT32_MAX){p->error=-1;return;}
 uint32_t models=title_u32(p,root),textures=title_u32(p,root+8);
 if(models>11||textures>5||!fighter_sound_span(p,vis,costumes*16)||
    !fighter_sound_span(p,tex,costumes*4)){p->error=-1;return;}
 uint32_t model;
 if(!fighter_sound_scalar(p,root+16)||!fighter_sound_pointer(p,root+20,&model)||model==UINT32_MAX){p->error=-1;return;}
 title_claim(p,root,16,4);title_pointer(p,root+4);title_pointer(p,root+12);
 title_claim(p,root+16,8,4);title_pointer(p,root+20);
 title_enqueue(p,model,TITLE_HSD_Joint);
 for(unsigned c=0;c<costumes;c++){
  uint32_t indices;
  if(!fighter_sound_pointer(p,tex+c*4,&indices)){p->error=-1;return;}
  title_pointer(p,tex+c*4);
  if(indices!=UINT32_MAX&&!fighter_parts_payload(p,indices,textures,2)){p->error=-1;return;}
  for(unsigned v=0;v<4;v++){
   uint32_t lookup;
   if(!fighter_sound_pointer(p,vis+c*16+v*4,&lookup)){p->error=-1;return;}
   title_pointer(p,vis+c*16+v*4);
   if(lookup==UINT32_MAX)continue;
   if(!fighter_sound_span(p,lookup,models*8)){p->error=-1;return;}
   for(unsigned m=0;m<models;m++){
    uint32_t at=lookup+m*8,groups;
    if(!fighter_sound_scalar(p,at)||!fighter_sound_pointer(p,at+4,&groups)){p->error=-1;return;}
    uint32_t count=title_u32(p,at);
    if(count>p->size/8||(count&&groups==UINT32_MAX)||
       (groups!=UINT32_MAX&&!fighter_sound_span(p,groups,count*8))){p->error=-1;return;}
    title_claim(p,at,8,4);title_pointer(p,at+4);
    for(uint32_t g=0;g<count;g++){
     uint32_t group=groups+g*8,bytes;
     if(!fighter_sound_scalar(p,group)||!fighter_sound_pointer(p,group+4,&bytes)){p->error=-1;return;}
     uint32_t n=title_u32(p,group);
     if(n>p->size||(n&&bytes==UINT32_MAX)||
        (bytes!=UINT32_MAX&&!fighter_parts_payload(p,bytes,n,1))){p->error=-1;return;}
     title_claim(p,group,8,4);title_pointer(p,group+4);
    }
   }
  }
 }
}

static void MeleePc_KirbyCopyDonkeyPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=47648||root!=200){p->error=-2;return;}
 kirby_copy_donkey_parts(p,root,6);
}
#endif
