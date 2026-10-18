#ifndef MELEE_PC_CAPTAIN_RESULTS_MOTION_H
#define MELEE_PC_CAPTAIN_RESULTS_MOTION_H
/* Validate deferred embedded archives on scratch; preserve source body bytes. */
static int MeleePc_CaptainResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=96614)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=15384 || MeleePc_DatBe32(b+4)!=14268 || MeleePc_DatBe32(b+8)!=259 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+15336)!=14248)return -1;
 uint32_t no=MeleePc_DatBe32(b+15340);
 if(no>=40 || !memchr(b+15344+no,0,40-no) || strcmp((const char*)b+15344+no,"PlyCaptain5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(15384);if(!scratch)return -1;memcpy(scratch,b,15384);
 int result=MeleePc_FigaTreeBody(scratch,15384,14268,259,14248);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+68224;
 if(MeleePc_DatBe32(b)!=6992 || MeleePc_DatBe32(b+4)!=5872 || MeleePc_DatBe32(b+8)!=259 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6940)!=5852)return -1;
 uint32_t no=MeleePc_DatBe32(b+6944);
 if(no>=44 || !memchr(b+6948+no,0,44-no) || strcmp((const char*)b+6948+no,"PlyCaptain5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(6992);if(!scratch)return -1;memcpy(scratch,b,6992);
 int result=MeleePc_FigaTreeBody(scratch,6992,5872,259,5852);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+15392;
 if(MeleePc_DatBe32(b)!=12536 || MeleePc_DatBe32(b+4)!=11420 || MeleePc_DatBe32(b+8)!=259 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+12488)!=11400)return -1;
 uint32_t no=MeleePc_DatBe32(b+12492);
 if(no>=40 || !memchr(b+12496+no,0,40-no) || strcmp((const char*)b+12496+no,"PlyCaptain5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(12536);if(!scratch)return -1;memcpy(scratch,b,12536);
 int result=MeleePc_FigaTreeBody(scratch,12536,11420,259,11400);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+75232;
 if(MeleePc_DatBe32(b)!=7028 || MeleePc_DatBe32(b+4)!=5908 || MeleePc_DatBe32(b+8)!=259 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6976)!=5888)return -1;
 uint32_t no=MeleePc_DatBe32(b+6980);
 if(no>=44 || !memchr(b+6984+no,0,44-no) || strcmp((const char*)b+6984+no,"PlyCaptain5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(7028);if(!scratch)return -1;memcpy(scratch,b,7028);
 int result=MeleePc_FigaTreeBody(scratch,7028,5908,259,5888);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+27936;
 if(MeleePc_DatBe32(b)!=13548 || MeleePc_DatBe32(b+4)!=12432 || MeleePc_DatBe32(b+8)!=259 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+13500)!=12412)return -1;
 uint32_t no=MeleePc_DatBe32(b+13504);
 if(no>=40 || !memchr(b+13508+no,0,40-no) || strcmp((const char*)b+13508+no,"PlyCaptain5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(13548);if(!scratch)return -1;memcpy(scratch,b,13548);
 int result=MeleePc_FigaTreeBody(scratch,13548,12432,259,12412);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+82272;
 if(MeleePc_DatBe32(b)!=7244 || MeleePc_DatBe32(b+4)!=6124 || MeleePc_DatBe32(b+8)!=259 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+7192)!=6104)return -1;
 uint32_t no=MeleePc_DatBe32(b+7196);
 if(no>=44 || !memchr(b+7200+no,0,44-no) || strcmp((const char*)b+7200+no,"PlyCaptain5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(7244);if(!scratch)return -1;memcpy(scratch,b,7244);
 int result=MeleePc_FigaTreeBody(scratch,7244,6124,259,6104);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+41504;
 if(MeleePc_DatBe32(b)!=14744 || MeleePc_DatBe32(b+4)!=13624 || MeleePc_DatBe32(b+8)!=259 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+14692)!=13604)return -1;
 uint32_t no=MeleePc_DatBe32(b+14696);
 if(no>=44 || !memchr(b+14700+no,0,44-no) || strcmp((const char*)b+14700+no,"PlyCaptain5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(14744);if(!scratch)return -1;memcpy(scratch,b,14744);
 int result=MeleePc_FigaTreeBody(scratch,14744,13624,259,13604);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+89536;
 if(MeleePc_DatBe32(b)!=6984 || MeleePc_DatBe32(b+4)!=5860 || MeleePc_DatBe32(b+8)!=259 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+6928)!=5840)return -1;
 uint32_t no=MeleePc_DatBe32(b+6932);
 if(no>=48 || !memchr(b+6936+no,0,48-no) || strcmp((const char*)b+6936+no,"PlyCaptain5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(6984);if(!scratch)return -1;memcpy(scratch,b,6984);
 int result=MeleePc_FigaTreeBody(scratch,6984,5860,259,5840);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+56256;
 if(MeleePc_DatBe32(b)!=11952 || MeleePc_DatBe32(b+4)!=10836 || MeleePc_DatBe32(b+8)!=259 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+11904)!=10816)return -1;
 uint32_t no=MeleePc_DatBe32(b+11908);
 if(no>=40 || !memchr(b+11912+no,0,40-no) || strcmp((const char*)b+11912+no,"PlyCaptain5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(11952);if(!scratch)return -1;memcpy(scratch,b,11952);
 int result=MeleePc_FigaTreeBody(scratch,11952,10836,259,10816);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
