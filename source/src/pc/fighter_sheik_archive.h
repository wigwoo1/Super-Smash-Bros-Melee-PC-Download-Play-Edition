#ifndef MELEE_PC_FIGHTER_SHEIK_ARCHIVE_H
#define MELEE_PC_FIGHTER_SHEIK_ARCHIVE_H
#include "archive_external_plan.h"
#include "fighter_sheik_plan.h"
/* Exact diagnostic profile only. Unowned pointer words are accepted only
 * through title_finish_plan's bounded, non-overlapping relocation fallback. */
static int MeleePc_FighterSheikBody(unsigned char* bytes,size_t length,
    uint32_t body,uint32_t relocs,size_t roots,size_t strings){
    if(!bytes || length!=166907 || body!=156352 || relocs!=2626 ||
       roots>length || length-roots<8 || strings>length)return -1;
    uint32_t root=MeleePc_DatBe32(bytes+roots),symbol=MeleePc_DatBe32(bytes+roots+4);
    if(root!=38052 || symbol>=length-strings ||
       !memchr(bytes+strings+symbol,0,length-strings-symbol) ||
       strcmp((char*)bytes+strings+symbol,"ftDataSeak"))return -2;
    MeleePc_TitlePlan p={0};p.bytes=bytes;p.size=body;
    p.capacity=(size_t)relocs+1+body/24+1;
    p.claims=calloc(body,1);p.relocs=calloc((body+3)/4,1);
    p.seen=calloc((body+3)/4,1);p.nodes=malloc(p.capacity*sizeof(*p.nodes));
    unsigned char* external=calloc((body+3)/4,1);uint32_t external_count=0;
    if(!p.claims || !p.relocs || !p.seen || !p.nodes || !external)p.error=-1;
    size_t table=32+(size_t)body;
    for(uint32_t i=0;i<relocs && !p.error;++i){
        uint32_t at=MeleePc_DatBe32(bytes+table+i*4);
        if((at&3) || at>body-4 || p.relocs[at/4])p.error=-1;
        else p.relocs[at/4]=1;
    }
    if(!p.error)p.error=MeleePc_ArchiveExternalPlan(bytes,length,external,&external_count);
    p.external_slots=external;
    if(!p.error && external_count)p.error=-1;
    if(!p.error)MeleePc_FighterSheikPlan(&p,root);
    if(!p.error)title_finish_plan(&p,1);
    int result=p.error;
    free(external);free(p.nodes);free(p.seen);free(p.relocs);free(p.claims);
    return result;
}
#endif
