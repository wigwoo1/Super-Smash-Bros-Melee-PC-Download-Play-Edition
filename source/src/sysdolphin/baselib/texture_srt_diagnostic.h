/* Original descriptor -> loaded SRT identity and independent current SRT matrix.
 * Owner-thread,opt-in,metadata only. Never exports original tuples/matrices. */
#ifndef HSD_TEXTURE_SRT_DIAGNOSTIC_H
#define HSD_TEXTURE_SRT_DIAGNOSTIC_H
#include <pc/asset_regions.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
typedef struct TextureSrtRecord {HSD_TObj* object;HSD_TObjDesc* descriptor;} TextureSrtRecord;
static TextureSrtRecord texture_srt_records[4096];
static int TextureSrtEnabled(void){static int value=-1;if(value<0)value=getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")&&getenv("MELEE_DIAGNOSTIC_TEXTURE_SRT");return value;}
static uint32_t TextureSrtBE32(const unsigned char* p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int TextureSrtSourceCheck(HSD_TObj* t,HSD_TObjDesc* d)
{
    const void* raw;unsigned c;const unsigned char* p;float* rotation;
    _Static_assert(sizeof(HSD_TObjDesc)==92,"original texture descriptor ABI");
    if(!t||!d||!MeleePc_OriginalAssetBytes(d,92,&raw))return 0;p=raw;rotation=(float*)&t->rotate;
    for(c=0;c<9;c++){
        uint32_t original=TextureSrtBE32(p+16+c*4),descriptor,loaded;
        const float value=c<3?rotation[c]:c<6?((float*)&t->scale)[c-3]:((float*)&t->translate)[c-6];
        memcpy(&descriptor,(unsigned char*)d+16+c*4,4);memcpy(&loaded,&value,4);
        if(descriptor!=original||loaded!=original||!isfinite(value))return 0;
    }
    if(d->id!=TextureSrtBE32(p+8)||t->id!=d->id||d->src!=TextureSrtBE32(p+12)||t->src!=d->src||
       d->wrap_s!=TextureSrtBE32(p+52)||t->wrap_s!=d->wrap_s||d->wrap_t!=TextureSrtBE32(p+56)||t->wrap_t!=d->wrap_t||
       d->repeat_s!=p[60]||t->repeat_s!=p[60]||d->repeat_t!=p[61]||t->repeat_t!=p[61]||
       d->blend_flags!=TextureSrtBE32(p+64)||t->flags!=(d->blend_flags|TEX_MTX_DIRTY))return 0;
    return 1;
}
static void TextureSrtLoaded(HSD_TObj* t,HSD_TObjDesc* d)
{
    const void* raw;unsigned n;if(!TextureSrtEnabled()||!MeleePc_OriginalAssetBytes(d,92,&raw))return;
    if(!TextureSrtSourceCheck(t,d)){OSReport("MESH_TEXTURE_SRT_LOAD_FAIL\n");return;}
    for(n=0;n<4096;n++)if(!texture_srt_records[n].object){texture_srt_records[n]=(TextureSrtRecord){t,d};OSReport("MESH_TEXTURE_SRT_LOADED bytes=36\n");return;}
    OSReport("MESH_TEXTURE_SRT_LOAD_FAIL reason=table-cap\n");
}
static void TextureSrtForget(HSD_TObj* t){unsigned n;if(!TextureSrtEnabled())return;for(n=0;n<4096;n++)if(texture_srt_records[n].object==t)memset(texture_srt_records+n,0,sizeof(texture_srt_records[n]));}
static int TextureSrtMatrix(const HSD_TObj* t,double out[12])
{
    double rotation[3][3]={{1,0,0},{0,1,0},{0,0,1}},scale[3],translation[3];unsigned axis,r,c,k;
    if(!t||!out||!t->repeat_s||!t->repeat_t)return 0;
    for(c=0;c<3;c++)if(!isfinite(((float*)&t->rotate)[c])||!isfinite(((float*)&t->scale)[c])||!isfinite(((float*)&t->translate)[c]))return 0;
    /* Compose elementary Rx,Ry,Rz in double,independently of HSD/MTX calls. */
    for(axis=0;axis<3;axis++){
        double elementary[3][3]={{1,0,0},{0,1,0},{0,0,1}},next[3][3]={{0}};
        double angle=((float*)&t->rotate)[axis]*(axis==2?-1:1),co=cos(angle),si=sin(angle);unsigned a=(axis+1)%3,b=(axis+2)%3;
        elementary[a][a]=elementary[b][b]=co;elementary[a][b]=-si;elementary[b][a]=si;
        for(r=0;r<3;r++)for(c=0;c<3;c++)for(k=0;k<3;k++)next[r][c]+=elementary[r][k]*rotation[k][c];
        memcpy(rotation,next,sizeof(rotation));
    }
    scale[0]=fabs(t->scale.x)<1.00000001335e-10?0:(double)t->repeat_s/t->scale.x;
    scale[1]=fabs(t->scale.y)<1.00000001335e-10?0:(double)t->repeat_t/t->scale.y;scale[2]=t->scale.z;
    translation[0]=-t->translate.x;translation[1]=-t->translate.y-(t->wrap_t==GX_MIRROR?(double)t->scale.y/t->repeat_t:0);translation[2]=t->translate.z;
    for(r=0;r<3;r++){
        out[r*4+3]=0;
        for(c=0;c<3;c++){out[r*4+c]=scale[r]*rotation[r][c];out[r*4+3]+=out[r*4+c]*translation[c];}
        for(c=0;c<4;c++)if(!isfinite(out[r*4+c]))return 0;
    }
    return 1;
}
int HSD_DiagnosticTextureSrtMatrix(HSD_TObj* t,double out[12])
{
    unsigned n,c;double expected[12];if(!TextureSrtEnabled()||!t||(t->flags&TEX_MTX_DIRTY)||!TextureSrtMatrix(t,expected))return 0;
    for(n=0;n<4096;n++)if(texture_srt_records[n].object==t)break;
    if(n==4096)return 0;
    for(c=0;c<12;c++)if(!isfinite(((float*)t->mtx)[c])||fabs(((float*)t->mtx)[c]-expected[c])>3e-5*(1+fabs(expected[c])))return 0;
    memcpy(out,expected,sizeof(expected));return 1;
}
#endif
