#ifndef MELEE_PC_STAGE_KONGO_ARCHIVE_PLAN_H
#define MELEE_PC_STAGE_KONGO_ARCHIVE_PLAN_H
#include "stage_archive_plan.h"
#include "stage_kongo_parameters.h"
#include "stage_material_commands.h"
#include "stage_kongo_model_wrappers.h"
#include "stage_kongo_aux_scenes.h"
#include "stage_kongo_opaque_klap.h"
#include "item_article_plan.h"
/* Ground installs a NULL-terminated list of {s32 kind, Article*} records.
 * Klaptrap source animation IDs [0,1,2,3,2] require four descriptors. */
static void MeleePc_StageKongoItemsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(root&3){p->error=-1;return;}
 for(uint32_t i=0;i<p->size/4 && !p->error;i++){
  uint32_t slot=root+i*4;
  if(!title_range(p,slot,4))return;
  if(p->external_slots && p->external_slots[slot/4]){p->error=-2;return;}
  uint32_t rec=title_pointer(p,slot);
  if(p->error)return;
  if(rec==UINT32_MAX)return;
  if((rec&3) || !title_range(p,rec,8)){p->error=-1;return;}
  item_dynamics_scalars(p,rec,4,4);
  uint32_t kind=title_u32(p,rec),article=item_dynamics_pointer(p,rec+4);
  if(kind!=218 || article==UINT32_MAX){p->error=-2;return;}
  MeleePc_ItemArticlePlan(p,article,kind,4);
 }
 if(!p->error)p->error=-1;
}
/* Pinned original auxiliary DynamicModelDesc, independently verified by
 * audit-kongo-aux-model.py and the full-file native graph oracle. This
 * converts its disconnected allocation without attaching a scene model. */
static void MeleePc_StageKongoAuxModelPlan(MeleePc_TitlePlan* p){
 if(p->error || !title_range(p,302788,16))return;
 static const uint32_t fields[]={277800,302740,302756,302772};
 static const uint32_t entries[][4]={
  {291060,296600,301172,0},{291900,297444,302028,0},
  {292300,297844,302428,0}};
 for(unsigned i=0;i<4;i++){
  uint32_t at=302788+i*4;
  if(!p->relocs[at/4] || (p->external_slots && p->external_slots[at/4]) ||
     title_u32(p,at)!=fields[i]){p->error=-2;return;}
 }
 for(unsigned i=0;i<3;i++){
  uint32_t table=fields[i+1];if(!title_range(p,table,16))return;
  for(unsigned j=0;j<4;j++){
   uint32_t at=table+j*4;
   if(!!p->relocs[at/4]!=(j<3) ||
      (p->external_slots && p->external_slots[at/4]) ||
      title_u32(p,at)!=entries[i][j]){p->error=-2;return;}
  }
 }
 title_enqueue(p,title_pointer(p,302788),TITLE_HSD_Joint);
 stage_model_list(p,title_pointer(p,302792),TITLE_HSD_AnimJoint);
 stage_model_list(p,title_pointer(p,302796),TITLE_HSD_MatAnimJoint);
 stage_model_list(p,title_pointer(p,302800),TITLE_HSD_ShapeAnimJoint);
}
/* Source SceneCameraDesc / SceneFogDesc pinned original wrappers. */
static void MeleePc_StageKongoCameraFogPlan(MeleePc_TitlePlan* p){
 static const uint32_t wrappers[][3]={
  {1156,1100,TITLE_HSD_CameraDescPerspective},
  {229964,229908,TITLE_HSD_CameraDescPerspective},
  {303412,303356,TITLE_HSD_CameraDescPerspective},
  {1292,1272,TITLE_HSD_FogDesc},{230100,230080,TITLE_HSD_FogDesc},
  {303548,303528,TITLE_HSD_FogDesc}};
 for(unsigned i=0;i<6 && !p->error;i++){
  uint32_t at=wrappers[i][0];if(!title_range(p,at,8))return;
  if(!p->relocs[at/4] || p->relocs[(at+4)/4] ||
     (p->external_slots && (p->external_slots[at/4] || p->external_slots[(at+4)/4])) ||
     title_u32(p,at)!=wrappers[i][1] || title_u32(p,at+4)){p->error=-2;return;}
 }
 for(unsigned i=0;i<6 && !p->error;i++){
  uint32_t at=wrappers[i][0];title_enqueue(p,title_pointer(p,at),wrappers[i][2]);
  stage_model_list(p,title_pointer(p,at+4),TITLE_HSD_CameraAnim);
 }
}
static void MeleePc_StageKongoPlan(MeleePc_TitlePlan* p,size_t length,
 uint32_t np,size_t public,size_t strings,uint32_t* lighting_out){
 static const char* required[]={"map_head","coll_data","grGroundParam","yakumono_param","ALDYakuAll","itemdata","map_ptcl","map_texg","map_plit","quake_model_set","GrdDonkeyEda1_CMPR_image"};
 unsigned roots_seen=0;
 if(p->size!=458592){p->error=-2;return;}
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
  else if(!strcmp(name,"itemdata"))MeleePc_StageKongoItemsPlan(p,at);
  else if(!strcmp(name,"coll_data"))MeleePc_StageCollisionPlan(p,at);
  else if(!strcmp(name,"grGroundParam"))MeleePc_StageParametersPlan(p,at);
  else if(!strcmp(name,"yakumono_param")){uint32_t script=UINT32_MAX;MeleePc_StageKongoParametersPlan(p,at,&script);MeleePc_StageMaterialCommandsPlan(p,script);}
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
  if(p->error)fprintf(stderr,"KONGO_ROOT name=%s at=%u error=%d\n",name,at,p->error);
#endif
 }
 MeleePc_StageKongoAuxModelPlan(p);
 MeleePc_StageKongoCameraFogPlan(p);
 MeleePc_StageKongoModelWrappersPlan(p);
 MeleePc_StageKongoAuxScenesPlan(p);
 MeleePc_StageKongoOpaqueKlapPlan(p);
 MeleePc_ParticleBanksPlan(p,cmd,tex);
 while(p->head<p->count && !p->error){MeleePc_TitleNode node=p->nodes[p->head++];title_walk(p,node);}
 if(!p->error && roots_seen!=(1U<<(sizeof(required)/sizeof(required[0])))-1)p->error=-2;
 for(uint32_t i=0;i<np && !p->error;i++){
  uint32_t at=MeleePc_DatBe32(p->bytes+public+i*8);
  if(at>=p->size || !p->claims[at])p->error=-2;
 }
 if(!p->error)*lighting_out=lighting;
}
static int MeleePc_StageKongoBody(unsigned char* bytes,size_t length,uint32_t body,
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
 if(!p.error)MeleePc_StageKongoPlan(&p,length,publics,roots,strings,&lighting);
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
#endif
