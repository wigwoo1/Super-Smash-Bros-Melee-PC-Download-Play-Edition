#ifndef MELEE_PC_CLEAR_SIS_ARCHIVE_H
#define MELEE_PC_CLEAR_SIS_ARCHIVE_H
#include "archive_external_plan.h"
#include "stage_stadium_sis.h"
#include "credits_sis_archive.h"
/* Original HSD SisLib table: atlas, byte metrics, three encoded text streams.
 * Default-font US atlas points one past the body and is never dereferenced.
 * The Japanese export contains three 512-byte glyph images. Keep all text,
 * glyph pixels and metrics byte-oriented; convert only the five pointers. */
static void MeleePc_ClearSisPlan(MeleePc_TitlePlan* p,uint32_t root){
 static const uint32_t us[]={128,97,20,55,75};
 static const uint32_t jp[]={96,65,20,33,48};
 const uint32_t* targets=p->size==128?us:jp;
 if(root || (p->size!=128 && p->size!=1632)){p->error=-2;return;}
 for(unsigned i=0;i<5&&!p->error;i++){
  if(p->relocs[i]!=1 || (p->external_slots&&p->external_slots[i]) ||
     title_u32(p,i*4)!=targets[i]){p->error=-2;return;}
  if(title_pointer(p,i*4)==UINT32_MAX){p->error=-2;return;}
 }
 for(unsigned i=2;i<5&&!p->error;i++){
  uint32_t at=targets[i],end=i<4?targets[i+1]:targets[1];int ended=0;
  while(at<end&&!p->error){
   unsigned op=p->bytes[32+at],size=1;
   if(op>=32){
    size=2;if(end-at<2){p->error=-1;return;}
    unsigned code=title_u16(p,at);
    if(!((code>=0x2000 && code<0x2120) ||
         (p->size==1632 && code>=0x4000 && code<0x4003))){p->error=-2;return;}
   }else if(op==8 || op==9 || op>26){p->error=-2;return;}
   else if(op==5)size=3;
   else if(op==6 || op==7 || op==10 || op==14)size=5;
   else if(op==12)size=4;
   if(size>end-at){p->error=-1;return;}
   stadium_sis_bytes(p,at,size);at+=size;
   if(!op){ended=1;break;}
  }
  if(!ended || at!=end){p->error=-1;return;}
 }
 if(!p->error)stadium_sis_bytes(p,targets[1],p->size-targets[1]);
}
/* Original bonus-name SIS table loaded by gmregclear::fn_8017F2A4.
 * 259 pointers: atlas/metrics followed by 257 bounded text streams. */
static void MeleePc_DecisionSisPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t atlas=p->size==8768?8256:6720;
 uint32_t metrics=p->size==8768?8234:6368;
 uint32_t glyphs=p->size==8768?1:162;
 if(root || (p->size!=8768 && p->size!=89664) ||
    title_u32(p,0)!=atlas || title_u32(p,4)!=metrics ||
    title_u32(p,8)!=1036){p->error=-2;return;}
 for(unsigned i=0;i<259&&!p->error;i++){
  if(p->relocs[i]!=1 || (p->external_slots&&p->external_slots[i]) ||
     title_pointer(p,i*4)==UINT32_MAX){p->error=-2;return;}
 }
 for(unsigned i=2;i<259&&!p->error;i++){
  uint32_t at=title_u32(p,i*4);
  uint32_t end=i<258?title_u32(p,(i+1)*4):metrics;
  int ended=0;
  if(at<1036 || at>=end || end>metrics){p->error=-2;return;}
  while(at<end&&!p->error){
   unsigned op=p->bytes[32+at],size=1;
   if(op>=32){
    size=2;if(end-at<2){p->error=-1;return;}
    unsigned code=title_u16(p,at);
    if(!((code>=0x2000 && code<0x2120) ||
         (code>=0x4000 && code-0x4000<glyphs))){p->error=-2;return;}
   }else if(op==8 || op==9 || op>26){p->error=-2;return;}
   else if(op==5)size=3;
   else if(op==6 || op==7 || op==10 || op==14)size=5;
   else if(op==12)size=4;
   if(size>end-at){p->error=-1;return;}
   stadium_sis_bytes(p,at,size);at+=size;
   if(!op){ended=1;break;}
  }
  if(!ended || at!=end){p->error=-1;return;}
 }
 if(!p->error)stadium_sis_bytes(p,metrics,p->size-metrics);
}
static int MeleePc_ClearSisBody(unsigned char* bytes,size_t length,uint32_t body,uint32_t relocs,uint32_t root){
 if(length<32 || !body || body>length-32 || body>64*1024*1024 || relocs>(length-32-body)/4 || relocs>body/4)return -1;
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
 if(!p.error){
  if(body==211712)MeleePc_CreditsSisPlan(&p,root);
  else if(body==8768 || body==89664)MeleePc_DecisionSisPlan(&p,root);
  else MeleePc_ClearSisPlan(&p,root);
 }
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
