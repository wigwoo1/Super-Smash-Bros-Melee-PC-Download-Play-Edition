#ifndef HSD_MESH_MATERIAL_DIAGNOSTIC_H
#define HSD_MESH_MATERIAL_DIAGNOSTIC_H
extern int HSD_DiagnosticMaterialMatches(HSD_MObj*,HSD_MObjDesc*);
extern int HSD_DiagnosticCopyMeshLighting(AuroraOriginalMesh*);
static int MeshLightingEnabled(void){static int value=-1;if(value<0)value=MeshDirectionsEnabled()&&getenv("MELEE_DIAGNOSTIC_MESH_LIGHTING");return value;}
static int MeshOriginalColor(HSD_VtxDescList* d,const unsigned char* dl,unsigned available,unsigned char out[4])
{
    static const unsigned widths[6]={2,3,4,2,3,4};unsigned type=d->comp_type,n;const void* raw=dl;const unsigned char* p;
    if(type>=6||d->attr_type<GX_DIRECT||d->attr_type>GX_INDEX16||available<MeshWidth(GX_VA_CLR0,d->attr_type,d->comp_cnt,type))return 0;
    if(d->attr_type!=GX_DIRECT){unsigned index=d->attr_type==GX_INDEX8?dl[0]:MeshBE16(dl);if(!d->stride||index>(UINT32_MAX-widths[type])/d->stride||!MeleePc_OriginalAssetBytes((const unsigned char*)d->vertex+index*d->stride,widths[type],&raw))return 0;}
    p=raw;out[3]=255;
    if(type==GX_RGB565){unsigned v=MeshBE16(p),r=v>>11,g=(v>>5)&63,b=v&31;out[0]=(r<<3)|(r>>2);out[1]=(g<<2)|(g>>4);out[2]=(b<<3)|(b>>2);}
    else if(type==GX_RGBA4){unsigned v=MeshBE16(p);for(n=0;n<4;n++)out[n]=((v>>(12-4*n))&15)*17;}
    else if(type==GX_RGBA6){unsigned v=(unsigned)p[0]<<16|(unsigned)p[1]<<8|p[2];for(n=0;n<4;n++){unsigned q=(v>>(18-6*n))&63;out[n]=(q<<2)|(q>>4);}}
    else {memcpy(out,p,3);if(type==GX_RGBA8)out[3]=p[3];}
    return 1;
}
static int MeshOriginalMaterialLighting(HSD_PObj* pobj,HSD_JObj* node,AuroraOriginalMesh* mesh)
{
    const void* base=NULL;unsigned size=0;MeshPObjOwner owner={0};
    if(!MeshPaletteSourcePObj(node,pobj,&base,&size,&owner)||!owner.dobj||!owner.material||
       !MeshPaletteAt(base,size,owner.material,24)||
       !HSD_DiagnosticMaterialMatches(owner.dobj->mobj,(HSD_MObjDesc*)((unsigned char*)base+owner.material)))return 0;
    mesh->sourceMaterialChecks=20;return HSD_DiagnosticCopyMeshLighting(mesh);
}
#endif
