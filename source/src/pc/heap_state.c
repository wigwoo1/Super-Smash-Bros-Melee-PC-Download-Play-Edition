#include <sysdolphin/baselib/initialize.h>
/* Shared with native startup; this module has no audio/video dependencies. */
volatile OSHeapHandle MeleePc_CurrentHeap = -1;
OSHeapHandle HSD_GetHeap(void) { return MeleePc_CurrentHeap; }
void HSD_SetHeap(OSHeapHandle handle) { MeleePc_CurrentHeap = handle; }
