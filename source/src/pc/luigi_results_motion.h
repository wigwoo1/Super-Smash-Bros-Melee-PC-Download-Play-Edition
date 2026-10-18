#ifndef MELEE_PC_LUIGI_RESULTS_MOTION_H
#define MELEE_PC_LUIGI_RESULTS_MOTION_H
/* Validate all deferred embedded archives on scratch before mutating outer metadata. */
static int MeleePc_LuigiResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=94820)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=7826 || MeleePc_DatBe32(b+4)!=7036 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+7780)!=7016)return -1;
 uint32_t no=MeleePc_DatBe32(b+7784);
 if(no>=38 || !memchr(b+7788+no,0,38-no) || strcmp((const char*)b+7788+no,"PlyLuigi5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(7826);if(!scratch)return -1;memcpy(scratch,b,7826);
 int result=MeleePc_FigaTreeBody(scratch,7826,7036,178,7016);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+62592;
 if(MeleePc_DatBe32(b)!=8046 || MeleePc_DatBe32(b+4)!=7252 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+7996)!=7232)return -1;
 uint32_t no=MeleePc_DatBe32(b+8000);
 if(no>=42 || !memchr(b+8004+no,0,42-no) || strcmp((const char*)b+8004+no,"PlyLuigi5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(8046);if(!scratch)return -1;memcpy(scratch,b,8046);
 int result=MeleePc_FigaTreeBody(scratch,8046,7252,178,7232);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+7840;
 if(MeleePc_DatBe32(b)!=13222 || MeleePc_DatBe32(b+4)!=12432 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+13176)!=12412)return -1;
 uint32_t no=MeleePc_DatBe32(b+13180);
 if(no>=38 || !memchr(b+13184+no,0,38-no) || strcmp((const char*)b+13184+no,"PlyLuigi5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(13222);if(!scratch)return -1;memcpy(scratch,b,13222);
 int result=MeleePc_FigaTreeBody(scratch,13222,12432,178,12412);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+70656;
 if(MeleePc_DatBe32(b)!=9022 || MeleePc_DatBe32(b+4)!=8228 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+8972)!=8208)return -1;
 uint32_t no=MeleePc_DatBe32(b+8976);
 if(no>=42 || !memchr(b+8980+no,0,42-no) || strcmp((const char*)b+8980+no,"PlyLuigi5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(9022);if(!scratch)return -1;memcpy(scratch,b,9022);
 int result=MeleePc_FigaTreeBody(scratch,9022,8228,178,8208);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+21088;
 if(MeleePc_DatBe32(b)!=19410 || MeleePc_DatBe32(b+4)!=18620 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+19364)!=18600)return -1;
 uint32_t no=MeleePc_DatBe32(b+19368);
 if(no>=38 || !memchr(b+19372+no,0,38-no) || strcmp((const char*)b+19372+no,"PlyLuigi5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(19410);if(!scratch)return -1;memcpy(scratch,b,19410);
 int result=MeleePc_FigaTreeBody(scratch,19410,18620,178,18600);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+79680;
 if(MeleePc_DatBe32(b)!=6026 || MeleePc_DatBe32(b+4)!=5232 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5976)!=5212)return -1;
 uint32_t no=MeleePc_DatBe32(b+5980);
 if(no>=42 || !memchr(b+5984+no,0,42-no) || strcmp((const char*)b+5984+no,"PlyLuigi5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(6026);if(!scratch)return -1;memcpy(scratch,b,6026);
 int result=MeleePc_FigaTreeBody(scratch,6026,5232,178,5212);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+40512;
 if(MeleePc_DatBe32(b)!=13422 || MeleePc_DatBe32(b+4)!=12628 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+13372)!=12608)return -1;
 uint32_t no=MeleePc_DatBe32(b+13376);
 if(no>=42 || !memchr(b+13380+no,0,42-no) || strcmp((const char*)b+13380+no,"PlyLuigi5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(13422);if(!scratch)return -1;memcpy(scratch,b,13422);
 int result=MeleePc_FigaTreeBody(scratch,13422,12628,178,12608);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+85728;
 if(MeleePc_DatBe32(b)!=9022 || MeleePc_DatBe32(b+4)!=8224 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+8968)!=8204)return -1;
 uint32_t no=MeleePc_DatBe32(b+8972);
 if(no>=46 || !memchr(b+8976+no,0,46-no) || strcmp((const char*)b+8976+no,"PlyLuigi5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(9022);if(!scratch)return -1;memcpy(scratch,b,9022);
 int result=MeleePc_FigaTreeBody(scratch,9022,8224,178,8204);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+53952;
 if(MeleePc_DatBe32(b)!=8638 || MeleePc_DatBe32(b+4)!=7848 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+8592)!=7828)return -1;
 uint32_t no=MeleePc_DatBe32(b+8596);
 if(no>=38 || !memchr(b+8600+no,0,38-no) || strcmp((const char*)b+8600+no,"PlyLuigi5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(8638);if(!scratch)return -1;memcpy(scratch,b,8638);
 int result=MeleePc_FigaTreeBody(scratch,8638,7848,178,7828);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
