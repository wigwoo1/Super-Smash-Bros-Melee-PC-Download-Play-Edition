#ifndef MELEE_PC_PAD_SAMPLING_H
#define MELEE_PC_PAD_SAMPLING_H
#include "pad_poll_backend.h"
/* Install before game startup can call PADSetSamplingRate. */
int MeleePc_InstallPadSampling(MeleePc_PadPollOwner owner,
    MeleePc_PadPollApply apply, void* userdata);
int MeleePc_PadSamplingRate(unsigned* rate);
/* Original SDK ABI: unsigned long is 32-bit in the native Win32 target. */
void PADSetSamplingRate(unsigned long rate);
#endif
