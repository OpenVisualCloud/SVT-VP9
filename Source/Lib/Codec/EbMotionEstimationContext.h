/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMotionEstimationContext_h
#define EbMotionEstimationContext_h

#include "EbDefinitions.h"
#include "EbCodingUnit.h"
#ifdef __cplusplus
extern "C" {
#endif

// Max Search Area
#define MAX_SEARCH_AREA_WIDTH   1350 // This should be a function for the MAX HME L0 * the multiplications per layers and per Hierarchichal structures
#define MAX_SEARCH_AREA_HEIGHT  675 // This should be a function for the MAX HME L0 * the multiplications per layers and per Hierarchichal structures

// 1-D interpolation shift value
#define if_shift 6

#define VARIANCE_PRECISION      16

#define MEAN_PRECISION      (VARIANCE_PRECISION >> 1)

#define HME_RECTANGULAR    0
#define HME_SPARSE         1

// Quater pel refinement methods
typedef enum EbQuarterPelRefinementMethod
{
    EB_QUARTER_IN_FULL,
    EB_QUARTER_IN_HALF_HORIZONTAL,
    EB_QUARTER_IN_HALF_VERTICAL,
    EB_QUARTER_IN_HALF_DIAGONAL
} EbQuarterPelInterpolationMethod;

// HME parameters

#define HME_DECIM_FILTER_TAP     9

typedef enum EbMeTierZeroPu
{

    // 2Nx2N [85 partitions]
    ME_TIER_ZERO_PU_64x64       =    0,
    ME_TIER_ZERO_PU_32x32_0     =    1,
    ME_TIER_ZERO_PU_32x32_1     =    2,
    ME_TIER_ZERO_PU_32x32_2     =    3,
    ME_TIER_ZERO_PU_32x32_3     =    4,
    ME_TIER_ZERO_PU_16x16_0     =    5,
    ME_TIER_ZERO_PU_16x16_1     =    6,
    ME_TIER_ZERO_PU_16x16_2     =    7,
    ME_TIER_ZERO_PU_16x16_3     =    8,
    ME_TIER_ZERO_PU_16x16_4     =    9,
    ME_TIER_ZERO_PU_16x16_5     =    10,
    ME_TIER_ZERO_PU_16x16_6     =    11,
    ME_TIER_ZERO_PU_16x16_7     =    12,
    ME_TIER_ZERO_PU_16x16_8     =    13,
    ME_TIER_ZERO_PU_16x16_9     =    14,
    ME_TIER_ZERO_PU_16x16_10    =    15,
    ME_TIER_ZERO_PU_16x16_11    =    16,
    ME_TIER_ZERO_PU_16x16_12    =    17,
    ME_TIER_ZERO_PU_16x16_13    =    18,
    ME_TIER_ZERO_PU_16x16_14    =    19,
    ME_TIER_ZERO_PU_16x16_15    =    20,
    ME_TIER_ZERO_PU_8x8_0       =    21,
    ME_TIER_ZERO_PU_8x8_1       =    22,
    ME_TIER_ZERO_PU_8x8_2       =    23,
    ME_TIER_ZERO_PU_8x8_3       =    24,
    ME_TIER_ZERO_PU_8x8_4       =    25,
    ME_TIER_ZERO_PU_8x8_5       =    26,
    ME_TIER_ZERO_PU_8x8_6       =    27,
    ME_TIER_ZERO_PU_8x8_7       =    28,
    ME_TIER_ZERO_PU_8x8_8       =    29,
    ME_TIER_ZERO_PU_8x8_9       =    30,
    ME_TIER_ZERO_PU_8x8_10      =    31,
    ME_TIER_ZERO_PU_8x8_11      =    32,
    ME_TIER_ZERO_PU_8x8_12      =    33,
    ME_TIER_ZERO_PU_8x8_13      =    34,
    ME_TIER_ZERO_PU_8x8_14      =    35,
    ME_TIER_ZERO_PU_8x8_15      =    36,
    ME_TIER_ZERO_PU_8x8_16      =    37,
    ME_TIER_ZERO_PU_8x8_17      =    38,
    ME_TIER_ZERO_PU_8x8_18      =    39,
    ME_TIER_ZERO_PU_8x8_19      =    40,
    ME_TIER_ZERO_PU_8x8_20      =    41,
    ME_TIER_ZERO_PU_8x8_21      =    42,
    ME_TIER_ZERO_PU_8x8_22      =    43,
    ME_TIER_ZERO_PU_8x8_23      =    44,
    ME_TIER_ZERO_PU_8x8_24      =    45,
    ME_TIER_ZERO_PU_8x8_25      =    46,
    ME_TIER_ZERO_PU_8x8_26      =    47,
    ME_TIER_ZERO_PU_8x8_27      =    48,
    ME_TIER_ZERO_PU_8x8_28      =    49,
    ME_TIER_ZERO_PU_8x8_29      =    50,
    ME_TIER_ZERO_PU_8x8_30      =    51,
    ME_TIER_ZERO_PU_8x8_31      =    52,
    ME_TIER_ZERO_PU_8x8_32      =    53,
    ME_TIER_ZERO_PU_8x8_33      =    54,
    ME_TIER_ZERO_PU_8x8_34      =    55,
    ME_TIER_ZERO_PU_8x8_35      =    56,
    ME_TIER_ZERO_PU_8x8_36      =    57,
    ME_TIER_ZERO_PU_8x8_37      =    58,
    ME_TIER_ZERO_PU_8x8_38      =    59,
    ME_TIER_ZERO_PU_8x8_39      =    60,
    ME_TIER_ZERO_PU_8x8_40      =    61,
    ME_TIER_ZERO_PU_8x8_41      =    62,
    ME_TIER_ZERO_PU_8x8_42      =    63,
    ME_TIER_ZERO_PU_8x8_43      =    64,
    ME_TIER_ZERO_PU_8x8_44      =    65,
    ME_TIER_ZERO_PU_8x8_45      =    66,
    ME_TIER_ZERO_PU_8x8_46      =    67,
    ME_TIER_ZERO_PU_8x8_47      =    68,
    ME_TIER_ZERO_PU_8x8_48      =    69,
    ME_TIER_ZERO_PU_8x8_49      =    70,
    ME_TIER_ZERO_PU_8x8_50      =    71,
    ME_TIER_ZERO_PU_8x8_51      =    72,
    ME_TIER_ZERO_PU_8x8_52      =    73,
    ME_TIER_ZERO_PU_8x8_53      =    74,
    ME_TIER_ZERO_PU_8x8_54      =    75,
    ME_TIER_ZERO_PU_8x8_55      =    76,
    ME_TIER_ZERO_PU_8x8_56      =    77,
    ME_TIER_ZERO_PU_8x8_57      =    78,
    ME_TIER_ZERO_PU_8x8_58      =    79,
    ME_TIER_ZERO_PU_8x8_59      =    80,
    ME_TIER_ZERO_PU_8x8_60      =    81,
    ME_TIER_ZERO_PU_8x8_61      =    82,
    ME_TIER_ZERO_PU_8x8_62      =    83,
    ME_TIER_ZERO_PU_8x8_63      =    84,
    // 2NxN  [42 partitions]
    ME_TIER_ZERO_PU_64x32_0     =    85,
    ME_TIER_ZERO_PU_64x32_1     =    86,
    ME_TIER_ZERO_PU_32x16_0     =    87,
    ME_TIER_ZERO_PU_32x16_1     =    88,
    ME_TIER_ZERO_PU_32x16_2     =    89,
    ME_TIER_ZERO_PU_32x16_3     =    90,
    ME_TIER_ZERO_PU_32x16_4     =    91,
    ME_TIER_ZERO_PU_32x16_5     =    92,
    ME_TIER_ZERO_PU_32x16_6     =    93,
    ME_TIER_ZERO_PU_32x16_7     =    94,
    ME_TIER_ZERO_PU_16x8_0      =    95,
    ME_TIER_ZERO_PU_16x8_1      =    96,
    ME_TIER_ZERO_PU_16x8_2      =    97,
    ME_TIER_ZERO_PU_16x8_3      =    98,
    ME_TIER_ZERO_PU_16x8_4      =    99,
    ME_TIER_ZERO_PU_16x8_5      =    100,
    ME_TIER_ZERO_PU_16x8_6      =    101,
    ME_TIER_ZERO_PU_16x8_7      =    102,
    ME_TIER_ZERO_PU_16x8_8      =    103,
    ME_TIER_ZERO_PU_16x8_9      =    104,
    ME_TIER_ZERO_PU_16x8_10     =    105,
    ME_TIER_ZERO_PU_16x8_11     =    106,
    ME_TIER_ZERO_PU_16x8_12     =    107,
    ME_TIER_ZERO_PU_16x8_13     =    108,
    ME_TIER_ZERO_PU_16x8_14     =    109,
    ME_TIER_ZERO_PU_16x8_15     =    110,
    ME_TIER_ZERO_PU_16x8_16     =    111,
    ME_TIER_ZERO_PU_16x8_17     =    112,
    ME_TIER_ZERO_PU_16x8_18     =    113,
    ME_TIER_ZERO_PU_16x8_19     =    114,
    ME_TIER_ZERO_PU_16x8_20     =    115,
    ME_TIER_ZERO_PU_16x8_21     =    116,
    ME_TIER_ZERO_PU_16x8_22     =    117,
    ME_TIER_ZERO_PU_16x8_23     =    118,
    ME_TIER_ZERO_PU_16x8_24     =    119,
    ME_TIER_ZERO_PU_16x8_25     =    120,
    ME_TIER_ZERO_PU_16x8_26     =    121,
    ME_TIER_ZERO_PU_16x8_27     =    122,
    ME_TIER_ZERO_PU_16x8_28     =    123,
    ME_TIER_ZERO_PU_16x8_29     =    124,
    ME_TIER_ZERO_PU_16x8_30     =    125,
    ME_TIER_ZERO_PU_16x8_31     =    126,
    // Nx2N  [42 partitions]
    ME_TIER_ZERO_PU_32x64_0     =    127,
    ME_TIER_ZERO_PU_32x64_1     =    128,
    ME_TIER_ZERO_PU_16x32_0     =    129,
    ME_TIER_ZERO_PU_16x32_1     =    130,
    ME_TIER_ZERO_PU_16x32_2     =    131,
    ME_TIER_ZERO_PU_16x32_3     =    132,
    ME_TIER_ZERO_PU_16x32_4     =    133,
    ME_TIER_ZERO_PU_16x32_5     =    134,
    ME_TIER_ZERO_PU_16x32_6     =    135,
    ME_TIER_ZERO_PU_16x32_7     =    136,
    ME_TIER_ZERO_PU_8x16_0      =    137,
    ME_TIER_ZERO_PU_8x16_1      =    138,
    ME_TIER_ZERO_PU_8x16_2      =    139,
    ME_TIER_ZERO_PU_8x16_3      =    140,
    ME_TIER_ZERO_PU_8x16_4      =    141,
    ME_TIER_ZERO_PU_8x16_5      =    142,
    ME_TIER_ZERO_PU_8x16_6      =    143,
    ME_TIER_ZERO_PU_8x16_7      =    144,
    ME_TIER_ZERO_PU_8x16_8      =    145,
    ME_TIER_ZERO_PU_8x16_9      =    146,
    ME_TIER_ZERO_PU_8x16_10     =    147,
    ME_TIER_ZERO_PU_8x16_11     =    148,
    ME_TIER_ZERO_PU_8x16_12     =    149,
    ME_TIER_ZERO_PU_8x16_13     =    150,
    ME_TIER_ZERO_PU_8x16_14     =    151,
    ME_TIER_ZERO_PU_8x16_15     =    152,
    ME_TIER_ZERO_PU_8x16_16     =    153,
    ME_TIER_ZERO_PU_8x16_17     =    154,
    ME_TIER_ZERO_PU_8x16_18     =    155,
    ME_TIER_ZERO_PU_8x16_19     =    156,
    ME_TIER_ZERO_PU_8x16_20     =    157,
    ME_TIER_ZERO_PU_8x16_21     =    158,
    ME_TIER_ZERO_PU_8x16_22     =    159,
    ME_TIER_ZERO_PU_8x16_23     =    160,
    ME_TIER_ZERO_PU_8x16_24     =    161,
    ME_TIER_ZERO_PU_8x16_25     =    162,
    ME_TIER_ZERO_PU_8x16_26     =    163,
    ME_TIER_ZERO_PU_8x16_27     =    164,
    ME_TIER_ZERO_PU_8x16_28     =    165,
    ME_TIER_ZERO_PU_8x16_29     =    166,
    ME_TIER_ZERO_PU_8x16_30     =    167,
    ME_TIER_ZERO_PU_8x16_31     =    168,

    // 2NxnU [10 partitions]
    ME_TIER_ZERO_PU_64x16_0     =    169,
    ME_TIER_ZERO_PU_64x16_1     =    170,
    ME_TIER_ZERO_PU_32x8_0      =    171,
    ME_TIER_ZERO_PU_32x8_1      =    172,
    ME_TIER_ZERO_PU_32x8_2      =    173,
    ME_TIER_ZERO_PU_32x8_3      =    174,
    ME_TIER_ZERO_PU_32x8_4      =    175,
    ME_TIER_ZERO_PU_32x8_5      =    176,
    ME_TIER_ZERO_PU_32x8_6      =    177,
    ME_TIER_ZERO_PU_32x8_7      =    178,
    // 2NxnD [10 partitions]
    ME_TIER_ZERO_PU_64x48_0     =    179,
    ME_TIER_ZERO_PU_64x48_1     =    180,
    ME_TIER_ZERO_PU_32x24_0     =    181,
    ME_TIER_ZERO_PU_32x24_1     =    182,
    ME_TIER_ZERO_PU_32x24_2     =    183,
    ME_TIER_ZERO_PU_32x24_3     =    184,
    ME_TIER_ZERO_PU_32x24_4     =    185,
    ME_TIER_ZERO_PU_32x24_5     =    186,
    ME_TIER_ZERO_PU_32x24_6     =    187,
    ME_TIER_ZERO_PU_32x24_7     =    188,
    // nLx2N [10 partitions]
    ME_TIER_ZERO_PU_16x64_0     =    189,
    ME_TIER_ZERO_PU_16x64_1     =    190,
    ME_TIER_ZERO_PU_8x32_0      =    191,
    ME_TIER_ZERO_PU_8x32_1      =    192,
    ME_TIER_ZERO_PU_8x32_2      =    193,
    ME_TIER_ZERO_PU_8x32_3      =    194,
    ME_TIER_ZERO_PU_8x32_4      =    195,
    ME_TIER_ZERO_PU_8x32_5      =    196,
    ME_TIER_ZERO_PU_8x32_6      =    197,
    ME_TIER_ZERO_PU_8x32_7      =    198,
    // nRx2N [10 partitions]
    ME_TIER_ZERO_PU_48x64_0     =    199,
    ME_TIER_ZERO_PU_48x64_1     =    200,
    ME_TIER_ZERO_PU_24x32_0     =    201,
    ME_TIER_ZERO_PU_24x32_1     =    202,
    ME_TIER_ZERO_PU_24x32_2     =    203,
    ME_TIER_ZERO_PU_24x32_3     =    204,
    ME_TIER_ZERO_PU_24x32_4     =    205,
    ME_TIER_ZERO_PU_24x32_5     =    206,
    ME_TIER_ZERO_PU_24x32_6     =    207,
    ME_TIER_ZERO_PU_24x32_7     =    208
} EbMeTierZeroPu;

typedef struct MePredUnit
{
    uint32_t         distortion;
    EB_PREDDIRECTION prediction_direction;
    uint32_t         Mv[MAX_NUM_OF_REF_PIC_LIST];

} MePredUnit;

typedef struct MotionEstimationTierZero {
    MePredUnit pu[MAX_ME_PU_COUNT];
} MotionEstimationTierZero;

typedef struct MeContext
{\
    // MV offset (search center)
    int16_t   x_mv_offset;
    int16_t   y_mv_offset;

    // Search region stride
    uint32_t  interpolated_stride;
    uint32_t  interpolated_full_stride[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];

    MotionEstimationTierZero  me_candidate[MAX_ME_CANDIDATE_PER_PU];

    uint8_t *hme_buffer;
    uint32_t hme_buffer_stride;

    // Intermediate LCU-sized buffer to retain the input samples
    uint8_t *sb_buffer;
    uint8_t *sb_buffer_ptr;
    uint32_t sb_buffer_stride;
    uint8_t *hme_sb_buffer;
    uint32_t hme_sb_buffer_stride;
    uint8_t *sb_src_ptr;
    uint32_t sb_src_stride;
    uint8_t *quarter_sb_buffer;
    uint32_t quarter_sb_buffer_stride;
    uint8_t *sixteenth_sb_buffer;
    uint32_t sixteenth_sb_buffer_stride;

    uint8_t *integer_buffer[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];
    uint8_t *integer_buffer_ptr[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];
    uint8_t *posb_buffer[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];
    uint8_t *posh_buffer[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];
    uint8_t *posj_buffer[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];

    uint8_t  *one_d_intermediate_results_buf0;
    uint8_t  *one_d_intermediate_results_buf1;

    int16_t x_search_area_origin[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];
    int16_t y_search_area_origin[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];

    uint8_t *avctemp_buffer;

    uint32_t *p_best_sad8x8;
    uint32_t *p_best_sad16x16;
    uint32_t *p_best_sad32x32;
    uint32_t *p_best_sad64x64;

    uint32_t *p_best_mv8x8;
    uint32_t *p_best_mv16x16;
    uint32_t *p_best_mv32x32;
    uint32_t *p_best_mv64x64;

    uint32_t *p_best_sad64x32;
    uint32_t *p_best_sad32x16;
    uint32_t *p_best_sad16x8;
    uint32_t *p_best_sad32x64;
    uint32_t *p_best_sad16x32;
    uint32_t *p_best_sad8x16;
    uint32_t *p_best_sad64x16;
    uint32_t *p_best_sad32x8;
    uint32_t *p_best_sad64x48;
    uint32_t *p_best_sad32x24;
    uint32_t *p_best_sad16x64;
    uint32_t *p_best_sad8x32;
    uint32_t *p_best_sad48x64;
    uint32_t *p_best_sad24x32;

    uint32_t *p_best_mv64x32;
    uint32_t *p_best_mv32x16;
    uint32_t *p_best_mv16x8;
    uint32_t *p_best_mv32x64;
    uint32_t *p_best_mv16x32;
    uint32_t *p_best_mv8x16;
    uint32_t *p_best_mv64x16;
    uint32_t *p_best_mv32x8;
    uint32_t *p_best_mv64x48;
    uint32_t *p_best_mv32x24;
    uint32_t *p_best_mv16x64;
    uint32_t *p_best_mv8x32;
    uint32_t *p_best_mv48x64;
    uint32_t *p_best_mv24x32;

    uint32_t p_sad32x32[4];
    uint32_t p_sad16x16[16];
    uint32_t p_sad8x8[64];

    uint8_t  psub_pel_direction64x64;

    uint8_t  psub_pel_direction32x32[4];
    uint8_t  psub_pel_direction16x16[16];
    uint8_t  psub_pel_direction8x8[64];

    uint32_t p_sb_best_sad[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX][MAX_ME_PU_COUNT];
    uint32_t p_sb_best_mv[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX][MAX_ME_PU_COUNT];
    uint32_t p_sb_bipred_sad[MAX_ME_PU_COUNT];//needs to be upgraded to 209 pus

    uint32_t p_sb_best_ssd[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX][MAX_ME_PU_COUNT];
    uint32_t *p_best_ssd8x8;
    uint32_t *p_best_ssd16x16;
    uint32_t *p_best_ssd32x32;
    uint32_t *p_best_ssd64x64;

    uint16_t *p_eight_pos_sad16x16;

    uint8_t  hme_search_type;

    // Multi-Mode signal(s)
    uint8_t  fractional_search_method;
    uint8_t  fractional_search_model;
    EB_BOOL  fractional_search64x64;
    EB_BOOL  single_hme_quadrant;

    // ME
    uint8_t  search_area_width;
    uint8_t  search_area_height;
    // HME
    uint16_t number_hme_search_region_in_width;
    uint16_t number_hme_search_region_in_height;
    uint16_t hme_level0_total_search_area_width;
    uint16_t hme_level0_total_search_area_height;
    uint16_t hme_level0_search_area_in_width_array[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    uint16_t hme_level0_search_area_in_height_array[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    uint16_t hme_level1_search_area_in_width_array[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    uint16_t hme_level1_search_area_in_height_array[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    uint16_t hme_level2_search_area_in_width_array[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    uint16_t hme_level2_search_area_in_height_array[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];

} MeContext;

extern EbErrorType eb_vp9_me_context_ctor(
    MeContext **object_dbl_ptr);

#ifdef __cplusplus
}
#endif
#endif // EbMotionEstimationContext_h
