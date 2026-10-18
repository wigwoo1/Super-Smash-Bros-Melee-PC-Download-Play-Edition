#ifndef MELEE_PC_PIKACHU_RESULTS_MOTION_H
#define MELEE_PC_PIKACHU_RESULTS_MOTION_H
/* Validate deferred embedded archives on scratch; preserve source body bytes. */
static int MeleePc_PikachuResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=59942)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=11404 || MeleePc_DatBe32(b+4)!=10796 || MeleePc_DatBe32(b+8)!=132 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+11356)!=10776)return -1;
 uint32_t no=MeleePc_DatBe32(b+11360);
 if(no>=40 || !memchr(b+11364+no,0,40-no) || strcmp((const char*)b+11364+no,"PlyPikachu5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(11404);if(!scratch)return -1;memcpy(scratch,b,11404);
 int result=MeleePc_FigaTreeBody(scratch,11404,10796,132,10776);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+40416;
 if(MeleePc_DatBe32(b)!=4988 || MeleePc_DatBe32(b+4)!=4376 || MeleePc_DatBe32(b+8)!=132 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4936)!=4356)return -1;
 uint32_t no=MeleePc_DatBe32(b+4940);
 if(no>=44 || !memchr(b+4944+no,0,44-no) || strcmp((const char*)b+4944+no,"PlyPikachu5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4988);if(!scratch)return -1;memcpy(scratch,b,4988);
 int result=MeleePc_FigaTreeBody(scratch,4988,4376,132,4356);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+11424;
 if(MeleePc_DatBe32(b)!=6444 || MeleePc_DatBe32(b+4)!=5836 || MeleePc_DatBe32(b+8)!=132 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6396)!=5816)return -1;
 uint32_t no=MeleePc_DatBe32(b+6400);
 if(no>=40 || !memchr(b+6404+no,0,40-no) || strcmp((const char*)b+6404+no,"PlyPikachu5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(6444);if(!scratch)return -1;memcpy(scratch,b,6444);
 int result=MeleePc_FigaTreeBody(scratch,6444,5836,132,5816);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+45408;
 if(MeleePc_DatBe32(b)!=4512 || MeleePc_DatBe32(b+4)!=3900 || MeleePc_DatBe32(b+8)!=132 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4460)!=3880)return -1;
 uint32_t no=MeleePc_DatBe32(b+4464);
 if(no>=44 || !memchr(b+4468+no,0,44-no) || strcmp((const char*)b+4468+no,"PlyPikachu5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4512);if(!scratch)return -1;memcpy(scratch,b,4512);
 int result=MeleePc_FigaTreeBody(scratch,4512,3900,132,3880);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+17888;
 if(MeleePc_DatBe32(b)!=4936 || MeleePc_DatBe32(b+4)!=4328 || MeleePc_DatBe32(b+8)!=132 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4888)!=4308)return -1;
 uint32_t no=MeleePc_DatBe32(b+4892);
 if(no>=40 || !memchr(b+4896+no,0,40-no) || strcmp((const char*)b+4896+no,"PlyPikachu5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(4936);if(!scratch)return -1;memcpy(scratch,b,4936);
 int result=MeleePc_FigaTreeBody(scratch,4936,4328,132,4308);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+49920;
 if(MeleePc_DatBe32(b)!=4780 || MeleePc_DatBe32(b+4)!=4168 || MeleePc_DatBe32(b+8)!=132 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4728)!=4148)return -1;
 uint32_t no=MeleePc_DatBe32(b+4732);
 if(no>=44 || !memchr(b+4736+no,0,44-no) || strcmp((const char*)b+4736+no,"PlyPikachu5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4780);if(!scratch)return -1;memcpy(scratch,b,4780);
 int result=MeleePc_FigaTreeBody(scratch,4780,4168,132,4148);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+22848;
 if(MeleePc_DatBe32(b)!=11344 || MeleePc_DatBe32(b+4)!=10732 || MeleePc_DatBe32(b+8)!=132 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+11292)!=10712)return -1;
 uint32_t no=MeleePc_DatBe32(b+11296);
 if(no>=44 || !memchr(b+11300+no,0,44-no) || strcmp((const char*)b+11300+no,"PlyPikachu5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(11344);if(!scratch)return -1;memcpy(scratch,b,11344);
 int result=MeleePc_FigaTreeBody(scratch,11344,10732,132,10712);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+54720;
 if(MeleePc_DatBe32(b)!=5144 || MeleePc_DatBe32(b+4)!=4528 || MeleePc_DatBe32(b+8)!=132 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5088)!=4508)return -1;
 uint32_t no=MeleePc_DatBe32(b+5092);
 if(no>=48 || !memchr(b+5096+no,0,48-no) || strcmp((const char*)b+5096+no,"PlyPikachu5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(5144);if(!scratch)return -1;memcpy(scratch,b,5144);
 int result=MeleePc_FigaTreeBody(scratch,5144,4528,132,4508);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+34208;
 if(MeleePc_DatBe32(b)!=6192 || MeleePc_DatBe32(b+4)!=5584 || MeleePc_DatBe32(b+8)!=132 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6144)!=5564)return -1;
 uint32_t no=MeleePc_DatBe32(b+6148);
 if(no>=40 || !memchr(b+6152+no,0,40-no) || strcmp((const char*)b+6152+no,"PlyPikachu5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(6192);if(!scratch)return -1;memcpy(scratch,b,6192);
 int result=MeleePc_FigaTreeBody(scratch,6192,5584,132,5564);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
