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
extern EbHandle eb_vp9_create_thread(
    void *thread_function(void *),
    void *thread_context);
extern EbErrorType eb_start_thread(
    EbHandle thread_handle);
extern EbErrorType eb_stop_thread(
    EbHandle thread_handle);
extern EbErrorType eb_vp9_destroy_thread(
    EbHandle thread_handle);

/**************************************
 * Semaphores
 **************************************/
extern EbHandle eb_vp9_create_semaphore(
    uint32_t initial_count,
    uint32_t max_count);
extern EbErrorType eb_vp9_post_semaphore(
    EbHandle semaphore_handle);
extern EbErrorType eb_vp9_block_on_semaphore(
    EbHandle semaphore_handle);
extern EbErrorType eb_vp9_destroy_semaphore(
    EbHandle semaphore_handle);
/**************************************
 * Mutex
 **************************************/
extern EbHandle eb_vp9_create_mutex(
    void);
extern EbErrorType eb_vp9_release_mutex(
    EbHandle mutex_handle);
extern EbErrorType eb_vp9_block_on_mutex(
    EbHandle mutex_handle);
extern EbErrorType eb_vp9_block_on_mutex_timeout(
    EbHandle mutex_handle,
    uint32_t timeout);
extern EbErrorType eb_vp9_destroy_mutex(
    EbHandle mutex_handle);

extern    EbMemoryMapEntry *memory_map;                // library Memory table
extern    uint32_t         *memory_map_index;          // library memory index
extern    uint64_t         *total_lib_memory;          // library Memory malloc'd

#ifdef _WIN32

#define EB_CREATETHREAD(type, pointer, n_elements, pointer_class, thread_function, thread_context) \
    pointer = eb_vp9_create_thread(thread_function, thread_context); \
    if (pointer == (type)EB_NULL) { \
        return EB_ErrorInsufficientResources; \
    } \
    else { \
        memory_map[*(memory_map_index)].ptr_type = pointer_class; \
        memory_map[(*(memory_map_index))++].ptr = pointer; \
        if (n_elements % 8 == 0) { \
            *total_lib_memory += (n_elements); \
        } \
        else { \
            *total_lib_memory += ((n_elements) + (8 - ((n_elements) % 8))); \
        } \
        if (eb_vp9_num_groups == 2 && eb_vp9_alternate_groups){ \
            eb_vp9_group_affinity.Group = 1 - eb_vp9_group_affinity.Group; \
            SetThreadGroupAffinity(pointer,&eb_vp9_group_affinity,NULL); \
        } \
        else if (eb_vp9_num_groups == 2 && !eb_vp9_alternate_groups){ \
            SetThreadGroupAffinity(pointer,&eb_vp9_group_affinity,NULL); \
        } \
    } \
    if (*(memory_map_index) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
    } \
    lib_thread_count++;
#elif defined(__linux__)
#define EB_CREATETHREAD(type, pointer, n_elements, pointer_class, thread_function, thread_context) \
    pointer = eb_vp9_create_thread(thread_function, thread_context); \
    if (pointer == (type)EB_NULL) { \
        return EB_ErrorInsufficientResources; \
    } \
    else { \
        pthread_setaffinity_np(*((pthread_t*)pointer),sizeof(cpu_set_t),&eb_vp9_group_affinity); \
        memory_map[*(memory_map_index)].ptr_type = pointer_class; \
        memory_map[(*(memory_map_index))++].ptr = pointer; \
        if (n_elements % 8 == 0) { \
            *total_lib_memory += (n_elements); \
        } \
        else { \
            *total_lib_memory += ((n_elements) + (8 - ((n_elements) % 8))); \
        } \
    } \
    if (*(memory_map_index) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
    } \
    lib_thread_count++;
#else
#define EB_CREATETHREAD(type, pointer, n_elements, pointer_class, thread_function, thread_context) \
    pointer = eb_vp9_create_thread(thread_function, thread_context); \
    if (pointer == (type)EB_NULL) { \
        return EB_ErrorInsufficientResources; \
    } \
   else { \
        memory_map[*(memory_map_index)].ptr_type = pointer_class; \
        memory_map[(*(memory_map_index))++].ptr = pointer; \
        if (n_elements % 8 == 0) { \
            *total_lib_memory += (n_elements); \
        } \
        else { \
            *total_lib_memory += ((n_elements) + (8 - ((n_elements) % 8))); \
        } \
    } \
    if (*(memory_map_index) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
    } \
    lib_thread_count++;
#endif

#ifdef __cplusplus
}
#endif
#endif // EbThreads_h
