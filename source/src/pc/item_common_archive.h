#ifndef MELEE_PC_ITEM_COMMON_ARCHIVE_H
#define MELEE_PC_ITEM_COMMON_ARCHIVE_H
#include "archive_external_plan.h"
#include "item_article_plan.h"
#include "item_archive_tables.h"
#include "item_archive_relocations.h"
#include "item_state_extents.h"
/* Original itPublicData schema. Typed source-consumed graphs are converted;
 * unused allocations retain opaque scalars but their relocation/link words
 * must be native for Archive Locate/LocateExtern. Commit only after planning. */
static int MeleePc_ItemCommonBody(unsigned char* bytes,size_t length,
 uint32_t body,uint32_t relocs,uint32_t root){
 if(!body || body>64*1024*1024 || relocs>body/4)return -1;
 MeleePc_TitlePlan p={0};p.bytes=bytes;p.size=body;
 p.capacity=(size_t)relocs+body/24+2;
 p.claims=calloc(body,1);p.relocs=calloc((body+3)/4,1);
 p.seen=calloc((body+3)/4,1);p.nodes=calloc(p.capacity,sizeof(*p.nodes));
 unsigned char* external=calloc((body+3)/4,1);uint32_t external_count=0;
 if(!p.claims || !p.relocs || !p.seen || !p.nodes || !external)p.error=-1;
 for(uint32_t i=0;i<relocs && !p.error;i++){
  uint32_t at=MeleePc_DatBe32(bytes+32+body+i*4);
  if((at&3)||!title_range(&p,at,4)||p.relocs[at/4])p.error=-1;
  else p.relocs[at/4]=1;
 }
 if(!p.error)p.error=MeleePc_ArchiveExternalPlan(bytes,length,external,&external_count);
 p.external_slots=external;
 if(!p.error)MeleePc_ItemArchiveTablesPlan(&p,root);
 const uint32_t counts[]={43,118,47},bases[]={0,43,161};
 for(unsigned group=0;group<3 && !p.error;group++){
  uint32_t table=title_pointer(&p,root+4+group*4);
  for(uint32_t index=0;index<counts[group] && !p.error;index++){
   uint32_t article=title_pointer(&p,table+index*4),kind=bases[group]+index;
   if(article!=UINT32_MAX)MeleePc_ItemArticlePlan(&p,article,kind,melee_pc_item_state_extents[kind]);
  }
 }
 while(p.head<p.count && !p.error){MeleePc_TitleNode node=p.nodes[p.head++];title_walk(&p,node);}
 MeleePc_ItemArchiveRelocationsPlan(&p);
 for(uint32_t i=0;i<(body+3)/4 && !p.error;i++)if(external[i])title_claim(&p,i*4,4,4);
 if(!p.error)for(uint32_t at=0;at<body;){unsigned width=p.claims[at];
  if(width==4){MeleePc_DatSwap32(bytes+32+at);at+=4;}
  else if(width==2){uint16_t v=(uint16_t)title_u16(&p,at);memcpy(bytes+32+at,&v,2);at+=2;}
  else at++;
 }
 free(external);free(p.nodes);free(p.seen);free(p.relocs);free(p.claims);return p.error;
}
#endif
