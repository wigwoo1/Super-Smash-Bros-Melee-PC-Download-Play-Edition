#ifndef MELEE_PC_SAMUS_RESULTS_MOTION_H
#define MELEE_PC_SAMUS_RESULTS_MOTION_H
/* Original demo-table slots0..9; slot4 is empty. Metadata only. Validate all
 * deferred FigaTrees on scratch, leaving outer body/packed samples untouched. */
typedef struct {
 uint32_t offset,length,body,relocs,root;const char* name;
} MeleePc_SamusResultsProfile;
static const MeleePc_SamusResultsProfile MeleePc_SamusResultsProfiles[]={
 {0,12118,11536,126,11516,"PlySamus5K_Share_ACTION_Win1_figatree"},
 {64576,7758,7172,126,7152,"PlySamus5K_Share_ACTION_Win1Wait_figatree"},
 {12128,16638,16056,126,16036,"PlySamus5K_Share_ACTION_Win2_figatree"},
 {72352,3642,3056,126,3036,"PlySamus5K_Share_ACTION_Win2Wait_figatree"},
 {28768,15310,14728,126,14708,"PlySamus5K_Share_ACTION_Win3_figatree"},
 {76000,4138,3552,126,3532,"PlySamus5K_Share_ACTION_Win3Wait_figatree"},
 {44096,15954,15368,126,15348,"PlySamus5K_Share_ACTION_Selected_figatree"},
 {80160,3874,3284,126,3264,"PlySamus5K_Share_ACTION_SelectedWait_figatree"},
 {60064,4494,3912,126,3892,"PlySamus5K_Share_ACTION_Lose_figatree"},
};
static int MeleePc_SamusResultsKnownInner(size_t length,uint32_t body,uint32_t relocs,uint32_t root,const char* name){
 for(unsigned i=0;i<sizeof(MeleePc_SamusResultsProfiles)/sizeof(*MeleePc_SamusResultsProfiles);i++){
  const MeleePc_SamusResultsProfile* p=&MeleePc_SamusResultsProfiles[i];
  if(length==p->length&&body==p->body&&relocs==p->relocs&&root==p->root&&!strcmp(name,p->name))return 1;
 }
 return 0;
}
static int MeleePc_SamusResultsMotion(const unsigned char* bytes,size_t length){
 if(!bytes||length!=84132)return -2;
 for(unsigned i=0;i<sizeof(MeleePc_SamusResultsProfiles)/sizeof(*MeleePc_SamusResultsProfiles);i++){
  const MeleePc_SamusResultsProfile* p=&MeleePc_SamusResultsProfiles[i];
  if(p->offset>84064||p->length>84064-p->offset)return -1;
  const unsigned char* b=bytes+32+p->offset;
  if(MeleePc_DatBe32(b)!=p->length||MeleePc_DatBe32(b+4)!=p->body||MeleePc_DatBe32(b+8)!=p->relocs||MeleePc_DatBe32(b+12)!=1||MeleePc_DatBe32(b+16)!=0)return -1;
  const size_t table=32+p->body+p->relocs*4,strings=table+8;
  if(strings>p->length||MeleePc_DatBe32(b+table)!=p->root)return -1;
  const uint32_t no=MeleePc_DatBe32(b+table+4);
  if(no>=p->length-strings||!memchr(b+strings+no,0,p->length-strings-no)||strcmp((const char*)b+strings+no,p->name))return -1;
  unsigned char* scratch=malloc(p->length);if(!scratch)return -1;
  memcpy(scratch,b,p->length);
  int result=MeleePc_FigaTreeBody(scratch,p->length,p->body,p->relocs,p->root);
  free(scratch);if(result)return result;
 }
 return 0;
}
#endif
