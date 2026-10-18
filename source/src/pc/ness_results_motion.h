#ifndef MELEE_PC_NESS_RESULTS_MOTION_H
#define MELEE_PC_NESS_RESULTS_MOTION_H
/* Original demo-table slots0..9; slot4 is empty. Metadata only. Validate all
 * deferred FigaTrees on scratch, leaving outer body/packed samples untouched. */
typedef struct {
 uint32_t offset,length,body,relocs,root;const char* name;
} MeleePc_NessResultsProfile;
static const MeleePc_NessResultsProfile MeleePc_NessResultsProfiles[]={
 {0,12121,11508,134,11488,"PlyNess5K_Share_ACTION_Win1_figatree"},
 {73824,3825,3208,134,3188,"PlyNess5K_Share_ACTION_Win1Wait_figatree"},
 {12128,22261,21648,134,21628,"PlyNess5K_Share_ACTION_Win2_figatree"},
 {77664,5233,4616,134,4596,"PlyNess5K_Share_ACTION_Win2Wait_figatree"},
 {34400,24405,23792,134,23772,"PlyNess5K_Share_ACTION_Win3_figatree"},
 {82912,4829,4212,134,4192,"PlyNess5K_Share_ACTION_Win3Wait_figatree"},
 {58816,10673,10056,134,10036,"PlyNess5K_Share_ACTION_Selected_figatree"},
 {87744,3821,3200,134,3180,"PlyNess5K_Share_ACTION_SelectedWait_figatree"},
 {69504,4289,3676,134,3656,"PlyNess5K_Share_ACTION_Lose_figatree"},
};
static int MeleePc_NessResultsKnownInner(size_t length,uint32_t body,uint32_t relocs,uint32_t root,const char* name){
 for(unsigned i=0;i<sizeof(MeleePc_NessResultsProfiles)/sizeof(*MeleePc_NessResultsProfiles);i++){
  const MeleePc_NessResultsProfile* p=&MeleePc_NessResultsProfiles[i];
  if(length==p->length&&body==p->body&&relocs==p->relocs&&root==p->root&&!strcmp(name,p->name))return 1;
 }
 return 0;
}
static int MeleePc_NessResultsMotion(const unsigned char* bytes,size_t length){
 if(!bytes||length!=91651)return -2;
 for(unsigned i=0;i<sizeof(MeleePc_NessResultsProfiles)/sizeof(*MeleePc_NessResultsProfiles);i++){
  const MeleePc_NessResultsProfile* p=&MeleePc_NessResultsProfiles[i];
  if(p->offset>91584||p->length>91584-p->offset)return -1;
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
