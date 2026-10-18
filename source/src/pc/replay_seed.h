#ifndef MELEE_PC_REPLAY_SEED_H
#define MELEE_PC_REPLAY_SEED_H
#include <stdint.h>
/* Strict wire/diagnostic decimal seed; failure leaves output untouched. */
static int MeleePc_ParseReplaySeed(const char* text,uint32_t* output)
{
    uint32_t value=0;
    if(!text || !*text || !output)return 0;
    for(;*text;text++){
        unsigned digit=(unsigned char)*text-'0';
        if(digit>9 || value>(UINT32_MAX-digit)/10)return 0;
        value=value*10+digit;
    }
    *output=value;return 1;
}
#endif
