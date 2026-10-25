#ifndef MELEE_PC_TROPHY_MODEL_PROFILE_H
#define MELEE_PC_TROPHY_MODEL_PROFILE_H
/* gm_19EF loads this ToyModelFile symbol as HSD_Joint. The export also
 * retains a no-animation model descriptor and a terminated descriptor list.
 * StaticModelDesc and DynamicModelDesc (sc/types.h) agree for null animations.
 * Admit only the audited envelope; never infer ownership from spare relocations. */
static int MeleePc_IsDrMarioTrophyProfile(
    unsigned char* bytes, uint32_t body, uint32_t relocs, uint32_t publics,
    size_t roots, size_t strings)
{
    return MeleePc_DatBe32(bytes) == 451059 && body == 449976 &&
        relocs == 254 && publics == 1 && MeleePc_DatBe32(bytes + 16) == 0 &&
        MeleePc_DatBe32(bytes + roots) == 84936 &&
        !strcmp((char*) bytes + strings + MeleePc_DatBe32(bytes + roots + 4),
                "ToyDrmarioModel_TopN_joint");
}
#endif
