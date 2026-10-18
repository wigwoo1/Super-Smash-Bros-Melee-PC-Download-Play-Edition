#ifndef MELEE_PC_ITEM_MIXED_ATTRIBUTES_H
#define MELEE_PC_ITEM_MIXED_ATTRIBUTES_H
/* Source typed layouts: preserve padding bytes, convert integer/float words. */
static void MeleePc_ItemMixedAttributesPlan(MeleePc_TitlePlan* p,uint32_t root,unsigned kind){
 uint32_t size=0,floats=0,bytes=0;
 switch(kind){
 case 79:size=12;floats=0x7u;break; /* itSeakNeedleThrownAttributes */
 /* Zelda Din's Fire source attributes: twelve and five float words. */
 case 108:size=48;floats=0xfffu;break;
 case 109:size=20;floats=0x1fu;break;
 case 4:size=48;floats=0xdc0u;bytes=0x200u;break;
 case 5:size=84;floats=0x1c03fdu;bytes=0x2u;break;
 case 8:size=28;floats=0x60u;bytes=0x0u;break;
 case 9:size=24;floats=0x20u;bytes=0x0u;break;
 case 38:size=128;floats=0xc7ffffffu;bytes=0x38000000u;break;
 case 40:size=8;floats=0x3u;bytes=0x0u;break;
 case 42:size=8;break;
 case 7:size=24;floats=0x5u;bytes=0x0u;break;
 case 14:size=64;floats=0xffbfu;bytes=0x40u;break;
 case 22:size=16;floats=0xeu;bytes=0x0u;break;
 case 36:size=32;floats=0x9fu;bytes=0x0u;break;
 case 37:size=28;floats=0x1fu;bytes=0x0u;break;
 case 159:size=76;floats=0x7ffffu;bytes=0x0u;break;
 /* itSamusChargeShot_Attributes: seven float words and one integer word. */
 case 151:size=32;floats=0xfdu;bytes=0x0u;break;
 case 15:size=88;floats=0x1fcfffu;bytes=0x3000u;break;
 case 19:size=24;floats=0x3fu;bytes=0x0u;break;
 /* Sword leading unused words and afterimage color bytes stay opaque. */
 case 12:size=48;floats=0x1b8u;bytes=0xe07u;break;
 case 13:size=16;floats=0xfu;break;
 case 28:size=12;floats=0x4u;break;
 case 41:size=4;floats=0x1u;break;
 case 31:size=4;floats=0x1u;break;
 case 32:size=8;floats=0x3u;break;
 /* Shared Pokemon type uses source-required prefixes, not sizeof. */
 case 173:size=8;floats=0x1u;break;
 case 174:size=8;floats=0x1u;break;
 case 175:size=8;floats=0x1u;break;
 case 177:size=16;floats=0x1u;break;
 case 181:size=24;floats=0x1u;break;
 case 182:size=28;floats=0x1u;break;
 /* Chicorita spawn offsets are read from the parent, not the leaf. */
 case 162:size=20;floats=0x1du;break;
 case 164:size=32;floats=0xfdu;break;
 case 165:size=16;floats=0xfu;break;
 case 191:size=4;floats=0x1u;break;
 case 192:size=4;floats=0x1u;break;
 /* Both gas kinds share a record; Gas2 consumes the third word. */
 case 193:size=8;floats=0x3u;break;
 case 194:size=12;floats=0x7u;break;
 /* Lizardon parent x4 is consumed as an integer timer via cast. */
 case 166:size=48;floats=0x1du;break;
 case 189:size=24;floats=0x3fu;break;
 /* Four flame variants alias a two-float lifetime/velocity record. */
 case 195:case 196:case 197:case 198:size=8;floats=0x3u;break;
 case 186:size=28;floats=0xfu;break;
 case 205:size=8;floats=0x1u;break;
 case 188:size=12;floats=0x7u;break;
 case 190:size=12;floats=0x7u;break;
 case 206:size=24;floats=0x37u;bytes=0x8u;break;
 case 163:size=28;floats=0x4fu;break;
 case 167:size=16;floats=0xfu;break;
 case 168:size=16;floats=0x7u;break;
 case 169:size=16;floats=0x7u;break;
 case 161:size=12;floats=0x7u;break;
 case 170:size=40;floats=0x1ffu;break;
 case 176:size=16;floats=0x1u;break;
 case 178:size=68;floats=0x1ffdfu;break;
 case 179:size=32;floats=0xddu;bytes=0x2u;break;
 case 200:size=8;floats=0x3u;break;
 case 201:size=12;floats=0x7u;break;
 case 202:size=16;floats=0xfu;break;
 case 203:size=4;floats=0x1u;break;
 case 185:size=92;floats=0x7c7fffu;break;
 case 204:size=4;floats=0x1u;break;
 case 171:size=36;floats=0x3fu;break;
 case 180:size=4;floats=0x1u;break;
 case 183:case 184:size=16;floats=0xfu;break;
 default:p->error=-2;return;
 }
 if(p->error || (root&3) || !title_range(p,root,size)){p->error=-1;return;}
 if(kind==42 && title_u32(p,root+4)){p->error=-2;return;}
 for(uint32_t off=0;off<size;off+=4){
  uint32_t slot=root+off,bit=1u<<(off/4);
  if(p->relocs[slot/4] || (p->external_slots && p->external_slots[slot/4])){p->error=-1;return;}
  if(floats&bit){uint32_t bits=title_u32(p,slot);float f;memcpy(&f,&bits,4);if(!isfinite(f)){p->error=-1;return;}}
  title_claim(p,slot,4,bytes&bit?1:4);
 }
}
#endif
