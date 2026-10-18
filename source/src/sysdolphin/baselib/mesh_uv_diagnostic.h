/* Source-owned UV tuples only. Texture generation/transforms are a later gate. */
#ifndef HSD_MESH_UV_DIAGNOSTIC_H
#define HSD_MESH_UV_DIAGNOSTIC_H
static int MeshUvEnabled(void)
{
    static int enabled=-1;
    if(enabled<0)enabled=MeshDirectionsEnabled()&&getenv("MELEE_DIAGNOSTIC_MESH_UV");
    return enabled;
}
static int MeshOriginalUvFormat(const HSD_VtxDescList* d,unsigned offset,AuroraMeshUv* out)
{
    if(!d||!out||d->attr<GX_VA_TEX0||d->attr>GX_VA_TEX7||d->comp_cnt>1||d->comp_type>4||d->frac>31||
       d->attr_type<GX_DIRECT||d->attr_type>GX_INDEX16||(d->attr_type!=GX_DIRECT&&!d->stride))return 0;
    out->storage=d->attr_type;out->offset=offset;out->components=d->comp_cnt+1;
    out->type=d->comp_type;out->fraction=d->frac;out->stride=d->stride;return 1;
}
static int MeshOriginalUv(const HSD_VtxDescList* d,const unsigned char* dl,unsigned available,double out[2])
{
    static const unsigned widths[5]={1,1,2,2,4};AuroraMeshUv layout;
    const void* raw=dl;unsigned width,bytes,c;double result[2]={0,0};
    if(!dl||!out||!MeshOriginalUvFormat(d,0,&layout))return 0;
    width=widths[layout.type];bytes=width*layout.components;
    if(layout.storage==GX_DIRECT){if(available<bytes)return 0;}
    else {
        unsigned index;uintptr_t address=(uintptr_t)d->vertex;
        if(available<(layout.storage==GX_INDEX8?1u:2u)||!d->vertex)return 0;
        index=layout.storage==GX_INDEX8?dl[0]:MeshBE16(dl);
        if(index>(UINT32_MAX-bytes)/layout.stride||address>UINTPTR_MAX-(uintptr_t)index*layout.stride)return 0;
        if(!MeleePc_OriginalAssetBytes((const void*)(address+(uintptr_t)index*layout.stride),bytes,&raw))return 0;
    }
    for(c=0;c<layout.components;c++){
        result[c]=MeshOriginalNumber((const unsigned char*)raw+c*width,layout.type,layout.fraction);
        if(!isfinite(result[c]))return 0;
    }
    memcpy(out,result,sizeof(result));return 1;
}
#endif
