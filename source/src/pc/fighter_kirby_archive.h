#ifndef MELEE_PC_FIGHTER_KIRBY_ARCHIVE_H
#define MELEE_PC_FIGHTER_KIRBY_ARCHIVE_H
#include "archive_external_plan.h"
#include "fighter_kirby_plan.h"
#include "item_hurtbones.h"
/* Verified original Kirby serialized layout. Auxiliary allocations are
 * converted in place, never attached to the live ftData/Article roots.
 * Layout-changing mods need a separately audited profile. */
static void MeleePc_FighterKirbyCompletePlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=183584 || root!=57936){p->error=-2;return;}
 const uint32_t animations[]={61092,62112,63140,64168,66188,68208,70228};
 MeleePc_FighterAnimationGraphsPlan(p,animations,7);
 const uint32_t actions[]={28904,29284};
 MeleePc_FighterActionStreamsPlan(p,actions,2);
 MeleePc_ItemHurtbonesPlan(p,57716);
 const uint32_t wrappers[]={74592,88352,107664};
 const uint32_t joints[]={72392,78344,90984};
 for(unsigned i=0;i<3&&!p->error;i++)for(unsigned off=0;off<16;off+=4){
  uint32_t target=title_pointer(p,wrappers[i]+off);
  uint32_t expected=off==0?joints[i]:(i==1&&off==4?88344:UINT32_MAX);
  if(target!=expected)p->error=-2;
 }
 const uint32_t tables[]={74608,88344,88368,107680};
 const uint32_t targets[]={74592,88304,88352,107664};
 for(unsigned i=0;i<4&&!p->error;i++){
  if(title_pointer(p,tables[i])!=targets[i] ||
     !title_range(p,tables[i]+4,4)||p->relocs[(tables[i]+4)/4]||
     (p->external_slots&&p->external_slots[(tables[i]+4)/4])||
     title_u32(p,tables[i]+4)){p->error=-1;break;}
  title_claim(p,tables[i]+4,4,4);
 }
 MeleePc_FighterKirbyPlan(p,root);
 while(p->head<p->count&&!p->error){MeleePc_TitleNode node=p->nodes[p->head++];title_walk(p,node);}
}
static int MeleePc_FighterKirbyBody(unsigned char* bytes,size_t length,uint32_t body,uint32_t relocs,uint32_t root){
 if(!body || body>64*1024*1024 || relocs>body/4)return -1;
 MeleePc_TitlePlan p={0};p.bytes=bytes;p.size=body;p.capacity=(size_t)relocs+body/24+2;
 p.claims=calloc(body,1);p.relocs=calloc((body+3)/4,1);p.seen=calloc((body+3)/4,1);p.nodes=malloc(p.capacity*sizeof(*p.nodes));
 unsigned char* external=calloc((body+3)/4,1);uint32_t external_count=0;p.external_slots=external;
 if(!p.claims||!p.relocs||!p.seen||!p.nodes||!external)p.error=-1;
 for(uint32_t i=0;i<relocs&&!p.error;i++){
  uint32_t at=MeleePc_DatBe32(bytes+32+body+i*4);
  if((at&3)||!title_range(&p,at,4)||p.relocs[at/4]){p.error=-1;break;}
  p.relocs[at/4]=1;
 }
 if(!p.error)p.error=MeleePc_ArchiveExternalPlan(bytes,length,external,&external_count);
 if(!p.error)MeleePc_FighterKirbyCompletePlan(&p,root);
 for(uint32_t i=0;i<(body+3)/4&&!p.error;i++)if(p.relocs[i]&&p.relocs[i]!=2)p.error=-2;
 for(uint32_t at=0;at<body&&!p.error;){
  unsigned width=p.claims[at];
  if(width==2||width==4){
   if(at%width||width>body-at){p.error=-1;break;}
   for(unsigned j=0;j<width;j++)if(p.claims[at+j]!=width)p.error=-1;
   at+=width;
  }else if(width<=1)++at;else p.error=-1;
 }
 if(!p.error)for(uint32_t at=0;at<body;){
  unsigned width=p.claims[at];
  if(width==2){uint16_t v=(uint16_t)title_u16(&p,at);memcpy(bytes+32+at,&v,2);at+=2;}
  else if(width==4){MeleePc_DatSwap32(bytes+32+at);at+=4;}else ++at;
 }
 free(external);free(p.claims);free(p.relocs);free(p.seen);free(p.nodes);return p.error;
}
#endif
