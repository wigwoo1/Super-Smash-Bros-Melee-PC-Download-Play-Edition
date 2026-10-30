#ifndef MELEE_PC_STAGE_SHRINE_ROUTE_ARCHIVE_PLAN_H
#define MELEE_PC_STAGE_SHRINE_ROUTE_ARCHIVE_PLAN_H
#include "stage_archive_plan.h"
#include "stage_material_commands.h"
#include "item_article_plan.h"
/* grpushon.c: six hazard descriptor links (the x0 link is returned through
 * a legacy s32 callback result), a 32-bit count, thirty {s32,s16,s16}
 * entries and thirty-three {s32,s32} lookup pairs. Collision payloads are
 * lbColl_80008D30_arg1 from melee/lb/forward.h, nine native words each. */
/* grshrineroute.c: four grMaterial streams, x10 legacy pointer to a
 * nine-word collision record, five scalars and packed spawn descriptors.
 * Exact profile contains 64 slots before the ALDY script allocation. */
static void MeleePc_StageShrineRouteParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error || (root&3) || !title_range(p,root,40+64*4)){p->error=-1;return;}
 for(unsigned i=0;i<4;i++)MeleePc_StageMaterialCommandsPlan(p,title_pointer(p,root+i*4));
 uint32_t collision=title_pointer(p,root+16);
 if(collision==UINT32_MAX){p->error=-1;return;}
 item_dynamics_scalars(p,collision,36,4);
 item_dynamics_scalars(p,root+20,20,4);
 for(unsigned i=0;i<64 && !p->error;i++){
  uint32_t at=root+40+i*4,kind=title_u16(p,at);
  if(p->relocs[at/4] || (p->external_slots && p->external_slots[at/4]) ||
     (kind!=0 && kind!=44 && kind!=45 && kind!=213) || p->bytes[32+at+3]>1){p->error=-2;return;}
  title_claim(p,at,2,2);title_claim(p,at+2,2,1);
 }
}
static void MeleePc_StageShrineRouteItemsPlan(MeleePc_TitlePlan* p,uint32_t root){
 uint32_t rec=title_pointer(p,root);
 if(rec==UINT32_MAX || !title_range(p,rec,8)){p->error=-1;return;}
 item_dynamics_scalars(p,rec,4,4);
 if(title_u32(p,rec)!=213){p->error=-2;return;}
 uint32_t article=title_pointer(p,rec+4);
 MeleePc_ItemArticlePlan(p,article,213,10); /* original motion IDs0..9 */
 if(!p->error && title_pointer(p,root+4)!=UINT32_MAX)p->error=-2;
}
static void MeleePc_StageShrineRoutePlan(MeleePc_TitlePlan* p,size_t length,
 uint32_t np,size_t public,size_t strings,uint32_t* lighting_out){
 static const char* required[]={"map_head","coll_data","grGroundParam","yakumono_param","ALDYakuAll","itemdata","map_plit","quake_model_set"};
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
  else if(!strcmp(name,"itemdata"))MeleePc_StageShrineRouteItemsPlan(p,at);
  else if(!strcmp(name,"coll_data"))MeleePc_StageCollisionPlan(p,at);
  else if(!strcmp(name,"grGroundParam"))MeleePc_StageParametersPlan(p,at);
  else if(!strcmp(name,"yakumono_param"))MeleePc_StageShrineRouteParametersPlan(p,at);
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
  if(p->error)fprintf(stderr,"SHRINE_ROUTE_ROOT name=%s at=%u error=%d\n",name,at,p->error);
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
static int MeleePc_StageShrineRouteBody(unsigned char* bytes,size_t length,uint32_t body,
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
 if(!p.error)MeleePc_StageShrineRoutePlan(&p,length,publics,roots,strings,&lighting);
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
static int MeleePc_IsShrineRouteProfile(const unsigned char* bytes,size_t length,uint32_t body,uint32_t nr,uint32_t np,size_t roots,size_t strings){
 if(length!=1655801 || body!=1630496 || nr!=5531 || np!=92 || MeleePc_DatBe32(bytes+16))return 0;
 static const char* names[]={"ALDYakuAll","GrdCastleWater1_C8_image","GrdCastleWater1_tlut","GrdCastleWater1_tlut_desc","GrdKinokoRWood3_CMPR_image","GrdKinokoRouteWoodB_C8_image","GrdKinokoRouteWoodB_tlut","GrdKinokoRouteWoodB_tlut_desc","GrdNSrGrad1_I8_image","GrdNSrVanishEff_IA4_image","GrdNSrWater_RGB5A3_image","GrdShrineKage1_C8_image","GrdShrineKage1_tlut","GrdShrineKage1_tlut_desc","GrdShrineMizu_CMPR_image","GrdShrineRouteDai_RGBA8_image","GrdShrineRouteDoor3_C8_image","GrdShrineRouteDoor3_tlut","GrdShrineRouteDoor3_tlut_desc","GrdShrineRouteDoor_C8_image","GrdShrineRouteDoor_tlut","GrdShrineRouteDoor_tlut_desc","GrdShrineRouteGold_CMPR_image","GrdShrineRouteIseki2_C8_image","GrdShrineRouteIseki2_tlut","GrdShrineRouteIseki2_tlut_desc","GrdShrineRouteIseki_C8_image","GrdShrineRouteIseki_tlut","GrdShrineRouteIseki_tlut_desc","GrdShrineRouteIshi10_CMPR_image","GrdShrineRouteIshi11_C8_image","GrdShrineRouteIshi11_tlut","GrdShrineRouteIshi11_tlut_desc","GrdShrineRouteIshi12_CMPR_image","GrdShrineRouteIshi2_CMPR_image","GrdShrineRouteIshi3_C8_image","GrdShrineRouteIshi3_tlut","GrdShrineRouteIshi3_tlut_desc","GrdShrineRouteIshi4_C8_image","GrdShrineRouteIshi4_tlut","GrdShrineRouteIshi4_tlut_desc","GrdShrineRouteIshi5_CMPR_image","GrdShrineRouteIshi6_C8_image","GrdShrineRouteIshi6_tlut","GrdShrineRouteIshi6_tlut_desc","GrdShrineRouteIshi72_CMPR_image","GrdShrineRouteIshi73_CMPR_image","GrdShrineRouteIshi9_CMPR_image","GrdShrineRouteKabe4_CMPR_image","GrdShrineRouteKabeeA_C8_image","GrdShrineRouteKabeeA_tlut","GrdShrineRouteKabeeA_tlut_desc","GrdShrineRouteKagi_C8_image","GrdShrineRouteKagi_tlut","GrdShrineRouteKagi_tlut_desc","GrdShrineRouteKen0_CMPR_image","GrdShrineRouteKenEdge0_CMPR_image","GrdShrineRouteKusari_C8_image","GrdShrineRouteKusari_tlut","GrdShrineRouteKusari_tlut_desc","GrdShrineRouteMiti2_CMPR_image","GrdShrineRouteMiti_CMPR_image","GrdShrineRouteSaku_C8_image","GrdShrineRouteSaku_tlut","GrdShrineRouteSaku_tlut_desc","GrdShrineRouteStone_CMPR_image","GrdShrineRouteTetu2_C8_image","GrdShrineRouteTetu2_tlut","GrdShrineRouteTetu2_tlut_desc","GrdShrineRouteTetu3_C8_image","GrdShrineRouteTetu3_tlut","GrdShrineRouteTetu3_tlut_desc","GrdShrineRouteTetu4_C8_image","GrdShrineRouteTetu4_tlut","GrdShrineRouteTetu4_tlut_desc","GrdShrineRouteTetu5_C8_image","GrdShrineRouteTetu5_tlut","GrdShrineRouteTetu5_tlut_desc","GrdShrineRouteTetu_C8_image","GrdShrineRouteTetu_tlut","GrdShrineRouteTetu_tlut_desc","GrdShrineRouteTuka0_CMPR_image","GrdVenomRiver1_CMPR_image","coll_data","grGroundParam","itemdata","map_head","map_plit","map_ptcl","map_texg","quake_model_set","yakumono_param"};
 static const uint32_t offsets[]={495040,539840,556224,556704,649984,970528,978720,979232,1567168,1604032,1571264,1416512,1424704,1424736,556736,1609664,1424768,1432960,1433472,1030400,1046784,1047296,1626304,844992,877760,878272,778912,844448,844960,1531808,878304,903904,904416,617216,979264,606432,608480,608992,589504,605888,606400,1383744,904448,969984,970496,1080096,1047328,609024,1433504,1112864,1375008,1375520,775584,778656,778880,1605056,1626048,1564576,1566624,1567136,1499040,1466272,1012032,1030336,1030368,1375552,658176,674560,675072,766848,775040,775552,758112,766304,766816,675104,707872,708384,708416,724800,725312,1605568,725344,453056,494340,494560,1008,534900,453120,455072,1630472,494716};
 for(unsigned i=0;i<92;i++){
  uint32_t sym=MeleePc_DatBe32(bytes+roots+i*8+4);
  if(sym>=length-strings || !memchr(bytes+strings+sym,0,length-strings-sym) || MeleePc_DatBe32(bytes+roots+i*8)!=offsets[i] || strcmp((char*)bytes+strings+sym,names[i]))return 0;
 }
 return 1;
}
#endif

