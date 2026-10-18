#ifndef MELEE_PC_ARCHIVE_LAYOUT_H
#define MELEE_PC_ARCHIVE_LAYOUT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
static uint32_t MeleePc_ArchiveWord(const unsigned char* p) {
    uint32_t value;memcpy(&value,p,4);return value;
}
/* Validate host-order metadata before absolute pointer fixups or publication.
 * This does not certify the typed body schema or GX command semantics. */
static int MeleePc_ArchiveLayout(const void* buffer,size_t length) {
    const unsigned char* p=buffer;size_t table,roots,refs,strings;
    uint32_t body,relocs,publics,externs;
    if(!p || length<32 || length>UINT32_MAX || ((uintptr_t)p&3))return 0;
    if(MeleePc_ArchiveWord(p)!=length)return 0;
    body=MeleePc_ArchiveWord(p+4);relocs=MeleePc_ArchiveWord(p+8);
    publics=MeleePc_ArchiveWord(p+12);externs=MeleePc_ArchiveWord(p+16);
    if(body>length-32)return 0;
    table=32+(size_t)body;
    if((relocs || publics || externs) && (table&3))return 0;
    if(relocs>(length-table)/4)return 0;
    roots=table+(size_t)relocs*4;
    if(publics>(length-roots)/8)return 0;
    refs=roots+(size_t)publics*8;
    if(externs>(length-refs)/8)return 0;
    strings=refs+(size_t)externs*8;
    for(size_t at=table;at<roots;at+=4) {
        uint32_t offset=MeleePc_ArchiveWord(p+at);
        if((offset&3) || body<4 || offset>body-4)return 0;
        if(MeleePc_ArchiveWord(p+32+offset)>body)return 0;
        for(size_t prev=table;prev<at;prev+=4)
            if(MeleePc_ArchiveWord(p+prev)==offset)return 0;
    }
    for(size_t at=roots;at<strings;at+=8) {
        uint32_t offset=MeleePc_ArchiveWord(p+at),symbol=MeleePc_ArchiveWord(p+at+4);
        if(symbol>=length-strings || !memchr(p+strings+symbol,0,length-strings-symbol))return 0;
        if(at<refs) { if(offset>=body)return 0; }
        else {
            uint32_t steps=0;
            while(offset!=UINT32_MAX) {
                if((offset&3) || body<4 || offset>body-4 || ++steps>body/4)return 0;
                /* An extern chain must not also receive absolute relocation. */
                for(size_t pos=table;pos<roots;pos+=4)
                    if(MeleePc_ArchiveWord(p+pos)==offset)return 0;
                offset=MeleePc_ArchiveWord(p+32+offset);
            }
        }
    }
    return 1;
}
#endif
