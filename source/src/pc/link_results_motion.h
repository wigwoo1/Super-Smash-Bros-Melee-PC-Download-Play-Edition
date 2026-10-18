#ifndef MELEE_PC_LINK_RESULTS_MOTION_H
#define MELEE_PC_LINK_RESULTS_MOTION_H
/* Validate deferred embedded archives on scratch; preserve source body bytes. */
static int MeleePc_LinkResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=70243)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=11265 || MeleePc_DatBe32(b+4)!=10512 || MeleePc_DatBe32(b+8)!=169 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+11220)!=10492)return -1;
 uint32_t no=MeleePc_DatBe32(b+11224);
 if(no>=37 || !memchr(b+11228+no,0,37-no) || strcmp((const char*)b+11228+no,"PlyLink5K_Share_ACTION_Win1_figatree"))return -1;
 unsigned char* scratch=malloc(11265);if(!scratch)return -1;memcpy(scratch,b,11265);
 int result=MeleePc_FigaTreeBody(scratch,11265,10512,169,10492);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+51744;
 if(MeleePc_DatBe32(b)!=4605 || MeleePc_DatBe32(b+4)!=3848 || MeleePc_DatBe32(b+8)!=169 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4556)!=3828)return -1;
 uint32_t no=MeleePc_DatBe32(b+4560);
 if(no>=41 || !memchr(b+4564+no,0,41-no) || strcmp((const char*)b+4564+no,"PlyLink5K_Share_ACTION_Win1Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4605);if(!scratch)return -1;memcpy(scratch,b,4605);
 int result=MeleePc_FigaTreeBody(scratch,4605,3848,169,3828);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+11296;
 if(MeleePc_DatBe32(b)!=11209 || MeleePc_DatBe32(b+4)!=10456 || MeleePc_DatBe32(b+8)!=169 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+11164)!=10436)return -1;
 uint32_t no=MeleePc_DatBe32(b+11168);
 if(no>=37 || !memchr(b+11172+no,0,37-no) || strcmp((const char*)b+11172+no,"PlyLink5K_Share_ACTION_Win2_figatree"))return -1;
 unsigned char* scratch=malloc(11209);if(!scratch)return -1;memcpy(scratch,b,11209);
 int result=MeleePc_FigaTreeBody(scratch,11209,10456,169,10436);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+56352;
 if(MeleePc_DatBe32(b)!=4645 || MeleePc_DatBe32(b+4)!=3888 || MeleePc_DatBe32(b+8)!=169 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4596)!=3868)return -1;
 uint32_t no=MeleePc_DatBe32(b+4600);
 if(no>=41 || !memchr(b+4604+no,0,41-no) || strcmp((const char*)b+4604+no,"PlyLink5K_Share_ACTION_Win2Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4645);if(!scratch)return -1;memcpy(scratch,b,4645);
 int result=MeleePc_FigaTreeBody(scratch,4645,3888,169,3868);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+22528;
 if(MeleePc_DatBe32(b)!=9565 || MeleePc_DatBe32(b+4)!=8812 || MeleePc_DatBe32(b+8)!=169 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+9520)!=8792)return -1;
 uint32_t no=MeleePc_DatBe32(b+9524);
 if(no>=37 || !memchr(b+9528+no,0,37-no) || strcmp((const char*)b+9528+no,"PlyLink5K_Share_ACTION_Win3_figatree"))return -1;
 unsigned char* scratch=malloc(9565);if(!scratch)return -1;memcpy(scratch,b,9565);
 int result=MeleePc_FigaTreeBody(scratch,9565,8812,169,8792);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+61024;
 if(MeleePc_DatBe32(b)!=4505 || MeleePc_DatBe32(b+4)!=3748 || MeleePc_DatBe32(b+8)!=169 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4456)!=3728)return -1;
 uint32_t no=MeleePc_DatBe32(b+4460);
 if(no>=41 || !memchr(b+4464+no,0,41-no) || strcmp((const char*)b+4464+no,"PlyLink5K_Share_ACTION_Win3Wait_figatree"))return -1;
 unsigned char* scratch=malloc(4505);if(!scratch)return -1;memcpy(scratch,b,4505);
 int result=MeleePc_FigaTreeBody(scratch,4505,3748,169,3728);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+32096;
 if(MeleePc_DatBe32(b)!=11457 || MeleePc_DatBe32(b+4)!=10700 || MeleePc_DatBe32(b+8)!=169 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+11408)!=10680)return -1;
 uint32_t no=MeleePc_DatBe32(b+11412);
 if(no>=41 || !memchr(b+11416+no,0,41-no) || strcmp((const char*)b+11416+no,"PlyLink5K_Share_ACTION_Selected_figatree"))return -1;
 unsigned char* scratch=malloc(11457);if(!scratch)return -1;memcpy(scratch,b,11457);
 int result=MeleePc_FigaTreeBody(scratch,11457,10700,169,10680);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+65536;
 if(MeleePc_DatBe32(b)!=4637 || MeleePc_DatBe32(b+4)!=3876 || MeleePc_DatBe32(b+8)!=169 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+4584)!=3856)return -1;
 uint32_t no=MeleePc_DatBe32(b+4588);
 if(no>=45 || !memchr(b+4592+no,0,45-no) || strcmp((const char*)b+4592+no,"PlyLink5K_Share_ACTION_SelectedWait_figatree"))return -1;
 unsigned char* scratch=malloc(4637);if(!scratch)return -1;memcpy(scratch,b,4637);
 int result=MeleePc_FigaTreeBody(scratch,4637,3876,169,3856);free(scratch);if(result)return result;
 }
 {
 const unsigned char* b=bytes+32+43584;
 if(MeleePc_DatBe32(b)!=8141 || MeleePc_DatBe32(b+4)!=7388 || MeleePc_DatBe32(b+8)!=169 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+8096)!=7368)return -1;
 uint32_t no=MeleePc_DatBe32(b+8100);
 if(no>=37 || !memchr(b+8104+no,0,37-no) || strcmp((const char*)b+8104+no,"PlyLink5K_Share_ACTION_Lose_figatree"))return -1;
 unsigned char* scratch=malloc(8141);if(!scratch)return -1;memcpy(scratch,b,8141);
 int result=MeleePc_FigaTreeBody(scratch,8141,7388,169,7368);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
