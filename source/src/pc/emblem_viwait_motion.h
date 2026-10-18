#ifndef MELEE_PC_EMBLEM_VIWAIT_MOTION_H
#define MELEE_PC_EMBLEM_VIWAIT_MOTION_H
static int MeleePc_EmblemViWaitMotion(const unsigned char* bytes,size_t length){
 if(length!=24677)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=24580 || MeleePc_DatBe32(b+4)!=23864 || MeleePc_DatBe32(b+8)!=159 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+24532)!=23844)return -1;
 uint32_t no=MeleePc_DatBe32(b+24536);
 if(no>=40 || !memchr(b+24540+no,0,40-no) || strcmp((const char*)b+24540+no,"PlyEmblem5K_Share_ACTION_Wait1_figatree"))return -1;
 unsigned char* scratch=malloc(24580);if(!scratch)return -1;memcpy(scratch,b,24580);
 int result=MeleePc_FigaTreeBody(scratch,24580,23864,159,23844);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
