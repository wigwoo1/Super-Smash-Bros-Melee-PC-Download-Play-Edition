#ifndef MELEE_PC_STAGE_KINOKO_ROUTE_ARCHIVE_PLAN_H
#define MELEE_PC_STAGE_KINOKO_ROUTE_ARCHIVE_PLAN_H
#include "stage_archive_plan.h"

#include "item_article_plan.h"
/* grkinokoroute.c: one s32 plus the first 80 packed spawn descriptors
 * consumed by grZakoGenerator_801CA67C/801CA8B4. No type is inferred for
 * unused trailing parameter bytes. Four stage articles use source state
 * maxima Nokonoko=5, Patapata=4, ZGShell=3 and ZRShell=4. */
static void MeleePc_StageKinokoRouteParametersPlan(MeleePc_TitlePlan* p,uint32_t at){
 if(p->error || (at&3) || !title_range(p,at,4+80*4)){p->error=-1;return;}
 item_dynamics_scalars(p,at,4,4);
 for(uint32_t i=0;i<80 && !p->error;i++){
  uint32_t rec=at+4+i*4;
  if(p->relocs[rec/4] || (p->external_slots && p->external_slots[rec/4])){p->error=-2;return;}
  uint32_t kind=title_u16(p,rec);
  if((kind!=0 && kind!=43 && kind!=211 && kind!=212) || p->bytes[32+rec+3]>1){p->error=-2;return;}
  title_claim(p,rec,2,2);title_claim(p,rec+2,2,1);
 }
}
static void MeleePc_StageKinokoRouteItemsPlan(MeleePc_TitlePlan* p,uint32_t root){
 static const uint32_t kinds[4]={211,212,219,220},counts[4]={6,5,4,5};
 if(p->error || (root&3) || !title_range(p,root,20)){p->error=-1;return;}
 for(unsigned i=0;i<4 && !p->error;i++){
  uint32_t rec=item_dynamics_pointer(p,root+i*4);
  if(rec==UINT32_MAX || (rec&3) || !title_range(p,rec,8)){p->error=-1;return;}
  item_dynamics_scalars(p,rec,4,4);
  if(title_u32(p,rec)!=kinds[i]){p->error=-2;return;}
  uint32_t article=item_dynamics_pointer(p,rec+4);
  if(article==UINT32_MAX){p->error=-1;return;}
  MeleePc_ItemArticlePlan(p,article,kinds[i],counts[i]);
 }
 if(!p->error && item_dynamics_pointer(p,root+16)!=UINT32_MAX)p->error=-2;
}
static void MeleePc_StageKinokoRoutePlan(MeleePc_TitlePlan* p,size_t length,
 uint32_t np,size_t public,size_t strings,uint32_t* lighting_out){
 static const char* required[]={"map_head","coll_data","grGroundParam","yakumono_param","ALDYakuAll","itemdata","map_ptcl","map_texg","map_plit","quake_model_set","GrdKinokoRSky1_CMPR_image"};
 unsigned roots_seen=0;
 if(public>length || strings>length || np>(length-public)/8){p->error=-1;return;}
 uint32_t cmd=UINT32_MAX,tex=UINT32_MAX,lighting=UINT32_MAX;
 for(uint32_t i=0;i<np && !p->error;i++){
  uint32_t at=MeleePc_DatBe32(p->bytes+public+i*8),s=MeleePc_DatBe32(p->bytes+public+i*8+4);
  if(s>=length-strings || !memchr(p->bytes+strings+s,0,length-strings-s)){p->error=-1;break;}char* name=(char*)p->bytes+strings+s;
  for(unsigned r=0;r<sizeof(required)/sizeof(required[0]);r++)if(!strcmp(name,required[r])){
   if(roots_seen&(1U<<r)){p->error=-1;return;}roots_seen|=1U<<r;
  }
  if(!strcmp(name,"map_head")){
   MeleePc_StageJointPairsPlan(p,at);MeleePc_StageModelsPlan(p,at);MeleePc_StageLightingPlan(p,at,&lighting);
   uint32_t splines=title_pointer(p,at+16),ns=title_u32(p,at+20);title_claim(p,at+20,4,4);
   if(ns>p->size/4)p->error=-1;else title_array(p,splines,TITLE_HSD_Spline,ns,0);
   uint32_t shadows=title_pointer(p,at+32),nc=title_u32(p,at+36);title_claim(p,at+36,4,4);
   if(nc>p->size/8 || (nc && shadows==UINT32_MAX))p->error=-1;
   else if(nc && title_range(p,shadows,nc*8))for(uint32_t j=0;j<nc && !p->error;j++){
    title_enqueue(p,title_pointer(p,shadows+j*8),TITLE_HSD_LightAnim);title_claim(p,shadows+j*8+4,4,1);
   }
   uint32_t mats=title_pointer(p,at+40),nm=title_u32(p,at+44);title_claim(p,at+44,4,4);
   if(nm>p->size/4)p->error=-1;else title_array(p,mats,TITLE__HSD_MObjDesc,nm,0);
  }else if(!strcmp(name,"ALDYakuAll"))MeleePc_StageItemScriptsPlan(p,at);
  else if(!strcmp(name,"itemdata"))MeleePc_StageKinokoRouteItemsPlan(p,at);
  else if(!strcmp(name,"coll_data"))MeleePc_StageCollisionPlan(p,at);
  else if(!strcmp(name,"grGroundParam"))MeleePc_StageParametersPlan(p,at);
  else if(!strcmp(name,"yakumono_param"))MeleePc_StageKinokoRouteParametersPlan(p,at);
  else if(!strcmp(name,"map_ptcl"))cmd=at;
  else if(!strcmp(name,"map_texg"))tex=at;
  else if(!strcmp(name,"map_plit"))stage_model_list(p,at,TITLE_LightList);
  else if(!strcmp(name,"quake_model_set")){
   if(title_range(p,at,16)){
    title_enqueue(p,title_pointer(p,at),TITLE_HSD_Joint);
    stage_model_list(p,title_pointer(p,at+4),TITLE_HSD_AnimJoint);
    stage_model_list(p,title_pointer(p,at+8),TITLE_HSD_MatAnimJoint);
    stage_model_list(p,title_pointer(p,at+12),TITLE_HSD_ShapeAnimJoint);
   }
  }else{
   size_t len=strlen(name);
   if(len>=6 && !strcmp(name+len-6,"_joint"))title_enqueue(p,at,TITLE_HSD_Joint);
   else if(len>=10 && !strcmp(name+len-10,"_tlut_desc"))title_enqueue(p,at,TITLE__HSD_TlutDesc);
   else if(len>=11 && !strcmp(name+len-11,"_image_desc"))title_enqueue(p,at,TITLE_HSD_ImageDesc);
   else if((len>=6 && !strcmp(name+len-6,"_image")) || (len>=5 && !strcmp(name+len-5,"_tlut"))){/* Byte roots must be claimed by a validated descriptor below. */}
   else p->error=-2;
  }
#ifdef MELEE_PC_TITLE_TRACE
  if(p->error)fprintf(stderr,"KINOKO_ROUTE_ROOT name=%s at=%u error=%d\n",name,at,p->error);
#endif
 }
 MeleePc_ParticleBanksPlan(p,cmd,tex);
 while(p->head<p->count && !p->error){MeleePc_TitleNode node=p->nodes[p->head++];title_walk(p,node);}
 if(!p->error && roots_seen!=(1U<<(sizeof(required)/sizeof(required[0])))-1)p->error=-2;
 for(uint32_t i=0;i<np && !p->error;i++){
  uint32_t at=MeleePc_DatBe32(p->bytes+public+i*8);
  if(at>=p->size || !p->claims[at])p->error=-2;
 }
 if(!p->error)*lighting_out=lighting;
}
static int MeleePc_StageKinokoRouteBody(unsigned char* bytes,size_t length,uint32_t body,
 uint32_t relocs,uint32_t publics,size_t roots,size_t strings){
 if(!body || body>64*1024*1024 || relocs>body/4)return -1;
 MeleePc_TitlePlan p={0};p.bytes=bytes;p.size=body;p.capacity=(size_t)relocs+body/24+2;
 p.claims=calloc(body,1);p.relocs=calloc((body+3)/4,1);p.seen=calloc((body+3)/4,1);
 p.nodes=malloc(p.capacity*sizeof(*p.nodes));
 if(!p.claims || !p.relocs || !p.seen || !p.nodes)p.error=-1;
 for(uint32_t i=0;i<relocs && !p.error;i++){
  uint32_t at=MeleePc_DatBe32(bytes+32+body+i*4);
  if((at&3) || !title_range(&p,at,4) || p.relocs[at/4]){p.error=-1;break;}
  p.relocs[at/4]=1;
 }
 unsigned char* external=calloc((body+3)/4,1);uint32_t external_count=0;
 if(!external)p.error=-1;
 if(!p.error)p.error=MeleePc_ArchiveExternalPlan(bytes,length,external,&external_count);
 p.external_slots=external;
 uint32_t lighting=UINT32_MAX,head=UINT32_MAX;
 if(!p.error)MeleePc_StageKinokoRoutePlan(&p,length,publics,roots,strings,&lighting);
 if(!p.error)p.error=MeleePc_StageDisconnectedPointersPlan(&p);
 for(uint32_t i=0;i<publics && !p.error;i++){
  uint32_t at=MeleePc_DatBe32(bytes+roots+i*8),s=MeleePc_DatBe32(bytes+roots+i*8+4);
  if(!strcmp((char*)bytes+strings+s,"map_head"))head=at;
 }
 /* Every external chain link must remain a validated native word for
  * ArchiveLocateExtern; it is never relocated as a local object pointer. */
 for(uint32_t at=0;at+4<=body && !p.error;at+=4)if(external[at/4])
  for(unsigned j=0;j<4;j++)if(p.claims[at+j]!=4)p.error=-2;
 if(!p.error)p.error=MeleePc_StageCommit(&p,head,lighting);
 free(external);free(p.claims);free(p.relocs);free(p.seen);free(p.nodes);return p.error;
}
static int MeleePc_IsKinokoRouteProfile(const unsigned char* bytes,size_t length,
 uint32_t body,uint32_t nr,uint32_t np,size_t roots,size_t strings){
 if(length!=1350290 || body!=1313216 || nr!=8164 || np!=133 || MeleePc_DatBe32(bytes+16))return 0;
 static const char* names[]={
  "ALDYakuAll",
  "GrdCastleWater1_C8_image",
  "GrdCastleWater1_tlut",
  "GrdCastleWater1_tlut_desc",
  "GrdDonkeyFloor1_C8_image",
  "GrdDonkeyFloor1_tlut",
  "GrdDonkeyFloor1_tlut_desc",
  "GrdDonkeywood1_C8_image",
  "GrdDonkeywood1_tlut",
  "GrdDonkeywood1_tlut_desc",
  "GrdDonkeywood2_CMPR_image",
  "GrdKinokoBlock1_C8_image",
  "GrdKinokoBlock1_tlut",
  "GrdKinokoBlock1_tlut_desc",
  "GrdKinokoRB_CMPR_image",
  "GrdKinokoRCastrk3_C8_image",
  "GrdKinokoRCastrk3_CMPR_image",
  "GrdKinokoRCastrk3_tlut",
  "GrdKinokoRCastrk3_tlut_desc",
  "GrdKinokoRCruseIsiA_C8_image",
  "GrdKinokoRCruseIsiA_tlut",
  "GrdKinokoRCruseIsiA_tlut_desc",
  "GrdKinokoRF1_C8_image",
  "GrdKinokoRF1_tlut",
  "GrdKinokoRF1_tlut_desc",
  "GrdKinokoRF2_C8_image",
  "GrdKinokoRF2_tlut",
  "GrdKinokoRF2_tlut_desc",
  "GrdKinokoRFlag_C4_image",
  "GrdKinokoRFlag_tlut",
  "GrdKinokoRFlag_tlut_desc",
  "GrdKinokoRG_C8_image",
  "GrdKinokoRG_tlut",
  "GrdKinokoRG_tlut_desc",
  "GrdKinokoRGdngrs2_CMPR_image",
  "GrdKinokoRGdngrs3_CMPR_image",
  "GrdKinokoRGdngrs4_C8_image",
  "GrdKinokoRGdngrs4_tlut",
  "GrdKinokoRGdngrs4_tlut_desc",
  "GrdKinokoRGdngrs_CMPR_image",
  "GrdKinokoRGdnrail1_C8_image",
  "GrdKinokoRGdnrail1_tlut",
  "GrdKinokoRGdnrail1_tlut_desc",
  "GrdKinokoRKinoShadow_IA4_image",
  "GrdKinokoRM1_C8_image",
  "GrdKinokoRM1_tlut",
  "GrdKinokoRM1_tlut_desc",
  "GrdKinokoRMark1_C8_image",
  "GrdKinokoRMark1_tlut",
  "GrdKinokoRMark1_tlut_desc",
  "GrdKinokoRSkl_RGBA8_image",
  "GrdKinokoRSky1_CMPR_image",
  "GrdKinokoRSky1b_CMPR_image",
  "GrdKinokoRSky1c_CMPR_image",
  "GrdKinokoRSky2_CMPR_image",
  "GrdKinokoRWood3_C8_image",
  "GrdKinokoRWood3_tlut",
  "GrdKinokoRWood3_tlut_desc",
  "GrdKinokoRWood4_C8_image",
  "GrdKinokoRWood4_tlut",
  "GrdKinokoRWood4_tlut_desc",
  "GrdKinokoRm_CMPR_image",
  "GrdKinokoRouteDokanAna_C4_image",
  "GrdKinokoRouteDokanAna_tlut",
  "GrdKinokoRouteDokanAna_tlut_desc",
  "GrdKinokoRouteDokan_C4_image",
  "GrdKinokoRouteDokan_tlut",
  "GrdKinokoRouteDokan_tlut_desc",
  "GrdKinokoRouteDokansoku_C8_image",
  "GrdKinokoRouteDokansoku_tlut",
  "GrdKinokoRouteDokansoku_tlut_desc",
  "GrdKinokoRouteIshiA_CMPR_image",
  "GrdKinokoRouteMt1_CMPR_image",
  "GrdKinokoRouteMt2_C8_image",
  "GrdKinokoRouteMt2_tlut",
  "GrdKinokoRouteMt2_tlut_desc",
  "GrdKinokoRouteMt3_CMPR_image",
  "GrdKinokoRouteWood2_C8_image",
  "GrdKinokoRouteWood2_tlut",
  "GrdKinokoRouteWood2_tlut_desc",
  "GrdKinokoRouteWoodB_C8_image",
  "GrdKinokoRouteWoodB_tlut",
  "GrdKinokoRouteWoodB_tlut_desc",
  "GrdKinokoRouteWood_C8_image",
  "GrdKinokoRouteWood_tlut",
  "GrdKinokoRouteWood_tlut_desc",
  "GrdPeachCastclm7_C8_image",
  "GrdPeachCastclm7_tlut",
  "GrdPeachCastclm7_tlut_desc",
  "GrdPeachCastent1_C8_image",
  "GrdPeachCastent1_tlut",
  "GrdPeachCastent1_tlut_desc",
  "GrdPeachCastrk1_C8_image",
  "GrdPeachCastrk1_tlut",
  "GrdPeachCastrk1_tlut_desc",
  "GrdPeachCastrk2_C8_image",
  "GrdPeachCastrk2_tlut",
  "GrdPeachCastrk2_tlut_desc",
  "GrdPeachGdngrs2_CMPR_image",
  "GrdPeachGdngrs_CMPR_image",
  "GrdPeachGdntr5_C8_image",
  "GrdPeachGdntr5_tlut",
  "GrdPeachGdntr5_tlut_desc",
  "GrdPeachGdntr6_C8_image",
  "GrdPeachGdntr6_tlut",
  "GrdPeachGdntr6_tlut_desc",
  "GrdRainbowCruseIsiA_C8_image",
  "GrdRainbowCruseIsiA_tlut",
  "GrdRainbowCruseIsiA_tlut_desc",
  "GrdRainbowCruseYukaSideD_C8_image",
  "GrdRainbowCruseYukaSideD_tlut",
  "GrdRainbowCruseYukaSideD_tlut_desc",
  "GrdRainbowCruseYukaSideE_C8_image",
  "GrdRainbowCruseYukaSideE_tlut",
  "GrdRainbowCruseYukaSideE_tlut_desc",
  "ItmPeachKinopioBody_CMPR_image",
  "ItmPeachKinopioFace1_CMPR_image",
  "ItmPeachKinopioFace_CMPR_image",
  "ItmPeachKinopioFoot_CMPR_image",
  "ItmPeachKinopioHead0_CMPR_image",
  "ItmPeachKinopioSA_I4_image",
  "ItmPeachKinopioSkin_CMPR_image",
  "ItmPeachKinopioSuit_CMPR_image",
  "coll_data",
  "ctest1_RGBA8_image",
  "grGroundParam",
  "itemdata",
  "map_head",
  "map_plit",
  "map_ptcl",
  "map_texg",
  "quake_model_set",
  "yakumono_param",
 };
 static const uint32_t offsets[]={332564,473344,489728,490208,507168,511264,511776,515904,520000,520512,511808,470752,471776,472288,753920,629152,962432,645536,646048,654528,670912,671424,845248,861632,862144,862176,878560,879072,928448,928512,928544,1024960,1057728,1058240,554400,587712,595904,628672,629120,579520,909472,925856,926368,472320,970624,987008,987520,671456,736992,737504,888448,1123808,1091040,1058272,1156576,520544,536928,537440,537472,553856,554368,987552,802080,802592,802624,799712,801760,802048,798560,799584,799680,802656,1003936,1012128,1016224,1016736,1016768,828320,844704,845216,810848,819040,819552,490240,506624,507136,819584,827776,828288,892544,908928,909440,781632,798016,798528,562592,578976,579488,745728,737536,945504,961888,962400,928576,944960,945472,646080,654272,654496,755968,772352,772864,772896,781088,781600,879744,926400,880768,887936,882816,879616,886912,879104,297024,1181152,331908,332128,1132,465932,297088,297824,1313192,332148};
 for(unsigned i=0;i<133;i++){
  uint32_t s=MeleePc_DatBe32(bytes+roots+i*8+4);
  if(s>=length-strings || !memchr(bytes+strings+s,0,length-strings-s) ||
     MeleePc_DatBe32(bytes+roots+i*8)!=offsets[i] || strcmp((const char*)bytes+strings+s,names[i]))return 0;
 }
 return 1;
}
#endif
