#include "../frame_clock.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    for(unsigned hz=60;hz<=240;hz*=2) {
        MeleePc_PresentationClock c;
        assert(MeleePc_PresentationClockInit(&c,hz));
        // Simulation backlog can hold every opportunity; one current image
        // consumes the interval without a burst of obsolete presents.
        assert(MeleePc_PresentationClockPoll(&c,1000000000));
        assert(c.pending==hz && c.opportunities==hz);
        assert(MeleePc_PresentationClockFinish(&c,1100000000,1));
        assert(c.pending==0 && c.presented==1 && c.missed==hz+hz/10-1);
        assert(c.opportunities==c.presented+c.missed);
        assert(MeleePc_PresentationClockPoll(&c,1100000000) && c.pending==0);
        MeleePc_PresentationClock before=c;
        assert(!MeleePc_PresentationClockPoll(&c,1) && !memcmp(&c,&before,sizeof(c)));
        assert(!MeleePc_PresentationClockFinish(&c,1100000000,1));
        // A surface/resource refusal skips only pictures, preserving phase.
        assert(MeleePc_PresentationClockPoll(&c,2000000000) && c.pending);
        assert(MeleePc_PresentationClockFinish(&c,2000000000,0));
        assert(c.presented==1 && c.missed==hz*2-1 && c.pending==0);
        // Fine-grained polling remains phase-exact over one full second.
        assert(MeleePc_PresentationClockInit(&c,hz));
        for(unsigned ms=1;ms<=1000;++ms) {
            assert(MeleePc_PresentationClockPoll(&c,(uint64_t)ms*1000000));
            if(c.pending)assert(MeleePc_PresentationClockFinish(&c,(uint64_t)ms*1000000,1));
        }
        assert(c.presented==hz && c.opportunities==hz && c.missed==0);
    }
    assert(!MeleePc_PresentationClockInit(NULL,120));
    MeleePc_PresentationClock c;
    assert(!MeleePc_PresentationClockInit(&c,90));
    assert(MeleePc_PresentationClockInit(&c,240));
    assert(MeleePc_PresentationClockPoll(&c,UINT64_MAX));
    assert(MeleePc_PresentationClockFinish(&c,UINT64_MAX,1));
    assert(c.opportunities==c.presented+c.missed);
    puts("PASS: presentation phase, backlog coalescing, render cost, deferred surface, no stale burst and monotonic extremes");
    return 0;
}
