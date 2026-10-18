#ifndef MELEE_PC_ITEM_PEACH_ATTRIBUTES_H
#define MELEE_PC_ITEM_PEACH_ATTRIBUTES_H
#include "title_archive.h"
/* Staged only: source itPeachTurnipAttributes and itPeachToadSporeAttributes. */
static void MeleePc_ItemPeachAttributesPlan(MeleePc_TitlePlan* p,
                                          uint32_t root, uint32_t kind)
{
    unsigned size = kind == 99 ? 72 : kind == 111 ? 16 : 0;
    if (p->error) return;
    if (!size || (root & 3) || !title_range(p, root, size)) {
        p->error = -1; return;
    }
    for (unsigned off = 0; off < size; off += 4) {
        uint32_t at = root + off;
        if (p->relocs[at / 4] ||
            (p->external_slots && p->external_slots[at / 4])) {
            p->error = -1; return;
        }
        if ((kind == 111 || off == 0) &&
            (title_u32(p, at) & 0x7f800000U) == 0x7f800000U) {
            p->error = -1; return;
        }
    }
    if (kind == 99 && title_u32(p, root + 4) != 8) {
        p->error = -1; return;
    }
    title_claim(p, root, size, 4);
}
#endif
