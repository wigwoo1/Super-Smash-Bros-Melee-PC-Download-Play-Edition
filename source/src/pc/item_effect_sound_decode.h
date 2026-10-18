#ifndef MELEE_PC_ITEM_EFFECT_SOUND_DECODE_H
#define MELEE_PC_ITEM_EFFECT_SOUND_DECODE_H
#include "item_script_bits.h"
typedef struct { uint32_t attachment,effect,parameter; int32_t coordinates[6]; } MeleePc_ItemEffectArgs;
static uint16_t MeleePc_ItemHalf(const unsigned char* p){return (uint16_t)((p[0]<<8)|p[1]);}
static int MeleePc_ItemEffectDecode(const void* data,uint32_t size,MeleePc_ItemEffectArgs* out){
 if(!data || !out || size<20)return 0;
 const unsigned char* p=data;if((MeleePc_ItemScriptWord(p)>>26)!=10)return 0;
 MeleePc_ItemEffectArgs value={0};value.attachment=MeleePc_ItemHalf(p)&1023;
 value.effect=MeleePc_ItemHalf(p+4);value.parameter=MeleePc_ItemHalf(p+6);
 for(unsigned i=0;i<6;i++){unsigned n=MeleePc_ItemHalf(p+8+i*2);value.coordinates[i]=n>=32768?(int32_t)n-65536:(int32_t)n;}
 *out=value;return 1;
}
typedef struct { uint32_t operation,sound,parameter2,parameter3; } MeleePc_ItemSoundArgs;
static int MeleePc_ItemSoundDecode(const void* data,uint32_t size,MeleePc_ItemSoundArgs* out){
 if(!data || !out || size<12)return 0;
 const unsigned char* p=data;uint32_t word=MeleePc_ItemScriptWord(p),operation=(word>>18)&255;
 if((word>>26)!=16 || (operation!=0 && operation!=1 && operation!=2 && operation!=10 && operation!=11))return 0;
 MeleePc_ItemSoundArgs value={operation,0,0,0};
 if(operation<3){value.sound=MeleePc_ItemScriptWord(p+4);value.parameter2=p[10];value.parameter3=p[11];}
 *out=value;return 1;
}
#endif
