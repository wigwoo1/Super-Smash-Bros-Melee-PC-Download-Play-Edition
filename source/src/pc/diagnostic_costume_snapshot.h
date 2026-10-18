#ifndef MELEE_PC_DIAGNOSTIC_COSTUME_SNAPSHOT_H
#define MELEE_PC_DIAGNOSTIC_COSTUME_SNAPSHOT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Bounded identity filter only. Full typed decoder still validates the file.
 * Return a body size only for a declared Ply..._Share_joint public symbol. */
static uint32_t MeleePc_DiagnosticBe32(const unsigned char* p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t MeleePc_DiagnosticCostumeBody(const unsigned char* p,size_t size)
{
    size_t at=32,publics,strings,i;uint32_t body,relocs,np,ne;
    static const char suffix[]="_Share_joint";
    if(!p||size<32||MeleePc_DiagnosticBe32(p)!=size)return 0;
    body=MeleePc_DiagnosticBe32(p+4);relocs=MeleePc_DiagnosticBe32(p+8);np=MeleePc_DiagnosticBe32(p+12);ne=MeleePc_DiagnosticBe32(p+16);
    if(!body||body>16*1024*1024||body>size-at)return 0;at+=body;
    if(relocs>(size-at)/4)return 0;at+=(size_t)relocs*4;publics=at;
    if(np>(size-at)/8)return 0;at+=(size_t)np*8;
    if(ne>(size-at)/8)return 0;at+=(size_t)ne*8;strings=at;
    for(i=0;i<np;i++){
        uint32_t root=MeleePc_DiagnosticBe32(p+publics+i*8),name=MeleePc_DiagnosticBe32(p+publics+i*8+4);const unsigned char* end;size_t length;
        if(root>=body||name>=size-strings)continue;
        at=strings+name;end=memchr(p+at,0,size-at);if(!end)continue;length=(size_t)(end-(p+at));
        if(length>=3+sizeof(suffix)-1&&!memcmp(p+at,"Ply",3)&&!memcmp(end-(sizeof(suffix)-1),suffix,sizeof(suffix)-1))return body;
    }
    return 0;
}
#endif
