#ifndef MELEE_PC_COSTUME_GANON_ARCHIVE_H
#define MELEE_PC_COSTUME_GANON_ARCHIVE_H
#include "archive_external_plan.h"
/* Source-registered original Ganondorf costumes. Dispatch checks the exact
 * symbol/envelope; all model, envelope and packed payload ownership is then
 * validated in scratch before commit. No public material animation is added. */
static int MeleePc_CostumeGanonBody(unsigned char* bytes,size_t length,
    uint32_t body,uint32_t relocs,uint32_t root) {
    if(root!=120872 || !((length==387979 && body==381344 && relocs==1643) ||
                        (length==391121 && body==383792 && relocs==1816)))return -2;
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
        title_enqueue(&p,root,TITLE_HSD_Joint);
        /* Four alternate colors retain an unexported empty MatAnimJoint tree:
         * 84 nodes mirror the model and 90 empty MatAnims mirror its DObjs.
         * Audit owns the full tail; conversion does not attach this tree. */
        if(body==383792)title_enqueue(&p,382784,TITLE_HSD_MatAnimJoint);
    }
    int result=p.error ? p.error : title_finish_plan(&p,0);
    free(external);free(p.claims);free(p.relocs);free(p.seen);free(p.nodes);
    return result;
}
#endif
