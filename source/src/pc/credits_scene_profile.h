#ifndef MELEE_PC_CREDITS_SCENE_PROFILE_H
#define MELEE_PC_CREDITS_SCENE_PROFILE_H
/* gmstaffroll consumes a SceneDesc and exactly ten DynamicModelDesc pointers.
 * The original export additionally retains one [model,NULL] directory beside
 * each model; all ten are checked and typed, not treated as unknown words. */
static int MeleePc_IsCreditsSceneProfile(unsigned char* bytes,uint32_t body,
    uint32_t relocs,uint32_t publics,size_t roots,size_t strings)
{
    return MeleePc_DatBe32(bytes)==2253035 && body==2216668 && relocs==9063 &&
        publics==2 && MeleePc_DatBe32(bytes+16)==0 &&
        MeleePc_DatBe32(bytes+roots)==2216628 &&
        MeleePc_DatBe32(bytes+roots+8)==1932564 &&
        !strcmp((char*)bytes+strings+MeleePc_DatBe32(bytes+roots+4),
                "ScGamRegStaffrollNames_scene_modelset") &&
        !strcmp((char*)bytes+strings+MeleePc_DatBe32(bytes+roots+12),
                "ScGamRegStaffroll_scene_data");
}
#endif
