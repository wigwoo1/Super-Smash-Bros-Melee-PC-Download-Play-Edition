#ifndef MELEE_PC_EFFECTS_DONKEY_PLAN_H
#define MELEE_PC_EFFECTS_DONKEY_PLAN_H
/* Original audited body profile only. Disconnected empty shape records are
 * pinned source-compatible metadata; their original execution is unproven.
 * No generic disconnected allocation inference is performed. */
static void MeleePc_EffectsDonkeyPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if(p->size!=59856||root!=0||!title_range(p,0,160)){p->error=-2;return;}
 for(unsigned at=0;at<8;at++)if(p->bytes[32+at]||p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 for(unsigned at=148;at<160;at++)if(p->bytes[32+at]||p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 static const uint32_t shape_fields[][3]={{39684,0,0},{39688,0,0},{39692,39700,1},{39696,0,0},{39700,39708,1},{39704,0,0},{39708,39716,1},{39712,0,0},{39716,0,0},{39720,0,0},{39724,0,0},{39728,0,0},{39732,0,0},{39736,0,0},{39740,0,0},{39744,0,0},{39748,0,0},{39752,0,0},{39756,0,0},{39760,0,0},{39764,0,0},{39768,0,0},{39772,39784,1},{39776,0,0},{39780,0,0},{39784,39796,1},{39788,39820,1},{39792,0,0},{39796,0,0},{39800,39808,1},{39804,39684,1},{39808,0,0},{39812,0,0},{39816,39692,1},{39820,39832,1},{39824,39880,1},{39828,0,0},{39832,0,0},{39836,39844,1},{39840,39724,1},{39844,0,0},{39848,39856,1},{39852,39732,1},{39856,0,0},{39860,39868,1},{39864,39740,1},{39868,0,0},{39872,0,0},{39876,39748,1},{39880,39892,1},{39884,0,0},{39888,39756,1},{39892,0,0},{39896,0,0},{39900,39764,1},{42944,0,0},{42948,0,0},{42952,42960,1},{42956,0,0},{42960,42968,1},{42964,0,0},{42968,42976,1},{42972,0,0},{42976,0,0},{42980,0,0},{42984,0,0},{42988,0,0},{42992,0,0},{42996,0,0},{43000,0,0},{43004,0,0},{43008,0,0},{43012,0,0},{43016,0,0},{43020,0,0},{43024,0,0},{43028,0,0},{43032,43044,1},{43036,0,0},{43040,0,0},{43044,43056,1},{43048,43080,1},{43052,0,0},{43056,0,0},{43060,43068,1},{43064,42944,1},{43068,0,0},{43072,0,0},{43076,42952,1},{43080,43092,1},{43084,43140,1},{43088,0,0},{43092,0,0},{43096,43104,1},{43100,42984,1},{43104,0,0},{43108,43116,1},{43112,42992,1},{43116,0,0},{43120,43128,1},{43124,43000,1},{43128,0,0},{43132,0,0},{43136,43008,1},{43140,43152,1},{43144,0,0},{43148,43016,1},{43152,0,0},{43156,0,0},{43160,43024,1}};
 for(unsigned i=0;i<sizeof(shape_fields)/sizeof(shape_fields[0]);i++){
  uint32_t at=shape_fields[i][0];
  if(!title_range(p,at,4)||title_u32(p,at)!=shape_fields[i][1]||!!p->relocs[at/4]!=shape_fields[i][2]||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
 }
 static const unsigned types[]={TITLE_HSD_Joint,TITLE_HSD_AnimJoint,TITLE_HSD_MatAnimJoint,TITLE_HSD_ShapeAnimJoint};
 for(unsigned at=8;at<148;at+=20){
  if(p->relocs[at/4]||(p->external_slots&&p->external_slots[at/4])){p->error=-2;return;}
  uint32_t bits=title_u32(p,at);float lifetime;memcpy(&lifetime,&bits,4);
  if(!isfinite(lifetime)){p->error=-1;return;}
  title_claim(p,at,4,4);
  for(unsigned i=0;i<4&&!p->error;i++){
   uint32_t target=title_pointer(p,at+4+i*4);
   if(target!=UINT32_MAX)title_enqueue(p,target,types[i]);
  }
 }
 title_claim(p,0,8,4);title_claim(p,148,12,1);
 title_enqueue(p,39772,TITLE_HSD_ShapeAnimJoint);title_enqueue(p,43032,TITLE_HSD_ShapeAnimJoint);
}
#endif
