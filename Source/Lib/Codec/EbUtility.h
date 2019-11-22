/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbUtility_h
#define EbUtility_h

#include "EbDefinitions.h"
#include "vp9_enums.h"

#ifdef __cplusplus
extern "C" {
#endif
/****************************
 * UTILITY FUNCTIONS
 ****************************/
void build_ep_block_stats();

typedef struct EpBlockStats
{
    uint8_t    depth;            // depth of the block
    Part       shape;            // P_N..P_V4 . P_S is not used.
    uint8_t    origin_x;         // orgin x from topleft of sb
    uint8_t    origin_y;         // orgin x from topleft of sb

    uint8_t    d1i;              // index of the block in d1 dimension 0..24  (0 is parent square, 1 top half of H , ...., 24:last quarter of V4)
    uint16_t   sqi_mds;          // index of the parent square in md scan.
    uint8_t    totns;             // max number of ns blocks within one partition 1..4 (N:1,H:2,V:2,HA:3,HB:3,VA:3,VB:3,H4:4,V4:4)
    uint8_t    nsi;              // non square index within a partition  0..totns-1

    uint8_t    bwidth;           // block width
    uint8_t    bheight;          // block height
    uint8_t    bwidth_uv;        // block width for chroma 4:2:0
    uint8_t    bheight_uv;       // block height for chroma 4:2:0

    uint8_t    bwidth_log2;      // block width log2
    uint8_t    bheight_log2;     // block height log2
    BLOCK_SIZE bsize;            // block size
    BLOCK_SIZE bsize_uv;         // block size for chroma 4:2:0

    TX_SIZE    tx_size;          // largest transform size for a block
    TX_SIZE    tx_size_uv;       // largest transform size for a block for chroma 4:2:0

    uint16_t   blkidx_mds;       // block index in md scan
    uint16_t   blkidx_dps;       // block index in depth scan

    int        has_uv;
    int        sq_size;
    int        sq_size_uv;
    int        is_last_quadrant; // only for square bloks, is this the fourth quadrant block?

} EpBlockStats;

static const uint16_t parent_depth_offset[5] = {   0, 512, 128,  32,  8 };
static const uint16_t sq_depth_offset    [5] = { 681, 169,  41,   9,  1 };
static const uint16_t nsq_depth_offset   [5] = {   5,   5,   5,   5,  1 };

static const  TX_SIZE blocksize_to_txsize[/*BLOCK_SIZES_ALL*/13] = {
        TX_4X4    ,      // BLOCK_4X4
        TX_4X4    ,      // BLOCK_4X8
        TX_4X4    ,      // BLOCK_8X4
        TX_8X8    ,      // BLOCK_8X8
        TX_8X8    ,      // BLOCK_8X16
        TX_8X8    ,      // BLOCK_16X8
        TX_16X16  ,      // BLOCK_16X16
        TX_16X16  ,      // BLOCK_16X32
        TX_16X16  ,      // BLOCK_32X16
        TX_32X32  ,      // BLOCK_32X32
        TX_32X32  ,      // BLOCK_32X64
        TX_32X32  ,      // BLOCK_64X32
        TX_32X32  ,      // BLOCK_64X64
};

static const uint16_t tu_size[4] = { 4,   8,   16,   32 };

// PA Stats Helper Functions
typedef struct PaBlockStats
{
    uint8_t      depth;
    uint8_t      size;
    uint8_t      size_log2;
    uint16_t     origin_x;
    uint16_t     origin_y;
    uint8_t      cu_num_in_depth;
    BLOCK_SIZE   sb_type;
} PaBlockStats;

// Hsan: Pre-alysis assumes 85 blocks only (all squares except 4x4 blocks)
// Hsan: MD,EP,EC handle all block sizes
extern PaBlockStats *pa_get_block_stats(int block_index);
const  EpBlockStats *ep_get_block_stats(uint32_t bidx_mds);

extern uint32_t Log2f(uint32_t x);
extern uint64_t log2f64(uint64_t x);
extern uint32_t eb_vp9_endian_swap(uint32_t ui);
extern uint64_t eb_vp9_log2f_high_precision(uint64_t x, uint8_t precision);
/****************************
 * MACROS
 ****************************/

#ifdef _MSC_VER
#define MULTI_LINE_MACRO_BEGIN do {
#define MULTI_LINE_MACRO_END \
    __pragma(warning(push)) \
    __pragma(warning(disable:4127)) \
    } while(0) \
    __pragma(warning(pop))
#else
#define MULTI_LINE_MACRO_BEGIN do {
#define MULTI_LINE_MACRO_END } while(0)
#endif

//**************************************************
// MACROS
//**************************************************
#define MAX(x, y)                       ((x)>(y)?(x):(y))
#define MIN(x, y)                       ((x)<(y)?(x):(y))
#define MEDIAN(a,b,c)                   ((a)>(b)?(a)>(c)?(b)>(c)?(b):(c):(a):(b)>(c)?(a)>(c)?(a):(c):(b))
#define CLIP3(min_val, max_val, a)        (((a)<(min_val)) ? (min_val) : (((a)>(max_val)) ? (max_val) :(a)))
#define CLIP3EQ(min_val, max_val, a)        (((a)<=(min_val)) ? (min_val) : (((a)>=(max_val)) ? (max_val) :(a)))
#define BITDEPTH_MIDRANGE_VALUE(precision)  (1 << ((precision) - 1))
#define SWAP(a, b)                      MULTI_LINE_MACRO_BEGIN (a) ^= (b); (b) ^= (a); (a) ^= (b); MULTI_LINE_MACRO_END
#define ABS(a)                          (((a) < 0) ? (-(a)) : (a))
#define EB_ABS_DIFF(a,b)                ((a) > (b) ? ((a) - (b)) : ((b) - (a)))
#define EB_DIFF_SQR(a,b)                (((a) - (b)) * ((a) - (b)))
#define SQR(x)                          ((x)*(x))
#define POW2(x)                         (1 << (x))
#define SIGN(a,b)                       (((a - b) < 0) ? (-1) : ((a - b) > 0) ? (1) : 0)
#define ROUND(a)                        (a >= 0) ? ( a + 1/2) : (a - 1/2);
#define UNSIGNED_DEC(x)                 MULTI_LINE_MACRO_BEGIN (x) = (((x) > 0) ? ((x) - 1) : 0); MULTI_LINE_MACRO_END
#define CIRCULAR_ADD(x,max)             (((x) >= (max)) ? ((x) - (max)) : ((x) < 0) ? ((max) + (x)) : (x))
#define CIRCULAR_ADD_UNSIGNED(x,max)    (((x) >= (max)) ? ((x) - (max)) : (x))
#define CEILING(x,base)                 ((((x) + (base) - 1) / (base)) * (base))
#define POW2_CHECK(x)                   ((x) == ((x) & (-((int32_t)(x)))))
#define ROUND_UP_MUL_8(x)               ((x) + ((8 - ((x) & 0x7)) & 0x7))
#define ROUND_UP_MULT(x,mult)           ((x) + (((mult) - ((x) & ((mult)-1))) & ((mult)-1)))
#define ROUND_UV(x)                     (((x)>>3)<<3)

// rounds down to the next power of two
#define FLOOR_POW2(x)       \
    MULTI_LINE_MACRO_BEGIN  \
        (x) |= ((x) >> 1);  \
        (x) |= ((x) >> 2);  \
        (x) |= ((x) >> 4);  \
        (x) |= ((x) >> 8);  \
        (x) |= ((x) >>16);  \
        (x) -= ((x) >> 1);  \
    MULTI_LINE_MACRO_END

// rounds up to the next power of two
#define CEIL_POW2(x)        \
    MULTI_LINE_MACRO_BEGIN  \
        (x) -= 1;           \
        (x) |= ((x) >> 1);  \
        (x) |= ((x) >> 2);  \
        (x) |= ((x) >> 4);  \
        (x) |= ((x) >> 8);  \
        (x) |= ((x) >>16);  \
        (x) += 1;           \
    MULTI_LINE_MACRO_END

// Calculates the Log2 floor of the integer 'x'
//   Intended to only be used for macro definitions
#define LOG2F(x) (              \
    ((x) < 0x0002u) ? 0u :      \
    ((x) < 0x0004u) ? 1u :      \
    ((x) < 0x0008u) ? 2u :      \
    ((x) < 0x0010u) ? 3u :      \
    ((x) < 0x0020u) ? 4u :      \
    ((x) < 0x0040u) ? 5u :      \
    ((x) < 0x0100u) ? 6u :      \
    ((x) < 0x0200u) ? 7u :      \
    ((x) < 0x0400u) ? 8u :      \
    ((x) < 0x0800u) ? 9u :      \
    ((x) < 0x1000u) ? 10u :     \
    ((x) < 0x2000u) ? 11u :     \
    ((x) < 0x4000u) ? 12u : 13u)

#define LOG2F_8(x) (            \
    ((x) < 0x0002u) ? 0u :      \
    ((x) < 0x0004u) ? 1u :      \
    ((x) < 0x0008u) ? 2u :      \
    ((x) < 0x0010u) ? 3u :      \
    ((x) < 0x0020u) ? 4u :      \
    ((x) < 0x0040u) ? 5u :6u )

#define TWO_D_INDEX(x, y, stride)   \
    (((y) * (stride)) + (x))

// MAX_CU_COUNT is used to find the total number of partitions for the max partition depth and for
// each parent partition up to the root partition level (i.e. LCU level).

// MAX_CU_COUNT is given by SUM from k=1 to n (4^(k-1)), reduces by using the following finite sum
// SUM from k=1 to n (q^(k-1)) = (q^n - 1)/(q-1) => (4^n - 1) / 3
#define MAX_CU_COUNT(max_depth_count) ((((1 << (max_depth_count)) * (1 << (max_depth_count))) - 1)/3)

//**************************************************
// CONSTANTS
//**************************************************
#define MIN_UNSIGNED_VALUE      0
#define MAX_UNSIGNED_VALUE     ~0u
#define MIN_SIGNED_VALUE       ~0 - ((signed) (~0u >> 1))
#define MAX_SIGNED_VALUE       ((signed) (~0u >> 1))
#define MINI_GOP_MAX_COUNT            15
#define MINI_GOP_WINDOW_MAX_COUNT     8    // widow subdivision: 8 x 3L

#define MIN_HIERARCHICAL_LEVEL         2
#if NEW_PRED_STRUCT
static const uint32_t mini_gop_offset[4] = { 1, 3, 7, 31 };
#endif
typedef struct MiniGopStats
{
    uint32_t  hierarchical_levels;
    uint32_t  start_index;
    uint32_t  end_index;
    uint32_t  lenght;

} MiniGopStats;
extern const MiniGopStats  * eb_vp9_get_mini_gop_stats(const uint32_t mini_gop_index);
typedef enum MiniGopIndex
{
    L6_INDEX   = 0,
    L5_0_INDEX = 1,
    L4_0_INDEX = 2,
    L3_0_INDEX = 3,
    L3_1_INDEX = 4,
    L4_1_INDEX = 5,
    L3_2_INDEX = 6,
    L3_3_INDEX = 7,
    L5_1_INDEX = 8,
    L4_2_INDEX = 9,
    L3_4_INDEX = 10,
    L3_5_INDEX = 11,
    L4_3_INDEX = 12,
    L3_6_INDEX = 13,
    L3_7_INDEX = 14
} MiniGopIndex;

#ifdef __cplusplus
}
#endif

#endif // EbUtility_h
