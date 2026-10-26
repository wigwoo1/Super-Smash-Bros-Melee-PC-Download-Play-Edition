#ifndef MELEE_PC_FIGHTER_MASTERHAND_ARCHIVE_H
#define MELEE_PC_FIGHTER_MASTERHAND_ARCHIVE_H
#include "fighter_attributes.h"
#include "fighter_animation_tables.h"
#include "fighter_animation_maps.h"
#include "fighter_scalar_children.h"
#include "fighter_hurtboxes.h"
#include "fighter_empty_dynamics.h"
#include "fighter_auxiliary_fields.h"
#include "fighter_model_parts.h"
#include "item_article_plan.h"
#ifdef MELEE_MASTERHAND_PLAN_TRACE
#define MH_CHECK(label) do {if(p->error){fprintf(stderr,"MASTERHAND_PLAN phase=%s error=%d\n",label,p->error);return;}} while(0)
#else
#define MH_CHECK(label) do {if(p->error)return;} while(0)
#endif
static void MeleePc_MasterHandScalars(MeleePc_TitlePlan* p,uint32_t at,
                                    uint32_t size,const uint16_t* ints,unsigned n){
    if(p->error || !fighter_sound_span(p,at,size)){p->error=-1;return;}
    for(uint32_t off=0;off<size;off+=4){
        unsigned integer=0;
        for(unsigned j=0;j<n;++j)if(off==ints[j])integer=1;
        if(!fighter_sound_scalar(p,at+off) ||
           (!integer && (title_u32(p,at+off)&0x7f800000U)==0x7f800000U)){
            p->error=-1;return;
        }
    }
    title_claim(p,at,size,4);
}
/* ftMh_Init_OnLoad registers Laser/Bullet only. The serialized third Article
 * is the shared Crazy Hand bomb layout (two states); conversion does not
 * register it or expose either boss as a selectable fighter. */
static void MeleePc_MasterHandArticle(MeleePc_TitlePlan* p,uint32_t at,unsigned index){
    static const unsigned words[]={2,1,3},states[]={1,2,2};
    if(index>=3 || !fighter_sound_span(p,at,24)){p->error=-1;return;}
    uint32_t t[6];
    for(unsigned i=0;i<6;++i){
        if(!fighter_sound_pointer(p,at+i*4,&t[i])){p->error=-1;return;}
        title_pointer(p,at+i*4);
    }
    if(t[0]==UINT32_MAX || t[1]==UINT32_MAX || t[2]!=UINT32_MAX ||
       t[3]==UINT32_MAX || t[4]==UINT32_MAX || t[5]!=UINT32_MAX){p->error=-1;return;}
    MeleePc_ItemArticleAttributesPlan(p,t[0]);
    MeleePc_MasterHandScalars(p,t[1],words[index]*4,NULL,0);
    MeleePc_ItemStateAnimationsPlan(p,t[3],states[index]);
    for(unsigned i=0;i<states[index] && !p->error;++i){
        uint32_t command=title_pointer(p,t[3]+i*16+12);
        MeleePc_ItemCommandPlan(p,&command,1);
    }
    MeleePc_ItemModelDescriptorPlan(p,t[4]);
}
static void MeleePc_FighterHandPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned crazy){
    static const unsigned null_mask=(1u<<9)|(1u<<10)|(1u<<22)|(1u<<23);
    uint32_t t[24];
    if(p->error)return;
    if(crazy>1 || p->size!=(crazy?53336:52280) || root!=(crazy?20132:19104) || !fighter_sound_span(p,root,96)){p->error=-2;return;}
    for(unsigned i=0;i<24;++i){
        if(!fighter_sound_pointer(p,root+i*4,&t[i]) ||
           ((null_mask>>i)&1 ? t[i]!=UINT32_MAX : t[i]==UINT32_MAX)){p->error=-1;return;}
        title_pointer(p,root+i*4);
    }
    MeleePc_FighterAttributesPlan(p,t[0],0);MH_CHECK("common attributes");
    /* ftMasterHand_SpecialAttrs: 0x17C bytes, s32 or finite f32/Vec components. */
    static const uint16_t ints[]={0,4,8,12,16,20,24,28,32,36,108,112,116,124,132,144,148,160,176,180,236,240,324,328,352,356,360,364,368,372};
    /* ftCrazyHand_DatAttrs is a distinct 0x144-byte scalar structure. */
    static const uint16_t crazy_ints[]={0,4,8,12,72,84,88,100,116,120,240,244,248,256,292,296};
    MeleePc_MasterHandScalars(p,t[1],crazy?0x144:0x17c,crazy?crazy_ints:ints,
        crazy?sizeof(crazy_ints)/sizeof(*crazy_ints):sizeof(ints)/sizeof(*ints));MH_CHECK("special attributes");
    /* Offset zero is a relocated, valid model-part record, not NULL. */
    MeleePc_FighterModelPartsPlan(p,t[2],1);MH_CHECK("model parts");
    const uint32_t tables[]={t[3],t[5]},counts[]={crazy?344:345,14};
    MeleePc_FighterAnimationTablesPlan(p,tables,counts);MH_CHECK("action tables");
    MeleePc_FighterAnimationMapPlan(p,t[4],counts[0]);
    MeleePc_FighterAnimationMapPlan(p,t[6],14);MH_CHECK("animation maps");
    /* Source optional part-animation/shield directories contain a NULL first
     * pointer, with no following descriptors or shield scale to consume. */
    if(title_pointer(p,t[7])!=UINT32_MAX || title_pointer(p,t[8])!=UINT32_MAX){p->error=-1;return;}
    MeleePc_FighterEmptyDynamicsPlan(p,t[11]);MH_CHECK("empty dynamics");
    MeleePc_FighterHurtboxesPlan(p,t[12]);MH_CHECK("hurtboxes");
    for(unsigned i=0;i<5;++i)MeleePc_FighterScalarChildPlan(p,t[13+i],i);
    MeleePc_FighterSoundTablesPlan(p,t[19]);MH_CHECK("sound and scalar children");
    MeleePc_FighterScalarChildPlan(p,t[20],5);
    MeleePc_FighterPartIdsPlan(p,t[21]);MH_CHECK("part IDs");
    if(!fighter_sound_span(p,t[18],12)){p->error=-1;return;}
    for(unsigned i=0;i<3 && !p->error;++i){
        uint32_t article=title_pointer(p,t[18]+i*4);
        if(article==UINT32_MAX){p->error=-1;return;}
        MeleePc_MasterHandArticle(p,article,i);
    }
    MH_CHECK("articles");
    /* Unexported DynamicModelDesc directories retain the same typed model /
     * animation layout used by other fighter archives. Decode without attaching
     * these auxiliary model lists to live game objects. */
    title_array(p,crazy?38928:37872,TITLE_DynamicModelDesc,0,1);
    title_array(p,crazy?43504:42448,TITLE_DynamicModelDesc,0,1);
    title_array(p,crazy?53328:52272,TITLE_DynamicModelDesc,0,1);
}
static void MeleePc_FighterMasterHandPlan(MeleePc_TitlePlan* p,uint32_t root){MeleePc_FighterHandPlan(p,root,0);}
static void MeleePc_FighterCrazyHandPlan(MeleePc_TitlePlan* p,uint32_t root){MeleePc_FighterHandPlan(p,root,1);}
static int MeleePc_MasterHandBody(unsigned char* bytes,size_t length,
                                uint32_t body,uint32_t relocs,uint32_t root){
    unsigned main=length==53509 && body==52280 && relocs==293 && root==19104;
    unsigned costume=length==313338 && body==310976 && relocs==574 && root==41296;
    unsigned crazy_main=length==54572 && body==53336 && relocs==295 && root==20132;
    unsigned crazy_costume=length==313513 && body==311104 && relocs==586 && root==41424;
    if(!main && !costume && !crazy_main && !crazy_costume)return -2;
    MeleePc_TitlePlan p={0};p.bytes=bytes;p.size=body;p.capacity=relocs+body/24+2;
    p.claims=calloc(body,1);p.relocs=calloc((body+3)/4,1);p.seen=calloc((body+3)/4,1);
    p.nodes=malloc(p.capacity*sizeof(*p.nodes));
    if(!p.claims || !p.relocs || !p.seen || !p.nodes)p.error=-1;
    for(uint32_t i=0;i<relocs && !p.error;++i){
        uint32_t at=MeleePc_DatBe32(bytes+32+body+i*4);
        if((at&3) || !title_range(&p,at,4) || p.relocs[at/4])p.error=-1;
        else p.relocs[at/4]=1;
    }
    if(!p.error){
        if(main)MeleePc_FighterMasterHandPlan(&p,root);
        else if(crazy_main)MeleePc_FighterCrazyHandPlan(&p,root);
        else title_enqueue(&p,root,TITLE_HSD_Joint);
    }
    int result=p.error?p.error:title_finish_plan(&p,0);
    free(p.nodes);free(p.seen);free(p.relocs);free(p.claims);return result;
}
#undef MH_CHECK
#endif
