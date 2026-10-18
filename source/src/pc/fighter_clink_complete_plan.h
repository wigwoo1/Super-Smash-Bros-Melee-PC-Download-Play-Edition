#ifndef MELEE_PC_FIGHTER_CLINK_COMPLETE_PLAN_H
#define MELEE_PC_FIGHTER_CLINK_COMPLETE_PLAN_H
#include "fighter_clink_plan.h"
/* Exact retail auxiliary directories audited against sc/types.h. They are
 * converted in place but never attached/published as additional game content. */
static void MeleePc_FighterCLinkCompletePlan(MeleePc_TitlePlan* p,uint32_t root) {
    if(p->error)return;
    const uint32_t parents[]={47848,50428,53108,55788,58368,60948,63728,66508,68372,70236,72100};
    MeleePc_FighterAnimationGraphsPlan(p,parents,11);
    MeleePc_FighterCLinkPlan(p,root);
    const uint32_t directories[]={92380,104116,116924,121200,140472,142544,144624,193316};
    for(unsigned i=0;i<8&&!p->error;++i)
        title_array(p,directories[i],TITLE_DynamicModelDesc,0,1);
    while(p->head<p->count&&!p->error)title_walk(p,p->nodes[p->head++]);
    /* Disconnected DAT-declared pointer aliases have no scalar reader. Pin
     * each slot/target/type; preserve adjacent opaque bytes. This is not a
     * generic untyped-relocation fallback or a guessed allocation extent. */
    static const uint32_t aliases[][3]={
        {16532,116676,TITLE_HSD_AnimJoint},{41156,103880,TITLE_HSD_Joint},
        {41160,103240,TITLE_HSD_Joint},{41164,104052,TITLE_HSD_AnimJoint},
        {41168,103412,TITLE_HSD_AnimJoint},{41244,116676,TITLE_HSD_AnimJoint},
        {41256,107752,TITLE_HSD_Joint},{41260,106184,TITLE_HSD_Joint},
        {41264,105000,TITLE_HSD_Joint},{41324,140904,TITLE_HSD_Joint},
        {41328,142984,TITLE_HSD_Joint},
    };
    for(unsigned i=0;i<11&&!p->error;++i) {
        uint32_t at=aliases[i][0],target=aliases[i][1];
        if(p->relocs[at/4]!=1 || (p->external_slots&&p->external_slots[at/4]) ||
           title_u32(p,at)!=target || p->seen[target/4]!=aliases[i][2]){p->error=-2;break;}
        for(unsigned j=0;j<4;++j)if(p->claims[at+j])p->error=-2;
        if(!p->error)title_pointer(p,at);
    }
    /* Preserve native-endian chain links until HSD_ArchiveLocateExtern binds
     * a provider (the normal archive loader initially binds imports to NULL). */
    static const uint32_t imports[][2]={{16536,41248},{16540,41252},{41248,UINT32_MAX},{41252,UINT32_MAX}};
    for(unsigned i=0;i<4&&!p->error;++i) {
        uint32_t at=imports[i][0];
        if(!p->external_slots || !p->external_slots[at/4] || p->relocs[at/4] ||
           title_u32(p,at)!=imports[i][1]){p->error=-2;break;}
        title_pointer(p,at);
    }
}
#endif
