#ifndef MELEE_PC_MUTECITY_PLATFORM_ISOLATION_H
#define MELEE_PC_MUTECITY_PLATFORM_ISOLATION_H
#include <pc/asset_regions.h>
#include <melee/gr/grdatfiles.h>
#include <melee/gr/types.h>
#include <stdlib.h>
/* Diagnostic rendering isolation only; no collision or simulation changes. */
static int MeleePc_MutecitySkipDisplayList(HSD_PObj* pobj)
{
    static uint32_t seen_offsets[246], seen_sizes[246], count;
    static const uint32_t platform_lists[][2] = {
{224960u,64u},
{225088u,64u},
{225216u,64u},
{225344u,160u},
{225568u,96u},
{225728u,96u},
{225888u,160u},
{226112u,96u},
{226272u,128u},
{226464u,160u},
{226688u,128u},
{226880u,96u},
{227040u,128u},
{227232u,96u},
{227392u,32u},
{227488u,32u},
{227584u,64u},
{227712u,160u},
{227936u,256u},
};
    const void *map_base=NULL,*map_owner=NULL,*draw_base=NULL,*draw_owner=NULL;
    uint32_t map_size=0,draw_size=0,offset,bytes,i;
    int platform=0;
    UnkArchiveStruct* archive;
    if (!getenv("MELEE_DIAGNOSTIC_MUTECITY_PLATFORM_ONLY")) return 0;
    archive=grDatFiles_GetArchive();
    if (!archive || !archive->unk4 || !pobj || !pobj->display) return 0;
    if (!MeleePc_AssetRegionInfo(archive->unk4,&map_base,&map_owner,&map_size) ||
        !MeleePc_AssetRegionInfo(pobj->display,&draw_base,&draw_owner,&draw_size)) return 0;
    if (map_base!=draw_base || map_owner!=draw_owner || map_size!=1336064 ||
        draw_size!=map_size || (uintptr_t)archive->unk4-(uintptr_t)map_base!=2524) return 0;
    offset=(uint32_t)((uintptr_t)pobj->display-(uintptr_t)draw_base);
    bytes=(uint32_t)pobj->n_display<<5;
    if (!bytes || offset>draw_size || bytes>draw_size-offset) return 0;
    for(i=0;i<sizeof(platform_lists)/sizeof(platform_lists[0]);++i)
        if(platform_lists[i][0]==offset && platform_lists[i][1]==bytes) {platform=1;break;}
    /* Source polygon225664 is the audited translucent platform candidate. */
    if (getenv("MELEE_DIAGNOSTIC_MUTECITY_PLATFORM_ALPHA_ONLY"))
        platform=platform && offset==225568u && bytes==96u;
    else if (getenv("MELEE_DIAGNOSTIC_MUTECITY_PLATFORM_ALPHA_EXCLUDED"))
        platform=platform && !(offset==225568u && bytes==96u);
    for(i=0;i<count;++i) if(seen_offsets[i]==offset && seen_sizes[i]==bytes) return !platform;
    if(count<246) {
        seen_offsets[count]=offset;seen_sizes[count++]=bytes;
        OSReport("NATIVE_MUTECITY_PLATFORM_ISOLATION source_offset=%u bytes=%u kept=%u allocation_identity=1\n",offset,bytes,(unsigned)platform);
    }
    return !platform;
}
#endif
