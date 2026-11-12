// Forced into the PS5 FFmpeg build only. Do not rely on a foreign/default
// pthread stack for codec workers; keep existing non-null attributes untouched.
#pragma once
#ifndef __ASSEMBLER__
#include <pthread.h>
static inline int pstubeCodecThread(pthread_t* thread,const pthread_attr_t* attributes,void* (*entry)(void*),void* context) {
    if(attributes) return pthread_create(thread,attributes,entry,context);
    pthread_attr_t local;int rc=pthread_attr_init(&local);if(rc) return rc;
    rc=pthread_attr_setstacksize(&local,2*1024*1024);
    if(!rc) rc=pthread_create(thread,&local,entry,context);
    pthread_attr_destroy(&local);return rc;
}
#define pthread_create pstubeCodecThread

#endif
