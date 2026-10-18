#ifndef MELEE_PC_FIGHTER_GANON_PLAN_H
#define MELEE_PC_FIGHTER_GANON_PLAN_H
#include "fighter_attributes.h"
#include "fighter_captain_attributes.h"
#include "fighter_animation_tables.h"
#include "fighter_animation_maps.h"
#include "fighter_part_animations.h"
#include "fighter_model_parts.h"
#include "fighter_scalar_children.h"
#include "fighter_hurtboxes.h"
#include "fighter_wait_table.h"
#include "fighter_dynamics.h"
#include "fighter_auxiliary_fields.h"
#ifdef MELEE_GANON_PLAN_TRACE
#define GANON_CHECK(label) do { if(p->error){fprintf(stderr,"GANON_PLAN phase=%s error=%d\n",label,p->error);return;} } while(0)
#else
#define GANON_CHECK(label) do { if(p->error)return; } while(0)
#endif
/* Exact source-counted main fighter profile. Caller commits only after full
 * graph traversal and local relocation ownership checks. */
static void MeleePc_FighterGanonPlan(MeleePc_TitlePlan* p,uint32_t root) {
    uint32_t targets[24];
    if(p->error)return;
    if(p->size!=183872 || root!=38312 || !fighter_sound_span(p,root,96)){p->error=-2;return;}
    for(unsigned i=0;i<24;++i) {
        if(!fighter_sound_pointer(p,root+i*4,&targets[i]) ||
           ((i==10 || i==18)?targets[i]!=UINT32_MAX:targets[i]==UINT32_MAX)){p->error=-1;return;}
        title_pointer(p,root+i*4);
    }
    MeleePc_FighterAttributesPlan(p,targets[0],0);GANON_CHECK("common attributes");
    /* ftGn_OnLoad/LoadSpecialAttrs explicitly use ftCaptain_DatAttrs. */
    MeleePc_FighterCaptainAttributesPlan(p,targets[1]);GANON_CHECK("special attributes");
    MeleePc_FighterModelPartsPlan(p,targets[2],5);GANON_CHECK("model parts");
    const uint32_t tables[]={targets[3],targets[5]},counts[]={318,14};
    MeleePc_FighterAnimationTablesPlan(p,tables,counts);GANON_CHECK("action tables");
    const uint32_t unused_roots[]={18356,18368};
    for(unsigned i=0;i<2;++i)if(!title_range(p,unused_roots[i],8) || title_u32(p,unused_roots[i])!=0x1c000000 || title_u32(p,unused_roots[i]+4)!=18256 || !p->relocs[(unused_roots[i]+4)/4] || p->claims[18256]!=1){p->error=-1;return;}
    MeleePc_FighterActionStreamsPlan(p,unused_roots,2);GANON_CHECK("unused goto records");
    MeleePc_FighterAnimationMapPlan(p,targets[4],318);
    MeleePc_FighterAnimationMapPlan(p,targets[6],14);GANON_CHECK("animation maps");
    /* ftdynamics.c directly indexes x10 by the second animation-map byte. */
    for(unsigned table=0;table<2;++table)for(unsigned i=0;i<counts[table];++i)
        if(p->bytes[32+targets[table?6:4]+i*2+1]>=2){p->error=-1;return;}
    const uint32_t parent_roots[]={44808,47488,50268,53048,55728,58408,61288,64168,65948,67728,69508,71572,73636,75700,77764,79828,81892,83956,86020,88084,90148,92212,94276};
    MeleePc_FighterAnimationGraphsPlan(p,parent_roots,23);GANON_CHECK("complete animation parents");
    const uint32_t part_counts[]={4,4,3,6,6};
    MeleePc_FighterPartAnimationsPlan(p,targets[7],5,part_counts);GANON_CHECK("part animations");
    MeleePc_FighterShieldDescriptorPlan(p,targets[8]);GANON_CHECK("shield descriptor");
    MeleePc_FighterWaitTablePlan(p,targets[9],318);GANON_CHECK("wait table");
    MeleePc_FighterDynamicsPlan(p,targets[11],2);GANON_CHECK("cape dynamics");
    MeleePc_FighterHurtboxesPlan(p,targets[12]);GANON_CHECK("hurtboxes");
    for(unsigned i=0;i<5;++i)MeleePc_FighterScalarChildPlan(p,targets[13+i],i);
    GANON_CHECK("scalar children");
    MeleePc_FighterSoundTablesPlan(p,targets[19]);GANON_CHECK("sound tables");
    MeleePc_FighterScalarChildPlan(p,targets[20],5);
    MeleePc_FighterPartIdsPlan(p,targets[21]);
    MeleePc_FighterScalarChildPlan(p,targets[22],6);GANON_CHECK("auxiliary fields");
    title_enqueue(p,title_u32(p,targets[8]),TITLE_HSD_Joint);
    title_enqueue(p,targets[23],TITLE_HSD_Joint);
}
#undef GANON_CHECK
#endif
