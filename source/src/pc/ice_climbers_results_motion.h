#ifndef MELEE_PC_ICE_CLIMBERS_RESULTS_MOTION_H
#define MELEE_PC_ICE_CLIMBERS_RESULTS_MOTION_H
/* Deferred inner archives remain BE; scratch validation never mutates input. */
static int MeleePc_IceClimbersResultsMotion(const unsigned char* bytes,size_t length){
 if(length!=181190)return -2;
 static const struct {uint32_t offset,size,body,relocs,root;const char* symbol;} profiles[]={
  {0,18725,18244,101,18224,"PlyPopo5K_Share_ACTION_Win1_figatree"},
  {18752,6169,5688,101,5668,"PlyPopo5K_Share_ACTION_Win2_figatree"},
  {24928,5617,5136,101,5116,"PlyPopo5K_Share_ACTION_Win3_figatree"},
  {30560,16241,15756,101,15736,"PlyPopo5K_Share_ACTION_Selected_figatree"},
  {46816,3153,2672,101,2652,"PlyPopo5K_Share_ACTION_Lose_figatree"},
  {49984,16273,15788,101,15768,"PlyPopo5K_Share_ACTION_Win1Wait_figatree"},
  {66272,3261,2776,101,2756,"PlyPopo5K_Share_ACTION_Win2Wait_figatree"},
  {69536,2957,2472,101,2452,"PlyPopo5K_Share_ACTION_Win3Wait_figatree"},
  {72512,16277,15788,101,15768,"PlyPopo5K_Share_ACTION_SelectedWait_figatree"},
  {88800,20061,19576,102,19556,"PlyNana5K_Share_ACTION_Win1_figatree"},
  {108864,6249,5764,102,5744,"PlyNana5K_Share_ACTION_Win2_figatree"},
  {115136,6017,5532,102,5512,"PlyNana5K_Share_ACTION_Win3_figatree"},
  {121184,16745,16256,102,16236,"PlyNana5K_Share_ACTION_Selected_figatree"},
  {137952,3329,2844,102,2824,"PlyNana5K_Share_ACTION_Lose_figatree"},
  {141312,16709,16220,102,16200,"PlyNana5K_Share_ACTION_Win1Wait_figatree"},
  {158048,3093,2604,102,2584,"PlyNana5K_Share_ACTION_Win2Wait_figatree"},
  {161152,3109,2620,102,2600,"PlyNana5K_Share_ACTION_Win3Wait_figatree"},
  {164288,16781,16288,102,16268,"PlyNana5K_Share_ACTION_SelectedWait_figatree"},
 };
 for(size_t i=0;i<sizeof(profiles)/sizeof(profiles[0]);i++){
  const unsigned char* b=bytes+32+profiles[i].offset;uint32_t size=profiles[i].size;
  if(MeleePc_DatBe32(b)!=size || MeleePc_DatBe32(b+4)!=profiles[i].body || MeleePc_DatBe32(b+8)!=profiles[i].relocs || MeleePc_DatBe32(b+12)!=1 || MeleePc_DatBe32(b+16))return -1;
  size_t root=32+profiles[i].body+profiles[i].relocs*4,strings=root+8;
  if(strings>=size || MeleePc_DatBe32(b+root)!=profiles[i].root)return -1;
  uint32_t name=MeleePc_DatBe32(b+root+4);
  if(name>=size-strings || !memchr(b+strings+name,0,size-strings-name) || strcmp((const char*)b+strings+name,profiles[i].symbol))return -1;
  unsigned char* scratch=malloc(size);if(!scratch)return -1;memcpy(scratch,b,size);
  int result=MeleePc_FigaTreeBody(scratch,size,profiles[i].body,profiles[i].relocs,profiles[i].root);free(scratch);if(result)return result;
 }
 return 0;
}
#endif
