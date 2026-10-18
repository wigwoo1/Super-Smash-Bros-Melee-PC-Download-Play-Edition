#ifndef MELEE_PC_FIGHTER_PICHU_ARCHIVE_H
#define MELEE_PC_FIGHTER_PICHU_ARCHIVE_H
#include "archive_external_plan.h"
#include "fighter_pichu_plan.h"
/* Exact source-audited main fighter profile. Packed actions/FObj payloads stay
 * byte-oriented. Commit only after complete typed ownership and validation. */
static int MeleePc_FighterPichuBody(unsigned char* bytes,size_t length,
    uint32_t body,uint32_t relocs,uint32_t root) {
    if(length!=240228 || body!=232960 || relocs!=1804 || root!=38400)return -2;
    MeleePc_TitlePlan p={0};p.bytes=bytes;p.size=body;p.capacity=(size_t)relocs+body/24+2;
    p.claims=calloc(body,1);p.relocs=calloc((body+3)/4,1);p.seen=calloc((body+3)/4,1);
    p.nodes=malloc(p.capacity*sizeof(*p.nodes));
    unsigned char* external=calloc((body+3)/4,1);uint32_t external_count=0;p.external_slots=external;
    if(!p.claims || !p.relocs || !p.seen || !p.nodes || !external)p.error=-1;
    for(uint32_t i=0;i<relocs && !p.error;++i) {
        uint32_t at=MeleePc_DatBe32(bytes+32+body+i*4);
        if((at&3) || !title_range(&p,at,4) || p.relocs[at/4])p.error=-1;
        else p.relocs[at/4]=1;
    }
    if(!p.error)p.error=MeleePc_ArchiveExternalPlan(bytes,length,external,&external_count);
    if(!p.error && external_count)p.error=-2;
    if(!p.error)MeleePc_FighterPichuPlan(&p,root);
    int result=p.error ? p.error : title_finish_plan(&p,0);
    free(external);free(p.claims);free(p.relocs);free(p.seen);free(p.nodes);
    return result;
}
#endif
