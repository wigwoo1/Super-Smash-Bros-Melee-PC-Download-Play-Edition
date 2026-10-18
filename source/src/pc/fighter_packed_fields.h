#ifndef MELEE_PC_FIGHTER_PACKED_FIELDS_H
#define MELEE_PC_FIGHTER_PACKED_FIELDS_H
#ifdef TARGET_PC
#include <stdint.h>
static inline uint32_t MeleePc_FighterPackedWord(const void* data){
 const unsigned char* b=data;
 return ((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3];
}
static inline int32_t MeleePc_FighterPackedSigned(uint32_t word,unsigned shift,unsigned width){
 uint32_t value=word>>shift,sign=UINT32_C(1)<<(width-1);
 uint32_t mask=width==32?UINT32_MAX:(UINT32_C(1)<<width)-1;
 return (int32_t)((int64_t)(value&mask)-((value&sign)?(INT64_C(1)<<width):0));
}
#define FT_PACKED_U(cmd,member,shift,mask) ((MeleePc_FighterPackedWord((cmd)->x8.u)>>(shift))&(mask))
#define FT_PACKED_S(cmd,member,shift,width) MeleePc_FighterPackedSigned(MeleePc_FighterPackedWord((cmd)->x8.u),shift,width)
#define FT_PACKED_WORD(cmd,offset) MeleePc_FighterPackedWord((const unsigned char*)(cmd)->x8.u+(offset))
#define FT_PACKED_HALF(cmd,offset) ((((const unsigned char*)(cmd)->x8.u)[offset]<<8)|((const unsigned char*)(cmd)->x8.u)[(offset)+1])
#else
#define FT_PACKED_U(cmd,member,shift,mask) ((cmd)->x8.u->member)
#define FT_PACKED_S(cmd,member,shift,width) ((cmd)->x8.u->member)
#define FT_PACKED_WORD(cmd,offset) (*(u32*)((u8*)(cmd)->x8.u+(offset)))
#define FT_PACKED_HALF(cmd,offset) (*(u16*)((u8*)(cmd)->x8.u+(offset)))
#endif
#endif
