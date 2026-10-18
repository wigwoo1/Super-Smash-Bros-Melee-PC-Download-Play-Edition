#include "pad_sampling.h"
#include <stdio.h>
#include <stdlib.h>
static MeleePc_PadPollBackend backend;
int MeleePc_InstallPadSampling(MeleePc_PadPollOwner owner,
    MeleePc_PadPollApply apply, void* userdata)
{
    return MeleePc_PadPollBackendInstall(&backend, owner, apply, userdata);
}
int MeleePc_PadSamplingRate(unsigned* rate)
{
    return MeleePc_PadPollBackendRate(&backend, rate);
}
void PADSetSamplingRate(unsigned long rate)
{
    if (rate > 11 || !MeleePc_PadPollBackendRequest(&backend, (unsigned)rate)) {
        fputs("Native PADSetSamplingRate: invalid rate, unavailable backend, or rejected owner-thread request\n", stderr);
        abort();
    }
}
