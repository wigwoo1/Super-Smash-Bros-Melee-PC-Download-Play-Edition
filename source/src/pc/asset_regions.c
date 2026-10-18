#include "asset_regions.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
/* Original preload cache has 80 slots, plus resident archives. */
#define REGION_COUNT 256
static struct { const void* base; const void* owner; uint32_t size; unsigned char* original; } regions[REGION_COUNT];
static void clear_region(int i){
    free(regions[i].original);memset(&regions[i],0,sizeof(regions[i]));
}
int MeleePc_PreserveAssetBytes(const void* owner,const void* base,const void* original,uint32_t size){
    if(!owner||!base||!original||!size)return 0;
    for(int i=0;i<REGION_COUNT;i++)if(regions[i].base==base){
        if(regions[i].owner!=owner||regions[i].size!=size||regions[i].original)return 0;
        unsigned char* copy=malloc(size);if(!copy)return 0;
        memcpy(copy,original,size);regions[i].original=copy;return 1;
    }
    return 0;
}
int MeleePc_OriginalAssetBytes(const void* pointer,uint32_t size,const void** original){
    uintptr_t at=(uintptr_t)pointer;
    if(!pointer||!original||!size)return 0;
    for(int i=0;i<REGION_COUNT;i++){
        uintptr_t base=(uintptr_t)regions[i].base;
        if(regions[i].base&&regions[i].original&&at>=base&&at-base<regions[i].size&&size<=regions[i].size-(at-base)){
            *original=regions[i].original+(at-base);return 1;
        }
    }
    return 0;
}
int MeleePc_RegisterAssetRegion(const void* base, uint32_t size) {
    return MeleePc_RegisterOwnedAssetRegion(base,base,size);
}
int MeleePc_RegisterOwnedAssetRegion(const void* owner, const void* base, uint32_t size) {
    uintptr_t at=(uintptr_t)base; int i, free_slot=-1;
    if(!owner || !base || !size || at>UINTPTR_MAX-size) return 0;
    for(i=0;i<REGION_COUNT;i++) {
        uintptr_t other=(uintptr_t)regions[i].base;
        if(!regions[i].base) { if(free_slot<0)free_slot=i; continue; }
        if(at<other+regions[i].size && other<at+size)return 0;
    }
    if(free_slot<0)return 0;
    regions[free_slot].base=base;regions[free_slot].owner=owner;regions[free_slot].size=size;return 1;
}
void MeleePc_UnregisterAssetRegion(const void* base) {
    int i; for(i=0;i<REGION_COUNT;i++)if(regions[i].base==base) {
        clear_region(i);return;
    }
}
void MeleePc_UnregisterAssetAllocation(const void* owner) {
    int i;
    if(!owner)return;
    for(i=0;i<REGION_COUNT;i++)if(regions[i].owner==owner) {
        clear_region(i);
    }
}
int MeleePc_AssetAllocationRegistered(const void* owner) {
    int i;
    if(!owner)return 0;
    for(i=0;i<REGION_COUNT;i++)if(regions[i].owner==owner)return 1;
    return 0;
}
void MeleePc_UnregisterAssetHeap(const void* low, const void* high) {
    uintptr_t lo=(uintptr_t)low,hi=(uintptr_t)high;int i;
    if(!low || lo>=hi)return;
    for(i=0;i<REGION_COUNT;i++) {
        uintptr_t owner=(uintptr_t)regions[i].owner;
        if(regions[i].base && owner>=lo && owner<hi) {
            clear_region(i);
        }
    }
}
int MeleePc_AssetBytesRemaining(const void* pointer, uint32_t* remaining) {
    uintptr_t at=(uintptr_t)pointer;int i;
    if(!pointer || !remaining)return 0;
    for(i=0;i<REGION_COUNT;i++) {
        uintptr_t base=(uintptr_t)regions[i].base;
        if(regions[i].base && at>=base && at-base<regions[i].size) {
            *remaining=regions[i].size-(uint32_t)(at-base);return 1;
        }
    }
    return 0;
}
int MeleePc_AssetRegionInfo(const void* pointer,const void** base_out,
                          const void** owner_out,uint32_t* size_out) {
    uintptr_t at=(uintptr_t)pointer;int i;
    if(!pointer || !base_out || !owner_out || !size_out)return 0;
    for(i=0;i<REGION_COUNT;i++) {
        uintptr_t base=(uintptr_t)regions[i].base;
        if(regions[i].base && at>=base && at-base<regions[i].size) {
            *base_out=regions[i].base;*owner_out=regions[i].owner;
            *size_out=regions[i].size;return 1;
        }
    }
    return 0;
}
