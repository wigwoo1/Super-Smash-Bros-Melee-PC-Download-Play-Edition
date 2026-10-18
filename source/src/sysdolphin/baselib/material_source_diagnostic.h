/* Optional source provenance table. Metadata only;original payload stays RAM-only. */
#ifndef HSD_MATERIAL_SOURCE_DIAGNOSTIC_H
#define HSD_MATERIAL_SOURCE_DIAGNOSTIC_H
#include <pc/asset_regions.h>
#include <stdint.h>
#include <stdlib.h>
typedef struct MaterialSourceRecord {HSD_MObj* object;HSD_MObjDesc* descriptor;} MaterialSourceRecord;
static MaterialSourceRecord material_sources[512];
static int MaterialSourceEnabled(void){static int v=-1;if(v<0)v=getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")&&getenv("MELEE_DIAGNOSTIC_MESH_LIGHTING");return v;}
static unsigned MaterialSourceBE32(const unsigned char* p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int MaterialSourceCheck(HSD_MObj* object,HSD_MObjDesc* desc)
{
    const void *base=NULL,*owner=NULL,*raw=NULL;uint32_t size=0;unsigned offset,n,bits;const unsigned char* source;
    if(!object||!object->mat||!desc||!desc->mat||!MeleePc_AssetRegionInfo(desc,&base,&owner,&size)||
       !MeleePc_OriginalAssetBytes(desc,24,&raw))return 0;
    source=raw;offset=MaterialSourceBE32(source+12);
    if(offset>size||20>size-offset||desc->mat!=(const HSD_Material*)((const unsigned char*)base+offset)||
       desc->rendermode!=MaterialSourceBE32(source+4)||(object->rendermode!=(desc->rendermode|RENDER_TOON))||
       !MeleePc_OriginalAssetBytes(desc->mat,20,&raw))return 0;
    source=raw;if(memcmp(desc->mat,source,12)||memcmp(object->mat,source,12))return 0;
    for(n=12;n<20;n+=4){
        memcpy(&bits,(const unsigned char*)desc->mat+n,4);if(bits!=MaterialSourceBE32(source+n))return 0;
        memcpy(&bits,(const unsigned char*)object->mat+n,4);if(bits!=MaterialSourceBE32(source+n))return 0;
    }
    return 1;
}
static void MaterialSourceLoaded(HSD_MObj* object,HSD_MObjDesc* desc)
{
    const void* raw;unsigned n;if(!MaterialSourceEnabled()||!MeleePc_OriginalAssetBytes(desc,24,&raw))return;
    if(!MaterialSourceCheck(object,desc)){OSReport("MESH_MATERIAL_LOAD_FAIL\n");return;}
    for(n=0;n<512;n++)if(!material_sources[n].object){material_sources[n].object=object;material_sources[n].descriptor=desc;OSReport("MESH_MATERIAL_LOADED bytes=20\n");return;}
    OSReport("MESH_MATERIAL_LOAD_FAIL reason=table-cap\n");
}
static void MaterialSourceForget(HSD_MObj* object){unsigned n;if(!MaterialSourceEnabled())return;for(n=0;n<512;n++)if(material_sources[n].object==object)memset(material_sources+n,0,sizeof(material_sources[n]));}
int HSD_DiagnosticMaterialMatches(HSD_MObj* object,HSD_MObjDesc* desc)
{
    unsigned n;if(!MaterialSourceEnabled()||current_mobj!=object)return 0;
    for(n=0;n<512;n++)if(material_sources[n].object==object)return material_sources[n].descriptor==desc;
    return 0;
}
#include "mesh_texgen_source_diagnostic.h"
#endif
