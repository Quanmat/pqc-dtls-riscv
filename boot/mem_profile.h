#ifndef MEM_PROFILE_H
#define MEM_PROFILE_H

#include <stddef.h>
#include <stdint.h>
#include <wolfssl/wolfcrypt/settings.h>

/* --- Exposed Globals --- */
extern volatile size_t g_heap_peak;
extern volatile size_t g_heap_current;

/* --- WolfSSL Allocator Hooks --- */
void* TrackMalloc(size_t sz);
void  TrackFree(void* p);
void* TrackRealloc(void* p, size_t sz);

#endif // MEM_PROFILE_H
