#ifndef MELEE_PC_HOST_LIFECYCLE_H
#define MELEE_PC_HOST_LIFECYCLE_H
/* Owner thread only; one active session cleanup. Data must remain alive until
 * unregister or terminal exit. Exit detaches before invoking to avoid repeats. */
int MeleePc_RegisterSessionCleanup(void (*cleanup)(void*),void* data);
int MeleePc_UnregisterSessionCleanup(void (*cleanup)(void*),void* data);
#endif
