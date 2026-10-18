#ifndef MELEE_PC_YOSHI_ISOLATED_POINTERS_H
#define MELEE_PC_YOSHI_ISOLATED_POINTERS_H
/* Exact original isolated pointer/zero islands. Unknown nonzero words refused. */
static void MeleePc_YoshiIsolatedPointersPlan(MeleePc_TitlePlan* p){
 if(p->error)return;
 if(p->size!=271232){p->error=-2;return;}
 if(title_u32(p,111968)!=103560u || p->relocs[111968/4]!=1 || (p->external_slots && p->external_slots[111968/4])){p->error=-1;return;}
 if(title_u32(p,111972)!=0u || p->relocs[111972/4]!=0 || (p->external_slots && p->external_slots[111972/4])){p->error=-1;return;}
 if(title_u32(p,111976)!=0u || p->relocs[111976/4]!=0 || (p->external_slots && p->external_slots[111976/4])){p->error=-1;return;}
 if(title_u32(p,111980)!=0u || p->relocs[111980/4]!=0 || (p->external_slots && p->external_slots[111980/4])){p->error=-1;return;}
 if(title_u32(p,111984)!=0u || p->relocs[111984/4]!=0 || (p->external_slots && p->external_slots[111984/4])){p->error=-1;return;}
 if(title_u32(p,111988)!=0u || p->relocs[111988/4]!=0 || (p->external_slots && p->external_slots[111988/4])){p->error=-1;return;}
 if(title_u32(p,111992)!=0u || p->relocs[111992/4]!=0 || (p->external_slots && p->external_slots[111992/4])){p->error=-1;return;}
 if(title_u32(p,111996)!=0u || p->relocs[111996/4]!=0 || (p->external_slots && p->external_slots[111996/4])){p->error=-1;return;}
 if(title_u32(p,123328)!=114920u || p->relocs[123328/4]!=1 || (p->external_slots && p->external_slots[123328/4])){p->error=-1;return;}
 if(title_u32(p,123332)!=0u || p->relocs[123332/4]!=0 || (p->external_slots && p->external_slots[123332/4])){p->error=-1;return;}
 if(title_u32(p,123336)!=0u || p->relocs[123336/4]!=0 || (p->external_slots && p->external_slots[123336/4])){p->error=-1;return;}
 if(title_u32(p,123340)!=0u || p->relocs[123340/4]!=0 || (p->external_slots && p->external_slots[123340/4])){p->error=-1;return;}
 if(title_u32(p,123344)!=111968u || p->relocs[123344/4]!=1 || (p->external_slots && p->external_slots[123344/4])){p->error=-1;return;}
 if(title_u32(p,123348)!=123328u || p->relocs[123348/4]!=1 || (p->external_slots && p->external_slots[123348/4])){p->error=-1;return;}
 if(title_u32(p,123352)!=0u || p->relocs[123352/4]!=0 || (p->external_slots && p->external_slots[123352/4])){p->error=-1;return;}
 if(title_u32(p,123356)!=0u || p->relocs[123356/4]!=0 || (p->external_slots && p->external_slots[123356/4])){p->error=-1;return;}
 if(title_u32(p,192032)!=192048u || p->relocs[192032/4]!=1 || (p->external_slots && p->external_slots[192032/4])){p->error=-1;return;}
 if(title_u32(p,192036)!=0u || p->relocs[192036/4]!=0 || (p->external_slots && p->external_slots[192036/4])){p->error=-1;return;}
 if(title_u32(p,192040)!=0u || p->relocs[192040/4]!=0 || (p->external_slots && p->external_slots[192040/4])){p->error=-1;return;}
 if(title_u32(p,192044)!=0u || p->relocs[192044/4]!=0 || (p->external_slots && p->external_slots[192044/4])){p->error=-1;return;}
 if(title_u32(p,192048)!=0u || p->relocs[192048/4]!=0 || (p->external_slots && p->external_slots[192048/4])){p->error=-1;return;}
 if(title_u32(p,192052)!=0u || p->relocs[192052/4]!=0 || (p->external_slots && p->external_slots[192052/4])){p->error=-1;return;}
 if(title_u32(p,192056)!=0u || p->relocs[192056/4]!=0 || (p->external_slots && p->external_slots[192056/4])){p->error=-1;return;}
 if(title_u32(p,192060)!=0u || p->relocs[192060/4]!=0 || (p->external_slots && p->external_slots[192060/4])){p->error=-1;return;}
 if(title_u32(p,192064)!=0u || p->relocs[192064/4]!=0 || (p->external_slots && p->external_slots[192064/4])){p->error=-1;return;}
 if(title_u32(p,192068)!=0u || p->relocs[192068/4]!=0 || (p->external_slots && p->external_slots[192068/4])){p->error=-1;return;}
 if(title_u32(p,192072)!=0u || p->relocs[192072/4]!=0 || (p->external_slots && p->external_slots[192072/4])){p->error=-1;return;}
 if(title_u32(p,192076)!=0u || p->relocs[192076/4]!=0 || (p->external_slots && p->external_slots[192076/4])){p->error=-1;return;}
 if(title_u32(p,192080)!=192092u || p->relocs[192080/4]!=1 || (p->external_slots && p->external_slots[192080/4])){p->error=-1;return;}
 if(title_u32(p,192084)!=0u || p->relocs[192084/4]!=0 || (p->external_slots && p->external_slots[192084/4])){p->error=-1;return;}
 if(title_u32(p,192088)!=0u || p->relocs[192088/4]!=0 || (p->external_slots && p->external_slots[192088/4])){p->error=-1;return;}
 if(title_u32(p,192092)!=192104u || p->relocs[192092/4]!=1 || (p->external_slots && p->external_slots[192092/4])){p->error=-1;return;}
 if(title_u32(p,192096)!=0u || p->relocs[192096/4]!=0 || (p->external_slots && p->external_slots[192096/4])){p->error=-1;return;}
 if(title_u32(p,192100)!=192032u || p->relocs[192100/4]!=1 || (p->external_slots && p->external_slots[192100/4])){p->error=-1;return;}
 if(title_u32(p,192104)!=0u || p->relocs[192104/4]!=0 || (p->external_slots && p->external_slots[192104/4])){p->error=-1;return;}
 if(title_u32(p,192108)!=0u || p->relocs[192108/4]!=0 || (p->external_slots && p->external_slots[192108/4])){p->error=-1;return;}
 if(title_u32(p,192112)!=192064u || p->relocs[192112/4]!=1 || (p->external_slots && p->external_slots[192112/4])){p->error=-1;return;}
 if(title_u32(p,192116)!=192124u || p->relocs[192116/4]!=1 || (p->external_slots && p->external_slots[192116/4])){p->error=-1;return;}
 if(title_u32(p,192120)!=0u || p->relocs[192120/4]!=0 || (p->external_slots && p->external_slots[192120/4])){p->error=-1;return;}
 if(title_u32(p,192124)!=0u || p->relocs[192124/4]!=0 || (p->external_slots && p->external_slots[192124/4])){p->error=-1;return;}
 if(title_u32(p,192128)!=0u || p->relocs[192128/4]!=0 || (p->external_slots && p->external_slots[192128/4])){p->error=-1;return;}
 if(title_u32(p,192132)!=0u || p->relocs[192132/4]!=0 || (p->external_slots && p->external_slots[192132/4])){p->error=-1;return;}
 if(title_u32(p,192136)!=0u || p->relocs[192136/4]!=0 || (p->external_slots && p->external_slots[192136/4])){p->error=-1;return;}
 if(title_u32(p,192140)!=192152u || p->relocs[192140/4]!=1 || (p->external_slots && p->external_slots[192140/4])){p->error=-1;return;}
 if(title_u32(p,192144)!=0u || p->relocs[192144/4]!=0 || (p->external_slots && p->external_slots[192144/4])){p->error=-1;return;}
 if(title_u32(p,192148)!=0u || p->relocs[192148/4]!=0 || (p->external_slots && p->external_slots[192148/4])){p->error=-1;return;}
 if(title_u32(p,192152)!=192164u || p->relocs[192152/4]!=1 || (p->external_slots && p->external_slots[192152/4])){p->error=-1;return;}
 if(title_u32(p,192156)!=0u || p->relocs[192156/4]!=0 || (p->external_slots && p->external_slots[192156/4])){p->error=-1;return;}
 if(title_u32(p,192160)!=192116u || p->relocs[192160/4]!=1 || (p->external_slots && p->external_slots[192160/4])){p->error=-1;return;}
 if(title_u32(p,192164)!=0u || p->relocs[192164/4]!=0 || (p->external_slots && p->external_slots[192164/4])){p->error=-1;return;}
 if(title_u32(p,192168)!=0u || p->relocs[192168/4]!=0 || (p->external_slots && p->external_slots[192168/4])){p->error=-1;return;}
 if(title_u32(p,192172)!=192132u || p->relocs[192172/4]!=1 || (p->external_slots && p->external_slots[192172/4])){p->error=-1;return;}
 if(title_u32(p,192176)!=191972u || p->relocs[192176/4]!=1 || (p->external_slots && p->external_slots[192176/4])){p->error=-1;return;}
 if(title_u32(p,192180)!=0u || p->relocs[192180/4]!=0 || (p->external_slots && p->external_slots[192180/4])){p->error=-1;return;}
 if(title_u32(p,192184)!=192080u || p->relocs[192184/4]!=1 || (p->external_slots && p->external_slots[192184/4])){p->error=-1;return;}
 if(title_u32(p,192188)!=0u || p->relocs[192188/4]!=0 || (p->external_slots && p->external_slots[192188/4])){p->error=-1;return;}
 if(title_u32(p,192192)!=192140u || p->relocs[192192/4]!=1 || (p->external_slots && p->external_slots[192192/4])){p->error=-1;return;}
 if(title_u32(p,192196)!=0u || p->relocs[192196/4]!=0 || (p->external_slots && p->external_slots[192196/4])){p->error=-1;return;}
 if(title_u32(p,192200)!=126056u || p->relocs[192200/4]!=1 || (p->external_slots && p->external_slots[192200/4])){p->error=-1;return;}
 if(title_u32(p,192204)!=192176u || p->relocs[192204/4]!=1 || (p->external_slots && p->external_slots[192204/4])){p->error=-1;return;}
 if(title_u32(p,192208)!=192184u || p->relocs[192208/4]!=1 || (p->external_slots && p->external_slots[192208/4])){p->error=-1;return;}
 if(title_u32(p,192212)!=192192u || p->relocs[192212/4]!=1 || (p->external_slots && p->external_slots[192212/4])){p->error=-1;return;}
 if(title_u32(p,192216)!=192200u || p->relocs[192216/4]!=1 || (p->external_slots && p->external_slots[192216/4])){p->error=-1;return;}
 if(title_u32(p,192220)!=0u || p->relocs[192220/4]!=0 || (p->external_slots && p->external_slots[192220/4])){p->error=-1;return;}
 title_pointer(p,111968);
 title_claim(p,111972,4,4);
 title_claim(p,111976,4,4);
 title_claim(p,111980,4,4);
 title_claim(p,111984,4,4);
 title_claim(p,111988,4,4);
 title_claim(p,111992,4,4);
 title_claim(p,111996,4,4);
 title_pointer(p,123328);
 title_claim(p,123332,4,4);
 title_claim(p,123336,4,4);
 title_claim(p,123340,4,4);
 title_pointer(p,123344);
 title_pointer(p,123348);
 title_claim(p,123352,4,4);
 title_claim(p,123356,4,4);
 title_pointer(p,192032);
 title_claim(p,192036,4,4);
 title_claim(p,192040,4,4);
 title_claim(p,192044,4,4);
 title_claim(p,192048,4,4);
 title_claim(p,192052,4,4);
 title_claim(p,192056,4,4);
 title_claim(p,192060,4,4);
 title_claim(p,192064,4,4);
 title_claim(p,192068,4,4);
 title_claim(p,192072,4,4);
 title_claim(p,192076,4,4);
 title_pointer(p,192080);
 title_claim(p,192084,4,4);
 title_claim(p,192088,4,4);
 title_pointer(p,192092);
 title_claim(p,192096,4,4);
 title_pointer(p,192100);
 title_claim(p,192104,4,4);
 title_claim(p,192108,4,4);
 title_pointer(p,192112);
 title_pointer(p,192116);
 title_claim(p,192120,4,4);
 title_claim(p,192124,4,4);
 title_claim(p,192128,4,4);
 title_claim(p,192132,4,4);
 title_claim(p,192136,4,4);
 title_pointer(p,192140);
 title_claim(p,192144,4,4);
 title_claim(p,192148,4,4);
 title_pointer(p,192152);
 title_claim(p,192156,4,4);
 title_pointer(p,192160);
 title_claim(p,192164,4,4);
 title_claim(p,192168,4,4);
 title_pointer(p,192172);
 title_pointer(p,192176);
 title_claim(p,192180,4,4);
 title_pointer(p,192184);
 title_claim(p,192188,4,4);
 title_pointer(p,192192);
 title_claim(p,192196,4,4);
 title_pointer(p,192200);
 title_pointer(p,192204);
 title_pointer(p,192208);
 title_pointer(p,192212);
 title_pointer(p,192216);
 title_claim(p,192220,4,4);
}
#endif
