#ifndef MELEE_PC_MARS_RESULTS_MOTION_H
#define MELEE_PC_MARS_RESULTS_MOTION_H
/* Validate deferred embedded archives on scratch; preserve source body bytes. */
static int MeleePc_MarsResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=78339)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=16033 || MeleePc_DatBe32(b+4)!=15192 || MeleePc_DatBe32(b+8)!=191 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+15988)!=15172)return -1;
 uint32_t no=MeleePc_DatBe32(b+15992);
 if(no>=37 || !memchr(b+15996+no,0,37-no) || strcmp((const char*)b+15996+no,"PlyMars5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(16033);if(!scratch)return -1;memcpy(scratch,b,16033);
 int result=MeleePc_FigaTreeBody(scratch,16033,15192,191,15172);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+59680;
 if(MeleePc_DatBe32(b)!=4773 || MeleePc_DatBe32(b+4)!=3928 || MeleePc_DatBe32(b+8)!=191 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4724)!=3908)return -1;
 uint32_t no=MeleePc_DatBe32(b+4728);
 if(no>=41 || !memchr(b+4732+no,0,41-no) || strcmp((const char*)b+4732+no,"PlyMars5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4773);if(!scratch)return -1;memcpy(scratch,b,4773);
 int result=MeleePc_FigaTreeBody(scratch,4773,3928,191,3908);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+16064;
 if(MeleePc_DatBe32(b)!=12297 || MeleePc_DatBe32(b+4)!=11456 || MeleePc_DatBe32(b+8)!=191 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+12252)!=11436)return -1;
 uint32_t no=MeleePc_DatBe32(b+12256);
 if(no>=37 || !memchr(b+12260+no,0,37-no) || strcmp((const char*)b+12260+no,"PlyMars5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(12297);if(!scratch)return -1;memcpy(scratch,b,12297);
 int result=MeleePc_FigaTreeBody(scratch,12297,11456,191,11436);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+64480;
 if(MeleePc_DatBe32(b)!=4761 || MeleePc_DatBe32(b+4)!=3916 || MeleePc_DatBe32(b+8)!=191 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4712)!=3896)return -1;
 uint32_t no=MeleePc_DatBe32(b+4716);
 if(no>=41 || !memchr(b+4720+no,0,41-no) || strcmp((const char*)b+4720+no,"PlyMars5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4761);if(!scratch)return -1;memcpy(scratch,b,4761);
 int result=MeleePc_FigaTreeBody(scratch,4761,3916,191,3896);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+28384;
 if(MeleePc_DatBe32(b)!=15037 || MeleePc_DatBe32(b+4)!=14196 || MeleePc_DatBe32(b+8)!=191 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+14992)!=14176)return -1;
 uint32_t no=MeleePc_DatBe32(b+14996);
 if(no>=37 || !memchr(b+15000+no,0,37-no) || strcmp((const char*)b+15000+no,"PlyMars5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(15037);if(!scratch)return -1;memcpy(scratch,b,15037);
 int result=MeleePc_FigaTreeBody(scratch,15037,14196,191,14176);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+69248;
 if(MeleePc_DatBe32(b)!=4769 || MeleePc_DatBe32(b+4)!=3924 || MeleePc_DatBe32(b+8)!=191 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4720)!=3904)return -1;
 uint32_t no=MeleePc_DatBe32(b+4724);
 if(no>=41 || !memchr(b+4728+no,0,41-no) || strcmp((const char*)b+4728+no,"PlyMars5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4769);if(!scratch)return -1;memcpy(scratch,b,4769);
 int result=MeleePc_FigaTreeBody(scratch,4769,3924,191,3904);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+43424;
 if(MeleePc_DatBe32(b)!=5673 || MeleePc_DatBe32(b+4)!=4828 || MeleePc_DatBe32(b+8)!=191 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5624)!=4808)return -1;
 uint32_t no=MeleePc_DatBe32(b+5628);
 if(no>=41 || !memchr(b+5632+no,0,41-no) || strcmp((const char*)b+5632+no,"PlyMars5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(5673);if(!scratch)return -1;memcpy(scratch,b,5673);
 int result=MeleePc_FigaTreeBody(scratch,5673,4828,191,4808);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+74048;
 if(MeleePc_DatBe32(b)!=4209 || MeleePc_DatBe32(b+4)!=3572 || MeleePc_DatBe32(b+8)!=138 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4156)!=3552)return -1;
 uint32_t no=MeleePc_DatBe32(b+4160);
 if(no>=45 || !memchr(b+4164+no,0,45-no) || strcmp((const char*)b+4164+no,"PlyMars5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(4209);if(!scratch)return -1;memcpy(scratch,b,4209);
 int result=MeleePc_FigaTreeBody(scratch,4209,3572,138,3552);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+49120;
 if(MeleePc_DatBe32(b)!=10553 || MeleePc_DatBe32(b+4)!=9712 || MeleePc_DatBe32(b+8)!=191 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+10508)!=9692)return -1;
 uint32_t no=MeleePc_DatBe32(b+10512);
 if(no>=37 || !memchr(b+10516+no,0,37-no) || strcmp((const char*)b+10516+no,"PlyMars5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(10553);if(!scratch)return -1;memcpy(scratch,b,10553);
 int result=MeleePc_FigaTreeBody(scratch,10553,9712,191,9692);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
