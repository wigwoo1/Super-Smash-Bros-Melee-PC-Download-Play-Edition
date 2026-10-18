#include "reset_host.h"
#include "callback_gate.h"
#include <stdatomic.h>
static MeleePc_ResetHandler reset_handler;
static void* reset_context;
static _Atomic(u32) boot_reset_code;
int MeleePc_InstallResetHost(MeleePc_ResetHandler handler,void* context,u32 boot_code)
{
    if(!MeleePc_IsCallbackOwner() || !handler || reset_handler)return 0;
    reset_handler=handler;reset_context=context;
    atomic_store_explicit(&boot_reset_code,boot_code,memory_order_release);
    return 1;
}
u32 OSGetResetCode(void)
{
    return atomic_load_explicit(&boot_reset_code,memory_order_acquire);
}
void OSResetSystem(int kind,u32 reset_code,BOOL force_menu)
{
    if(!MeleePc_IsCallbackOwner())OSPanic(__FILE__,__LINE__,"Native reset requires host owner thread");
    if(kind<OS_RESET_RESTART || kind>OS_RESET_SHUTDOWN)OSPanic(__FILE__,__LINE__,"Invalid native reset kind %d",kind);
    if(force_menu)OSPanic(__FILE__,__LINE__,"Console system-menu reset is unavailable on native host");
    if(!reset_handler)OSPanic(__FILE__,__LINE__,"Native reset host is not installed");
    reset_handler(kind,reset_code,reset_context);
    OSPanic(__FILE__,__LINE__,"Native reset host returned without restarting or exiting");
}
