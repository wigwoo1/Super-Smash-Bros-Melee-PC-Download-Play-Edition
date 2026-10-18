/* Readonly, opt-in original-to-live mesh preflight. Never suppresses a draw. */
#ifndef HSD_MESH_ORIGINAL_DIAGNOSTIC_H
#define HSD_MESH_ORIGINAL_DIAGNOSTIC_H
#ifdef TARGET_PC
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <aurora/mesh_diagnostics.h>
#include <pc/asset_regions.h>
#include <melee/ft/types.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/gobj.h>
typedef struct MeshOriginalResult {
    unsigned valid,vertices,primitives,selectors,nbt,arrays,bytes,palette;
    unsigned descriptor_errors,byte_errors,range_errors,selector_errors;
} MeshOriginalResult;
static unsigned MeshBE16(const unsigned char* p){return (unsigned)p[0]*256+p[1];}
static unsigned MeshBE32(const unsigned char* p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned MeshWidth(unsigned attr,unsigned storage,unsigned cnt,unsigned type)
{
    static const unsigned numeric[5]={1,1,2,2,4},colour[6]={2,3,4,2,3,4};
    if(storage==2||storage==3)return (storage==2?1:2)*(attr==10&&cnt==2?3:1);
    if(storage!=1)return 0;
    if(attr<9)return 1;
    if(attr==11||attr==12)return type<6?colour[type]:0;
    if(type>=5)return 0;
    if(attr==9)return cnt<=1?numeric[type]*(cnt?3:2):0;
    if(attr==10)return cnt<=2?numeric[type]*(cnt?9:3):0;
    if(attr>=13&&attr<=20)return cnt<=1?numeric[type]*(cnt?2:1):0;
    return 0;
}
static MeshOriginalResult MeshOriginalCheck(HSD_PObj* pobj)
{
    MeshOriginalResult result={0};HSD_VtxDescList* descs[21]={0};
    unsigned offsets[21]={0},widths[21]={0},max_indices[21]={0},used[21]={0};
    unsigned i,a,at=0,stride=0,length=(unsigned)pobj->n_display<<5;
    const void *original=NULL,*base=NULL,*owner=NULL;uint32_t size=0;
    const unsigned char* bytes=pobj->display;
    _Static_assert(sizeof(HSD_VtxDescList)==24,"native original descriptor ABI");
    if(!length||!MeleePc_OriginalAssetBytes(bytes,length,&original)){result.range_errors++;return result;}
    if(memcmp(bytes,original,length))result.byte_errors++;
    for(i=0;i<32;i++){
        HSD_VtxDescList* d=pobj->verts+i;const unsigned char* raw;
        if(!MeleePc_OriginalAssetBytes(d,4,&original)||!MeleePc_AssetRegionInfo(d,&base,&owner,&size)){result.range_errors++;return result;}
        if(d->attr!=MeshBE32(original)){result.descriptor_errors++;return result;}
        if(d->attr==GX_VA_NULL)break;
        if(!MeleePc_OriginalAssetBytes(d,24,&original)){result.range_errors++;return result;}
        raw=original;
        if(d->attr!=MeshBE32(raw)||d->attr_type!=MeshBE32(raw+4)||d->comp_cnt!=MeshBE32(raw+8)||d->comp_type!=MeshBE32(raw+12)||d->frac!=raw[16]||d->stride!=MeshBE16(raw+18))result.descriptor_errors++;
        a=d->attr==GX_VA_NBT?GX_VA_NRM:d->attr;
        if(a>20||d->attr_type>3||descs[a]){result.descriptor_errors++;return result;}
        if(d->attr_type==GX_NONE)continue;
        descs[a]=d;widths[a]=MeshWidth(a,d->attr_type,d->comp_cnt,d->comp_type);
        if(!widths[a]){result.descriptor_errors++;return result;}
        if(d->attr==GX_VA_NBT)result.nbt++;
        if(d->attr_type==GX_INDEX8||d->attr_type==GX_INDEX16){
            unsigned offset=MeshBE32(raw+20);
            if(offset>=size||d->vertex!=(const unsigned char*)base+offset){result.descriptor_errors++;return result;}
        }
    }
    if(i==32){result.descriptor_errors++;return result;}
    for(a=0;a<=20;a++)if(descs[a]){offsets[a]=stride;stride+=widths[a];}
    if(!stride){result.descriptor_errors++;return result;}
    result.palette=1;
    if((pobj->flags&0x3000)==POBJ_ENVELOPE){
        HSD_SList* list=pobj->u.envelope_list;result.palette=0;
        for(;list&&result.palette<11;list=list->next)result.palette++;
        if(list||!result.palette||result.palette>10){result.range_errors++;return result;}
    }else if((pobj->flags&0x3000)==POBJ_SKIN&&pobj->u.jobj)result.palette=2;
    while(at<length){
        unsigned command=bytes[at++],n,v;
        if(!command)continue;
        if((command&7)||((command&248)!=0x80&&(command&248)!=0x90&&(command&248)!=0x98&&(command&248)!=0xa0&&(command&248)!=0xa8&&(command&248)!=0xb0&&(command&248)!=0xb8)||at+2>length){result.range_errors++;return result;}
        n=MeshBE16(bytes+at);at+=2;
        if(n>(length-at)/stride){result.range_errors++;return result;}
        result.primitives++;result.vertices+=n;
        for(v=0;v<n;v++)for(a=0;a<=20;a++)if(descs[a]){
            HSD_VtxDescList* d=descs[a];const unsigned char* p=bytes+at+v*stride+offsets[a];
            if(a==0){unsigned selector=*p;result.selectors++;if(selector%3||selector/3>=result.palette)result.selector_errors++;}
            if(d->attr_type==GX_INDEX8||d->attr_type==GX_INDEX16){
                unsigned step=d->attr_type==GX_INDEX8?1:2,k;
                for(k=0;k<widths[a];k+=step){unsigned index=step==1?p[k]:MeshBE16(p+k);if(!used[a]||index>max_indices[a])max_indices[a]=index;used[a]=1;}
            }
        }
        at+=n*stride;
    }
    for(a=9;a<=20;a++)if(used[a]){
        HSD_VtxDescList* d=descs[a];unsigned component=MeshWidth(a,GX_DIRECT,d->comp_cnt,d->comp_type),needed;
        if(!d->stride||!component||max_indices[a]>(UINT32_MAX-component)/d->stride){result.range_errors++;continue;}
        needed=max_indices[a]*d->stride+component;
        if(!MeleePc_OriginalAssetBytes(d->vertex,needed,&original)){result.range_errors++;continue;}
        if(memcmp(d->vertex,original,needed))result.byte_errors++;
        result.arrays++;result.bytes+=needed;
    }
    result.valid=!(result.descriptor_errors||result.byte_errors||result.range_errors||result.selector_errors);return result;
}
extern unsigned int HSD_JObjDiagnosticMatchFrame;
static void PObjOriginalMeshDiagnostic(HSD_PObj* pobj)
{
    static int enabled=-1;static unsigned count;static const void* seen_display[512];static const void* seen_verts[512];static int seen_actor[512];
    HSD_JObj* node=HSD_JObjGetCurrent();int player,kind=-1,depth;unsigned i;MeshOriginalResult r;
    if(enabled<0)enabled=getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")&&getenv("MELEE_DIAGNOSTIC_ORIGINAL_MESH");
    if(!enabled||count>=512||HSD_JObjDiagnosticMatchFrame<300)return;
    for(player=0;player<4&&kind<0;player++){
        HSD_GObj* g=Player_GetEntity(player);Fighter* f=g?g->user_data:NULL;HSD_JObj* ancestor;
        if(!f)continue;
        for(ancestor=node,depth=0;ancestor&&depth<128;ancestor=ancestor->parent,depth++)if(ancestor==(HSD_JObj*)g->hsd_obj){kind=f->kind;break;}
    }
    if(kind<0)return;
    for(i=0;i<count;i++)if(seen_display[i]==pobj->display&&seen_verts[i]==pobj->verts&&seen_actor[i]==kind)return;
    seen_display[count]=pobj->display;seen_verts[count]=pobj->verts;seen_actor[count]=kind;
    r=MeshOriginalCheck(pobj);
    OSReport("NATIVE_ORIGINAL_MESH sample=%u frame=%u actor=%d valid=%u vertices=%u primitives=%u selectors=%u nbt=%u arrays=%u bytes=%u palette=%u descriptor_errors=%u byte_errors=%u range_errors=%u selector_errors=%u\n",
        ++count,HSD_JObjDiagnosticMatchFrame,kind,r.valid,r.vertices,r.primitives,r.selectors,r.nbt,r.arrays,r.bytes,r.palette,r.descriptor_errors,r.byte_errors,r.range_errors,r.selector_errors);
}
static double MeshOriginalNumber(const unsigned char* p,unsigned type,unsigned fraction)
{
    double value;
    if(type==GX_F32){uint32_t word=MeshBE32(p);float number;memcpy(&number,&word,4);return number;}
    if(type==GX_U8)value=p[0];
    else if(type==GX_S8)value=(int8_t)p[0];
    else if(type==GX_U16)value=MeshBE16(p);
    else {unsigned word=MeshBE16(p);value=word>=32768?(int)word-65536:(int)word;}
    return ldexp(value,-(int)fraction);
}
#include "mesh_palette_diagnostic.h"
#include "mesh_direction_diagnostic.h"
#include "mesh_uv_diagnostic.h"
#include "mesh_material_diagnostic.h"
extern int HSD_DiagnosticCopyMeshTexgens(AuroraOriginalMesh*);
static void PObjOriginalGpuDiagnostic(HSD_PObj* pobj,Mtx view,Mtx model,unsigned has_normal)
{
    static int enabled=-1;static unsigned quota[34][3][2][2][3],palette_first_frame,direction_last_frame,direction_window;
    HSD_JObj* node=HSD_JObjGetCurrent();HSD_JObj* root=NULL;int player,kind=-1,depth;unsigned i,a,at,stride=0,offsets[21]={0};
    HSD_VtxDescList* descs[21]={0};const unsigned char* original=NULL;
    AuroraOriginalMesh* mesh;MeshOriginalResult checked;unsigned bucket,nbt,palette_kind,normal_class;
    if(enabled<0)enabled=getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")&&getenv("MELEE_DIAGNOSTIC_ORIGINAL_MESH_GPU");
    if(!enabled||HSD_JObjDiagnosticMatchFrame<300||HSD_JObjDiagnosticMatchFrame>=(MeshDirectionsEnabled()?3600:MeshPaletteEnabled()?1200:600))return;
    for(player=0;player<4&&kind<0;player++){
        HSD_GObj* g=Player_GetEntity(player);Fighter* f=g?g->user_data:NULL;HSD_JObj* ancestor;
        if(!f)continue;
        for(ancestor=node,depth=0;ancestor&&depth<128;ancestor=ancestor->parent,depth++)if(ancestor==(HSD_JObj*)g->hsd_obj){kind=f->kind;root=ancestor;break;}
    }
    if(kind<0||kind>=34)return;
    /* Anchor palette windows to a rendered fighter, not simulated cold startup. */
    if(MeshDirectionsEnabled()){
        unsigned frame=HSD_JObjDiagnosticMatchFrame;
        if(!direction_last_frame||frame-direction_last_frame>=100){
            if(direction_last_frame&&++direction_window>=3)return;
            direction_last_frame=frame;
            OSReport("MESH_DIRECTION_CHECKPOINT index=%u frame=%u\n",direction_window,frame);
        }
        if(direction_window>=3)return;
        bucket=direction_window;
        if(!palette_first_frame){palette_first_frame=frame;OSReport("MESH_PALETTE_WINDOW_START frame=%u period=100\n",frame);}
    }else if(MeshPaletteEnabled()){
        if(!palette_first_frame){palette_first_frame=HSD_JObjDiagnosticMatchFrame;OSReport("MESH_PALETTE_WINDOW_START frame=%u period=100\n",palette_first_frame);}
        bucket=(HSD_JObjDiagnosticMatchFrame-palette_first_frame)/100;
        if(bucket>=3)return;
    }else bucket=(HSD_JObjDiagnosticMatchFrame-300)/100;
    nbt=0;
    for(i=0;i<32&&pobj->verts[i].attr!=GX_VA_NULL;i++){
        HSD_VtxDescList* d=pobj->verts+i;a=d->attr==GX_VA_NBT?GX_VA_NRM:d->attr;
        if(a>20)return;if(d->attr==GX_VA_NBT)nbt=1;
        if(d->attr_type!=GX_NONE)descs[a]=d;
    }
    palette_kind=MeshPaletteEnabled()?((pobj->flags&0x3000)==POBJ_ENVELOPE?2:pobj->u.jobj?1:0):0;
    normal_class=MeshPaletteEnabled()?has_normal:0;
    if(i==32||!descs[GX_VA_POS]||quota[kind][bucket][nbt][normal_class][palette_kind])return;
    checked=MeshOriginalCheck(pobj);
    if(!checked.valid||!checked.primitives||checked.primitives>AURORA_MESH_PRIMITIVES)return;
    if(!MeleePc_OriginalAssetBytes(pobj->display,(unsigned)pobj->n_display<<5,(const void**)&original))return;
    for(a=0;a<=20;a++)if(descs[a]){offsets[a]=stride;stride+=MeshWidth(a,descs[a]->attr_type,descs[a]->comp_cnt,descs[a]->comp_type);}
    mesh=calloc(1,sizeof(*mesh));if(!mesh)return;
    mesh->display=pobj->display;mesh->original=original;mesh->bytes=(unsigned)pobj->n_display<<5;
    mesh->frame=HSD_JObjDiagnosticMatchFrame;mesh->actor=kind;mesh->nbt=nbt;mesh->stride=stride;
    {
        HSD_VtxDescList* d=descs[GX_VA_POS];
        mesh->positionOffset=offsets[GX_VA_POS];mesh->positionStorage=d->attr_type;
        mesh->positionComponents=d->comp_cnt?3:2;mesh->positionType=d->comp_type;
        mesh->positionFraction=d->frac;mesh->positionStride=d->stride;
    }
    if(MeshDirectionsEnabled()&&has_normal&&descs[GX_VA_NRM]&&
       !MeshOriginalDirectionFormat(descs[GX_VA_NRM],offsets[GX_VA_NRM],mesh))goto refused;
    if(MeshUvEnabled()&&mesh->normalComponents)for(i=0;i<8;i++)if(descs[GX_VA_TEX0+i]){
        if(!MeshOriginalUvFormat(descs[GX_VA_TEX0+i],offsets[GX_VA_TEX0+i],mesh->uv+i))goto refused;
        mesh->uvMask|=1u<<i;
    }
    if(MeshLightingEnabled()&&mesh->normalComponents)for(i=0;i<2;i++)if(descs[GX_VA_CLR0+i]){
        HSD_VtxDescList* d=descs[GX_VA_CLR0+i];mesh->colorStorage[i]=d->attr_type;mesh->colorOffset[i]=offsets[GX_VA_CLR0+i];
        mesh->colorType[i]=d->comp_type;mesh->colorStride[i]=d->stride;
    }
    for(at=0;at<mesh->bytes;){
        unsigned command=original[at++],n,v,c,component;
        AuroraMeshPrimitive* primitive;HSD_VtxDescList* d=descs[GX_VA_POS];
        static const unsigned width[5]={1,1,2,2,4};
        if(!command)continue;
        if(mesh->primitives>=AURORA_MESH_PRIMITIVES||d->comp_type>4||at+2>mesh->bytes)goto refused;
        if((command&248)!=GX_QUADS&&(command&248)!=GX_TRIANGLES&&(command&248)!=GX_TRIANGLESTRIP&&(command&248)!=GX_TRIANGLEFAN)goto refused;
        n=MeshBE16(original+at);at+=2;if(!n||n>(mesh->bytes-at)/stride)goto refused;
        primitive=mesh->primitive+mesh->primitives++;primitive->offset=at;primitive->bytes=n*stride;
        primitive->vertices=n;primitive->points=n<AURORA_MESH_POINTS?n:AURORA_MESH_POINTS;component=width[d->comp_type];
        for(v=0;v<primitive->points;v++){
            const unsigned char* p=original+at+v*stride+offsets[GX_VA_POS];const void* raw=p;
            AuroraMeshPoint* point=primitive->point+v;
            point->selector=descs[GX_VA_PNMTXIDX]?original[at+v*stride+offsets[GX_VA_PNMTXIDX]]:UINT32_MAX;
            if(d->attr_type==GX_INDEX8||d->attr_type==GX_INDEX16){
                unsigned index=d->attr_type==GX_INDEX8?p[0]:MeshBE16(p);
                if(!MeleePc_OriginalAssetBytes((const unsigned char*)d->vertex+index*d->stride,component*mesh->positionComponents,&raw))goto refused;
            }
            for(c=0;c<mesh->positionComponents;c++)point->position[c]=MeshOriginalNumber((const unsigned char*)raw+c*component,d->comp_type,d->frac);
            for(i=0;i<8;i++)if((mesh->uvMask&(1u<<i))&&!MeshOriginalUv(descs[GX_VA_TEX0+i],
                original+at+v*stride+mesh->uv[i].offset,stride-mesh->uv[i].offset,point->uv[i]))goto refused;
            if(mesh->normalComponents&&!MeshOriginalDirections(descs[GX_VA_NRM],original+at+v*stride+mesh->normalOffset,
                stride-mesh->normalOffset,point->directions))goto refused;
            if(MeshLightingEnabled()&&mesh->normalComponents)for(i=0;i<2;i++)if(mesh->colorStorage[i]&&
                !MeshOriginalColor(descs[GX_VA_CLR0+i],original+at+v*stride+mesh->colorOffset[i],stride-mesh->colorOffset[i],point->color[i]))goto refused;
        }
        at+=primitive->bytes;
    }
    if(MeshPaletteEnabled()&&!MeshOriginalPalette(pobj,node,root,view,model,has_normal,mesh)){
        quota[kind][bucket][nbt][normal_class][palette_kind]++;
        OSReport("MESH_GPU_SOURCE_PALETTE_FAIL actor=%d frame=%u kind=%u normal=%u\n",kind,mesh->frame,palette_kind,has_normal);goto refused;
    }
    if(MeshLightingEnabled()&&mesh->normalComponents&&!MeshOriginalMaterialLighting(pobj,node,mesh)){
        OSReport("MESH_MATERIAL_SOURCE_FAIL actor=%d frame=%u\n",kind,mesh->frame);goto refused;
    }
    if(mesh->lightingReference&&MeshUvEnabled()&&getenv("MELEE_DIAGNOSTIC_MESH_TEXGEN")&&!HSD_DiagnosticCopyMeshTexgens(mesh)){
        OSReport("MESH_TEXGEN_SOURCE_FAIL actor=%d frame=%u\n",kind,mesh->frame);goto refused;
    }
    if(aurora_diagnostic_original_mesh(mesh)){
        quota[kind][bucket][nbt][normal_class][palette_kind]++;
        OSReport("MESH_GPU_SOURCE actor=%d frame=%u nbt=%u primitives=%u stride=%u\n",kind,mesh->frame,nbt,mesh->primitives,stride);
    }else OSReport("MESH_GPU_SOURCE_REFUSED actor=%d frame=%u\n",kind,mesh->frame);
refused:
    free(mesh);
}
#endif
#endif
