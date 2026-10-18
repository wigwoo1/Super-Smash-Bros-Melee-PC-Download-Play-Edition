#ifndef MELEE_PC_FOX_VIWAIT_MOTION_H
#define MELEE_PC_FOX_VIWAIT_MOTION_H
/* Validate deferred embedded archives on scratch; preserve source body bytes. */
static int MeleePc_FoxViWaitMotion(const unsigned char* bytes,size_t length){
 if(length!=5154)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=5077 || MeleePc_DatBe32(b+4)!=4508 || MeleePc_DatBe32(b+8)!=123 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5032)!=4488)return -1;
 uint32_t no=MeleePc_DatBe32(b+5036);
 if(no>=37 || !memchr(b+5040+no,0,37-no) || strcmp((const char*)b+5040+no,"PlyFox5K_Share_ACTION_Wait1_figatree"))return -1;
 unsigned char* scratch=malloc(5077);if(!scratch)return -1;memcpy(scratch,b,5077);
 int result=MeleePc_FigaTreeBody(scratch,5077,4508,123,4488);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
