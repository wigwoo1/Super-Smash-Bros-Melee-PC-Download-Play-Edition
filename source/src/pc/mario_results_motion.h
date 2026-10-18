#ifndef MELEE_PC_MARIO_RESULTS_MOTION_H
#define MELEE_PC_MARIO_RESULTS_MOTION_H
/* Validate deferred embedded archives on scratch; preserve source body bytes. */
static int MeleePc_MarioResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=56484)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=9686 || MeleePc_DatBe32(b+4)!=9032 || MeleePc_DatBe32(b+8)!=144 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+9640)!=9012)return -1;
 uint32_t no=MeleePc_DatBe32(b+9644);
 if(no>=38 || !memchr(b+9648+no,0,38-no) || strcmp((const char*)b+9648+no,"PlyMario5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(9686);if(!scratch)return -1;memcpy(scratch,b,9686);
 int result=MeleePc_FigaTreeBody(scratch,9686,9032,144,9012);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+41824;
 if(MeleePc_DatBe32(b)!=3642 || MeleePc_DatBe32(b+4)!=2984 || MeleePc_DatBe32(b+8)!=144 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+3592)!=2964)return -1;
 uint32_t no=MeleePc_DatBe32(b+3596);
 if(no>=42 || !memchr(b+3600+no,0,42-no) || strcmp((const char*)b+3600+no,"PlyMario5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(3642);if(!scratch)return -1;memcpy(scratch,b,3642);
 int result=MeleePc_FigaTreeBody(scratch,3642,2984,144,2964);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+9696;
 if(MeleePc_DatBe32(b)!=9770 || MeleePc_DatBe32(b+4)!=9116 || MeleePc_DatBe32(b+8)!=144 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+9724)!=9096)return -1;
 uint32_t no=MeleePc_DatBe32(b+9728);
 if(no>=38 || !memchr(b+9732+no,0,38-no) || strcmp((const char*)b+9732+no,"PlyMario5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(9770);if(!scratch)return -1;memcpy(scratch,b,9770);
 int result=MeleePc_FigaTreeBody(scratch,9770,9116,144,9096);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+45472;
 if(MeleePc_DatBe32(b)!=3674 || MeleePc_DatBe32(b+4)!=3016 || MeleePc_DatBe32(b+8)!=144 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+3624)!=2996)return -1;
 uint32_t no=MeleePc_DatBe32(b+3628);
 if(no>=42 || !memchr(b+3632+no,0,42-no) || strcmp((const char*)b+3632+no,"PlyMario5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(3674);if(!scratch)return -1;memcpy(scratch,b,3674);
 int result=MeleePc_FigaTreeBody(scratch,3674,3016,144,2996);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+19488;
 if(MeleePc_DatBe32(b)!=9342 || MeleePc_DatBe32(b+4)!=8688 || MeleePc_DatBe32(b+8)!=144 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+9296)!=8668)return -1;
 uint32_t no=MeleePc_DatBe32(b+9300);
 if(no>=38 || !memchr(b+9304+no,0,38-no) || strcmp((const char*)b+9304+no,"PlyMario5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(9342);if(!scratch)return -1;memcpy(scratch,b,9342);
 int result=MeleePc_FigaTreeBody(scratch,9342,8688,144,8668);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+49152;
 if(MeleePc_DatBe32(b)!=3614 || MeleePc_DatBe32(b+4)!=2956 || MeleePc_DatBe32(b+8)!=144 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+3564)!=2936)return -1;
 uint32_t no=MeleePc_DatBe32(b+3568);
 if(no>=42 || !memchr(b+3572+no,0,42-no) || strcmp((const char*)b+3572+no,"PlyMario5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(3614);if(!scratch)return -1;memcpy(scratch,b,3614);
 int result=MeleePc_FigaTreeBody(scratch,3614,2956,144,2936);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+28832;
 if(MeleePc_DatBe32(b)!=8222 || MeleePc_DatBe32(b+4)!=7564 || MeleePc_DatBe32(b+8)!=144 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+8172)!=7544)return -1;
 uint32_t no=MeleePc_DatBe32(b+8176);
 if(no>=42 || !memchr(b+8180+no,0,42-no) || strcmp((const char*)b+8180+no,"PlyMario5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(8222);if(!scratch)return -1;memcpy(scratch,b,8222);
 int result=MeleePc_FigaTreeBody(scratch,8222,7564,144,7544);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+52768;
 if(MeleePc_DatBe32(b)!=3634 || MeleePc_DatBe32(b+4)!=2972 || MeleePc_DatBe32(b+8)!=144 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+3580)!=2952)return -1;
 uint32_t no=MeleePc_DatBe32(b+3584);
 if(no>=46 || !memchr(b+3588+no,0,46-no) || strcmp((const char*)b+3588+no,"PlyMario5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(3634);if(!scratch)return -1;memcpy(scratch,b,3634);
 int result=MeleePc_FigaTreeBody(scratch,3634,2972,144,2952);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+37056;
 if(MeleePc_DatBe32(b)!=4746 || MeleePc_DatBe32(b+4)!=4092 || MeleePc_DatBe32(b+8)!=144 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4700)!=4072)return -1;
 uint32_t no=MeleePc_DatBe32(b+4704);
 if(no>=38 || !memchr(b+4708+no,0,38-no) || strcmp((const char*)b+4708+no,"PlyMario5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(4746);if(!scratch)return -1;memcpy(scratch,b,4746);
 int result=MeleePc_FigaTreeBody(scratch,4746,4092,144,4072);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
