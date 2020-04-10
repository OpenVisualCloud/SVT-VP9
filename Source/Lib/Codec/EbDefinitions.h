/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbDefinitions_h
#define EbDefinitions_h

#include <stdint.h>
#include <stdio.h>
#include "EbSvtVp9Enc.h"
#include "vp9_enums.h"
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
#define EB_EXTERN extern "C"
#else
#define EB_EXTERN
#endif // __cplusplus
#ifdef    _MSC_VER
#define FORCE_INLINE            __forceinline
#else
#ifdef __cplusplus
#define FORCE_INLINE            inline   __attribute__((always_inline))
#else
#define FORCE_INLINE            __inline __attribute__((always_inline))
#endif
#endif

#define PKT_PRINT           0 // DEBUG: to print output pictures and ref from packetization
#define LIB_PRINTF_ENABLE   1 // When 0 mute all printfs from library

#if LIB_PRINTF_ENABLE
#define SVT_LOG printf
#else
#if _MSC_VER
#define SVT_LOG(s, ...) printf("")
#else
#define SVT_LOG(s, ...) printf("",##__VA_ARGS__)
#endif
#endif
#ifdef _WIN32
#ifndef __cplusplus
#define inline __inline
#endif
#elif __GNUC__
#define inline __inline__
#define fseeko64 fseek
#define ftello64 ftell
#else
#error OS not supported
#endif

#ifdef __GNUC__
#define AVX512_FUNC_TARGET __attribute__(( target( "avx512f,avx512dq,avx512bw,avx512vl" ) ))
#define AVX2_FUNC_TARGET   __attribute__(( target( "avx2" ) ))
#else
#define AVX512_FUNC_TARGET
#define AVX2_FUNC_TARGET
#endif // __GNUC__

#define EB_STRINGIZE( L )       #L
#define EB_MAKESTRING( M, L )   M( L )
#define EB_SRC_LINE_NUM         EB_MAKESTRING( EB_STRINGIZE, __LINE__ )
#define EB_MIN(a,b)             (((a) < (b)) ? (a) : (b))

#ifdef    _MSC_VER

#define NOINLINE                __declspec ( noinline )
#define FORCE_INLINE            __forceinline
#pragma warning( disable : 4068 ) // unknown pragma
#define EB_MESSAGE(m)           ( __FILE__ "(" EB_SRC_LINE_NUM "): message: " m )

#else // _MSC_VER

#define NOINLINE                __attribute__(( noinline ))

#ifdef __cplusplus
#define FORCE_INLINE            inline   __attribute__((always_inline))
#else
#define FORCE_INLINE            __inline __attribute__((always_inline))
#endif // __cplusplus

//#define __popcnt                __builtin_popcount
#define EB_MESSAGE(m)           __FILE__ "(" EB_SRC_LINE_NUM "): message: " m

#endif // _MSC_VER

    // Define Cross-Platform 64-bit fseek() and ftell()
#ifdef _WIN32
    typedef __int64 Off64;
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#else // *Note -- fseeko and ftello are already defined for linux
#ifndef __cplusplus
#ifndef __USE_LARGEFILE
#define __USE_LARGEFILE
#endif
#endif
#endif

// Used to hide GCC warnings for unused function tables
#ifdef __GNUC__
#define FUNC_TABLE __attribute__ ((unused))
#else
#define FUNC_TABLE
#endif

#define NEW_PRED_STRUCT             1
// DEV MACROS
#define DISABLE_AVX512
#define NEW_API                     1
#define VP9_RD                      1
#define VP9_PERFORM_EP              1
#define CHROMA_QP_OFFSET            1   // Turn on to change default from DC/AC = -15/-15

#define HME_ENHANCED_CENTER_SEARCH  1
#define ADP_STATS_PER_LAYER         0   // Lossless: ADP stats
#define TURN_OFF_PRE_PROCESSING     1   // SQ only:Turn OFF enable_denoise_src_flag and check_input_for_borders_and_preprocess
#define VP9_RC                      1

#define SHUT_64x64_BASE_RESTRICTION 1   // OQ

#define USE_SRC_REF                 0
#define INTER_INTRA_BIAS            0

#define SEG_SUPPORT                 0

#define RC_FEEDBACK                 1

#define INTRA_4x4_I_SLICE           1    // Hsan: to test/fix/enable
#define INTRA_4x4_SB_DEPTH_84_85    1    // Hsan: to test/fix/enable

#define ADAPTIVE_QP_INDEX_GEN       1

#if SEG_SUPPORT
#define BEA                         1
#define BEA_SCENE_CHANGE            1
#endif
#if VP9_RC
#define VP9_RC_PRINTS               0
#define RC_NO_EXTRA                 1
#endif

#define    Log2f                              eb_vp9_Log2f_SSE2

#ifndef _RSIZE_T_DEFINED
    typedef size_t rsize_t;
#define _RSIZE_T_DEFINED
#endif /* _RSIZE_T_DEFINED */

#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
    typedef int Errno;
#endif  /* _ERRNO_T_DEFINED */

    // *Note - This work around is needed for the windows visual studio compiler
    //  (MSVC) because it doesn't support the C99 header file stdint.h.
    //  All other compilers should support the stdint.h C99 standard types.
    typedef enum AttributePacked
    {
        PART_N,
        PART_H,
        PART_V,
        PART_S
    } Part;

#define PREAVX2_MASK    1
#define AVX2_MASK       2
#define AVX512_MASK     4

#define ASM_AVX2_BIT    3

    /** The EB_GOP type is used to describe the hierarchical coding structure of
    Groups of Pictures (GOP) units.
    */
#define EB_PRED                 uint8_t
#define EB_PRED_LOW_DELAY_P     0
#define EB_PRED_LOW_DELAY_B     1
#define EB_PRED_RANDOM_ACCESS   2
#define EB_PRED_TOTAL_COUNT     3
#define EB_PRED_INVALID         0xFF

/** The EB_SAMPLE type is intended to be used to pass arrays of bytes such as
buffers to and from the eBrisk API.  The EbByte type is a 32 bit pointer.
The pointer is word aligned and the buffer is byte aligned.
*/

/** The EbBitDepth type is used to describe the bit_depth of video data.
*/
    typedef enum EbBitDepth
    {
        EB_8BIT = 8,
        EB_10BIT = 10,
        EB_12BIT = 12,
        EB_14BIT = 14,
        EB_16BIT = 16
    } EbBitDepth;

    /** Assembly Types
    */
    typedef enum EbAsm
    {
        ASM_NON_AVX2,
        ASM_AVX2,
        ASM_TYPE_TOTAL,
        ASM_AVX512,
        ASM_TYPE_INVALID = ~0
    } EbAsm;

#define MAX_MPM_CANDIDATES 3

/** The EB_SLICE type is used to describe the slice prediction type.
*/

#define EB_SLICE        uint8_t
#define B_SLICE         0
#define P_SLICE         1
#define I_SLICE         2

/** The EB_PIC_STRUCT type is used to describe the picture structure.
*/
#define EB_PIC_STRUCT           uint8_t
#define PROGRESSIVE_PIC_STRUCT  0
#define TOP_FIELD_PIC_STRUCT    1
#define BOTTOM_FIELD_PIC_STRUCT 2

/** The EbModeType type is used to describe the PU type.
*/
typedef uint8_t EbModeType;
#define INTER_MODE 1
#define INTRA_MODE 2

extern uint32_t eb_vp9_ASM_TYPES;

/** Depth offsets
*/
static const uint8_t depth_offset[4]       = { 85, 21, 5, 1 };

/** ME 2Nx2N offsets
*/
static const uint32_t me2_nx2_n_offset[4]    = { 0, 1, 5, 21 };

/** The EB_ENC_MODE type is used to describe the encoder mode .
*/

#define EB_ENC_MODE         uint8_t
#define ENC_MODE_0          0
#define ENC_MODE_1          1
#define ENC_MODE_2          2
#define ENC_MODE_3          3
#define ENC_MODE_4          4
#define ENC_MODE_5          5
#define ENC_MODE_6          6
#define ENC_MODE_7          7
#define ENC_MODE_8          8
#define ENC_MODE_9          9
#define ENC_MODE_10         10
#define ENC_MODE_11         11
#define ENC_MODE_12         12

#define MAX_SUPPORTED_MODES 13

#define MAX_SUPPORTED_MODES_SUB1080P    10
#define MAX_SUPPORTED_MODES_1080P       11
#define MAX_SUPPORTED_MODES_4K_OQ       11
#define MAX_SUPPORTED_MODES_4K_SQ       13

#define SPEED_CONTROL_INIT_MOD ENC_MODE_5;

/** The EB_TUID type is used to identify a TU within a CU.
*/

#define REF_LIST_0             0
#define REF_LIST_1             1

#define EB_PREDDIRECTION         uint8_t
#define UNI_PRED_LIST_0          0
#define UNI_PRED_LIST_1          1
#define BI_PRED                  2

// The EB_QP_OFFSET_MODE type is used to describe the QP offset
#define EB_FRAME_CARACTERICTICS uint8_t
#define EB_FRAME_CARAC_0           0
#define EB_FRAME_CARAC_1           1
#define EB_FRAME_CARAC_2           2

// Rate Control
#define THRESHOLD1QPINCREASE     1 //AMIR changes mode 1 to be modified
#define THRESHOLD2QPINCREASE     2
//#define THRESHOLD1QPINCREASE     0
//#define THRESHOLD2QPINCREASE     1

#define EB_IOS_POINT            uint8_t
#define OIS_VERY_FAST_MODE       0
#define OIS_FAST_MODE            1
#define OIS_MEDUIM_MODE          2
#define OIS_COMPLEX_MODE         3
#define OIS_VERY_COMPLEX_MODE    4

#define _MVXT(mv) ( (int16_t)((mv) &  0xFFFF) )
#define _MVYT(mv) ( (int16_t)((mv) >> 16    ) )

//WEIGHTED PREDICTION

#define  WP_SHIFT1D_10BIT      2
#define  WP_OFFSET1D_10BIT     (-32768)

#define  WP_SHIFT2D_10BIT      6

#define  WP_IF_OFFSET_10BIT    8192//2^(14-1)

#define  WP_SHIFT_10BIT        4   // 14 - 10
#define  WP_BI_SHIFT_10BIT     5

// Will contain the EbEncApi which will live in the EncHandle class
// Modifiable during encode time.
typedef struct EbH265DynEncConfiguration {
    uint32_t available_target_bitrate;
} EbH265DynEncConfiguration;

#ifdef __GNUC__
#define EB_ALIGN(n) __attribute__((__aligned__(n)))
#elif defined(_MSC_VER)
#define EB_ALIGN(n) __declspec(align(n))
#else
#define EB_ALIGN(n)
#endif

/** The EbHandle type is used to define OS object handles for threads,
semaphores, mutexs, etc.
*/
typedef void * EbHandle;

#define EB_INPUT_RESOLUTION             uint8_t
#define INPUT_SIZE_576p_RANGE_OR_LOWER     0
#define INPUT_SIZE_1080i_RANGE             1
#define INPUT_SIZE_1080p_RANGE             2
#define INPUT_SIZE_4K_RANGE                 3

#define INPUT_SIZE_576p_TH                0x90000        // 0.58 Million
#define INPUT_SIZE_1080i_TH                0xB71B0        // 0.75 Million
#define INPUT_SIZE_1080p_TH                0x1AB3F0    // 1.75 Million
#define INPUT_SIZE_4K_TH                0x29F630    // 2.75 Million
#define INPUT_SIZE_8K_TH                (0x29F630 << 2)    // 2.75 Million

/** The EB_BOOL type is intended to be used to represent a true or a false
value when passing parameters to and from the eBrisk API.  The
EB_BOOL is a 32 bit quantity and is aligned on a 32 bit word boundary.
*/

#define EB_BOOL   uint8_t
#define EB_FALSE  0
#define EB_TRUE   1

/** The EbHandleType type is intended to be used to pass pointers to and from the eBrisk
API.  This is a 32 bit pointer and is aligned on a 32 bit word boundary.
*/
typedef void* EbHandleType;

/** The EbPtr  type is intended to be used to pass pointers to and from the eBrisk
API.  This is a 32 bit pointer and is aligned on a 32 bit word boundary.
*/
typedef void * EbPtr;

/** The EbByte type is intended to be used to pass arrays of bytes such as
buffers to and from the eBrisk API.  The EbByte type is a 32 bit pointer.
The pointer is word aligned and the buffer is byte aligned.
*/
typedef uint8_t * EbByte;

/** The EB_NULL type is used to define the C style NULL pointer.
*/
#define EB_NULL ((void*) 0)

typedef enum EbPtrType
{
    EB_N_PTR = 0,                          // malloc'd pointer
    EB_A_PTR = 1,                          // malloc'd pointer aligned
    EB_MUTEX = 2,                          // mutex
    EB_SEMAPHORE = 3,                      // semaphore
    EB_THREAD = 4                          // thread handle
}EbPtrType;

typedef struct EbMemoryMapEntry
{
    EbPtr      ptr;                        // points to a memory pointer
    EbPtrType  ptr_type;                   // pointer type
} EbMemoryMapEntry;

// Display Total Memory at the end of the memory allocations
#define DISPLAY_MEMORY                                  0

// TODO EbUtility.h is probably a better place for it, would require including EbUtility.h

/********************************************************************************************
* faster memcopy for <= 64B blocks, great w/ inlining and size known at compile time (or w/ PGO)
* THIS NEEDS TO STAY IN A HEADER FOR BEST PERFORMANCE
********************************************************************************************/

#include <immintrin.h>

#if !defined(__clang__) && !defined(__INTEL_COMPILER) && defined(__GNUC__)
__attribute__((optimize("unroll-loops")))
#endif
FORCE_INLINE void eb_memcpy_small(void* dst_ptr, void const* src_ptr, size_t size)
{
    const char* src = (const char*)src_ptr;
    char*       dst = (char*)dst_ptr;
    size_t      i = 0;

#ifdef _INTEL_COMPILER
#pragma unroll
#endif
    while ((i + 16) <= size)
    {
        _mm_storeu_ps((float*)(dst + i), _mm_loadu_ps((const float*)(src + i)));
        i += 16;
    }

    if ((i + 8) <= size)
    {
        _mm_store_sd((double*)(dst + i), _mm_load_sd((const double*)(src + i)));
        i += 8;
    }

    for (; i < size; ++i)
        dst[i] = src[i];
}

FORCE_INLINE void eb_memcpy_sse(void* dst_ptr, void const* src_ptr, size_t size)
{
    const char* src = (const char*)src_ptr;
    char*       dst = (char*)dst_ptr;
    size_t      i = 0;
    size_t align_cnt = EB_MIN((64 - ((size_t)dst & 63)), size);

    // align dest to a $line
    if (align_cnt != 64)
    {
        eb_memcpy_small(dst, src, align_cnt);
        dst += align_cnt;
        src += align_cnt;
        size -= align_cnt;
    }

    // copy a $line at a time
    // dst aligned to a $line
    size_t cline_cnt = (size & ~(size_t)63);
    for (i = 0; i < cline_cnt; i += 64)
    {

        __m128 c0 = _mm_loadu_ps((const float*)(src + i));
        __m128 c1 = _mm_loadu_ps((const float*)(src + i + sizeof(c0)));
        __m128 c2 = _mm_loadu_ps((const float*)(src + i + sizeof(c0) * 2));
        __m128 c3 = _mm_loadu_ps((const float*)(src + i + sizeof(c0) * 3));

        _mm_storeu_ps((float*)(dst + i), c0);
        _mm_storeu_ps((float*)(dst + i + sizeof(c0)), c1);
        _mm_storeu_ps((float*)(dst + i + sizeof(c0) * 2), c2);
        _mm_storeu_ps((float*)(dst + i + sizeof(c0) * 3), c3);

    }

    // copy the remainder
    if (i < size)
        eb_memcpy_small(dst + i, src + i, size - i);
}

FORCE_INLINE void eb_memcpy(void  *dst_ptr, void  *src_ptr, size_t size)
{
    if (size > 64) {
        eb_memcpy_sse(dst_ptr, src_ptr, size);
    }
    else
    {
        eb_memcpy_small(dst_ptr, src_ptr, size);
    }
}

#define EB_MEMCPY(dst, src, size) \
    eb_memcpy(dst, src, size)

#define EB_MEMSET(dst, val, count) \
    memset(dst, val, count)

#define EB_STRCMP(target,token) \
    strcmp(target,token)

/* string length */
EB_API rsize_t eb_vp9_strnlen_ss(const char *s, rsize_t smax);

#define EB_STRLEN(target, max_size) \
    eb_vp9_strnlen_ss(target, max_size)

#define MAX_NUM_PTR                (0x1312D00 << 2) //0x4C4B4000            // Maximum number of pointers to be allocated for the library

#define ALVALUE                     32

extern    EbMemoryMapEntry *memory_map;                // library Memory table
extern    uint32_t         *memory_map_index;          // library memory index
extern    uint64_t         *total_lib_memory;          // library Memory malloc'd

extern    uint32_t          lib_malloc_count;
extern    uint32_t          lib_thread_count;
extern    uint32_t          lib_semaphore_count;
extern    uint32_t          lib_mutex_count;

#ifdef _WIN32
#define EB_ALLIGN_MALLOC(type, pointer, n_elements, pointer_class) \
    pointer = (type) _aligned_malloc(n_elements,ALVALUE); \
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
    lib_malloc_count++;

#else
#define EB_ALLIGN_MALLOC(type, pointer, n_elements, pointer_class) \
    if (posix_memalign((void**)(&(pointer)), ALVALUE, n_elements) != 0) { \
        return EB_ErrorInsufficientResources; \
            } \
                else { \
        pointer = (type) pointer;  \
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
    lib_malloc_count++;
#endif

#define EB_MEMORY() \
    SVT_LOG("Total Number of Mallocs in Library: %d\n", lib_malloc_count); \
    SVT_LOG("Total Number of Threads in Library: %d\n", lib_thread_count); \
    SVT_LOG("Total Number of Semaphore in Library: %d\n", lib_semaphore_count); \
    SVT_LOG("Total Number of Mutex in Library: %d\n", lib_mutex_count); \
    SVT_LOG("Total Library Memory: %.2lf KB\n\n",*total_lib_memory/(double)1024);

#define EB_MALLOC(type, pointer, n_elements, pointer_class) \
    pointer = (type) malloc(n_elements); \
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
    lib_malloc_count++;

#define EB_CALLOC(type, pointer, count, size, pointer_class) \
    pointer = (type) calloc(count, size); \
    if (pointer == (type)EB_NULL) { \
        return EB_ErrorInsufficientResources; \
    } \
    else { \
        memory_map[*(memory_map_index)].ptr_type = pointer_class; \
        memory_map[(*(memory_map_index))++].ptr = pointer; \
        if (count % 8 == 0) { \
            *total_lib_memory += (count); \
        } \
        else { \
            *total_lib_memory += ((count) + (8 - ((count) % 8))); \
        } \
    } \
    if (*(memory_map_index) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
    } \
    lib_malloc_count++;

#define EB_CREATESEMAPHORE(type, pointer, n_elements, pointer_class, initial_count, max_count) \
    pointer = eb_vp9_create_semaphore(initial_count, max_count); \
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
    lib_semaphore_count++;

#define EB_CREATEMUTEX(type, pointer, n_elements, pointer_class) \
    pointer = eb_vp9_create_mutex(); \
    if (pointer == (type)EB_NULL){ \
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
    lib_mutex_count++;

/** The EB_CTOR type is used to define the eBrisk object constructors.
object_ptr is a EbPtr  to the object being constructed.
object_init_data_ptr is a EbPtr  to a data structure used to initialize the object.
*/
typedef EbErrorType (*EB_CTOR)(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr);

/** The EbDtor type is used to define the eBrisk object destructors.
object_ptr is a EbPtr  to the object being constructed.
*/
typedef void(*EbDtor)(
    EbPtr object_ptr);

/**************************************
* Callback Functions
**************************************/
typedef struct EbCallback
{
    EbPtr app_private_data;
    EbPtr handle;

    void(*error_handler)(
        EbPtr    handle,
        uint32_t error_code);

} EbCallback;

typedef enum DistCalcType
{
    DIST_CALC_RESIDUAL = 0,    // SSE(Coefficients - ReconCoefficients)
    DIST_CALC_PREDICTION = 1,    // SSE(Coefficients) *Note - useful in modes that don't send residual coeff bits
    DIST_CALC_TOTAL = 2
} DistCalcType;

typedef enum EbSei
{
    BUFFERING_PERIOD = 0,
    PICTURE_TIMING = 1,
    REG_USER_DATA = 4,
    UNREG_USER_DATA = 5,
    RECOVERY_POINT = 6,
    DECODED_PICTURE_HASH = 132,
    PAN_SCAN_RECT = 2,
    FILLER_PAYLOAD = 3,
    USER_DATA_REGISTERED_ITU_T_T35 = 4,
    USER_DATA_UNREGISTERED = 5,
    SCENE_INFO = 9,
    FULL_FRAME_SNAPSHOT = 15,
    PROGRESSIVE_REFINEMENT_SEGMENT_START = 16,
    PROGRESSIVE_REFINEMENT_SEGMENT_END = 17,
    FILM_GRAIN_CHARACTERISTICS = 19,
    POST_FILTER_HINT = 22,
    TONE_MAPPING_INFO = 23,
    FRAME_PACKING = 45,
    DISPLAY_ORIENTATION = 47,
    SOP_DESCRIPTION = 128,
    ACTIVE_PARAMETER_SETS = 129,
    DECODING_UNIT_INFO = 130,
    TEMPORAL_LEVEL0_INDEX = 131,
    SCALABLE_NESTING = 133,
    REGION_REFRESH_INFO = 134,

} EbSei;

/** The EB_GOP type is used to describe the hierarchical coding structure of
Groups of Pictures (GOP) units.
*/
#define EB_PRED                 uint8_t
#define EB_PRED_LOW_DELAY_P     0
#define EB_PRED_LOW_DELAY_B     1
#define EB_PRED_RANDOM_ACCESS   2
#define EB_PRED_TOTAL_COUNT     3
#define EB_PRED_INVALID         0xFF

#define TUNE_SQ                 0
#define TUNE_OQ                 1
#define TUNE_VMAF               2

#define ME_FILTER_TAP           4

#define    SUB_SAD_SEARCH   0
#define    FULL_SAD_SEARCH  1
#define    SSD_SEARCH       2

//***Profile, tier, level***
#define TOTAL_LEVEL_COUNT                           13

#define C1_TRSHLF_N       1
#define C1_TRSHLF_D       1
#define C2_TRSHLF_N       16
#define C2_TRSHLF_D       10

#define ANTI_CONTOURING_TH_0     16 * 16
#define ANTI_CONTOURING_TH_1     32 * 32
#define ANTI_CONTOURING_TH_2 2 * 32 * 32

#define ANTI_CONTOURING_LUMA_T1                40
#define ANTI_CONTOURING_LUMA_T2                180

#define VAR_BASED_DETAIL_PRESERVATION_SELECTOR_THRSLHD         (64*64)

#define MAX_BITS_PER_FRAME            8000000

#define MEAN_DIFF_THRSHOLD              10
#define VAR_DIFF_THRSHOLD               10

#define SC_FRAMES_TO_IGNORE             100 // The speed control algorith starts after SC_FRAMES_TO_IGNORE number frames.
#define SC_FRAMES_INTERVAL_SPEED        60 // The speed control Interval To Check the speed
#define SC_FRAMES_INTERVAL_T1           60 // The speed control Interval Threshold1
#define SC_FRAMES_INTERVAL_T2          180 // The speed control Interval Threshold2
#define SC_FRAMES_INTERVAL_T3          120 // The speed control Interval Threshold3

#define CMPLX_LOW                0
#define CMPLX_MEDIUM             1
#define CMPLX_HIGH               2
#define CMPLX_VHIGH              3
#define CMPLX_NOISE              4

#define YBITS_THSHLD_1(x)                  ((x < ENC_MODE_12) ? 80 : 120)

//***Encoding Parameters***

#define MAX_PICTURE_WIDTH_SIZE                      9344u
#define MAX_PICTURE_HEIGHT_SIZE                     5120u

#define INTERNAL_BIT_DEPTH                          8 // to be modified
#define MAX_SAMPLE_VALUE                            ((1 << INTERNAL_BIT_DEPTH) - 1)
#define MAX_SAMPLE_VALUE_10BIT                      0x3FF
#define MAX_SB_SIZE                                 64u
#define MAX_SB_SIZE_MINUS_1                         63u
#define LOG2F_MAX_SB_SIZE                           6u
#define MAX_SB_SIZE_CHROMA                          32u
#define MAX_LOG2_SB_SIZE                            6 // log2(MAX_SB_SIZE)
#define MAX_LEVEL_COUNT                             5 // log2(MAX_SB_SIZE) - log2(MIN_CU_SIZE)
#define MAX_TU_SIZE                                 32
#define MIN_TU_SIZE                                 4
#define LOG_MIN_CU_SIZE                             2
#define MIN_CU_SIZE                                 (1 << LOG_MIN_CU_SIZE)
#define MAX_CU_SIZE                                 64
#define MAX_INTRA_SIZE                              32
#define MIN_INTRA_SIZE                              8
#define LOG_MIN_PU_SIZE                             2
#define MIN_PU_SIZE                                 (1 << LOG_MIN_PU_SIZE)
#define MAX_NUM_OF_REF_PIC_LIST                     2
#define EB_MAX_SB_DEPTH                             5
#define MAX_QP_VALUE                                63
#define MAX_LAD                                     120
#define SCD_LAD                                     6

#define MAX_SB_ROWS                                ((MAX_PICTURE_HEIGHT_SIZE) / (MAX_SB_SIZE))

#define MAX_NUMBER_OF_TREEBLOCKS_PER_PICTURE       ((MAX_PICTURE_WIDTH_SIZE + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE) * \
                                                   ((MAX_PICTURE_HEIGHT_SIZE + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE)

//***Prediction Structure***
#define MAX_TEMPORAL_LAYERS                         6
#define MAX_HIERARCHICAL_LEVEL                      6
#define MAX_REF_IDX                                 1        // Set MAX_REF_IDX as 1 to avoid sending extra refPicIdc for each PU in IPPP flat GOP structure.
#define INVALID_POC                                 (((uint32_t) (~0)) - (((uint32_t) (~0)) >> 1))
#define MAX_ELAPSED_IDR_COUNT                       1024

//***HME***
#define EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT         2
#define EB_HME_SEARCH_AREA_ROW_MAX_COUNT            2

//***Mode Decision Candidate List***
#define LOG_MAX_AMVP_MODE_DECISION_CANDIDATE_NUM    2
#define MAX_AMVP_MODE_DECISION_CANDIDATE_NUM        (1 << LOG_MAX_AMVP_MODE_DECISION_CANDIDATE_NUM)

#define EP_BLOCK_MAX_COUNT                         681
#define PA_BLOCK_MAX_COUNT                          85

#define UNUSED(a) (void) a

#define _MVXT(mv) ( (int16_t)((mv) &  0xFFFF) )
#define _MVYT(mv) ( (int16_t)((mv) >> 16    ) )

//***MCP***
#define MaxChromaFilterTag          4

#define MCPXPaddingOffset           16                                    // to be modified
#define MCPYPaddingOffset           16                                    // to be modified

#define InternalBitDepth            8                                     // to be modified
#define MAX_Sample_Value            ((1 << InternalBitDepth) - 1)
#define IF_Shift                    6                                     // to be modified
#define IF_Prec                     14                                    // to be modified
#define IF_Negative_Offset          (IF_Prec - 1)                         // to be modified
#define InternalBitDepthIncrement   (InternalBitDepth - 8)

//***Transforms***
#define TRANSFORMS_LUMA_FLAG        0
#define TRANSFORMS_CHROMA_FLAG      1

#define TRANS_BIT_INCREMENT    0
#define QUANT_SHIFT            14 // Q(4) = 2^14
#define MAX_POS_16BIT_NUM      32767
#define MIN_NEG_16BIT_NUM      -32768
#define MEDIUM_SB_VARIANCE        50

// INTRA restriction for global motion
#define INTRA_GLOBAL_MOTION_NON_MOVING_INDEX_TH  2
#define INTRA_GLOBAL_MOTION_DARK_SB_TH         50

/*********************************************************
* used for the first time, but not the last time interpolation filter
*********************************************************/
#define Shift1       InternalBitDepthIncrement
#define MinusOffset1 (1 << (IF_Negative_Offset + InternalBitDepthIncrement))
#if (InternalBitDepthIncrement == 0)
#define ChromaMinusOffset1 0
#else
#define ChromaMinusOffset1 MinusOffset1
#endif

/*********************************************************
* used for neither the first time nor the last time interpolation filter
*********************************************************/
#define Shift2       IF_Shift

/*********************************************************
* used for the first time, and also the last time interpolation filter
*********************************************************/
#define Shift3       IF_Shift
#define Offset3      (1<<(Shift3-1))

/*********************************************************
* used for weighted sample prediction
*********************************************************/
#define Shift5       (IF_Shift - InternalBitDepthIncrement + 1)
#define Offset5      ((1 << (Shift5 - 1)) + (1 << (IF_Negative_Offset + 1)))
#if (InternalBitDepthIncrement == 0)
#define ChromaOffset5 (1 << (Shift5 - 1))
#else
#define ChromaOffset5 Offset5
#endif

/*********************************************************
* used for biPredCopy()
*********************************************************/
#define Shift6       (IF_Shift - InternalBitDepthIncrement)
#define MinusOffset6 (1 << IF_Negative_Offset)
#if (InternalBitDepthIncrement == 0)
#define ChromaMinusOffset6 0
#else
#define ChromaMinusOffset6 MinusOffset6
#endif

/*********************************************************
* 10bit case
*********************************************************/

#define  SHIFT1D_10BIT      6
#define  OFFSET1D_10BIT     32

#define  SHIFT2D1_10BIT     2
#define  OFFSET2D1_10BIT    (-32768)

#define  SHIFT2D2_10BIT     10
#define  OFFSET2D2_10BIT    524800

    //BIPRED
#define  BI_SHIFT_10BIT         4
#define  BI_OFFSET_10BIT        8192//2^(14-1)

#define  BI_AVG_SHIFT_10BIT     5
#define  BI_AVG_OFFSET_10BIT    16400

#define  BI_SHIFT2D2_10BIT      6
#define  BI_OFFSET2D2_10BIT     0

// Noise detection
#define  NOISE_VARIANCE_TH                390

#define  EB_PICNOISE_CLASS    uint8_t
#define  PIC_NOISE_CLASS_INV  0 //not computed
#define  PIC_NOISE_CLASS_1    1 //No Noise
#define  PIC_NOISE_CLASS_2    2
#define  PIC_NOISE_CLASS_3    3
#define  PIC_NOISE_CLASS_3_1  4
#define  PIC_NOISE_CLASS_4    5
#define  PIC_NOISE_CLASS_5    6
#define  PIC_NOISE_CLASS_6    7
#define  PIC_NOISE_CLASS_7    8
#define  PIC_NOISE_CLASS_8    9
#define  PIC_NOISE_CLASS_9    10
#define  PIC_NOISE_CLASS_10   11 //Extreme Noise

// Intrinisc

#define NON_MOVING_SCORE_0     0
#define NON_MOVING_SCORE_1    10
#define NON_MOVING_SCORE_2    20
#define NON_MOVING_SCORE_3    30
#define INVALID_NON_MOVING_SCORE (uint8_t) ~0

#define CLASS_1_REGION_SPLIT_PER_WIDTH        2
#define CLASS_1_REGION_SPLIT_PER_HEIGHT        2

#define HIGHER_THAN_CLASS_1_REGION_SPLIT_PER_WIDTH        4
#define HIGHER_THAN_CLASS_1_REGION_SPLIT_PER_HEIGHT        4

// Dynamic GOP activity TH - to tune
#define IS_COMPLEX_SB_VARIANCE_TH 100

// The EB_AURA_STATUS type is used to describe the aura status
#define EB_AURA_STATUS       uint8_t
#define AURA_STATUS_0        0
#define AURA_STATUS_1        1
#define INVALID_AURA_STATUS  128

// Aura detection definitions
#define    AURA_4K_DISTORTION_TH    25

static const int32_t global_motion_threshold[MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = { // [Highest Temporal Layer] [Temporal Layer Index]
    { 2 },
    { 4, 2 },
    { 8, 4, 2 },
    { 16, 8, 4, 2 },
    { 32, 16, 8, 4, 2 },    // Derived by analogy from 4-layer settings
    { 64, 32, 16, 8, 4, 2 }
};

#define EB_SCD_MODE uint8_t
#define SCD_MODE_0   0     // SCD OFF
#define SCD_MODE_1   1     // Light SCD (histograms generation on the 1/16 decimated input)
#define SCD_MODE_2   2     // Full SCD

#define EB_QP_SCALING_MODE uint8_t
#define QP_SCALING_MODE_0   0     // Default QP Scaling
#define QP_SCALING_MODE_1   1     // Adaptive QP Scaling

#define EB_CHROMA_LEVEL uint8_t
#define CHROMA_LEVEL_0  0 // CHROMA ON @ MD + separate CHROMA search ON
#define CHROMA_LEVEL_1  1 // CHROMA ON @ MD + separate CHROMA search OFF
#define CHROMA_LEVEL_2  2 // CHROMA OFF @ MD

#define EB_NOISE_DETECT_MODE uint8_t
#define NOISE_DETECT_HALF_PRECISION      0     // Use Half-pel decimated input to detect noise
#define NOISE_DETECT_QUARTER_PRECISION   1     // Use Quarter-pel decimated input to detect noise
#define NOISE_DETECT_FULL_PRECISION      2     // Use Full-pel decimated input to detect noise

#define EB_SB_COMPLEXITY_STATUS uint8_t
#define SB_COMPLEXITY_STATUS_0                  0
#define SB_COMPLEXITY_STATUS_1                  1
#define SB_COMPLEXITY_STATUS_2                  2
#define SB_COMPLEXITY_STATUS_INVALID   (uint8_t) ~0

#define SB_COMPLEXITY_NON_MOVING_INDEX_TH_0 30
#define SB_COMPLEXITY_NON_MOVING_INDEX_TH_1 29
#define SB_COMPLEXITY_NON_MOVING_INDEX_TH_2 23

typedef enum EbCu16x16Mode
{
    CU_16x16_MODE_0 = 0,  // Perform OIS, Full_Search, Fractional_Search & Bipred for CU_16x16
    CU_16x16_MODE_1 = 1   // Perform OIS and only Full_Search for CU_16x16
} EbCu16x16Mode;

typedef enum EbCu8x8Mode
{
    CU_8x8_MODE_0 = 0,  // Perform OIS, Full_Search, Fractional_Search & Bipred for CU_8x8
    CU_8x8_MODE_1 = 1   // Do not perform OIS @ P/B Slices and only Full_Search for CU_8x8
} EbCu8x8Mode;

typedef enum EbPictureDepthMode
{
    PIC_SB_SWITCH_DEPTH_MODE    = 0,
    PIC_FULL85_DEPTH_MODE       = 1,
    PIC_FULL84_DEPTH_MODE       = 2,
    PIC_BDP_DEPTH_MODE          = 3,
    PIC_LIGHT_BDP_DEPTH_MODE    = 4,
    PIC_OPEN_LOOP_DEPTH_MODE    = 5
} EbPictureDepthMode;

#define EB_SB_DEPTH_MODE uint8_t
#define SB_FULL85_DEPTH_MODE                1
#define SB_FULL84_DEPTH_MODE                2
#define SB_BDP_DEPTH_MODE                   3
#define SB_LIGHT_BDP_DEPTH_MODE             4
#define SB_OPEN_LOOP_DEPTH_MODE             5
#define SB_LIGHT_OPEN_LOOP_DEPTH_MODE       6
#define SB_AVC_DEPTH_MODE                   7
#define SB_LIGHT_AVC_DEPTH_MODE             8
#define SB_PRED_OPEN_LOOP_DEPTH_MODE        9
#define SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE 10

typedef enum EbAdpDepthSensitivePicClass
{
    DEPTH_SENSITIVE_PIC_CLASS_0 = 0,    // Normal picture
    DEPTH_SENSITIVE_PIC_CLASS_1 = 1,    // High complex picture
    DEPTH_SENSITIVE_PIC_CLASS_2 = 2     // Moderate complex picture
} EbAdpDepthSensitivePicClass;

typedef enum EbAdpRefinementMode
{
    ADP_REFINEMENT_OFF = 0,  // Off
    ADP_MODE_0         = 1,  // Light AVC (only 16x16)
    ADP_MODE_1         = 2   // AVC (only 8x8 & 16x16 @ the Open Loop Search)
} EbAdpRefinementMode;

typedef enum EbMdStage
{
    MDC_STAGE                 = 0,
    BDP_PILLAR_STAGE          = 1,
    BDP_64X64_32X32_REF_STAGE = 2,
    BDP_8X8_REF_STAGE         = 3,
    BDP_NEAREST_NEAR_STAGE    = 4

} EbMdStage;

static const int32_t hme_level_0_search_area_multiplier_x[MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = { // [Highest Temporal Layer] [Temporal Layer Index]
    { 100 },
    { 100, 100 },
    { 100, 100, 100 },
    { 200, 140, 100,  70 },
    { 350, 200, 100, 100, 100 },
    { 525, 350, 200, 100, 100, 100 }
};

static const int32_t hme_level_0_search_area_multiplier_y[MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = { // [Highest Temporal Layer] [Temporal Layer Index]
    { 100 },
    { 100, 100 },
    { 100, 100, 100 },
    { 200, 140, 100, 70 },
    { 350, 200, 100, 100, 100 },
    { 525, 350, 200, 100, 100, 100 }
};

typedef enum PaRasterScanCuIndex
{
     // 2Nx2N [85 partitions]
     PA_RASTER_SCAN_CU_INDEX_64x64 = 0,
     PA_RASTER_SCAN_CU_INDEX_32x32_0 = 1,
     PA_RASTER_SCAN_CU_INDEX_32x32_1 = 2,
     PA_RASTER_SCAN_CU_INDEX_32x32_2 = 3,
     PA_RASTER_SCAN_CU_INDEX_32x32_3 = 4,
     PA_RASTER_SCAN_CU_INDEX_16x16_0 = 5,
     PA_RASTER_SCAN_CU_INDEX_16x16_1 = 6,
     PA_RASTER_SCAN_CU_INDEX_16x16_2 = 7,
     PA_RASTER_SCAN_CU_INDEX_16x16_3 = 8,
     PA_RASTER_SCAN_CU_INDEX_16x16_4 = 9,
     PA_RASTER_SCAN_CU_INDEX_16x16_5 = 10,
     PA_RASTER_SCAN_CU_INDEX_16x16_6 = 11,
     PA_RASTER_SCAN_CU_INDEX_16x16_7 = 12,
     PA_RASTER_SCAN_CU_INDEX_16x16_8 = 13,
     PA_RASTER_SCAN_CU_INDEX_16x16_9 = 14,
     PA_RASTER_SCAN_CU_INDEX_16x16_10 = 15,
     PA_RASTER_SCAN_CU_INDEX_16x16_11 = 16,
     PA_RASTER_SCAN_CU_INDEX_16x16_12 = 17,
     PA_RASTER_SCAN_CU_INDEX_16x16_13 = 18,
     PA_RASTER_SCAN_CU_INDEX_16x16_14 = 19,
     PA_RASTER_SCAN_CU_INDEX_16x16_15 = 20,
     PA_RASTER_SCAN_CU_INDEX_8x8_0 = 21,
     PA_RASTER_SCAN_CU_INDEX_8x8_1 = 22,
     PA_RASTER_SCAN_CU_INDEX_8x8_2 = 23,
     PA_RASTER_SCAN_CU_INDEX_8x8_3 = 24,
     PA_RASTER_SCAN_CU_INDEX_8x8_4 = 25,
     PA_RASTER_SCAN_CU_INDEX_8x8_5 = 26,
     PA_RASTER_SCAN_CU_INDEX_8x8_6 = 27,
     PA_RASTER_SCAN_CU_INDEX_8x8_7 = 28,
     PA_RASTER_SCAN_CU_INDEX_8x8_8 = 29,
     PA_RASTER_SCAN_CU_INDEX_8x8_9 = 30,
     PA_RASTER_SCAN_CU_INDEX_8x8_10 = 31,
     PA_RASTER_SCAN_CU_INDEX_8x8_11 = 32,
     PA_RASTER_SCAN_CU_INDEX_8x8_12 = 33,
     PA_RASTER_SCAN_CU_INDEX_8x8_13 = 34,
     PA_RASTER_SCAN_CU_INDEX_8x8_14 = 35,
     PA_RASTER_SCAN_CU_INDEX_8x8_15 = 36,
     PA_RASTER_SCAN_CU_INDEX_8x8_16 = 37,
     PA_RASTER_SCAN_CU_INDEX_8x8_17 = 38,
     PA_RASTER_SCAN_CU_INDEX_8x8_18 = 39,
     PA_RASTER_SCAN_CU_INDEX_8x8_19 = 40,
     PA_RASTER_SCAN_CU_INDEX_8x8_20 = 41,
     PA_RASTER_SCAN_CU_INDEX_8x8_21 = 42,
     PA_RASTER_SCAN_CU_INDEX_8x8_22 = 43,
     PA_RASTER_SCAN_CU_INDEX_8x8_23 = 44,
     PA_RASTER_SCAN_CU_INDEX_8x8_24 = 45,
     PA_RASTER_SCAN_CU_INDEX_8x8_25 = 46,
     PA_RASTER_SCAN_CU_INDEX_8x8_26 = 47,
     PA_RASTER_SCAN_CU_INDEX_8x8_27 = 48,
     PA_RASTER_SCAN_CU_INDEX_8x8_28 = 49,
     PA_RASTER_SCAN_CU_INDEX_8x8_29 = 50,
     PA_RASTER_SCAN_CU_INDEX_8x8_30 = 51,
     PA_RASTER_SCAN_CU_INDEX_8x8_31 = 52,
     PA_RASTER_SCAN_CU_INDEX_8x8_32 = 53,
     PA_RASTER_SCAN_CU_INDEX_8x8_33 = 54,
     PA_RASTER_SCAN_CU_INDEX_8x8_34 = 55,
     PA_RASTER_SCAN_CU_INDEX_8x8_35 = 56,
     PA_RASTER_SCAN_CU_INDEX_8x8_36 = 57,
     PA_RASTER_SCAN_CU_INDEX_8x8_37 = 58,
     PA_RASTER_SCAN_CU_INDEX_8x8_38 = 59,
     PA_RASTER_SCAN_CU_INDEX_8x8_39 = 60,
     PA_RASTER_SCAN_CU_INDEX_8x8_40 = 61,
     PA_RASTER_SCAN_CU_INDEX_8x8_41 = 62,
     PA_RASTER_SCAN_CU_INDEX_8x8_42 = 63,
     PA_RASTER_SCAN_CU_INDEX_8x8_43 = 64,
     PA_RASTER_SCAN_CU_INDEX_8x8_44 = 65,
     PA_RASTER_SCAN_CU_INDEX_8x8_45 = 66,
     PA_RASTER_SCAN_CU_INDEX_8x8_46 = 67,
     PA_RASTER_SCAN_CU_INDEX_8x8_47 = 68,
     PA_RASTER_SCAN_CU_INDEX_8x8_48 = 69,
     PA_RASTER_SCAN_CU_INDEX_8x8_49 = 70,
     PA_RASTER_SCAN_CU_INDEX_8x8_50 = 71,
     PA_RASTER_SCAN_CU_INDEX_8x8_51 = 72,
     PA_RASTER_SCAN_CU_INDEX_8x8_52 = 73,
     PA_RASTER_SCAN_CU_INDEX_8x8_53 = 74,
     PA_RASTER_SCAN_CU_INDEX_8x8_54 = 75,
     PA_RASTER_SCAN_CU_INDEX_8x8_55 = 76,
     PA_RASTER_SCAN_CU_INDEX_8x8_56 = 77,
     PA_RASTER_SCAN_CU_INDEX_8x8_57 = 78,
     PA_RASTER_SCAN_CU_INDEX_8x8_58 = 79,
     PA_RASTER_SCAN_CU_INDEX_8x8_59 = 80,
     PA_RASTER_SCAN_CU_INDEX_8x8_60 = 81,
     PA_RASTER_SCAN_CU_INDEX_8x8_61 = 82,
     PA_RASTER_SCAN_CU_INDEX_8x8_62 = 83,
     PA_RASTER_SCAN_CU_INDEX_8x8_63 = 84
} PaRasterScanCuIndex;

static const uint32_t pa_raster_scan_cu_x[PA_BLOCK_MAX_COUNT] =
{
    0,
    0, 32,
    0, 32,
    0, 16, 32, 48,
    0, 16, 32, 48,
    0, 16, 32, 48,
    0, 16, 32, 48,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56
};

static const uint32_t pa_raster_scan_cu_y[PA_BLOCK_MAX_COUNT] =
{
    0,
    0, 0,
    32, 32,
    0, 0, 0, 0,
    16, 16, 16, 16,
    32, 32, 32, 32,
    48, 48, 48, 48,
    0, 0, 0, 0, 0, 0, 0, 0,
    8, 8, 8, 8, 8, 8, 8, 8,
    16, 16, 16, 16, 16, 16, 16, 16,
    24, 24, 24, 24, 24, 24, 24, 24,
    32, 32, 32, 32, 32, 32, 32, 32,
    40, 40, 40, 40, 40, 40, 40, 40,
    48, 48, 48, 48, 48, 48, 48, 48,
    56, 56, 56, 56, 56, 56, 56, 56
};

static const uint32_t pa_raster_scan_cu_size[PA_BLOCK_MAX_COUNT] =
{    64,
    32, 32,
    32, 32,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8
};

static const uint32_t pa_raster_scan_cu_depth[PA_BLOCK_MAX_COUNT] =
{    0,
    1, 1,
    1, 1,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3
};

static const uint32_t pa_parent_block_index[85] = {
0, 0, 0, 2, 2, 2, 2, 0, 7, 7, 7, 7, 0, 12, 12, 12, 12, 0, 17, 17, 17, 17, 0, 0,
23, 23, 23, 23, 0, 28, 28, 28, 28, 0, 33, 33, 33, 33, 0, 38, 38, 38, 38, 0, 0,
44, 44, 44, 44, 0, 49, 49, 49, 49, 0, 54, 54, 54, 54, 0, 59, 59, 59, 59, 0, 0,
65, 65, 65, 65, 0, 70, 70, 70, 70, 0, 75, 75, 75, 75, 0, 80, 80, 80, 80 };

static const uint32_t raster_scan_to_md_scan[PA_BLOCK_MAX_COUNT] =
{
    0,
    1, 22,
    43, 64,
    2, 7, 23, 28,
    12, 17, 33, 38,
    44, 49, 65, 70,
    54, 59, 75, 80,
    3, 4, 8, 9, 24, 25, 29, 30,
    5, 6, 10, 11, 26, 27, 31, 32,
    13, 14, 18, 19, 34, 35, 39, 40,
    15, 16, 20, 21, 36, 37, 41, 42,
    45, 46, 50, 51, 66, 67, 71, 72,
    47, 48, 52, 53, 68, 69, 73, 74,
    55, 56, 60, 61, 76, 77, 81, 82,
    57, 58, 62, 63, 78, 79, 83, 84
};

// VP9 lookup tables

typedef enum EpRasterScanBlockIndex
{
    // 64x64
    EP_RASTER_SCAN_BLOCK_INDEX_64x64 = 0,
    EP_RASTER_SCAN_BLOCK_INDEX_64x32_0 = 1,
    EP_RASTER_SCAN_BLOCK_INDEX_64x32_1 = 2,
    EP_RASTER_SCAN_BLOCK_INDEX_32x64_0 = 3,
    EP_RASTER_SCAN_BLOCK_INDEX_32x64_1 = 4,
    // 32x32
    EP_RASTER_SCAN_BLOCK_INDEX_32x32_0 = 5,
    EP_RASTER_SCAN_BLOCK_INDEX_32x32_1 = 6,
    EP_RASTER_SCAN_BLOCK_INDEX_32x32_2 = 7,
    EP_RASTER_SCAN_BLOCK_INDEX_32x32_3 = 8,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_0 = 9,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_1 = 10,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_2 = 11,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_3 = 12,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_4 = 13,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_5 = 14,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_6 = 15,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_7 = 16,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_0 = 17,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_1 = 18,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_2 = 19,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_3 = 20,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_4 = 21,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_5 = 22,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_6 = 23,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_7 = 24,
    // 16x16
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_0 = 25,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_1 = 26,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_2 = 27,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_3 = 28,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_4 = 29,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_5 = 30,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_6 = 31,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_7 = 32,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_8 = 33,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_9 = 34,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_10 = 35,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_11 = 36,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_12 = 37,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_13 = 38,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_14 = 39,
    EP_RASTER_SCAN_BLOCK_INDEX_16x16_15 = 40,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_0 = 41,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_1 = 42,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_2 = 43,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_3 = 44,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_4 = 45,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_5 = 46,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_6 = 47,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_7 = 48,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_8 = 49,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_9 = 50,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_10 = 51,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_11 = 52,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_12 = 53,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_13 = 54,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_14 = 55,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_15 = 56,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_16 = 57,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_17 = 58,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_18 = 59,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_19 = 60,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_20 = 61,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_21 = 62,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_22 = 63,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_23 = 64,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_24 = 65,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_25 = 66,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_26 = 67,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_27 = 68,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_28 = 69,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_29 = 70,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_30 = 71,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_31 = 72,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_0 = 73,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_1 = 74,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_2 = 75,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_3 = 76,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_4 = 77,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_5 = 78,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_6 = 79,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_7 = 80,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_8 = 81,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_9 = 82,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_10 = 83,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_11 = 84,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_12 = 85,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_13 = 86,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_14 = 87,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_15 = 88,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_16 = 89,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_17 = 90,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_18 = 91,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_19 = 92,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_20 = 93,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_21 = 94,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_22 = 95,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_23 = 96,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_24 = 97,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_25 = 98,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_26 = 99,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_27 = 100,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_28 = 101,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_29 = 102,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_30 = 103,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_31 = 104,
    // 8x8
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_0 = 105,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_1 = 106,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_2 = 107,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_3 = 108,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_4 = 109,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_5 = 110,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_6 = 111,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_7 = 112,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_8 = 113,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_9 = 114,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_10 = 115,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_11 = 116,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_12 = 117,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_13 = 118,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_14 = 119,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_15 = 120,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_16 = 121,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_17 = 122,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_18 = 123,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_19 = 124,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_20 = 125,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_21 = 126,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_22 = 127,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_23 = 128,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_24 = 129,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_25 = 130,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_26 = 131,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_27 = 132,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_28 = 133,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_29 = 134,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_30 = 135,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_31 = 136,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_32 = 137,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_33 = 138,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_34 = 139,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_35 = 140,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_36 = 141,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_37 = 142,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_38 = 143,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_39 = 144,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_40 = 145,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_41 = 146,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_42 = 147,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_43 = 148,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_44 = 149,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_45 = 150,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_46 = 151,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_47 = 152,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_48 = 153,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_49 = 154,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_50 = 155,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_51 = 156,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_52 = 157,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_53 = 158,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_54 = 159,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_55 = 160,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_56 = 161,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_57 = 162,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_58 = 163,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_59 = 164,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_60 = 165,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_61 = 166,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_62 = 167,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_63 = 168,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_0 = 169,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_1 = 170,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_2 = 171,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_3 = 172,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_4 = 173,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_5 = 174,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_6 = 175,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_7 = 176,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_8 = 177,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_9 = 178,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_10 = 179,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_11 = 180,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_12 = 181,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_13 = 182,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_14 = 183,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_15 = 184,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_16 = 185,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_17 = 186,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_18 = 187,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_19 = 188,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_20 = 189,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_21 = 190,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_22 = 191,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_23 = 192,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_24 = 193,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_25 = 194,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_26 = 195,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_27 = 196,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_28 = 197,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_29 = 198,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_30 = 199,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_31 = 200,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_32 = 201,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_33 = 202,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_34 = 203,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_35 = 204,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_36 = 205,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_37 = 206,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_38 = 207,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_39 = 208,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_40 = 209,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_41 = 210,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_42 = 211,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_43 = 212,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_44 = 213,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_45 = 214,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_46 = 215,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_47 = 216,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_48 = 217,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_49 = 218,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_50 = 219,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_51 = 220,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_52 = 221,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_53 = 222,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_54 = 223,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_55 = 224,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_56 = 225,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_57 = 226,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_58 = 227,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_59 = 228,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_60 = 229,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_61 = 230,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_62 = 231,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_63 = 232,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_64 = 233,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_65 = 234,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_66 = 235,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_67 = 236,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_68 = 237,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_69 = 238,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_70 = 239,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_71 = 240,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_72 = 241,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_73 = 242,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_74 = 243,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_75 = 244,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_76 = 245,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_77 = 246,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_78 = 247,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_79 = 248,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_80 = 249,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_81 = 250,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_82 = 251,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_83 = 252,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_84 = 253,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_85 = 254,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_86 = 255,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_87 = 256,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_88 = 257,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_89 = 258,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_90 = 259,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_91 = 260,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_92 = 261,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_93 = 262,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_94 = 263,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_95 = 264,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_96 = 265,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_97 = 266,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_98 = 267,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_99 = 268,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_100 = 269,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_101 = 270,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_102 = 271,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_103 = 272,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_104 = 273,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_105 = 274,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_106 = 275,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_107 = 276,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_108 = 277,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_109 = 278,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_110 = 279,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_111 = 280,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_112 = 281,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_113 = 282,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_114 = 283,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_115 = 284,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_116 = 285,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_117 = 286,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_118 = 287,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_119 = 288,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_120 = 289,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_121 = 290,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_122 = 291,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_123 = 292,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_124 = 293,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_125 = 294,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_126 = 295,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_127 = 296,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_0 = 297,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_1 = 298,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_2 = 299,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_3 = 300,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_4 = 301,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_5 = 302,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_6 = 303,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_7 = 304,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_8 = 305,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_9 = 306,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_10 = 307,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_11 = 308,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_12 = 309,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_13 = 310,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_14 = 311,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_15 = 312,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_16 = 313,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_17 = 314,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_18 = 315,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_19 = 316,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_20 = 317,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_21 = 318,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_22 = 319,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_23 = 320,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_24 = 321,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_25 = 322,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_26 = 323,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_27 = 324,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_28 = 325,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_29 = 326,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_30 = 327,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_31 = 328,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_32 = 329,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_33 = 330,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_34 = 331,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_35 = 332,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_36 = 333,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_37 = 334,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_38 = 335,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_39 = 336,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_40 = 337,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_41 = 338,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_42 = 339,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_43 = 340,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_44 = 341,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_45 = 342,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_46 = 343,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_47 = 344,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_48 = 345,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_49 = 346,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_50 = 347,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_51 = 348,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_52 = 349,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_53 = 350,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_54 = 351,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_55 = 352,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_56 = 353,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_57 = 354,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_58 = 355,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_59 = 356,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_60 = 357,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_61 = 358,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_62 = 359,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_63 = 360,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_64 = 361,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_65 = 362,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_66 = 363,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_67 = 364,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_68 = 365,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_69 = 366,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_70 = 367,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_71 = 368,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_72 = 369,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_73 = 370,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_74 = 371,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_75 = 372,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_76 = 373,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_77 = 374,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_78 = 375,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_79 = 376,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_80 = 377,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_81 = 378,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_82 = 379,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_83 = 380,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_84 = 381,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_85 = 382,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_86 = 383,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_87 = 384,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_88 = 385,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_89 = 386,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_90 = 387,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_91 = 388,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_92 = 389,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_93 = 390,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_94 = 391,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_95 = 392,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_96 = 393,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_97 = 394,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_98 = 395,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_99 = 396,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_100 = 397,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_101 = 398,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_102 = 399,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_103 = 400,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_104 = 401,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_105 = 402,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_106 = 403,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_107 = 404,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_108 = 405,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_109 = 406,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_110 = 407,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_111 = 408,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_112 = 409,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_113 = 410,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_114 = 411,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_115 = 412,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_116 = 413,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_117 = 414,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_118 = 415,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_119 = 416,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_120 = 417,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_121 = 418,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_122 = 419,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_123 = 420,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_124 = 421,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_125 = 422,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_126 = 423,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_127 = 424,
    // 4x4
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_0 = 425,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_1 = 426,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_2 = 427,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_3 = 428,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_4 = 429,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_5 = 430,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_6 = 431,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_7 = 432,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_8 = 433,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_9 = 434,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_10 = 435,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_11 = 436,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_12 = 437,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_13 = 438,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_14 = 439,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_15 = 440,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_16 = 441,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_17 = 442,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_18 = 443,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_19 = 444,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_20 = 445,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_21 = 446,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_22 = 447,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_23 = 448,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_24 = 449,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_25 = 450,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_26 = 451,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_27 = 452,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_28 = 453,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_29 = 454,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_30 = 455,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_31 = 456,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_32 = 457,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_33 = 458,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_34 = 459,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_35 = 460,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_36 = 461,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_37 = 462,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_38 = 463,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_39 = 464,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_40 = 465,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_41 = 466,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_42 = 467,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_43 = 468,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_44 = 469,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_45 = 470,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_46 = 471,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_47 = 472,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_48 = 473,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_49 = 474,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_50 = 475,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_51 = 476,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_52 = 477,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_53 = 478,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_54 = 479,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_55 = 480,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_56 = 481,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_57 = 482,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_58 = 483,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_59 = 484,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_60 = 485,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_61 = 486,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_62 = 487,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_63 = 488,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_64 = 489,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_65 = 490,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_66 = 491,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_67 = 492,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_68 = 493,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_69 = 494,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_70 = 495,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_71 = 496,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_72 = 497,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_73 = 498,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_74 = 499,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_75 = 500,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_76 = 501,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_77 = 502,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_78 = 503,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_79 = 504,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_80 = 505,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_81 = 506,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_82 = 507,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_83 = 508,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_84 = 509,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_85 = 510,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_86 = 511,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_87 = 512,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_88 = 513,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_89 = 514,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_90 = 515,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_91 = 516,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_92 = 517,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_93 = 518,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_94 = 519,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_95 = 520,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_96 = 521,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_97 = 522,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_98 = 523,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_99 = 524,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_100 = 525,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_101 = 526,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_102 = 527,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_103 = 528,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_104 = 529,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_105 = 530,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_106 = 531,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_107 = 532,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_108 = 533,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_109 = 534,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_110 = 535,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_111 = 536,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_112 = 537,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_113 = 538,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_114 = 539,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_115 = 540,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_116 = 541,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_117 = 542,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_118 = 543,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_119 = 544,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_120 = 545,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_121 = 546,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_122 = 547,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_123 = 548,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_124 = 549,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_125 = 550,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_126 = 551,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_127 = 552,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_128 = 553,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_129 = 554,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_130 = 555,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_131 = 556,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_132 = 557,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_133 = 558,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_134 = 559,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_135 = 560,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_136 = 561,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_137 = 562,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_138 = 563,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_139 = 564,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_140 = 565,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_141 = 566,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_142 = 567,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_143 = 568,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_144 = 569,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_145 = 570,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_146 = 571,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_147 = 572,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_148 = 573,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_149 = 574,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_150 = 575,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_151 = 576,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_152 = 577,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_153 = 578,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_154 = 579,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_155 = 580,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_156 = 581,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_157 = 582,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_158 = 583,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_159 = 584,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_160 = 585,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_161 = 586,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_162 = 587,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_163 = 588,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_164 = 589,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_165 = 590,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_166 = 591,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_167 = 592,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_168 = 593,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_169 = 594,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_170 = 595,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_171 = 596,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_172 = 597,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_173 = 598,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_174 = 599,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_175 = 600,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_176 = 601,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_177 = 602,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_178 = 603,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_179 = 604,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_180 = 605,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_181 = 606,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_182 = 607,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_183 = 608,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_184 = 609,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_185 = 610,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_186 = 611,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_187 = 612,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_188 = 613,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_189 = 614,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_190 = 615,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_191 = 616,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_192 = 617,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_193 = 618,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_194 = 619,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_195 = 620,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_196 = 621,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_197 = 622,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_198 = 623,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_199 = 624,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_200 = 625,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_201 = 626,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_202 = 627,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_203 = 628,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_204 = 629,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_205 = 630,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_206 = 631,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_207 = 632,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_208 = 633,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_209 = 634,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_210 = 635,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_211 = 636,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_212 = 637,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_213 = 638,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_214 = 639,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_215 = 640,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_216 = 641,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_217 = 642,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_218 = 643,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_219 = 644,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_220 = 645,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_221 = 646,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_222 = 647,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_223 = 648,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_224 = 649,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_225 = 650,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_226 = 651,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_227 = 652,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_228 = 653,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_229 = 654,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_230 = 655,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_231 = 656,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_232 = 657,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_233 = 658,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_234 = 659,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_235 = 660,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_236 = 661,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_237 = 662,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_238 = 663,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_239 = 664,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_240 = 665,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_241 = 666,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_242 = 667,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_243 = 668,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_244 = 669,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_245 = 670,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_246 = 671,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_247 = 672,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_248 = 673,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_249 = 674,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_250 = 675,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_251 = 676,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_252 = 677,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_253 = 678,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_254 = 679,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_255 = 680
} EpRasterScanBlockIndex;

static const uint32_t ep_raster_scan_origin_x[EP_BLOCK_MAX_COUNT] =
{
    // 64x64
    0,
    // 64x32
    0,
    0,
    // 32x64
    0, 32,
    // 32x32
    0, 32,
    0, 32,
    // 32x16
    0, 32,
    0, 32,
    0, 32,
    0, 32,
    // 16x32
    0, 16, 32, 48,
    0, 16, 32, 48,
    // 16x16
    0, 16, 32, 48,
    0, 16, 32, 48,
    0, 16, 32, 48,
    0, 16, 32, 48,
    // 16x8
    0, 16, 32, 48,
    0, 16, 32, 48,
    0, 16, 32, 48,
    0, 16, 32, 48,
    0, 16, 32, 48,
    0, 16, 32, 48,
    0, 16, 32, 48,
    0, 16, 32, 48,
    // 8x16
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    // 8x8
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    // 8x4
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    0, 8, 16, 24, 32, 40, 48, 56,
    // 4x8
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    // 4x4
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    0, 4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60
};

static const uint32_t ep_raster_scan_origin_y[EP_BLOCK_MAX_COUNT] =
{
    // 64x64
    0,
    // 64x32
    0,
    32,
    // 32x64
    0, 0,
    // 32x32
     0,  0,
    32, 32,
    // 32x16
     0,  0,
    16, 16,
    32, 32,
    48, 48,
    // 16x32
     0,  0,  0,  0,
    32, 32, 32, 32,
    // 16x16
     0,  0,  0,  0,
    16, 16, 16, 16,
    32, 32, 32, 32,
    48, 48, 48, 48,
    // 16x8
     0,  0,  0,  0,
     8,  8,  8,  8,
    16, 16, 16, 16,
    24, 24, 24, 24,
    32, 32, 32, 32,
    40, 40, 40, 40,
    48, 48, 48, 48,
    56, 56, 56, 56,
    // 8x16
     0,  0,  0,  0,  0,  0,  0,  0,
    16, 16, 16, 16, 16, 16, 16, 16,
    32, 32, 32, 32, 32, 32, 32, 32,
    48, 48, 48, 48, 48, 48, 48, 48,
    // 8x8
     0,  0,  0,  0,  0,  0,  0,  0,
     8,  8,  8,  8,  8,  8,  8,  8,
    16, 16, 16, 16, 16, 16, 16, 16,
    24, 24, 24, 24, 24, 24, 24, 24,
    32, 32, 32, 32, 32, 32, 32, 32,
    40, 40, 40, 40, 40, 40, 40, 40,
    48, 48, 48, 48, 48, 48, 48, 48,
    56, 56, 56, 56, 56, 56, 56, 56,
    // 8x4
     0,  0,  0,  0,  0,  0,  0,  0,
     4,  4,  4,  4,  4,  4,  4,  4,
     8,  8,  8,  8,  8,  8,  8,  8,
    12, 12, 12, 12, 12, 12, 12, 12,
    16, 16, 16, 16, 16, 16, 16, 16,
    20, 20, 20, 20, 20, 20, 20, 20,
    24, 24, 24, 24, 24, 24, 24, 24,
    28, 28, 28, 28, 28, 28, 28, 28,
    32, 32, 32, 32, 32, 32, 32, 32,
    36, 36, 36, 36, 36, 36, 36, 36,
    40, 40, 40, 40, 40, 40, 40, 40,
    44, 44, 44, 44, 44, 44, 44, 44,
    48, 48, 48, 48, 48, 48, 48, 48,
    52, 52, 52, 52, 52, 52, 52, 52,
    56, 56, 56, 56, 56, 56, 56, 56,
    60, 60, 60, 60, 60, 60, 60, 60,
    //4x8
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    40, 40, 40, 40, 40, 40, 40, 40,    40, 40, 40, 40, 40, 40, 40, 40,
    48, 48, 48, 48, 48, 48, 48, 48,    48, 48, 48, 48, 48, 48, 48, 48,
    56, 56, 56, 56, 56, 56, 56, 56,    56, 56, 56, 56, 56, 56, 56, 56,
    //4x4
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
     8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
    40, 40, 40, 40, 40, 40, 40, 40,    40, 40, 40, 40, 40, 40, 40, 40,
    44, 44, 44, 44, 44, 44, 44, 44,    44, 44, 44, 44, 44, 44, 44, 44,
    48, 48, 48, 48, 48, 48, 48, 48,    48, 48, 48, 48, 48, 48, 48, 48,
    52, 52, 52, 52, 52, 52, 52, 52,    52, 52, 52, 52, 52, 52, 52, 52,
    56, 56, 56, 56, 56, 56, 56, 56,    56, 56, 56, 56, 56, 56, 56, 56,
    60, 60, 60, 60, 60, 60, 60, 60,    60, 60, 60, 60, 60, 60, 60, 60
};

static const uint32_t ep_raster_scan_block_width[EP_BLOCK_MAX_COUNT] =
{     // 64x64
    64,
    // 64x32
    64,
    64,
    // 32x64
    32, 32,
    // 32x32
    32, 32,
    32, 32,
    // 32x16
    32, 32,
    32, 32,
    32, 32,
    32, 32,
    // 16x32
    16, 16, 16, 16,
    16, 16, 16, 16,
    // 16x16
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    // 16x8
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    //8x16
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    // 8x8
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    // 8x4
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    // 4x8
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    //4x4
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
};

static const uint32_t ep_raster_scan_block_height[EP_BLOCK_MAX_COUNT] =
{     // 64x64
    64,
    // 64x32
    32,
    32,
    // 32x64
    64, 64,
    // 32x32
    32, 32,
    32, 32,
    // 32x16
    16, 16,
    16, 16,
    16, 16,
    16, 16,
    // 16x32
    32, 32, 32, 32,
    32, 32, 32, 32,
    // 16x16
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    // 16x8
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    // 8x16
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    // 8x8
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    // 8x4
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    // 4x8
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    // 4x4
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
};
static const uint32_t ep_raster_scan_block_bsize[EP_BLOCK_MAX_COUNT] =
{     // 64x64
    BLOCK_64X64,
    // 64x32
    BLOCK_64X32,
    BLOCK_64X32,
    // 32x64
    BLOCK_32X64, BLOCK_32X64,
    // 32x32
    BLOCK_32X32, BLOCK_32X32,
    BLOCK_32X32, BLOCK_32X32,
    // 32x16
    BLOCK_32X16, BLOCK_32X16,
    BLOCK_32X16, BLOCK_32X16,
    BLOCK_32X16, BLOCK_32X16,
    BLOCK_32X16, BLOCK_32X16,
    // 16x32
    BLOCK_16X32, BLOCK_16X32, BLOCK_16X32, BLOCK_16X32,
    BLOCK_16X32, BLOCK_16X32, BLOCK_16X32, BLOCK_16X32,
    // 16x16
    BLOCK_16X16, BLOCK_16X16, BLOCK_16X16, BLOCK_16X16,
    BLOCK_16X16, BLOCK_16X16, BLOCK_16X16, BLOCK_16X16,
    BLOCK_16X16, BLOCK_16X16, BLOCK_16X16, BLOCK_16X16,
    BLOCK_16X16, BLOCK_16X16, BLOCK_16X16, BLOCK_16X16,
    // 16x8
    BLOCK_16X8, BLOCK_16X8, BLOCK_16X8, BLOCK_16X8,
    BLOCK_16X8, BLOCK_16X8, BLOCK_16X8, BLOCK_16X8,
    BLOCK_16X8, BLOCK_16X8, BLOCK_16X8, BLOCK_16X8,
    BLOCK_16X8, BLOCK_16X8, BLOCK_16X8, BLOCK_16X8,
    BLOCK_16X8, BLOCK_16X8, BLOCK_16X8, BLOCK_16X8,
    BLOCK_16X8, BLOCK_16X8, BLOCK_16X8, BLOCK_16X8,
    BLOCK_16X8, BLOCK_16X8, BLOCK_16X8, BLOCK_16X8,
    BLOCK_16X8, BLOCK_16X8, BLOCK_16X8, BLOCK_16X8,
    // 8x16
    BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16,
    BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16,
    BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16,
    BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16, BLOCK_8X16,
    // 8x8
    BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8,
    BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8,
    BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8,
    BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8,
    BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8,
    BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8,
    BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8,
    BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8, BLOCK_8X8,
    // 8x4
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4, BLOCK_8X4,
    // 4x8
    BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8,
    BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8,
    BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8,
    BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8,
    BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8,
    BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8,
    BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8,
    BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8, BLOCK_4X8,
    // 4x4
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
    BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4, BLOCK_4X4,
};

static const uint32_t ep_raster_scan_block_depth[EP_BLOCK_MAX_COUNT] =
{     // 64x64
    0,
    // 64x32
    0,
    0,
    // 32x64
    0, 0,
    // 32x32
    1, 1,
    1, 1,
    // 32x16
    1, 1,
    1, 1,
    1, 1,
    1, 1,
    // 16x32
    1, 1, 1, 1,
    1, 1, 1, 1,
    // 16x16
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    // 16x8
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    2, 2, 2, 2,
    // 8x16
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    // 8x8
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    // 8x4
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    // 4x8
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    // 4x4
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};

static const uint32_t ep_scan_to_raster_scan[EP_BLOCK_MAX_COUNT] =
{

    EP_RASTER_SCAN_BLOCK_INDEX_64x64 ,
    EP_RASTER_SCAN_BLOCK_INDEX_64x32_0,
    EP_RASTER_SCAN_BLOCK_INDEX_64x32_1,
    EP_RASTER_SCAN_BLOCK_INDEX_32x64_0,
    EP_RASTER_SCAN_BLOCK_INDEX_32x64_1,

    EP_RASTER_SCAN_BLOCK_INDEX_32x32_0,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_0,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_2,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_0,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_1,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_0,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_0,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_4,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_0,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_1,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_0,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_0,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_8,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_0,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_1,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_0,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_1,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_16,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_17,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_1,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_1,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_9,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_2,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_3,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_2,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_3,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_18,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_19,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_8,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_16,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_24,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_16,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_17,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_32,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_33,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_48,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_49,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_9,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_17,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_25,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_18,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_19,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_34,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_35,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_50,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_51,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_1,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_1,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_5,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_2,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_3,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_2,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_2,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_10,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_4,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_5,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_4,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_5,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_20,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_21,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_3,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_3,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_11,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_6,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_7,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_6,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_7,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_22,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_23,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_10,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_18,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_26,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_20,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_21,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_36,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_37,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_52,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_53,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_11,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_19,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_27,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_22,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_23,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_38,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_39,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_54,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_55,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_4,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_8,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_12,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_8,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_9,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_16,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_32,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_40,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_32,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_33,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_64,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_65,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_80,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_81,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_17,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_33,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_41,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_34,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_35,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_66,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_67,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_82,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_83,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_24,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_48,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_56,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_48,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_49,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_96,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_97,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_112,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_113,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_25,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_49,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_57,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_50,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_51,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_98,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_99,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_114,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_115,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_5,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_9,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_13,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_10,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_11,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_18,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_34,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_42,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_36,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_37,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_68,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_69,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_84,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_85,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_19,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_35,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_53,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_38,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_39,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_70,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_71,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_86,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_87,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_26,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_50,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_58,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_52,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_53,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_100,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_101,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_116,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_117,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_27,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_51,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_59,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_54,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_55,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_102,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_103,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_118,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_119,

    EP_RASTER_SCAN_BLOCK_INDEX_32x32_1,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_1,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_3,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_2,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_3,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_2,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_2,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_6,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_4,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_5,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_4,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_4,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_12,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_8,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_9,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_8,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_9,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_24,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_25,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_5,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_5,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_13,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_10,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_11,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_10,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_11,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_26,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_27,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_12,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_20,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_28,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_24,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_25,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_40,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_41,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_56,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_57,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_13,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_21,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_29,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_26,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_27,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_42,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_43,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_58,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_59,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_3,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_3,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_7,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_6,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_7,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_6,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_6,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_14,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_12,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_13,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_12,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_13,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_28,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_29,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_7,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_7,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_15,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_14,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_15,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_14,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_15,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_30,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_31,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_14,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_22,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_30,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_28,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_29,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_44,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_45,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_60,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_61,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_15,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_23,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_31,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_30,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_31,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_46,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_47,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_62,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_63,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_6,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_10,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_14,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_12,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_13,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_20,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_36,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_44,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_40,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_41,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_72,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_73,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_88,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_89,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_21,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_37,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_45,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_42,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_43,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_74,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_75,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_90,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_91,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_28,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_52,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_60,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_56,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_57,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_104,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_105,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_120,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_121,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_29,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_53,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_61,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_58,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_59,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_106,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_107,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_122,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_123,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_7,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_11,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_15,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_14,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_15,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_22,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_38,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_46,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_44,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_45,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_76,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_77,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_92,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_93,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_23,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_39,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_57,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_46,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_47,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_78,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_79,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_94,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_95,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_30,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_54,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_62,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_60,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_61,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_108,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_109,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_124,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_125,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_31,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_55,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_63,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_62,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_63,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_110,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_111,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_126,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_127,

    EP_RASTER_SCAN_BLOCK_INDEX_32x32_2,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_4,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_6,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_4,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_5,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_8,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_16,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_20,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_16,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_17,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_32,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_64,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_72,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_64,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_65,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_128,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_129,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_144,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_145,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_33,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_65,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_73,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_66,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_67,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_130,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_131,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_146,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_147,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_40,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_80,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_88,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_80,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_81,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_160,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_161,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_176,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_177,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_41,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_81,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_89,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_82,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_83,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_162,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_163,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_178,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_179,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_9,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_17,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_21,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_18,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_19,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_34,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_66,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_74,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_68,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_69,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_132,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_133,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_148,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_149,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_35,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_67,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_75,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_70,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_71,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_134,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_135,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_150,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_151,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_42,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_82,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_90,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_84,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_85,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_164,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_165,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_180,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_181,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_43,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_83,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_91,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_86,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_87,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_166,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_167,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_182,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_183,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_12,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_24,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_28,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_24,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_25,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_48,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_96,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_104,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_96,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_97,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_192,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_193,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_208,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_209,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_49,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_97,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_105,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_98,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_99,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_194,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_195,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_210,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_211,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_56,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_112,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_120,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_112,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_113,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_224,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_225,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_240,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_241,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_57,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_113,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_121,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_114,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_115,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_226,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_227,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_242,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_243,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_13,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_25,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_29,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_26,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_27,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_50,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_98,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_106,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_100,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_101,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_196,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_197,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_212,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_213,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_51,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_99,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_117,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_102,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_103,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_198,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_199,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_214,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_215,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_58,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_114,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_122,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_116,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_117,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_228,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_229,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_244,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_245,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_59,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_115,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_123,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_118,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_119,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_230,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_231,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_246,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_247,

    EP_RASTER_SCAN_BLOCK_INDEX_32x32_3,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_5,
    EP_RASTER_SCAN_BLOCK_INDEX_32x16_7,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_6,
    EP_RASTER_SCAN_BLOCK_INDEX_16x32_7,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_10,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_18,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_22,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_20,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_21,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_36,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_68,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_76,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_72,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_73,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_136,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_137,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_152,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_153,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_37,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_69,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_77,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_74,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_75,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_138,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_139,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_154,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_155,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_44,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_84,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_92,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_88,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_89,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_168,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_169,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_184,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_185,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_45,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_85,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_93,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_90,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_91,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_170,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_171,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_186,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_187,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_11,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_19,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_23,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_22,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_23,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_38,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_70,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_78,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_76,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_77,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_140,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_141,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_156,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_157,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_39,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_71,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_79,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_78,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_79,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_142,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_143,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_158,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_159,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_46,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_86,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_94,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_92,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_93,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_172,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_173,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_188,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_189,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_47,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_87,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_95,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_94,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_95,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_174,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_175,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_190,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_191,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_14,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_26,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_30,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_28,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_29,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_52,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_100,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_108,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_104,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_105,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_200,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_201,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_216,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_217,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_53,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_101,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_109,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_106,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_107,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_202,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_203,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_218,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_219,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_60,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_116,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_124,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_120,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_121,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_232,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_233,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_248,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_249,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_61,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_117,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_125,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_122,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_123,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_234,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_235,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_250,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_251,

    EP_RASTER_SCAN_BLOCK_INDEX_16x16_15,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_27,
    EP_RASTER_SCAN_BLOCK_INDEX_16x8_31,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_30,
    EP_RASTER_SCAN_BLOCK_INDEX_8x16_31,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_54,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_102,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_110,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_108,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_109,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_204,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_205,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_220,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_221,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_55,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_103,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_121,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_110,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_111,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_206,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_207,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_222,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_223,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_62,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_118,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_126,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_124,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_125,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_236,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_237,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_252,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_253,
    EP_RASTER_SCAN_BLOCK_INDEX_8x8_63,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_119,
    EP_RASTER_SCAN_BLOCK_INDEX_8x4_127,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_126,
    EP_RASTER_SCAN_BLOCK_INDEX_4x8_127,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_238,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_239,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_254,
    EP_RASTER_SCAN_BLOCK_INDEX_4x4_255,

};

static const uint16_t pa_to_ep_block_index[PA_BLOCK_MAX_COUNT] =
{
    0,
    5,
    10,
    15, 24,33,42,
    51,
    56, 65, 74,83,
    92,
    97,106,115,124,
    133,
    138,147,156,165,
    174,
    179,
    184,193,202,211,
    220,
    225,234,243,252,
    261,
    266,275,284,293,
    302,
    307,316,325,334,
    343,
    348,
    353,362,371,380,
    389,
    394,403,412,421,
    430,
    435,444,453,462,
    471,
    476,485,494,503,
    512,
    517,
    522,531,540,549,
    558,
    563,572,581,590,
    599,
    604,613,622,631,
    640,
    645,654,663,672,

};

#define EP_CU_DEPTH 5
static const uint16_t ep_start_cu_index_per_depth[EP_CU_DEPTH] = { 0, 5, 10, 15, 20 };
static const uint16_t ep_intra_depth_offset[EP_CU_DEPTH] = { 681, 169, 41, 9, 1};
static const uint16_t ep_inter_depth_offset = 5;

static const uint32_t MD_SCAN_TO_RASTER_SCAN[PA_BLOCK_MAX_COUNT] =
{
    0,
    1,
    5, 21, 22, 29, 30,
    6, 23, 24, 31, 32,
    9, 37, 38, 45, 46,
    10, 39, 40, 47, 48,
    2,
    7, 25, 26, 33, 34,
    8, 27, 28, 35, 36,
    11, 41, 42, 49, 50,
    12, 43, 44, 51, 52,
    3,
    13, 53, 54, 61, 62,
    14, 55, 56, 63, 64,
    17, 69, 70, 77, 78,
    18, 71, 72, 79, 80,
    4,
    15, 57, 58, 65, 66,
    16, 59, 60, 67, 68,
    19, 73, 74, 81, 82,
    20, 75, 76, 83, 84
};

static const uint32_t RASTER_SCAN_CU_PARENT_INDEX[PA_BLOCK_MAX_COUNT] =
{ 0,
0, 0,
0, 0,
1, 1, 2, 2,
1, 1, 2, 2,
3, 3, 4, 4,
3, 3, 4, 4,
5, 5, 6, 6, 7, 7, 8, 8,
5, 5, 6, 6, 7, 7, 8, 8,
9, 9, 10, 10, 11, 11, 12, 12,
9, 9, 10, 10, 11, 11, 12, 12,
13, 13, 14, 14, 15, 15, 16, 16,
13, 13, 14, 14, 15, 15, 16, 16,
17, 17, 18, 18, 19, 19, 20, 20,
17, 17, 18, 18, 19, 19, 20, 20
};

static const uint32_t md_scan_to_ois_32x32_scan[PA_BLOCK_MAX_COUNT] =
{
/*0  */0,
/*1  */0,
/*2  */0,
/*3  */0,
/*4  */0,
/*5  */0,
/*6  */0,
/*7  */0,
/*8  */0,
/*9  */0,
/*10 */0,
/*11 */0,
/*12 */0,
/*13 */0,
/*14 */0,
/*15 */0,
/*16 */0,
/*17 */0,
/*18 */0,
/*19 */0,
/*20 */0,
/*21 */0,
/*22 */1,
/*23 */1,
/*24 */1,
/*25 */1,
/*26 */1,
/*27 */1,
/*28 */1,
/*29 */1,
/*30 */1,
/*31 */1,
/*32 */1,
/*33 */1,
/*34 */1,
/*35 */1,
/*36 */1,
/*37 */1,
/*38 */1,
/*39 */1,
/*40 */1,
/*41 */1,
/*42 */1,
/*43 */2,
/*44 */2,
/*45 */2,
/*46 */2,
/*47 */2,
/*48 */2,
/*49 */2,
/*50 */2,
/*51 */2,
/*52 */2,
/*53 */2,
/*54 */2,
/*55 */2,
/*56 */2,
/*57 */2,
/*58 */2,
/*59 */2,
/*60 */2,
/*61 */2,
/*62 */2,
/*63 */2,
/*64 */3,
/*65 */3,
/*66 */3,
/*67 */3,
/*68 */3,
/*69 */3,
/*70 */3,
/*71 */3,
/*72 */3,
/*73 */3,
/*74 */3,
/*75 */3,
/*76 */3,
/*77 */3,
/*78 */3,
/*79 */3,
/*80 */3,
/*81 */3,
/*82 */3,
/*83 */3,
/*84 */3,
};
/******************************************************************************
                            ME/HME settings VMAF
*******************************************************************************/
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11
static const uint8_t enable_hme_level0_flag_vmaf[5][MAX_SUPPORTED_MODES] = {
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_720P_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080i_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080p_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_4K_RANGE
};
static const uint16_t hme_level0_total_search_area_width_vmaf[5][MAX_SUPPORTED_MODES] = {
    {  48,   48,    48,   48,   48,   48,   48,   48,   32,   32,   32,   32,   32 },
    {  64,   64,    64,   64,   64,   64,   64,   48,   48,   48,   48,   48,   48 },
    {  96,   96,    96,   96,   96,   96,   96,   64,   48,   48,   48,   48,   48 },
    {  96,   96,    96,   96,   96,   96,   96,   64,   48,   48,   48,   48,   48 },
    { 128,  128,   128,   64,   64,   64,   64,   64,   64,   64,   64,   64,   64 }
};

static const uint16_t hme_level0_search_area_in_width_array_left_vmaf[5][MAX_SUPPORTED_MODES] = {
    {  24,   24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16,   16 },
    {  32,   32,   32,   32,   32,   32,   32,   24,   24,   24,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   48,   48,   32,   24,   24,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   48,   48,   32,   24,   24,   24,   24,   24 },
    {  64,   64,   64,   32,   32,   32,   32,   32,   32,   32,   32,   32,   32 }
};
static const uint16_t hme_level0_search_area_in_width_array_right_vmaf[5][MAX_SUPPORTED_MODES] = {
    {  24,   24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16,   16 },
    {  32,   32,   32,   32,   32,   32,   32,   24,   24,   24,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   48,   48,   32,   24,   24,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   48,   48,   32,   24,   24,   24,   24,   24 },
    {  64,   64,   64,   32,   32,   32,   32,   32,   32,   32,   32,   32,   32 }
};
static const uint16_t hme_level0_total_search_area_height_vmaf[5][MAX_SUPPORTED_MODES] = {
    {  40,   40,   40,   40,   40,   40,   40,   32,   24,   24,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   48,   48,   40,   32,   32,   32,   32,   32 },
    {  48,   48,   48,   48,   48,   48,   48,   32,   32,   32,   32,   32,   32 },
    {  48,   48,   48,   48,   48,   48,   48,   48,   40,   40,   40,   40,   40 },
    {  80,   80,   80,   32,   32,   32,   32,   32,   32,   32,   32,   32,   32 }
};
static const uint16_t hme_level0_search_area_in_height_array_top_vmaf[5][MAX_SUPPORTED_MODES] = {
    {  20,   20,   20,   20,   20,   20,   20,   16,   12,   12,   12,   12,   12 },
    {  24,   24,   24,   24,   24,   24,   24,   20,   16,   16,   16,   16,   16 },
    {  24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16,   16,   16 },
    {  24,   24,   24,   24,   24,   24,   24,   24,   20,   20,   20,   20,   20 },
    {  40,   40,   40,   16,   16,   16,   16,   16,   16,   16,   16,   16,   16 }
};
static const uint16_t hme_level0_search_area_in_height_array_bottom_vmaf[5][MAX_SUPPORTED_MODES] = {
    {  20,   20,   20,   20,   20,   20,   20,   16,   12,   12,   12,   12,   12 },
    {  24,   24,   24,   24,   24,   24,   24,   20,   16,   16,   16,   16,   16 },
    {  24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16,   16,   16 },
    {  24,   24,   24,   24,   24,   24,   24,   24,   20,   20,   20,   20,   20 },
    {  40,   40,   40,   16,   16,   16,   16,   16,   16,   16,   16,   16,   16 }
};

// HME LEVEL 1
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11
static const uint8_t enable_hme_level1_flag_vmaf[5][MAX_SUPPORTED_MODES] = {
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_720P_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080i_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080p_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0 }       // INPUT_SIZE_4K_RANGE
};
static const uint16_t hme_level1_search_area_in_width_array_left_vmaf[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,    8,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
    {  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level1_search_area_in_width_array_right_vmaf[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,    8,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
    {  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level1_search_area_in_height_array_top_vmaf[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,    8,    8,    8,    8,    8,    4,    4,    2,    2,    2 },
    {  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,    8,    8,    8,    8,    4,    2,    2,    2,    2,    2 },
    {  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level1_search_area_in_height_array_bottom_vmaf[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,    8,    8,    8,    8,    8,    4,    4,    2,    2,    2 },
    {  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,    8,    8,    8,    8,    4,    2,    2,    2,    2,    2 },
    {  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
// HME LEVEL 2
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11
static const uint8_t enable_hme_level2_flag_vmaf[5][MAX_SUPPORTED_MODES] = {
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
    {   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0 },      // INPUT_SIZE_720P_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0 },      // INPUT_SIZE_1080i_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0 },      // INPUT_SIZE_1080p_RANGE
    {   1,    1,    1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }       // INPUT_SIZE_4K_RANGE
};
static const uint16_t hme_level2_search_area_in_width_array_left_vmaf[5][MAX_SUPPORTED_MODES] = {
    {   8,    8,    8,    4,    4,    4,    4,    4,    4,    4,    4,    4,    4 },
    {   8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 },
    {   8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 },
    {   8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 },
    {   8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level2_search_area_in_width_array_right_vmaf[5][MAX_SUPPORTED_MODES] = {
    {   8,    8,    8,    4,    4,    4,    4,    4,    4,    4,    4,    4,    4 },
    {   8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 },
    {   8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 },
    {   8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 },
    {   8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level2_search_area_in_height_array_top_vmaf[5][MAX_SUPPORTED_MODES] = {
    {   8,    8,    8,    4,    4,    4,    4,    4,    2,    2,    2,    2,    2 },
    {   8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0,    0 },
    {   8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0,    0 },
    {   8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0,    0 },
    {   8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level2_search_area_in_height_array_bottom_vmaf[5][MAX_SUPPORTED_MODES] = {
    {   8,    8,    8,    4,    4,    4,    4,    4,    2,    2,    2,    2,    2 },
    {   8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0,    0 },
    {   8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0,    0 },
    {   8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0,    0 },
    {   8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};

static const uint8_t search_area_width_vmaf[5][MAX_SUPPORTED_MODES] = {
    {  64,   64,   64,   16,   16,   16,   16,   16,   16,   16,   16,   16,   16 },
    {  64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8,    8 },
    {  64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8,    8 },
    {  64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8,    8 },
    {  64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8,    8 }
};

static const uint8_t search_area_height_vmaf[5][MAX_SUPPORTED_MODES] = {
    {  64,   64,   64,   16,   16,   16,   16,    9,    7,    7,    7,    7,    7 },
    {  64,   64,   64,   16,   16,   16,   16,    9,    7,    7,    7,    7,    7 },
    {  64,   64,   64,   16,   16,   16,   16,    7,    7,    7,    7,    7,    7 },
    {  64,   64,   64,   16,   16,   16,   16,    9,    7,    7,    7,    7,    7 },
    {  64,   64,   64,    9,    9,    9,    9,    9,    7,    7,    7,    7,    7 }
};
/******************************************************************************
                            ME/HME settings OQ
*******************************************************************************/
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11
static const uint8_t enable_hme_level0_flag_oq[5][MAX_SUPPORTED_MODES] = {
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_720P_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080i_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080p_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_4K_RANGE
};
static const uint16_t hme_level0_total_search_area_width_oq[5][MAX_SUPPORTED_MODES] = {
    {  48,   48,   48,   48,   48,   48,   48,   48,   48,   32,   32,   32,   32 },
    {  64,   64,   64,   64,   64,   64,   64,   48,   48,   48,   48,   48,   48 },
    {  96,   96,   96,   96,   96,   96,   96,   64,   64,   48,   48,   48,   48 },
    {  96,   96,   96,   96,   96,   96,   96,   64,   64,   48,   48,   48,   48 },
    { 128,  128,  128,  128,   64,   64,   64,   64,   64,   64,   64,   64,   64 }
};

static const uint16_t hme_level0_search_area_in_width_array_left_oq[5][MAX_SUPPORTED_MODES] = {
    {  24,   24,   24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16 },
    {  32,   32,   32,   32,   32,   32,   32,   24,   24,   24,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   48,   48,   32,   32,   24,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   48,   48,   32,   32,   24,   24,   24,   24 },
    {  64,   64,   64,   64,   32,   32,   32,   32,   32,   32,   32,   32,   32 }
};
static const uint16_t hme_level0_search_area_in_width_array_right_oq[5][MAX_SUPPORTED_MODES] = {
    {  24,   24,   24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16 },
    {  32,   32,   32,   32,   32,   32,   32,   24,   24,   24,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   48,   48,   32,   32,   24,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   48,   48,   32,   32,   24,   24,   24,   24 },
    {  64,   64,   64,   64,   32,   32,   32,   32,   32,   32,   32,   32,   32 }
};
static const uint16_t hme_level0_total_search_area_height_oq[5][MAX_SUPPORTED_MODES] = {
    {  40,   40,   40,   40,   40,   32,   32,   32,   32,   24,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   40,   40,   40,   40,   32,   32,   32,   32 },
    {  48,   48,   48,   48,   48,   32,   32,   32,   32,   32,   32,   32,   32 },
    {  48,   48,   48,   48,   48,   48,   48,   48,   48,   40,   40,   40,   40 },
    {  80,   80,   80,   80,   32,   32,   32,   32,   32,   32,   32,   32,   32 }
};
static const uint16_t hme_level0_search_area_in_height_array_top_oq[5][MAX_SUPPORTED_MODES] = {
    {  20,   20,   20,   20,   20,   16,   16,   16,   16,   12,   12,   12,   12 },
    {  24,   24,   24,   24,   24,   20,   20,   20,   20,   16,   16,   16,   16 },
    {  24,   24,   24,   24,   24,   16,   16,   16,   16,   16,   16,   16,   16 },
    {  24,   24,   24,   24,   24,   24,   24,   24,   24,   20,   20,   20,   20 },
    {  40,   40,   40,   40,   16,   16,   16,   16,   16,   16,   16,   16,   16 }
};
static const uint16_t hme_level0_search_area_in_height_array_bottom_oq[5][MAX_SUPPORTED_MODES] = {
    {  20,   20,   20,   20,   20,   16,   16,   16,   16,   12,   12,   12,   12 },
    {  24,   24,   24,   24,   24,   20,   20,   20,   20,   16,   16,   16,   16 },
    {  24,   24,   24,   24,   24,   16,   16,   16,   16,   16,   16,   16,   16 },
    {  24,   24,   24,   24,   24,   24,   24,   24,   24,   20,   20,   20,   20 },
    {  40,   40,   40,   40,   16,   16,   16,   16,   16,   16,   16,   16,   16 }
};

// HME LEVEL 1
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11
static const uint8_t enable_hme_level1_flag_oq[5][MAX_SUPPORTED_MODES] = {
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_720P_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080i_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080p_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0 }       // INPUT_SIZE_4K_RANGE
};
static const uint16_t hme_level1_search_area_in_width_array_left_oq[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,   16,    8,    8,    8,    8,    8,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level1_search_area_in_width_array_right_oq[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,   16,    8,    8,    8,    8,    8,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level1_search_area_in_height_array_top_oq[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,   16,    8,    8,    8,    8,    8,    4,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    4,    4,    4,    4,    2,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level1_search_area_in_height_array_bottom_oq[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,   16,    8,    8,    8,    8,    8,    4,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    4,    4,    4,    4,    2,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
// HME LEVEL 2
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11
static const uint8_t enable_hme_level2_flag_oq[5][MAX_SUPPORTED_MODES] = {
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_720P_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_1080i_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_1080p_RANGE
    {   1,    1,    1,    1,    0,    0,    0,    0,    0,    0,    0,    0,    0 }       // INPUT_SIZE_4K_RANGE
};
static const uint16_t hme_level2_search_area_in_width_array_left_oq[5][MAX_SUPPORTED_MODES] = {
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    4,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level2_search_area_in_width_array_right_oq[5][MAX_SUPPORTED_MODES] = {
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    4,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level2_search_area_in_height_array_top_oq[5][MAX_SUPPORTED_MODES] = {
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    2,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    2,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    2,    2,    2,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    2,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level2_search_area_in_height_array_bottom_oq[5][MAX_SUPPORTED_MODES] = {
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    2,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    2,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    2,    2,    2,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    2,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};

static const uint8_t search_area_width_oq[5][MAX_SUPPORTED_MODES] = {
    {  64,   64,   64,   64,   16,   16,   16,   16,   16,   16,    8,    8,    8 },
    {  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8 },
    {  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8 },
    {  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8 },
    {  64,   64,   64,   64,   16,   16,   16,   16,    8,    8,    8,    8,    8 }
};

static const uint8_t search_area_height_oq[5][MAX_SUPPORTED_MODES] = {
    {  64,   64,   64,   64,   16,    9,    9,    9,    9,    7,    7,    7,    7 },
    {  64,   64,   64,   64,   16,   13,   13,    9,    9,    7,    7,    7,    7 },
    {  64,   64,   64,   64,   16,    9,    9,    7,    7,    7,    7,    7,    7 },
    {  64,   64,   64,   64,   16,   13,   13,    9,    9,    7,    7,    7,    7 },
    {  64,   64,   64,   64,    9,    9,    9,    9,    7,    7,    7,    7,    7 }
};
/******************************************************************************
                            ME/HME settings SQ
*******************************************************************************/
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11
static const uint8_t enable_hme_level0_flag_sq[5][MAX_SUPPORTED_MODES] = {
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_720P_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080i_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080p_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_4K_RANGE
};
static const uint16_t hme_level0_total_search_area_width_sq[5][MAX_SUPPORTED_MODES] = {
    {  48,   48,   48,   48,   48,   48,   48,   48,   32,   32,   32,   32,   32 },
    {  64,   64,   64,   64,   64,   64,   64,   64,   48,   48,   32,   32,   32 },
    {  96,   96,   96,   96,   96,   96,   96,   96,   64,   48,   48,   48,   48 },
    {  96,   96,   96,   96,   96,   96,   64,   64,   64,   48,   48,   48,   48 },
    { 128,  128,  128,  128,  128,   96,   96,   64,   64,   64,   64,   64,   64 }
};

static const uint16_t hme_level0_search_area_in_width_array_left_sq[5][MAX_SUPPORTED_MODES] = {
    {  24,   24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16,   16 },
    {  32,   32,   32,   32,   32,   32,   32,   32,   24,   24,   16,   16,   16 },
    {  48,   48,   48,   48,   48,   48,   48,   48,   32,   24,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   48,   32,   32,   32,   24,   24,   24,   24 },
    {  64,   64,   64,   64,   64,   48,   48,   32,   32,   32,   32,   32,   32 }
};
static const uint16_t hme_level0_search_area_in_width_array_right_sq[5][MAX_SUPPORTED_MODES] = {
    {  24,   24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16,   16 },
    {  32,   32,   32,   32,   32,   32,   32,   32,   24,   24,   16,   16,   16 },
    {  48,   48,   48,   48,   48,   48,   48,   48,   32,   24,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   48,   32,   32,   32,   24,   24,   24,   24 },
    {  64,   64,   64,   64,   64,   48,   48,   32,   32,   32,   32,   32,   32 }
};
static const uint16_t hme_level0_total_search_area_height_sq[5][MAX_SUPPORTED_MODES] = {
    {  40,   40,   40,   40,   40,   32,   32,   32,   24,   24,   16,   16,   16 },
    {  48,   48,   48,   48,   48,   40,   40,   40,   32,   32,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   32,   32,   32,   32,   32,   24,   24,   24 },
    {  48,   48,   48,   48,   48,   48,   48,   48,   48,   40,   40,   40,   40 },
    {  80,   80,   80,   80,   80,   64,   64,   32,   32,   32,   32,   32,   32 }
};
static const uint16_t hme_level0_search_area_in_height_array_top_sq[5][MAX_SUPPORTED_MODES] = {
    {  20,   20,   20,   20,   20,   16,   16,   16,   12,   12,    8,    8,    8 },
    {  24,   24,   24,   24,   24,   20,   20,   20,   16,   16,   12,   12,   12 },
    {  24,   24,   24,   24,   24,   16,   16,   16,   16,   16,   12,   12,   12 },
    {  24,   24,   24,   24,   24,   24,   24,   24,   24,   20,   20,   20,   20 },
    {  40,   40,   40,   40,   40,   32,   32,   16,   16,   16,   16,   16,   16 }
};
static const uint16_t hme_level0_search_area_in_height_array_bottom_sq[5][MAX_SUPPORTED_MODES] = {
    {  20,   20,   20,   20,   20,   16,   16,   16,   12,   12,    8,    8,    8 },
    {  24,   24,   24,   24,   24,   20,   20,   20,   16,   16,   12,   12,   12 },
    {  24,   24,   24,   24,   24,   16,   16,   16,   16,   16,   12,   12,   12 },
    {  24,   24,   24,   24,   24,   24,   24,   24,   24,   20,   20,   20,   20 },
    {  40,   40,   40,   40,   40,   32,   32,   16,   16,   16,   16,   16,   16 }
};

// HME LEVEL 1
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11
static const uint8_t enable_hme_level1_flag_sq[5][MAX_SUPPORTED_MODES] = {
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_720P_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080i_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080p_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0 }       // INPUT_SIZE_4K_RANGE
};
static const uint16_t hme_level1_search_area_in_width_array_left_sq[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    8,    4,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level1_search_area_in_width_array_right_sq[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    8,    4,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level1_search_area_in_height_array_top_sq[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    2,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    4,    4,    4,    2,    2,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    4,    4,    4,    4,    2,    2,    2 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level1_search_area_in_height_array_bottom_sq[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    2,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    4,    4,    4,    2,    2,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    4,    4,    4,    4,    2,    2,    2 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
// HME LEVEL 2
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11
static const uint8_t enable_hme_level2_flag_sq[5][MAX_SUPPORTED_MODES] = {
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_720P_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_1080i_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_1080p_RANGE
    {   1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0,    0 }       // INPUT_SIZE_4K_RANGE
};
static const uint16_t hme_level2_search_area_in_width_array_left_sq[5][MAX_SUPPORTED_MODES] = {
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    0,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level2_search_area_in_width_array_right_sq[5][MAX_SUPPORTED_MODES] = {
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    0,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level2_search_area_in_height_array_top_sq[5][MAX_SUPPORTED_MODES] = {
    {   8,    8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    2,    2,    2,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    2,    2,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    2,    2,    2,    0,    0,    0,    0,    0,    0 }
};
static const uint16_t hme_level2_search_area_in_height_array_bottom_sq[5][MAX_SUPPORTED_MODES] = {
    {   8,    8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    2,    2,    2,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    2,    2,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    2,    2,    2,    0,    0,    0,    0,    0,    0 }
};

static const uint8_t search_area_width_sq[5][MAX_SUPPORTED_MODES] = {
    {  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8 ,    8 },
    {  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8 ,    8 },
    {  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8 ,    8 },
    {  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8 ,    8 },
    {  64,   64,   64,   64,   24,   16,   16,   16,   16,   16,    8,    8 ,    8 }
};

static const uint8_t search_area_height_sq[5][MAX_SUPPORTED_MODES] = {
    {  64,   64,   64,   64,   16,    9,    9,    9,    7,    7,    5,    5,    5 },
    {  64,   64,   64,   64,   16,   13,   13,   13,    9,    7,    7,    7,    7 },
    {  64,   64,   64,   64,   16,    9,    9,    9,    7,    7,    5,    5,    5 },
    {  64,   64,   64,   64,   16,   13,    9,    9,    9,    7,    7,    7,    7 },
    {  64,   64,   64,   64,   24,   16,   13,    9,    9,    7,    7,    7,    5 }
};

#define MAX_SUPPORTED_SEGMENTS       7

#ifdef __cplusplus
}
#endif
#endif // EbDefinitions_h
/* File EOF */
