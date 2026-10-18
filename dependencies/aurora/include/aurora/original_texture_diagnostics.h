#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Opt-in owner-thread publication. Original bytes are copied immediately into
 * diagnostic RAM;data is an identity only,never dereferenced by the consumer.
 * Return1 is registration,not drawing/submission/GPU verification. */
typedef struct AuroraOriginalTexture {
    const void* data;const void* original;const void* owner;
    uint32_t bytes,width,height,format,textureOffset,imageOffset,animOffset;
    uint64_t bodyHash;
    /* Optional original C8 palette. Non-paletted sources leave these zero.
     * Both payloads are copied into diagnostic-owned RAM before returning. */
    const void* paletteData;const void* paletteOriginal;
    uint32_t paletteBytes,paletteFormat,paletteEntries,paletteOffset;
} AuroraOriginalTexture;
int aurora_diagnostic_original_texture(const AuroraOriginalTexture* source);
void aurora_diagnostic_forget_original_texture(const void* owner);
/* Attach an existing GX object ID/version to its exact original owner. The
 * ordinary FIFO already carries that ID/version;no GX command/state changes. */
int aurora_diagnostic_original_texture_object(const void* textureObject,const void* owner);
#ifdef __cplusplus
}
#endif
