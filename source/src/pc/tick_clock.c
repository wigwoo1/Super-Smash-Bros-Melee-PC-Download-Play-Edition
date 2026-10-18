#include "tick_clock.h"
#include <limits.h>
int MeleePc_NanosecondsToTicks(uint64_t elapsed_ns,uint32_t rate,int64_t* ticks) {
    const uint64_t billion=UINT64_C(1000000000);
    uint64_t seconds=elapsed_ns/billion,remainder=elapsed_ns%billion,whole,fraction;
    if(!ticks || !rate || seconds>(uint64_t)INT64_MAX/rate)return 0;
    whole=seconds*rate;
    /* remainder < 1e9 and rate <= UINT32_MAX, so this product fits uint64. */
    fraction=remainder*rate/billion;
    if(fraction>(uint64_t)INT64_MAX-whole)return 0;
    *ticks=(int64_t)(whole+fraction);return 1;
}
int MeleePc_TicksToNanosecondsCeil(int64_t ticks,uint32_t rate,uint64_t* ns) {
    const uint64_t billion=UINT64_C(1000000000);
    uint64_t seconds,remainder,whole,fraction;
    if(!ns || !rate || ticks<0)return 0;
    seconds=(uint64_t)ticks/rate;remainder=(uint64_t)ticks%rate;
    if(seconds>UINT64_MAX/billion)return 0;
    whole=seconds*billion;fraction=(remainder*billion+rate-1)/rate;
    if(fraction>UINT64_MAX-whole)return 0;
    *ns=whole+fraction;return 1;
}
