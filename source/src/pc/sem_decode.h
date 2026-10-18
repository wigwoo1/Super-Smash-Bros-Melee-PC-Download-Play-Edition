#ifndef MELEE_PC_SEM_DECODE_H
#define MELEE_PC_SEM_DECODE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
static uint32_t MeleePc_SemBe32(const unsigned char* p){
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
/* SEM is five counted u32 tables followed by u32 command streams. Tables
 * 1, 3, 4 contain relative pointers. Validate before converting in place;
 * the original driver subsequently relocates those pointers exactly once. */
static int MeleePc_SemDecode(void* data,size_t length){
    unsigned char* bytes=data;size_t pos=0,starts[5];uint32_t counts[5];
    if(!data || length<20 || (length&3))return 0;
    for(unsigned table=0;table<5;++table){
        if(pos>length || length-pos<4)return 0;
        counts[table]=MeleePc_SemBe32(bytes+pos);pos+=4;starts[table]=pos;
        if(counts[table]>(length-pos)/4)return 0;
        pos+=(size_t)counts[table]*4;
    }
    for(unsigned table=0;table<5;++table){
        for(unsigned i=0;i<counts[table];++i){
            uint32_t value=MeleePc_SemBe32(bytes+starts[table]+i*4);
            if(table==1 || table==3 || table==4){
                size_t offset=value&~3u;
                if(offset<pos || offset>length-4)return 0;
            } else if(table==2 && value>counts[3])return 0;
        }
    }
    for(size_t offset=0;offset<length;offset+=4){
        uint32_t value=MeleePc_SemBe32(bytes+offset);memcpy(bytes+offset,&value,4);
    }
    return 1;
}
#endif
