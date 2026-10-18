#ifndef MELEE_PC_PARTICLE_STREAM_H
#define MELEE_PC_PARTICLE_STREAM_H
#include <stdint.h>
#include <string.h>
/* Packed command operands stay in original BE byte order, independent of
 * descriptor conversion. Caller validates four readable bytes first. */
static uint32_t MeleePc_ParticleWord(const unsigned char* bytes) {
    return ((uint32_t)bytes[0]<<24)|((uint32_t)bytes[1]<<16)|
           ((uint32_t)bytes[2]<<8)|bytes[3];
}
static float MeleePc_ParticleFloat(uint32_t bits) {
    float value;memcpy(&value,&bits,sizeof(value));return value;
}
#endif
