#ifndef MELEE_PC_TROPHY_ARCHIVE_H
#define MELEE_PC_TROPHY_ARCHIVE_H
/* Included after DAT endian primitives. Types match ty/types.h: TrophyData
 * has eight 32-bit scalars plus four bytes; TyDspEntry has u32, four bytes,
 * two floats. Remaining tables are s16 arrays/ToyNameData records. */
static int MeleePc_TrophyBody(unsigned char* bytes,uint32_t body,size_t roots,size_t strings){
    static const char* names[7]={"tyInitModelTbl","tyInitModelDTbl","tyNoGetUsTbl",
        "tyExpDifferentTbl","tyModelSortTbl","tyDisplayModelTbl","tyDisplayModelUsTbl"};
    uint32_t offsets[7];unsigned seen=0;
    for(unsigned i=0;i<7;++i){
        uint32_t offset=MeleePc_DatBe32(bytes+roots+i*8);
        const char* name=(char*)bytes+strings+MeleePc_DatBe32(bytes+roots+i*8+4);
        unsigned kind;
        for(kind=0;kind<7 && strcmp(name,names[kind]);++kind){}
        if(kind==7)return -2;
        if(seen&(1u<<kind))return -1;
        seen|=1u<<kind;offsets[kind]=offset;
    }
    if(offsets[0]!=0)return -1;
    for(unsigned kind=0;kind<7;++kind){
        uint32_t end=kind==6 ? body : offsets[kind+1];
        if(offsets[kind]>=end || end>body || (offsets[kind]&3))return -1;
        unsigned stride=kind<2 ? 36 : kind<4 ? 2 : kind==4 ? 12 : 16;
        uint32_t length=end-offsets[kind],padding=length%stride;
        if(padding && !(kind<2 && padding==4))return -1;
        for(uint32_t pos=end-padding;pos<end;++pos)if(bytes[32+pos])return -1;
    }
    /* The complete conversion plan has been validated before mutation. */
    for(unsigned kind=0;kind<7;++kind){
        uint32_t end=kind==6 ? body : offsets[kind+1];
        unsigned stride=kind<2 ? 36 : kind<4 ? 2 : kind==4 ? 12 : 16;
        end-=(end-offsets[kind])%stride;
        if(kind>=2 && kind<=4){
            for(uint32_t pos=offsets[kind];pos<end;pos+=2){
                uint16_t value=(uint16_t)((bytes[32+pos]<<8)|bytes[33+pos]);
                memcpy(bytes+32+pos,&value,2);
            }
        }else for(uint32_t pos=offsets[kind];pos<end;pos+=stride){
            if(kind<2)for(unsigned word=0;word<8;++word)MeleePc_DatSwap32(bytes+32+pos+word*4);
            else {MeleePc_DatSwap32(bytes+32+pos);MeleePc_DatSwap32(bytes+40+pos);MeleePc_DatSwap32(bytes+44+pos);}
        }
    }
    return 0;
}
#endif
