#ifndef MELEE_PC_STAGE_FLATZONE_ARCHIVE_PLAN_H
#define MELEE_PC_STAGE_FLATZONE_ARCHIVE_PLAN_H
#include <stdio.h>
#include "stage_archive_plan.h"
#include "stage_flatzone_parameters.h"
#include "item_article_plan.h"
#include "item_tools_attributes.h"
static void MeleePc_StageFlatzoneArticleProbe(MeleePc_TitlePlan* p,uint32_t root,unsigned kind,uint32_t state_count){
 if(p->error||(root&3)||!title_range(p,root,24)){p->error=-1;return;}
 uint32_t targets[6];
 for(unsigned i=0;i<6 && !p->error;i++)targets[i]=item_dynamics_pointer(p,root+i*4);
 if(p->error){fprintf(stderr,"FLATZONE_ARTICLE phase=pointers root=%u kind=%u error=%d\n",root,kind,p->error);return;}
 if(targets[0]!=UINT32_MAX)MeleePc_ItemArticleAttributesPlan(p,targets[0]);
 if(p->error){fprintf(stderr,"FLATZONE_ARTICLE phase=common root=%u kind=%u error=%d\n",root,kind,p->error);return;}
 if(targets[1]!=UINT32_MAX)MeleePc_ItemToolsAttributesPlan(p,targets[1]);
 if(p->error){fprintf(stderr,"FLATZONE_ARTICLE phase=special root=%u kind=%u error=%d\n",root,kind,p->error);return;}
 if(targets[2]!=UINT32_MAX)MeleePc_ItemHurtbonesPlan(p,targets[2]);
 if(p->error){fprintf(stderr,"FLATZONE_ARTICLE phase=hurtbones root=%u kind=%u error=%d\n",root,kind,p->error);return;}
 if(targets[3]==UINT32_MAX && state_count){p->error=-1;return;}
 if(targets[3]!=UINT32_MAX){
  MeleePc_ItemStateAnimationsPlan(p,targets[3],state_count);
  if(p->error)return;
  for(uint32_t i=0;i<state_count && !p->error;i++){
   uint32_t command=title_pointer(p,targets[3]+i*16+12);
   MeleePc_ItemCommandPlan(p,&command,1);
  }
 }
 if(targets[4]!=UINT32_MAX)MeleePc_ItemModelDescriptorPlan(p,targets[4]);
 if(targets[5]!=UINT32_MAX)MeleePc_ItemDynamicsPlan(p,targets[5]);
}
/* Source Ground item list and literal ItemStateTable IDs require prefixes
 * ten for Tools IDs0..9; motion attributes are indexed0..4. No unknown article payload bypass. */
static void MeleePc_StageFlatzoneItemsPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(root&3){p->error=-1;return;}
 for(uint32_t i=0;i<p->size/4 && !p->error;i++){
  uint32_t slot=root+i*4;
  if(!title_range(p,slot,4))return;
  if(p->external_slots && p->external_slots[slot/4]){p->error=-2;return;}
  uint32_t rec=title_pointer(p,slot);if(p->error)return;
  if(rec==UINT32_MAX)return;
  if((rec&3) || !title_range(p,rec,8)){p->error=-1;return;}
  item_dynamics_scalars(p,rec,4,4);
  uint32_t kind=title_u32(p,rec),article=item_dynamics_pointer(p,rec+4);
  unsigned states=kind==230?10:0;
  if(!states || article==UINT32_MAX){p->error=-2;return;}
  MeleePc_StageFlatzoneArticleProbe(p,article,kind,states);
#ifdef MELEE_PC_TITLE_TRACE
  if(p->error)fprintf(stderr,"FLATZONE_ITEM kind=%u required_states=%u error=%d\n",kind,states,p->error);
#endif
 }
 if(!p->error)p->error=-1;
}
static void MeleePc_StageFlatzonePlan(MeleePc_TitlePlan* p,size_t length,
 uint32_t np,size_t public,size_t strings,uint32_t* lighting_out){
 static const char* required[]={"map_head","coll_data","grGroundParam","yakumono_param","ALDYakuAll","itemdata","map_plit","quake_model_set","GrdFlatzoneAclmark_IA4_image"};
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
   /* Keep default auxiliary schema until source establishes other types. */
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
  else if(!strcmp(name,"itemdata"))MeleePc_StageFlatzoneItemsPlan(p,at);
  else if(!strcmp(name,"coll_data"))MeleePc_StageCollisionPlan(p,at);
  else if(!strcmp(name,"grGroundParam"))MeleePc_StageParametersPlan(p,at);
  else if(!strcmp(name,"yakumono_param"))MeleePc_StageFlatzoneParametersPlan(p,at);
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
  if(p->error)fprintf(stderr,"FLATZONE_ROOT name=%s at=%u error=%d\n",name,at,p->error);
#endif
 }
 /* Audited Flat Zone archive has no particle-bank roots. */
 while(p->head<p->count && !p->error){MeleePc_TitleNode node=p->nodes[p->head++];title_walk(p,node);if(p->error)fprintf(stderr,"FLATZONE_NODE at=%u error=%d\n",node.at,p->error);}
 if(!p->error && roots_seen!=(1U<<(sizeof(required)/sizeof(required[0])))-1)p->error=-2;
 for(uint32_t i=0;i<np && !p->error;i++){
  uint32_t at=MeleePc_DatBe32(p->bytes+public+i*8);
  if(at>=p->size || !p->claims[at]){fprintf(stderr,"FLATZONE_UNCLAIMED_PUBLIC at=%u\n",at);p->error=-2;}
 }
 if(!p->error)*lighting_out=lighting;
}
static int MeleePc_StageFlatzoneBody(unsigned char* bytes,size_t length,uint32_t body,
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
 if(!p.error)MeleePc_StageFlatzonePlan(&p,length,publics,roots,strings,&lighting);
 if(!p.error){for(uint32_t i=0;i<(p.size+3)/4;i++)if(p.relocs[i]==2){uint32_t target=title_u32(&p,i*4);if(target<p.size && !p.claims[target])fprintf(stderr,"FLATZONE_UNCLAIMED_TARGET slot=%u target=%u deferred=%u\n",i*4,target,!(target&3)&&p.relocs[target/4]==1);}p.error=MeleePc_StageDisconnectedPointersPlan(&p);if(p.error)fprintf(stderr,"FLATZONE_DISCONNECTED error=%d\n",p.error);}
 for(uint32_t i=0;i<publics && !p.error;i++){
  uint32_t at=MeleePc_DatBe32(bytes+roots+i*8),s=MeleePc_DatBe32(bytes+roots+i*8+4);
  if(!strcmp((char*)bytes+strings+s,"map_head"))head=at;
 }
 /* Every external chain link must remain a validated native word for
  * ArchiveLocateExtern; it is never relocated as a local object pointer. */
 for(uint32_t at=0;at+4<=body && !p.error;at+=4)if(external[at/4])
  for(unsigned j=0;j<4;j++)if(p.claims[at+j]!=4)p.error=-2;
 if(!p.error){p.error=MeleePc_StageCommit(&p,head,lighting);if(p.error)fprintf(stderr,"FLATZONE_COMMIT error=%d\n",p.error);}
 free(external);free(p.claims);free(p.relocs);free(p.seen);free(p.nodes);return p.error;
}
#endif
