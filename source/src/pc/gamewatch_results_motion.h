#ifndef MELEE_PC_GAMEWATCH_RESULTS_MOTION_H
#define MELEE_PC_GAMEWATCH_RESULTS_MOTION_H
/* Validate deferred embedded archives on scratch; never mutate the container body. */
static int MeleePc_GameWatchResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=52776)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=5670 || MeleePc_DatBe32(b+4)!=4740 || MeleePc_DatBe32(b+8)!=212 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5620)!=4720)return -1;
 uint32_t no=MeleePc_DatBe32(b+5624);
 if(no>=42 || !memchr(b+5628+no,0,42-no) || strcmp((const char*)b+5628+no,"PlyGamewatch5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(5670);if(!scratch)return -1;memcpy(scratch,b,5670);
 int result=MeleePc_FigaTreeBody(scratch,5670,4740,212,4720);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+29344;
 if(MeleePc_DatBe32(b)!=5694 || MeleePc_DatBe32(b+4)!=4760 || MeleePc_DatBe32(b+8)!=212 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5640)!=4740)return -1;
 uint32_t no=MeleePc_DatBe32(b+5644);
 if(no>=46 || !memchr(b+5648+no,0,46-no) || strcmp((const char*)b+5648+no,"PlyGamewatch5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(5694);if(!scratch)return -1;memcpy(scratch,b,5694);
 int result=MeleePc_FigaTreeBody(scratch,5694,4760,212,4740);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+5696;
 if(MeleePc_DatBe32(b)!=6694 || MeleePc_DatBe32(b+4)!=5764 || MeleePc_DatBe32(b+8)!=212 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6644)!=5744)return -1;
 uint32_t no=MeleePc_DatBe32(b+6648);
 if(no>=42 || !memchr(b+6652+no,0,42-no) || strcmp((const char*)b+6652+no,"PlyGamewatch5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(6694);if(!scratch)return -1;memcpy(scratch,b,6694);
 int result=MeleePc_FigaTreeBody(scratch,6694,5764,212,5744);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+35040;
 if(MeleePc_DatBe32(b)!=6914 || MeleePc_DatBe32(b+4)!=5980 || MeleePc_DatBe32(b+8)!=212 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6860)!=5960)return -1;
 uint32_t no=MeleePc_DatBe32(b+6864);
 if(no>=46 || !memchr(b+6868+no,0,46-no) || strcmp((const char*)b+6868+no,"PlyGamewatch5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(6914);if(!scratch)return -1;memcpy(scratch,b,6914);
 int result=MeleePc_FigaTreeBody(scratch,6914,5980,212,5960);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+12416;
 if(MeleePc_DatBe32(b)!=6090 || MeleePc_DatBe32(b+4)!=5160 || MeleePc_DatBe32(b+8)!=212 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6040)!=5140)return -1;
 uint32_t no=MeleePc_DatBe32(b+6044);
 if(no>=42 || !memchr(b+6048+no,0,42-no) || strcmp((const char*)b+6048+no,"PlyGamewatch5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(6090);if(!scratch)return -1;memcpy(scratch,b,6090);
 int result=MeleePc_FigaTreeBody(scratch,6090,5160,212,5140);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+41984;
 if(MeleePc_DatBe32(b)!=5210 || MeleePc_DatBe32(b+4)!=4276 || MeleePc_DatBe32(b+8)!=212 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5156)!=4256)return -1;
 uint32_t no=MeleePc_DatBe32(b+5160);
 if(no>=46 || !memchr(b+5164+no,0,46-no) || strcmp((const char*)b+5164+no,"PlyGamewatch5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(5210);if(!scratch)return -1;memcpy(scratch,b,5210);
 int result=MeleePc_FigaTreeBody(scratch,5210,4276,212,4256);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+18528;
 if(MeleePc_DatBe32(b)!=5482 || MeleePc_DatBe32(b+4)!=4548 || MeleePc_DatBe32(b+8)!=212 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5428)!=4528)return -1;
 uint32_t no=MeleePc_DatBe32(b+5432);
 if(no>=46 || !memchr(b+5436+no,0,46-no) || strcmp((const char*)b+5436+no,"PlyGamewatch5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(5482);if(!scratch)return -1;memcpy(scratch,b,5482);
 int result=MeleePc_FigaTreeBody(scratch,5482,4548,212,4528);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+47200;
 if(MeleePc_DatBe32(b)!=5486 || MeleePc_DatBe32(b+4)!=4548 || MeleePc_DatBe32(b+8)!=212 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5428)!=4528)return -1;
 uint32_t no=MeleePc_DatBe32(b+5432);
 if(no>=50 || !memchr(b+5436+no,0,50-no) || strcmp((const char*)b+5436+no,"PlyGamewatch5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(5486);if(!scratch)return -1;memcpy(scratch,b,5486);
 int result=MeleePc_FigaTreeBody(scratch,5486,4548,212,4528);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+24032;
 if(MeleePc_DatBe32(b)!=5294 || MeleePc_DatBe32(b+4)!=4364 || MeleePc_DatBe32(b+8)!=212 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5244)!=4344)return -1;
 uint32_t no=MeleePc_DatBe32(b+5248);
 if(no>=42 || !memchr(b+5252+no,0,42-no) || strcmp((const char*)b+5252+no,"PlyGamewatch5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(5294);if(!scratch)return -1;memcpy(scratch,b,5294);
 int result=MeleePc_FigaTreeBody(scratch,5294,4364,212,4344);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
