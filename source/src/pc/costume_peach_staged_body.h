#ifndef MELEE_PC_COSTUME_PEACH_STAGED_ARCHIVE_H
#define MELEE_PC_COSTUME_PEACH_STAGED_ARCHIVE_H
#include "archive_external_plan.h"
#include "costume_peach_plan.h"
/* Five exact Peach costume layouts; guarded production dispatch, commit only after full scratch validation. */
static int MeleePc_CostumePeachStagedBody(unsigned char* bytes,size_t length,uint32_t body,uint32_t relocs,uint32_t root){
 if(!bytes || !((length==809794 && body==801824 && relocs==1967 && root==127624) || (length==764870 && body==756640 && relocs==2031 && root==134024) || (length==806798 && body==798848 && relocs==1961 && root==127624) || (length==806990 && body==799040 && relocs==1961 && root==127624) || (length==806862 && body==798912 && relocs==1961 && root==127624)))return -1;
 if(MeleePc_DatBe32(bytes)!=length || MeleePc_DatBe32(bytes+4)!=body || MeleePc_DatBe32(bytes+8)!=relocs || MeleePc_DatBe32(bytes+12)!=2 || MeleePc_DatBe32(bytes+16)!=0)return -1;
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
 if(!p.error)MeleePc_CostumePeachPlan(&p,root);
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
