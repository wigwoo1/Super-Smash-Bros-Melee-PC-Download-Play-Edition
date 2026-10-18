#ifndef MELEE_PC_EFFECT_PALETTE_ALIAS_H
#define MELEE_PC_EFFECT_PALETTE_ALIAS_H

#include <stdint.h>
#include <string.h>

#include <pc/asset_regions.h>
#include <sysdolphin/baselib/psstructs.h>

/* Kirby's Samus-copy effect reuses Samus's CI8 image, but its serialized
 * palette word is a non-relocated address outside the archive. The archive
 * adapter replaces only that exact profile's word with NULL. Resolve it only
 * after both banks have been natively located, and only if the image bytes
 * prove that the source palette belongs to the same texture. */
static int MeleePc_AliasKirbySamusPalette(HSD_PSTexGroup* dst,
                                          const HSD_PSTexGroup* src)
{
    uint32_t dst_image_bytes, src_image_bytes, src_palette_bytes;

    if (!dst || !src || dst == src || dst->num != 1 || src->num != 1 ||
        dst->fmt != 9 || src->fmt != 9 || dst->tlutfmt != src->tlutfmt ||
        dst->width != 64 || src->width != 64 || dst->height != 64 ||
        src->height != 64 || dst->palnum != 0 || dst->palflag != 0 ||
        src->palnum != 0 || (src->palflag & 1) == 0 || !dst->texTable[0] ||
        !src->texTable[0] || !src->texTable[1] || dst->texTable[1]) {
        return 0;
    }
    if (!MeleePc_AssetBytesRemaining(dst->texTable[0], &dst_image_bytes) ||
        !MeleePc_AssetBytesRemaining(src->texTable[0], &src_image_bytes) ||
        !MeleePc_AssetBytesRemaining(src->texTable[1], &src_palette_bytes) ||
        dst_image_bytes < 4096 || src_image_bytes < 4096 ||
        src_palette_bytes < 512 ||
        memcmp(dst->texTable[0], src->texTable[0], 4096) != 0) {
        return 0;
    }

    dst->texTable[1] = src->texTable[1];
    return 1;
}

#endif
