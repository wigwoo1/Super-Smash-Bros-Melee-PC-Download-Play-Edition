#ifndef MELEE_PC_DRMARIO_VIWAIT_MOTION_H
#define MELEE_PC_DRMARIO_VIWAIT_MOTION_H
/* Validate all deferred embedded archives on scratch before mutating outer metadata. */
static int MeleePc_DrmarioViWaitMotion(const unsigned char* bytes,size_t length){
 if(length!=3782)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=3709 || MeleePc_DatBe32(b+4)!=3180 || MeleePc_DatBe32(b+8)!=112 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+3660)!=3160)return -1;
 uint32_t no=MeleePc_DatBe32(b+3664);
 if(no>=41 || !memchr(b+3668+no,0,41-no) || strcmp((const char*)b+3668+no,"PlyDrmario5K_Share_ACTION_Wait1_figatree"))return -1;
 unsigned char* scratch=malloc(3709);if(!scratch)return -1;memcpy(scratch,b,3709);
 int result=MeleePc_FigaTreeBody(scratch,3709,3180,112,3160);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
