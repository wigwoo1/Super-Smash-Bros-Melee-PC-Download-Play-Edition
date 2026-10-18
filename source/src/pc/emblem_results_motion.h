#ifndef MELEE_PC_EMBLEM_RESULTS_MOTION_H
#define MELEE_PC_EMBLEM_RESULTS_MOTION_H
/* Validate deferred inner archives without changing original body bytes. */
static int MeleePc_EmblemResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=73253)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=11555 || MeleePc_DatBe32(b+4)!=10764 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+11508)!=10744)return -1;
 uint32_t no=MeleePc_DatBe32(b+11512);
 if(no>=39 || !memchr(b+11516+no,0,39-no) || strcmp((const char*)b+11516+no,"PlyEmblem5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(11555);if(!scratch)return -1;memcpy(scratch,b,11555);
 int result=MeleePc_FigaTreeBody(scratch,11555,10764,178,10744);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+55552;
 if(MeleePc_DatBe32(b)!=4635 || MeleePc_DatBe32(b+4)!=3964 || MeleePc_DatBe32(b+8)!=147 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4584)!=3944)return -1;
 uint32_t no=MeleePc_DatBe32(b+4588);
 if(no>=43 || !memchr(b+4592+no,0,43-no) || strcmp((const char*)b+4592+no,"PlyEmblem5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4635);if(!scratch)return -1;memcpy(scratch,b,4635);
 int result=MeleePc_FigaTreeBody(scratch,4635,3964,147,3944);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+11584;
 if(MeleePc_DatBe32(b)!=10711 || MeleePc_DatBe32(b+4)!=9920 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+10664)!=9900)return -1;
 uint32_t no=MeleePc_DatBe32(b+10668);
 if(no>=39 || !memchr(b+10672+no,0,39-no) || strcmp((const char*)b+10672+no,"PlyEmblem5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(10711);if(!scratch)return -1;memcpy(scratch,b,10711);
 int result=MeleePc_FigaTreeBody(scratch,10711,9920,178,9900);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+60192;
 if(MeleePc_DatBe32(b)!=3907 || MeleePc_DatBe32(b+4)!=3236 || MeleePc_DatBe32(b+8)!=147 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+3856)!=3216)return -1;
 uint32_t no=MeleePc_DatBe32(b+3860);
 if(no>=43 || !memchr(b+3864+no,0,43-no) || strcmp((const char*)b+3864+no,"PlyEmblem5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(3907);if(!scratch)return -1;memcpy(scratch,b,3907);
 int result=MeleePc_FigaTreeBody(scratch,3907,3236,147,3216);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+22304;
 if(MeleePc_DatBe32(b)!=9883 || MeleePc_DatBe32(b+4)!=9092 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+9836)!=9072)return -1;
 uint32_t no=MeleePc_DatBe32(b+9840);
 if(no>=39 || !memchr(b+9844+no,0,39-no) || strcmp((const char*)b+9844+no,"PlyEmblem5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(9883);if(!scratch)return -1;memcpy(scratch,b,9883);
 int result=MeleePc_FigaTreeBody(scratch,9883,9092,178,9072);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+64128;
 if(MeleePc_DatBe32(b)!=4859 || MeleePc_DatBe32(b+4)!=4188 || MeleePc_DatBe32(b+8)!=147 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4808)!=4168)return -1;
 uint32_t no=MeleePc_DatBe32(b+4812);
 if(no>=43 || !memchr(b+4816+no,0,43-no) || strcmp((const char*)b+4816+no,"PlyEmblem5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4859);if(!scratch)return -1;memcpy(scratch,b,4859);
 int result=MeleePc_FigaTreeBody(scratch,4859,4188,147,4168);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+32192;
 if(MeleePc_DatBe32(b)!=12471 || MeleePc_DatBe32(b+4)!=11800 || MeleePc_DatBe32(b+8)!=147 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+12420)!=11780)return -1;
 uint32_t no=MeleePc_DatBe32(b+12424);
 if(no>=43 || !memchr(b+12428+no,0,43-no) || strcmp((const char*)b+12428+no,"PlyEmblem5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(12471);if(!scratch)return -1;memcpy(scratch,b,12471);
 int result=MeleePc_FigaTreeBody(scratch,12471,11800,147,11780);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+68992;
 if(MeleePc_DatBe32(b)!=4179 || MeleePc_DatBe32(b+4)!=3504 || MeleePc_DatBe32(b+8)!=147 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4124)!=3484)return -1;
 uint32_t no=MeleePc_DatBe32(b+4128);
 if(no>=47 || !memchr(b+4132+no,0,47-no) || strcmp((const char*)b+4132+no,"PlyEmblem5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(4179);if(!scratch)return -1;memcpy(scratch,b,4179);
 int result=MeleePc_FigaTreeBody(scratch,4179,3504,147,3484);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+44672;
 if(MeleePc_DatBe32(b)!=10867 || MeleePc_DatBe32(b+4)!=10076 || MeleePc_DatBe32(b+8)!=178 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+10820)!=10056)return -1;
 uint32_t no=MeleePc_DatBe32(b+10824);
 if(no>=39 || !memchr(b+10828+no,0,39-no) || strcmp((const char*)b+10828+no,"PlyEmblem5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(10867);if(!scratch)return -1;memcpy(scratch,b,10867);
 int result=MeleePc_FigaTreeBody(scratch,10867,10076,178,10056);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
