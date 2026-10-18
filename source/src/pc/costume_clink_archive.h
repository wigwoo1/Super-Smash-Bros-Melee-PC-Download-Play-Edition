#ifndef MELEE_PC_COSTUME_CLINK_ARCHIVE_H
#define MELEE_PC_COSTUME_CLINK_ARCHIVE_H
#include "archive_external_plan.h"
#include "costume_clink_profiles.h"
/* Exact read-only-audited original CLink profiles. Conversion commits only
 * after both source-selected roots and every local relocation validate. */
static int MeleePc_CostumeCLinkBody(unsigned char* bytes, size_t length,
                                      uint32_t body, uint32_t relocs) {
    const MeleePcCLinkCostumeProfile* profile=MeleePc_CLinkCostumeProfile(bytes,length);
    if (!profile || body!=profile->body || relocs!=profile->relocations) return -1;
    MeleePc_TitlePlan p = { 0 };
    p.bytes = bytes;
    p.size = body;
    p.capacity = (size_t) relocs + body / 24 + 2;
    p.claims = calloc(body, 1);
    p.relocs = calloc((body + 3) / 4, 1);
    p.seen = calloc((body + 3) / 4, 1);
    p.nodes = malloc(p.capacity * sizeof(*p.nodes));
    unsigned char* external = calloc((body + 3) / 4, 1);
    uint32_t external_count = 0;
    p.external_slots = external;
    if (!p.claims || !p.relocs || !p.seen || !p.nodes || !external)
        p.error = -1;
    for (uint32_t i = 0; i < relocs && !p.error; ++i) {
        uint32_t at = MeleePc_DatBe32(bytes + 32 + body + i * 4);
        if ((at & 3) || !title_range(&p, at, 4) || p.relocs[at / 4])
            p.error = -1;
        else
            p.relocs[at / 4] = 1;
    }
    if (!p.error)
        p.error = MeleePc_ArchiveExternalPlan(bytes, length, external,
                                             &external_count);
    if (!p.error && external_count) p.error = -2;
    if (!p.error) {
        title_enqueue(&p,profile->joint,TITLE_HSD_Joint);
        title_enqueue(&p,profile->matanim,TITLE_HSD_MatAnimJoint);
    }
    int result = p.error ? p.error : title_finish_plan(&p, 0);
    free(external);
    free(p.claims);
    free(p.relocs);
    free(p.seen);
    free(p.nodes);
    return result;
}
#endif
