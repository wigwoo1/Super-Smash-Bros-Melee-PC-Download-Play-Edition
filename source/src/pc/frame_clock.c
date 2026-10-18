#include "frame_clock.h"
#define NS_PER_SECOND UINT64_C(1000000000)
static int valid_rate(uint32_t rate) { return rate == 60 || rate == 120 || rate == 240; }
int MeleePc_FrameClockInit(MeleePc_FrameClock* clock, uint32_t hz)
{
    if (!clock || !valid_rate(hz)) return 0;
    clock->presentation_hz = hz;
    clock->simulation_phase = clock->presentation_phase = 0;
    return 1;
}
static uint64_t due(uint64_t elapsed, uint32_t hz, uint64_t* phase)
{
    /* Split seconds first so even UINT64_MAX nanoseconds cannot overflow. */
    uint64_t fraction = (elapsed % NS_PER_SECOND) * hz + *phase;
    uint64_t steps = (elapsed / NS_PER_SECOND) * hz + fraction / NS_PER_SECOND;
    *phase = fraction % NS_PER_SECOND;
    return steps;
}
int MeleePc_FrameClockAdvance(MeleePc_FrameClock* clock, uint64_t elapsed,
                             uint64_t* simulation, uint64_t* presentation)
{
    if (!clock || !simulation || !presentation || simulation == presentation ||
        !valid_rate(clock->presentation_hz) ||
        clock->simulation_phase >= NS_PER_SECOND ||
        clock->presentation_phase >= NS_PER_SECOND) return 0;
    *simulation = due(elapsed, 60, &clock->simulation_phase);
    *presentation = due(elapsed, clock->presentation_hz, &clock->presentation_phase);
    return 1;
}
int MeleePc_PresentationClockInit(MeleePc_PresentationClock* c,uint32_t hz)
{
    if(!c || !valid_rate(hz))return 0;
    MeleePc_PresentationClock next={0};
    if(!MeleePc_FrameClockInit(&next.phase,hz))return 0;
    *c=next;return 1;
}
int MeleePc_PresentationClockPoll(MeleePc_PresentationClock* c,uint64_t elapsed)
{
    uint64_t simulation,pictures;
    if(!c || elapsed<c->elapsed_ns)return 0;
    MeleePc_PresentationClock next=*c;
    if(!MeleePc_FrameClockAdvance(&next.phase,elapsed-c->elapsed_ns,&simulation,&pictures) ||
       pictures>UINT64_MAX-next.opportunities || pictures>UINT64_MAX-next.pending)return 0;
    /* PAD owns simulation debt; do not execute simulation from this count. */
    next.elapsed_ns=elapsed;next.opportunities+=pictures;next.pending+=pictures;
    *c=next;return 1;
}
int MeleePc_PresentationClockFinish(MeleePc_PresentationClock* c,uint64_t elapsed,int presented)
{
    if(!c || !c->pending || (presented!=0 && presented!=1))return 0;
    MeleePc_PresentationClock next=*c;
    if(!MeleePc_PresentationClockPoll(&next,elapsed) ||
       (presented && next.presented==UINT64_MAX) ||
       next.pending-(uint64_t)presented>UINT64_MAX-next.missed)return 0;
    next.presented+=(uint64_t)presented;
    next.missed+=next.pending-(uint64_t)presented;next.pending=0;
    *c=next;return 1;
}
