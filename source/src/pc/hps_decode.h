#ifndef MELEE_PC_HPS_DECODE_H
#define MELEE_PC_HPS_DECODE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
static uint32_t MeleePc_HpsBe32(const unsigned char* p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static void MeleePc_HpsSwap(void* data,size_t begin,size_t end,size_t width) {
    unsigned char* p=data;
    for(size_t i=begin;i<end;i+=width)
        for(size_t j=0;j<width/2;++j){unsigned char t=p[i+j];p[i+j]=p[i+width-1-j];p[i+width-1-j]=t;}
}
/* HALPST: signature bytes, rate/channel u32, two 56-byte AX u16 records.
 * Validate before mutation; compressed audio is never passed to this helper. */
static int MeleePc_HpsDecodeHeader(void* data,size_t length) {
    unsigned char* p=data;
    if(!p || length!=128 || memcmp(p," HALPST\0",8))return 0;
    uint32_t rate=MeleePc_HpsBe32(p+8),channels=MeleePc_HpsBe32(p+12);
    if(!rate || rate>96000 || channels<1 || channels>2)return 0;
    for(uint32_t i=0;i<channels;++i)
        if(p[16+i*56+2] || p[16+i*56+3])return 0; /* DSP ADPCM format */
    MeleePc_HpsSwap(p,8,16,4);
    MeleePc_HpsSwap(p,16,128,2);
    return 1;
}
/* Block: byte length, final nibble index, next file offset (u32),
 * per-channel loop ADPCM history and padding (u16). */
static int MeleePc_HpsDecodeBlock(void* data,size_t length) {
    unsigned char* p=data;
    if(!p || length!=32)return 0;
    uint32_t size=MeleePc_HpsBe32(p),end=MeleePc_HpsBe32(p+4),next=MeleePc_HpsBe32(p+8);
    if(!size || size>65536 || (size&31) || end>=size*2 ||
       (next!=UINT32_MAX && (next<128 || (next&31))))return 0;
    MeleePc_HpsSwap(p,0,12,4);MeleePc_HpsSwap(p,12,32,2);
    return 1;
}
#endif
