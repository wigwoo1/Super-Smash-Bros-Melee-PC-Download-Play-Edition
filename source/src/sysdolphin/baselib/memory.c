#include "memory.h"

#include <Runtime/platform.h>

#include "debug.h"
#include "initialize.h"
#include <dolphin/os/OSAlloc.h>
#ifdef TARGET_PC
#include <pc/asset_regions.h>
#include <intrin.h>
#include <stdlib.h>
#include <dolphin/os.h>
#endif

void HSD_Free(void* ptr)
{
#ifdef TARGET_PC
    MeleePc_UnregisterAssetAllocation(ptr);
#endif
    OSFreeToHeap(HSD_GetHeap(), ptr);
}

void* HSD_MemAlloc(ssize_t size)
{
    void* adr;

    if (size <= 0) {
        return NULL;
    }

    adr = OSAllocFromHeap(HSD_GetHeap(), size);
#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_HEAP_ALLOCATIONS")) {
        OSReport("NATIVE_HSD_ALLOC requested=%ld heap=%d result=%p caller=%p\n",
                 (long) size, HSD_GetHeap(), adr, _ReturnAddress());
    }
    if (!adr) {
        OSReport("NATIVE_HSD_ALLOC_FAILURE requested=%ld heap=%d free=%ld caller=%p\n",
                 (long) size, HSD_GetHeap(),
                 (long) OSCheckHeap(HSD_GetHeap()), _ReturnAddress());
    }
#endif
    HSD_ASSERT(52, adr);

    return adr;
}
