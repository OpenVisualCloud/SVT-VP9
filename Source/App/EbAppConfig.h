/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbAppConfig_h
#define EbAppConfig_h

#include <stdio.h>

#include "EbSvtVp9Enc.h"

// Define Cross-Platform 64-bit fseek() and ftell()
#ifdef _WIN32
typedef __int64 Off64;
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#elif defined(__GNUC__)
#define fseeko64 fseek
#define ftello64 ftell
#endif

#ifndef _RSIZE_T_DEFINED
typedef size_t rsize_t;
#define _RSIZE_T_DEFINED
#endif /* _RSIZE_T_DEFINED */

#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
typedef int32_t errno_t;
#endif  /* _ERRNO_T_DEFINED */

/** The AppExitConditionType type is used to define the App main loop exit
conditions.
*/
typedef enum AppExitConditionType
{
    APP_ExitConditionNone = 0,
    APP_ExitConditionFinished,
    APP_ExitConditionError
} AppExitConditionType;

/** The AppPortActiveType type is used to define the state of output ports in
the App.
*/
typedef enum AppPortActiveType
{
    APP_PortActive = 0,
    APP_PortInactive
} AppPortActiveType;

/** The EB_PTR type is intended to be used to pass pointers to and from the svt
API.  This is a 32 bit pointer and is aligned on a 32 bit word boundary.
*/
typedef void * EB_PTR;

/** The EB_NULL type is used to define the C style NULL pointer.
*/
#define EB_NULL ((void*) 0)

// memory map to be removed and replaced by malloc / free
typedef enum EbPtrType
{
    EB_N_PTR = 0,                                   // malloc'd pointer
    EB_A_PTR = 1,                                   // malloc'd pointer aligned
    EB_MUTEX = 2,                                   // mutex
    EB_SEMAPHORE = 3,                               // semaphore
    EB_THREAD = 4                                   // thread handle
}EbPtrType;
typedef void * EbPtr;
typedef struct EbMemoryMapEntry
{
    EbPtr                     ptr;                   // points to a memory pointer
    EbPtrType                 ptr_type;              // pointer type
} EbMemoryMapEntry;

extern    EbMemoryMapEntry *app_memory_map;          // App Memory table
extern    uint32_t         *app_memory_map_index;    // App Memory index
extern    uint64_t         *total_app_memory;        // App Memory malloc'd
extern    uint32_t          app_malloc_count;

#define MAX_APP_NUM_PTR                             (0x186A0 << 2)             // Maximum number of pointers to be allocated for the app

#define EB_APP_MALLOC(type, pointer, nElements, pointerClass, returnType) \
    pointer = (type)malloc(nElements); \
    if (pointer == (type)EB_NULL){ \
        return returnType; \
            } \
                else { \
        app_memory_map[*(app_memory_map_index)].ptr_type = pointerClass; \
        app_memory_map[(*(app_memory_map_index))++].ptr = pointer; \
        if (nElements % 8 == 0) { \
            *total_app_memory += (nElements); \
                        } \
                                else { \
            *total_app_memory += ((nElements) + (8 - ((nElements) % 8))); \
            } \
        } \
    if (*(app_memory_map_index) >= MAX_APP_NUM_PTR) { \
        return returnType; \
                } \
    app_malloc_count++;

#define EB_APP_MALLOC_NR(type, pointer, nElements, pointerClass,returnType) \
    (void)returnType; \
    pointer = (type)malloc(nElements); \
    if (pointer == (type)EB_NULL){ \
        returnType = EB_ErrorInsufficientResources; \
        SVT_LOG("Malloc has failed due to insuffucient resources"); \
        return; \
            } \
                else { \
        app_memory_map[*(app_memory_map_index)].ptrType = pointerClass; \
        app_memory_map[(*(app_memory_map_index))++].ptr = pointer; \
        if (nElements % 8 == 0) { \
            *total_app_memory += (nElements); \
                        } \
                                else { \
            *total_app_memory += ((nElements) + (8 - ((nElements) % 8))); \
            } \
        } \
    if (*(app_memory_map_index) >= MAX_APP_NUM_PTR) { \
        returnType = EB_ErrorInsufficientResources; \
        SVT_LOG("Malloc has failed due to insuffucient resources"); \
        return; \
                } \
    app_malloc_count++;

/* string copy */
extern errno_t strcpy_ss(char *dest, rsize_t dmax, const char *src);

/* fitted string copy */
extern errno_t strncpy_ss(char *dest, rsize_t dmax, const char *src, rsize_t slen);

/* string length */
extern rsize_t eb_vp9_strnlen_ss(const char *s, rsize_t smax);

#define EB_STRNCPY(dst, src, count) \
    strncpy_ss(dst, sizeof(dst), src, count)

#define EB_STRCPY(dst, size, src) \
    strcpy_ss(dst, size, src)

#define EB_STRCMP(target,token) \
    strcmp(target,token)

#define EB_STRLEN(target, max_size) \
    eb_vp9_strnlen_ss(target, max_size)

#define EB_APP_MEMORY() \
    SVT_LOG("Total Number of Mallocs in App: %d\n", app_malloc_count); \
    SVT_LOG("Total App Memory: %.2lf KB\n\n",*total_app_memory/(double)1024);

#define MAX_CHANNEL_NUMBER      6
#define MAX_NUM_TOKENS          200

#ifdef _WIN32
#define FOPEN(f,s,m) fopen_s(&f,s,m)
#else
#define FOPEN(f,s,m) f=fopen(s,m)
#endif

typedef struct EbPerformanceContext
{

    /****************************************
     * Computational Performance Data
     ****************************************/
    uint64_t  lib_start_time[2];       // [sec, micro_sec] including init time
    uint64_t  encode_start_time[2];    // [sec, micro_sec] first frame sent

    double    total_execution_time;    // includes init
    double    total_encode_time;       // not including init

    uint64_t  total_latency;
    uint32_t  max_latency;

    uint64_t  starts_time;
    uint64_t  start_utime;

    uint64_t  frame_count;

    double    average_speed;
    double    average_latency;

    uint64_t  byte_count;

}EbPerformanceContext;

typedef struct EbConfig
{
    /****************************************
     * File I/O
     ****************************************/
    FILE           *config_file;
    FILE           *input_file;
    FILE           *bitstream_file;
    FILE           *recon_file;
    FILE           *error_log_file;
    FILE           *qp_file;

    uint8_t         use_qp_file;
    int32_t         frame_rate;
    int32_t         frame_rate_numerator;
    int32_t         frame_rate_denominator;
    int32_t         injector_frame_rate;
    uint32_t        injector;
    uint32_t        speed_control_flag;
    uint32_t        encoder_bit_depth;
    uint32_t        source_width;
    uint32_t        source_height;

    int64_t         frames_to_be_encoded;
    int32_t         buffered_input;
    unsigned char **sequence_buffer;

    /*****************************************
     * Coding Structure
     *****************************************/
    uint32_t        base_layer_switch_mode;
    uint8_t         enc_mode;
    int32_t         intra_period;
    uint32_t        pred_structure;

    /****************************************
     * Quantization
     ****************************************/
    uint32_t        qp;

    /****************************************
     * Loop Filter
     ****************************************/
    uint32_t       loop_filter;

    /****************************************
     * ME Tools
     ****************************************/
    uint8_t        use_default_me_hme;
    uint8_t        enable_hme_flag;

    /****************************************
     * ME Parameters
     ****************************************/
    uint32_t       search_area_width;
    uint32_t       search_area_height;

    /****************************************
     * Rate Control
     ****************************************/
    uint32_t       rate_control_mode;
    uint32_t       target_bit_rate;
    uint32_t       max_qp_allowed;
    uint32_t       min_qp_allowed;
    uint32_t       vbv_max_rate;
    uint32_t       vbv_buf_size;
    /****************************************
    * TUNE
    ****************************************/
    uint8_t        tune;

    /****************************************
     * Annex A Parameters
     ****************************************/
    uint32_t       profile;
    uint32_t       level;

    /****************************************
     * On-the-fly Testing
     ****************************************/
    uint8_t        eos_flag;

    /****************************************
    * Optimization Type
    ****************************************/
    uint8_t        asm_type;

    /****************************************
     * Computational Performance Data
     ****************************************/
    EbPerformanceContext  performance_context;

    /****************************************
    * Instance Info
    ****************************************/
    uint32_t        channel_id;
    uint32_t        active_channel_count;
    int32_t         target_socket;
    uint32_t        logical_processors;
    uint8_t         stop_encoder;         // to signal CTRL+C Event, need to stop encoding.

    uint64_t        processed_frame_count;
    uint64_t        processed_byte_count;

} EbConfig;

extern void eb_config_ctor(EbConfig *config_ptr);
extern void eb_config_dtor(EbConfig *config_ptr);

extern EbErrorType read_command_line(
    int         argc,
    char *const argv[],
    EbConfig  **config,
    uint32_t    num_channels,
    EbErrorType *return_errors);

extern uint32_t get_help(int argc, char *const argv[]);
extern uint32_t get_svt_version(int argc, char *const argv[]);
extern uint32_t get_number_of_channels(int argc, char *const argv[]);

#endif //EbAppConfig_h
