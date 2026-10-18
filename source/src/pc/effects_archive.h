#ifndef MELEE_PC_EFFECTS_ARCHIVE_H
#define MELEE_PC_EFFECTS_ARCHIVE_H
#include "particle_bank.h"
/* EF_EffectDesc is f32 lifetime + four StaticModelDesc pointers. The
 * allocation boundary is checked rather than assuming a universal count.
 * Rooted records require typed owners. Disconnected unused relocation data
 * can retain its opaque bytes: DAT metadata itself types its pointer words. */
static int effects_rooted_record(MeleePc_TitlePlan* p,uint32_t at,uint32_t size){
 for(size_t i=0;i<p->count;i++){
  uint32_t start=p->nodes[i].at,end=start+title_types[p->nodes[i].type].size;
  if(at<end && start<at+size)return 1;
 }
 return 0;
}
static int effects_unrooted_relocations(MeleePc_TitlePlan* p){
 /* This is restricted to one public effects root and no externals by the
  * caller. The typed walk has exhausted every rooted pointer field. All
  * covered targets must belong to claimed data, and every leftover pointer
  * slot AND target must be outside rooted records and claimed payloads.
  * Consequently no leftover relocation can be reached through those typed
  * fields. No scalar layout is guessed for the unused allocations. */
 for(uint32_t i=0;i<(p->size+3)/4;i++)if(p->relocs[i]){
  uint32_t slot=i*4,target=title_u32(p,slot);
  if(target>=p->size)return -1;
  if(p->relocs[i]==2){if(!p->claims[target])return -2;continue;}
  if((target&3) || effects_rooted_record(p,slot,4) || effects_rooted_record(p,target,1))return -2;
  for(unsigned j=0;j<4;j++)if(p->claims[slot+j])return -2;
  if(p->claims[target])return -2;
 }
 /* Validate the complete set before adding any claims. Targets can point
  * to other unused pointer words; their subsequent claims are intentional. */
 for(uint32_t i=0;i<(p->size+3)/4;i++)if(p->relocs[i]==1){
  title_claim(p,i*4,4,4);p->relocs[i]=2;
 }
 return p->error;
}
static int MeleePc_EffectsBody(unsigned char* bytes,uint32_t body,uint32_t relocs,uint32_t root,uint32_t allowed_bank_pad,uint32_t external_palette_slot){
 if(!body || body>64*1024*1024 || relocs>body/4 || (root&3))return -1;
 MeleePc_TitlePlan p={0};p.bytes=bytes;p.size=body;p.capacity=(size_t)relocs+body/24+2;
 p.claims=calloc(body,1);p.relocs=calloc((body+3)/4,1);p.seen=calloc((body+3)/4,1);
 p.nodes=malloc(p.capacity*sizeof(*p.nodes));
 if(!p.claims || !p.relocs || !p.seen || !p.nodes)p.error=-1;
 uint32_t boundary=body;
 if(!p.error)title_range(&p,root,8);
 for(uint32_t i=0;i<relocs && !p.error;i++){
  uint32_t slot=MeleePc_DatBe32(bytes+32+body+i*4);
  if((slot&3) || !title_range(&p,slot,4) || p.relocs[slot/4]){p.error=-1;break;}
  p.relocs[slot/4]=1;uint32_t target=title_u32(&p,slot);
  if(target>body){p.error=-1;break;}
  if(target>root+8 && target<boundary)boundary=target;
 }
 uint32_t cmd=UINT32_MAX,tex=UINT32_MAX,descriptor_end=UINT32_MAX;
 if(!p.error){cmd=title_pointer(&p,root);tex=title_pointer(&p,root+4);}
#ifdef MELEE_PC_TITLE_TRACE
 if(!p.error)fprintf(stderr,"EFFECTS_ROOT root=%u boundary=%u cmd=%u tex=%u span=%u pad=%u relocs=%u\n",root,boundary,cmd,tex,boundary>=root+8?boundary-root-8:0,allowed_bank_pad,relocs);
#endif
 if(!p.error){
  if(boundary==body || boundary<root+8)p.error=-2;
  else if((boundary-root-8)%20){
   uint32_t span=boundary-root-8;
   /* Some exact retail effect roots put zero-filled alignment after their
    * typed 20-byte descriptors. It precedes a particle bank when one exists,
    * or the first referenced payload for the exact null-bank Kirby copies.
    * The caller may authorize only the observed padding length for an exact
    * root/envelope profile. */
   if(allowed_bank_pad){
    if(span<=allowed_bank_pad || (span-allowed_bank_pad)%20 ||
       ((cmd!=UINT32_MAX || tex!=UINT32_MAX) && cmd!=boundary) ||
       (boundary&7))p.error=-2;
    else{
     descriptor_end=boundary-allowed_bank_pad;
     for(uint32_t at=descriptor_end;at<boundary;at++)
      if(bytes[32+at]){p.error=-2;break;}
     if(!p.error)title_claim(&p,descriptor_end,allowed_bank_pad,1);
    }
   }
   else{
    /* Particle-only archives have just the bank pointer relocations and a
     * zero-filled area before their first bank. No model scalar schema is
     * required for all-zero unused descriptors/alignment bytes. */
    if(relocs!=2 || cmd==UINT32_MAX || tex==UINT32_MAX || boundary!=cmd)p.error=-2;
    else for(uint32_t at=root+8;at<boundary;at++)if(bytes[32+at]){p.error=-2;break;}
    if(!p.error)title_claim(&p,root+8,boundary-root-8,1);
   }
  }
  else{
   descriptor_end=boundary;
  }
  if(!p.error && descriptor_end!=UINT32_MAX){
   static const unsigned types[4]={TITLE_HSD_Joint,TITLE_HSD_AnimJoint,TITLE_HSD_MatAnimJoint,TITLE_HSD_ShapeAnimJoint};
   for(uint32_t at=root+8;at<descriptor_end && !p.error;at+=20){
    title_claim(&p,at,4,4);uint32_t bits=title_u32(&p,at);float lifetime;memcpy(&lifetime,&bits,4);
    if(!isfinite(lifetime)){p.error=-1;break;}
    for(unsigned j=0;j<4 && !p.error;j++){uint32_t target=title_pointer(&p,at+4+j*4);
     if(target!=UINT32_MAX)title_enqueue(&p,target,types[j]);}
   }
  }
 }
#ifdef MELEE_PC_TITLE_TRACE
 fprintf(stderr,"EFFECTS_DESCRIPTORS end=%u count=%zu error=%d\n",descriptor_end,p.count,p.error);
#endif
 /* efAsync_OnLoad and efAsync_LoadSync call the particle-bank routines only
  * when cmd_bank OR tex_bank is non-null. Several Kirby-copy effect tables
  * have both root fields as literal, non-relocated zero, while retaining
  * typed EF_EffectDesc records; skip the bank walk for that exact null case. */
 if(cmd!=UINT32_MAX || tex!=UINT32_MAX)MeleePc_ParticleBanksPlanExternal(&p,cmd,tex,external_palette_slot);
 while(p.head<p.count && !p.error){MeleePc_TitleNode node=p.nodes[p.head++];title_walk(&p,node);}
 uint32_t unresolved=0;for(uint32_t i=0;i<(body+3)/4 && !p.error;i++)if(p.relocs[i]==1)unresolved++;
 if(unresolved)p.error=effects_unrooted_relocations(&p);
#ifdef MELEE_PC_TITLE_TRACE
 fprintf(stderr,"EFFECTS_PLAN error=%d typed_nodes=%zu unresolved_relocations=%u\n",p.error,p.head,unresolved);
#endif
 if(!p.error)for(uint32_t at=0;at<body;){unsigned width=p.claims[at];
  if(width==2){uint16_t v=title_u16(&p,at);memcpy(bytes+32+at,&v,2);at+=2;}
  else if(width==4){if(at==external_palette_slot)memset(bytes+32+at,0,4);else MeleePc_DatSwap32(bytes+32+at);at+=4;}else ++at;
 }
 free(p.claims);free(p.relocs);free(p.seen);free(p.nodes);return p.error;
}
#endif
