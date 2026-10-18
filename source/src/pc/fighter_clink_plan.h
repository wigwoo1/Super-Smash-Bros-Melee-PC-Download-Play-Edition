#ifndef MELEE_PC_FIGHTER_CLINK_PLAN_H
#define MELEE_PC_FIGHTER_CLINK_PLAN_H
#include "fighter_link_plan.h"
#include "fighter_wait_table.h"
#ifdef MELEE_CLINK_PLAN_TRACE
#define CLINK_CHECK(label) do { if(p->error){fprintf(stderr,"CLINK_PLAN phase=%s error=%d\n",label,p->error);return;} } while(0)
#else
#define CLINK_CHECK(label) do { if(p->error)return; } while(0)
#endif
/* Source-audited Young Link components. Not registered with production dispatch:
 * complete archive graph ownership and independent byte-oracle gates follow. */
static void MeleePc_FighterCLinkPlan(MeleePc_TitlePlan* p,uint32_t root) {
    if(p->error)return;
    if(p->size!=271904 || root!=41680 || !fighter_sound_span(p,root,96)){p->error=-2;return;}
    uint32_t fields[24];
    for(unsigned i=0;i<24;++i) {
        if(!fighter_sound_pointer(p,root+i*4,&fields[i]) ||
           ((i==10)?fields[i]!=UINT32_MAX:fields[i]==UINT32_MAX)){p->error=-1;return;}
        title_pointer(p,root+i*4);
    }
    MeleePc_FighterAttributesPlan(p,fields[0],0);CLINK_CHECK("common attributes");
    /* ftCl_OnLoad calls ftLk_OnLoadForCLink, which copies ftLk_DatAttrs. */
    MeleePc_FighterLinkAttributesPlan(p,fields[1]);CLINK_CHECK("shared special attributes");
    MeleePc_FighterModelPartsPlan(p,fields[2],5);CLINK_CHECK("model parts at body offset zero");
    const uint32_t tables[]={fields[3],fields[5]},counts[]={314,14};
    MeleePc_FighterAnimationTablesPlan(p,tables,counts);CLINK_CHECK("actions");
    MeleePc_FighterAnimationMapPlan(p,fields[4],314);
    MeleePc_FighterAnimationMapPlan(p,fields[6],14);CLINK_CHECK("animation maps");
    const uint32_t parts[]={4,4,3};
    MeleePc_FighterPartAnimationsPlan(p,fields[7],3,parts);CLINK_CHECK("part animations");
    MeleePc_FighterShieldDescriptorPlan(p,fields[8]);
    MeleePc_FighterWaitTablePlan(p,fields[9],314);
    MeleePc_FighterLinkDynamicsPlan(p,fields[11]);CLINK_CHECK("shield/wait/dynamics");
    MeleePc_FighterHurtboxesPlan(p,fields[12]);
    for(unsigned i=0;i<5;++i)MeleePc_FighterScalarChildPlan(p,fields[13+i],i);
    MeleePc_FighterSoundTablesPlan(p,fields[19]);
    MeleePc_FighterScalarChildPlan(p,fields[20],5);
    MeleePc_FighterPartIdsPlan(p,fields[21]);
    MeleePc_FighterScalarChildPlan(p,fields[22],6);CLINK_CHECK("scalar children");
    title_enqueue(p,title_u32(p,fields[8]),TITLE_HSD_Joint);
    title_enqueue(p,fields[23],TITLE_HSD_Joint);
    if(!fighter_sound_span(p,fields[18],28)){p->error=-1;return;}
    /* The first five pairs share state/attribute readers. Young Link hookshot
     * omits Link's OnLoad initializer. Select layouts only, retain kinds. */
    const unsigned layout_kinds[]={58,60,62,64,76,80},states[]={4,3,0,1,6,2};
    for(unsigned i=0;i<7&&!p->error;++i) {
        uint32_t item;
        if(!fighter_sound_pointer(p,fields[18]+i*4,&item) || item==UINT32_MAX){p->error=-1;return;}
        title_pointer(p,fields[18]+i*4);
        if(i<6)MeleePc_ItemArticlePlan(p,item,layout_kinds[i],states[i]);
        else title_enqueue(p,item,TITLE_HSD_Joint);
    }
    CLINK_CHECK("items");
}
#undef CLINK_CHECK
#endif
