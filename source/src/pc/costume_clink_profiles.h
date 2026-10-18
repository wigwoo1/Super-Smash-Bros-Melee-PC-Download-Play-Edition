#ifndef MELEE_PC_COSTUME_CLINK_PROFILES_H
#define MELEE_PC_COSTUME_CLINK_PROFILES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
typedef struct MeleePcCLinkCostumeProfile {
    uint32_t length, body, relocations, joint, matanim;
    const char *joint_name, *matanim_name;
} MeleePcCLinkCostumeProfile;
static uint32_t MeleePc_CLinkCostumeBe32(const unsigned char* p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
/* Source-registered retail profiles shared by dispatch and body conversion.
 * This envelope gate never substitutes for full typed graph validation.
 * Packed texture/geometry/animation payloads remain byte-oriented. */
static const MeleePcCLinkCostumeProfile* MeleePc_CLinkCostumeProfile(
    const unsigned char* bytes, size_t length) {
    static const MeleePcCLinkCostumeProfile profiles[] = {
        {452046,445552,1598,118632,275312,"PlyClink5K_Share_joint","PlyClink5K_Share_matanim_joint"},
        {452050,445552,1598,118632,275312,"PlyClink5KRe_Share_joint","PlyClink5KRe_Share_matanim_joint"},
        {452050,445552,1598,118632,275312,"PlyClink5KBu_Share_joint","PlyClink5KBu_Share_matanim_joint"},
        {452050,445552,1598,118632,275312,"PlyClink5KWh_Share_joint","PlyClink5KWh_Share_matanim_joint"},
        {452050,445552,1598,118632,275312,"PlyClink5KBk_Share_joint","PlyClink5KBk_Share_matanim_joint"},
    };
    if (!bytes || length < 32 || MeleePc_CLinkCostumeBe32(bytes) != length ||
        MeleePc_CLinkCostumeBe32(bytes+12) != 2 || MeleePc_CLinkCostumeBe32(bytes+16)) return NULL;
    for (size_t i=0; i<sizeof(profiles)/sizeof(profiles[0]); ++i) {
        const MeleePcCLinkCostumeProfile* p=&profiles[i];
        if (length!=p->length || MeleePc_CLinkCostumeBe32(bytes+4)!=p->body ||
            MeleePc_CLinkCostumeBe32(bytes+8)!=p->relocations) continue;
        size_t roots=32+(size_t)p->body+(size_t)p->relocations*4, strings=roots+16;
        if (strings>=length || MeleePc_CLinkCostumeBe32(bytes+roots)!=p->joint ||
            MeleePc_CLinkCostumeBe32(bytes+roots+8)!=p->matanim) continue;
        uint32_t a=MeleePc_CLinkCostumeBe32(bytes+roots+4),b=MeleePc_CLinkCostumeBe32(bytes+roots+12);
        if (a>=length-strings || b>=length-strings ||
            !memchr(bytes+strings+a,0,length-strings-a) ||
            !memchr(bytes+strings+b,0,length-strings-b)) continue;
        if (!strcmp((const char*)bytes+strings+a,p->joint_name) &&
            !strcmp((const char*)bytes+strings+b,p->matanim_name)) return p;
    }
    return NULL;
}
#endif
