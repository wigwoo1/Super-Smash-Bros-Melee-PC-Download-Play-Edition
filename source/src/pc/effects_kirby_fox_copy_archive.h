#ifndef MELEE_PC_EFFECTS_KIRBY_FOX_COPY_ARCHIVE_H
#define MELEE_PC_EFFECTS_KIRBY_FOX_COPY_ARCHIVE_H
#include "archive_external_plan.h"
#include "effects_kirby_fox_copy_plan.h"
/* Original audited Kirby Fox-copy effects layout; commit only after full scratch validation. */
static int MeleePc_EffectsKirbyFoxCopyBody(unsigned char* bytes,size_t length,uint32_t body,uint32_t relocs,uint32_t root){
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
 if(!p.error&&external_count)p.error=-2;
 if(!p.error)MeleePc_EffectsKirbyFoxCopyPlan(&p,root);
 while(p.head<p.count&&!p.error)title_walk(&p,p.nodes[p.head++]);
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


