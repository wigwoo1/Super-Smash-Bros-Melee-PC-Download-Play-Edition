#ifndef HSD_MESH_HSD_LIGHTING_DIAGNOSTIC_H
#define HSD_MESH_HSD_LIGHTING_DIAGNOSTIC_H
#include "state.h"
#include "lobj.h"
#include <aurora/mesh_diagnostics.h>
#include <math.h>
#include <dolphin/gx/GXGet.h>
static unsigned mesh_hsd_channel_owned;
static void MeshHsdChannelOwned(unsigned channel){if(channel<4)mesh_hsd_channel_owned|=1u<<channel;else if(channel<6)mesh_hsd_channel_owned|=(1u<<(channel-4))|(1u<<(channel-2));}
/* Included after tev.c's private channel cache. Do not modify HSD state. */
static int MeshHsdLight(AuroraOriginalMesh* mesh,unsigned id,const GXLightObj* object)
{
    unsigned slot=0,n;AuroraMeshLight* light;GXLightObj* native=(GXLightObj*)object;GXColor color;float pos[3],dir[3],cosine[3],distance[3];
    if(!id)return 1;if(id>128||(id&(id-1)))return 0;while((1u<<slot)!=id)slot++;
    if(mesh->lightMask&id)return 0;mesh->lightMask|=id;light=mesh->lights+slot;
    /* Native GXLightObj is opaque and differs from the console XF packet. */
    GXGetLightColor(native,&color);GXGetLightPos(native,pos,pos+1,pos+2);GXGetLightDir(native,dir,dir+1,dir+2);
    GXGetLightAttnA(native,cosine,cosine+1,cosine+2);GXGetLightAttnK(native,distance,distance+1,distance+2);
    for(n=0;n<4;n++)light->color[n]=((unsigned char*)&color)[n]/255.0;
    for(n=0;n<3;n++){
        light->position[n]=pos[n];light->direction[n]=-dir[n]; /* Getter returns caller convention;XF uses stored negative. */
        light->cosine[n]=cosine[n];light->distance[n]=distance[n];
        if(!isfinite(light->position[n])||!isfinite(light->direction[n])||!isfinite(light->cosine[n])||!isfinite(light->distance[n]))return 0;
    }
    return 1;
}
int HSD_DiagnosticCopyMeshLighting(AuroraOriginalMesh* mesh)
{
    int n,count=HSD_LObjGetNbActive();if(count<0||count>9)return 0;
    for(n=0;n<4;n++){
        HSD_Chan* ch=prev_ch+n;AuroraMeshChannel* target=mesh->channels+n;
        if(!(mesh_hsd_channel_owned&(1u<<n)))continue;
        if(ch->enable>1||ch->amb_src>1||ch->mat_src>1||ch->light_mask>255||ch->diff_fn>2||ch->attn_fn>2||
           (ch->mat_src==GX_SRC_REG&&prev_mat_invalid[n&1])||
           (ch->enable&&ch->amb_src==GX_SRC_REG&&prev_amb_invalid[n&1]))continue;
        mesh->channelMask|=1u<<n;
        target->enabled=ch->enable!=0;target->ambientSource=ch->amb_src;target->materialSource=ch->mat_src;
        target->lightMask=ch->light_mask;target->diffuse=ch->attn_fn==GX_AF_SPEC?GX_DF_NONE:ch->diff_fn;target->attenuation=ch->attn_fn;
        memcpy(target->ambient,&prev_ch[n&1].amb_color,4);memcpy(target->material,&prev_ch[n&1].mat_color,4);
    }
    for(n=0;n<count;n++){
        HSD_LObj* l=HSD_LObjGetActiveByIndex(n);if(!l)return 0;
        if(!MeshHsdLight(mesh,l->id,&l->lightobj)||!MeshHsdLight(mesh,l->spec_id,&l->spec_lightobj))return 0;
    }
    mesh->lightingReference=1;return 1;
}
#endif
