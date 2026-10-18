#ifndef MELEE_PC_MEWTWO_RESULTS_MOTION_H
#define MELEE_PC_MEWTWO_RESULTS_MOTION_H
/* Validate all deferred embedded archives on scratch before mutating outer metadata. */
static int MeleePc_MewtwoResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=62981)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=11267 || MeleePc_DatBe32(b+4)!=10652 || MeleePc_DatBe32(b+8)!=134 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+11220)!=10632)return -1;
 uint32_t no=MeleePc_DatBe32(b+11224);
 if(no>=39 || !memchr(b+11228+no,0,39-no) || strcmp((const char*)b+11228+no,"PlyMewtwo5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(11267);if(!scratch)return -1;memcpy(scratch,b,11267);
 int result=MeleePc_FigaTreeBody(scratch,11267,10652,134,10632);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+43136;
 if(MeleePc_DatBe32(b)!=5327 || MeleePc_DatBe32(b+4)!=4708 || MeleePc_DatBe32(b+8)!=134 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5276)!=4688)return -1;
 uint32_t no=MeleePc_DatBe32(b+5280);
 if(no>=43 || !memchr(b+5284+no,0,43-no) || strcmp((const char*)b+5284+no,"PlyMewtwo5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(5327);if(!scratch)return -1;memcpy(scratch,b,5327);
 int result=MeleePc_FigaTreeBody(scratch,5327,4708,134,4688);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+11296;
 if(MeleePc_DatBe32(b)!=8735 || MeleePc_DatBe32(b+4)!=8120 || MeleePc_DatBe32(b+8)!=134 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+8688)!=8100)return -1;
 uint32_t no=MeleePc_DatBe32(b+8692);
 if(no>=39 || !memchr(b+8696+no,0,39-no) || strcmp((const char*)b+8696+no,"PlyMewtwo5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(8735);if(!scratch)return -1;memcpy(scratch,b,8735);
 int result=MeleePc_FigaTreeBody(scratch,8735,8120,134,8100);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+48480;
 if(MeleePc_DatBe32(b)!=4431 || MeleePc_DatBe32(b+4)!=3812 || MeleePc_DatBe32(b+8)!=134 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4380)!=3792)return -1;
 uint32_t no=MeleePc_DatBe32(b+4384);
 if(no>=43 || !memchr(b+4388+no,0,43-no) || strcmp((const char*)b+4388+no,"PlyMewtwo5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4431);if(!scratch)return -1;memcpy(scratch,b,4431);
 int result=MeleePc_FigaTreeBody(scratch,4431,3812,134,3792);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+20032;
 if(MeleePc_DatBe32(b)!=9079 || MeleePc_DatBe32(b+4)!=8464 || MeleePc_DatBe32(b+8)!=134 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+9032)!=8444)return -1;
 uint32_t no=MeleePc_DatBe32(b+9036);
 if(no>=39 || !memchr(b+9040+no,0,39-no) || strcmp((const char*)b+9040+no,"PlyMewtwo5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(9079);if(!scratch)return -1;memcpy(scratch,b,9079);
 int result=MeleePc_FigaTreeBody(scratch,9079,8464,134,8444);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+52928;
 if(MeleePc_DatBe32(b)!=5215 || MeleePc_DatBe32(b+4)!=4596 || MeleePc_DatBe32(b+8)!=134 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5164)!=4576)return -1;
 uint32_t no=MeleePc_DatBe32(b+5168);
 if(no>=43 || !memchr(b+5172+no,0,43-no) || strcmp((const char*)b+5172+no,"PlyMewtwo5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(5215);if(!scratch)return -1;memcpy(scratch,b,5215);
 int result=MeleePc_FigaTreeBody(scratch,5215,4596,134,4576);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+29120;
 if(MeleePc_DatBe32(b)!=8835 || MeleePc_DatBe32(b+4)!=8216 || MeleePc_DatBe32(b+8)!=134 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+8784)!=8196)return -1;
 uint32_t no=MeleePc_DatBe32(b+8788);
 if(no>=43 || !memchr(b+8792+no,0,43-no) || strcmp((const char*)b+8792+no,"PlyMewtwo5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(8835);if(!scratch)return -1;memcpy(scratch,b,8835);
 int result=MeleePc_FigaTreeBody(scratch,8835,8216,134,8196);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+58144;
 if(MeleePc_DatBe32(b)!=4751 || MeleePc_DatBe32(b+4)!=4128 || MeleePc_DatBe32(b+8)!=134 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4696)!=4108)return -1;
 uint32_t no=MeleePc_DatBe32(b+4700);
 if(no>=47 || !memchr(b+4704+no,0,47-no) || strcmp((const char*)b+4704+no,"PlyMewtwo5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(4751);if(!scratch)return -1;memcpy(scratch,b,4751);
 int result=MeleePc_FigaTreeBody(scratch,4751,4128,134,4108);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+37984;
 if(MeleePc_DatBe32(b)!=5143 || MeleePc_DatBe32(b+4)!=4528 || MeleePc_DatBe32(b+8)!=134 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5096)!=4508)return -1;
 uint32_t no=MeleePc_DatBe32(b+5100);
 if(no>=39 || !memchr(b+5104+no,0,39-no) || strcmp((const char*)b+5104+no,"PlyMewtwo5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(5143);if(!scratch)return -1;memcpy(scratch,b,5143);
 int result=MeleePc_FigaTreeBody(scratch,5143,4528,134,4508);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
