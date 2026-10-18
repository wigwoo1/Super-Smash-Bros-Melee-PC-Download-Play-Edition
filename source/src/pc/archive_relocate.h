#ifndef MELEE_PC_ARCHIVE_RELOCATE_H
#define MELEE_PC_ARCHIVE_RELOCATE_H
#include <stdint.h>
/* Input words/slot table must already be native endian. Returns 0 on success;
 * -1 on invalid ranges/duplicates/host ABI. Failure leaves data unchanged.
 * This is not an endian converter or a complete DAT loader. */
int MeleePc_RelocateOffsets(uint8_t* data, uint32_t size,
                           const uint32_t* slots, uint32_t count);
#endif
