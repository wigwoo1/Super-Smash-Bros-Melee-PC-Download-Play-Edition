#ifndef MELEE_PC_GAME_PREFS_SESSION_H
#define MELEE_PC_GAME_PREFS_SESSION_H

#include <stdint.h>

/* Only original options, not card state, pointers, statistics or unlock flags.
 * Masks retain every bit, including choices currently hidden by stage menus. */
typedef struct {
    uint64_t item_mask;
    uint32_t stage_mask;
    uint8_t item_freq, rumble[4];
    int8_t sound_balance;
    uint8_t deflicker, saved_language;
} MeleePc_GamePrefsTransfer;

int MeleePc_GamePrefsCapture(MeleePc_GamePrefsTransfer* output);
int MeleePc_GamePrefsQueue(const MeleePc_GamePrefsTransfer* input);
/* Call after the no-save boot scene's final default-data reset, before A/V
 * configuration. Ordinary launches and subsequent resets have no queued data. */
void MeleePc_GamePrefsAfterBootstrap(void);
/* Opt-in same-build test fixture, consumed before spawning a replacement. */
int MeleePc_GamePrefsDiagnosticSeed(void);

#endif
