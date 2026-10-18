#ifndef MELEE_PC_PAD_POLL_POLICY_H
#define MELEE_PC_PAD_POLL_POLICY_H
#include <stdint.h>
typedef struct {
    uint64_t last_ns;
    uint64_t interval_ns;
} MeleePc_PadPollPolicy;
/* Native policy: rate 0 requests 1 ms, otherwise rate ms (1..11).
 * This is an event-service target, not a hardware guarantee or game clock. */
int MeleePc_PadPollPolicyInit(MeleePc_PadPollPolicy* policy,
                            unsigned rate, uint64_t now_ns);
/* 1: service once now; 0: not due; -1: invalid/reversed time.
 * A late service resets the origin; it never invents historical reads. */
int MeleePc_PadPollPolicyService(MeleePc_PadPollPolicy* policy,
                               uint64_t now_ns);
/* Earliest next service time for a native wait. Returns zero for invalid state,
 * null output or overflow without changing either policy or output. */
int MeleePc_PadPollPolicyDeadline(const MeleePc_PadPollPolicy* policy,
                                uint64_t* deadline_ns);
#endif
