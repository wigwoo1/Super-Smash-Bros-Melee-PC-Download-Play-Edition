#ifndef MELEE_PC_LUIGI_VIWAIT_MOTION_H
#define MELEE_PC_LUIGI_VIWAIT_MOTION_H
/* Validate all deferred embedded archives on scratch before mutating outer metadata. */
static int MeleePc_LuigiViWaitMotion(const unsigned char* bytes,size_t length){
 if(length!=6084)return -2;
 {
 const unsigned char* b=bytes+32+0;
 if(MeleePc_DatBe32(b)!=6015 || MeleePc_DatBe32(b+4)!=5364 || MeleePc_DatBe32(b+8)!=143 || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16)!=0)return -1;
 if(MeleePc_DatBe32(b+5968)!=5344)return -1;
 uint32_t no=MeleePc_DatBe32(b+5972);
 if(no>=39 || !memchr(b+5976+no,0,39-no) || strcmp((const char*)b+5976+no,"PlyLuigi5K_Share_ACTION_Wait1_figatree"))return -1;
 unsigned char* scratch=malloc(6015);if(!scratch)return -1;memcpy(scratch,b,6015);
 int result=MeleePc_FigaTreeBody(scratch,6015,5364,143,5344);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
