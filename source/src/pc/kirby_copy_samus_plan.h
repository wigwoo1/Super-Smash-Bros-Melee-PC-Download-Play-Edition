#ifndef MELEE_PC_KIRBY_COPY_SAMUS_PLAN_H
#define MELEE_PC_KIRBY_COPY_SAMUS_PLAN_H
#include "fighter_model_parts.h"
#include "fighter_dynamics.h"
#include "item_article_plan.h"

/* Source ftKbCopyHat_Item; hat initialization uses the row-0 model. */
static void MeleePc_KirbyCopySamusPlan(MeleePc_TitlePlan* p, uint32_t root)
{
    if (p->error) return;
    if (p->size != 52696 || root != 640 || !title_range(p, root, 16)) {
        p->error = -2;
        return;
    }
    if (!fighter_sound_scalar(p, root + 4) ||
        title_u32(p, root + 4) != 1) {
        p->error = -2;
        return;
    }
    title_claim(p, root + 4, 4, 4);
    title_enqueue(p, title_pointer(p, root), TITLE_HSD_Joint);

    uint32_t vis = title_pointer(p, root + 8);
    if (vis == UINT32_MAX || !title_range(p, vis, 16)) {
        p->error = -1;
        return;
    }
    for (unsigned v = 0; v < 4 && !p->error; ++v) {
        uint32_t lookup = title_pointer(p, vis + v * 4);
        if (lookup == UINT32_MAX) continue;
        if (!title_range(p, lookup, 8) || !fighter_sound_scalar(p, lookup)) {
            p->error = -1;
            return;
        }
        uint32_t count = title_u32(p, lookup);
        uint32_t groups = title_pointer(p, lookup + 4);
        title_claim(p, lookup, 4, 4);
        if (count > p->size / 8 || (count && groups == UINT32_MAX) ||
            (groups != UINT32_MAX &&
             !title_range(p, groups, count * 8))) {
            p->error = -1;
            return;
        }
        for (uint32_t i = 0; i < count && !p->error; ++i) {
            uint32_t at = groups + i * 8;
            if (!fighter_sound_scalar(p, at)) {
                p->error = -1;
                return;
            }
            uint32_t n = title_u32(p, at);
            uint32_t indices = title_pointer(p, at + 4);
            title_claim(p, at, 4, 4);
            if (n > p->size || (n && indices == UINT32_MAX) ||
                (indices != UINT32_MAX &&
                 !fighter_parts_payload(p, indices, n, 1))) {
                p->error = -1;
                return;
            }
        }
    }

    uint32_t article = title_pointer(p, root + 12);
    if (article == UINT32_MAX) {
        p->error = -1;
        return;
    }
    /* Source item kind 151 is Kirby's copied Samus charge shot. Its state
     * table has nine indexed animation rows; imported mat/shape roots are
     * validated external chains and must not be walked as local nodes. */
    MeleePc_ItemArticlePlan(p, article, 151, 9);
    /* The detached tail has no source-declared HSD pointee schema. The DAT
     * relocation table does establish that these exact aligned words are
     * pointer slots; the source archive loader adds the data base to each
     * listed slot. Whitelist the original offsets/targets, require each word
     * to be unclaimed by the typed/packed graph and not an external import,
     * then use the normal bounds-checked relocation path without enqueueing
     * an assumed pointee type. */
    static const uint32_t tail[][2] = {
        { 52664, 52504 }, { 52672, 41128 },
        { 52676, 52664 }, { 52688, 52672 },
    };
    for (unsigned i = 0; i < sizeof(tail) / sizeof(tail[0]) && !p->error;
         ++i) {
        uint32_t at = tail[i][0];
        if (!title_range(p, at, 4) || (at & 3) ||
            title_u32(p, at) != tail[i][1] || !p->relocs[at / 4] ||
            (p->external_slots && p->external_slots[at / 4]) ||
            p->claims[at] || p->claims[at + 1] || p->claims[at + 2] ||
            p->claims[at + 3]) {
            p->error = -2;
            return;
        }
        if (title_pointer(p, at) == UINT32_MAX) {
            p->error = -1;
            return;
        }
    }
}
#endif
