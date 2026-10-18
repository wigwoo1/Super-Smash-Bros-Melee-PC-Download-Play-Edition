#ifndef MELEE_PC_VIDEO_CONFIG_H
#define MELEE_PC_VIDEO_CONFIG_H
#include <sysdolphin/baselib/video.h>
/* Requires an initialized Aurora host. Configures progressive/non-interlaced
 * camera state and Aurora dimensions; does not implement retrace or XFB queues. */
int MeleePc_ConfigureProgressiveVideo(const HSD_VIStatus* status);
#endif
