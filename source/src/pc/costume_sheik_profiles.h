#ifndef MELEE_PC_COSTUME_SHEIK_PROFILES_H
#define MELEE_PC_COSTUME_SHEIK_PROFILES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
typedef struct MeleePcSheikCostumeProfile {
    uint32_t length, body, relocations, joint, matanim;
    const char *joint_name, *matanim_name;
} MeleePcSheikCostumeProfile;
static uint32_t MeleePc_SheikCostumeBe32(const unsigned char* p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
/* Source-registered retail profiles; shared by conversion and packed-byte
 * preservation so admitting a costume cannot silently omit its original view.
 * This is an envelope gate, never a substitute for full typed graph validation. */
static const MeleePcSheikCostumeProfile* MeleePc_SheikCostumeProfile(
    const unsigned char* bytes, size_t length) {
    static const MeleePcSheikCostumeProfile profiles[] = {
        {380176,374576,1375,110024,276272,"PlySeak5K_Share_joint","PlySeak5K_Share_matanim_joint"},
        {380020,374416,1375,110024,276112,"PlySeak5KRe_Share_joint","PlySeak5KRe_Share_matanim_joint"},
        {380180,374576,1375,110024,276272,"PlySeak5KBu_Share_joint","PlySeak5KBu_Share_matanim_joint"},
        {380084,374480,1375,110024,276176,"PlySeak5KGr_Share_joint","PlySeak5KGr_Share_matanim_joint"},
        {380180,374576,1375,110024,276272,"PlySeak5KWh_Share_joint","PlySeak5KWh_Share_matanim_joint"},
    };
    if (!bytes || length < 32 || MeleePc_SheikCostumeBe32(bytes) != length ||
        MeleePc_SheikCostumeBe32(bytes+12) != 2 || MeleePc_SheikCostumeBe32(bytes+16)) return NULL;
    for (size_t i=0; i<sizeof(profiles)/sizeof(profiles[0]); ++i) {
        const MeleePcSheikCostumeProfile* p=&profiles[i];
        if (length!=p->length || MeleePc_SheikCostumeBe32(bytes+4)!=p->body ||
            MeleePc_SheikCostumeBe32(bytes+8)!=p->relocations) continue;
        size_t roots=32+(size_t)p->body+(size_t)p->relocations*4, strings=roots+16;
        if (strings>=length || MeleePc_SheikCostumeBe32(bytes+roots)!=p->joint ||
            MeleePc_SheikCostumeBe32(bytes+roots+8)!=p->matanim) continue;
        uint32_t a=MeleePc_SheikCostumeBe32(bytes+roots+4),b=MeleePc_SheikCostumeBe32(bytes+roots+12);
        if (a>=length-strings || b>=length-strings ||
            !memchr(bytes+strings+a,0,length-strings-a) ||
            !memchr(bytes+strings+b,0,length-strings-b)) continue;
        if (!strcmp((const char*)bytes+strings+a,p->joint_name) &&
            !strcmp((const char*)bytes+strings+b,p->matanim_name)) return p;
    }
    return NULL;
}
#endif
