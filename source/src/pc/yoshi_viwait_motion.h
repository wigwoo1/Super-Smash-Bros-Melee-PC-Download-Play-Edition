#ifndef MELEE_PC_YOSHI_VIWAIT_MOTION_H
#define MELEE_PC_YOSHI_VIWAIT_MOTION_H
/* Validate all deferred embedded archives on scratch before mutating outer metadata. */
static int MeleePc_YoshiViWaitMotion(const unsigned char* bytes,size_t length){
 if(length!=5732)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=5650 || MeleePc_DatBe32(b+4)!=5064 || MeleePc_DatBe32(b+8)!=127 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5604)!=5044)return -1;
 uint32_t no=MeleePc_DatBe32(b+5608);
 if(no>=38 || !memchr(b+5612+no,0,38-no) || strcmp((const char*)b+5612+no,"PlyYoshi5K_Share_ACTION_Wait_figatree"))return -1;
 unsigned char* scratch=malloc(5650);if(!scratch)return -1;memcpy(scratch,b,5650);
 int result=MeleePc_FigaTreeBody(scratch,5650,5064,127,5044);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
