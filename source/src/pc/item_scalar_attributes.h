#ifndef MELEE_PC_ITEM_SCALAR_ATTRIBUTES_H
#define MELEE_PC_ITEM_SCALAR_ATTRIBUTES_H
/* Source-audited all-word attributes; bytes and pointer-bearing types require distinct planners. */
static void MeleePc_ItemScalarAttributesPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 uint32_t size=0, floats=0;
 switch(kind){
 case 0:size=8;floats=0x0u;break;
 case 1:size=32;floats=0xe0u;break;
 case 2:size=56;floats=0x3ffcu;break;
 case 10:size=24;floats=0x3fu;break;
 case 11:size=16;floats=0x8u;break;
 case 16:size=16;floats=0xeu;break;
 case 17:size=20;floats=0x1fu;break;
 case 20:size=40;floats=0x3d8u;break;
 case 21:size=64;floats=0xfff8u;break;
 case 23:size=16;floats=0xeu;break;
 case 24:size=4;floats=0x1u;break;
 case 25:size=24;floats=0x30u;break;
 case 35:size=12;floats=0x7u;break;
 case 39:size=20;floats=0x1fu;break;
 case 48:size=20;floats=0x1fu;break; /* Mario fireball: source itUnkAttributes. */
 case 49:size=20;floats=0x1fu;break; /* itDrMarioPillAttributes: five source floats. */
 case 50:size=16;floats=0xfu;break; /* itKirbyCutterBeamAttributes */
 case 52:size=4;floats=0x1u;break; /* Source reads only x0_float prefix. */
 case 54:case 55:size=40;floats=0x3ffu;break; /* FoxLaserAttr, shared Falco laser. */
 case 56:case 57:size=8;floats=0x3u;break; /* FoxIllusionAttr, Falco Phantasm. */
 case 58:size=52;floats=0x1fe6u;break; /* Source-used prefix through x30; unused vel[3] overlaps packed commands. */
 case 110:size=8;floats=0x3u;break; /* Disable lifetime and horizontal velocity. */
 case 112:size=48;floats=0xeffu;break; /* ShadowBall source-used prefix through x2C; x20 signed integer, packed commands begin at x30. */
 case 105:case 132:size=16;floats=0xfu;break; /* Luigi fireball source-used itUnkAttributes prefix through xC; commands follow. */
 case 93:size=16;floats=0xfu;break; /* Samus bomb source-used prefix; commands follow. */
 case 94:size=32;floats=0xfdu;break; /* Samus charge shot: x4 integer. */
 case 95:size=56;floats=0x3fffu;break; /* Samus missile source-used through x34; packed commands follow. */
 case 86:case 88:size=8;floats=0x3u;break; /* Source EggThrow and StarAttrs: two floats. */
 case 100:size=24;floats=0x3fu;break; /* Source itKoopaFlame_Attributes. */
 default:p->error=-2;return;
 }
 if(p->error || (root&3) || !title_range(p,root,size)){p->error=-1;return;}
 for(uint32_t off=0;off<size;off+=4){
  uint32_t slot=root+off;
  if(p->relocs[slot/4] || (p->external_slots && p->external_slots[slot/4])){p->error=-1;return;}
  if(floats & (1u<<(off/4))){uint32_t bits=title_u32(p,slot);float f;memcpy(&f,&bits,4);if(!isfinite(f)){p->error=-1;return;}}
 }
 title_claim(p,root,size,4);
}
#endif
