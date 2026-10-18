#ifndef MELEE_PC_MEWTWO_VIWAIT_MOTION_H
#define MELEE_PC_MEWTWO_VIWAIT_MOTION_H
/* Validate all deferred embedded archives on scratch before mutating outer metadata. */
static int MeleePc_MewtwoViWaitMotion(const unsigned char* bytes,size_t length){
 if(length!=7461)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=7368 || MeleePc_DatBe32(b+4)!=6736 || MeleePc_DatBe32(b+8)!=138 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+7320)!=6716)return -1;
 uint32_t no=MeleePc_DatBe32(b+7324);
 if(no>=40 || !memchr(b+7328+no,0,40-no) || strcmp((const char*)b+7328+no,"PlyMewtwo5K_Share_ACTION_Wait1_figatree"))return -1;
 unsigned char* scratch=malloc(7368);if(!scratch)return -1;memcpy(scratch,b,7368);
 int result=MeleePc_FigaTreeBody(scratch,7368,6736,138,6716);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
