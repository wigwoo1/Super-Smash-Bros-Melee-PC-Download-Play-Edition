#ifndef MELEE_PC_MUTECITY_DISPLAY_LIST_DIAGNOSTICS_H
#define MELEE_PC_MUTECITY_DISPLAY_LIST_DIAGNOSTICS_H
#include <pc/asset_regions.h>
#include <melee/gr/grdatfiles.h>
#include <melee/gr/types.h>
#include <stdlib.h>
/* Exact native allocation identity, metadata only; no graphics-state changes. */
static void MeleePc_MutecityDisplayListDiagnostic(HSD_PObj* pobj)
{
    static uint32_t seen_offsets[246], seen_sizes[246], count;
    const void *map_base = NULL, *map_owner = NULL;
    const void *draw_base = NULL, *draw_owner = NULL;
    uint32_t map_size = 0, draw_size = 0, offset, bytes, i;
    uint64_t hash = 14695981039346656037ULL;
    UnkArchiveStruct* archive;
    if (!getenv("MELEE_DIAGNOSTIC_MUTECITY_DISPLAY_LIST") || count >= 246) return;
    archive = grDatFiles_GetArchive();
    if (!archive || !archive->unk4 || !pobj || !pobj->display) return;
    if (!MeleePc_AssetRegionInfo(archive->unk4, &map_base, &map_owner, &map_size) ||
        !MeleePc_AssetRegionInfo(pobj->display, &draw_base, &draw_owner, &draw_size)) return;
    if (map_base != draw_base || map_owner != draw_owner || map_size != 1336064 ||
        draw_size != map_size || (uintptr_t)archive->unk4 - (uintptr_t)map_base != 2524) return;
    offset = (uint32_t)((uintptr_t)pobj->display - (uintptr_t)draw_base);
    bytes = (uint32_t)pobj->n_display << 5;
    if (!bytes || offset > draw_size || bytes > draw_size - offset) return;
    for (i = 0; i < count; ++i)
        if (seen_offsets[i] == offset && seen_sizes[i] == bytes) return;
    seen_offsets[count] = offset; seen_sizes[count++] = bytes;
    for (i = 0; i < bytes; ++i)
        hash = (hash ^ ((const unsigned char*)pobj->display)[i]) * 1099511628211ULL;
    OSReport("NATIVE_MUTECITY_DL allocation_identity=1 source_offset=%u bytes=%u hash=%llu\n",
             offset, bytes, (unsigned long long)hash);
}
#endif
