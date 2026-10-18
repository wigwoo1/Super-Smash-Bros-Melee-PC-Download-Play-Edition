#ifndef MELEE_PC_ARENA_RANGE_H
#define MELEE_PC_ARENA_RANGE_H
#include <stdint.h>
/* Native MEM1 pointers are unrelated to the console's 0x80000000 split.
 * Validate membership in the actual CPU arena or logical ARAM offset range. */
static int MeleePc_ArenaRange(uintptr_t lo,uintptr_t hi,
                             uintptr_t cpu_lo,uintptr_t cpu_hi,
                             uintptr_t aram_lo,uintptr_t aram_hi){
    if(lo>=hi || ((lo|hi)&31))return 0;
    if(cpu_lo<cpu_hi && lo>=cpu_lo && hi<=cpu_hi)return 2;
    if(aram_lo<aram_hi && lo>=aram_lo && hi<=aram_hi)return 1;
    return 0;
}
#endif
