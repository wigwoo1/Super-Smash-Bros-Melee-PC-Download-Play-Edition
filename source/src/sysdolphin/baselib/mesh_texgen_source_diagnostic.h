/* The validated current MObj owns these runtime texture matrices. No GX readback
 * is used as the reference. Original SRT byte/track provenance is a later gate. */
#ifndef HSD_MESH_TEXGEN_SOURCE_DIAGNOSTIC_H
#define HSD_MESH_TEXGEN_SOURCE_DIAGNOSTIC_H
#include <aurora/mesh_diagnostics.h>
#include <math.h>
extern int HSD_DiagnosticTextureSrtMatrix(HSD_TObj*,double[12]);
int HSD_DiagnosticCopyMeshTexgens(AuroraOriginalMesh* mesh)
{
    HSD_TObj* t;unsigned n,owned=0,count=0,mask=0;AuroraMeshTexgen refs[8]={{0}};
    if(!mesh||!MaterialSourceEnabled()||!current_mobj||!mesh->lightingReference)return 0;
    for(n=0;n<512;n++)if(material_sources[n].object==current_mobj){owned=1;break;}
    if(!owned)return 0;
    for(t=current_mobj->tobj;t;t=t->next){
        unsigned coord,source,c;AuroraMeshTexgen* out;
        if(++count>32)return 0; /* Also bounds cyclic lists. */
        if(t->id==GX_TEXMAP_NULL||tobj_coord(t)!=TEX_COORD_UV||tobj_bump(t)||
           t->src<GX_TG_TEX0||t->src>GX_TG_TEX7)continue;
        coord=(unsigned)t->coord;source=(unsigned)t->src-GX_TG_TEX0;
        if(coord>=8||(mask&(1u<<coord))||!(mesh->uvMask&(1u<<source))||
           t->mtxid<GX_PTTEXMTX0||t->mtxid>GX_PTTEXMTX19||(t->mtxid-GX_PTTEXMTX0)%3)return 0;
        out=refs+coord;out->source=source;out->postMatrix=t->mtxid;
        for(c=0;c<12;c++){double value=((float*)t->mtx)[c];if(!isfinite(value))return 0;out->matrix[c]=value;}
        if(getenv("MELEE_DIAGNOSTIC_TEXTURE_SRT")){
            if(!HSD_DiagnosticTextureSrtMatrix(t,out->matrix))return 0;
            out->sourceSrtBytes=36;
        }
        mask|=1u<<coord;
    }
    memcpy(mesh->texgen,refs,sizeof(refs));mesh->texgenMask=mask;return 1;
}
#endif
