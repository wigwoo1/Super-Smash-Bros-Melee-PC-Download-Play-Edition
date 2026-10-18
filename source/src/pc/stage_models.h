#ifndef MELEE_PC_STAGE_MODELS_H
#define MELEE_PC_STAGE_MODELS_H
static uint32_t stage_model_list(MeleePc_TitlePlan* p,uint32_t at,unsigned type){
 if(at==UINT32_MAX)return 0;
 for(uint32_t i=0;i<p->size/4 && !p->error;i++){
  uint32_t target=title_pointer(p,at+i*4);
  if(p->error)return 0;
  if(target==UINT32_MAX)return i;
  title_enqueue(p,target,type);
 }
 if(!p->error)p->error=-1;return 0;
}
/* UnkStageDat_x8_t records from gr/types.h. Unknown x14 may only be
 * accepted as an explicitly empty 12-byte payload; nonempty cases require
 * a schema. Packed per-animation x28 flags remain bytes. */
static void MeleePc_StageModelsPlanWithCameraAnimations(MeleePc_TitlePlan* p,uint32_t root,int camera_animations){
 if(p->error || !title_range(p,root,48))return;
 uint32_t models=title_pointer(p,root+8),count=title_u32(p,root+12);
 title_claim(p,root+12,4,4);
 if(count>p->size/52 || (count && models==UINT32_MAX)){p->error=-1;return;}
 if(count && !title_range(p,models,count*52))return;
 for(uint32_t i=0;i<count && !p->error;i++){
  uint32_t at=models+i*52;title_enqueue(p,title_pointer(p,at),TITLE_HSD_Joint);
  uint32_t choices=0;
  static const unsigned types[]={TITLE_HSD_AnimJoint,TITLE_HSD_MatAnimJoint,TITLE_HSD_ShapeAnimJoint};
  for(unsigned j=0;j<3 && !p->error;j++){
   uint32_t n=stage_model_list(p,title_pointer(p,at+4+j*4),types[j]);if(n>choices)choices=n;
  }
  uint32_t camera=title_pointer(p,at+16);
  if(camera!=UINT32_MAX && !p->error){
   if(!title_range(p,camera,8))return;
   unsigned projection=title_u16(p,camera+6);
   if(projection!=1){p->error=projection==2 || projection==3?-2:-1;return;}
   title_enqueue(p,camera,TITLE_HSD_CameraDescPerspective);
  }
  uint32_t unknown=title_pointer(p,at+20);
  if(unknown!=UINT32_MAX && !p->error){
   if(!title_range(p,unknown,12))return;
   unsigned nonempty=0;
   for(unsigned j=0;j<12;j++)if(p->bytes[32+unknown+j])nonempty=1;
   if(nonempty && camera_animations){
    /* Explicit stage opt-in; validate the complete descriptor graph. */
    title_enqueue(p,unknown,TITLE_HSD_CameraAnim);
   }else{
   for(unsigned j=0;j<12;j++)if(p->bytes[32+unknown+j]){
#ifdef MELEE_PC_TITLE_TRACE
    fprintf(stderr,"STAGE_MODEL_UNTYPED model=%u target=%u byte=%u value=%u\n",i,unknown,j,p->bytes[32+unknown+j]);
#endif
    p->error=-2;return;
   }
   title_claim(p,unknown,12,1);
   }
  }
  stage_model_list(p,title_pointer(p,at+24),TITLE_LightList);
  title_enqueue(p,title_pointer(p,at+28),TITLE_HSD_FogDesc);
  uint32_t joints=title_pointer(p,at+32),nj=title_u32(p,at+36);
  title_claim(p,at+36,4,4);
  if(nj>p->size/6 || (nj && joints==UINT32_MAX)){p->error=-1;return;}
  if(nj)title_claim(p,joints,nj*6,2);
  uint32_t flags=title_pointer(p,at+40);
  /* Imported animation lists can become empty after NULL binding. Keep
   * their flag target bounded byte data even when no choices are consumed. */
  if(flags!=UINT32_MAX)title_claim(p,flags,choices?choices:1,1);
  uint32_t indices=title_pointer(p,at+44),ni=title_u32(p,at+48);
  title_claim(p,at+48,4,4);
  if(ni>p->size/2 || (ni && indices==UINT32_MAX)){p->error=-1;return;}
  if(ni)title_claim(p,indices,ni*2,2);
 }
}
static void MeleePc_StageModelsPlan(MeleePc_TitlePlan* p,uint32_t root){
 MeleePc_StageModelsPlanWithCameraAnimations(p,root,0);
}
#endif
