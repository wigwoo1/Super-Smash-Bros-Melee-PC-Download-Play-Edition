#include "callback_gate.h"
#include <stdlib.h>
#include <dolphin/os.h>
static MeleePc_OwnerCheck owner;
static BOOL enabled=TRUE,reset_pressed=FALSE;
int MeleePc_IsCallbackOwner(void){return owner && owner();}
static void require_owner(void){
    if(!MeleePc_IsCallbackOwner())
        OSPanic(__FILE__,__LINE__,"Native callback/interrupt operation outside installed owner thread");
}
int MeleePc_InstallCallbackOwner(MeleePc_OwnerCheck check){if(!check || owner || !check())return 0;owner=check;return 1;}
int MeleePc_CallbacksEnabled(void){require_owner();return enabled;}
BOOL OSDisableInterrupts(void){require_owner();BOOL previous=enabled;enabled=FALSE;return previous;}
BOOL OSEnableInterrupts(void){require_owner();BOOL previous=enabled;enabled=TRUE;return previous;}
BOOL OSRestoreInterrupts(BOOL state){require_owner();BOOL previous=enabled;enabled=state?TRUE:FALSE;return previous;}
BOOL OSGetResetSwitchState(void){require_owner();return reset_pressed;}
void MeleePc_SetResetSwitchRequest(int pressed){require_owner();reset_pressed=pressed?TRUE:FALSE;}
