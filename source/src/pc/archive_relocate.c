#include "archive_relocate.h"
#include <string.h>
#include <stddef.h>
int MeleePc_RelocateOffsets(uint8_t* data, uint32_t size,
                           const uint32_t* slots, uint32_t count)
{
    uint32_t i, j, target;
    uintptr_t base = (uintptr_t)data;
    if (sizeof(void*) != 4 || data == NULL || (count && slots == NULL) ||
        base > UINT32_MAX - size || count > size / 4) return -1;
    /* Slot metadata must not alias the mutable body. */
    if (count) {
        uintptr_t table = (uintptr_t)slots;
        uint32_t bytes = count * 4;
        if (table > UINT32_MAX - bytes ||
            (table < base + size && base < table + bytes)) return -1;
    }
    /* Validate the entire plan before changing any pointer. */
    for (i = 0; i < count; ++i) {
        if ((slots[i] & 3) || size < 4 || slots[i] > size - 4) return -1;
        for (j = 0; j < i; ++j) if (slots[j] == slots[i]) return -1;
        memcpy(&target, data + slots[i], 4);
        /* One-past-body pointers occur in original archives. */
        if (target > size) return -1;
    }
    for (i = 0; i < count; ++i) {
        memcpy(&target, data + slots[i], 4);
        target += (uint32_t)base;
        memcpy(data + slots[i], &target, 4);
    }
    return 0;
}

