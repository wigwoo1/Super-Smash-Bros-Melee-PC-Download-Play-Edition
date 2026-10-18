#ifndef MELEE_PC_STANDARD_RESULTS_MOTION_H
#define MELEE_PC_STANDARD_RESULTS_MOTION_H
/* Exact source-selected Results slots0..9 for seven standard fighters.
 * Slot4 is empty; intro/ending archives are excluded. Metadata only.
 * Validate deferred inner FigaTrees on scratch; preserve packed samples. */
typedef struct {
 uint32_t offset,length,body,relocs,root;const char* name;
} MeleePc_StandardResultsProfile;
typedef struct {
 uint32_t length,body,first,count;const char* name;
} MeleePc_StandardResultsContainer;
static const MeleePc_StandardResultsProfile MeleePc_StandardResultsProfiles[]={
 /* Donkey */
 {0,10951,10240,158,10220,"PlyDonkey5K_Share_ACTION_Win1_figatree"},
 {49312,4023,3308,158,3288,"PlyDonkey5K_Share_ACTION_Win1Wait_figatree"},
 {10976,12419,11708,158,11688,"PlyDonkey5K_Share_ACTION_Win2_figatree"},
 {53344,4179,3464,158,3444,"PlyDonkey5K_Share_ACTION_Win2Wait_figatree"},
 {23424,9751,9040,158,9020,"PlyDonkey5K_Share_ACTION_Win3_figatree"},
 {57536,4115,3400,158,3380,"PlyDonkey5K_Share_ACTION_Win3Wait_figatree"},
 {33184,10111,9396,158,9376,"PlyDonkey5K_Share_ACTION_Selected_figatree"},
 {61664,4043,3324,158,3304,"PlyDonkey5K_Share_ACTION_SelectedWait_figatree"},
 {43296,6011,5300,158,5280,"PlyDonkey5K_Share_ACTION_Lose_figatree"},
 /* Peach */
 {0,19246,18300,217,18280,"PlyPeach5K_Share_ACTION_Win1_figatree"},
 {84512,6222,5272,217,5252,"PlyPeach5K_Share_ACTION_Win1Wait_figatree"},
 {19264,19730,18784,217,18764,"PlyPeach5K_Share_ACTION_Win2_figatree"},
 {90752,6930,5980,217,5960,"PlyPeach5K_Share_ACTION_Win2Wait_figatree"},
 {39008,19442,18496,217,18476,"PlyPeach5K_Share_ACTION_Win3_figatree"},
 {97696,7074,6124,217,6104,"PlyPeach5K_Share_ACTION_Win3Wait_figatree"},
 {58464,16470,15520,217,15500,"PlyPeach5K_Share_ACTION_Selected_figatree"},
 {104800,6178,5224,217,5204,"PlyPeach5K_Share_ACTION_SelectedWait_figatree"},
 {74944,9554,8608,217,8588,"PlyPeach5K_Share_ACTION_Lose_figatree"},
 /* Zelda */
 {0,9538,8672,197,8652,"PlyZelda5K_Share_ACTION_Win1_figatree"},
 {43744,5790,4920,197,4900,"PlyZelda5K_Share_ACTION_Win1Wait_figatree"},
 {9568,10926,10060,197,10040,"PlyZelda5K_Share_ACTION_Win2_figatree"},
 {49536,5598,4728,197,4708,"PlyZelda5K_Share_ACTION_Win2Wait_figatree"},
 {20512,8370,7504,197,7484,"PlyZelda5K_Share_ACTION_Win3_figatree"},
 {55136,5642,4772,197,4752,"PlyZelda5K_Share_ACTION_Win3Wait_figatree"},
 {28896,7930,7060,197,7040,"PlyZelda5K_Share_ACTION_Selected_figatree"},
 {60800,5546,4672,197,4652,"PlyZelda5K_Share_ACTION_SelectedWait_figatree"},
 {36832,6898,6032,197,6012,"PlyZelda5K_Share_ACTION_Lose_figatree"},
 /* Seak */
 {0,5825,5288,115,5268,"PlySeak5K_Share_ACTION_Win1_figatree"},
 {49088,3693,3152,115,3132,"PlySeak5K_Share_ACTION_Win1Wait_figatree"},
 {5856,14129,13592,115,13572,"PlySeak5K_Share_ACTION_Win2_figatree"},
 {52800,3445,2904,115,2884,"PlySeak5K_Share_ACTION_Win2Wait_figatree"},
 {20000,10709,10172,115,10152,"PlySeak5K_Share_ACTION_Win3_figatree"},
 {56256,3513,2972,115,2952,"PlySeak5K_Share_ACTION_Win3Wait_figatree"},
 {30720,14121,13580,115,13560,"PlySeak5K_Share_ACTION_Selected_figatree"},
 {59776,3409,2864,115,2844,"PlySeak5K_Share_ACTION_SelectedWait_figatree"},
 {44864,4205,3668,115,3648,"PlySeak5K_Share_ACTION_Lose_figatree"},
 /* CLink */
 {0,11330,10448,201,10428,"PlyClink5K_Share_ACTION_Win1_figatree"},
 {51840,5438,4552,201,4532,"PlyClink5K_Share_ACTION_Win1Wait_figatree"},
 {11360,11810,10928,201,10908,"PlyClink5K_Share_ACTION_Win2_figatree"},
 {57280,5390,4504,201,4484,"PlyClink5K_Share_ACTION_Win2Wait_figatree"},
 {23200,13066,12184,201,12164,"PlyClink5K_Share_ACTION_Win3_figatree"},
 {62688,5046,4160,201,4140,"PlyClink5K_Share_ACTION_Win3Wait_figatree"},
 {36288,6474,5588,201,5568,"PlyClink5K_Share_ACTION_Selected_figatree"},
 {67744,5106,4216,201,4196,"PlyClink5K_Share_ACTION_SelectedWait_figatree"},
 {42784,9042,8160,201,8140,"PlyClink5K_Share_ACTION_Lose_figatree"},
 /* Pichu */
 {0,15130,14476,144,14456,"PlyPichu5K_Share_ACTION_Win1_figatree"},
 {55296,6258,5600,144,5580,"PlyPichu5K_Share_ACTION_Win1Wait_figatree"},
 {15136,12926,12272,144,12252,"PlyPichu5K_Share_ACTION_Win2_figatree"},
 {61568,5894,5236,144,5216,"PlyPichu5K_Share_ACTION_Win2Wait_figatree"},
 {28064,14094,13440,144,13420,"PlyPichu5K_Share_ACTION_Win3_figatree"},
 {67488,6226,5568,144,5548,"PlyPichu5K_Share_ACTION_Win3Wait_figatree"},
 {42176,4434,3776,144,3756,"PlyPichu5K_Share_ACTION_Selected_figatree"},
 {73728,4822,4160,144,4140,"PlyPichu5K_Share_ACTION_SelectedWait_figatree"},
 {46624,8666,8012,144,7992,"PlyPichu5K_Share_ACTION_Lose_figatree"},
 /* Ganon */
 {0,9602,8836,172,8816,"PlyGanon5K_Share_ACTION_Win1_figatree"},
 {47584,4394,3624,172,3604,"PlyGanon5K_Share_ACTION_Win1Wait_figatree"},
 {9632,10142,9376,172,9356,"PlyGanon5K_Share_ACTION_Win2_figatree"},
 {52000,4854,4084,172,4064,"PlyGanon5K_Share_ACTION_Win2Wait_figatree"},
 {19776,11434,10668,172,10648,"PlyGanon5K_Share_ACTION_Win3_figatree"},
 {56864,4342,3572,172,3552,"PlyGanon5K_Share_ACTION_Win3Wait_figatree"},
 {31232,4314,3544,172,3524,"PlyGanon5K_Share_ACTION_Selected_figatree"},
 {61216,4318,3544,172,3524,"PlyGanon5K_Share_ACTION_SelectedWait_figatree"},
 {35552,12010,11244,172,11224,"PlyGanon5K_Share_ACTION_Lose_figatree"},
};
static const MeleePc_StandardResultsContainer MeleePc_StandardResultsContainers[]={
 {65797,65728,0,9,"ftDemoResultMotionFileDonkey"},
 {111076,111008,9,9,"ftDemoResultMotionFilePeach"},
 {66436,66368,18,9,"ftDemoResultMotionFileZelda"},
 {63267,63200,27,9,"ftDemoResultMotionFileSeak"},
 {72932,72864,36,9,"ftDemoResultMotionFileClink"},
 {78628,78560,45,9,"ftDemoResultMotionFilePichu"},
 {65604,65536,54,9,"ftDemoResultMotionFileGanon"},
};
static unsigned MeleePc_StandardResultsKnownOuter(size_t length,uint32_t body,const char* name){
 for(unsigned i=0;i<sizeof(MeleePc_StandardResultsContainers)/sizeof(*MeleePc_StandardResultsContainers);i++){
  const MeleePc_StandardResultsContainer* p=&MeleePc_StandardResultsContainers[i];
  if(length==p->length&&body==p->body&&!strcmp(name,p->name))return i+1;
 }
 return 0;
}
static int MeleePc_StandardResultsKnownInner(size_t length,uint32_t body,uint32_t relocs,uint32_t root,const char* name){
 for(unsigned i=0;i<sizeof(MeleePc_StandardResultsProfiles)/sizeof(*MeleePc_StandardResultsProfiles);i++){
  const MeleePc_StandardResultsProfile* p=&MeleePc_StandardResultsProfiles[i];
  if(length==p->length&&body==p->body&&relocs==p->relocs&&root==p->root&&!strcmp(name,p->name))return 1;
 }
 return 0;
}
static int MeleePc_StandardResultsMotion(const unsigned char* bytes,size_t length,unsigned identity){
 if(!bytes||!identity||identity>sizeof(MeleePc_StandardResultsContainers)/sizeof(*MeleePc_StandardResultsContainers))return -2;
 const MeleePc_StandardResultsContainer* outer=&MeleePc_StandardResultsContainers[identity-1];
 if(length!=outer->length)return -2;
 const size_t count=sizeof(MeleePc_StandardResultsProfiles)/sizeof(*MeleePc_StandardResultsProfiles);
 if(outer->first>count||outer->count>count-outer->first)return -1;
 for(unsigned i=0;i<outer->count;i++){
  const MeleePc_StandardResultsProfile* p=&MeleePc_StandardResultsProfiles[outer->first+i];
  if(p->offset>outer->body||p->length>outer->body-p->offset)return -1;
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
