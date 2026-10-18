#ifndef MELEE_PC_THREAD_DIAGNOSTICS_H
#define MELEE_PC_THREAD_DIAGNOSTICS_H
#include <stdint.h>
int MeleePc_CurrentStackBounds(uintptr_t* low,uintptr_t* high);
void MeleePc_PrintThreadInfo(void);
#endif
