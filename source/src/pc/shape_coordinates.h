#ifndef MELEE_PC_SHAPE_COORDINATES_H
#define MELEE_PC_SHAPE_COORDINATES_H
#include <stdint.h>
#include <string.h>

/* Shape payloads remain GX big-endian, even when descriptors are native.
 * Byte reads also allow unaligned archive buffers; never rewrite shared data. */
static inline unsigned pc_shape_u16(const void* source)
{
    const unsigned char* p = source;
    return ((unsigned) p[0] << 8) | p[1];
}
static inline int pc_shape_s16(const void* source)
{
    unsigned value = pc_shape_u16(source);
    return value < 32768 ? (int) value : (int) value - 65536;
}
static inline void pc_shape_f32_array(const void* source, float* output,
                                      unsigned count)
{
    const unsigned char* p = source;
    unsigned i;
    for (i = 0; i < count; ++i, p += 4) {
        uint32_t bits = ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16) |
                        ((uint32_t) p[2] << 8) | p[3];
        memcpy(output + i, &bits, sizeof(bits));
    }
}
#endif
