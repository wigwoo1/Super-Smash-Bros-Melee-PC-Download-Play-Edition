#ifndef MELEE_PC_PAD_POLL_BACKEND_H
#define MELEE_PC_PAD_POLL_BACKEND_H
typedef int (*MeleePc_PadPollOwner)(void* userdata);
/* Apply must preserve the previous request on failure. Acceptance describes
 * a configured service target, not achieved hardware latency. */
typedef int (*MeleePc_PadPollApply)(void* userdata, unsigned rate);
typedef struct {
    MeleePc_PadPollOwner owner;
    MeleePc_PadPollApply apply;
    void* userdata;
    unsigned rate;
    int configured;
    int applying;
} MeleePc_PadPollBackend;
/* Initialize to {0}. Install once on the owner thread. */
int MeleePc_PadPollBackendInstall(MeleePc_PadPollBackend* backend,
    MeleePc_PadPollOwner owner, MeleePc_PadPollApply apply, void* userdata);
int MeleePc_PadPollBackendRequest(MeleePc_PadPollBackend* backend, unsigned rate);
/* Missing/unconfigured/off-thread backend leaves output untouched. */
int MeleePc_PadPollBackendRate(MeleePc_PadPollBackend* backend, unsigned* rate);
#endif
