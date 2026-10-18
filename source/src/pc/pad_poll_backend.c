#include "pad_poll_backend.h"
int MeleePc_PadPollBackendInstall(MeleePc_PadPollBackend* backend,
    MeleePc_PadPollOwner owner, MeleePc_PadPollApply apply, void* userdata)
{
    if (!backend || !owner || !apply || backend->owner || !owner(userdata))
        return 0;
    backend->owner = owner;
    backend->apply = apply;
    backend->userdata = userdata;
    backend->rate = 0;
    backend->configured = backend->applying = 0;
    return 1;
}
int MeleePc_PadPollBackendRequest(MeleePc_PadPollBackend* backend, unsigned rate)
{
    int accepted;
    if (!backend || !backend->owner || !backend->apply || rate > 11 ||
        !backend->owner(backend->userdata) || backend->applying) return 0;
    backend->applying = 1;
    accepted = backend->apply(backend->userdata, rate);
    backend->applying = 0;
    if (!accepted) return 0;
    backend->rate = rate;
    backend->configured = 1;
    return 1;
}
int MeleePc_PadPollBackendRate(MeleePc_PadPollBackend* backend, unsigned* rate)
{
    if (!backend || !rate || !backend->owner || !backend->configured ||
        !backend->owner(backend->userdata) || backend->applying) return 0;
    *rate = backend->rate;
    return 1;
}
