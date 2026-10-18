#ifndef MELEE_PC_COSTUME_PICHU_PROFILES_H
#define MELEE_PC_COSTUME_PICHU_PROFILES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
typedef struct MeleePcPichuCostumeProfile {
    uint32_t length, body, relocations, joint, matanim;
    const char *joint_name, *matanim_name;
} MeleePcPichuCostumeProfile;
static uint32_t MeleePc_PichuCostumeBe32(const unsigned char* p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
/* Source-registered retail profiles shared by dispatch and body conversion.
 * This envelope gate never substitutes for full typed graph validation.
 * Packed texture/geometry/animation payloads remain byte-oriented. */
static const MeleePcPichuCostumeProfile* MeleePc_PichuCostumeProfile(
    const unsigned char* bytes, size_t length) {
    static const MeleePcPichuCostumeProfile profiles[] = {
        {168974,165824,762,61136,116096,"PlyPichu5K_Share_joint","PlyPichu5K_Share_matanim_joint"},
        {206334,202784,861,81648,153072,"PlyPichu5KRe_Share_joint","PlyPichu5KRe_Share_matanim_joint"},
        {230146,226048,998,87312,176320,"PlyPichu5KBu_Share_joint","PlyPichu5KBu_Share_matanim_joint"},
        {316154,311840,1052,88080,262112,"PlyPichu5KGr_Share_joint","PlyPichu5KGr_Share_matanim_joint"},
    };
    if (!bytes || length < 32 || MeleePc_PichuCostumeBe32(bytes) != length ||
        MeleePc_PichuCostumeBe32(bytes+12) != 2 || MeleePc_PichuCostumeBe32(bytes+16)) return NULL;
    for (size_t i=0; i<sizeof(profiles)/sizeof(profiles[0]); ++i) {
        const MeleePcPichuCostumeProfile* p=&profiles[i];
        if (length!=p->length || MeleePc_PichuCostumeBe32(bytes+4)!=p->body ||
            MeleePc_PichuCostumeBe32(bytes+8)!=p->relocations) continue;
        size_t roots=32+(size_t)p->body+(size_t)p->relocations*4, strings=roots+16;
        if (strings>=length || MeleePc_PichuCostumeBe32(bytes+roots)!=p->joint ||
            MeleePc_PichuCostumeBe32(bytes+roots+8)!=p->matanim) continue;
        uint32_t a=MeleePc_PichuCostumeBe32(bytes+roots+4),b=MeleePc_PichuCostumeBe32(bytes+roots+12);
        if (a>=length-strings || b>=length-strings ||
            !memchr(bytes+strings+a,0,length-strings-a) ||
            !memchr(bytes+strings+b,0,length-strings-b)) continue;
        if (!strcmp((const char*)bytes+strings+a,p->joint_name) &&
            !strcmp((const char*)bytes+strings+b,p->matanim_name)) return p;
    }
    return NULL;
}
#endif
