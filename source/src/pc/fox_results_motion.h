#ifndef MELEE_PC_FOX_RESULTS_MOTION_H
#define MELEE_PC_FOX_RESULTS_MOTION_H
/* Validate deferred embedded archives on scratch; preserve source body bytes. */
static int MeleePc_FoxResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=65506)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=9440 || MeleePc_DatBe32(b+4)!=8748 || MeleePc_DatBe32(b+8)!=154 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+9396)!=8728)return -1;
 uint32_t no=MeleePc_DatBe32(b+9400);
 if(no>=36 || !memchr(b+9404+no,0,36-no) || strcmp((const char*)b+9404+no,"PlyFox5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(9440);if(!scratch)return -1;memcpy(scratch,b,9440);
 int result=MeleePc_FigaTreeBody(scratch,9440,8748,154,8728);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+42368;
 if(MeleePc_DatBe32(b)!=4724 || MeleePc_DatBe32(b+4)!=4028 || MeleePc_DatBe32(b+8)!=154 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4676)!=4008)return -1;
 uint32_t no=MeleePc_DatBe32(b+4680);
 if(no>=40 || !memchr(b+4684+no,0,40-no) || strcmp((const char*)b+4684+no,"PlyFox5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4724);if(!scratch)return -1;memcpy(scratch,b,4724);
 int result=MeleePc_FigaTreeBody(scratch,4724,4028,154,4008);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+9440;
 if(MeleePc_DatBe32(b)!=10840 || MeleePc_DatBe32(b+4)!=10148 || MeleePc_DatBe32(b+8)!=154 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+10796)!=10128)return -1;
 uint32_t no=MeleePc_DatBe32(b+10800);
 if(no>=36 || !memchr(b+10804+no,0,36-no) || strcmp((const char*)b+10804+no,"PlyFox5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(10840);if(!scratch)return -1;memcpy(scratch,b,10840);
 int result=MeleePc_FigaTreeBody(scratch,10840,10148,154,10128);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+47104;
 if(MeleePc_DatBe32(b)!=4217 || MeleePc_DatBe32(b+4)!=3520 || MeleePc_DatBe32(b+8)!=154 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4168)!=3500)return -1;
 uint32_t no=MeleePc_DatBe32(b+4172);
 if(no>=41 || !memchr(b+4176+no,0,41-no) || strcmp((const char*)b+4176+no,"PlyFox5K_Share_ACTION_Win2Wait1_figatree"))return -1;
 unsigned char* scratch=malloc(4217);if(!scratch)return -1;memcpy(scratch,b,4217);
 int result=MeleePc_FigaTreeBody(scratch,4217,3520,154,3500);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+51328;
 if(MeleePc_DatBe32(b)!=5305 || MeleePc_DatBe32(b+4)!=4608 || MeleePc_DatBe32(b+8)!=154 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5256)!=4588)return -1;
 uint32_t no=MeleePc_DatBe32(b+5260);
 if(no>=41 || !memchr(b+5264+no,0,41-no) || strcmp((const char*)b+5264+no,"PlyFox5K_Share_ACTION_Win2Wait2_figatree"))return -1;
 unsigned char* scratch=malloc(5305);if(!scratch)return -1;memcpy(scratch,b,5305);
 int result=MeleePc_FigaTreeBody(scratch,5305,4608,154,4588);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+20288;
 if(MeleePc_DatBe32(b)!=7576 || MeleePc_DatBe32(b+4)!=6884 || MeleePc_DatBe32(b+8)!=154 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+7532)!=6864)return -1;
 uint32_t no=MeleePc_DatBe32(b+7536);
 if(no>=36 || !memchr(b+7540+no,0,36-no) || strcmp((const char*)b+7540+no,"PlyFox5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(7576);if(!scratch)return -1;memcpy(scratch,b,7576);
 int result=MeleePc_FigaTreeBody(scratch,7576,6884,154,6864);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+56640;
 if(MeleePc_DatBe32(b)!=4228 || MeleePc_DatBe32(b+4)!=3532 || MeleePc_DatBe32(b+8)!=154 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4180)!=3512)return -1;
 uint32_t no=MeleePc_DatBe32(b+4184);
 if(no>=40 || !memchr(b+4188+no,0,40-no) || strcmp((const char*)b+4188+no,"PlyFox5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4228);if(!scratch)return -1;memcpy(scratch,b,4228);
 int result=MeleePc_FigaTreeBody(scratch,4228,3532,154,3512);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+27872;
 if(MeleePc_DatBe32(b)!=8236 || MeleePc_DatBe32(b+4)!=7540 || MeleePc_DatBe32(b+8)!=154 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+8188)!=7520)return -1;
 uint32_t no=MeleePc_DatBe32(b+8192);
 if(no>=40 || !memchr(b+8196+no,0,40-no) || strcmp((const char*)b+8196+no,"PlyFox5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(8236);if(!scratch)return -1;memcpy(scratch,b,8236);
 int result=MeleePc_FigaTreeBody(scratch,8236,7540,154,7520);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+60896;
 if(MeleePc_DatBe32(b)!=4516 || MeleePc_DatBe32(b+4)!=3816 || MeleePc_DatBe32(b+8)!=154 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4464)!=3796)return -1;
 uint32_t no=MeleePc_DatBe32(b+4468);
 if(no>=44 || !memchr(b+4472+no,0,44-no) || strcmp((const char*)b+4472+no,"PlyFox5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(4516);if(!scratch)return -1;memcpy(scratch,b,4516);
 int result=MeleePc_FigaTreeBody(scratch,4516,3816,154,3796);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+36128;
 if(MeleePc_DatBe32(b)!=6220 || MeleePc_DatBe32(b+4)!=5528 || MeleePc_DatBe32(b+8)!=154 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6176)!=5508)return -1;
 uint32_t no=MeleePc_DatBe32(b+6180);
 if(no>=36 || !memchr(b+6184+no,0,36-no) || strcmp((const char*)b+6184+no,"PlyFox5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(6220);if(!scratch)return -1;memcpy(scratch,b,6220);
 int result=MeleePc_FigaTreeBody(scratch,6220,5528,154,5508);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
