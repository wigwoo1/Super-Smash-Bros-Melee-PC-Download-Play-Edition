/* Original normal/NBT tuples, independently decoded from preserved BE bytes. */
#ifndef HSD_MESH_DIRECTION_DIAGNOSTIC_H
#define HSD_MESH_DIRECTION_DIAGNOSTIC_H
static int MeshDirectionsEnabled(void)
{
    static int value=-1;
    if(value<0)value=getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")&&getenv("MELEE_DIAGNOSTIC_MESH_PALETTE")&&
        getenv("MELEE_DIAGNOSTIC_ORIGINAL_MESH_GPU")&&getenv("MELEE_DIAGNOSTIC_MESH_DIRECTIONS");
    return value;
}
static int MeshOriginalDirectionFormat(HSD_VtxDescList* d,unsigned offset,AuroraOriginalMesh* mesh)
{
    if(!d||d->comp_cnt>2||(d->comp_type!=GX_S8&&d->comp_type!=GX_S16&&d->comp_type!=GX_F32)||
       d->attr_type<GX_DIRECT||d->attr_type>GX_INDEX16||
       (d->attr_type!=GX_DIRECT&&!d->stride)||(d->comp_cnt==GX_NRM_NBT3&&d->attr_type==GX_DIRECT))return 0;
    mesh->normalComponents=d->comp_cnt?9:3;mesh->normalOffset=offset;mesh->normalStorage=d->attr_type;
    mesh->normalType=d->comp_type;mesh->normalStride=d->stride;mesh->normalNbt3=d->comp_cnt==GX_NRM_NBT3;
    return 1;
}
static int MeshOriginalDirections(HSD_VtxDescList* d,const unsigned char* dl,unsigned available,double output[9])
{
    unsigned slice,components=d->comp_cnt?9:3,step=d->attr_type==GX_INDEX8?1:2;
    unsigned width=d->comp_type==GX_S8?1:d->comp_type==GX_S16?2:4;
    unsigned fraction=d->comp_type==GX_S8?6:d->comp_type==GX_S16?14:0;
    if(available<MeshWidth(GX_VA_NRM,d->attr_type,d->comp_cnt,d->comp_type))return 0;
    for(slice=0;slice<components/3;slice++){
        const void* raw=dl+slice*3*width;unsigned c;
        if(d->attr_type!=GX_DIRECT){
            unsigned at=d->comp_cnt==GX_NRM_NBT3?slice*step:0;
            unsigned index=step==1?dl[at]:MeshBE16(dl+at);
            if(!d->stride||index>(UINT32_MAX-slice*3*width)/d->stride||
               !MeleePc_OriginalAssetBytes((const unsigned char*)d->vertex+index*d->stride+slice*3*width,3*width,&raw))return 0;
        }
        for(c=0;c<3;c++){
            output[slice*3+c]=MeshOriginalNumber((const unsigned char*)raw+c*width,d->comp_type,fraction);
            if(!isfinite(output[slice*3+c]))return 0;
        }
    }
    return 1;
}
#endif
