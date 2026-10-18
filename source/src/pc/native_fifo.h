#ifndef MELEE_PC_NATIVE_FIFO_H
#define MELEE_PC_NATIVE_FIFO_H
#include <dolphin/gx/GXVert.h>
/* One scalar at a time preserves the original FIFO write order. */
static inline void MeleePc_FifoF32(f32 value)
{
#ifdef TARGET_PC
    GXParam1f32(value);
#else
    GXWGFifo.f32 = value;
#endif
}
static inline void MeleePc_FifoEnd(void)
{
#ifdef TARGET_PC
    GXEnd();
#endif
}
#endif
