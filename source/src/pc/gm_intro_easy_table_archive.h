#ifndef MELEE_PC_GM_INTRO_EASY_TABLE_ARCHIVE_H
#define MELEE_PC_GM_INTRO_EASY_TABLE_ARCHIVE_H

/* GmIntEz.dat contains the source-defined lbl_804D6604_t numeric table.
 * Its 0x9B8-byte body has no pointers or padding outside the explicitly
 * skipped structure padding below. Keep the profile exact: this is not a
 * general-purpose endian conversion for unknown HSD archives. */
static int MeleePc_GmIntroEasyTableBody(unsigned char* bytes, size_t length,
                                        uint32_t body, uint32_t relocs,
                                        uint32_t root)
{
    if (bytes == NULL || length != 2545 || body != 0x9B8 || relocs != 0 ||
        root != 0) {
        return -2;
    }

    /* ClassicSlotVals[9] at 0x000. */
    for (uint32_t at = 0x000; at < 0x06C; at += 4) {
        MeleePc_DatSwap32(bytes + 32 + at);
    }
    /* ClassicCharLayout[28]: five floats, then eight bytes of padding. */
    for (uint32_t base = 0x06C; base < 0x37C; base += 0x1C) {
        for (uint32_t i = 0; i < 5; ++i) {
            MeleePc_DatSwap32(bytes + 32 + base + i * 4);
        }
    }
    /* ClassicTeamEntry[25]: three floats, then eight bytes of padding. */
    for (uint32_t base = 0x37C; base < 0x570; base += 0x14) {
        for (uint32_t i = 0; i < 3; ++i) {
            MeleePc_DatSwap32(bytes + 32 + base + i * 4);
        }
    }
    /* ClassicSplashRow[3] contains twelve floats per row. */
    for (uint32_t at = 0x57C; at < 0x60C; at += 4) {
        MeleePc_DatSwap32(bytes + 32 + at);
    }
    /* Slot tables x630/x654/x678. */
    for (uint32_t at = 0x630; at < 0x6A8; at += 4) {
        MeleePc_DatSwap32(bytes + 32 + at);
    }
    /* Final ClassicCharLayout[28]. */
    for (uint32_t base = 0x6A8; base < 0x9B8; base += 0x1C) {
        for (uint32_t i = 0; i < 5; ++i) {
            MeleePc_DatSwap32(bytes + 32 + base + i * 4);
        }
    }
    return 0;
}

#endif
