#ifndef MELEE_PC_ITEM_SCRIPT_BITS_H
#define MELEE_PC_ITEM_SCRIPT_BITS_H
#include <stdint.h>
/* Packed script words remain in original byte order. No C bitfield ABI. */
static uint32_t MeleePc_ItemScriptWord(const void* pointer){
 const unsigned char* p=pointer;
 return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static int32_t MeleePc_ItemScriptBits(const void* pointer,unsigned shift,unsigned width,int signed_value){
 uint32_t mask=width==32?UINT32_MAX:((1u<<width)-1),value=(MeleePc_ItemScriptWord(pointer)>>shift)&mask;
 if(signed_value && width<32 && (value&(1u<<(width-1))))return (int32_t)((int64_t)value-(1LL<<width));
 return (int32_t)value;
}
#endif
