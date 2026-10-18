#ifndef MELEE_PC_BOWSER_RESULTS_MOTION_H
#define MELEE_PC_BOWSER_RESULTS_MOTION_H
/* Validate deferred embedded archives on scratch; preserve source body bytes. */
static int MeleePc_BowserResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=84580)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=11430 || MeleePc_DatBe32(b+4)!=10500 || MeleePc_DatBe32(b+8)!=213 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+11384)!=10480)return -1;
 uint32_t no=MeleePc_DatBe32(b+11388);
 if(no>=38 || !memchr(b+11392+no,0,38-no) || strcmp((const char*)b+11392+no,"PlyKoopa5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(11430);if(!scratch)return -1;memcpy(scratch,b,11430);
 int result=MeleePc_FigaTreeBody(scratch,11430,10500,213,10480);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+58848;
 if(MeleePc_DatBe32(b)!=6858 || MeleePc_DatBe32(b+4)!=5924 || MeleePc_DatBe32(b+8)!=213 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6808)!=5904)return -1;
 uint32_t no=MeleePc_DatBe32(b+6812);
 if(no>=42 || !memchr(b+6816+no,0,42-no) || strcmp((const char*)b+6816+no,"PlyKoopa5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(6858);if(!scratch)return -1;memcpy(scratch,b,6858);
 int result=MeleePc_FigaTreeBody(scratch,6858,5924,213,5904);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+11456;
 if(MeleePc_DatBe32(b)!=10318 || MeleePc_DatBe32(b+4)!=9388 || MeleePc_DatBe32(b+8)!=213 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+10272)!=9368)return -1;
 uint32_t no=MeleePc_DatBe32(b+10276);
 if(no>=38 || !memchr(b+10280+no,0,38-no) || strcmp((const char*)b+10280+no,"PlyKoopa5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(10318);if(!scratch)return -1;memcpy(scratch,b,10318);
 int result=MeleePc_FigaTreeBody(scratch,10318,9388,213,9368);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+65728;
 if(MeleePc_DatBe32(b)!=5698 || MeleePc_DatBe32(b+4)!=4764 || MeleePc_DatBe32(b+8)!=213 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5648)!=4744)return -1;
 uint32_t no=MeleePc_DatBe32(b+5652);
 if(no>=42 || !memchr(b+5656+no,0,42-no) || strcmp((const char*)b+5656+no,"PlyKoopa5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(5698);if(!scratch)return -1;memcpy(scratch,b,5698);
 int result=MeleePc_FigaTreeBody(scratch,5698,4764,213,4744);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+21792;
 if(MeleePc_DatBe32(b)!=11202 || MeleePc_DatBe32(b+4)!=10272 || MeleePc_DatBe32(b+8)!=213 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+11156)!=10252)return -1;
 uint32_t no=MeleePc_DatBe32(b+11160);
 if(no>=38 || !memchr(b+11164+no,0,38-no) || strcmp((const char*)b+11164+no,"PlyKoopa5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(11202);if(!scratch)return -1;memcpy(scratch,b,11202);
 int result=MeleePc_FigaTreeBody(scratch,11202,10272,213,10252);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+71456;
 if(MeleePc_DatBe32(b)!=6034 || MeleePc_DatBe32(b+4)!=5100 || MeleePc_DatBe32(b+8)!=213 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5984)!=5080)return -1;
 uint32_t no=MeleePc_DatBe32(b+5988);
 if(no>=42 || !memchr(b+5992+no,0,42-no) || strcmp((const char*)b+5992+no,"PlyKoopa5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(6034);if(!scratch)return -1;memcpy(scratch,b,6034);
 int result=MeleePc_FigaTreeBody(scratch,6034,5100,213,5080);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+33024;
 if(MeleePc_DatBe32(b)!=19318 || MeleePc_DatBe32(b+4)!=18384 || MeleePc_DatBe32(b+8)!=213 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+19268)!=18364)return -1;
 uint32_t no=MeleePc_DatBe32(b+19272);
 if(no>=42 || !memchr(b+19276+no,0,42-no) || strcmp((const char*)b+19276+no,"PlyKoopa5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(19318);if(!scratch)return -1;memcpy(scratch,b,19318);
 int result=MeleePc_FigaTreeBody(scratch,19318,18384,213,18364);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+77504;
 if(MeleePc_DatBe32(b)!=6978 || MeleePc_DatBe32(b+4)!=6040 || MeleePc_DatBe32(b+8)!=213 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6924)!=6020)return -1;
 uint32_t no=MeleePc_DatBe32(b+6928);
 if(no>=46 || !memchr(b+6932+no,0,46-no) || strcmp((const char*)b+6932+no,"PlyKoopa5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(6978);if(!scratch)return -1;memcpy(scratch,b,6978);
 int result=MeleePc_FigaTreeBody(scratch,6978,6040,213,6020);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+52352;
 if(MeleePc_DatBe32(b)!=6478 || MeleePc_DatBe32(b+4)!=5548 || MeleePc_DatBe32(b+8)!=213 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6432)!=5528)return -1;
 uint32_t no=MeleePc_DatBe32(b+6436);
 if(no>=38 || !memchr(b+6440+no,0,38-no) || strcmp((const char*)b+6440+no,"PlyKoopa5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(6478);if(!scratch)return -1;memcpy(scratch,b,6478);
 int result=MeleePc_FigaTreeBody(scratch,6478,5548,213,5528);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
