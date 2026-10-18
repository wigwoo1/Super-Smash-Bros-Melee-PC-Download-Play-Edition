#ifndef MELEE_PC_DRMARIO_RESULTS_MOTION_H
#define MELEE_PC_DRMARIO_RESULTS_MOTION_H
/* Validate all deferred embedded archives on scratch before mutating outer metadata. */
static int MeleePc_DrmarioResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=69158)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=11380 || MeleePc_DatBe32(b+4)!=10668 || MeleePc_DatBe32(b+8)!=158 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+11332)!=10648)return -1;
 uint32_t no=MeleePc_DatBe32(b+11336);
 if(no>=40 || !memchr(b+11340+no,0,40-no) || strcmp((const char*)b+11340+no,"PlyDrmario5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(11380);if(!scratch)return -1;memcpy(scratch,b,11380);
 int result=MeleePc_FigaTreeBody(scratch,11380,10668,158,10648);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+51872;
 if(MeleePc_DatBe32(b)!=4532 || MeleePc_DatBe32(b+4)!=3816 || MeleePc_DatBe32(b+8)!=158 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4480)!=3796)return -1;
 uint32_t no=MeleePc_DatBe32(b+4484);
 if(no>=44 || !memchr(b+4488+no,0,44-no) || strcmp((const char*)b+4488+no,"PlyDrmario5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4532);if(!scratch)return -1;memcpy(scratch,b,4532);
 int result=MeleePc_FigaTreeBody(scratch,4532,3816,158,3796);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+11392;
 if(MeleePc_DatBe32(b)!=14976 || MeleePc_DatBe32(b+4)!=14264 || MeleePc_DatBe32(b+8)!=158 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+14928)!=14244)return -1;
 uint32_t no=MeleePc_DatBe32(b+14932);
 if(no>=40 || !memchr(b+14936+no,0,40-no) || strcmp((const char*)b+14936+no,"PlyDrmario5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(14976);if(!scratch)return -1;memcpy(scratch,b,14976);
 int result=MeleePc_FigaTreeBody(scratch,14976,14264,158,14244);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+56416;
 if(MeleePc_DatBe32(b)!=4204 || MeleePc_DatBe32(b+4)!=3488 || MeleePc_DatBe32(b+8)!=158 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4152)!=3468)return -1;
 uint32_t no=MeleePc_DatBe32(b+4156);
 if(no>=44 || !memchr(b+4160+no,0,44-no) || strcmp((const char*)b+4160+no,"PlyDrmario5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4204);if(!scratch)return -1;memcpy(scratch,b,4204);
 int result=MeleePc_FigaTreeBody(scratch,4204,3488,158,3468);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+26368;
 if(MeleePc_DatBe32(b)!=9700 || MeleePc_DatBe32(b+4)!=8988 || MeleePc_DatBe32(b+8)!=158 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+9652)!=8968)return -1;
 uint32_t no=MeleePc_DatBe32(b+9656);
 if(no>=40 || !memchr(b+9660+no,0,40-no) || strcmp((const char*)b+9660+no,"PlyDrmario5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(9700);if(!scratch)return -1;memcpy(scratch,b,9700);
 int result=MeleePc_FigaTreeBody(scratch,9700,8988,158,8968);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+60640;
 if(MeleePc_DatBe32(b)!=4196 || MeleePc_DatBe32(b+4)!=3480 || MeleePc_DatBe32(b+8)!=158 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4144)!=3460)return -1;
 uint32_t no=MeleePc_DatBe32(b+4148);
 if(no>=44 || !memchr(b+4152+no,0,44-no) || strcmp((const char*)b+4152+no,"PlyDrmario5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4196);if(!scratch)return -1;memcpy(scratch,b,4196);
 int result=MeleePc_FigaTreeBody(scratch,4196,3480,158,3460);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+36096;
 if(MeleePc_DatBe32(b)!=8516 || MeleePc_DatBe32(b+4)!=7800 || MeleePc_DatBe32(b+8)!=158 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+8464)!=7780)return -1;
 uint32_t no=MeleePc_DatBe32(b+8468);
 if(no>=44 || !memchr(b+8472+no,0,44-no) || strcmp((const char*)b+8472+no,"PlyDrmario5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(8516);if(!scratch)return -1;memcpy(scratch,b,8516);
 int result=MeleePc_FigaTreeBody(scratch,8516,7800,158,7780);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+64864;
 if(MeleePc_DatBe32(b)!=4200 || MeleePc_DatBe32(b+4)!=3480 || MeleePc_DatBe32(b+8)!=158 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4144)!=3460)return -1;
 uint32_t no=MeleePc_DatBe32(b+4148);
 if(no>=48 || !memchr(b+4152+no,0,48-no) || strcmp((const char*)b+4152+no,"PlyDrmario5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(4200);if(!scratch)return -1;memcpy(scratch,b,4200);
 int result=MeleePc_FigaTreeBody(scratch,4200,3480,158,3460);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+44640;
 if(MeleePc_DatBe32(b)!=7204 || MeleePc_DatBe32(b+4)!=6492 || MeleePc_DatBe32(b+8)!=158 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+7156)!=6472)return -1;
 uint32_t no=MeleePc_DatBe32(b+7160);
 if(no>=40 || !memchr(b+7164+no,0,40-no) || strcmp((const char*)b+7164+no,"PlyDrmario5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(7204);if(!scratch)return -1;memcpy(scratch,b,7204);
 int result=MeleePc_FigaTreeBody(scratch,7204,6492,158,6472);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
