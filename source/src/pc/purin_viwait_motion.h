#ifndef MELEE_PC_PURIN_VIWAIT_MOTION_H
#define MELEE_PC_PURIN_VIWAIT_MOTION_H
/* Validate all deferred embedded archives on scratch before mutating outer metadata. */
static int MeleePc_PurinViWaitMotion(const unsigned char* bytes,size_t length){
 if(length!=12132)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=12050 || MeleePc_DatBe32(b+4)!=11636 || MeleePc_DatBe32(b+8)!=84 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+12004)!=11616)return -1;
 uint32_t no=MeleePc_DatBe32(b+12008);
 if(no>=38 || !memchr(b+12012+no,0,38-no) || strcmp((const char*)b+12012+no,"PlyPurin5K_Share_ACTION_Wait_figatree"))return -1;
 unsigned char* scratch=malloc(12050);if(!scratch)return -1;memcpy(scratch,b,12050);
 int result=MeleePc_FigaTreeBody(scratch,12050,11636,84,11616);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
