/* Opt-in original costume binding/update/upload-source checks. Owner thread only.
 * This observes HSD's native path;it does not alter animation or renderer state. */
#ifndef HSD_TEXTURE_BINDING_DIAGNOSTIC_H
#define HSD_TEXTURE_BINDING_DIAGNOSTIC_H
#include "texture_binding_reference.h"
#include <aurora/original_texture_diagnostics.h>
typedef struct TextureBindingRecord {
    HSD_TObj* object;HSD_TexAnim* anim;HSD_AObj* aobj;
    const TextureBindingReference* reference;const unsigned char* base;
    HSD_ImageDesc* image;unsigned palette,callbacks;
    const void* last_image;const void* last_palette;
} TextureBindingRecord;
static TextureBindingRecord texture_bindings[256];
static unsigned texture_binding_count;
static int TextureBindingEnabled(void){return TextureSrtEnabled()&&getenv("MELEE_DIAGNOSTIC_TEXTURE_BINDING");}
static TextureBindingRecord* TextureBindingFind(HSD_TObj* t){unsigned n;for(n=0;n<texture_binding_count;n++)if(texture_bindings[n].object==t)return texture_bindings+n;return NULL;}
static void TextureBindingForget(HSD_TObj* t){TextureBindingRecord* r;if(!TextureBindingEnabled()||!(r=TextureBindingFind(t)))return;if(getenv("MELEE_DIAGNOSTIC_ORIGINAL_TEXTURE_GPU"))aurora_diagnostic_forget_original_texture(t);*r=texture_bindings[--texture_binding_count];memset(texture_bindings+texture_binding_count,0,sizeof(*r));}
static unsigned TextureBindingBE16(const unsigned char* p){return p[0]*256u+p[1];}
static uint64_t TextureBindingHash(const unsigned char* p,unsigned n){uint64_t h=14695981039346656037ULL;while(n--)h=(h^*p++)*1099511628211ULL;return h;}
static int TextureBindingPointer(const unsigned char* base,unsigned size,const unsigned char* raw,unsigned off,const void* pointer,unsigned bytes)
{
    unsigned target=TextureSrtBE32(raw+off);const void* original;
    return target?target<size&&bytes<=size-target&&pointer==base+target&&MeleePc_OriginalAssetBytes(pointer,bytes,&original):!pointer;
}
/* 1 = a scoped pair,0 = outside this audited body,-1 = wrong pair in it. */
static int TextureBindingPair(HSD_TObjDesc* td,HSD_TexAnim* ta,const TextureBindingReference** result,const unsigned char** body)
{
    const void *base,*owner,*other,*other_owner,*raw;unsigned size,other_size,n;uint64_t hash;int known=0;
    if(!td||!ta||!MeleePc_AssetRegionInfo(td,&base,&owner,&size)||!MeleePc_OriginalAssetBytes(base,size,&raw))return 0;
    hash=TextureBindingHash(raw,size);
    for(n=0;n<sizeof(texture_binding_reference)/sizeof(*texture_binding_reference);n++){
        const TextureBindingReference* r=texture_binding_reference+n;
        if(r->size!=size||r->body_hash!=hash)continue;known=1;
        if((const unsigned char*)td!=(const unsigned char*)base+r->texture)continue;
        if(!MeleePc_AssetRegionInfo(ta,&other,&other_owner,&other_size)||other!=base||other_owner!=owner||other_size!=size)return 0;
        if((const unsigned char*)ta==(const unsigned char*)base+r->anim){*result=r;*body=base;return 1;}
    }
    return known?-1:0;
}
static int TextureBindingCheck(HSD_TObj* t,HSD_TexAnim* ta,const unsigned char* base,unsigned size)
{
    const void* original;const unsigned char* p;unsigned n,k;
    _Static_assert(sizeof(HSD_TexAnim)==24,"original TexAnim ABI");
    if(!t||!ta||!MeleePc_OriginalAssetBytes(ta,24,&original))return 0;p=original;
    if(t->id!=TextureSrtBE32(p+4)||ta->id!=t->id||!ta->aobjdesc||!t->aobj||
       !TextureBindingPointer(base,size,p,8,ta->aobjdesc,16)||
       ta->n_imagetbl!=TextureBindingBE16(p+20)||ta->n_tluttbl!=TextureBindingBE16(p+22)||
       !ta->n_imagetbl||ta->n_imagetbl>32||ta->n_tluttbl>32||t->tlut_no!=255||
       t->imagetbl!=ta->imagetbl||!TextureBindingPointer(base,size,p,12,ta->imagetbl,4*ta->n_imagetbl))return 0;
    if(ta->n_tluttbl&&(!t->tluttbl||!TextureBindingPointer(base,size,p,16,ta->tluttbl,4*ta->n_tluttbl)))return 0;
    if(!ta->n_tluttbl&&t->tluttbl)return 0;
    for(k=0;k<2;k++){
        unsigned count=k?ta->n_tluttbl:ta->n_imagetbl;void** table=k?(void**)ta->tluttbl:(void**)ta->imagetbl;
        if(!count)continue;if(!MeleePc_OriginalAssetBytes(table,4*count,&original))return 0;p=original;
        for(n=0;n<count;n++){
            const unsigned char* desc;
            if(!TextureBindingPointer(base,size,p,4*n,table[n],k?16:24))return 0;
            if(k){HSD_Tlut* loaded=t->tluttbl[n];HSD_TlutDesc* source=table[n];
                if(!source||!loaded||!MeleePc_OriginalAssetBytes(source,16,&original))return 0;desc=original;
                if(loaded->lut!=source->lut||loaded->fmt!=TextureSrtBE32(desc+4)||source->fmt!=loaded->fmt||
                   loaded->n_entries!=TextureBindingBE16(desc+12)||source->n_entries!=loaded->n_entries||
                   !TextureBindingPointer(base,size,desc,0,source->lut,loaded->n_entries*2u))return 0;
            }
        }
    }
    return !ta->n_tluttbl||t->tluttbl[ta->n_tluttbl]==NULL;
}
static void TextureBindingBound(HSD_TObj* t,HSD_TexAnim* ta)
{
    unsigned n;HSD_TObjDesc* td=NULL;const TextureBindingReference* ref;const unsigned char* base;int pair;
    if(!TextureBindingEnabled())return;TextureBindingForget(t);
    for(n=0;n<4096;n++)if(texture_srt_records[n].object==t){td=texture_srt_records[n].descriptor;break;}
    pair=TextureBindingPair(td,ta,&ref,&base);if(!pair)return;
    if(pair<0||!TextureBindingCheck(t,ta,base,ref->size)||texture_binding_count==256){OSReport("TEXTURE_BINDING_FAIL phase=attach\n");return;}
    texture_bindings[texture_binding_count++]=(TextureBindingRecord){t,ta,t->aobj,ref,base,t->imagedesc,255,0,NULL,NULL};
    OSReport("TEXTURE_BINDING_PASS file=%s texture=%u anim=%u images=%u palettes=%u\n",ref->file,ref->texture,ref->anim,ta->n_imagetbl,ta->n_tluttbl);
}
static int TextureBindingCurrent(TextureBindingRecord* r)
{
    const void* original;return r->object&&r->object->aobj==r->aobj&&r->object->imagetbl==r->anim->imagetbl&&
        r->object->imagedesc==r->image&&r->object->tlut_no==r->palette&&
        MeleePc_OriginalAssetBytes(r->base,r->reference->size,&original);
}
static void TextureBindingUpdated(HSD_TObj* t,enum_t type,HSD_ObjData* val)
{
    TextureBindingRecord* r;int index;unsigned count;int changed=0;
    if(!TextureBindingEnabled()||(type!=HSD_A_T_TIMG&&type!=HSD_A_T_TCLT)||!(r=TextureBindingFind(t)))return;
    count=type==HSD_A_T_TIMG?r->anim->n_imagetbl:r->anim->n_tluttbl;
    if(!isfinite(val->fv)||val->fv<0||val->fv>=count){OSReport("TEXTURE_BINDING_FAIL phase=index\n");return;}index=(int)val->fv;
    if(type==HSD_A_T_TIMG){if(r->anim->imagetbl[index]){changed=r->image!=r->anim->imagetbl[index];r->image=r->anim->imagetbl[index];}}
    else{changed=r->palette!=(unsigned)index;r->palette=index;}
    r->callbacks++;
    if(!TextureBindingCurrent(r)){OSReport("TEXTURE_BINDING_FAIL phase=update\n");return;}
    if(changed||r->callbacks==1)OSReport("TEXTURE_UPDATE_PASS file=%s texture=%u type=%u index=%u callbacks=%u\n",r->reference->file,r->reference->texture,type,(unsigned)index,r->callbacks);
}
/* Called immediately before GXLoadTexObj. Validates source selection and bytes,
 * not the renderer's eventual texture cache/bind group or sampled GPU pixels. */
static int TextureBindingImage(TextureBindingRecord* r)
{
    HSD_ImageDesc* d=r->image;const void* raw;const unsigned char* p;unsigned n;uint32_t bits;
    if(!d||!MeleePc_OriginalAssetBytes(d,24,&raw))return 0;p=raw;
    if(d->width!=TextureBindingBE16(p+4)||d->height!=TextureBindingBE16(p+6)||d->format!=TextureSrtBE32(p+8)||d->mipmap!=TextureSrtBE32(p+12))return 0;
    memcpy(&bits,&d->minLOD,4);if(bits!=TextureSrtBE32(p+16))return 0;memcpy(&bits,&d->maxLOD,4);if(bits!=TextureSrtBE32(p+20))return 0;
    if(!isfinite(d->maxLOD)||d->maxLOD<0||d->maxLOD>16)return 0;
    n=GXGetTexBufferSize(d->width,d->height,d->format,d->mipmap?GX_TRUE:GX_FALSE,(u8)d->maxLOD+1);
    return n&&TextureBindingPointer(r->base,r->reference->size,p,0,d->image_ptr,n)&&MeleePc_OriginalAssetBytes(d->image_ptr,n,&raw)&&!memcmp(d->image_ptr,raw,n);
}
static void TextureBindingTag(HSD_TObj* t,GXTexObj* object){if(getenv("MELEE_DIAGNOSTIC_ORIGINAL_TEXTURE_GPU")&&!aurora_diagnostic_original_texture_object(object,t))OSReport("ORIGINAL_TEXTURE_SOURCE_FAIL reason=object-tag\n");}
static void TextureBindingSubmitted(HSD_TObj* t,GXTexObj* object)
{
    TextureBindingRecord* r;HSD_Tlut* palette=NULL;const void* original;
    if(!TextureBindingEnabled()||!(r=TextureBindingFind(t)))return;
    if(!TextureBindingCurrent(r)){OSReport("TEXTURE_BINDING_FAIL phase=submit-state\n");return;}
    if(r->palette!=255){if(!t->tluttbl||r->palette>=r->anim->n_tluttbl){OSReport("TEXTURE_BINDING_FAIL phase=submit-index\n");return;}palette=t->tluttbl[r->palette];}
    else palette=t->tlut;
    if(r->last_image==r->image&&r->last_palette==palette){TextureBindingTag(t,object);return;}
    if(!TextureBindingImage(r)||(palette&&(!palette->n_entries||!MeleePc_OriginalAssetBytes(palette->lut,palette->n_entries*2u,&original)||memcmp(palette->lut,original,palette->n_entries*2u)))){OSReport("TEXTURE_BINDING_FAIL phase=submit-source\n");return;}
    r->last_image=r->image;r->last_palette=palette;
    if(getenv("MELEE_DIAGNOSTIC_ORIGINAL_TEXTURE_GPU")){
        HSD_ImageDesc* d=r->image;const void* raw;const void* palette_raw=NULL;unsigned bytes=GXGetTexBufferSize(d->width,d->height,d->format,GX_FALSE,1);
        if((d->format!=GX_TF_CMPR&&d->format!=GX_TF_C8)||d->mipmap||d->minLOD!=0||d->maxLOD!=0||!MeleePc_OriginalAssetBytes(d->image_ptr,bytes,&raw)||
           (d->format==GX_TF_C8&&(!palette||palette->n_entries>256||palette->fmt>GX_TL_RGB5A3||!MeleePc_OriginalAssetBytes(palette->lut,palette->n_entries*2u,&palette_raw)))){
            OSReport("ORIGINAL_TEXTURE_SOURCE_FAIL reason=scope\n");
        }else{
            AuroraOriginalTexture source={d->image_ptr,raw,t,bytes,d->width,d->height,d->format,r->reference->texture,(unsigned)((const unsigned char*)d-r->base),r->reference->anim,r->reference->body_hash};
            if(d->format==GX_TF_C8){source.paletteData=palette->lut;source.paletteOriginal=palette_raw;source.paletteBytes=palette->n_entries*2u;source.paletteFormat=palette->fmt;source.paletteEntries=palette->n_entries;source.paletteOffset=(unsigned)((const unsigned char*)palette->lut-r->base);}
            if(!aurora_diagnostic_original_texture(&source))OSReport("ORIGINAL_TEXTURE_SOURCE_FAIL reason=registration\n");
        }
    }
    TextureBindingTag(t,object);
    OSReport("TEXTURE_SUBMIT_PASS file=%s texture=%u image=%u palette=%u callbacks=%u\n",r->reference->file,r->reference->texture,(unsigned)((const unsigned char*)r->image-r->base),r->palette,r->callbacks);
}
#endif
