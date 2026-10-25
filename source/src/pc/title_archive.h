#ifndef MELEE_PC_TITLE_ARCHIVE_H
#define MELEE_PC_TITLE_ARCHIVE_H
#include <stdlib.h>
#include <math.h>
#include "title_layouts.h"
#include "game_over_scene_profile.h"
#include "trophy_model_profile.h"
/* Original offsets remain immutable until every typed claim is validated.
 * Packed GX/animation streams are byte payloads, never scalar arrays. */
typedef struct { uint32_t at; unsigned type; } MeleePc_TitleNode;
typedef struct {
    unsigned char* bytes; uint32_t size;
    unsigned char *claims,*relocs,*seen;
    const unsigned char* external_slots;
    MeleePc_TitleNode* nodes; size_t count,head,capacity;
    int error;
    /* Exact source-audited sound parameter alias only. Runtime must retain
     * pre-conversion bytes for sound decoding; default zero forbids aliases. */
    uint32_t sound_alias_word;
    unsigned sound_alias_enabled;
} MeleePc_TitlePlan;
static uint32_t title_u32(MeleePc_TitlePlan* p,uint32_t at){return MeleePc_DatBe32(p->bytes+32+at);}
static unsigned title_u16(MeleePc_TitlePlan* p,uint32_t at){return (p->bytes[32+at]<<8)|p->bytes[33+at];}
static int title_range(MeleePc_TitlePlan* p,uint32_t at,uint32_t size){
    if(at>p->size || size>p->size-at){p->error=-1;return 0;}return 1;
}
static int title_claim(MeleePc_TitlePlan* p,uint32_t at,uint32_t size,unsigned width){
    if(!title_range(p,at,size) || (width!=1 && ((at%width)||(size%width)))){p->error=-1;return 0;}
    if(p->sound_alias_enabled &&
       (at==p->sound_alias_word ||
        (at==p->sound_alias_word+4 && title_u32(p,at)==0)) && size==4 && width==1 &&
       p->claims[at]==4 && p->claims[at+1]==4 && p->claims[at+2]==4 && p->claims[at+3]==4)
        return 1; /* Native scalar wins; packed consumer uses original snapshot. */
    for(uint32_t i=0;i<size;++i)if(p->claims[at+i] && p->claims[at+i]!=width){
        p->error=-1;return 0;}
    memset(p->claims+at,width,size);return 1;
}
static uint32_t title_pointer(MeleePc_TitlePlan* p,uint32_t slot){
    if(!title_claim(p,slot,4,4))return UINT32_MAX;
    uint32_t value=title_u32(p,slot);
    /* Only a validated external-chain bitmap may install this field. The
     * original lbArchive startup binds imports to NULL after ArchiveParse.
     * Claim the link word for endian conversion without treating its -1 or
     * next-slot value as an in-archive descriptor target. */
    if(p->external_slots && p->external_slots[slot/4]){
        if(p->relocs[slot/4])p->error=-1;
        return UINT32_MAX;
    }
    if(p->relocs[slot/4]){
        if(value>p->size){p->error=-1;return UINT32_MAX;}
        p->relocs[slot/4]=2;return value;
    }
    if(value){
        p->error=-1;}
    return UINT32_MAX;
}
static void title_enqueue(MeleePc_TitlePlan* p,uint32_t at,unsigned type){
    if(at==UINT32_MAX || p->error)return;
    if(!type || type>=sizeof(title_types)/sizeof(title_types[0]) || (at&3) ||
       !title_range(p,at,title_types[type].size) || p->count==p->capacity){p->error=-1;return;}
    p->nodes[p->count++]=(MeleePc_TitleNode){at,type};
}
static void title_array(MeleePc_TitlePlan* p,uint32_t at,unsigned type,unsigned count,int terminated){
    if(at==UINT32_MAX){if(count && !terminated)p->error=-1;return;}
    for(unsigned i=0;i<(terminated ? p->size/4 : count) && !p->error;++i){
        if(i>UINT32_MAX/4 || at>UINT32_MAX-i*4){p->error=-1;return;}
        uint32_t target=title_pointer(p,at+i*4);
        if(terminated && target==UINT32_MAX)return;
        title_enqueue(p,target,type);
    }
    if(terminated && !p->error)p->error=-1;
}
static int title_image_size(MeleePc_TitlePlan* p,uint32_t at,uint32_t* size){
    unsigned w=title_u16(p,at+4),h=title_u16(p,at+6),fmt=title_u32(p,at+8);
    uint32_t lod_bits=title_u32(p,at+20);float lod;memcpy(&lod,&lod_bits,4);
    unsigned tw=4,th=4,tb=32;
    switch(fmt){case 0:case 8:case 14:tw=8;th=8;break;case 1:case 2:case 9:tw=8;break;
    case 3:case 4:case 5:case 10:break;case 6:tb=64;break;default:return -2;}
    if(!w || !h || !isfinite(lod) || lod<0 || lod>254 || lod!=(unsigned)lod)return -1;
    unsigned levels=title_u32(p,at+12) ? (unsigned)lod+1 : 1;uint64_t total=0;
    for(unsigned i=0;i<levels;++i){total+=(uint64_t)((w+tw-1)/tw)*((h+th-1)/th)*tb;
        if(total>UINT32_MAX)return -1;if(w==1 && h==1)break;w=w>1?w/2:1;h=h>1?h/2:1;}
    *size=(uint32_t)total;return 0;
}
static void title_payload(MeleePc_TitlePlan* p,uint32_t target,uint32_t size,unsigned width){
    if(target==UINT32_MAX){if(size)p->error=-1;return;}title_claim(p,target,size,width);
}
/* Shape index/coordinate arrays stay GX BE. Only the counted pointer table
 * becomes native; the CPU reader explicitly decodes the coordinate scalars. */
static void title_shape_indices(MeleePc_TitlePlan* p,uint32_t at,unsigned off,uint32_t table){
    unsigned count=title_u16(p,at+2);
    uint32_t n=title_u32(p,at+(off==12?4:16));
    uint32_t desc=title_u32(p,at+(off==12?8:20));
    if(!count || count>1024 || n>p->size){p->error=-1;return;}
    if(table==UINT32_MAX){if(n)p->error=-1;return;}
    if(!p->relocs[(at+(off==12?8:20))/4] || !title_range(p,desc,24) ||
       !title_range(p,table,count*4)){p->error=-1;return;}
    unsigned attr=title_u32(p,desc),kind=title_u32(p,desc+4),
             comp=title_u32(p,desc+8),format=title_u32(p,desc+12),
             stride=title_u16(p,desc+18);
    unsigned components=attr==25?9:3,scalar=format<2?1:format<4?2:4;
    if((off==12 && (attr!=9 || comp!=1)) ||
       (off==24 && ((attr!=10 && attr!=25) || comp!=0)) ||
       (kind!=2 && kind!=3) || format>4 || stride<components*scalar){p->error=-2;return;}
    unsigned width=kind==3?2:1;
    if(n>p->size/width || !p->relocs[(desc+20)/4]){p->error=-1;return;}
    uint32_t base=title_u32(p,desc+20);
    for(unsigned i=0;i<count && !p->error;++i){
        uint32_t index=title_pointer(p,table+i*4);
        if(index==UINT32_MAX || !title_range(p,index,n*width)){p->error=-1;return;}
        unsigned maximum=0;
        for(uint32_t j=0;j<n;++j){unsigned v=width==2?title_u16(p,index+j*2):p->bytes[32+index+j];if(v>maximum)maximum=v;}
        uint64_t extent=n?(uint64_t)maximum*stride+components*scalar:0;
        if(extent>UINT32_MAX || !title_range(p,base,(uint32_t)extent)){p->error=-1;return;}
        /* A relocation inside packed data would corrupt index/float bytes. */
        uint32_t starts[2]={index,base},sizes[2]={n*width,(uint32_t)extent};
        for(unsigned k=0;k<2;++k)if(sizes[k])for(uint32_t s=starts[k]/4;s<=(starts[k]+sizes[k]-1)/4;++s)
            if(p->relocs[s] || (p->external_slots && p->external_slots[s])){p->error=-1;return;}
        title_payload(p,index,n*width,1);title_payload(p,base,(uint32_t)extent,1);
    }
}
static void title_envelopes(MeleePc_TitlePlan* p,uint32_t table){
    if(table==UINT32_MAX){p->error=-1;return;}
    for(unsigned i=0;i<1024 && !p->error;++i){
        if(table>UINT32_MAX-i*4){p->error=-1;return;}
        uint32_t list=title_pointer(p,table+i*4);if(list==UINT32_MAX)return;
        unsigned j;
        for(j=0;j<1024 && !p->error;++j){
            if(list>UINT32_MAX-j*8){p->error=-1;return;}
            uint32_t entry=list+j*8,joint=title_pointer(p,entry);if(joint==UINT32_MAX)break;
            if(!title_range(p,entry+4,4) || p->relocs[(entry+4)/4] ||
               (p->external_slots && p->external_slots[(entry+4)/4])){p->error=-1;return;}
            if(!title_claim(p,entry+4,4,4))return;
            uint32_t bits=title_u32(p,entry+4);float weight;memcpy(&weight,&bits,4);
            if(!isfinite(weight)){p->error=-1;return;}title_enqueue(p,joint,TITLE_HSD_Joint);
        }
        if(j==1024)p->error=-1;
    }
    if(!p->error)p->error=-1;
}
/* HSD_RObjDesc's active union is selected by its source-declared flags. The
 * native runtime loads joint links, limits, bytecode expressions and IK hints;
 * ordinary function expressions remain unsupported until their function
 * import ownership is represented by the archive plan. */
static void title_joint_constraints(MeleePc_TitlePlan* p,uint32_t at){
    if(at==UINT32_MAX || p->error)return;
    unsigned char* chain=calloc((p->size+3)/4,1);
    if(!chain){p->error=-1;return;}
    while(at!=UINT32_MAX && !p->error){
        if((at&3) || !title_range(p,at,12) || chain[at/4]){p->error=-1;break;}
        chain[at/4]=1;
        if(p->relocs[(at+4)/4] || (p->external_slots &&
           (p->external_slots[at/4] || p->external_slots[(at+4)/4] || p->external_slots[(at+8)/4]))){p->error=-1;break;}
        if(!title_claim(p,at+4,4,4))break;
        uint32_t flags=title_u32(p,at+4),kind=flags&0x70000000,next=title_pointer(p,at);
        if(kind==0x10000000){
            uint32_t joint=title_pointer(p,at+8);
            if(joint==UINT32_MAX)p->error=-1;
            else title_enqueue(p,joint,TITLE_HSD_Joint);
        }else if(kind==0x20000000){
            uint32_t bits=title_u32(p,at+8);float limit;memcpy(&limit,&bits,4);
            if(p->relocs[(at+8)/4] || (p->external_slots && p->external_slots[(at+8)/4]) ||
               !isfinite(limit) || !title_claim(p,at+8,4,4))p->error=-1;
        }else if(kind==0x30000000){
            uint32_t exp=title_pointer(p,at+8);
            if(exp!=UINT32_MAX){
                if((exp&3) || !title_range(p,exp,8) || (p->external_slots &&
                   (p->external_slots[exp/4] || p->external_slots[(exp+4)/4]))){p->error=-1;break;}
                uint32_t code=title_pointer(p,exp),list=title_pointer(p,exp+4),count=0;
                if(list!=UINT32_MAX){
                    if(list&3){p->error=-1;break;}
                    for(;;){
                        if(!title_range(p,list,8) || p->relocs[list/4] ||
                           (p->external_slots && (p->external_slots[list/4] || p->external_slots[(list+4)/4]))){p->error=-1;break;}
                        if(!title_claim(p,list,4,4))break;
                        uint32_t joint=title_pointer(p,list+4);
                        if(joint==UINT32_MAX)break;
                        title_enqueue(p,joint,TITLE_HSD_Joint);count++;list+=8;
                        if(p->error)break;
                    }
                }
                if(code!=UINT32_MAX && !p->error){
                    if(!title_range(p,code,4))break;
                    const unsigned char* script=p->bytes+32+code;
                    if(script[0]!=2 || script[3]!=1){p->error=-2;break;}
                    if((((unsigned)script[1]<<8)|script[2])>=count){p->error=-1;break;}
                    for(uint32_t slot=code&~3u;slot<code+4;slot+=4)
                        if(p->relocs[slot/4] || (p->external_slots && p->external_slots[slot/4]))p->error=-1;
                    if(!p->error)title_claim(p,code,4,1);
                }
            }
        }else if(kind==0x40000000){
            uint32_t hint=title_pointer(p,at+8);
            if(hint==UINT32_MAX || (hint&3) || !title_range(p,hint,8))p->error=-1;
            else if(p->relocs[hint/4] || p->relocs[(hint+4)/4] ||
                    (p->external_slots && (p->external_slots[hint/4] || p->external_slots[(hint+4)/4])))p->error=-1;
            else {
                float bone_length,rotate_x;uint32_t a=title_u32(p,hint),b=title_u32(p,hint+4);
                memcpy(&bone_length,&a,4);memcpy(&rotate_x,&b,4);
                if(!isfinite(bone_length) || !isfinite(rotate_x) || !title_claim(p,hint,8,4))p->error=-1;
            }
        }else{
            /* REFTYPE_EXP contains a function pointer whose import is not
             * safely classified by the current title archive plan. */
            p->error=-2;
        }
        at=next;
    }
    free(chain);
}
static void title_constraint_animations(MeleePc_TitlePlan* p,uint32_t at){
    if(at==UINT32_MAX || p->error)return;
    unsigned char* chain=calloc((p->size+3)/4,1);
    if(!chain){p->error=-1;return;}
    while(at!=UINT32_MAX && !p->error){
        if((at&3) || !title_range(p,at,8) || chain[at/4]){p->error=-1;break;}
        chain[at/4]=1;
        if(p->external_slots && (p->external_slots[at/4] || p->external_slots[(at+4)/4])){p->error=-2;break;}
        uint32_t next=title_pointer(p,at);
        title_enqueue(p,title_pointer(p,at+4),TITLE_HSD_AObjDesc);at=next;
    }
    free(chain);
}
static void title_nested_archive(MeleePc_TitlePlan* p,uint32_t at){
    uint32_t length,body,relocs,publics,externs;
    size_t cursor;
    if(at==UINT32_MAX || !title_range(p,at,32)){p->error=-1;return;}
    length=title_u32(p,at);body=title_u32(p,at+4);
    relocs=title_u32(p,at+8);publics=title_u32(p,at+12);externs=title_u32(p,at+16);
    if(length<32 || length>p->size-at || body>length-32){p->error=-1;return;}
    cursor=(size_t)at+32+body;
    if(relocs>(at+length-cursor)/4){p->error=-1;return;}
    for(uint32_t i=0;i<relocs;++i){
        uint32_t slot=title_u32(p,(uint32_t)cursor+i*4);
        if((slot&3) || slot>body-4){p->error=-1;return;}
    }
    cursor+=(size_t)relocs*4;
    if(publics>(at+length-cursor)/8){p->error=-1;return;}
    size_t roots=cursor;cursor+=(size_t)publics*8;
    if(externs>(at+length-cursor)/8){p->error=-1;return;}
    cursor+=(size_t)externs*8;
    for(uint32_t i=0;i<publics+externs;++i){
        uint32_t entry=(uint32_t)roots+i*8;
        uint32_t name=title_u32(p,entry+4);
        size_t strings=cursor;
        if(name>=at+length-strings ||
           memchr(p->bytes+32+strings+name,0,at+length-strings-name)==NULL){
            p->error=-1;return;
        }
        if(i<publics){uint32_t value=title_u32(p,entry);
            if(value>=body || (value&3)){p->error=-1;return;}}
    }
    title_payload(p,at,length,1);
}
static uint32_t title_figatree_tracks(MeleePc_TitlePlan* p,uint32_t at){
    uint32_t nodes=title_u32(p,at+12),sum=0;
    if(nodes>=p->size){p->error=-1;return 0;}
    for(uint32_t i=0;i<p->size-nodes;++i){
        int value=(int)(signed char)p->bytes[32+nodes+i];
        if(value==-1){if(!title_claim(p,nodes,i+1,1))return 0;return sum;}
        if(value<0 || (uint32_t)value>p->size/12-sum){p->error=-1;return 0;}
        sum+=(uint32_t)value;
    }
    p->error=-1;return 0;
}
static void title_walk(MeleePc_TitlePlan* p,MeleePc_TitleNode node){
    uint32_t at=node.at;unsigned type=node.type;
    if(p->seen[at/4]){
        unsigned prior=p->seen[at/4],base=prior&0x7f;
        /* HSD_CameraAnim starts with HSD_AObjDesc* and its eye/interest
         * members are HSD_WObjAnim*. In the All-Star scene the interest
         * animation intentionally aliases the camera-animation prefix; only
         * admit that source-defined prefix when the overlapping second word
         * (CameraAnim.eye_anim / WObjAnim.robjanim) is null. */
        if(base==TITLE_HSD_CameraAnim && type==TITLE_HSD_WObjAnim &&
           !(prior&0x80) && title_u32(p,at+4)==0){p->seen[at/4]=(unsigned char)(prior|0x80);}
        else if((prior&0x80) && (type==base || type==TITLE_HSD_WObjAnim))return;
        else if(base!=type)p->error=-1;
        else return;
        if(p->error)return;
    }else p->seen[at/4]=(unsigned char)type;
    const MeleePc_TitleType* layout=title_types+type;
    /* Scalar declarations must not conceal relocations, including byte and
     * halfword fields sharing a word. Pointer declarations are handled below. */
    for(unsigned i=0;i<layout->scalar_count;++i){
        const MeleePc_TitleScalar* field=layout->scalars+i;unsigned pointer=0;
        for(unsigned j=0;j<layout->pointer_count;++j)
            if(layout->pointers[j].offset==field->offset)pointer=1;
        if(pointer)continue;
        uint32_t first=(at+field->offset)/4,last=(at+field->offset+field->width-1)/4;
        for(uint32_t slot=first;slot<=last;++slot)
            if(p->relocs[slot] || (p->external_slots && p->external_slots[slot])){p->error=-1;return;}
    }
    for(unsigned i=0;i<layout->scalar_count && !p->error;++i){const MeleePc_TitleScalar* field=layout->scalars+i;
        title_claim(p,at+field->offset,field->width,field->width);}
    for(unsigned i=0;i<layout->pointer_count && !p->error;++i){
        const MeleePc_TitlePointer* field=layout->pointers+i;
        unsigned off=field->offset;uint32_t target=title_pointer(p,at+off);
        if(type==TITLE_SceneDesc && (off==0 || off==8)){
            title_array(p,target,off==0?TITLE_DynamicModelDesc:TITLE_LightList,0,1);continue;
        }
        if(type==TITLE_DynamicModelDesc && (off==4 || off==8 || off==12)){
            unsigned child_type=off==4?TITLE_HSD_AnimJoint:
                                off==8?TITLE_HSD_MatAnimJoint:TITLE_HSD_ShapeAnimJoint;
            title_array(p,target,child_type,0,1);continue;
        }
        if(type==TITLE_SceneCameraDesc && off==0){
            if(target==UINT32_MAX || !title_range(p,target,8) ||
               title_u16(p,target+6)!=1){p->error=-2;continue;}
            title_enqueue(p,target,TITLE_HSD_CameraDescPerspective);continue;
        }
        if(type==TITLE_FigaTree && off==12){
            if(target==UINT32_MAX)p->error=-1;
            else (void)title_figatree_tracks(p,at);
            continue;
        }
        if(type==TITLE_FigaTree && off==16){
            uint32_t count=title_figatree_tracks(p,at);
            if(target==UINT32_MAX){if(count)p->error=-1;continue;}
            if(count>p->size/12 || !title_range(p,target,count*12))continue;
            for(uint32_t i=0;i<count && !p->error;++i)title_enqueue(p,target+i*12,TITLE_FigaTrack);
            continue;
        }
        if(type==TITLE_FigaTrack && off==8){
            uint32_t length=title_u16(p,at);
            if(target==UINT32_MAX){if(length)p->error=-1;}
            else title_payload(p,target,length,1);
            continue;
        }
        if((type==TITLE_SceneCameraDesc || type==TITLE_SceneFogDesc) && off==4){
            title_array(p,target,TITLE_HSD_CameraAnim,0,1);continue;
        }
        if(type==TITLE_HSD_ShapeSetDesc && (off==12 || off==24)){title_shape_indices(p,at,off,target);continue;}
        if(type==TITLE_HSD_AnimJoint && off==12){title_constraint_animations(p,target);continue;}
        if(type==TITLE_HSD_Spline){
            unsigned kind=p->bytes[32+at],count=title_u16(p,at+2);
            if(kind>3){p->error=-2;continue;}
            if(count<2 || count>32767){p->error=-1;continue;}
            uint32_t size=off==8 ? (kind==1 ? count*3-2 : count+(kind>=2?2:0))*12 :
                          off==16 ? count*4 : (count-1)*20;
            if(target==UINT32_MAX){if(off!=20 || kind)p->error=-1;}
            else title_payload(p,target,size,4);
            continue;
        }
        if(type==TITLE_HSD_PObjDesc && off==8){
            if(target!=UINT32_MAX)for(unsigned v=0;v<64 && !p->error;++v){
                uint32_t item=target+v*24;if(item<target || !title_range(p,item,4))break;
                if(title_u32(p,item)==255){title_claim(p,item,4,4);target=UINT32_MAX;break;}
                title_enqueue(p,item,TITLE_HSD_VtxDescList);
            }
            if(target!=UINT32_MAX && !p->error)p->error=-1;continue;
        }
        if(type==TITLE__HSD_TexAnim && (off==12 || off==16)){
            title_array(p,target,off==12?TITLE_HSD_ImageDesc:TITLE__HSD_TlutDesc,title_u16(p,at+(off==12?20:22)),0);continue;}
        if(type==TITLE_LightList && off==4){title_array(p,target,TITLE_HSD_LightAnim,0,1);continue;}
        if(type==TITLE_HSD_ImageDesc && off==0){uint32_t size=0;int result=title_image_size(p,at,&size);
            if(result)p->error=result;else title_payload(p,target,size,1);continue;}
        if(type==TITLE__HSD_FObjDesc && off==16){title_payload(p,target,title_u32(p,at+4),1);continue;}
        if(type==TITLE_HSD_PObjDesc && off==16){title_payload(p,target,title_u16(p,at+14)*32,1);continue;}
        if(type==TITLE_HSD_Joint && off==56){if(target!=UINT32_MAX)title_payload(p,target,48,4);continue;}
        if(type==TITLE_HSD_Joint && off==60){title_joint_constraints(p,target);continue;}
        if(type==TITLE__HSD_TlutDesc && off==0){title_payload(p,target,title_u16(p,at+12)*2,1);continue;}
        if(type==TITLE_HSD_VtxDescList && off==20){
            unsigned kind=title_u32(p,at+4),stride=title_u16(p,at+18);
            if(kind>3 || (kind>=2 && !stride)){p->error=-1;continue;}
            /* GX indexed array data remains BE. Full index extents are checked
             * by the display decoder, not guessed from adjacent allocations. */
            if(kind>=2)title_payload(p,target,stride,1);continue;
        }
        if(type==TITLE_HSD_Joint && off==16){
            unsigned flags=title_u32(p,at+4);
            if(flags&(1u<<5)){if(target!=UINT32_MAX)p->error=-2;}
            else if(flags&(1u<<14))title_enqueue(p,target,TITLE_HSD_Spline);
            else title_enqueue(p,target,TITLE_HSD_DObjDesc);continue;
        }
        if(type==TITLE_HSD_PObjDesc && off==20){
            if((title_u16(p,at+12)&0x3000)==0)title_enqueue(p,target,TITLE_HSD_Joint);
            else if((title_u16(p,at+12)&0x3000)==0x1000)title_enqueue(p,target,TITLE_HSD_ShapeSetDesc);
            else if((title_u16(p,at+12)&0x3000)==0x2000)title_envelopes(p,target);
            else if(target!=UINT32_MAX)p->error=-2;continue;
        }
        if(type==TITLE_HSD_LightDesc && off==24){
            unsigned kind=title_u16(p,at+8)&3,attn=title_u16(p,at+10);
            if(kind==2)title_enqueue(p,target,(attn&1)?TITLE_HSD_LightAttn:TITLE_HSD_LightPointDesc);
            else if(kind==3)title_enqueue(p,target,attn?TITLE_HSD_LightAttn:TITLE_HSD_LightSpotDesc);
            /* LObjLoad does not consume the union for ambient or infinite
             * lights. Preserve its opaque target bytes; only the pointer
             * field is converted. No allocation size/type follows from it. */
            else if(target!=UINT32_MAX)title_range(p,target,1);
            continue;
        }
        if(target==UINT32_MAX)continue;
        if(field->target>0)title_enqueue(p,target,(unsigned)field->target);
        else if(field->target==-1){if(!title_range(p,target,1))continue;
            unsigned char* end=memchr(p->bytes+32+target,0,p->size-target);
            if(!end)p->error=-1;else title_claim(p,target,(uint32_t)(end-(p->bytes+32+target))+1,1);
        }else p->error=-2;
    }
}
static int title_finish_plan(MeleePc_TitlePlan* p,unsigned allow_untyped_local_relocations){
    while(p->head<p->count && !p->error){MeleePc_TitleNode node=p->nodes[p->head++];
        title_walk(p,node);
    }
    /* A DAT relocation table contains only slot offsets. For the explicitly
     * gated Sheik archive, remaining local relocation words may be converted
     * generically only when bounded and disjoint from every typed/packed claim.
     * Other archive profiles retain the strict source-typed ownership gate. */
    for(uint32_t i=0;i<(p->size+3)/4 && !p->error;++i)if(p->relocs[i]==1){
        if(!allow_untyped_local_relocations){p->error=-2;break;}
        uint32_t at=i*4;
        if(!title_range(p,at,4) || (p->external_slots && p->external_slots[i])){p->error=-1;break;}
        if(p->claims[at] || p->claims[at+1] || p->claims[at+2] || p->claims[at+3]){p->error=-1;break;}
        if(title_u32(p,at)>=p->size){p->error=-1;break;}
        if(!title_claim(p,at,4,4)){p->error=-1;break;}
        p->relocs[i]=2;
    }
    if(!p->error)for(uint32_t at=0;at<p->size;){unsigned width=p->claims[at];
        if(width==2){uint16_t v=(uint16_t)title_u16(p,at);memcpy(p->bytes+32+at,&v,2);at+=2;}
        else if(width==4){MeleePc_DatSwap32(p->bytes+32+at);at+=4;}else ++at;}
    return p->error;
}
static int MeleePc_TitleBody(unsigned char* bytes,uint32_t body,uint32_t relocs,uint32_t publics,size_t roots,size_t strings){
    if(!body || body>64*1024*1024 || relocs>body/4)return -1;
    MeleePc_TitlePlan p={0};p.bytes=bytes;p.size=body;p.capacity=(size_t)relocs+publics+body/24+1;
    p.claims=calloc(body,1);p.relocs=calloc((body+3)/4,1);p.seen=calloc((body+3)/4,1);
    p.nodes=malloc(p.capacity*sizeof(*p.nodes));
    if(!p.claims || !p.relocs || !p.seen || !p.nodes)p.error=-1;
    for(uint32_t i=0;i<relocs && !p.error;++i){uint32_t slot=MeleePc_DatBe32(bytes+32+body+i*4);
        if((slot&3) || !title_range(&p,slot,4) || p.relocs[slot/4])p.error=-1;
        else p.relocs[slot/4]=1;}
    /* Trophy model assets use the same typed Joint graph as the title models.
     * Admit only a single ToyDsp Joint/MatAnim root or the two-root pair so this
     * graph walker does not become a generic decoder for unknown archives. */
    unsigned toy_joint=0,toy_matanim=0;
    int toy_model=(publics==1 || publics==2);
    if(toy_model)for(uint32_t i=0;i<publics;++i){
        const char* name=(const char*)bytes+strings+MeleePc_DatBe32(bytes+roots+i*8+4);
        size_t len=strlen(name);
        if(strncmp(name,"ToyDsp",6)){toy_model=0;break;}
        if(len>=14 && !strcmp(name+len-14,"_matanim_joint"))toy_matanim++;
        else if(len>=6 && !strcmp(name+len-6,"_joint"))toy_joint++;
        else {toy_model=0;break;}
    }
    if(toy_model && (publics==1 ?
       (toy_joint+toy_matanim!=1) :
       (toy_joint!=1 || toy_matanim!=1)))toy_model=0;
    for(uint32_t i=0;i<publics && !p.error;++i){
        uint32_t at=MeleePc_DatBe32(bytes+roots+i*8);
        const char* name=(const char*)bytes+strings+MeleePc_DatBe32(bytes+roots+i*8+4);unsigned type=0;
        if(!strcmp(name,"ScTitle_cam_int1_camera")){if(!title_range(&p,at,8))break;
            if(title_u16(&p,at+6)!=1){p.error=-2;break;}type=TITLE_HSD_CameraDescPerspective;}
        else if(!strcmp(name,"ScTitle_cam_int1_camanim"))type=TITLE_HSD_CameraAnim;
        else if(!strcmp(name,"ScTitle_fog"))type=TITLE_HSD_FogDesc;
        else if(!strcmp(name,"ScTitle_scene_lights")){title_array(&p,at,TITLE_LightList,0,1);continue;}
        else if(!strcmp(name,"TitleMark_sobjdesc"))type=TITLE_HSD_SObjDesc;
        else if(!strcmp(name,"ScItrAllstar_scene_data"))type=TITLE_SceneDesc;
        else if(MeleePc_IsGameOverSceneProfile(bytes,body,relocs,publics,roots,strings))type=TITLE_SceneDesc;
        else if(strlen(name)>=9 && !strcmp(name+strlen(name)-9,"_figatree"))type=TITLE_FigaTree;
        else if(!strncmp(name,"ftDemoIntroMotionFile",21)){
            title_nested_archive(&p,at);continue;
        }
        else if(MeleePc_IsStandardTrophyModelProfile(bytes,body,relocs,publics,roots,strings))type=TITLE_HSD_Joint;
        else if(toy_model){
            size_t len=strlen(name);
            if(len>=14 && !strcmp(name+len-14,"_matanim_joint"))type=TITLE_HSD_MatAnimJoint;
            else if(len>=6 && !strcmp(name+len-6,"_joint"))type=TITLE_HSD_Joint;
        }
        else {const char* suffix=NULL;
            if(!strncmp(name,"TtlBg_Top",9))suffix=name+9;
            else if(!strncmp(name,"TtlMoji_Top",11))suffix=name+11;
            if(suffix){if(!strcmp(suffix,"_joint"))type=TITLE_HSD_Joint;
                else if(!strcmp(suffix,"_animjoint"))type=TITLE_HSD_AnimJoint;
                else if(!strcmp(suffix,"_matanim_joint"))type=TITLE_HSD_MatAnimJoint;
                else if(!strcmp(suffix,"_shapeanim_joint"))type=TITLE_HSD_ShapeAnimJoint;}}
        if(!type){p.error=-2;break;}title_enqueue(&p,at,type);
    }
    /* GmRegClr's AVERAGE shape set consumes ids0..10 in pobj.c. Its export
     * retains an unused twelfth index directory entry for both vertex and
     * normal tables. Pin those entries; do not increase the runtime count or
     * infer arbitrary adjacent pointer arrays for other archives. */
    if(!p.error && body==183304 && relocs==579 &&
       MeleePc_IsGameOverSceneProfile(bytes,body,relocs,publics,roots,strings)){
        if(title_u16(&p,76216)!=5 || title_u16(&p,76218)!=11 ||
           title_u32(&p,76220)!=441 || title_u32(&p,76228)!=66396 ||
           title_u32(&p,76232)!=441 || title_u32(&p,76240)!=76168 ||
           title_pointer(&p,66440)!=65512 || title_pointer(&p,76212)!=75284)
            p.error=-1;
        if(!p.error){title_payload(&p,65512,882,1);title_payload(&p,75284,882,1);}
    }
    if(!p.error && MeleePc_IsStandardTrophyModelProfile(bytes,body,relocs,publics,roots,strings)){
        /* Exact retained model/list, not a second scene or an animation list. */
        uint32_t model=body-24, root=MeleePc_DatBe32(bytes+roots);
        if(title_u32(&p,model)!=root || title_u32(&p,model+4)!=0 ||
           title_u32(&p,model+8)!=0 || title_u32(&p,model+12)!=0 ||
           title_u32(&p,model+16)!=model || title_u32(&p,model+20)!=0)p.error=-1;
        if(!p.error)title_array(&p,model+16,TITLE_DynamicModelDesc,0,1);
    }
    title_finish_plan(&p,0);
    free(p.claims);free(p.relocs);free(p.seen);free(p.nodes);return p.error;
}
static int MeleePc_IsAllstarRootSet(unsigned char* bytes,uint32_t publics,size_t roots,size_t strings){
    unsigned scene=0,intros=0;
    if(publics!=28)return 0;
    for(uint32_t i=0;i<publics;++i){
        const char* name=(const char*)bytes+strings+MeleePc_DatBe32(bytes+roots+i*8+4);
        if(!strcmp(name,"ScItrAllstar_scene_data")){scene++;continue;}
        if(!strncmp(name,"ftDemoIntroMotionFile",21)){intros++;continue;}
        return 0;
    }
    return scene==1 && intros==27;
}
#endif
