#pragma once
#include "mesh_diagnostics.h"
#include <float.h>
static inline int AuroraOriginalMeshValid(const AuroraOriginalMesh* s){
  uint32_t p,n,c,end=0;
  if(!s||!s->display||!s->original||!s->bytes||s->bytes>262144||!s->primitives||s->primitives>AURORA_MESH_PRIMITIVES||
     !s->stride||s->stride>255||s->actor>=34||s->frame<300||s->positionOffset>=s->stride||
     s->positionStorage<1||s->positionStorage>3||s->positionType>4||
     s->positionComponents<2||s->positionComponents>3||s->positionFraction>31)return 0;
  for(p=0;p<s->primitives;p++){
    const AuroraMeshPrimitive* v=s->primitive+p;
    if(!v->vertices||!v->points||v->points>AURORA_MESH_POINTS||v->points>v->vertices||v->offset<end||
       v->offset>s->bytes||v->bytes>s->bytes-v->offset||(uint64_t)v->vertices*s->stride!=v->bytes)return 0;
    end=v->offset+v->bytes;
    for(n=0;n<v->points;n++){
      if(v->point[n].selector!=UINT32_MAX&&(v->point[n].selector%3||v->point[n].selector>=30))return 0;
      for(c=0;c<3;c++)if(!(v->point[n].position[c]<=DBL_MAX&&v->point[n].position[c]>=-DBL_MAX))return 0;
    }
  }
  if(s->paletteMask||s->normalMask){
    if((s->paletteMask&~1023u)||(s->normalMask&~s->paletteMask)||!s->paletteMask||s->paletteKind>2||!s->sourceBoneChecks||
       (s->paletteKind==2&&!s->sourceWeightChecks))return 0;
    for(p=0;p<10;p++){
      if(s->paletteMask&(1u<<p))for(c=0;c<12;c++)if(!(s->positionPalette[p][c]<=DBL_MAX&&s->positionPalette[p][c]>=-DBL_MAX))return 0;
      if(s->normalMask&(1u<<p))for(c=0;c<9;c++)if(!(s->normalPalette[p][c]<=DBL_MAX&&s->normalPalette[p][c]>=-DBL_MAX))return 0;
    }
  }
  if(s->normalComponents){
    if((s->normalComponents!=3&&s->normalComponents!=9)||!s->normalMask||
       s->normalOffset>=s->stride||s->normalStorage<1||s->normalStorage>3||
       (s->normalType!=1&&s->normalType!=3&&s->normalType!=4)||s->normalNbt3>1||
       (s->normalNbt3&&(s->normalComponents!=9||s->normalStorage==1))||
       (s->normalStorage!=1&&!s->normalStride))return 0;
    for(p=0;p<s->primitives;p++)for(n=0;n<s->primitive[p].points;n++)for(c=0;c<s->normalComponents;c++)
      if(!(s->primitive[p].point[n].directions[c]<=DBL_MAX&&s->primitive[p].point[n].directions[c]>=-DBL_MAX))return 0;
  }
  if(s->lightingReference){
    if(s->lightingReference!=1||!s->normalComponents||!s->normalMask||s->sourceMaterialChecks!=20||s->lightMask>255||s->channelMask>15)return 0;
    for(p=0;p<4;p++)if((s->channelMask&(1u<<p))&&(s->channels[p].enabled>1||s->channels[p].ambientSource>1||s->channels[p].materialSource>1||s->channels[p].lightMask>255||s->channels[p].diffuse>2||s->channels[p].attenuation>2))return 0;
    for(p=0;p<2;p++)if(s->colorStorage[p]&&(s->colorStorage[p]>3||s->colorOffset[p]>=s->stride||s->colorType[p]>5||(s->colorStorage[p]>1&&!s->colorStride[p])))return 0;
    for(p=0;p<8;p++)if(s->lightMask&(1u<<p)){
      const AuroraMeshLight* l=s->lights+p;
      for(n=0;n<3;n++)if(!(l->position[n]<=DBL_MAX&&l->position[n]>=-DBL_MAX&&l->direction[n]<=DBL_MAX&&l->direction[n]>=-DBL_MAX&&l->cosine[n]<=DBL_MAX&&l->cosine[n]>=-DBL_MAX&&l->distance[n]<=DBL_MAX&&l->distance[n]>=-DBL_MAX))return 0;
      for(n=0;n<4;n++)if(!(l->color[n]>=0&&l->color[n]<=1))return 0;
    }
  }
  if(s->uvMask){
    static const uint32_t width[5]={1,1,2,2,4};
    if(s->uvMask>255||!s->normalComponents)return 0;
    for(p=0;p<8;p++)if(s->uvMask&(1u<<p)){
      const AuroraMeshUv* u=s->uv+p;uint32_t bytes;
      if(u->storage<1||u->storage>3||u->components<1||u->components>2||u->type>4||u->fraction>31||
         u->offset>=s->stride||(u->storage>1&&!u->stride))return 0;
      bytes=u->storage==1?width[u->type]*u->components:u->storage==2?1:2;
      if(bytes>s->stride-u->offset)return 0;
      for(n=0;n<s->primitives;n++)for(c=0;c<s->primitive[n].points;c++){
        const double* uv=s->primitive[n].point[c].uv[p];
        if(!(uv[0]<=DBL_MAX&&uv[0]>=-DBL_MAX&&uv[1]<=DBL_MAX&&uv[1]>=-DBL_MAX)||
           (u->components==1&&uv[1]!=0))return 0;
      }
    }
  }
  if(s->texgenMask){
    if(s->texgenMask>255||!s->uvMask||!s->lightingReference)return 0;
    for(p=0;p<8;p++)if(s->texgenMask&(1u<<p)){
      const AuroraMeshTexgen* t=s->texgen+p;
      if(t->source>7||!(s->uvMask&(1u<<t->source))||t->postMatrix<64||t->postMatrix>121||(t->postMatrix-64)%3)return 0;
      if(t->sourceSrtBytes&&t->sourceSrtBytes!=36)return 0;
      for(c=0;c<12;c++)if(!(t->matrix[c]<=DBL_MAX&&t->matrix[c]>=-DBL_MAX))return 0;
    }
  }
  return 1;
}
