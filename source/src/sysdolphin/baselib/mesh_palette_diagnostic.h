/* Source bone/weight ownership and independent palette reference. RAM only. */
#ifndef HSD_MESH_PALETTE_DIAGNOSTIC_H
#define HSD_MESH_PALETTE_DIAGNOSTIC_H
#include "envelope_diagnostic.h"
#include "dobj.h"
static int MeshPaletteEnabled(void)
{
    static int enabled=-1;
    if(enabled<0)enabled=getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")&&getenv("MELEE_DIAGNOSTIC_ORIGINAL_MESH_GPU")&&getenv("MELEE_DIAGNOSTIC_MESH_PALETTE");
    return enabled;
}
static const unsigned char* MeshPaletteAt(const void* base,unsigned size,unsigned offset,unsigned bytes)
{
    const void* raw=NULL;
    if(offset>size||bytes>size-offset||!MeleePc_OriginalAssetBytes((const unsigned char*)base+offset,bytes,&raw))return NULL;
    return raw;
}
static int MeshPaletteWithin(HSD_JObj* bone,HSD_JObj* root)
{
    unsigned depth;for(depth=0;bone&&depth<128;bone=bone->parent,depth++)if(bone==root)return 1;
    return 0;
}
typedef struct MeshPObjOwner {HSD_DObj* dobj;unsigned material;} MeshPObjOwner;
static const unsigned char* MeshPaletteSourcePObj(HSD_JObj* node,HSD_PObj* pobj,const void** base_out,unsigned* size_out,MeshPObjOwner* owner_out)
{
    const void *base=NULL,*owner=NULL,*raw=NULL;uint32_t size=0;unsigned doff,di;
    HSD_DObj* dobj;
    if(!node||!node->id||(node->flags&(JOBJ_INSTANCE|JOBJ_SPLINE|JOBJ_PTCL)))return NULL;
    if(!MeleePc_AssetRegionInfo((const void*)(uintptr_t)node->id,&base,&owner,&size)||
       !MeleePc_OriginalAssetBytes((const void*)(uintptr_t)node->id,20,&raw))return NULL;
    doff=MeshBE32((const unsigned char*)raw+16);
    for(di=0,dobj=node->u.dobj;dobj&&doff&&di<128;di++,dobj=dobj->next){
        const unsigned char* d=MeshPaletteAt(base,size,doff,16);unsigned poff,pi;HSD_PObj* p;
        if(!d)return NULL;poff=MeshBE32(d+12);
        for(pi=0,p=dobj->pobj;p&&poff&&pi<256;pi++,p=p->next){
            const unsigned char* pd=MeshPaletteAt(base,size,poff,24);
            if(!pd)return NULL;
            if(p==pobj){
                if(MeshBE32(pd+8)!=(uintptr_t)pobj->verts-(uintptr_t)base||
                   MeshBE32(pd+16)!=(uintptr_t)pobj->display-(uintptr_t)base||
                   MeshBE16(pd+14)!=pobj->n_display||((MeshBE16(pd+12)^pobj->flags)&0x3000))return NULL;
                *base_out=base;*size_out=size;if(owner_out){owner_out->dobj=dobj;owner_out->material=MeshBE32(d+8);}return pd;
            }
            poff=MeshBE32(pd+4);
        }
        doff=MeshBE32(d+4);
    }
    return NULL;
}
static int MeshPaletteBoneSource(HSD_JObj* bone,HSD_JObj* root,const void* base,unsigned size,unsigned offset)
{
    return bone&&MeshPaletteWithin(bone,root)&&MeshPaletteAt(base,size,offset,20)&&
        (uintptr_t)bone->id==(uintptr_t)base+offset;
}
static int MeshPaletteBindSource(HSD_JObj* bone,const void* base,unsigned size,unsigned offset,AuroraOriginalMesh* mesh)
{
    const unsigned char* joint=MeshPaletteAt(base,size,offset,60);const unsigned char* bind;unsigned at,n,bits;
    if(!joint)return 0;at=MeshBE32(joint+56);
    if(!at)return bone->envelopemtx==NULL;
    if(!bone->envelopemtx||(bind=MeshPaletteAt(base,size,at,48))==NULL)return 0;
    for(n=0;n<12;n++){
        memcpy(&bits,(const unsigned char*)bone->envelopemtx+n*4,4);
        if(bits!=MeshBE32(bind+n*4))return 0;
        mesh->sourceBindComponents++;
    }
    return 1;
}
static int MeshPaletteEnvelopeSource(HSD_Envelope* envelope,HSD_JObj* root,const void* base,unsigned size,unsigned offset,AuroraOriginalMesh* mesh)
{
    unsigned n;
    for(n=0;n<=128;n++){
        const unsigned char* entry;unsigned bone,bits;
        if(n>UINT32_MAX/8||offset>UINT32_MAX-n*8)return 0;
        entry=MeshPaletteAt(base,size,offset+n*8,8);if(!entry)return 0;bone=MeshBE32(entry);
        if(!bone)return envelope==NULL;
        if(!envelope||!MeshPaletteBoneSource(envelope->jobj,root,base,size,bone)||
           !MeshPaletteBindSource(envelope->jobj,base,size,bone,mesh))return 0;
        memcpy(&bits,&envelope->weight,4);
        if(bits!=MeshBE32(entry+4)||!isfinite(envelope->weight))return 0;
        mesh->sourceBoneChecks++;mesh->sourceWeightChecks++;envelope=envelope->next;
    }
    return 0;
}
static int MeshPaletteNormal(const double position[3][4],double output[9])
{
    EnvelopeRefMtx input,inverse;double det;unsigned r,c;
    memset(input,0,sizeof(input));for(r=0;r<3;r++)for(c=0;c<4;c++)input[r][c]=position[r][c];input[3][3]=1;
    det=input[0][0]*(input[1][1]*input[2][2]-input[1][2]*input[2][1])-input[0][1]*(input[1][0]*input[2][2]-input[1][2]*input[2][0])+input[0][2]*(input[1][0]*input[2][1]-input[1][1]*input[2][0]);
    if(fabs(det)<1e-10){for(r=0;r<3;r++)for(c=0;c<3;c++)output[r*3+c]=input[r][c];return 1;}
    if(!EnvelopeRefInverse(input,inverse))return 0;
    for(r=0;r<3;r++)for(c=0;c<3;c++)output[r*3+c]=inverse[c][r];return 1;
}
static int MeshOriginalPalette(HSD_PObj* pobj,HSD_JObj* node,HSD_JObj* root,Mtx view,Mtx model,unsigned has_normal,AuroraOriginalMesh* mesh)
{
    const void* base=NULL;unsigned size=0,slot=0,type=pobj->flags&0x3000;
    const unsigned char* source=MeshPaletteSourcePObj(node,pobj,&base,&size,NULL);
    if(!source||!MeshPaletteWithin(node,root)||
       !MeshPaletteBindSource(node,base,size,(unsigned)((uintptr_t)node->id-(uintptr_t)base),mesh))return 0;
    mesh->sourceBoneChecks++;
    if(type==POBJ_ENVELOPE){
        unsigned table=MeshBE32(source+20);HSD_SList* list;
        mesh->paletteKind=2;if(table>UINT32_MAX-44||!MeshPaletteAt(base,size,table,4))return 0;
        for(list=pobj->u.envelope_list;list&&slot<10;slot++,list=list->next){
            const unsigned char* entry=MeshPaletteAt(base,size,table+slot*4,4);unsigned influences=0;int right=0;
            if(!entry||!MeshBE32(entry)||!MeshPaletteEnvelopeSource(list->data,root,base,size,MeshBE32(entry),mesh))return 0;
            if(!EnvelopeRefPosition(node,list->data,view,(double(*)[4])mesh->positionPalette[slot],&influences,&right))return 0;
            mesh->paletteMask|=1u<<slot;if(influences>1)mesh->weightedSlots++;if(right)mesh->rightSlots++;
        }
        if(list||!slot)return 0;
        {const unsigned char* end=MeshPaletteAt(base,size,table+slot*4,4);if(!end||MeshBE32(end))return 0;}
    }else if(type==POBJ_SKIN){
        unsigned r,c,other=MeshBE32(source+20);mesh->paletteKind=pobj->u.jobj?1:0;
        for(r=0;r<3;r++)for(c=0;c<4;c++)mesh->positionPalette[0][r*4+c]=model[r][c];
        mesh->paletteMask=1;slot=1;
        if(pobj->u.jobj){
            EnvelopeRefMtx a,b,result;
            if(!other||!MeshPaletteBoneSource(pobj->u.jobj,root,base,size,other)||
               !MeshPaletteBindSource(pobj->u.jobj,base,size,other,mesh))return 0;
            mesh->sourceBoneChecks++;EnvelopeRefCopy(view,a);EnvelopeRefCopy(pobj->u.jobj->mtx,b);EnvelopeRefMultiply(a,b,result);
            for(r=0;r<3;r++)for(c=0;c<4;c++)mesh->positionPalette[1][r*4+c]=result[r][c];
            mesh->paletteMask|=2;slot=2;
        }else if(other)return 0;
    }else return 0;
    if(has_normal)for(slot=0;slot<10;slot++)if(mesh->paletteMask&(1u<<slot)){
        if(!MeshPaletteNormal((const double(*)[4])mesh->positionPalette[slot],mesh->normalPalette[slot]))return 0;
        mesh->normalMask|=1u<<slot;
    }
    return 1;
}
#endif
