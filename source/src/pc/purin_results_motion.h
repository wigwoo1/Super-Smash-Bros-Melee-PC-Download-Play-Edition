#ifndef MELEE_PC_PURIN_RESULTS_MOTION_H
#define MELEE_PC_PURIN_RESULTS_MOTION_H
/* Validate all deferred embedded archives on scratch before mutating outer metadata. */
static int MeleePc_PurinResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=151844)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=17138 || MeleePc_DatBe32(b+4)!=16676 || MeleePc_DatBe32(b+8)!=96 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+17092)!=16656)return -1;
 uint32_t no=MeleePc_DatBe32(b+17096);
 if(no>=38 || !memchr(b+17100+no,0,38-no) || strcmp((const char*)b+17100+no,"PlyPurin5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(17138);if(!scratch)return -1;memcpy(scratch,b,17138);
 int result=MeleePc_FigaTreeBody(scratch,17138,16676,96,16656);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+106368;
 if(MeleePc_DatBe32(b)!=9530 || MeleePc_DatBe32(b+4)!=9064 || MeleePc_DatBe32(b+8)!=96 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+9480)!=9044)return -1;
 uint32_t no=MeleePc_DatBe32(b+9484);
 if(no>=42 || !memchr(b+9488+no,0,42-no) || strcmp((const char*)b+9488+no,"PlyPurin5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(9530);if(!scratch)return -1;memcpy(scratch,b,9530);
 int result=MeleePc_FigaTreeBody(scratch,9530,9064,96,9044);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+17152;
 if(MeleePc_DatBe32(b)!=21510 || MeleePc_DatBe32(b+4)!=21048 || MeleePc_DatBe32(b+8)!=96 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+21464)!=21028)return -1;
 uint32_t no=MeleePc_DatBe32(b+21468);
 if(no>=38 || !memchr(b+21472+no,0,38-no) || strcmp((const char*)b+21472+no,"PlyPurin5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(21510);if(!scratch)return -1;memcpy(scratch,b,21510);
 int result=MeleePc_FigaTreeBody(scratch,21510,21048,96,21028);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+115904;
 if(MeleePc_DatBe32(b)!=8758 || MeleePc_DatBe32(b+4)!=8292 || MeleePc_DatBe32(b+8)!=96 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+8708)!=8272)return -1;
 uint32_t no=MeleePc_DatBe32(b+8712);
 if(no>=42 || !memchr(b+8716+no,0,42-no) || strcmp((const char*)b+8716+no,"PlyPurin5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(8758);if(!scratch)return -1;memcpy(scratch,b,8758);
 int result=MeleePc_FigaTreeBody(scratch,8758,8292,96,8272);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+38688;
 if(MeleePc_DatBe32(b)!=15378 || MeleePc_DatBe32(b+4)!=14916 || MeleePc_DatBe32(b+8)!=96 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+15332)!=14896)return -1;
 uint32_t no=MeleePc_DatBe32(b+15336);
 if(no>=38 || !memchr(b+15340+no,0,38-no) || strcmp((const char*)b+15340+no,"PlyPurin5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(15378);if(!scratch)return -1;memcpy(scratch,b,15378);
 int result=MeleePc_FigaTreeBody(scratch,15378,14916,96,14896);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+124672;
 if(MeleePc_DatBe32(b)!=18126 || MeleePc_DatBe32(b+4)!=17660 || MeleePc_DatBe32(b+8)!=96 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+18076)!=17640)return -1;
 uint32_t no=MeleePc_DatBe32(b+18080);
 if(no>=42 || !memchr(b+18084+no,0,42-no) || strcmp((const char*)b+18084+no,"PlyPurin5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(18126);if(!scratch)return -1;memcpy(scratch,b,18126);
 int result=MeleePc_FigaTreeBody(scratch,18126,17660,96,17640);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+54080;
 if(MeleePc_DatBe32(b)!=20818 || MeleePc_DatBe32(b+4)!=20352 || MeleePc_DatBe32(b+8)!=96 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+20768)!=20332)return -1;
 uint32_t no=MeleePc_DatBe32(b+20772);
 if(no>=42 || !memchr(b+20776+no,0,42-no) || strcmp((const char*)b+20776+no,"PlyPurin5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(20818);if(!scratch)return -1;memcpy(scratch,b,20818);
 int result=MeleePc_FigaTreeBody(scratch,20818,20352,96,20332);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+142816;
 if(MeleePc_DatBe32(b)!=8930 || MeleePc_DatBe32(b+4)!=8460 || MeleePc_DatBe32(b+8)!=96 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+8876)!=8440)return -1;
 uint32_t no=MeleePc_DatBe32(b+8880);
 if(no>=46 || !memchr(b+8884+no,0,46-no) || strcmp((const char*)b+8884+no,"PlyPurin5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(8930);if(!scratch)return -1;memcpy(scratch,b,8930);
 int result=MeleePc_FigaTreeBody(scratch,8930,8460,96,8440);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+74912;
 if(MeleePc_DatBe32(b)!=31434 || MeleePc_DatBe32(b+4)!=30972 || MeleePc_DatBe32(b+8)!=96 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+31388)!=30952)return -1;
 uint32_t no=MeleePc_DatBe32(b+31392);
 if(no>=38 || !memchr(b+31396+no,0,38-no) || strcmp((const char*)b+31396+no,"PlyPurin5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(31434);if(!scratch)return -1;memcpy(scratch,b,31434);
 int result=MeleePc_FigaTreeBody(scratch,31434,30972,96,30952);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
