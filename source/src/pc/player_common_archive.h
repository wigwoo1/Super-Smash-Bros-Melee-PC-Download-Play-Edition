#ifndef MELEE_PC_PLAYER_COMMON_ARCHIVE_H
#define MELEE_PC_PLAYER_COMMON_ARCHIVE_H
/* plLoadCommonData -> pl_804D6470_t. Original source declares 32-bit
 * scalar fields through +0x180, except the four byte members at +0xC0.
 * Native ABI fixture checks every field offset against that source layout. */
static int MeleePc_PlayerCommonBody(unsigned char* bytes,uint32_t body,uint32_t relocs,uint32_t root){
 static const unsigned short floats[]={0x0,0x8,0x10,0x14,0x20,0x28,0x34,0x38,
  0x44,0x48,0x4c,0x58,0x5c,0x60,0x64,0x68,0x6c,0x78,0x84,0x98,0x9c,0xa0,
  0xcc,0xd0,0xd4,0xd8,0xe8,0xf4,0xf8,0xfc,0x100,0x104,0x10c,0x114,0x11c,0x160,0x180};
 if(body!=0x188 || root!=0x184 || relocs!=1 ||
    MeleePc_DatBe32(bytes+32+body)!=root || MeleePc_DatBe32(bytes+32+root)!=0)return -1;
 for(unsigned i=0;i<sizeof(floats)/sizeof(*floats);i++)
  if((MeleePc_DatBe32(bytes+32+floats[i])&0x7f800000U)==0x7f800000U)return -1;
 for(uint32_t at=0;at<body;at+=4)if(at!=0xc0)MeleePc_DatSwap32(bytes+32+at);
 return 0;
}
#endif
