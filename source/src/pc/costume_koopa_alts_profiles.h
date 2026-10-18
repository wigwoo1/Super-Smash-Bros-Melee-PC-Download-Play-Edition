#ifndef MELEE_PC_COSTUME_KOOPA_ALTS_PROFILES_H
#define MELEE_PC_COSTUME_KOOPA_ALTS_PROFILES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
typedef struct MeleePcKoopaAltCostumeProfile {
    uint32_t length, body, relocations, joint, matanim;
    const char *joint_name, *matanim_name;
} MeleePcKoopaAltCostumeProfile;
static uint32_t MeleePc_KoopaAltCostumeBe32(const unsigned char* p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
/* Source-registered retail profiles shared by dispatch and body conversion.
 * This envelope gate never substitutes for full typed graph validation.
 * Packed texture/geometry/animation payloads remain byte-oriented. */
static const MeleePcKoopaAltCostumeProfile* MeleePc_KoopaAltCostumeProfile(
    const unsigned char* bytes, size_t length) {
    static const MeleePcKoopaAltCostumeProfile profiles[] = {
        {493522,484624,2198,165640,460752,"PlyKoopa5KRe_Share_joint","PlyKoopa5KRe_Share_matanim_joint"},
        {493650,484752,2198,165640,460656,"PlyKoopa5KBu_Share_joint","PlyKoopa5KBu_Share_matanim_joint"},
        {493586,484688,2198,165640,460656,"PlyKoopa5KBk_Share_joint","PlyKoopa5KBk_Share_matanim_joint"},
    };
    if (!bytes || length < 32 || MeleePc_KoopaAltCostumeBe32(bytes) != length ||
        MeleePc_KoopaAltCostumeBe32(bytes+12) != 2 || MeleePc_KoopaAltCostumeBe32(bytes+16)) return NULL;
    for (size_t i=0; i<sizeof(profiles)/sizeof(profiles[0]); ++i) {
        const MeleePcKoopaAltCostumeProfile* p=&profiles[i];
        if (length!=p->length || MeleePc_KoopaAltCostumeBe32(bytes+4)!=p->body ||
            MeleePc_KoopaAltCostumeBe32(bytes+8)!=p->relocations) continue;
        size_t roots=32+(size_t)p->body+(size_t)p->relocations*4, strings=roots+16;
        if (strings>=length || MeleePc_KoopaAltCostumeBe32(bytes+roots)!=p->joint ||
            MeleePc_KoopaAltCostumeBe32(bytes+roots+8)!=p->matanim) continue;
        uint32_t a=MeleePc_KoopaAltCostumeBe32(bytes+roots+4),b=MeleePc_KoopaAltCostumeBe32(bytes+roots+12);
        if (a>=length-strings || b>=length-strings ||
            !memchr(bytes+strings+a,0,length-strings-a) ||
            !memchr(bytes+strings+b,0,length-strings-b)) continue;
        if (!strcmp((const char*)bytes+strings+a,p->joint_name) &&
            !strcmp((const char*)bytes+strings+b,p->matanim_name)) return p;
    }
    return NULL;
}
#endif
