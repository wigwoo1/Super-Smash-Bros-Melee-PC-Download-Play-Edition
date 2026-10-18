#ifndef MELEE_PC_FIGHTER_CLINK_ARCHIVE_H
#define MELEE_PC_FIGHTER_CLINK_ARCHIVE_H
#include "archive_external_plan.h"
#include "fighter_clink_complete_plan.h"
/* Exact profile and import identities, followed by a full scratch transaction. */
static int MeleePc_FighterCLinkBody(unsigned char* bytes,size_t length,
    uint32_t body,uint32_t relocs,uint32_t root) {
    if(length!=289480 || body!=271904 || relocs!=4355 || root!=41680 ||
       MeleePc_DatBe32(bytes+12)!=1 || MeleePc_DatBe32(bytes+16)!=2)return -2;
    size_t refs=32+(size_t)body+(size_t)relocs*4+8,strings=refs+16;
    const uint32_t first_slots[]={16536,16540};
    const char* names[]={"ItmLinkHShot_TopN_ACTION_Out_matanim_joint","ItmLinkHShot_TopN_ACTION_Out_shapeanim_joint"};
    for(unsigned i=0;i<2;++i) {
        uint32_t off=MeleePc_DatBe32(bytes+refs+i*8+4);
        if(MeleePc_DatBe32(bytes+refs+i*8)!=first_slots[i] || off>=length-strings ||
           !memchr(bytes+strings+off,0,length-strings-off) || strcmp((char*)bytes+strings+off,names[i]))return -2;
    }
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
    if(!p.error && external_count!=4)p.error=-2;
    if(!p.error)MeleePc_FighterCLinkCompletePlan(&p,root);
    int result=p.error ? p.error : title_finish_plan(&p,0);
    free(external);free(p.claims);free(p.relocs);free(p.seen);free(p.nodes);
    return result;
}
#endif
