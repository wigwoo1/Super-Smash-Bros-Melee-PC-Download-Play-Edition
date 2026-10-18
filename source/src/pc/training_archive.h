#ifndef MELEE_PC_TRAINING_ARCHIVE_H
#define MELEE_PC_TRAINING_ARCHIVE_H
#include "archive_external_plan.h"
#include "stage_stadium_sis.h"
/* gm_1884.c loads DynamicModelDesc** and SIS_TrainingData, not SceneDesc.
 * Admit the two audited language layouts; all local relocations must receive
 * typed ownership before committing. Shared walkers validate full payloads. */
static int MeleePc_TrainingBody(unsigned char* bytes,size_t length,uint32_t body,
                                uint32_t relocs,uint32_t root,int text) {
    if (text) {
        if (relocs!=83 || root || !((length==16709 && body==16320) ||
                                    (length==13445 && body==13056))) return -2;
    } else if (relocs!=1499 || !((length==79383 && body==73320 && root==73312) ||
                                (length==81879 && body==75816 && root==75808))) return -2;
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
    if(!p.error) {
        if(text)MeleePc_StageSisPlan(&p,root,83);
        else title_array(&p,root,TITLE_DynamicModelDesc,0,1);
    }
    int result=p.error ? p.error : title_finish_plan(&p,0);
    free(external);free(p.claims);free(p.relocs);free(p.seen);free(p.nodes);
    return result;
}
#endif
