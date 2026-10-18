#ifndef MELEE_PC_KIRBY_RESULTS_MOTION_H
#define MELEE_PC_KIRBY_RESULTS_MOTION_H
/* Validate deferred embedded archives on scratch; preserve source body bytes. */
static int MeleePc_KirbyResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=131268)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=20710 || MeleePc_DatBe32(b+4)!=20136 || MeleePc_DatBe32(b+8)!=124 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+20664)!=20116)return -1;
 uint32_t no=MeleePc_DatBe32(b+20668);
 if(no>=38 || !memchr(b+20672+no,0,38-no) || strcmp((const char*)b+20672+no,"PlyKirby5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(20710);if(!scratch)return -1;memcpy(scratch,b,20710);
 int result=MeleePc_FigaTreeBody(scratch,20710,20136,124,20116);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+106656;
 if(MeleePc_DatBe32(b)!=5906 || MeleePc_DatBe32(b+4)!=5328 || MeleePc_DatBe32(b+8)!=124 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5856)!=5308)return -1;
 uint32_t no=MeleePc_DatBe32(b+5860);
 if(no>=42 || !memchr(b+5864+no,0,42-no) || strcmp((const char*)b+5864+no,"PlyKirby5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(5906);if(!scratch)return -1;memcpy(scratch,b,5906);
 int result=MeleePc_FigaTreeBody(scratch,5906,5328,124,5308);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+20736;
 if(MeleePc_DatBe32(b)!=27542 || MeleePc_DatBe32(b+4)!=26968 || MeleePc_DatBe32(b+8)!=124 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+27496)!=26948)return -1;
 uint32_t no=MeleePc_DatBe32(b+27500);
 if(no>=38 || !memchr(b+27504+no,0,38-no) || strcmp((const char*)b+27504+no,"PlyKirby5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(27542);if(!scratch)return -1;memcpy(scratch,b,27542);
 int result=MeleePc_FigaTreeBody(scratch,27542,26968,124,26948);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+112576;
 if(MeleePc_DatBe32(b)!=5970 || MeleePc_DatBe32(b+4)!=5392 || MeleePc_DatBe32(b+8)!=124 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5920)!=5372)return -1;
 uint32_t no=MeleePc_DatBe32(b+5924);
 if(no>=42 || !memchr(b+5928+no,0,42-no) || strcmp((const char*)b+5928+no,"PlyKirby5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(5970);if(!scratch)return -1;memcpy(scratch,b,5970);
 int result=MeleePc_FigaTreeBody(scratch,5970,5392,124,5372);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+48288;
 if(MeleePc_DatBe32(b)!=26806 || MeleePc_DatBe32(b+4)!=26232 || MeleePc_DatBe32(b+8)!=124 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+26760)!=26212)return -1;
 uint32_t no=MeleePc_DatBe32(b+26764);
 if(no>=38 || !memchr(b+26768+no,0,38-no) || strcmp((const char*)b+26768+no,"PlyKirby5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(26806);if(!scratch)return -1;memcpy(scratch,b,26806);
 int result=MeleePc_FigaTreeBody(scratch,26806,26232,124,26212);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+118560;
 if(MeleePc_DatBe32(b)!=5982 || MeleePc_DatBe32(b+4)!=5404 || MeleePc_DatBe32(b+8)!=124 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5932)!=5384)return -1;
 uint32_t no=MeleePc_DatBe32(b+5936);
 if(no>=42 || !memchr(b+5940+no,0,42-no) || strcmp((const char*)b+5940+no,"PlyKirby5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(5982);if(!scratch)return -1;memcpy(scratch,b,5982);
 int result=MeleePc_FigaTreeBody(scratch,5982,5404,124,5384);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+75104;
 if(MeleePc_DatBe32(b)!=13170 || MeleePc_DatBe32(b+4)!=12592 || MeleePc_DatBe32(b+8)!=124 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+13120)!=12572)return -1;
 uint32_t no=MeleePc_DatBe32(b+13124);
 if(no>=42 || !memchr(b+13128+no,0,42-no) || strcmp((const char*)b+13128+no,"PlyKirby5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(13170);if(!scratch)return -1;memcpy(scratch,b,13170);
 int result=MeleePc_FigaTreeBody(scratch,13170,12592,124,12572);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+124544;
 if(MeleePc_DatBe32(b)!=6630 || MeleePc_DatBe32(b+4)!=6048 || MeleePc_DatBe32(b+8)!=124 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6576)!=6028)return -1;
 uint32_t no=MeleePc_DatBe32(b+6580);
 if(no>=46 || !memchr(b+6584+no,0,46-no) || strcmp((const char*)b+6584+no,"PlyKirby5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(6630);if(!scratch)return -1;memcpy(scratch,b,6630);
 int result=MeleePc_FigaTreeBody(scratch,6630,6048,124,6028);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+88288;
 if(MeleePc_DatBe32(b)!=18354 || MeleePc_DatBe32(b+4)!=17780 || MeleePc_DatBe32(b+8)!=124 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+18308)!=17760)return -1;
 uint32_t no=MeleePc_DatBe32(b+18312);
 if(no>=38 || !memchr(b+18316+no,0,38-no) || strcmp((const char*)b+18316+no,"PlyKirby5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(18354);if(!scratch)return -1;memcpy(scratch,b,18354);
 int result=MeleePc_FigaTreeBody(scratch,18354,17780,124,17760);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
