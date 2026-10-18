#ifndef MELEE_PC_ITEM_GAMEWATCH_ARTICLE_PLAN_H
#define MELEE_PC_ITEM_GAMEWATCH_ARTICLE_PLAN_H
#include "item_article_plan.h"
/* Staged GameWatch article profile. Attachment special attrs are a source-used
 * pointer prefix; Chef adds three floats and five source-indexed 20-byte entries.
 * Drawing reads two u16 counts and byte-index lists through it_266F_ItemVars. */
static void MeleePc_GameWatchAttachmentPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error||(root&3)||!title_range(p,root,16)){p->error=-1;return;}
 for(unsigned off=0;off<16;off+=8){
  uint32_t at=root+off;
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-1;return;}
  unsigned count=((unsigned)p->bytes[32+at]<<8)|p->bytes[32+at+1];
  uint32_t list=item_dynamics_pointer(p,at+4);
  if(p->error||(count&&(list==UINT32_MAX||!title_range(p,list,count)))){p->error=-1;return;}
  title_claim(p,at,2,2);title_claim(p,at+2,2,1);
  if(count)title_claim(p,list,count,1);
 }
}
static void MeleePc_ItemGameWatchArticlePlan(MeleePc_TitlePlan* p,uint32_t root,unsigned index){
 static const unsigned counts[]={4,1,1,2,2,2,1,2,2,2};
 if(p->error||index>=10||(root&3)||!title_range(p,root,24)){p->error=-1;return;}
 uint32_t targets[6];
 for(unsigned i=0;i<6&&!p->error;i++)targets[i]=item_dynamics_pointer(p,root+i*4);
 if(p->error)return;
 if(targets[0]!=UINT32_MAX)MeleePc_ItemArticleAttributesPlan(p,targets[0]);
 if(targets[1]==UINT32_MAX){p->error=-1;return;}
 uint32_t attachment=item_dynamics_pointer(p,targets[1]);
 if(attachment==UINT32_MAX){p->error=-1;return;}
 MeleePc_GameWatchAttachmentPlan(p,attachment);
 if(index==8){
  if(!title_range(p,targets[1],116)){p->error=-1;return;}
  for(unsigned off=4;off<116;off+=4){
   uint32_t at=targets[1]+off;
   if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])||
      (title_u32(p,at)&0x7f800000U)==0x7f800000U){p->error=-1;return;}
  }
  title_claim(p,targets[1]+4,112,4);
 }
 if(targets[2]!=UINT32_MAX)MeleePc_ItemHurtbonesPlan(p,targets[2]);
 if(targets[3]==UINT32_MAX){p->error=-1;return;}
 MeleePc_ItemStateAnimationsPlan(p,targets[3],counts[index]);
 for(unsigned i=0;i<counts[index]&&!p->error;i++){
  uint32_t command=title_pointer(p,targets[3]+i*16+12);
  MeleePc_ItemCommandPlan(p,&command,1);
 }
 if(targets[4]!=UINT32_MAX)MeleePc_ItemModelDescriptorPlan(p,targets[4]);
 if(targets[5]!=UINT32_MAX)MeleePc_ItemDynamicsPlan(p,targets[5]);
}
#endif
