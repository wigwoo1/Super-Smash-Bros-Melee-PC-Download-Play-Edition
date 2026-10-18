#ifndef MELEE_PC_RESET_HOST_H
#define MELEE_PC_RESET_HOST_H
#include <dolphin/os.h>
/* Called on the registered owner. The host must tear down and restart/exit
 * without returning. Restart boot code is provided before game startup. */
typedef void (*MeleePc_ResetHandler)(int kind,u32 reset_code,void* context);
int MeleePc_InstallResetHost(MeleePc_ResetHandler handler,void* context,u32 boot_code);
#endif
