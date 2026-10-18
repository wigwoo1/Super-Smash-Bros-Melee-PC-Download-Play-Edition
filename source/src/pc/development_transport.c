/* The native host has no GameCube EXI USB/MCC development server.
 * DVD/ARAM DevCom asset transfers remain in the original devcom.c. */
#include <sysdolphin/baselib/hsd_392C.h>
#include <sysdolphin/baselib/hsd_3933.h>
#include <dolphin/os.h>
#include <string.h>
s32 hsd_804CF740[16];
static int reporting_level;
bool hsd_803931A4(s32 channel)
{
    (void)channel;memset(hsd_804CF740,0,sizeof(hsd_804CF740));
    OSReport("Native host: console USB development server unavailable\n");
    return false;
}
int hsd_80392E80(void){return 0;} /* Empty event queue, not connected. */
bool hsd_80393A04(void){return false;}
void hsd_80393A54(int level){reporting_level=level;}
s32 hsd_80393328(void){return 0;} /* No remote handshake succeeded. */
int hsd_80393A5C(char* filename,void* data,int size)
{
    (void)filename;(void)data;(void)size;(void)reporting_level;
    return -5; /* Original disconnected/not-ready result, no file opened. */
}
