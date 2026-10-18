#ifndef MELEE_PC_STATIC_TEXTURE_H
#define MELEE_PC_STATIC_TEXTURE_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Source-owned native u16 texture words must be encoded before the ordinary
 * GX byte-stream upload. This is NOT for DAT textures,which already store BE.
 * Separate storage keeps source words immutable and repeated use idempotent.
 * The caller owns readable sourceCount words;destinationCapacity is in bytes.
 * No partial writes on invalid size/null/overlapping ranges. */
static int MeleePc_EncodeTextureU16BE(void* destination,size_t destinationCapacity,
                                   const void* sourceWords,size_t sourceCount)
{
    size_t i,bytes;uintptr_t dst=(uintptr_t)destination,src=(uintptr_t)sourceWords;
    unsigned char* output=destination;
    if(!destination||!sourceWords||!sourceCount||sourceCount>SIZE_MAX/2)return 0;
    bytes=sourceCount*2;
    if(bytes>destinationCapacity||bytes>UINTPTR_MAX-dst||bytes>UINTPTR_MAX-src)return 0;
    if(dst<src+bytes&&src<dst+bytes)return 0;
    for(i=0;i<sourceCount;i++){
        uint16_t value;memcpy(&value,(const unsigned char*)sourceWords+2*i,2);
        output[2*i]=(unsigned char)(value>>8);output[2*i+1]=(unsigned char)value;
    }
    return 1;
}
#endif
