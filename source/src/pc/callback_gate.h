#ifndef MELEE_PC_CALLBACK_GATE_H
#define MELEE_PC_CALLBACK_GATE_H
#include <dolphin/os.h>
typedef int (*MeleePc_OwnerCheck)(void);
int MeleePc_InstallCallbackOwner(MeleePc_OwnerCheck check);
int MeleePc_CallbacksEnabled(void);
/* Safe owner query, including before installation. */
int MeleePc_IsCallbackOwner(void);
/* Explicit native host request, not a polled GameCube hardware switch. */
void MeleePc_SetResetSwitchRequest(int pressed);
#endif
