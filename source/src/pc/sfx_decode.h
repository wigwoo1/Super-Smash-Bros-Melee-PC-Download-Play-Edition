#ifndef MELEE_PC_SFX_DECODE_H
#define MELEE_PC_SFX_DECODE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
static uint32_t MeleePc_SfxBe32(const unsigned char* p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
/* Record prelude: two u32 values; each voice: 32 u16 values (AX address,
 * ADPCM coefficients/history, loop history, padding). Validate all records
 * before mutating, so truncated or invalid input remains unchanged. */
static int MeleePc_SfxDecodeRecords(void* data,size_t length,unsigned count) {
    unsigned char* bytes=data;
    size_t pos=0;
    if(!data || !count)return 0;
    for(unsigned i=0;i<count;++i){
        if(pos>length || length-pos<8)return 0;
        uint32_t voices=MeleePc_SfxBe32(bytes+pos);
        uint32_t rate=MeleePc_SfxBe32(bytes+pos+4);
        if(!voices || voices>2 || !rate || rate>96000 || length-pos-8<voices*64)return 0;
        pos+=8+voices*64;
    }
    if(pos!=length)return 0;
    pos=0;
    for(unsigned i=0;i<count;++i){
        uint32_t voices=MeleePc_SfxBe32(bytes+pos),rate=MeleePc_SfxBe32(bytes+pos+4);
        memcpy(bytes+pos,&voices,4);memcpy(bytes+pos+4,&rate,4);pos+=8;
        for(unsigned j=0;j<voices*32;++j){
            uint16_t value=(uint16_t)((bytes[pos]<<8)|bytes[pos+1]);
            memcpy(bytes+pos,&value,2);pos+=2;
        }
    }
    return 1;
}
static void MeleePc_SfxAddAddress(void* data,uint32_t delta) {
    uint16_t halves[2];memcpy(halves,data,4);
    uint32_t address=((uint32_t)halves[0]<<16)|halves[1];
    address+=delta;halves[0]=(uint16_t)(address>>16);halves[1]=(uint16_t)address;
    memcpy(data,halves,4);
}
#endif
