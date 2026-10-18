#ifndef MELEE_PC_ICE_CLIMBERS_VIWAIT_MOTION_H
#define MELEE_PC_ICE_CLIMBERS_VIWAIT_MOTION_H
/* Both climbers select this source motion; deferred original inner data. */
static int MeleePc_IceClimbersViWaitMotion(const unsigned char* bytes,size_t length){
 if(length!=10147)return -2;
 const unsigned char* b=bytes+32;
 if(MeleePc_DatBe32(b)!=10057 || MeleePc_DatBe32(b+4)!=9340 || MeleePc_DatBe32(b+8)!=160 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16))return -1;
 const size_t roots=32+9340+160*4,strings=roots+8,size=10057;
 if(MeleePc_DatBe32(b+roots)!=9320)return -1;
 uint32_t name=MeleePc_DatBe32(b+roots+4);
 if(name>=size-strings || !memchr(b+strings+name,0,size-strings-name) || strcmp((const char*)b+strings+name,"PlyPopo5K_Share_ACTION_Wait_figatree"))return -1;
 unsigned char* scratch=malloc(size);if(!scratch)return -1;memcpy(scratch,b,size);
 int result=MeleePc_FigaTreeBody(scratch,size,9340,160,9320);free(scratch);return result;
}
#endif
