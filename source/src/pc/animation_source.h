#ifndef MELEE_PC_ANIMATION_SOURCE_H
#define MELEE_PC_ANIMATION_SOURCE_H
#include <stdint.h>
#include <stddef.h>
/* 2 = native MEM1 bytes, 1 = bounded aligned logical ARAM transfer. */
static int MeleePc_AnimationSource(uintptr_t address,size_t length,
                                  uintptr_t cpu_lo,size_t cpu_size,size_t aram_size){
 if(!length)return 0;
 if(cpu_size && cpu_lo<=UINTPTR_MAX-cpu_size && address>=cpu_lo &&
    length<=cpu_size && address-cpu_lo<=cpu_size-length)return 2;
 if(length>SIZE_MAX-31)return 0;
 size_t rounded=(length+31)&~(size_t)31;
 if(!(address&31) && rounded<=aram_size && address<=aram_size-rounded)return 1;
 return 0;
}
#endif
