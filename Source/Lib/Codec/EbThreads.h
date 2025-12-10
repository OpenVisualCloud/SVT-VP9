/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbThreads_h
#define EbThreads_h

#include "EbDefinitions.h"
#include "EbDefinitions.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
// Create wrapper functions that hide thread calls,
// semaphores, mutex, etc. These wrappers also hide
// platform specific implementations of these objects.

/**************************************
 * Threads
 **************************************/
extern EbHandle    eb_vp9_create_thread(void *thread_function(void *), void *thread_context);
extern EbErrorType eb_vp9_destroy_thread(EbHandle thread_handle);

/**************************************
 * Semaphores
 **************************************/
extern EbHandle    eb_vp9_create_semaphore(uint32_t initial_count, uint32_t max_count);
extern EbErrorType eb_vp9_post_semaphore(EbHandle semaphore_handle);
extern EbErrorType eb_vp9_block_on_semaphore(EbHandle semaphore_handle);
extern EbErrorType eb_vp9_destroy_semaphore(EbHandle semaphore_handle);
/**************************************
 * Mutex
 **************************************/
extern EbHandle    eb_vp9_create_mutex(void);
extern EbErrorType eb_vp9_release_mutex(EbHandle mutex_handle);
extern EbErrorType eb_vp9_block_on_mutex(EbHandle mutex_handle);
extern EbErrorType eb_vp9_destroy_mutex(EbHandle mutex_handle);

extern EbMemoryMapEntry *memory_map; // library Memory table
extern uint32_t         *memory_map_index; // library memory index
extern uint64_t         *total_lib_memory; // library Memory malloc'd

#ifdef __cplusplus
}
#endif
#endif // EbThreads_h
