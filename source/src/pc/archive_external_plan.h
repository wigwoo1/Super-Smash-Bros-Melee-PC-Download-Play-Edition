#ifndef MELEE_PC_ARCHIVE_EXTERNAL_PLAN_H
#define MELEE_PC_ARCHIVE_EXTERNAL_PLAN_H
#include <stdlib.h>
/* BE external-reference chains contain next-slot offsets, ending at -1.
 * These words are not DAT relocation targets and must remain chain links
 * until ArchiveLocateExtern binds a provider. No archive bytes are changed.
 * Output bitmap/count are committed only after every chain validates. */
static int MeleePc_ArchiveExternalPlan(const unsigned char* bytes,size_t length,
 unsigned char* slots,uint32_t* total){
 if(!bytes || !slots || !total || length<32)return -1;
 uint32_t body=MeleePc_DatBe32(bytes+4),nr=MeleePc_DatBe32(bytes+8);
 uint32_t np=MeleePc_DatBe32(bytes+12),ne=MeleePc_DatBe32(bytes+16);
 if(body>length-32)return -1;
 size_t table=32+(size_t)body;
 if(nr>(length-table)/4)return -1;
 size_t roots=table+(size_t)nr*4;
 if(np>(length-roots)/8)return -1;
 size_t refs=roots+(size_t)np*8;
 if(ne>(length-refs)/8)return -1;
 size_t strings=refs+(size_t)ne*8,words=((size_t)body+3)/4;
 unsigned char* seen=calloc(words?words:1,1);
 if(!seen)return -1;
 int error=0;uint32_t count=0;
 for(uint32_t i=0;i<ne && !error;i++){
  uint32_t at=MeleePc_DatBe32(bytes+refs+i*8),symbol=MeleePc_DatBe32(bytes+refs+i*8+4);
  if(symbol>=length-strings || !memchr(bytes+strings+symbol,0,length-strings-symbol)){error=-1;break;}
  while(at!=UINT32_MAX){
   if((at&3) || body<4 || at>body-4 || seen[at/4]){error=-1;break;}
   for(uint32_t r=0;r<nr;r++)if(MeleePc_DatBe32(bytes+table+r*4)==at){error=-1;break;}
   if(error)break;
   seen[at/4]=1;++count;at=MeleePc_DatBe32(bytes+32+at);
  }
 }
 if(!error){memcpy(slots,seen,words);*total=count;}
 free(seen);return error;
}
#endif
