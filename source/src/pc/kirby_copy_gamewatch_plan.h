#ifndef MELEE_PC_KIRBY_COPY_GAMEWATCH_PLAN_H
#define MELEE_PC_KIRBY_COPY_GAMEWATCH_PLAN_H
#include "fighter_model_parts.h"
#include "item_gamewatch_article_plan.h"
/* ftKbCopyParts: source six-costume visibility/texture tables, mask and model. */
static void kirby_copy_gamewatch_parts(MeleePc_TitlePlan* p,uint32_t root,unsigned costumes){
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
 if(!fighter_sound_scalar(p,root+16)||!fighter_sound_pointer(p,root+20,&model)){p->error=-1;return;}
 title_claim(p,root,16,4);title_pointer(p,root+4);title_pointer(p,root+12);
 title_claim(p,root+16,8,4);title_pointer(p,root+20);
 if(model!=UINT32_MAX)title_enqueue(p,model,TITLE_HSD_Joint);
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

/* Source ftKbCopyParts_GameWatch; shared model comes from costume. */
static void MeleePc_KirbyCopyGameWatchPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=3296||root!=788||!title_range(p,root,40)){p->error=-2;return;}
 kirby_copy_gamewatch_parts(p,root,6);
 uint32_t vis=title_pointer(p,root+24),colors=title_pointer(p,root+28);
 MeleePc_FighterVisibilityLookupPlan(p,vis,1);
 if(colors!=0||!title_range(p,colors,12)){p->error=-2;return;}
 for(unsigned off=0;off<12;off+=4)if(p->relocs[(colors+off)/4]||(p->external_slots&&p->external_slots[(colors+off)/4])){p->error=-1;return;}
 if((title_u32(p,colors)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
 title_claim(p,colors,4,4);title_claim(p,colors+4,8,1);
 uint32_t chef=title_pointer(p,root+32),pan=title_pointer(p,root+36);
 if(chef==UINT32_MAX||pan==UINT32_MAX){p->error=-1;return;}
 MeleePc_ItemGameWatchArticlePlan(p,chef,8);
 /* Source ChefPan state table has motion -1; no archived state animations. */
 if(!title_range(p,pan,24)){p->error=-1;return;}
 uint32_t targets[6];
 for(unsigned i=0;i<6&&!p->error;i++)targets[i]=item_dynamics_pointer(p,pan+i*4);
 if(p->error)return;
 if(targets[0]!=UINT32_MAX)MeleePc_ItemArticleAttributesPlan(p,targets[0]);
 if(targets[1]==UINT32_MAX||targets[3]!=UINT32_MAX){p->error=-2;return;}
 uint32_t attachment=item_dynamics_pointer(p,targets[1]);
 if(attachment==UINT32_MAX){p->error=-1;return;}
 MeleePc_GameWatchAttachmentPlan(p,attachment);
 if(targets[2]!=UINT32_MAX)MeleePc_ItemHurtbonesPlan(p,targets[2]);
 if(targets[4]!=UINT32_MAX)MeleePc_ItemModelDescriptorPlan(p,targets[4]);
 if(targets[5]!=UINT32_MAX)MeleePc_ItemDynamicsPlan(p,targets[5]);
}
#endif
