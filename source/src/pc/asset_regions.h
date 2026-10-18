#ifndef MELEE_PC_ASSET_REGIONS_H
#define MELEE_PC_ASSET_REGIONS_H
#include <stdint.h>
/* Call only on the asset/render owner thread. Registry does not own memory.
 * Unregister before freeing. Overlap and duplicate registration reject. */
int MeleePc_RegisterAssetRegion(const void* base, uint32_t size);
/* Owner is the allocation address, which can precede the validated body.
 * One allocation may contain several disjoint registered regions. */
int MeleePc_RegisterOwnedAssetRegion(const void* owner, const void* base, uint32_t size);
void MeleePc_UnregisterAssetAllocation(const void* owner);
int MeleePc_AssetAllocationRegistered(const void* owner);
/* Forget allocations owned by the half-open heap interval before destruction. */
void MeleePc_UnregisterAssetHeap(const void* low, const void* high);
void MeleePc_UnregisterAssetRegion(const void* base);
int MeleePc_AssetBytesRemaining(const void* pointer, uint32_t* remaining);
/* Copy immutable pre-conversion bytes for one exact registered region.
 * Snapshot lifetime ends with the region/owner/heap unregister operation. */
int MeleePc_PreserveAssetBytes(const void* owner,const void* base,
                              const void* original,uint32_t size);
/* Bounded original view; outputs remain unchanged on failure. */
int MeleePc_OriginalAssetBytes(const void* pointer,uint32_t size,
                             const void** original);
/* Exact registered region identity; outputs remain unchanged on failure. */
int MeleePc_AssetRegionInfo(const void* pointer, const void** base,
                          const void** owner, uint32_t* size);
#endif
