#ifndef MELEE_PC_FALCO_VIWAIT_MOTION_H
#define MELEE_PC_FALCO_VIWAIT_MOTION_H
/* Validate deferred embedded archives on scratch; preserve source body bytes. */
static int MeleePc_FalcoViWaitMotion(const unsigned char* bytes,size_t length){
 if(length!=9860)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=9783 || MeleePc_DatBe32(b+4)!=9216 || MeleePc_DatBe32(b+8)!=122 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+9736)!=9196)return -1;
 uint32_t no=MeleePc_DatBe32(b+9740);
 if(no>=39 || !memchr(b+9744+no,0,39-no) || strcmp((const char*)b+9744+no,"PlyFalco5K_Share_ACTION_Wait1_figatree"))return -1;
 unsigned char* scratch=malloc(9783);if(!scratch)return -1;memcpy(scratch,b,9783);
 int result=MeleePc_FigaTreeBody(scratch,9783,9216,122,9196);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
