#include "video_config.h"
#include <sysdolphin/baselib/debug.h>
#include <dolphin/vi.h>
static int configured;
int MeleePc_ConfigureProgressiveVideo(const HSD_VIStatus* status)
{
    unsigned scan;
    if (!status) return 0;
    scan = (unsigned) status->rmode.viTVmode & 3;
    if ((scan != VI_PROGRESSIVE && scan != VI_NON_INTERLACE) ||
        status->rmode.field_rendering || !status->rmode.fbWidth ||
        !status->rmode.efbHeight || !status->rmode.viWidth ||
        !status->rmode.viHeight || !status->rmode.xfbHeight) return 0;
    HSD_VIData.current.vi = *status;
    VIConfigure(&HSD_VIData.current.vi.rmode);
    configured = 1;
    return 1;
}
u32 VIGetNextField(void)
{
    if (!configured || HSD_VIData.current.vi.rmode.field_rendering)
        HSD_Panic(__FILE__, __LINE__, "native progressive video is not configured");
    /* Progressive frames have one field; never fabricate an interlaced counter. */
    return 0;
}
