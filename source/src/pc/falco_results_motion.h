#ifndef MELEE_PC_FALCO_RESULTS_MOTION_H
#define MELEE_PC_FALCO_RESULTS_MOTION_H
/* Validate deferred embedded archives on scratch; preserve source body bytes. */
static int MeleePc_FalcoResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=106148)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=39966 || MeleePc_DatBe32(b+4)!=39244 || MeleePc_DatBe32(b+8)!=161 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+39920)!=39224)return -1;
 uint32_t no=MeleePc_DatBe32(b+39924);
 if(no>=38 || !memchr(b+39928+no,0,38-no) || strcmp((const char*)b+39928+no,"PlyFalco5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(39966);if(!scratch)return -1;memcpy(scratch,b,39966);
 int result=MeleePc_FigaTreeBody(scratch,39966,39244,161,39224);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+74368;
 if(MeleePc_DatBe32(b)!=5018 || MeleePc_DatBe32(b+4)!=4292 || MeleePc_DatBe32(b+8)!=161 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4968)!=4272)return -1;
 uint32_t no=MeleePc_DatBe32(b+4972);
 if(no>=42 || !memchr(b+4976+no,0,42-no) || strcmp((const char*)b+4976+no,"PlyFalco5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(5018);if(!scratch)return -1;memcpy(scratch,b,5018);
 int result=MeleePc_FigaTreeBody(scratch,5018,4292,161,4272);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+39968;
 if(MeleePc_DatBe32(b)!=19822 || MeleePc_DatBe32(b+4)!=19100 || MeleePc_DatBe32(b+8)!=161 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+19776)!=19080)return -1;
 uint32_t no=MeleePc_DatBe32(b+19780);
 if(no>=38 || !memchr(b+19784+no,0,38-no) || strcmp((const char*)b+19784+no,"PlyFalco5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(19822);if(!scratch)return -1;memcpy(scratch,b,19822);
 int result=MeleePc_FigaTreeBody(scratch,19822,19100,161,19080);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+79392;
 if(MeleePc_DatBe32(b)!=4934 || MeleePc_DatBe32(b+4)!=4208 || MeleePc_DatBe32(b+8)!=161 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4884)!=4188)return -1;
 uint32_t no=MeleePc_DatBe32(b+4888);
 if(no>=42 || !memchr(b+4892+no,0,42-no) || strcmp((const char*)b+4892+no,"PlyFalco5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4934);if(!scratch)return -1;memcpy(scratch,b,4934);
 int result=MeleePc_FigaTreeBody(scratch,4934,4208,161,4188);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+84352;
 if(MeleePc_DatBe32(b)!=10498 || MeleePc_DatBe32(b+4)!=9776 || MeleePc_DatBe32(b+8)!=161 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+10452)!=9756)return -1;
 uint32_t no=MeleePc_DatBe32(b+10456);
 if(no>=38 || !memchr(b+10460+no,0,38-no) || strcmp((const char*)b+10460+no,"PlyFalco5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(10498);if(!scratch)return -1;memcpy(scratch,b,10498);
 int result=MeleePc_FigaTreeBody(scratch,10498,9776,161,9756);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+94880;
 if(MeleePc_DatBe32(b)!=7150 || MeleePc_DatBe32(b+4)!=6424 || MeleePc_DatBe32(b+8)!=161 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+7100)!=6404)return -1;
 uint32_t no=MeleePc_DatBe32(b+7104);
 if(no>=42 || !memchr(b+7108+no,0,42-no) || strcmp((const char*)b+7108+no,"PlyFalco5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(7150);if(!scratch)return -1;memcpy(scratch,b,7150);
 int result=MeleePc_FigaTreeBody(scratch,7150,6424,161,6404);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+59808;
 if(MeleePc_DatBe32(b)!=4022 || MeleePc_DatBe32(b+4)!=3296 || MeleePc_DatBe32(b+8)!=161 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+3972)!=3276)return -1;
 uint32_t no=MeleePc_DatBe32(b+3976);
 if(no>=42 || !memchr(b+3980+no,0,42-no) || strcmp((const char*)b+3980+no,"PlyFalco5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(4022);if(!scratch)return -1;memcpy(scratch,b,4022);
 int result=MeleePc_FigaTreeBody(scratch,4022,3296,161,3276);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+102048;
 if(MeleePc_DatBe32(b)!=4026 || MeleePc_DatBe32(b+4)!=3296 || MeleePc_DatBe32(b+8)!=161 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+3972)!=3276)return -1;
 uint32_t no=MeleePc_DatBe32(b+3976);
 if(no>=46 || !memchr(b+3980+no,0,46-no) || strcmp((const char*)b+3980+no,"PlyFalco5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(4026);if(!scratch)return -1;memcpy(scratch,b,4026);
 int result=MeleePc_FigaTreeBody(scratch,4026,3296,161,3276);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+63840;
 if(MeleePc_DatBe32(b)!=10498 || MeleePc_DatBe32(b+4)!=9776 || MeleePc_DatBe32(b+8)!=161 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+10452)!=9756)return -1;
 uint32_t no=MeleePc_DatBe32(b+10456);
 if(no>=38 || !memchr(b+10460+no,0,38-no) || strcmp((const char*)b+10460+no,"PlyFalco5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(10498);if(!scratch)return -1;memcpy(scratch,b,10498);
 int result=MeleePc_FigaTreeBody(scratch,10498,9776,161,9756);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
