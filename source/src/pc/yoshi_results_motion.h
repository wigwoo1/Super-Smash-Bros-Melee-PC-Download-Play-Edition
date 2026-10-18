#ifndef MELEE_PC_YOSHI_RESULTS_MOTION_H
#define MELEE_PC_YOSHI_RESULTS_MOTION_H
/* Validate all deferred embedded archives on scratch before mutating outer metadata. */
static int MeleePc_YoshiResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=83556)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=15338 || MeleePc_DatBe32(b+4)!=14588 || MeleePc_DatBe32(b+8)!=168 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+15292)!=14568)return -1;
 uint32_t no=MeleePc_DatBe32(b+15296);
 if(no>=38 || !memchr(b+15300+no,0,38-no) || strcmp((const char*)b+15300+no,"PlyYoshi5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(15338);if(!scratch)return -1;memcpy(scratch,b,15338);
 int result=MeleePc_FigaTreeBody(scratch,15338,14588,168,14568);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+64800;
 if(MeleePc_DatBe32(b)!=4590 || MeleePc_DatBe32(b+4)!=3836 || MeleePc_DatBe32(b+8)!=168 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4540)!=3816)return -1;
 uint32_t no=MeleePc_DatBe32(b+4544);
 if(no>=42 || !memchr(b+4548+no,0,42-no) || strcmp((const char*)b+4548+no,"PlyYoshi5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4590);if(!scratch)return -1;memcpy(scratch,b,4590);
 int result=MeleePc_FigaTreeBody(scratch,4590,3836,168,3816);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+15360;
 if(MeleePc_DatBe32(b)!=16310 || MeleePc_DatBe32(b+4)!=15560 || MeleePc_DatBe32(b+8)!=168 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+16264)!=15540)return -1;
 uint32_t no=MeleePc_DatBe32(b+16268);
 if(no>=38 || !memchr(b+16272+no,0,38-no) || strcmp((const char*)b+16272+no,"PlyYoshi5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(16310);if(!scratch)return -1;memcpy(scratch,b,16310);
 int result=MeleePc_FigaTreeBody(scratch,16310,15560,168,15540);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+69408;
 if(MeleePc_DatBe32(b)!=4494 || MeleePc_DatBe32(b+4)!=3740 || MeleePc_DatBe32(b+8)!=168 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4444)!=3720)return -1;
 uint32_t no=MeleePc_DatBe32(b+4448);
 if(no>=42 || !memchr(b+4452+no,0,42-no) || strcmp((const char*)b+4452+no,"PlyYoshi5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4494);if(!scratch)return -1;memcpy(scratch,b,4494);
 int result=MeleePc_FigaTreeBody(scratch,4494,3740,168,3720);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+31680;
 if(MeleePc_DatBe32(b)!=18270 || MeleePc_DatBe32(b+4)!=17520 || MeleePc_DatBe32(b+8)!=168 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+18224)!=17500)return -1;
 uint32_t no=MeleePc_DatBe32(b+18228);
 if(no>=38 || !memchr(b+18232+no,0,38-no) || strcmp((const char*)b+18232+no,"PlyYoshi5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(18270);if(!scratch)return -1;memcpy(scratch,b,18270);
 int result=MeleePc_FigaTreeBody(scratch,18270,17520,168,17500);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+73920;
 if(MeleePc_DatBe32(b)!=5062 || MeleePc_DatBe32(b+4)!=4308 || MeleePc_DatBe32(b+8)!=168 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5012)!=4288)return -1;
 uint32_t no=MeleePc_DatBe32(b+5016);
 if(no>=42 || !memchr(b+5020+no,0,42-no) || strcmp((const char*)b+5020+no,"PlyYoshi5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(5062);if(!scratch)return -1;memcpy(scratch,b,5062);
 int result=MeleePc_FigaTreeBody(scratch,5062,4308,168,4288);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+49952;
 if(MeleePc_DatBe32(b)!=9542 || MeleePc_DatBe32(b+4)!=8788 || MeleePc_DatBe32(b+8)!=168 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+9492)!=8768)return -1;
 uint32_t no=MeleePc_DatBe32(b+9496);
 if(no>=42 || !memchr(b+9500+no,0,42-no) || strcmp((const char*)b+9500+no,"PlyYoshi5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(9542);if(!scratch)return -1;memcpy(scratch,b,9542);
 int result=MeleePc_FigaTreeBody(scratch,9542,8788,168,8768);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+79008;
 if(MeleePc_DatBe32(b)!=4478 || MeleePc_DatBe32(b+4)!=3720 || MeleePc_DatBe32(b+8)!=168 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4424)!=3700)return -1;
 uint32_t no=MeleePc_DatBe32(b+4428);
 if(no>=46 || !memchr(b+4432+no,0,46-no) || strcmp((const char*)b+4432+no,"PlyYoshi5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(4478);if(!scratch)return -1;memcpy(scratch,b,4478);
 int result=MeleePc_FigaTreeBody(scratch,4478,3720,168,3700);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+59520;
 if(MeleePc_DatBe32(b)!=5262 || MeleePc_DatBe32(b+4)!=4512 || MeleePc_DatBe32(b+8)!=168 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5216)!=4492)return -1;
 uint32_t no=MeleePc_DatBe32(b+5220);
 if(no>=38 || !memchr(b+5224+no,0,38-no) || strcmp((const char*)b+5224+no,"PlyYoshi5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(5262);if(!scratch)return -1;memcpy(scratch,b,5262);
 int result=MeleePc_FigaTreeBody(scratch,5262,4512,168,4492);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
