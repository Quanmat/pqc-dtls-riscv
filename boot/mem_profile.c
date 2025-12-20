#include "mem_profile.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>        
#include <generated/mem.h> 

volatile size_t g_heap_current = 0;
volatile size_t g_heap_peak = 0;

/* 
 * HEAP TRACKER
 */
void* TrackMalloc(size_t sz) {
    size_t total_sz = sz + sizeof(size_t);
    size_t *ptr = (size_t *)malloc(total_sz);
    if (ptr) {
        *ptr = sz;
        g_heap_current += sz;
        if (g_heap_current > g_heap_peak) g_heap_peak = g_heap_current;
        return (void*)(ptr + 1); 
    }
    return NULL;
}

void TrackFree(void* p) {
    if (p) {
        size_t *ptr = (size_t *)p;
        size_t *header = ptr - 1;
        g_heap_current -= *header;
        free(header); 
    }
}

void* TrackRealloc(void* p, size_t sz) {
    void* new_p = TrackMalloc(sz);
    if (new_p && p) {
        memcpy(new_p, p, sz); 
        TrackFree(p);
    }
    return new_p;
}
