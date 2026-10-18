#include "objalloc.h"

#include <string.h>

#include "initialize.h"
#include "memory.h"
#include <dolphin/os/OSAlloc.h>
#ifdef TARGET_PC
#include <stdio.h>
#include <stdlib.h>
#endif

static objheap obj_heap = { 0, 0, -1, -1 };

static HSD_ObjAllocData* alloc_datas;

#ifdef TARGET_PC
void HSD_ObjAllocDiagnosticTrace(const char* phase)
{
    HSD_ObjAllocData* scan;
    unsigned count = 0;
    HSD_ObjAllocData* low_node = NULL;

    if (getenv("MELEE_DIAGNOSTIC_OBJALLOC_CHAIN") == NULL) {
        return;
    }
    scan = alloc_datas;
    fprintf(stderr, "NATIVE_OBJALLOC_CHECK phase=%s head=%p\n", phase,
            alloc_datas);
    while (scan != NULL && count < 32) {
        uintptr_t address = (uintptr_t) scan;
        if (address < 0x10000) {
            low_node = scan;
            break;
        }
        scan = scan->next;
        count++;
    }
    fprintf(stderr,
            "NATIVE_OBJALLOC_CHECK phase=%s head=%p nodes=%u low_node=%p next_not_read=1\n",
            phase, alloc_datas, count, low_node);
}
#endif

void HSD_ObjSetHeap(u32 size, void* ptr)
{
    obj_heap.curr = (uintptr_t) ptr;
    obj_heap.top = (uintptr_t) ptr;
    obj_heap.remain = size;
    obj_heap.size = size;
}

s32 HSD_ObjAllocAddFree(HSD_ObjAllocData* data, u32 num)
{
    uintptr_t pool_end;
    uintptr_t pool_size;
    uintptr_t pool_start;

    HSD_ASSERT(0xEE, data);
    pool_size = data->size * num;
    if (obj_heap.top != 0) {
        uintptr_t align = data->align;
        pool_end = obj_heap.top + obj_heap.size;
        pool_start = (obj_heap.curr + align) & ~align;
        if (pool_start > pool_end) {
            return 0;
        }
        if (pool_end - pool_start < pool_size) {
            pool_size =
                pool_end - pool_start - (pool_end - pool_start) % data->size;
        }
        num = pool_size / data->size;
        if (num == 0) {
            return 0;
        }
        obj_heap.curr = pool_start + pool_size;
        obj_heap.remain = pool_end - obj_heap.curr;
    } else {
        pool_start = (uintptr_t) HSD_MemAlloc(pool_size);
        if (pool_start == 0) {
            return 0;
        }
        obj_heap.remain -= pool_size;
    }

    {
        int i;
        HSD_ObjAllocLink* link;
        for (i = 0; (unsigned) i < num - 1; i++) {
            link = (HSD_ObjAllocLink*) (pool_start + data->size * i);
            link->next =
                (HSD_ObjAllocLink*) (pool_start + data->size * (i + 1));
        }
        link = (HSD_ObjAllocLink*) (pool_start + data->size * i);
        link->next = data->freehead;
    }

    data->freehead = (HSD_ObjAllocLink*) pool_start;
    data->free += num;
    return num;
}

void* HSD_ObjAlloc(HSD_ObjAllocData* data)
{
    HSD_ObjAllocLink* cur;
    u32 size;

    if (data->num_limit_flag && data->used >= data->num_limit) {
        return NULL;
    }
    if (data->heap_limit_flag) {
        if (data->heap_limit_num == (unsigned) -1) {
            if (obj_heap.top != 0) {
                size = obj_heap.remain;
            } else {
                size = OSCheckHeap(HSD_GetHeap());
            }
            if (size <= data->heap_limit_size) {
                data->heap_limit_num = data->used + data->free;
            }
        } else {
            if (obj_heap.top != 0) {
                size = obj_heap.remain;
            } else {
                size = OSCheckHeap(HSD_GetHeap());
            }
            if (size > data->heap_limit_size) {
                data->heap_limit_num = -1;
            }
        }
        if (data->used >= data->heap_limit_num) {
            return NULL;
        }
    }
    if (data->free == 0) {
        HSD_ObjAllocAddFree(data, 1);
        if (data->free == 0) {
            return NULL;
        }
    }
    cur = data->freehead;
    data->freehead = cur->next;
    data->used += 1;
    data->free -= 1;
    if (data->used > data->peak) {
        data->peak = data->used;
    }
    return cur;
}

void HSD_ObjFree(HSD_ObjAllocData* data, void* obj)
{
    HSD_ObjAllocLink* link = obj;
    link->next = data->freehead;
    data->freehead = link;
    data->free += 1;
    data->used -= 1;
}

static inline void removeAll(HSD_ObjAllocData* data)
{
    HSD_ObjAllocData** cur = &alloc_datas;
#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_OBJALLOC_CHAIN") != NULL) {
        HSD_ObjAllocData* scan = alloc_datas;
        unsigned count = 0;
        fprintf(stderr, "NATIVE_OBJALLOC_REMOVE request=%p head=%p\n", data,
                alloc_datas);
        while (scan != NULL && count < 32) {
            uintptr_t address = (uintptr_t) scan;
            if (address < 0x10000) {
                fprintf(stderr,
                        "NATIVE_OBJALLOC_CHAIN low_node=%p depth=%u next_not_read=1\n",
                        scan, count);
                break;
            }
            fprintf(stderr, "NATIVE_OBJALLOC_CHAIN node=%p next=%p depth=%u\n",
                    scan, scan->next, count);
            scan = scan->next;
            count++;
        }
    }
#endif
    while (*cur != NULL) {
        if (*cur == data) {
            *cur = (*cur)->next;
        } else {
            cur = &(*cur)->next;
        }
    }
}

void HSD_ObjAllocInit(HSD_ObjAllocData* data, size_t size, u32 align)
{
#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_OBJALLOC_CHAIN") != NULL &&
        ((uintptr_t) data < 0x10000 || (uintptr_t) alloc_datas < 0x10000)) {
        fprintf(stderr,
                "NATIVE_OBJALLOC_INIT low_pointer data=%p head=%p size=%zu align=%u\n",
                data, alloc_datas, size, align);
    }
#endif
    HSD_ASSERT(0x185, data);
    if (data != NULL) {
        removeAll(data);
    } else {
        alloc_datas = NULL;
    }
    memset(data, 0, sizeof(HSD_ObjAllocData));
    data->num_limit = -1;
    data->heap_limit_size = 0;
    data->heap_limit_num = -1;
    data->align = align - 1;
    data->size = (size + data->align) & ~data->align;
    data->next = alloc_datas;
    alloc_datas = data;
}

void _HSD_ObjAllocForgetMemory(void* low, void* high)
{
    alloc_datas = NULL;
}
