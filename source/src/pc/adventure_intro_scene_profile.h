#ifndef MELEE_PC_ADVENTURE_INTRO_SCENE_PROFILE_H
#define MELEE_PC_ADVENTURE_INTRO_SCENE_PROFILE_H
/* gm_1879.c loads one SceneDesc plus mc01..mc12 SceneCameraDesc records.
 * These are camera descriptors/animation lists, not nested motion archives.
 * Admit only the exact source-audited original regional envelopes/roots. */
static int MeleePc_IsAdventureIntroProfile(const unsigned char* bytes,uint32_t body,
 uint32_t nr,uint32_t np,size_t roots,size_t strings){
 if(np!=13 || MeleePc_DatBe32(bytes+16))return 0;
 unsigned variant;
 if(MeleePc_DatBe32(bytes)==699611 && body==691624 && nr==1942)variant=0;
 else if(MeleePc_DatBe32(bytes)==777499 && body==769512 && nr==1942)variant=1;
 else return 0;
 static const char* names[]={"ScItrNormal_scene_data","mc01","mc02","mc03","mc04","mc05","mc06","mc07","mc08","mc09","mc10","mc11","mc12"};
 static const uint32_t offsets[2][13]={
  {662740,664988,667260,669600,671764,674016,677136,679316,681784,684108,686888,688916,691616},
  {740628,742876,745148,747488,749652,751904,755024,757204,759672,761996,764776,766804,769504},
 };
 for(unsigned i=0;i<13;i++){
  uint32_t s=MeleePc_DatBe32(bytes+roots+i*8+4);
  if(MeleePc_DatBe32(bytes+roots+i*8)!=offsets[variant][i] || strcmp((const char*)bytes+strings+s,names[i]))return 0;
 }
 return 1;
}
#endif
