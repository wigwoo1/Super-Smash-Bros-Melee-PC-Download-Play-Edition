#include "pad_poll_policy.h"
int MeleePc_PadPollPolicyDeadline(const MeleePc_PadPollPolicy* policy,
                                uint64_t* deadline_ns)
{
    if (!policy || !deadline_ns || !policy->interval_ns ||
        policy->last_ns > UINT64_MAX - policy->interval_ns) return 0;
    *deadline_ns = policy->last_ns + policy->interval_ns;
    return 1;
}
int MeleePc_PadPollPolicyInit(MeleePc_PadPollPolicy* policy,
                            unsigned rate, uint64_t now_ns)
{
    if (!policy || rate > 11) return 0;
    policy->last_ns = now_ns;
    policy->interval_ns = (uint64_t) (rate ? rate : 1) * 1000000;
    return 1;
}
int MeleePc_PadPollPolicyService(MeleePc_PadPollPolicy* policy,
                               uint64_t now_ns)
{
    if (!policy || !policy->interval_ns || now_ns < policy->last_ns)
        return -1;
    if (now_ns - policy->last_ns < policy->interval_ns) return 0;
    policy->last_ns = now_ns;
    return 1;
}
