#ifndef MELEE_PC_VIDEO_SLOTS_H
#define MELEE_PC_VIDEO_SLOTS_H
#include <stdint.h>
#include <sysdolphin/baselib/video.h>
typedef struct {
    uint64_t id;
    unsigned active, submitted, completed, presented;
} MeleePc_VideoSlot;
typedef struct {
    HSD_VIInfo* vi;
    uint64_t generation, last_id, promoted;
    unsigned failed;
    MeleePc_VideoSlot slots[HSD_VI_XFB_MAX];
} MeleePc_VideoSlots;
/* Prototype native snapshot ownership adapter, not production HSD integration.
 * Zero initialize; owner thread only. Requires distinct GPU snapshot identities,
 * not CPU YUV buffers. No callbacks, retrace fabrication or device reset.
 * Init once; supports2/3slots, keeps current VI configuration. */
int MeleePc_VideoSlotsInit(MeleePc_VideoSlots*, HSD_VIInfo*, uint64_t generation,
                          void* const* identities, unsigned count);
/* Returns FREE slot claimed as DRAWING, or-1. IDs strictly increase. */
int MeleePc_VideoSlotAcquire(MeleePc_VideoSlots*, uint64_t generation, uint64_t id);
/* Owner-only read-only capacity check before begin_frame. Does not reserve a
 * resource or predict an ID. Acquire must still succeed before snapshot use. */
int MeleePc_VideoSlotsAvailable(const MeleePc_VideoSlots*, uint64_t generation);
/* Owner may release an acquired slot ONLY before recording any GPU command
 * that references its identity. Not GPU cancellation. Submitted slots and
 * failed queues cannot be released. The consumed ID is never reusable. */
int MeleePc_VideoSlotReleaseUnencoded(MeleePc_VideoSlots*, uint64_t generation,
                                     uint64_t id);
int MeleePc_VideoSlotSubmit(MeleePc_VideoSlots*, uint64_t generation, uint64_t id);
/* kind1=GPUcomplete,2=surfaceAPIacceptance; status0=success,1=failure.
 * GPUcompletion must cover all snapshot/presentation resource uses. Failures
 * latch globally and forbid further acquire/promotions; never clear to reset.
 * Both successful signals required for ordered DISPLAY promotion. */
int MeleePc_VideoSlotAck(MeleePc_VideoSlots*, uint64_t generation, uint64_t id,
                        unsigned kind, unsigned status);
#endif
