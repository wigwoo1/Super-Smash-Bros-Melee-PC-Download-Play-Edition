#ifndef MELEE_PC_GAME_OVER_SCENE_PROFILE_H
#define MELEE_PC_GAME_OVER_SCENE_PROFILE_H
/* gm_19EF.c and gmregclear.c consume these original SceneDesc archives. Admit only the
 * source-audited envelopes, then use the existing typed graph/claim traversal.
 * Model, animation, camera, light and fog pointers retain their source types. */
static int MeleePc_IsGameOverSceneProfile(
    unsigned char* bytes, uint32_t body, uint32_t relocs, uint32_t publics,
    size_t roots, size_t strings)
{
    static const struct {
        uint32_t length, body, relocs, root;
        const char* symbol;
    } profiles[] = {
        {87861, 83672, 1031, 83656, "ScGamRegGover_scene_data"},
        {23841, 22852, 231, 22836, "ScGamRegGover_scene_data"},
        {1417, 1164, 47, 1148, "ScGamRegGover_scene_data"},
        {22831, 22604, 44, 22588, "standScene"},
        {185685, 183304, 579, 183288, "ScGamRegClear_scene_data"},
    };
    if (publics != 1 || MeleePc_DatBe32(bytes + 16) != 0) return 0;
    for (unsigned i = 0; i < sizeof(profiles) / sizeof(profiles[0]); ++i) {
        if (MeleePc_DatBe32(bytes) == profiles[i].length &&
            body == profiles[i].body && relocs == profiles[i].relocs &&
            MeleePc_DatBe32(bytes + roots) == profiles[i].root &&
            !strcmp((char*) bytes + strings +
                        MeleePc_DatBe32(bytes + roots + 4),
                    profiles[i].symbol))
            return 1;
    }
    return 0;
}
#endif
