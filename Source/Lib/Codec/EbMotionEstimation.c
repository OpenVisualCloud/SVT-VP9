/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdio.h>
#include <stdint.h>

#include "EbDefinitions.h"

#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbMotionEstimation.h"
#include "EbUtility.h"

#include "EbComputeSAD.h"
#include "EbReferenceObject.h"
#include "EbAvcStyleMcp.h"
#include "EbMeSadCalculation.h"

#include <math.h>
#include "EbPictureOperators.h"

/********************************************
 * Macros
 ********************************************/
#define SIXTEENTH_DECIMATED_SEARCH_WIDTH    16
#define SIXTEENTH_DECIMATED_SEARCH_HEIGHT    8

/********************************************
 * Constants
 ********************************************/

#define TOP_LEFT_POSITION       0
#define TOP_POSITION            1
#define TOP_RIGHT_POSITION      2
#define RIGHT_POSITION          3
#define BOTTOM_RIGHT_POSITION   4
#define BOTTOM_POSITION         5
#define BOTTOM_LEFT_POSITION    6
#define LEFT_POSITION           7

// The interpolation is performed using a set of three 4 tap filters
#define IFShiftAvcStyle         1
#define F0 0
#define F1 1
#define F2 2

#define  MAX_SAD_VALUE 64*64*255

uint32_t eb_vp9_tab32x32[16] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15 };
uint32_t eb_vp9_tab8x8[64] = { 0, 1, 4, 5, 16, 17, 20, 21, 2, 3, 6, 7, 18, 19, 22, 23, 8, 9, 12, 13, 24, 25, 28, 29, 10, 11, 14, 15, 26, 27, 30, 31,
32, 33, 36, 37, 48, 49, 52, 53, 34, 35, 38, 39, 50, 51, 54, 55, 40, 41, 44, 45, 56, 57, 60, 61, 42, 43, 46, 47, 58, 59, 62, 63
};

static const uint32_t partition_width[MAX_ME_PU_COUNT] = {
    64,                                                                     // 0
    32, 32, 32, 32,                                                         // 1-4
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,   // 5-20
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,   // 21-36
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,   // 37-52
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,   // 53-68
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};  // 69-84

static const uint32_t partition_height[MAX_ME_PU_COUNT] = {
    64,
    32, 32, 32, 32,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

static const uint32_t pu_search_index_map[MAX_ME_PU_COUNT][2] = {
    { 0, 0 },
    { 0, 0 }, { 32, 0 }, { 0, 32 }, { 32, 32 },

    { 0, 0 }, { 16, 0 }, { 32, 0 }, { 48, 0 },
    { 0, 16 }, { 16, 16 }, { 32, 16 }, { 48, 16 },
    { 0, 32 }, { 16, 32 }, { 32, 32 }, { 48, 32 },
    { 0, 48 }, { 16, 48 }, { 32, 48 }, { 48, 48 },

    { 0, 0 }, { 8, 0 }, { 16, 0 }, { 24, 0 }, { 32, 0 }, { 40, 0 }, { 48, 0 }, { 56, 0 },
    { 0, 8 }, { 8, 8 }, { 16, 8 }, { 24, 8 }, { 32, 8 }, { 40, 8 }, { 48, 8 }, { 56, 8 },
    { 0, 16 }, { 8, 16 }, { 16, 16 }, { 24, 16 }, { 32, 16 }, { 40, 16 }, { 48, 16 }, { 56, 16 },
    { 0, 24 }, { 8, 24 }, { 16, 24 }, { 24, 24 }, { 32, 24 }, { 40, 24 }, { 48, 24 }, { 56, 24 },
    { 0, 32 }, { 8, 32 }, { 16, 32 }, { 24, 32 }, { 32, 32 }, { 40, 32 }, { 48, 32 }, { 56, 32 },
    { 0, 40 }, { 8, 40 }, { 16, 40 }, { 24, 40 }, { 32, 40 }, { 40, 40 }, { 48, 40 }, { 56, 40 },
    { 0, 48 }, { 8, 48 }, { 16, 48 }, { 24, 48 }, { 32, 48 }, { 40, 48 }, { 48, 48 }, { 56, 48 },
    { 0, 56 }, { 8, 56 }, { 16, 56 }, { 24, 56 }, { 32, 56 }, { 40, 56 }, { 48, 56 }, { 56, 56 }};

static const uint8_t sub_position_type[16] = { 0, 2, 1, 2, 2, 2, 2, 2, 1, 2, 1, 2, 2, 2, 2, 2 };

#define REFERENCE_PIC_LIST_0  0
#define REFERENCE_PIC_LIST_1  1
/*******************************************
* GetEightHorizontalSearchPointResultsAll85CUs
*******************************************/
static void eb_vp9_get_eight_horizontal_search_point_results_all85_p_us_c(
    MeContext *context_ptr,
    uint32_t   list_index,
    uint32_t   search_region_index,
    uint32_t   x_search_index,
    uint32_t   y_search_index
)
{
    uint8_t  *src_ptr                 = context_ptr->sb_src_ptr;

    uint8_t  *ref_ptr                 = context_ptr->integer_buffer_ptr[list_index][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * context_ptr->interpolated_full_stride[list_index][0]);

    uint32_t refluma_stride           = context_ptr->interpolated_full_stride[list_index][0];

    uint32_t search_position_tl_index = search_region_index;
    uint32_t search_position_index;
    uint32_t block_index;

    uint32_t src_next16x16_offset     = (MAX_SB_SIZE << 4);
    uint32_t ref_next16x16_offset     = (refluma_stride << 4);

    uint32_t curr_mv_y                = (((uint16_t)y_search_index) << 18);
    uint16_t curr_mv_x                = (((uint16_t)x_search_index << 2));
    uint32_t curr_mv                  = curr_mv_y | curr_mv_x;

    uint32_t *p_best_sad8x8           = context_ptr->p_best_sad8x8;
    uint32_t *p_best_sad16x16         = context_ptr->p_best_sad16x16;
    uint32_t *p_best_sad32x32         = context_ptr->p_best_sad32x32;
    uint32_t *p_best_sad64x64         = context_ptr->p_best_sad64x64;

    uint32_t *p_best_mv8x8            = context_ptr->p_best_mv8x8;
    uint32_t *p_best_mv16x16          = context_ptr->p_best_mv16x16;
    uint32_t *p_best_mv32x32          = context_ptr->p_best_mv32x32;
    uint32_t *p_best_mv64x64          = context_ptr->p_best_mv64x64;

    uint16_t *p_sad16x16              = context_ptr->p_eight_pos_sad16x16;

    /*
    ----------------------    ----------------------
    |  16x16_0  |  16x16_1  |  16x16_4  |  16x16_5  |
    ----------------------    ----------------------
    |  16x16_2  |  16x16_3  |  16x16_6  |  16x16_7  |
    -----------------------   -----------------------
    |  16x16_8  |  16x16_9  |  16x16_12 |  16x16_13 |
    ----------------------    ----------------------
    |  16x16_10 |  16x16_11 |  16x16_14 |  16x16_15 |
    -----------------------   -----------------------
    */

    const uint32_t  src_stride = context_ptr->sb_src_stride;
    src_next16x16_offset = src_stride << 4;

    //---- 16x16_0
    block_index = 0;
    search_position_index = search_position_tl_index;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[0], &p_best_mv8x8[0], &p_best_sad16x16[0], &p_best_mv16x16[0], curr_mv, &p_sad16x16[0 * 8]);
    //---- 16x16_1
    block_index = block_index + 16;
    search_position_index = search_position_tl_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[4], &p_best_mv8x8[4], &p_best_sad16x16[1], &p_best_mv16x16[1], curr_mv, &p_sad16x16[1 * 8]);
    //---- 16x16_4
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[16], &p_best_mv8x8[16], &p_best_sad16x16[4], &p_best_mv16x16[4], curr_mv, &p_sad16x16[4 * 8]);
    //---- 16x16_5
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[20], &p_best_mv8x8[20], &p_best_sad16x16[5], &p_best_mv16x16[5], curr_mv, &p_sad16x16[5 * 8]);

    //---- 16x16_2
    block_index = src_next16x16_offset;
    search_position_index = search_position_tl_index + ref_next16x16_offset;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[8], &p_best_mv8x8[8], &p_best_sad16x16[2], &p_best_mv16x16[2], curr_mv, &p_sad16x16[2 * 8]);
    //---- 16x16_3
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[12], &p_best_mv8x8[12], &p_best_sad16x16[3], &p_best_mv16x16[3], curr_mv, &p_sad16x16[3 * 8]);
    //---- 16x16_6
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[24], &p_best_mv8x8[24], &p_best_sad16x16[6], &p_best_mv16x16[6], curr_mv, &p_sad16x16[6 * 8]);
    //---- 16x16_7
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[28], &p_best_mv8x8[28], &p_best_sad16x16[7], &p_best_mv16x16[7], curr_mv, &p_sad16x16[7 * 8]);

    //---- 16x16_8
    block_index = (src_next16x16_offset << 1);
    search_position_index = search_position_tl_index + (ref_next16x16_offset << 1);
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[32], &p_best_mv8x8[32], &p_best_sad16x16[8], &p_best_mv16x16[8], curr_mv, &p_sad16x16[8 * 8]);
    //---- 16x16_9
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[36], &p_best_mv8x8[36], &p_best_sad16x16[9], &p_best_mv16x16[9], curr_mv, &p_sad16x16[9 * 8]);
    //---- 16x16_12
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[48], &p_best_mv8x8[48], &p_best_sad16x16[12], &p_best_mv16x16[12], curr_mv, &p_sad16x16[12 * 8]);
    //---- 16x1_13
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[52], &p_best_mv8x8[52], &p_best_sad16x16[13], &p_best_mv16x16[13], curr_mv, &p_sad16x16[13 * 8]);

    //---- 16x16_10
    block_index = (src_next16x16_offset * 3);
    search_position_index = search_position_tl_index + (ref_next16x16_offset * 3);
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[40], &p_best_mv8x8[40], &p_best_sad16x16[10], &p_best_mv16x16[10], curr_mv, &p_sad16x16[10 * 8]);
    //---- 16x16_11
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[44], &p_best_mv8x8[44], &p_best_sad16x16[11], &p_best_mv16x16[11], curr_mv, &p_sad16x16[11 * 8]);
    //---- 16x16_14
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[56], &p_best_mv8x8[56], &p_best_sad16x16[14], &p_best_mv16x16[14], curr_mv, &p_sad16x16[14 * 8]);
    //---- 16x16_15
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[60], &p_best_mv8x8[60], &p_best_sad16x16[15], &p_best_mv16x16[15], curr_mv, &p_sad16x16[15 * 8]);

    //32x32 and 64x64
    eb_vp9_get_eight_horizontal_search_point_results_32x32_64x64(p_sad16x16, p_best_sad32x32, p_best_sad64x64, p_best_mv32x32, p_best_mv64x64, curr_mv);

}

#ifdef DISABLE_AVX512
/*******************************************
* GetEightHorizontalSearchPointResultsAll85CUs
*******************************************/
static void eb_vp9_get_eight_horizontal_search_point_results_all85_p_us(
    MeContext *context_ptr,
    uint32_t   list_index,
    uint32_t   search_region_index,
    uint32_t   x_search_index,
    uint32_t   y_search_index
)
{
    uint8_t *src_ptr = context_ptr->sb_src_ptr;

    uint8_t *ref_ptr = context_ptr->integer_buffer_ptr[list_index][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * context_ptr->interpolated_full_stride[list_index][0]);

    uint32_t refluma_stride = context_ptr->interpolated_full_stride[list_index][0];

    uint32_t search_position_tl_index = search_region_index;
    uint32_t search_position_index;
    uint32_t block_index;

    uint32_t src_next16x16_offset = (MAX_SB_SIZE << 4);
    uint32_t ref_next16x16_offset = (refluma_stride << 4);

    uint32_t curr_mv_y = (((uint16_t)y_search_index) << 18);
    uint16_t curr_mv_x = (((uint16_t)x_search_index << 2));
    uint32_t curr_mv = curr_mv_y | curr_mv_x;

    uint32_t *p_best_sad8x8 = context_ptr->p_best_sad8x8;
    uint32_t *p_best_sad16x16 = context_ptr->p_best_sad16x16;
    uint32_t *p_best_sad32x32 = context_ptr->p_best_sad32x32;
    uint32_t *p_best_sad64x64 = context_ptr->p_best_sad64x64;

    uint32_t *p_best_mv8x8 = context_ptr->p_best_mv8x8;
    uint32_t *p_best_mv16x16 = context_ptr->p_best_mv16x16;
    uint32_t *p_best_mv32x32 = context_ptr->p_best_mv32x32;
    uint32_t *p_best_mv64x64 = context_ptr->p_best_mv64x64;

    uint16_t *p_sad16x16 = context_ptr->p_eight_pos_sad16x16;

    /*
    ----------------------    ----------------------
    |  16x16_0  |  16x16_1  |  16x16_4  |  16x16_5  |
    ----------------------    ----------------------
    |  16x16_2  |  16x16_3  |  16x16_6  |  16x16_7  |
    -----------------------   -----------------------
    |  16x16_8  |  16x16_9  |  16x16_12 |  16x16_13 |
    ----------------------    ----------------------
    |  16x16_10 |  16x16_11 |  16x16_14 |  16x16_15 |
    -----------------------   -----------------------
    */

    const uint32_t  src_stride = context_ptr->sb_src_stride;
    src_next16x16_offset = src_stride << 4;

    //---- 16x16_0
    block_index = 0;
    search_position_index = search_position_tl_index;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[0], &p_best_mv8x8[0], &p_best_sad16x16[0], &p_best_mv16x16[0], curr_mv, &p_sad16x16[0 * 8]);
    //---- 16x16_1
    block_index = block_index + 16;
    search_position_index = search_position_tl_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[4], &p_best_mv8x8[4], &p_best_sad16x16[1], &p_best_mv16x16[1], curr_mv, &p_sad16x16[1 * 8]);
    //---- 16x16_4
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[16], &p_best_mv8x8[16], &p_best_sad16x16[4], &p_best_mv16x16[4], curr_mv, &p_sad16x16[4 * 8]);
    //---- 16x16_5
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[20], &p_best_mv8x8[20], &p_best_sad16x16[5], &p_best_mv16x16[5], curr_mv, &p_sad16x16[5 * 8]);

    //---- 16x16_2
    block_index = src_next16x16_offset;
    search_position_index = search_position_tl_index + ref_next16x16_offset;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[8], &p_best_mv8x8[8], &p_best_sad16x16[2], &p_best_mv16x16[2], curr_mv, &p_sad16x16[2 * 8]);
    //---- 16x16_3
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[12], &p_best_mv8x8[12], &p_best_sad16x16[3], &p_best_mv16x16[3], curr_mv, &p_sad16x16[3 * 8]);
    //---- 16x16_6
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[24], &p_best_mv8x8[24], &p_best_sad16x16[6], &p_best_mv16x16[6], curr_mv, &p_sad16x16[6 * 8]);
    //---- 16x16_7
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[28], &p_best_mv8x8[28], &p_best_sad16x16[7], &p_best_mv16x16[7], curr_mv, &p_sad16x16[7 * 8]);

    //---- 16x16_8
    block_index = (src_next16x16_offset << 1);
    search_position_index = search_position_tl_index + (ref_next16x16_offset << 1);
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[32], &p_best_mv8x8[32], &p_best_sad16x16[8], &p_best_mv16x16[8], curr_mv, &p_sad16x16[8 * 8]);
    //---- 16x16_9
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[36], &p_best_mv8x8[36], &p_best_sad16x16[9], &p_best_mv16x16[9], curr_mv, &p_sad16x16[9 * 8]);
    //---- 16x16_12
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[48], &p_best_mv8x8[48], &p_best_sad16x16[12], &p_best_mv16x16[12], curr_mv, &p_sad16x16[12 * 8]);
    //---- 16x1_13
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[52], &p_best_mv8x8[52], &p_best_sad16x16[13], &p_best_mv16x16[13], curr_mv, &p_sad16x16[13 * 8]);

    //---- 16x16_10
    block_index = (src_next16x16_offset * 3);
    search_position_index = search_position_tl_index + (ref_next16x16_offset * 3);
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[40], &p_best_mv8x8[40], &p_best_sad16x16[10], &p_best_mv16x16[10], curr_mv, &p_sad16x16[10 * 8]);
    //---- 16x16_11
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[44], &p_best_mv8x8[44], &p_best_sad16x16[11], &p_best_mv16x16[11], curr_mv, &p_sad16x16[11 * 8]);
    //---- 16x16_14
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[56], &p_best_mv8x8[56], &p_best_sad16x16[14], &p_best_mv16x16[14], curr_mv, &p_sad16x16[14 * 8]);
    //---- 16x16_15
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](src_ptr + block_index, context_ptr->sb_src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[60], &p_best_mv8x8[60], &p_best_sad16x16[15], &p_best_mv16x16[15], curr_mv, &p_sad16x16[15 * 8]);

    //32x32 and 64x64
    eb_vp9_get_eight_horizontal_search_point_results_32x32_64x64_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](p_sad16x16, p_best_sad32x32, p_best_sad64x64, p_best_mv32x32, p_best_mv64x64, curr_mv);

}
#endif

typedef void(*EbFpSearchFunc)(
    MeContext *context_ptr,
    uint32_t   list_index,
    uint32_t   search_region_index,
    uint32_t   x_search_index,
    uint32_t   y_search_index);

/*****************************
* Function Tables
*****************************/
static EbFpSearchFunc FUNC_TABLE eb_vp9_get_eight_horizontal_search_point_results_all85_p_us_func_ptr_array[ASM_TYPE_TOTAL] =
{
    eb_vp9_get_eight_horizontal_search_point_results_all85_p_us_c,
#ifndef DISABLE_AVX512
    eb_vp9_get_eight_horizontal_search_point_results_all85_p_us_avx512_intrin
#else
    eb_vp9_get_eight_horizontal_search_point_results_all85_p_us
#endif
};

/*******************************************
 * get_search_point_results
 *******************************************/
static void get_search_point_results(
    MeContext *context_ptr,                     // input parameter, ME context Ptr, used to get LCU Ptr
    uint32_t   list_index,                      // input parameter, reference list index
    uint32_t   search_region_index,             // input parameter, search area origin, used to point to reference samples
    uint32_t   x_search_index,                  // input parameter, search region position in the horizontal direction, used to derive x_mv
    uint32_t   y_search_index)                  // input parameter, search region position in the vertical direction, used to derive y_mv
{
    uint8_t  *src_ptr                 = context_ptr->sb_src_ptr;

    uint8_t  *ref_ptr                  = context_ptr->integer_buffer_ptr[list_index][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * context_ptr->interpolated_full_stride[list_index][0]);

    uint32_t refluma_stride           = context_ptr->interpolated_full_stride[list_index][0];

    uint32_t search_position_tl_index = search_region_index;
    uint32_t search_position_index;
    uint32_t block_index;

    uint32_t src_next16x16_offset     = (MAX_SB_SIZE << 4);
    uint32_t ref_next16x16_offset     = (refluma_stride << 4);

    uint32_t curr_mv1                  = (((uint16_t)y_search_index) << 18);
    uint16_t curr_mv2                  = (((uint16_t)x_search_index << 2));
    uint32_t curr_mv                  = curr_mv1 | curr_mv2;

    uint32_t  *p_best_sad8x8          = context_ptr->p_best_sad8x8;
    uint32_t  *p_best_sad16x16        = context_ptr->p_best_sad16x16;
    uint32_t  *p_best_sad32x32        = context_ptr->p_best_sad32x32;
    uint32_t  *p_best_sad64x64        = context_ptr->p_best_sad64x64;

    uint32_t  *p_best_mv8x8           = context_ptr->p_best_mv8x8;
    uint32_t  *p_best_mv16x16         = context_ptr->p_best_mv16x16;
    uint32_t  *p_best_mv32x32         = context_ptr->p_best_mv32x32;
    uint32_t  *p_best_mv64x64         = context_ptr->p_best_mv64x64;
    uint32_t  *p_sad16x16             = context_ptr->p_sad16x16;

    //TODO: block_index search_position_index could be removed  + Connect asm_type

    const uint32_t  src_stride        = context_ptr->sb_src_stride;
    src_next16x16_offset            = src_stride << 4;

    //---- 16x16 : 0
    block_index                     = 0;
    search_position_index = search_position_tl_index;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[0], &p_best_sad16x16[0], &p_best_mv8x8[0], &p_best_mv16x16[0], curr_mv, &p_sad16x16[0]);

    //---- 16x16 : 1
    block_index = block_index + 16;
    search_position_index = search_position_tl_index + 16;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[4], &p_best_sad16x16[1], &p_best_mv8x8[4], &p_best_mv16x16[1], curr_mv, &p_sad16x16[1]);

    //---- 16x16 : 4
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[16], &p_best_sad16x16[4], &p_best_mv8x8[16], &p_best_mv16x16[4], curr_mv, &p_sad16x16[4]);

    //---- 16x16 : 5
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[20], &p_best_sad16x16[5], &p_best_mv8x8[20], &p_best_mv16x16[5], curr_mv, &p_sad16x16[5]);

    //---- 16x16 : 2
    block_index = src_next16x16_offset;
    search_position_index = search_position_tl_index + ref_next16x16_offset;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[8], &p_best_sad16x16[2], &p_best_mv8x8[8], &p_best_mv16x16[2], curr_mv, &p_sad16x16[2]);

    //---- 16x16 : 3
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[12], &p_best_sad16x16[3], &p_best_mv8x8[12], &p_best_mv16x16[3], curr_mv, &p_sad16x16[3]);

    //---- 16x16 : 6
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[24], &p_best_sad16x16[6], &p_best_mv8x8[24], &p_best_mv16x16[6], curr_mv, &p_sad16x16[6]);

    //---- 16x16 : 7
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[28], &p_best_sad16x16[7], &p_best_mv8x8[28], &p_best_mv16x16[7], curr_mv, &p_sad16x16[7]);

    //---- 16x16 : 8
    block_index = (src_next16x16_offset << 1);
    search_position_index = search_position_tl_index + (ref_next16x16_offset << 1);
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[32], &p_best_sad16x16[8], &p_best_mv8x8[32], &p_best_mv16x16[8], curr_mv, &p_sad16x16[8]);

    //---- 16x16 : 9
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[36], &p_best_sad16x16[9], &p_best_mv8x8[36], &p_best_mv16x16[9], curr_mv, &p_sad16x16[9]);

    //---- 16x16 : 12
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;    search_position_index = search_position_index + 16;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[48], &p_best_sad16x16[12], &p_best_mv8x8[48], &p_best_mv16x16[12], curr_mv, &p_sad16x16[12]);

    //---- 16x16 : 13
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[52], &p_best_sad16x16[13], &p_best_mv8x8[52], &p_best_mv16x16[13], curr_mv, &p_sad16x16[13]);

    //---- 16x16 : 10
    block_index = (src_next16x16_offset * 3);
    search_position_index = search_position_tl_index + (ref_next16x16_offset * 3);
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[40], &p_best_sad16x16[10], &p_best_mv8x8[40], &p_best_mv16x16[10], curr_mv, &p_sad16x16[10]);

    //---- 16x16 : 11
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[44], &p_best_sad16x16[11], &p_best_mv8x8[44], &p_best_mv16x16[11], curr_mv, &p_sad16x16[11]);

    //---- 16x16 : 14
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[56], &p_best_sad16x16[14], &p_best_mv8x8[56], &p_best_mv16x16[14], curr_mv, &p_sad16x16[14]);

    //---- 16x16 : 15
    block_index = block_index + 16;
    search_position_index = search_position_index + 16;
    eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](src_ptr + block_index, src_stride, ref_ptr + search_position_index, refluma_stride, &p_best_sad8x8[60], &p_best_sad16x16[15], &p_best_mv8x8[60], &p_best_mv16x16[15], curr_mv, &p_sad16x16[15]);

    eb_vp9_sad_calculation_32x32_64x64_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](p_sad16x16, p_best_sad32x32, p_best_sad64x64, p_best_mv32x32, p_best_mv64x64, curr_mv);

}

/*******************************************
 * full_pel_search_sb
 *******************************************/
static void full_pel_search_sb(
    MeContext *context_ptr,
    uint32_t   list_index,
    int16_t    x_search_area_origin,
    int16_t    y_search_area_origin,
    uint32_t   search_area_width,
    uint32_t   search_area_height
)
{

    uint32_t  x_search_index, y_search_index;

    uint32_t  search_area_width_rest8 = search_area_width & 7;
    uint32_t  search_area_width_mult8 = search_area_width - search_area_width_rest8;

    for (y_search_index = 0; y_search_index < search_area_height; y_search_index++){

        for (x_search_index = 0; x_search_index < search_area_width_mult8; x_search_index += 8){

            //this function will do:  x_search_index, +1, +2, ..., +7

            eb_vp9_get_eight_horizontal_search_point_results_all85_p_us_func_ptr_array[ (eb_vp9_ASM_TYPES & AVX2_MASK) && 1 ](
                context_ptr,
                list_index,
                x_search_index + y_search_index * context_ptr->interpolated_full_stride[list_index][0],
                x_search_index + x_search_area_origin,
                y_search_index + y_search_area_origin
            );
        }

        for (x_search_index = search_area_width_mult8; x_search_index < search_area_width; x_search_index++){

            get_search_point_results(
                context_ptr,
                list_index,
                x_search_index + y_search_index * context_ptr->interpolated_full_stride[list_index][0],
                x_search_index + x_search_area_origin,
                y_search_index + y_search_area_origin);

        }

    }
}

/*******************************************
 * InterpolateSearchRegion AVC
 *   interpolates the search area
 *   the whole search area is interpolated 15 times
 *   for each sub position an interpolation is done
 *   15 buffers are required for the storage of the interpolated samples.
 *   F0: {-4, 54, 16, -2}
 *   F1: {-4, 36, 36, -4}
 *   F2: {-2, 16, 54, -4}
 ********************************************/
void interpolate_search_region_avc(
    MeContext *context_ptr,            // input/output parameter, ME context ptr, used to get/set interpolated search area Ptr
    uint32_t   list_index,             // reference picture list index
    uint8_t   *search_region_buffer,   // input parameter, search region index, used to point to reference samples
    uint32_t   luma_stride,            // input parameter, reference Picture stride
    uint32_t   search_area_width,      // input parameter, search area width
    uint32_t   search_area_height)     // input parameter, search area height
{

    //      0    1    2    3
    // 0    A    a    b    c
    // 1    d    e    f    g
    // 2    h    i    j    k
    // 3    n    p    q    r

    // Position  Frac-pos Y  Frac-pos X  Horizontal filter  Vertical filter
    // A         0           0           -                  -
    // a         0           1           F0                 -
    // b         0           2           F1                 -
    // c         0           3           F2                 -
    // d         1           0           -                  F0
    // e         1           1           F0                 F0
    // f         1           2           F1                 F0
    // g         1           3           F2                 F0
    // h         2           0           -                  F1
    // i         2           1           F0                 F1
    // j         2           2           F1                 F1
    // k         2           3           F2                 F1
    // n         3           0           -                  F2
    // p         3           1           F0                 F2
    // q         3           2           F1                 F2
    // r         3           3           F2                 F2

    // Start a b c

    // The Search area needs to be a multiple of 8 to align with the ASM kernel
    // Also the search area must be oversized by 2 to account for edge conditions
    uint32_t search_area_width_for_asm = ROUND_UP_MUL_8(search_area_width + 2);

    // Half pel interpolation of the search region using f1 -> posb_buffer
    if (search_area_width_for_asm){

        avc_style_uni_pred_luma_if_function_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][1](
            search_region_buffer - (ME_FILTER_TAP >> 1) * luma_stride - (ME_FILTER_TAP >> 1) + 1,
            luma_stride,
            context_ptr->posb_buffer[list_index][0],
            context_ptr->interpolated_stride,
            search_area_width_for_asm,
            search_area_height + ME_FILTER_TAP,
            context_ptr->avctemp_buffer,
            2);
    }

    // Half pel interpolation of the search region using f1 -> posh_buffer
    if (search_area_width_for_asm){
        avc_style_uni_pred_luma_if_function_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][2](
            search_region_buffer - (ME_FILTER_TAP >> 1) * luma_stride - 1 + luma_stride,
            luma_stride,
            context_ptr->posh_buffer[list_index][0],
            context_ptr->interpolated_stride,
            search_area_width_for_asm,
            search_area_height + 1,
            context_ptr->avctemp_buffer,
            2);
    }

    if (search_area_width_for_asm){
        // Half pel interpolation of the search region using f1 -> posj_buffer
        avc_style_uni_pred_luma_if_function_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][2](
            context_ptr->posb_buffer[list_index][0] + context_ptr->interpolated_stride,
            context_ptr->interpolated_stride,
            context_ptr->posj_buffer[list_index][0],
            context_ptr->interpolated_stride,
            search_area_width_for_asm,
            search_area_height + 1,
            context_ptr->avctemp_buffer,
            2);
    }

    return;
}

/*******************************************
 * pu_half_pel_refinement
 *   performs Half Pel refinement for one PU
 *******************************************/
static void pu_half_pel_refinement(
    SequenceControlSet *sequence_control_set_ptr,           // input parameter, Sequence control set Ptr
    MeContext          *context_ptr,                        // input parameter, ME context Ptr, used to get LCU Ptr
    uint8_t            *ref_buffer,
    uint32_t            ref_stride,
    uint32_t           *p_best_ssd,
    uint32_t            pu_sb_buffer_index,                 // input parameter, PU origin, used to point to source samples
    uint8_t            *posb_buffer,                        // input parameter, position "b" interpolated search area Ptr
    uint8_t            *posh_buffer,                        // input parameter, position "h" interpolated search area Ptr
    uint8_t            *posj_buffer,                        // input parameter, position "j" interpolated search area Ptr
    uint32_t            pu_width,                           // input parameter, PU width
    uint32_t            pu_height,                          // input parameter, PU height
    int16_t             x_search_area_origin,               // input parameter, search area origin in the horizontal direction, used to point to reference samples
    int16_t             y_search_area_origin,               // input parameter, search area origin in the vertical direction, used to point to reference samples
    uint32_t           *p_best_sad,
    uint32_t           *p_best_mv,
    uint8_t            *psub_pel_direction
)
{

    EncodeContext *encode_context_ptr = sequence_control_set_ptr->encode_context_ptr;

    int32_t  search_region_index;
    uint64_t best_half_sad                        = 0;
    uint64_t distortion_left_position           = 0;
    uint64_t distortion_right_position          = 0;
    uint64_t distortion_top_position            = 0;
    uint64_t distortion_bottom_position         = 0;
    uint64_t distortion_top_left_position       = 0;
    uint64_t distortion_top_right_position      = 0;
    uint64_t distortion_bottom_left_position    = 0;
    uint64_t distortion_bottom_right_position   = 0;

    int16_t x_mv_half[8];
    int16_t y_mv_half[8];

    int16_t x_mv                               = _MVXT(*p_best_mv);
    int16_t y_mv                               = _MVYT(*p_best_mv);
    int16_t x_search_index                     = (x_mv >> 2) - x_search_area_origin;
    int16_t y_search_index                     = (y_mv >> 2) - y_search_area_origin;

    (void)sequence_control_set_ptr;
    (void)encode_context_ptr;

    x_mv_half[0] = x_mv - 2; // L  position
    x_mv_half[1] = x_mv + 2; // R  position
    x_mv_half[2] = x_mv;     // T  position
    x_mv_half[3] = x_mv;     // B  position
    x_mv_half[4] = x_mv - 2; // TL position
    x_mv_half[5] = x_mv + 2; // TR position
    x_mv_half[6] = x_mv + 2; // BR position
    x_mv_half[7] = x_mv - 2; // BL position

    y_mv_half[0] = y_mv;     // L  position
    y_mv_half[1] = y_mv;     // R  position
    y_mv_half[2] = y_mv - 2; // T  position
    y_mv_half[3] = y_mv + 2; // B  position
    y_mv_half[4] = y_mv - 2; // TL position
    y_mv_half[5] = y_mv - 2; // TR position
    y_mv_half[6] = y_mv + 2; // BR position
    y_mv_half[7] = y_mv + 2; // BL position

    // Compute SSD for the best full search candidate
    if (context_ptr->fractional_search_method == SSD_SEARCH) {
        *p_best_ssd = (uint32_t) spatialfull_distortion_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][Log2f(pu_width) - 2](
            &(context_ptr->sb_src_ptr[pu_sb_buffer_index]),
            context_ptr->sb_src_stride,
            &(ref_buffer[y_search_index * ref_stride + x_search_index]),
            ref_stride,
            pu_height,
            pu_width);
    }

    {
        // L position
        search_region_index = x_search_index + (int16_t)context_ptr->interpolated_stride * y_search_index;

        distortion_left_position = (context_ptr->fractional_search_method == SSD_SEARCH) ?
            spatialfull_distortion_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][Log2f(pu_width) - 2](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posb_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width) :
            (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                (n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride << 1, &(posb_buffer[search_region_index]), context_ptr->interpolated_stride << 1, pu_height >> 1, pu_width)) << 1 :
                n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posb_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);

        if (context_ptr->fractional_search_method == SSD_SEARCH) {
            if (distortion_left_position < *p_best_ssd) {
                *p_best_sad = (uint32_t)n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posb_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);
                *p_best_mv = ((uint16_t)y_mv_half[0] << 16) | ((uint16_t)x_mv_half[0]);
                *p_best_ssd = (uint32_t)distortion_left_position;
            }
        }
        else {
            if (distortion_left_position < *p_best_sad) {
                *p_best_sad = (uint32_t)distortion_left_position;
                *p_best_mv = ((uint16_t)y_mv_half[0] << 16) | ((uint16_t)x_mv_half[0]);
            }
        }

        // R position
        search_region_index++;

        distortion_right_position = (context_ptr->fractional_search_method == SSD_SEARCH) ?
            spatialfull_distortion_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][Log2f(pu_width) - 2](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posb_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width) :
            (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                (n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride << 1, &(posb_buffer[search_region_index]), context_ptr->interpolated_stride << 1, pu_height >> 1, pu_width)) << 1 :
                 n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posb_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);

        if (context_ptr->fractional_search_method == SSD_SEARCH) {
            if (distortion_right_position < *p_best_ssd) {
                *p_best_sad = (uint32_t)n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posb_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);
                *p_best_mv = ((uint16_t)y_mv_half[1] << 16) | ((uint16_t)x_mv_half[1]);
                *p_best_ssd = (uint32_t)distortion_right_position;
            }
        }
        else {
            if (distortion_right_position < *p_best_sad) {
                *p_best_sad = (uint32_t)distortion_right_position;
                *p_best_mv = ((uint16_t)y_mv_half[1] << 16) | ((uint16_t)x_mv_half[1]);
            }
        }

        // T position
        search_region_index = x_search_index + (int16_t)context_ptr->interpolated_stride * y_search_index;

        distortion_top_position = (context_ptr->fractional_search_method == SSD_SEARCH) ?
            spatialfull_distortion_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][Log2f(pu_width) - 2](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posh_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width) :
            (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                (n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride << 1, &(posh_buffer[search_region_index]), context_ptr->interpolated_stride << 1, pu_height >> 1, pu_width)) << 1 :
                 n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posh_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);

        if (context_ptr->fractional_search_method == SSD_SEARCH) {
            if (distortion_top_position < *p_best_ssd) {
                *p_best_sad = (uint32_t)n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posh_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);
                *p_best_mv = ((uint16_t)y_mv_half[2] << 16) | ((uint16_t)x_mv_half[2]);
                *p_best_ssd = (uint32_t)distortion_top_position;
            }
        }
        else {
            if (distortion_top_position < *p_best_sad) {
                *p_best_sad = (uint32_t)distortion_top_position;
                *p_best_mv = ((uint16_t)y_mv_half[2] << 16) | ((uint16_t)x_mv_half[2]);
            }
        }

        // B position
        search_region_index += (int16_t)context_ptr->interpolated_stride;

        distortion_bottom_position = (context_ptr->fractional_search_method == SSD_SEARCH) ?
            spatialfull_distortion_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][Log2f(pu_width) - 2](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posh_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width) :
            (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                (n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride << 1, &(posh_buffer[search_region_index]), context_ptr->interpolated_stride << 1, pu_height >> 1, pu_width)) << 1 :
                n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posh_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);

        if (context_ptr->fractional_search_method == SSD_SEARCH) {
            if (distortion_bottom_position < *p_best_ssd) {
                *p_best_sad = (uint32_t)n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posh_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);
                *p_best_mv = ((uint16_t)y_mv_half[3] << 16) | ((uint16_t)x_mv_half[3]);
                *p_best_ssd = (uint32_t)distortion_bottom_position;
            }
        }
        else {
            if (distortion_bottom_position < *p_best_sad) {
                *p_best_sad = (uint32_t)distortion_bottom_position;
                *p_best_mv = ((uint16_t)y_mv_half[3] << 16) | ((uint16_t)x_mv_half[3]);
            }
        }

        //TL position
        search_region_index = x_search_index + (int16_t)context_ptr->interpolated_stride * y_search_index;

        distortion_top_left_position = (context_ptr->fractional_search_method == SSD_SEARCH) ?
            spatialfull_distortion_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][Log2f(pu_width) - 2](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width) :
            (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                (n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride << 1, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride << 1, pu_height >> 1, pu_width)) << 1 :
                n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);

        if (context_ptr->fractional_search_method == SSD_SEARCH) {
            if (distortion_top_left_position < *p_best_ssd) {
                *p_best_sad = (uint32_t)n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);
                *p_best_mv = ((uint16_t)y_mv_half[4] << 16) | ((uint16_t)x_mv_half[4]);
                *p_best_ssd = (uint32_t)distortion_top_left_position;
            }
        }
        else {
            if (distortion_top_left_position < *p_best_sad) {
                *p_best_sad = (uint32_t)distortion_top_left_position;
                *p_best_mv = ((uint16_t)y_mv_half[4] << 16) | ((uint16_t)x_mv_half[4]);
            }

        }

        //TR position
        search_region_index++;

        distortion_top_right_position = (context_ptr->fractional_search_method == SSD_SEARCH) ?
            spatialfull_distortion_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][Log2f(pu_width) - 2](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width) :
            (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                (n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride << 1, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride << 1, pu_height >> 1, pu_width)) << 1 :
                n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);

        if (context_ptr->fractional_search_method == SSD_SEARCH) {
            if (distortion_top_right_position < *p_best_ssd) {
                *p_best_sad = (uint32_t)n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);
                *p_best_mv = ((uint16_t)y_mv_half[5] << 16) | ((uint16_t)x_mv_half[5]);
                *p_best_ssd = (uint32_t)distortion_top_right_position;
            }
        }
        else {
            if (distortion_top_right_position < *p_best_sad) {
                *p_best_sad = (uint32_t)distortion_top_right_position;
                *p_best_mv = ((uint16_t)y_mv_half[5] << 16) | ((uint16_t)x_mv_half[5]);
            }
        }

        //BR position
        search_region_index += (int16_t)context_ptr->interpolated_stride;

        distortion_bottom_right_position = (context_ptr->fractional_search_method == SSD_SEARCH) ?
            spatialfull_distortion_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][Log2f(pu_width) - 2](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width) :
            (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                (n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride << 1, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride << 1, pu_height >> 1, pu_width)) << 1 :
                n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);

        if (context_ptr->fractional_search_method == SSD_SEARCH) {
            if (distortion_bottom_right_position < *p_best_ssd) {
                *p_best_sad = (uint32_t)n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width);
                *p_best_mv = ((uint16_t)y_mv_half[6] << 16) | ((uint16_t)x_mv_half[6]);
                *p_best_ssd = (uint32_t)distortion_bottom_right_position;
            }
        }
        else {
            if (distortion_bottom_right_position < *p_best_sad) {
                *p_best_sad = (uint32_t)distortion_bottom_right_position;
                *p_best_mv = ((uint16_t)y_mv_half[6] << 16) | ((uint16_t)x_mv_half[6]);
            }
        }

        //BL position
        search_region_index--;

        distortion_bottom_left_position = (context_ptr->fractional_search_method == SSD_SEARCH) ?
            spatialfull_distortion_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][Log2f(pu_width) - 2](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width) :
            (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                (n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride << 1, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride << 1, pu_height >> 1, pu_width)) << 1 :
                (n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width));

        if (context_ptr->fractional_search_method == SSD_SEARCH) {
            if (distortion_bottom_left_position < *p_best_ssd) {
                *p_best_sad = (uint32_t)(n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_src_ptr[pu_sb_buffer_index]), context_ptr->sb_src_stride, &(posj_buffer[search_region_index]), context_ptr->interpolated_stride, pu_height, pu_width));
                *p_best_mv = ((uint16_t)y_mv_half[7] << 16) | ((uint16_t)x_mv_half[7]);
                *p_best_ssd = (uint32_t)distortion_bottom_left_position;
            }
        }
        else {
            if (distortion_bottom_left_position < *p_best_sad) {
                *p_best_sad = (uint32_t)distortion_bottom_left_position;
                *p_best_mv = ((uint16_t)y_mv_half[7] << 16) | ((uint16_t)x_mv_half[7]);
            }
        }
    }

    best_half_sad = MIN(distortion_left_position, MIN(distortion_right_position, MIN(distortion_top_position, MIN(distortion_bottom_position, MIN(distortion_top_left_position, MIN(distortion_top_right_position, MIN(distortion_bottom_left_position, distortion_bottom_right_position)))))));

    if (best_half_sad == distortion_left_position) {
        *psub_pel_direction = LEFT_POSITION;
    }
    else if (best_half_sad == distortion_right_position) {
        *psub_pel_direction = RIGHT_POSITION;
    }
    else if (best_half_sad == distortion_top_position) {
        *psub_pel_direction = TOP_POSITION;
    }
    else if (best_half_sad == distortion_bottom_position) {
        *psub_pel_direction = BOTTOM_POSITION;
    }
    else if (best_half_sad == distortion_top_left_position) {
        *psub_pel_direction = TOP_LEFT_POSITION;
    }
    else if (best_half_sad == distortion_top_right_position) {
        *psub_pel_direction = TOP_RIGHT_POSITION;
    }
    else if (best_half_sad == distortion_bottom_left_position) {
        *psub_pel_direction = BOTTOM_LEFT_POSITION;
    }
    else if (best_half_sad == distortion_bottom_right_position) {
        *psub_pel_direction = BOTTOM_RIGHT_POSITION;
    }

    return;
}

/*******************************************
 * half_pel_search_sb
 *   performs Half Pel refinement for the 85 PUs
 *******************************************/
void half_pel_search_sb(
    SequenceControlSet *sequence_control_set_ptr,             // input parameter, Sequence control set Ptr
    MeContext          *context_ptr,                          // input/output parameter, ME context Ptr, used to get/update ME results
    uint8_t            *ref_buffer,
    uint32_t            ref_stride,
    uint8_t            *posb_buffer,                          // input parameter, position "b" interpolated search area Ptr
    uint8_t            *posh_buffer,                          // input parameter, position "h" interpolated search area Ptr
    uint8_t            *posj_buffer,                          // input parameter, position "j" interpolated search area Ptr
    int16_t             x_search_area_origin,                 // input parameter, search area origin in the horizontal direction, used to point to reference samples
    int16_t             y_search_area_origin,                 // input parameter, search area origin in the vertical direction, used to point to reference samples
    EB_BOOL             disable8x8_cu_in_me_flag,
    EB_BOOL             enable_half_pel32x32,
    EB_BOOL             enable_half_pel16x16,
    EB_BOOL             enable_half_pel8x8){

    uint32_t idx;
    uint32_t pu_index;
    uint32_t pu_shift_x_index;
    uint32_t pu_shift_y_index;
    uint32_t pu_sb_buffer_index;
    uint32_t posb_buffer_index;
    uint32_t posh_buffer_index;
    uint32_t posj_buffer_index;

    if (context_ptr->fractional_search64x64){
        pu_half_pel_refinement(
            sequence_control_set_ptr,
            context_ptr,
            &(ref_buffer[0]),
            ref_stride,
            context_ptr->p_best_ssd64x64,
            0,
            &(posb_buffer[0]),
            &(posh_buffer[0]),
            &(posj_buffer[0]),
            64,
            64,
            x_search_area_origin,
            y_search_area_origin,
            context_ptr->p_best_sad64x64,
            context_ptr->p_best_mv64x64,
            &context_ptr->psub_pel_direction64x64);
    }

    if ( enable_half_pel32x32 )
    {
        // 32x32 [4 partitions]
        for (pu_index = 0; pu_index < 4; ++pu_index) {

            pu_shift_x_index = (pu_index & 0x01) << 5;
            pu_shift_y_index = (pu_index >> 1) << 5;

            pu_sb_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->sb_src_stride;
            posb_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->interpolated_stride;
            posh_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->interpolated_stride;
            posj_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->interpolated_stride;

            pu_half_pel_refinement(
                sequence_control_set_ptr,
                context_ptr,
                &(ref_buffer[pu_shift_y_index * ref_stride + pu_shift_x_index]),
                ref_stride,
                &context_ptr->p_best_ssd32x32[pu_index],
                pu_sb_buffer_index,
                &(posb_buffer[posb_buffer_index]),
                &(posh_buffer[posh_buffer_index]),
                &(posj_buffer[posj_buffer_index]),
                32,
                32,
                x_search_area_origin,
                y_search_area_origin,
                &context_ptr->p_best_sad32x32[pu_index],
                &context_ptr->p_best_mv32x32[pu_index],
                &context_ptr->psub_pel_direction32x32[pu_index]);
        }
    }
    if ( enable_half_pel16x16 )
    {
        // 16x16 [16 partitions]
        for (pu_index = 0; pu_index < 16; ++pu_index) {

            idx = eb_vp9_tab32x32[pu_index];   //TODO bitwise this

            pu_shift_x_index = (pu_index & 0x03) << 4;
            pu_shift_y_index = (pu_index >> 2) << 4;

            pu_sb_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->sb_src_stride;
            posb_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->interpolated_stride;
            posh_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->interpolated_stride;
            posj_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->interpolated_stride;

            pu_half_pel_refinement(
                sequence_control_set_ptr,
                context_ptr,
                &(ref_buffer[pu_shift_y_index * ref_stride + pu_shift_x_index]),
                ref_stride,
                &context_ptr->p_best_ssd16x16[idx],
                pu_sb_buffer_index,
                &(posb_buffer[posb_buffer_index]),
                &(posh_buffer[posh_buffer_index]),
                &(posj_buffer[posj_buffer_index]),
                16,
                16,
                x_search_area_origin,
                y_search_area_origin,
                &context_ptr->p_best_sad16x16[idx],
                &context_ptr->p_best_mv16x16[idx],
                &context_ptr->psub_pel_direction16x16[idx]);
        }
    }
    if ( enable_half_pel8x8   )
    {
        // 8x8   [64 partitions]
        if (!disable8x8_cu_in_me_flag){
            for (pu_index = 0; pu_index < 64; ++pu_index) {

                idx = eb_vp9_tab8x8[pu_index];  //TODO bitwise this

                pu_shift_x_index = (pu_index & 0x07) << 3;
                pu_shift_y_index = (pu_index >> 3) << 3;

                pu_sb_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->sb_src_stride;

                posb_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->interpolated_stride;
                posh_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->interpolated_stride;
                posj_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->interpolated_stride;

                pu_half_pel_refinement(
                    sequence_control_set_ptr,
                    context_ptr,
                    &(ref_buffer[pu_shift_y_index * ref_stride + pu_shift_x_index]),
                    ref_stride,
                    &context_ptr->p_best_ssd8x8[idx],
                    pu_sb_buffer_index,
                    &(posb_buffer[posb_buffer_index]),
                    &(posh_buffer[posh_buffer_index]),
                    &(posj_buffer[posj_buffer_index]),
                    8,
                    8,
                    x_search_area_origin,
                    y_search_area_origin,
                    &context_ptr->p_best_sad8x8[idx],
                    &context_ptr->p_best_mv8x8[idx],
                    &context_ptr->psub_pel_direction8x8[idx]);

            }
        }
    }

    return;
}

/*******************************************
* eb_vp9_combined_averaging_ssd
*
*******************************************/
uint32_t eb_vp9_combined_averaging_ssd(
    uint8_t  *src,
    uint32_t  src_stride,
    uint8_t  *ref1,
    uint32_t  ref1_stride,
    uint8_t  *ref2,
    uint32_t  ref2_stride,
    uint32_t  height,
    uint32_t  width)
{
    uint32_t x, y;
    uint32_t ssd = 0;
    uint8_t avgpel;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            avgpel = (ref1[x] + ref2[x] + 1) >> 1;
            ssd += SQR((int64_t)(src[x]) - (avgpel));
        }
        src += src_stride;
        ref1 += ref1_stride;
        ref2 += ref2_stride;
    }

    return ssd;
}

/*******************************************
* pu_quarter_pel_refinement_on_the_fly
*   performs Quarter Pel refinement for each PU
*******************************************/
static void pu_quarter_pel_refinement_on_the_fly(
    MeContext  *context_ptr,                        // [IN] ME context Ptr, used to get LCU Ptr
    uint32_t   *p_best_ssd,
    uint32_t    pu_sb_buffer_index,                 // [IN] PU origin, used to point to source samples
    uint8_t   **buf1,                               // [IN]
    uint32_t   *buf1_stride,
    uint8_t   **buf2,                               // [IN]
    uint32_t   *buf2_stride,
    uint32_t    pu_width,                           // [IN]  PU width
    uint32_t    pu_height,                          // [IN]  PU height
    int16_t     x_search_area_origin,               // [IN] search area origin in the horizontal direction, used to point to reference samples
    int16_t     y_search_area_origin,               // [IN] search area origin in the vertical direction, used to point to reference samples
    uint32_t   *p_best_sad,
    uint32_t   *p_best_mv,
    uint8_t     sub_pel_direction)
{

    int16_t x_mv = _MVXT(*p_best_mv);
    int16_t y_mv = _MVYT(*p_best_mv);

    int16_t x_search_index = ((x_mv + 2) >> 2) - x_search_area_origin;
    int16_t y_search_index = ((y_mv + 2) >> 2) - y_search_area_origin;

    uint64_t dist;

    EB_BOOL valid_tl, valid_t, valid_tr, valid_r, valid_br, valid_b, valid_bl, valid_l;

    int16_t x_mv_quarter[8];
    int16_t y_mv_quarter[8];
    int32_t search_region_index1 = 0;
    int32_t search_region_index2 = 0;

    if ((y_mv & 2) + ((x_mv & 2) >> 1)) {

        valid_tl = (EB_BOOL)(sub_pel_direction == RIGHT_POSITION || sub_pel_direction == BOTTOM_RIGHT_POSITION || sub_pel_direction == BOTTOM_POSITION);
        valid_t = (EB_BOOL)(sub_pel_direction == BOTTOM_RIGHT_POSITION || sub_pel_direction == BOTTOM_POSITION || sub_pel_direction == BOTTOM_LEFT_POSITION);
        valid_tr = (EB_BOOL)(sub_pel_direction == BOTTOM_POSITION || sub_pel_direction == BOTTOM_LEFT_POSITION || sub_pel_direction == LEFT_POSITION);
        valid_r = (EB_BOOL)(sub_pel_direction == BOTTOM_LEFT_POSITION || sub_pel_direction == LEFT_POSITION || sub_pel_direction == TOP_LEFT_POSITION);
        valid_br = (EB_BOOL)(sub_pel_direction == LEFT_POSITION || sub_pel_direction == TOP_LEFT_POSITION || sub_pel_direction == TOP_POSITION);
        valid_b = (EB_BOOL)(sub_pel_direction == TOP_LEFT_POSITION || sub_pel_direction == TOP_POSITION || sub_pel_direction == TOP_RIGHT_POSITION);
        valid_bl = (EB_BOOL)(sub_pel_direction == TOP_POSITION || sub_pel_direction == TOP_RIGHT_POSITION || sub_pel_direction == RIGHT_POSITION);
        valid_l = (EB_BOOL)(sub_pel_direction == TOP_RIGHT_POSITION || sub_pel_direction == RIGHT_POSITION || sub_pel_direction == BOTTOM_RIGHT_POSITION);

    }
    else {

        valid_tl = (EB_BOOL)(sub_pel_direction == LEFT_POSITION || sub_pel_direction == TOP_LEFT_POSITION || sub_pel_direction == TOP_POSITION);
        valid_t = (EB_BOOL)(sub_pel_direction == TOP_LEFT_POSITION || sub_pel_direction == TOP_POSITION || sub_pel_direction == TOP_RIGHT_POSITION);
        valid_tr = (EB_BOOL)(sub_pel_direction == TOP_POSITION || sub_pel_direction == TOP_RIGHT_POSITION || sub_pel_direction == RIGHT_POSITION);
        valid_r = (EB_BOOL)(sub_pel_direction == TOP_RIGHT_POSITION || sub_pel_direction == RIGHT_POSITION || sub_pel_direction == BOTTOM_RIGHT_POSITION);
        valid_br = (EB_BOOL)(sub_pel_direction == RIGHT_POSITION || sub_pel_direction == BOTTOM_RIGHT_POSITION || sub_pel_direction == BOTTOM_POSITION);
        valid_b = (EB_BOOL)(sub_pel_direction == BOTTOM_RIGHT_POSITION || sub_pel_direction == BOTTOM_POSITION || sub_pel_direction == BOTTOM_LEFT_POSITION);
        valid_bl = (EB_BOOL)(sub_pel_direction == BOTTOM_POSITION || sub_pel_direction == BOTTOM_LEFT_POSITION || sub_pel_direction == LEFT_POSITION);
        valid_l = (EB_BOOL)(sub_pel_direction == BOTTOM_LEFT_POSITION || sub_pel_direction == LEFT_POSITION || sub_pel_direction == TOP_LEFT_POSITION);
    }

    x_mv_quarter[0] = x_mv - 1; // L  position
    x_mv_quarter[1] = x_mv + 1; // R  position
    x_mv_quarter[2] = x_mv;     // T  position
    x_mv_quarter[3] = x_mv;     // B  position
    x_mv_quarter[4] = x_mv - 1; // TL position
    x_mv_quarter[5] = x_mv + 1; // TR position
    x_mv_quarter[6] = x_mv + 1; // BR position
    x_mv_quarter[7] = x_mv - 1; // BL position

    y_mv_quarter[0] = y_mv;     // L  position
    y_mv_quarter[1] = y_mv;     // R  position
    y_mv_quarter[2] = y_mv - 1; // T  position
    y_mv_quarter[3] = y_mv + 1; // B  position
    y_mv_quarter[4] = y_mv - 1; // TL position
    y_mv_quarter[5] = y_mv - 1; // TR position
    y_mv_quarter[6] = y_mv + 1; // BR position
    y_mv_quarter[7] = y_mv + 1; // BL position

    {
        // L position
        if (valid_l) {

            search_region_index1 = (int32_t)x_search_index + (int32_t)buf1_stride[0] * (int32_t)y_search_index;
            search_region_index2 = (int32_t)x_search_index + (int32_t)buf2_stride[0] * (int32_t)y_search_index;

            dist = (context_ptr->fractional_search_method == SSD_SEARCH) ?
                eb_vp9_combined_averaging_ssd(&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[0] + search_region_index1, buf1_stride[0], buf2[0] + search_region_index2, buf2_stride[0], pu_height, pu_width) :
                (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                    (nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE << 1, buf1[0] + search_region_index1, buf1_stride[0] << 1, buf2[0] + search_region_index2, buf2_stride[0] << 1, pu_height >> 1, pu_width)) << 1 :
                    nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[0] + search_region_index1, buf1_stride[0], buf2[0] + search_region_index2, buf2_stride[0], pu_height, pu_width);

            if (context_ptr->fractional_search_method == SSD_SEARCH) {
                if (dist < *p_best_ssd) {
                    *p_best_sad = (uint32_t)nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[0] + search_region_index1, buf1_stride[0], buf2[0] + search_region_index2, buf2_stride[0], pu_height, pu_width);
                    *p_best_mv = ((uint16_t)y_mv_quarter[0] << 16) | ((uint16_t)x_mv_quarter[0]);
                    *p_best_ssd = (uint32_t)dist;
                }
            }
            else {
                if (dist < *p_best_sad) {
                    *p_best_sad = (uint32_t)dist;
                    *p_best_mv = ((uint16_t)y_mv_quarter[0] << 16) | ((uint16_t)x_mv_quarter[0]);
                }
            }
        }

        // R positions
        if (valid_r) {

            search_region_index1 = (int32_t)x_search_index + (int32_t)buf1_stride[1] * (int32_t)y_search_index;
            search_region_index2 = (int32_t)x_search_index + (int32_t)buf2_stride[1] * (int32_t)y_search_index;

            dist = (context_ptr->fractional_search_method == SSD_SEARCH) ?
                eb_vp9_combined_averaging_ssd(&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[1] + search_region_index1, buf1_stride[1], buf2[1] + search_region_index2, buf2_stride[1], pu_height, pu_width) :
                (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                    (nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE << 1, buf1[1] + search_region_index1, buf1_stride[1] << 1, buf2[1] + search_region_index2, buf2_stride[1] << 1, pu_height >> 1, pu_width)) << 1 :
                    nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[1] + search_region_index1, buf1_stride[1], buf2[1] + search_region_index2, buf2_stride[1], pu_height, pu_width);

            if (context_ptr->fractional_search_method == SSD_SEARCH) {
                if (dist < *p_best_ssd) {
                    *p_best_sad = (uint32_t)nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[1] + search_region_index1, buf1_stride[1], buf2[1] + search_region_index2, buf2_stride[1], pu_height, pu_width);
                    *p_best_mv = ((uint16_t)y_mv_quarter[1] << 16) | ((uint16_t)x_mv_quarter[1]);
                    *p_best_ssd = (uint32_t)dist;
                }
            }
            else {
                if (dist < *p_best_sad) {
                    *p_best_sad = (uint32_t)dist;
                    *p_best_mv = ((uint16_t)y_mv_quarter[1] << 16) | ((uint16_t)x_mv_quarter[1]);
                }
            }
        }

        // T position
        if (valid_t) {

            search_region_index1 = (int32_t)x_search_index + (int32_t)buf1_stride[2] * (int32_t)y_search_index;
            search_region_index2 = (int32_t)x_search_index + (int32_t)buf2_stride[2] * (int32_t)y_search_index;

            dist = (context_ptr->fractional_search_method == SSD_SEARCH) ?
                eb_vp9_combined_averaging_ssd(&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[2] + search_region_index1, buf1_stride[2], buf2[2] + search_region_index2, buf2_stride[2], pu_height, pu_width) :
                (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                    (nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE << 1, buf1[2] + search_region_index1, buf1_stride[2] << 1, buf2[2] + search_region_index2, buf2_stride[2] << 1, pu_height >> 1, pu_width)) << 1 :
                    nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[2] + search_region_index1, buf1_stride[2], buf2[2] + search_region_index2, buf2_stride[2], pu_height, pu_width);

            if (context_ptr->fractional_search_method == SSD_SEARCH) {
                if (dist < *p_best_ssd) {
                    *p_best_sad = (uint32_t)nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[2] + search_region_index1, buf1_stride[2], buf2[2] + search_region_index2, buf2_stride[2], pu_height, pu_width);
                    *p_best_mv = ((uint16_t)y_mv_quarter[2] << 16) | ((uint16_t)x_mv_quarter[2]);
                    *p_best_ssd = (uint32_t)dist;
                }
            }
            else {
                if (dist < *p_best_sad) {
                    *p_best_sad = (uint32_t)dist;
                    *p_best_mv = ((uint16_t)y_mv_quarter[2] << 16) | ((uint16_t)x_mv_quarter[2]);
                }
            }
        }

        // B position
        if (valid_b) {

            search_region_index1 = (int32_t)x_search_index + (int32_t)buf1_stride[3] * (int32_t)y_search_index;
            search_region_index2 = (int32_t)x_search_index + (int32_t)buf2_stride[3] * (int32_t)y_search_index;

            dist = (context_ptr->fractional_search_method == SSD_SEARCH) ?
                eb_vp9_combined_averaging_ssd(&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[3] + search_region_index1, buf1_stride[3], buf2[3] + search_region_index2, buf2_stride[3], pu_height, pu_width) :
                (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                    (nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE << 1, buf1[3] + search_region_index1, buf1_stride[3] << 1, buf2[3] + search_region_index2, buf2_stride[3] << 1, pu_height >> 1, pu_width)) << 1 :
                    nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[3] + search_region_index1, buf1_stride[3], buf2[3] + search_region_index2, buf2_stride[3], pu_height, pu_width);

            if (context_ptr->fractional_search_method == SSD_SEARCH) {
                if (dist < *p_best_ssd) {
                    *p_best_sad = (uint32_t)nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[3] + search_region_index1, buf1_stride[3], buf2[3] + search_region_index2, buf2_stride[3], pu_height, pu_width);
                    *p_best_mv  = ((uint16_t)y_mv_quarter[3] << 16) | ((uint16_t)x_mv_quarter[3]);
                    *p_best_ssd = (uint32_t)dist;
                }
            }
            else {
                if (dist < *p_best_sad) {
                    *p_best_sad = (uint32_t)dist;
                    *p_best_mv = ((uint16_t)y_mv_quarter[3] << 16) | ((uint16_t)x_mv_quarter[3]);
                }
            }
        }

        //TL position
        if (valid_tl) {

            search_region_index1 = (int32_t)x_search_index + (int32_t)buf1_stride[4] * (int32_t)y_search_index;
            search_region_index2 = (int32_t)x_search_index + (int32_t)buf2_stride[4] * (int32_t)y_search_index;

            dist = (context_ptr->fractional_search_method == SSD_SEARCH) ?
                eb_vp9_combined_averaging_ssd(&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[4] + search_region_index1, buf1_stride[4], buf2[4] + search_region_index2, buf2_stride[4], pu_height, pu_width) :
                (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                    (nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE << 1, buf1[4] + search_region_index1, buf1_stride[4] << 1, buf2[4] + search_region_index2, buf2_stride[4] << 1, pu_height >> 1, pu_width)) << 1 :
                    nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[4] + search_region_index1, buf1_stride[4], buf2[4] + search_region_index2, buf2_stride[4], pu_height, pu_width);

            if (context_ptr->fractional_search_method == SSD_SEARCH) {
                if (dist < *p_best_ssd) {
                    *p_best_sad = (uint32_t)nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[4] + search_region_index1, buf1_stride[4], buf2[4] + search_region_index2, buf2_stride[4], pu_height, pu_width);
                    *p_best_mv = ((uint16_t)y_mv_quarter[4] << 16) | ((uint16_t)x_mv_quarter[4]);
                    *p_best_ssd = (uint32_t)dist;
                }
            }
            else {
                if (dist < *p_best_sad) {
                    *p_best_sad = (uint32_t)dist;
                    *p_best_mv = ((uint16_t)y_mv_quarter[4] << 16) | ((uint16_t)x_mv_quarter[4]);
                }

            }
        }

        //TR position
        if (valid_tr) {

            search_region_index1 = (int32_t)x_search_index + (int32_t)buf1_stride[5] * (int32_t)y_search_index;
            search_region_index2 = (int32_t)x_search_index + (int32_t)buf2_stride[5] * (int32_t)y_search_index;

            dist = (context_ptr->fractional_search_method == SSD_SEARCH) ?
                eb_vp9_combined_averaging_ssd(&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[5] + search_region_index1, buf1_stride[5], buf2[5] + search_region_index2, buf2_stride[5], pu_height, pu_width) :\
                (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                    (nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE << 1, buf1[5] + search_region_index1, buf1_stride[5] << 1, buf2[5] + search_region_index2, buf2_stride[5] << 1, pu_height >> 1, pu_width)) << 1 :
                    nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[5] + search_region_index1, buf1_stride[5], buf2[5] + search_region_index2, buf2_stride[5], pu_height, pu_width);

            if (context_ptr->fractional_search_method == SSD_SEARCH) {
                if (dist < *p_best_ssd) {
                    *p_best_sad = (uint32_t)nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[5] + search_region_index1, buf1_stride[5], buf2[5] + search_region_index2, buf2_stride[5], pu_height, pu_width);
                    *p_best_mv = ((uint16_t)y_mv_quarter[5] << 16) | ((uint16_t)x_mv_quarter[5]);
                    *p_best_ssd = (uint32_t)dist;
                }
            }
            else {
                if (dist < *p_best_sad) {
                    *p_best_sad = (uint32_t)dist;
                    *p_best_mv = ((uint16_t)y_mv_quarter[5] << 16) | ((uint16_t)x_mv_quarter[5]);
                }
            }
        }

        //BR position
        if (valid_br) {

            search_region_index1 = (int32_t)x_search_index + (int32_t)buf1_stride[6] * (int32_t)y_search_index;
            search_region_index2 = (int32_t)x_search_index + (int32_t)buf2_stride[6] * (int32_t)y_search_index;

            dist = (context_ptr->fractional_search_method == SSD_SEARCH) ?
                eb_vp9_combined_averaging_ssd(&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[6] + search_region_index1, buf1_stride[6], buf2[6] + search_region_index2, buf2_stride[6], pu_height, pu_width) :
                (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                    (nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE << 1, buf1[6] + search_region_index1, buf1_stride[6] << 1, buf2[6] + search_region_index2, buf2_stride[6] << 1, pu_height >> 1, pu_width)) << 1 :
                    nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[6] + search_region_index1, buf1_stride[6], buf2[6] + search_region_index2, buf2_stride[6], pu_height, pu_width);

            if (context_ptr->fractional_search_method == SSD_SEARCH) {
                if (dist < *p_best_ssd) {
                    *p_best_sad = (uint32_t)nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[6] + search_region_index1, buf1_stride[6], buf2[6] + search_region_index2, buf2_stride[6], pu_height, pu_width);
                    *p_best_mv = ((uint16_t)y_mv_quarter[6] << 16) | ((uint16_t)x_mv_quarter[6]);
                    *p_best_ssd = (uint32_t)dist;
                }
            }
            else {
                if (dist < *p_best_sad) {
                    *p_best_sad = (uint32_t)dist;
                    *p_best_mv = ((uint16_t)y_mv_quarter[6] << 16) | ((uint16_t)x_mv_quarter[6]);
                }
            }
        }

        //BL position
        if (valid_bl) {

            search_region_index1 = (int32_t)x_search_index + (int32_t)buf1_stride[7] * (int32_t)y_search_index;
            search_region_index2 = (int32_t)x_search_index + (int32_t)buf2_stride[7] * (int32_t)y_search_index;

            dist = (context_ptr->fractional_search_method == SSD_SEARCH) ?
                eb_vp9_combined_averaging_ssd(&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[7] + search_region_index1, buf1_stride[7], buf2[7] + search_region_index2, buf2_stride[7], pu_height, pu_width) :
                (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
                    (nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE << 1, buf1[7] + search_region_index1, buf1_stride[7] << 1, buf2[7] + search_region_index2, buf2_stride[7] << 1, pu_height >> 1, pu_width)) << 1 :
                    nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[7] + search_region_index1, buf1_stride[7], buf2[7] + search_region_index2, buf2_stride[7], pu_height, pu_width);

            if (context_ptr->fractional_search_method == SSD_SEARCH) {
                if (dist < *p_best_ssd) {
                    *p_best_sad = (uint32_t)nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](&(context_ptr->sb_buffer[pu_sb_buffer_index]), MAX_SB_SIZE, buf1[7] + search_region_index1, buf1_stride[7], buf2[7] + search_region_index2, buf2_stride[7], pu_height, pu_width);
                    *p_best_mv = ((uint16_t)y_mv_quarter[7] << 16) | ((uint16_t)x_mv_quarter[7]);
                    *p_best_ssd = (uint32_t)dist;
                }
            }
            else {
                if (dist < *p_best_sad) {
                    *p_best_sad = (uint32_t)dist;
                    *p_best_mv = ((uint16_t)y_mv_quarter[7] << 16) | ((uint16_t)x_mv_quarter[7]);
                }
            }
        }
    }

    return;
}

/*******************************************
* set_quarter_pel_refinement_inputs_on_the_fly
*   determine the 2 half pel buffers to do
averaging for Quarter Pel Refinement
*******************************************/
static void set_quarter_pel_refinement_inputs_on_the_fly(
    uint8_t  *pos_full,   //[IN] points to A
    uint32_t  full_stride, //[IN]
    uint8_t  *pos_b,     //[IN] points to b
    uint8_t  *pos_h,     //[IN] points to h
    uint8_t  *pos_j,     //[IN] points to j
    uint32_t  stride,    //[IN]
    int16_t   x_mv,        //[IN]
    int16_t   y_mv,        //[IN]
    uint8_t **buf1,       //[OUT]
    uint32_t *buf1_stride, //[OUT]
    uint8_t  **buf2,       //[OUT]
    uint32_t  *buf2_stride  //[OUT]
)
{

    uint32_t  quarter_pel_refinement_method = (y_mv & 2) + ((x_mv & 2) >> 1);

    //for each one of the 8 postions, we need to determine the 2 half pel buffers to  do averaging

    //     A    a    b    c
    //     d    e    f    g
    //     h    i    j    k
    //     n    p    q    r

    switch (quarter_pel_refinement_method) {

    case EB_QUARTER_IN_FULL:

        /*c=b+A*/ buf1[0] = pos_b;                     buf1_stride[0] = stride;        buf2[0] = pos_full;             buf2_stride[0] = full_stride;
        /*a=A+b*/ buf1[1] = pos_full;                  buf1_stride[1] = full_stride;    buf2[1] = pos_b + 1;             buf2_stride[1] = stride;
        /*n=h+A*/ buf1[2] = pos_h;                      buf1_stride[2] = stride;        buf2[2] = pos_full;              buf2_stride[2] = full_stride;
        /*d=A+h*/ buf1[3] = pos_full;                   buf1_stride[3] = full_stride;    buf2[3] = pos_h + stride;        buf2_stride[3] = stride;
        /*r=b+h*/ buf1[4] = pos_b;                      buf1_stride[4] = stride;        buf2[4] = pos_h;                 buf2_stride[4] = stride;
        /*p=h+b*/ buf1[5] = pos_h;                      buf1_stride[5] = stride;        buf2[5] = pos_b + 1;             buf2_stride[5] = stride;
        /*e=h+b*/ buf1[6] = pos_h + stride;             buf1_stride[6] = stride;        buf2[6] = pos_b + 1;             buf2_stride[6] = stride;
        /*g=b+h*/ buf1[7] = pos_b;                      buf1_stride[7] = stride;        buf2[7] = pos_h + stride;        buf2_stride[7] = stride;

        break;

    case EB_QUARTER_IN_HALF_HORIZONTAL:

        /*a=A+b*/ buf1[0] = pos_full - 1;               buf1_stride[0] = full_stride;    buf2[0] = pos_b;                buf2_stride[0] = stride;
        /*c=b+A*/ buf1[1] = pos_b;                     buf1_stride[1] = stride;        buf2[1] = pos_full;             buf2_stride[1] = full_stride;
        /*q=j+b*/ buf1[2] = pos_j;                     buf1_stride[2] = stride;        buf2[2] = pos_b;                buf2_stride[2] = stride;
        /*f=b+j*/ buf1[3] = pos_b;                     buf1_stride[3] = stride;        buf2[3] = pos_j + stride;        buf2_stride[3] = stride;
        /*p=h+b*/ buf1[4] = pos_h - 1;                  buf1_stride[4] = stride;        buf2[4] = pos_b;                buf2_stride[4] = stride;
        /*r=b+h*/ buf1[5] = pos_b;                     buf1_stride[5] = stride;        buf2[5] = pos_h;                buf2_stride[5] = stride;
        /*g=b+h*/ buf1[6] = pos_b;                     buf1_stride[6] = stride;        buf2[6] = pos_h + stride;        buf2_stride[6] = stride;
        /*e=h+b*/ buf1[7] = pos_h - 1 + stride;         buf1_stride[7] = stride;        buf2[7] = pos_b;                buf2_stride[7] = stride;

        break;

    case EB_QUARTER_IN_HALF_VERTICAL:

        /*k=j+h*/buf1[0] = pos_j;                      buf1_stride[0] = stride;        buf2[0] = pos_h;                 buf2_stride[0] = stride;
        /*i=h+j*/buf1[1] = pos_h;                      buf1_stride[1] = stride;        buf2[1] = pos_j + 1;              buf2_stride[1] = stride;
        /*d=A+h*/buf1[2] = pos_full - full_stride;      buf1_stride[2] = full_stride;    buf2[2] = pos_h;                  buf2_stride[2] = stride;
        /*n=h+A*/buf1[3] = pos_h;                       buf1_stride[3] = stride;        buf2[3] = pos_full;               buf2_stride[3] = full_stride;
        /*g=b+h*/buf1[4] = pos_b - stride;              buf1_stride[4] = stride;        buf2[4] = pos_h;                  buf2_stride[4] = stride;
        /*e=h+b*/buf1[5] = pos_h;                      buf1_stride[5] = stride;        buf2[5] = pos_b + 1 - stride;     buf2_stride[5] = stride;
        /*p=h+b*/buf1[6] = pos_h;                      buf1_stride[6] = stride;        buf2[6] = pos_b + 1;              buf2_stride[6] = stride;
        /*r=b+h*/buf1[7] = pos_b;                      buf1_stride[7] = stride;        buf2[7] = pos_h;                 buf2_stride[7] = stride;

        break;

    case EB_QUARTER_IN_HALF_DIAGONAL:

        /*i=h+j*/buf1[0] = pos_h - 1;                   buf1_stride[0] = stride;        buf2[0] = pos_j;                  buf2_stride[0] = stride;
        /*k=j+h*/buf1[1] = pos_j;                       buf1_stride[1] = stride;        buf2[1] = pos_h;                  buf2_stride[1] = stride;
        /*f=b+j*/buf1[2] = pos_b - stride;              buf1_stride[2] = stride;        buf2[2] = pos_j;                  buf2_stride[2] = stride;
        /*q=j+b*/buf1[3] = pos_j;                       buf1_stride[3] = stride;        buf2[3] = pos_b;                  buf2_stride[3] = stride;
        /*e=h+b*/buf1[4] = pos_h - 1;                   buf1_stride[4] = stride;        buf2[4] = pos_b - stride;         buf2_stride[4] = stride;
        /*g=b+h*/buf1[5] = pos_b - stride;              buf1_stride[5] = stride;        buf2[5] = pos_h;                  buf2_stride[5] = stride;
        /*r=b+h*/buf1[6] = pos_b;                      buf1_stride[6] = stride;        buf2[6] = pos_h;                  buf2_stride[6] = stride;
        /*p=h+b*/buf1[7] = pos_h - 1;                   buf1_stride[7] = stride;        buf2[7] = pos_b;                  buf2_stride[7] = stride;

        break;

    default:
        break;

    }

    return;
}

/*******************************************
* quarter_pel_search_sb
*   performs Quarter Pel refinement for the 85 PUs
*******************************************/
static void quarter_pel_search_sb(
    MeContext *context_ptr,                     //[IN/OUT]  ME context Ptr, used to get/update ME results
    uint8_t   *pos_full,                       //[IN]
    uint32_t   full_stride,                    //[IN]
    uint8_t   *pos_b,                          //[IN]
    uint8_t   *pos_h,                          //[IN]
    uint8_t   *pos_j,                          //[IN]
    int16_t    x_search_area_origin,           //[IN] search area origin in the horizontal direction, used to point to reference samples
    int16_t    y_search_area_origin,           //[IN] search area origin in the vertical direction, used to point to reference samples
    EB_BOOL    disable8x8_cu_in_me_flag,
    EB_BOOL    enable_half_pel32x32,
    EB_BOOL    enable_half_pel16x16,
    EB_BOOL    enable_half_pel8x8,
    EB_BOOL    enable_quarter_pel)
{
    uint32_t  pu_index;

    uint32_t  pu_shift_x_index;
    uint32_t  pu_shift_y_index;

    uint32_t  pu_sb_buffer_index;

    //for each one of the 8 positions, we need to determine the 2 buffers to  do averaging
    uint8_t  *buf1[8];
    uint8_t  *buf2[8];

    uint32_t  buf1_stride[8];
    uint32_t  buf2_stride[8];

    int16_t  x_mv, y_mv;
    uint32_t  n_idx;

    if (context_ptr->fractional_search64x64) {
        x_mv = _MVXT(*context_ptr->p_best_mv64x64);
        y_mv = _MVYT(*context_ptr->p_best_mv64x64);

        set_quarter_pel_refinement_inputs_on_the_fly(
            pos_full,
            full_stride,
            pos_b,
            pos_h,
            pos_j,
            context_ptr->interpolated_stride,
            x_mv,
            y_mv,
            buf1, buf1_stride,
            buf2, buf2_stride);

        pu_quarter_pel_refinement_on_the_fly(
            context_ptr,
            context_ptr->p_best_ssd64x64,
            0,
            buf1, buf1_stride,
            buf2, buf2_stride,
            32, 32,
            x_search_area_origin,
            y_search_area_origin,
            context_ptr->p_best_sad64x64,
            context_ptr->p_best_mv64x64,
            context_ptr->psub_pel_direction64x64);
    }

    if (enable_quarter_pel && enable_half_pel32x32)
    {
        // 32x32 [4 partitions]
        for (pu_index = 0; pu_index < 4; ++pu_index) {

            x_mv = _MVXT(context_ptr->p_best_mv32x32[pu_index]);
            y_mv = _MVYT(context_ptr->p_best_mv32x32[pu_index]);

            set_quarter_pel_refinement_inputs_on_the_fly(
                pos_full,
                full_stride,
                pos_b,
                pos_h,
                pos_j,
                context_ptr->interpolated_stride,
                x_mv,
                y_mv,
                buf1, buf1_stride,
                buf2, buf2_stride);

            pu_shift_x_index = (pu_index & 0x01) << 5;
            pu_shift_y_index = (pu_index >> 1) << 5;

            pu_sb_buffer_index = pu_shift_x_index + pu_shift_y_index * MAX_SB_SIZE;

            buf1[0] = buf1[0] + pu_shift_x_index + pu_shift_y_index * buf1_stride[0];              buf2[0] = buf2[0] + pu_shift_x_index + pu_shift_y_index * buf2_stride[0];
            buf1[1] = buf1[1] + pu_shift_x_index + pu_shift_y_index * buf1_stride[1];              buf2[1] = buf2[1] + pu_shift_x_index + pu_shift_y_index * buf2_stride[1];
            buf1[2] = buf1[2] + pu_shift_x_index + pu_shift_y_index * buf1_stride[2];              buf2[2] = buf2[2] + pu_shift_x_index + pu_shift_y_index * buf2_stride[2];
            buf1[3] = buf1[3] + pu_shift_x_index + pu_shift_y_index * buf1_stride[3];              buf2[3] = buf2[3] + pu_shift_x_index + pu_shift_y_index * buf2_stride[3];
            buf1[4] = buf1[4] + pu_shift_x_index + pu_shift_y_index * buf1_stride[4];              buf2[4] = buf2[4] + pu_shift_x_index + pu_shift_y_index * buf2_stride[4];
            buf1[5] = buf1[5] + pu_shift_x_index + pu_shift_y_index * buf1_stride[5];              buf2[5] = buf2[5] + pu_shift_x_index + pu_shift_y_index * buf2_stride[5];
            buf1[6] = buf1[6] + pu_shift_x_index + pu_shift_y_index * buf1_stride[6];              buf2[6] = buf2[6] + pu_shift_x_index + pu_shift_y_index * buf2_stride[6];
            buf1[7] = buf1[7] + pu_shift_x_index + pu_shift_y_index * buf1_stride[7];              buf2[7] = buf2[7] + pu_shift_x_index + pu_shift_y_index * buf2_stride[7];

            pu_quarter_pel_refinement_on_the_fly(
                context_ptr,
                &context_ptr->p_best_ssd32x32[pu_index],
                pu_sb_buffer_index,
                buf1, buf1_stride,
                buf2, buf2_stride,
                32, 32,
                x_search_area_origin,
                y_search_area_origin,
                &context_ptr->p_best_sad32x32[pu_index],
                &context_ptr->p_best_mv32x32[pu_index],
                context_ptr->psub_pel_direction32x32[pu_index]);

        }
    }

    if (enable_quarter_pel && enable_half_pel16x16)
    {
        // 16x16 [16 partitions]
        for (pu_index = 0; pu_index < 16; ++pu_index) {

            n_idx = eb_vp9_tab32x32[pu_index];

            x_mv = _MVXT(context_ptr->p_best_mv16x16[n_idx]);
            y_mv = _MVYT(context_ptr->p_best_mv16x16[n_idx]);

            set_quarter_pel_refinement_inputs_on_the_fly(
                pos_full,
                full_stride,
                pos_b,
                pos_h,
                pos_j,
                context_ptr->interpolated_stride,
                x_mv,
                y_mv,
                buf1, buf1_stride,
                buf2, buf2_stride);

            pu_shift_x_index = (pu_index & 0x03) << 4;
            pu_shift_y_index = (pu_index >> 2) << 4;

            pu_sb_buffer_index = pu_shift_x_index + pu_shift_y_index * MAX_SB_SIZE;

            buf1[0] = buf1[0] + pu_shift_x_index + pu_shift_y_index * buf1_stride[0];              buf2[0] = buf2[0] + pu_shift_x_index + pu_shift_y_index * buf2_stride[0];
            buf1[1] = buf1[1] + pu_shift_x_index + pu_shift_y_index * buf1_stride[1];              buf2[1] = buf2[1] + pu_shift_x_index + pu_shift_y_index * buf2_stride[1];
            buf1[2] = buf1[2] + pu_shift_x_index + pu_shift_y_index * buf1_stride[2];              buf2[2] = buf2[2] + pu_shift_x_index + pu_shift_y_index * buf2_stride[2];
            buf1[3] = buf1[3] + pu_shift_x_index + pu_shift_y_index * buf1_stride[3];              buf2[3] = buf2[3] + pu_shift_x_index + pu_shift_y_index * buf2_stride[3];
            buf1[4] = buf1[4] + pu_shift_x_index + pu_shift_y_index * buf1_stride[4];              buf2[4] = buf2[4] + pu_shift_x_index + pu_shift_y_index * buf2_stride[4];
            buf1[5] = buf1[5] + pu_shift_x_index + pu_shift_y_index * buf1_stride[5];              buf2[5] = buf2[5] + pu_shift_x_index + pu_shift_y_index * buf2_stride[5];
            buf1[6] = buf1[6] + pu_shift_x_index + pu_shift_y_index * buf1_stride[6];              buf2[6] = buf2[6] + pu_shift_x_index + pu_shift_y_index * buf2_stride[6];
            buf1[7] = buf1[7] + pu_shift_x_index + pu_shift_y_index * buf1_stride[7];              buf2[7] = buf2[7] + pu_shift_x_index + pu_shift_y_index * buf2_stride[7];

            pu_quarter_pel_refinement_on_the_fly(
                context_ptr,
                &context_ptr->p_best_ssd16x16[n_idx],
                pu_sb_buffer_index,
                buf1, buf1_stride,
                buf2, buf2_stride,
                16, 16,
                x_search_area_origin,
                y_search_area_origin,
                &context_ptr->p_best_sad16x16[n_idx],
                &context_ptr->p_best_mv16x16[n_idx],
                context_ptr->psub_pel_direction16x16[n_idx]);
        }
    }

    if (enable_quarter_pel && enable_half_pel8x8)
    {
        // 8x8   [64 partitions]
        if (!disable8x8_cu_in_me_flag) {
            for (pu_index = 0; pu_index < 64; ++pu_index) {

                n_idx = eb_vp9_tab8x8[pu_index];

                x_mv = _MVXT(context_ptr->p_best_mv8x8[n_idx]);
                y_mv = _MVYT(context_ptr->p_best_mv8x8[n_idx]);

                set_quarter_pel_refinement_inputs_on_the_fly(
                    pos_full,
                    full_stride,
                    pos_b,
                    pos_h,
                    pos_j,
                    context_ptr->interpolated_stride,
                    x_mv,
                    y_mv,
                    buf1, buf1_stride,
                    buf2, buf2_stride);

                pu_shift_x_index = (pu_index & 0x07) << 3;
                pu_shift_y_index = (pu_index >> 3) << 3;

                pu_sb_buffer_index = pu_shift_x_index + pu_shift_y_index * MAX_SB_SIZE;

                buf1[0] = buf1[0] + pu_shift_x_index + pu_shift_y_index * buf1_stride[0];              buf2[0] = buf2[0] + pu_shift_x_index + pu_shift_y_index * buf2_stride[0];
                buf1[1] = buf1[1] + pu_shift_x_index + pu_shift_y_index * buf1_stride[1];              buf2[1] = buf2[1] + pu_shift_x_index + pu_shift_y_index * buf2_stride[1];
                buf1[2] = buf1[2] + pu_shift_x_index + pu_shift_y_index * buf1_stride[2];              buf2[2] = buf2[2] + pu_shift_x_index + pu_shift_y_index * buf2_stride[2];
                buf1[3] = buf1[3] + pu_shift_x_index + pu_shift_y_index * buf1_stride[3];              buf2[3] = buf2[3] + pu_shift_x_index + pu_shift_y_index * buf2_stride[3];
                buf1[4] = buf1[4] + pu_shift_x_index + pu_shift_y_index * buf1_stride[4];              buf2[4] = buf2[4] + pu_shift_x_index + pu_shift_y_index * buf2_stride[4];
                buf1[5] = buf1[5] + pu_shift_x_index + pu_shift_y_index * buf1_stride[5];              buf2[5] = buf2[5] + pu_shift_x_index + pu_shift_y_index * buf2_stride[5];
                buf1[6] = buf1[6] + pu_shift_x_index + pu_shift_y_index * buf1_stride[6];              buf2[6] = buf2[6] + pu_shift_x_index + pu_shift_y_index * buf2_stride[6];
                buf1[7] = buf1[7] + pu_shift_x_index + pu_shift_y_index * buf1_stride[7];              buf2[7] = buf2[7] + pu_shift_x_index + pu_shift_y_index * buf2_stride[7];

                pu_quarter_pel_refinement_on_the_fly(
                    context_ptr,
                    &context_ptr->p_best_ssd8x8[n_idx],
                    pu_sb_buffer_index,
                    buf1, buf1_stride,
                    buf2, buf2_stride,
                    8, 8,
                    x_search_area_origin,
                    y_search_area_origin,
                    &context_ptr->p_best_sad8x8[n_idx],
                    &context_ptr->p_best_mv8x8[n_idx],
                    context_ptr->psub_pel_direction8x8[n_idx]);
            }
        }
    }

    return;
}

void single_hme_quadrant_level0(
    MeContext           *context_ptr,                       // input/output parameter, ME context Ptr, used to get/update ME results
    int16_t              origin_x,                          // input parameter, LCU position in the horizontal direction- sixteenth resolution
    int16_t              origin_y,                          // input parameter, LCU position in the vertical direction- sixteenth resolution
    uint32_t             sb_width,                          // input parameter, LCU pwidth - sixteenth resolution
    uint32_t             sb_height,                         // input parameter, LCU height - sixteenth resolution
    int16_t              x_hme_search_center,               // input parameter, HME search center in the horizontal direction
    int16_t              y_hme_search_center,               // input parameter, HME search center in the vertical direction
    EbPictureBufferDesc *sixteenth_ref_pic_ptr,             // input parameter, sixteenth reference Picture Ptr
    uint64_t            *level0_best_sad,                   // output parameter, Level0 SAD at (search_region_number_in_width, search_region_number_in_height)
    int16_t             *x_level0_search_center,            // output parameter, Level0 x_mv at (search_region_number_in_width, search_region_number_in_height)
    int16_t             *y_level0_search_center,            // output parameter, Level0 y_mv at (search_region_number_in_width, search_region_number_in_height)
    uint32_t             search_area_multiplier_x,
    uint32_t             search_area_multiplier_y)
{

    int16_t x_top_left_search_region;
    int16_t y_top_left_search_region;
    uint32_t search_region_index;
    int16_t x_search_area_origin;
    int16_t y_search_area_origin;
    int16_t x_search_region_distance;
    int16_t y_search_region_distance;

    int16_t pad_width;
    int16_t pad_height;

    int16_t search_area_width  = (int16_t)(((context_ptr->hme_level0_total_search_area_width  * search_area_multiplier_x) / 100));
    int16_t search_area_height = (int16_t)(((context_ptr->hme_level0_total_search_area_height * search_area_multiplier_y) / 100));

    if(context_ptr->hme_search_type == HME_SPARSE)
          search_area_width  = ((search_area_width +4)>>3)<<3;  //round down/up the width to the nearest multiple of 8.

    x_search_region_distance = x_hme_search_center;
    y_search_region_distance = y_hme_search_center;
    pad_width = (int16_t)(sixteenth_ref_pic_ptr->origin_x) - 1;
    pad_height = (int16_t)(sixteenth_ref_pic_ptr->origin_y) - 1;

    x_search_area_origin = -(int16_t)(search_area_width  >> 1) + x_search_region_distance;
    y_search_area_origin = -(int16_t)(search_area_height >> 1) + y_search_region_distance;

    // Correct the left edge of the Search Area if it is not on the reference Picture
    x_search_area_origin = ((origin_x + x_search_area_origin) < -pad_width) ?
        -pad_width - origin_x :
        x_search_area_origin;

    search_area_width = ((origin_x + x_search_area_origin) < -pad_width) ?
        search_area_width - (-pad_width - (origin_x + x_search_area_origin)) :
        search_area_width;

    // Correct the right edge of the Search Area if its not on the reference Picture
    x_search_area_origin = ((origin_x + x_search_area_origin) > (int16_t)sixteenth_ref_pic_ptr->width - 1) ?
        x_search_area_origin - ((origin_x + x_search_area_origin) - ((int16_t)sixteenth_ref_pic_ptr->width - 1)) :
        x_search_area_origin;

    search_area_width = ((origin_x + x_search_area_origin + search_area_width) > (int16_t)sixteenth_ref_pic_ptr->width) ?
        MAX(1, search_area_width - ((origin_x + x_search_area_origin + search_area_width) - (int16_t)sixteenth_ref_pic_ptr->width)) :
        search_area_width;

    // Correct the top edge of the Search Area if it is not on the reference Picture
    y_search_area_origin = ((origin_y + y_search_area_origin) < -pad_height) ?
        -pad_height - origin_y :
        y_search_area_origin;

    search_area_height = ((origin_y + y_search_area_origin) < -pad_height) ?
        search_area_height - (-pad_height - (origin_y + y_search_area_origin)) :
        search_area_height;

    // Correct the bottom edge of the Search Area if its not on the reference Picture
    y_search_area_origin = ((origin_y + y_search_area_origin) > (int16_t)sixteenth_ref_pic_ptr->height - 1) ?
        y_search_area_origin - ((origin_y + y_search_area_origin) - ((int16_t)sixteenth_ref_pic_ptr->height - 1)) :
        y_search_area_origin;

    search_area_height = (origin_y + y_search_area_origin + search_area_height > (int16_t)sixteenth_ref_pic_ptr->height) ?
        MAX(1, search_area_height - ((origin_y + y_search_area_origin + search_area_height) - (int16_t)sixteenth_ref_pic_ptr->height)) :
        search_area_height;

    x_top_left_search_region = ((int16_t)sixteenth_ref_pic_ptr->origin_x + origin_x) + x_search_area_origin;
    y_top_left_search_region = ((int16_t)sixteenth_ref_pic_ptr->origin_y + origin_y) + y_search_area_origin;
    search_region_index = x_top_left_search_region + y_top_left_search_region * sixteenth_ref_pic_ptr->stride_y;

    {

        if ((search_area_width & 15) != 0)
        {
            search_area_width = (int16_t)(floor((double)((search_area_width >> 4) << 4)));
        }

        if (((search_area_width & 15) == 0) && ((eb_vp9_ASM_TYPES & AVX2_MASK) && 1))
        {
#ifndef DISABLE_AVX512
            eb_vp9_sad_loop_kernel_avx512_hme_l0_intrin(
                &context_ptr->sixteenth_sb_buffer[0],
                context_ptr->sixteenth_sb_buffer_stride,
                &sixteenth_ref_pic_ptr->buffer_y[search_region_index],
                sixteenth_ref_pic_ptr->stride_y * 2,
                sb_height >> 1, sb_width,
                // results
                level0_best_sad,
                x_level0_search_center,
                y_level0_search_center,
                // range
                sixteenth_ref_pic_ptr->stride_y,
                search_area_width,
                search_area_height
                );
#else
            eb_vp9_sad_loop_kernel_avx2_hme_l0_intrin(
                &context_ptr->sixteenth_sb_buffer[0],
                context_ptr->sixteenth_sb_buffer_stride,
                &sixteenth_ref_pic_ptr->buffer_y[search_region_index],
                sixteenth_ref_pic_ptr->stride_y * 2,
                sb_height >> 1, sb_width,
                // results
                level0_best_sad,
                x_level0_search_center,
                y_level0_search_center,
                // range
                sixteenth_ref_pic_ptr->stride_y,
                search_area_width,
                search_area_height
            );
#endif

        }
        else {
            {
                // Only width equals 16 (LCU equals 64) is updated
                // other width sizes work with the old code as the one in"SadLoopKernel_SSE4_1_INTRIN"

                eb_vp9_sad_loop_kernel(
                    &context_ptr->sixteenth_sb_buffer[0],
                    context_ptr->sixteenth_sb_buffer_stride,
                    &sixteenth_ref_pic_ptr->buffer_y[search_region_index],
                    sixteenth_ref_pic_ptr->stride_y * 2,
                    sb_height >> 1, sb_width,
                    /* results */
                    level0_best_sad,
                    x_level0_search_center,
                    y_level0_search_center,
                    /* range */
                    sixteenth_ref_pic_ptr->stride_y,
                    search_area_width,
                    search_area_height
                    );
            }
        }
    }

    *level0_best_sad *= 2; // Multiply by 2 because considered only ever other line
    *x_level0_search_center += x_search_area_origin;
    *x_level0_search_center *= 4; // Multiply by 4 because operating on 1/4 resolution
    *y_level0_search_center += y_search_area_origin;
    *y_level0_search_center *= 4; // Multiply by 4 because operating on 1/4 resolution

    return;
}

void hme_level0(
    MeContext           *context_ptr,                          // input/output parameter, ME context Ptr, used to get/update ME results
    int16_t              origin_x,                             // input parameter, LCU position in the horizontal direction- sixteenth resolution
    int16_t              origin_y,                             // input parameter, LCU position in the vertical direction- sixteenth resolution
    uint32_t             sb_width,                             // input parameter, LCU pwidth - sixteenth resolution
    uint32_t             sb_height,                            // input parameter, LCU height - sixteenth resolution
    int16_t              x_hme_search_center,                  // input parameter, HME search center in the horizontal direction
    int16_t              y_hme_search_center,                  // input parameter, HME search center in the vertical direction
    EbPictureBufferDesc *sixteenth_ref_pic_ptr,                // input parameter, sixteenth reference Picture Ptr
    uint32_t             search_region_number_in_width,        // input parameter, search region number in the horizontal direction
    uint32_t             search_region_number_in_height,       // input parameter, search region number in the vertical direction
    uint64_t             *level0_best_sad,                     // output parameter, Level0 SAD at (search_region_number_in_width, search_region_number_in_height)
    int16_t              *x_level0_search_center,              // output parameter, Level0 x_mv at (search_region_number_in_width, search_region_number_in_height)
    int16_t              *y_level0_search_center,              // output parameter, Level0 y_mv at (search_region_number_in_width, search_region_number_in_height)
    uint32_t              search_area_multiplier_x,
    uint32_t              search_area_multiplier_y)
{

    int16_t x_top_left_search_region;
    int16_t y_top_left_search_region;
    uint32_t search_region_index;
    int16_t x_search_area_origin;
    int16_t y_search_area_origin;
    int16_t x_search_region_distance;
    int16_t y_search_region_distance;

    int16_t pad_width;
    int16_t pad_height;

    // Adjust SR size based on the searchAreaShift
    int16_t search_area_width = (int16_t)(((context_ptr->hme_level0_search_area_in_width_array[search_region_number_in_width] * search_area_multiplier_x) / 100));
    int16_t search_area_height = (int16_t)(((context_ptr->hme_level0_search_area_in_height_array[search_region_number_in_height] * search_area_multiplier_y) / 100));

    x_search_region_distance = x_hme_search_center;
    y_search_region_distance = y_hme_search_center;
    pad_width = (int16_t)(sixteenth_ref_pic_ptr->origin_x) - 1;
    pad_height = (int16_t)(sixteenth_ref_pic_ptr->origin_y) - 1;

    while (search_region_number_in_width) {
        search_region_number_in_width--;
        x_search_region_distance += (int16_t)(((context_ptr->hme_level0_search_area_in_width_array[search_region_number_in_width] * search_area_multiplier_x) / 100));
    }

    while (search_region_number_in_height) {
        search_region_number_in_height--;
        y_search_region_distance += (int16_t)(((context_ptr->hme_level0_search_area_in_height_array[search_region_number_in_height] * search_area_multiplier_y) / 100));
    }

    x_search_area_origin = -(int16_t)((((context_ptr->hme_level0_total_search_area_width * search_area_multiplier_x) / 100) ) >> 1) + x_search_region_distance;
    y_search_area_origin = -(int16_t)((((context_ptr->hme_level0_total_search_area_height * search_area_multiplier_y) / 100)) >> 1) + y_search_region_distance;

    // Correct the left edge of the Search Area if it is not on the reference Picture
    x_search_area_origin = ((origin_x + x_search_area_origin) < -pad_width) ?
        -pad_width - origin_x :
        x_search_area_origin;

    search_area_width = ((origin_x + x_search_area_origin) < -pad_width) ?
        search_area_width - (-pad_width - (origin_x + x_search_area_origin)) :
        search_area_width;

    // Correct the right edge of the Search Area if its not on the reference Picture
    x_search_area_origin = ((origin_x + x_search_area_origin) > (int16_t)sixteenth_ref_pic_ptr->width - 1) ?
        x_search_area_origin - ((origin_x + x_search_area_origin) - ((int16_t)sixteenth_ref_pic_ptr->width - 1)) :
        x_search_area_origin;

    search_area_width = ((origin_x + x_search_area_origin + search_area_width) > (int16_t)sixteenth_ref_pic_ptr->width) ?
        MAX(1, search_area_width - ((origin_x + x_search_area_origin + search_area_width) - (int16_t)sixteenth_ref_pic_ptr->width)) :
        search_area_width;

    // Correct the top edge of the Search Area if it is not on the reference Picture
    y_search_area_origin = ((origin_y + y_search_area_origin) < -pad_height) ?
        -pad_height - origin_y :
        y_search_area_origin;

    search_area_height = ((origin_y + y_search_area_origin) < -pad_height) ?
        search_area_height - (-pad_height - (origin_y + y_search_area_origin)) :
        search_area_height;

    // Correct the bottom edge of the Search Area if its not on the reference Picture
    y_search_area_origin = ((origin_y + y_search_area_origin) > (int16_t)sixteenth_ref_pic_ptr->height - 1) ?
        y_search_area_origin - ((origin_y + y_search_area_origin) - ((int16_t)sixteenth_ref_pic_ptr->height - 1)) :
        y_search_area_origin;

    search_area_height = (origin_y + y_search_area_origin + search_area_height > (int16_t)sixteenth_ref_pic_ptr->height) ?
        MAX(1, search_area_height - ((origin_y + y_search_area_origin + search_area_height) - (int16_t)sixteenth_ref_pic_ptr->height)) :
        search_area_height;

    x_top_left_search_region = ((int16_t)sixteenth_ref_pic_ptr->origin_x + origin_x) + x_search_area_origin;
    y_top_left_search_region = ((int16_t)sixteenth_ref_pic_ptr->origin_y + origin_y) + y_search_area_origin;
    search_region_index = x_top_left_search_region + y_top_left_search_region * sixteenth_ref_pic_ptr->stride_y;

    if (((sb_width  & 7) == 0) || (sb_width == 4))
    {
        if (((search_area_width & 15) == 0) && ((eb_vp9_ASM_TYPES & AVX2_MASK) && 1))
        {
#ifndef DISABLE_AVX512
            eb_vp9_sad_loop_kernel_avx512_hme_l0_intrin(
                &context_ptr->sixteenth_sb_buffer[0],
                context_ptr->sixteenth_sb_buffer_stride,
                &sixteenth_ref_pic_ptr->buffer_y[search_region_index],
                sixteenth_ref_pic_ptr->stride_y * 2,
                sb_height >> 1, sb_width,
                // results
                level0_best_sad,
                x_level0_search_center,
                y_level0_search_center,
                // range
                sixteenth_ref_pic_ptr->stride_y,
                search_area_width,
                search_area_height
                );
#else
            eb_vp9_sad_loop_kernel_avx2_hme_l0_intrin(
                &context_ptr->sixteenth_sb_buffer[0],
                context_ptr->sixteenth_sb_buffer_stride,
                &sixteenth_ref_pic_ptr->buffer_y[search_region_index],
                sixteenth_ref_pic_ptr->stride_y * 2,
                sb_height >> 1, sb_width,
                // results
                level0_best_sad,
                x_level0_search_center,
                y_level0_search_center,
                // range
                sixteenth_ref_pic_ptr->stride_y,
                search_area_width,
                search_area_height
            );
#endif
        }
        else
            {
                // Put the first search location into level0 results
                nx_m_sad_loop_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
                    &context_ptr->sixteenth_sb_buffer[0],
                    context_ptr->sixteenth_sb_buffer_stride,
                    &sixteenth_ref_pic_ptr->buffer_y[search_region_index],
                    sixteenth_ref_pic_ptr->stride_y * 2,
                    sb_height >> 1, sb_width,
                    /* results */
                    level0_best_sad,
                    x_level0_search_center,
                    y_level0_search_center,
                    /* range */
                    sixteenth_ref_pic_ptr->stride_y,
                    search_area_width,
                    search_area_height
                    );
        }
    }
    else
    {
        eb_vp9_sad_loop_kernel(
            &context_ptr->sixteenth_sb_buffer[0],
            context_ptr->sixteenth_sb_buffer_stride,
            &sixteenth_ref_pic_ptr->buffer_y[search_region_index],
            sixteenth_ref_pic_ptr->stride_y * 2,
            sb_height >> 1, sb_width,
            /* results */
            level0_best_sad,
            x_level0_search_center,
            y_level0_search_center,
            /* range */
            sixteenth_ref_pic_ptr->stride_y,
            search_area_width,
            search_area_height
            );
    }

    *level0_best_sad *= 2; // Multiply by 2 because considered only ever other line
    *x_level0_search_center += x_search_area_origin;
    *x_level0_search_center *= 4; // Multiply by 4 because operating on 1/4 resolution
    *y_level0_search_center += y_search_area_origin;
    *y_level0_search_center *= 4; // Multiply by 4 because operating on 1/4 resolution

    return;
}

void hme_level1(
    MeContext           *context_ptr,                        // input/output parameter, ME context Ptr, used to get/update ME results
    int16_t              origin_x,                           // input parameter, LCU position in the horizontal direction - quarter resolution
    int16_t              origin_y,                           // input parameter, LCU position in the vertical direction - quarter resolution
    uint32_t             sb_width,                           // input parameter, LCU pwidth - quarter resolution
    uint32_t             sb_height,                          // input parameter, LCU height - quarter resolution
    EbPictureBufferDesc *quarter_ref_pic_ptr,                // input parameter, quarter reference Picture Ptr
    int16_t              hme_level1_search_area_in_width,    // input parameter, hme level 1 search area in width
    int16_t              hme_level1_search_area_in_height,   // input parameter, hme level 1 search area in height
    int16_t              x_level0_search_center,             // input parameter, best Level0 x_mv at (search_region_number_in_width, search_region_number_in_height)
    int16_t              y_level0_search_center,             // input parameter, best Level0 y_mv at (search_region_number_in_width, search_region_number_in_height)
    uint64_t            *level1_best_sad,                    // output parameter, Level1 SAD at (search_region_number_in_width, search_region_number_in_height)
    int16_t             *x_level1_search_center,             // output parameter, Level1 x_mv at (search_region_number_in_width, search_region_number_in_height)
    int16_t             *y_level1_search_center)             // output parameter, Level1 y_mv at (search_region_number_in_width, search_region_number_in_height)
{

    int16_t x_top_left_search_region;
    int16_t y_top_left_search_region;
    uint32_t search_region_index;
    // round the search region width to nearest multiple of 8 if it is less than 8 or non multiple of 8
    // SAD calculation performance is the same for searchregion width from 1 to 8
    int16_t search_area_width = (hme_level1_search_area_in_width < 8) ? 8 : (hme_level1_search_area_in_width & 7) ? hme_level1_search_area_in_width + (hme_level1_search_area_in_width - ((hme_level1_search_area_in_width >> 3) << 3)) : hme_level1_search_area_in_width;
    int16_t search_area_height = hme_level1_search_area_in_height;

    int16_t x_search_area_origin;
    int16_t y_search_area_origin;

    int16_t pad_width = (int16_t)(quarter_ref_pic_ptr->origin_x) - 1;
    int16_t pad_height = (int16_t)(quarter_ref_pic_ptr->origin_y) - 1;

    x_search_area_origin = -(search_area_width >> 1) + x_level0_search_center;
    y_search_area_origin = -(search_area_height >> 1) + y_level0_search_center;

    // Correct the left edge of the Search Area if it is not on the reference Picture
    x_search_area_origin = ((origin_x + x_search_area_origin) < -pad_width) ?
        -pad_width - origin_x :
        x_search_area_origin;

    search_area_width = ((origin_x + x_search_area_origin) < -pad_width) ?
        search_area_width - (-pad_width - (origin_x + x_search_area_origin)) :
        search_area_width;
    // Correct the right edge of the Search Area if its not on the reference Picture
    x_search_area_origin = ((origin_x + x_search_area_origin) > (int16_t)quarter_ref_pic_ptr->width - 1) ?
        x_search_area_origin - ((origin_x + x_search_area_origin) - ((int16_t)quarter_ref_pic_ptr->width - 1)) :
        x_search_area_origin;

    search_area_width = ((origin_x + x_search_area_origin + search_area_width) > (int16_t)quarter_ref_pic_ptr->width) ?
        MAX(1, search_area_width - ((origin_x + x_search_area_origin + search_area_width) - (int16_t)quarter_ref_pic_ptr->width)) :
        search_area_width;

    // Correct the top edge of the Search Area if it is not on the reference Picture
    y_search_area_origin = ((origin_y + y_search_area_origin) < -pad_height) ?
        -pad_height - origin_y :
        y_search_area_origin;

    search_area_height = ((origin_y + y_search_area_origin) < -pad_height) ?
        search_area_height - (-pad_height - (origin_y + y_search_area_origin)) :
        search_area_height;

    // Correct the bottom edge of the Search Area if its not on the reference Picture
    y_search_area_origin = ((origin_y + y_search_area_origin) > (int16_t)quarter_ref_pic_ptr->height - 1) ?
        y_search_area_origin - ((origin_y + y_search_area_origin) - ((int16_t)quarter_ref_pic_ptr->height - 1)) :
        y_search_area_origin;

    search_area_height = (origin_y + y_search_area_origin + search_area_height > (int16_t)quarter_ref_pic_ptr->height) ?
        MAX(1, search_area_height - ((origin_y + y_search_area_origin + search_area_height) - (int16_t)quarter_ref_pic_ptr->height)) :
        search_area_height;

    // Move to the top left of the search region
    x_top_left_search_region = ((int16_t)quarter_ref_pic_ptr->origin_x + origin_x) + x_search_area_origin;
    y_top_left_search_region = ((int16_t)quarter_ref_pic_ptr->origin_y + origin_y) + y_search_area_origin;
    search_region_index = x_top_left_search_region + y_top_left_search_region * quarter_ref_pic_ptr->stride_y;

    if (((sb_width & 7) == 0) || (sb_width == 4))
    {
        // Put the first search location into level0 results
        nx_m_sad_loop_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
            &context_ptr->quarter_sb_buffer[0],
            context_ptr->quarter_sb_buffer_stride * 2,
            &quarter_ref_pic_ptr->buffer_y[search_region_index],
            quarter_ref_pic_ptr->stride_y * 2,
            sb_height >> 1, sb_width,
            /* results */
            level1_best_sad,
            x_level1_search_center,
            y_level1_search_center,
            /* range */
            quarter_ref_pic_ptr->stride_y,
            search_area_width,
            search_area_height
            );
    }
    else
    {
        eb_vp9_sad_loop_kernel(
            &context_ptr->quarter_sb_buffer[0],
            context_ptr->quarter_sb_buffer_stride * 2,
            &quarter_ref_pic_ptr->buffer_y[search_region_index],
            quarter_ref_pic_ptr->stride_y * 2,
            sb_height >> 1, sb_width,
            /* results */
            level1_best_sad,
            x_level1_search_center,
            y_level1_search_center,
            /* range */
            quarter_ref_pic_ptr->stride_y,
            search_area_width,
            search_area_height
            );
    }

    *level1_best_sad *= 2; // Multiply by 2 because considered only ever other line
    *x_level1_search_center += x_search_area_origin;
    *x_level1_search_center *= 2; // Multiply by 2 because operating on 1/2 resolution
    *y_level1_search_center += y_search_area_origin;
    *y_level1_search_center *= 2; // Multiply by 2 because operating on 1/2 resolution

    return;
}

void hme_level2(
    MeContext           *context_ptr,                        // input/output parameter, ME context Ptr, used to get/update ME results
    int16_t              origin_x,                           // input parameter, LCU position in the horizontal direction
    int16_t              origin_y,                           // input parameter, LCU position in the vertical direction
    uint32_t             sb_width,                           // input parameter, LCU pwidth - full resolution
    uint32_t             sb_height,                          // input parameter, LCU height - full resolution
    EbPictureBufferDesc *ref_pic_ptr,                        // input parameter, reference Picture Ptr
    uint32_t             search_region_number_in_width,      // input parameter, search region number in the horizontal direction
    uint32_t             search_region_number_in_height,     // input parameter, search region number in the vertical direction
    int16_t              x_level1_search_center,             // input parameter, best Level1 x_mv at(search_region_number_in_width, search_region_number_in_height)
    int16_t              y_level1_search_center,             // input parameter, best Level1 y_mv at(search_region_number_in_width, search_region_number_in_height)
    uint64_t            *level2_best_sad,                    // output parameter, Level2 SAD at (search_region_number_in_width, search_region_number_in_height)
    int16_t             *x_level2_search_center,             // output parameter, Level2 x_mv at (search_region_number_in_width, search_region_number_in_height)
    int16_t             *y_level2_search_center)             // output parameter, Level2 y_mv at (search_region_number_in_width, search_region_number_in_height)
{

    int16_t x_top_left_search_region;
    int16_t y_top_left_search_region;
    uint32_t search_region_index;

    // round the search region width to nearest multiple of 8 if it is less than 8 or non multiple of 8
    // SAD calculation performance is the same for searchregion width from 1 to 8
    int16_t hme_level2_search_area_in_width = (int16_t)context_ptr->hme_level2_search_area_in_width_array[search_region_number_in_width];
    int16_t search_area_width = (hme_level2_search_area_in_width < 8) ? 8 : (hme_level2_search_area_in_width & 7) ? hme_level2_search_area_in_width + (hme_level2_search_area_in_width - ((hme_level2_search_area_in_width >> 3) << 3)) : hme_level2_search_area_in_width;
    int16_t search_area_height = (int16_t)context_ptr->hme_level2_search_area_in_height_array[search_region_number_in_height];

    int16_t x_search_area_origin;
    int16_t y_search_area_origin;

    int16_t pad_width = (int16_t)MAX_SB_SIZE_MINUS_1;
    int16_t pad_height = (int16_t)MAX_SB_SIZE_MINUS_1;

    x_search_area_origin = -(search_area_width >> 1) + x_level1_search_center;
    y_search_area_origin = -(search_area_height >> 1) + y_level1_search_center;

    // Correct the left edge of the Search Area if it is not on the reference Picture
    x_search_area_origin = ((origin_x + x_search_area_origin) < -pad_width) ?
        -pad_width - origin_x :
        x_search_area_origin;

    search_area_width = ((origin_x + x_search_area_origin) < -pad_width) ?
        search_area_width - (-pad_width - (origin_x + x_search_area_origin)) :
        search_area_width;

    // Correct the right edge of the Search Area if its not on the reference Picture
    x_search_area_origin = ((origin_x + x_search_area_origin) > (int16_t)ref_pic_ptr->width - 1) ?
        x_search_area_origin - ((origin_x + x_search_area_origin) - ((int16_t)ref_pic_ptr->width - 1)) :
        x_search_area_origin;

    search_area_width = ((origin_x + x_search_area_origin + search_area_width) > (int16_t)ref_pic_ptr->width) ?
        MAX(1, search_area_width - ((origin_x + x_search_area_origin + search_area_width) - (int16_t)ref_pic_ptr->width)) :
        search_area_width;

    // Correct the top edge of the Search Area if it is not on the reference Picture
    y_search_area_origin = ((origin_y + y_search_area_origin) < -pad_height) ?
        -pad_height - origin_y :
        y_search_area_origin;

    search_area_height = ((origin_y + y_search_area_origin) < -pad_height) ?
        search_area_height - (-pad_height - (origin_y + y_search_area_origin)) :
        search_area_height;

    // Correct the bottom edge of the Search Area if its not on the reference Picture
    y_search_area_origin = ((origin_y + y_search_area_origin) > (int16_t)ref_pic_ptr->height - 1) ?
        y_search_area_origin - ((origin_y + y_search_area_origin) - ((int16_t)ref_pic_ptr->height - 1)) :
        y_search_area_origin;

    search_area_height = (origin_y + y_search_area_origin + search_area_height > (int16_t)ref_pic_ptr->height) ?
        MAX(1, search_area_height - ((origin_y + y_search_area_origin + search_area_height) - (int16_t)ref_pic_ptr->height)) :
        search_area_height;

    // Move to the top left of the search region
    x_top_left_search_region = ((int16_t)ref_pic_ptr->origin_x + origin_x) + x_search_area_origin;
    y_top_left_search_region = ((int16_t)ref_pic_ptr->origin_y + origin_y) + y_search_area_origin;
    search_region_index = x_top_left_search_region + y_top_left_search_region * ref_pic_ptr->stride_y;
    if ((((sb_width & 7) == 0) && (sb_width != 40) && (sb_width != 56)))
    {
        // Put the first search location into level0 results
        nx_m_sad_loop_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
            context_ptr->sb_src_ptr,
            context_ptr->sb_src_stride * 2,
            &ref_pic_ptr->buffer_y[search_region_index],
            ref_pic_ptr->stride_y * 2,
            sb_height >> 1, sb_width,
            /* results */
            level2_best_sad,
            x_level2_search_center,
            y_level2_search_center,
            /* range */
            ref_pic_ptr->stride_y,
            search_area_width,
            search_area_height
            );
    }
    else
    {
        // Put the first search location into level0 results
        eb_vp9_sad_loop_kernel(
            context_ptr->sb_src_ptr,
            context_ptr->sb_src_stride * 2,
            &ref_pic_ptr->buffer_y[search_region_index],
            ref_pic_ptr->stride_y * 2,
            sb_height >> 1, sb_width,
            /* results */
            level2_best_sad,
            x_level2_search_center,
            y_level2_search_center,
            /* range */
            ref_pic_ptr->stride_y,
            search_area_width,
            search_area_height
            );

    }

    *level2_best_sad *= 2; // Multiply by 2 because considered only every other line
    *x_level2_search_center += x_search_area_origin;
    *y_level2_search_center += y_search_area_origin;

    return;
}

static void select_buffer(
    uint32_t pu_index,                         //[IN]
    uint8_t  frac_position,                    //[IN]
    uint32_t pu_width,                         //[IN] reference picture list index
    uint32_t pu_height,                        //[IN] reference picture index in the list
    uint8_t *pos_full,                         //[IN]
    uint8_t *pos_b,                            //[IN]
    uint8_t *pos_h,                            //[IN]
    uint8_t *pos_j,                            //[IN]
    uint32_t ref_half_stride,                  //[IN]
    uint32_t ref_buffer_full_stride,
    uint8_t **dst_ptr,                         //[OUT]
    uint32_t *dst_ptr_stride)                  //[OUT]
{

    (void)pu_width;
    (void)pu_height;

    uint32_t pu_shift_x_index = pu_search_index_map[pu_index][0];
    uint32_t pu_shift_y_index = pu_search_index_map[pu_index][1];
    uint32_t ref_stride = ref_half_stride;

    //for each one of the 8 positions, we need to determine the 2 buffers to  do averaging
    uint8_t  *buf1 = pos_full;

    switch (frac_position)
    {
    case 0: // integer
        buf1 = pos_full;
        ref_stride = ref_buffer_full_stride;
        break;
    case 2: // b
        buf1 = pos_b;
        break;
    case 8: // h
        buf1 = pos_h;
        break;
    case 10: // j
        buf1 = pos_j;
        break;
    default:
        break;
    }

    buf1 = buf1 + pu_shift_x_index + pu_shift_y_index * ref_stride;

    *dst_ptr = buf1;
    *dst_ptr_stride = ref_stride;

    return;
}

static void quarter_pel_compensation(
    uint32_t  pu_index,                         //[IN]
    uint8_t   frac_position,                    //[IN]
    uint32_t  pu_width,                         //[IN] reference picture list index
    uint32_t  pu_height,                        //[IN] reference picture index in the list
    uint8_t  *pos_full,                         //[IN]
    uint8_t  *pos_b,                            //[IN]
    uint8_t  *pos_h,                            //[IN]
    uint8_t  *pos_j,                            //[IN]
    uint32_t  ref_half_stride,                  //[IN]
    uint32_t  ref_buffer_full_stride,
    uint8_t  *dst,                              //[IN]
    uint32_t  dst_stride)                       //[IN]
{

    uint32_t pu_shift_x_index = pu_search_index_map[pu_index][0];
    uint32_t pu_shift_y_index = pu_search_index_map[pu_index][1];
    uint32_t ref_stride1 = ref_half_stride;
    uint32_t ref_stride2 = ref_half_stride;

    //for each one of the 8 positions, we need to determine the 2 buffers to  do averaging
    uint8_t  *buf1 = pos_full;
    uint8_t  *buf2 = pos_full;

    switch (frac_position)
    {
    case 1: // a
        buf1 = pos_full;
        buf2 = pos_b;
        ref_stride1 = ref_buffer_full_stride;
        break;

    case 3: // c
        buf1 = pos_b;
        buf2 = pos_full + 1;
        ref_stride2 = ref_buffer_full_stride;
        break;

    case 4: // d
        buf1 = pos_full;
        buf2 = pos_h;
        ref_stride1 = ref_buffer_full_stride;
        break;

    case 5: // e
        buf1 = pos_b;
        buf2 = pos_h;
        break;

    case 6: // f
        buf1 = pos_b;
        buf2 = pos_j;
        break;

    case 7: // g
        buf1 = pos_b;
        buf2 = pos_h + 1;
        break;

    case 9: // i
        buf1 = pos_h;
        buf2 = pos_j;
        break;

    case 11: // k
        buf1 = pos_j;
        buf2 = pos_h + 1;
        break;

    case 12: // L
        buf1 = pos_h;
        buf2 = pos_full + ref_buffer_full_stride;
        ref_stride2 = ref_buffer_full_stride;
        break;

    case 13: // m
        buf1 = pos_h;
        buf2 = pos_b + ref_half_stride;
        break;

    case 14: // n
        buf1 = pos_j;
        buf2 = pos_b + ref_half_stride;
        break;
    case 15: // 0
        buf1 = pos_h + 1;
        buf2 = pos_b + ref_half_stride;
        break;
    default:
        break;
    }

    buf1 = buf1 + pu_shift_x_index + pu_shift_y_index * ref_stride1;
    buf2 = buf2 + pu_shift_x_index + pu_shift_y_index * ref_stride2;

    picture_average_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](buf1, ref_stride1, buf2, ref_stride2, dst, dst_stride, pu_width, pu_height);

    return;
}

/*******************************************************************************
 * Requirement: pu_width      = 8, 16, 24, 32, 48 or 64
 * Requirement: pu_height % 2 = 0
 * Requirement: skip         = 0 or 1
 * Requirement (x86 only): temp_buf % 16 = 0
 * Requirement (x86 only): (dst->buffer_y  + dst_luma_index  ) % 16 = 0 when pu_width %16 = 0
 * Requirement (x86 only): (dst->buffer_cb + dst_chroma_index) % 16 = 0 when pu_width %32 = 0
 * Requirement (x86 only): (dst->buffer_cr + dst_chroma_index) % 16 = 0 when pu_width %32 = 0
 * Requirement (x86 only): dst->stride_y   % 16 = 0 when pu_width %16 = 0
 * Requirement (x86 only): dst->chromaStride % 16 = 0 when pu_width %32 = 0
 *******************************************************************************/
uint32_t bi_pred_averging(
    MeContext  *context_ptr,
    MePredUnit *me_candidate,
    uint32_t    pu_index,
    uint8_t    *source_pic,
    uint32_t    luma_stride,
    uint8_t     first_frac_pos,
    uint8_t     second_frac_pos,
    uint32_t    pu_width,
    uint32_t    pu_height,
    uint8_t     *first_ref_integer,
    uint8_t     *first_ref_pos_b,
    uint8_t     *first_ref_pos_h,
    uint8_t     *first_ref_pos_j,
    uint8_t     *second_ref_integer,
    uint8_t     *second_ref_pos_b,
    uint8_t     *second_ref_pos_h,
    uint8_t     *second_ref_pos_j,
    uint32_t    ref_buffer_stride,
    uint32_t    ref_buffer_full_list0_stride,
    uint32_t    ref_buffer_full_list1_stride,
    uint8_t     *first_ref_temp_dst,
    uint8_t     *second_ref_temp_dst)
{

    uint8_t *ptr_list0, *ptr_list1;
    uint32_t ptr_list0_stride, ptr_list1_stride;

    // Buffer Selection and quater-pel compensation on the fly
    if (sub_position_type[first_frac_pos] != 2){

        select_buffer(
            pu_index,
            first_frac_pos,
            pu_width,
            pu_height,
            first_ref_integer,
            first_ref_pos_b,
            first_ref_pos_h,
            first_ref_pos_j,
            ref_buffer_stride,
            ref_buffer_full_list0_stride,
            &ptr_list0,
            &ptr_list0_stride);

    }
    else{

        quarter_pel_compensation(
            pu_index,
            first_frac_pos,
            pu_width,
            pu_height,
            first_ref_integer,
            first_ref_pos_b,
            first_ref_pos_h,
            first_ref_pos_j,
            ref_buffer_stride,
            ref_buffer_full_list0_stride,
            first_ref_temp_dst,
            MAX_SB_SIZE);

        ptr_list0 = first_ref_temp_dst;
        ptr_list0_stride = MAX_SB_SIZE;
    }

    if (sub_position_type[second_frac_pos] != 2){

        select_buffer(
            pu_index,
            second_frac_pos,
            pu_width,
            pu_height,
            second_ref_integer,
            second_ref_pos_b,
            second_ref_pos_h,
            second_ref_pos_j,
            ref_buffer_stride,
            ref_buffer_full_list1_stride,
            &ptr_list1,
            &ptr_list1_stride);

    }
    else{
        //uni-prediction List1 luma
        //doing the luma interpolation
        quarter_pel_compensation(
            pu_index,
            second_frac_pos,
            pu_width,
            pu_height,
            second_ref_integer,
            second_ref_pos_b,
            second_ref_pos_h,
            second_ref_pos_j,
            ref_buffer_stride,
            ref_buffer_full_list1_stride,
            second_ref_temp_dst,
            MAX_SB_SIZE);

        ptr_list1 = second_ref_temp_dst;
        ptr_list1_stride = MAX_SB_SIZE;

    }

    // bi-pred luma
    me_candidate->distortion = (context_ptr->fractional_search_method == SUB_SAD_SEARCH) ?
        nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](source_pic,
            luma_stride << 1,
            ptr_list0,
            ptr_list0_stride << 1,
            ptr_list1,
            ptr_list1_stride << 1,
            pu_height >> 1,
            pu_width) << 1 :
        nx_m_sad_averaging_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][pu_width >> 3](source_pic,
            luma_stride,
            ptr_list0,
            ptr_list0_stride,
            ptr_list1,
            ptr_list1_stride,
            pu_height,
            pu_width);

    return me_candidate->distortion;
}

/*******************************************
 * BiPredictionComponsation
 *   performs componsation fro List 0 and
 *   List1 Candidates and then compute the
 *   average
 *******************************************/
EbErrorType  bi_prediction_compensation(
    MeContext  *context_ptr,
    uint32_t    pu_index,
    MePredUnit *me_candidate,
    uint32_t    first_list,
    uint32_t    first_ref_mv,
    uint32_t    second_list,
    uint32_t    second_ref_mv)
{
    EbErrorType                    return_error = EB_ErrorNone;

    int16_t  first_ref_pos_x;
    int16_t  first_ref_pos_y;
    int16_t  first_ref_integ_posx;
    int16_t  first_ref_integ_posy;
    uint8_t  first_ref_frac_posx;
    uint8_t  first_ref_frac_posy;
    uint8_t  first_ref_frac_pos;
    int32_t  xfirst_search_index;
    int32_t  yfirst_search_index;
    int32_t  first_search_region_index_pos_integ;
    int32_t  first_search_region_index_posb;
    int32_t  first_search_region_index_posh;
    int32_t  first_search_region_index_posj;

    int16_t  second_ref_pos_x;
    int16_t  second_ref_pos_y;
    int16_t  second_ref_integ_posx;
    int16_t  second_ref_integ_posy;
    uint8_t  second_ref_frac_posx;
    uint8_t  second_ref_frac_posy;
    uint8_t  second_ref_frac_pos;
    int32_t  xsecond_search_index;
    int32_t  ysecond_search_index;
    int32_t  second_search_region_index_pos_integ;
    int32_t  second_search_region_index_posb;
    int32_t  second_search_region_index_posh;
    int32_t  second_search_region_index_posj;

    uint32_t pu_shift_x_index = pu_search_index_map[pu_index][0];
    uint32_t pu_shift_y_index = pu_search_index_map[pu_index][1];

    const uint32_t pu_sb_buffer_index = pu_shift_x_index + pu_shift_y_index * context_ptr->sb_src_stride;

    me_candidate->prediction_direction = BI_PRED;

    // First reference
    // Set Candidate information

    first_ref_pos_x = _MVXT(first_ref_mv),
        first_ref_pos_y = _MVYT(first_ref_mv),
        me_candidate->Mv[0] = first_ref_mv;

    first_ref_integ_posx = (first_ref_pos_x >> 2);
    first_ref_integ_posy = (first_ref_pos_y >> 2);
    first_ref_frac_posx = (uint8_t)first_ref_pos_x & 0x03;
    first_ref_frac_posy = (uint8_t)first_ref_pos_y & 0x03;

    first_ref_frac_pos = (uint8_t)(first_ref_frac_posx + (first_ref_frac_posy << 2));

    xfirst_search_index = (int32_t)first_ref_integ_posx - context_ptr->x_search_area_origin[first_list][0];
    yfirst_search_index = (int32_t)first_ref_integ_posy - context_ptr->y_search_area_origin[first_list][0];

    first_search_region_index_pos_integ = (int32_t)(xfirst_search_index + (ME_FILTER_TAP >> 1)) + (int32_t)context_ptr->interpolated_full_stride[first_list][0] * (int32_t)(yfirst_search_index + (ME_FILTER_TAP >> 1));
    first_search_region_index_posb = (int32_t)(xfirst_search_index + (ME_FILTER_TAP >> 1) - 1) + (int32_t)context_ptr->interpolated_stride * (int32_t)(yfirst_search_index + (ME_FILTER_TAP >> 1));
    first_search_region_index_posh = (int32_t)(xfirst_search_index + (ME_FILTER_TAP >> 1) - 1) + (int32_t)context_ptr->interpolated_stride * (int32_t)(yfirst_search_index + (ME_FILTER_TAP >> 1) - 1);
    first_search_region_index_posj = (int32_t)(xfirst_search_index + (ME_FILTER_TAP >> 1) - 1) + (int32_t)context_ptr->interpolated_stride * (int32_t)(yfirst_search_index + (ME_FILTER_TAP >> 1) - 1);

    // Second reference

    // Set Candidate information
    second_ref_pos_x = _MVXT(second_ref_mv),
        second_ref_pos_y = _MVYT(second_ref_mv),
        me_candidate->Mv[1] = second_ref_mv;

    second_ref_integ_posx = (second_ref_pos_x >> 2);
    second_ref_integ_posy = (second_ref_pos_y >> 2);
    second_ref_frac_posx = (uint8_t)second_ref_pos_x & 0x03;
    second_ref_frac_posy = (uint8_t)second_ref_pos_y & 0x03;

    second_ref_frac_pos = (uint8_t)(second_ref_frac_posx + (second_ref_frac_posy << 2));

    xsecond_search_index = second_ref_integ_posx - context_ptr->x_search_area_origin[second_list][0];
    ysecond_search_index = second_ref_integ_posy - context_ptr->y_search_area_origin[second_list][0];

    second_search_region_index_pos_integ = (int32_t)(xsecond_search_index + (ME_FILTER_TAP >> 1)) + (int32_t)context_ptr->interpolated_full_stride[second_list][0] * (int32_t)(ysecond_search_index + (ME_FILTER_TAP >> 1));
    second_search_region_index_posb     = (int32_t)(xsecond_search_index + (ME_FILTER_TAP >> 1) - 1) + (int32_t)context_ptr->interpolated_stride * (int32_t)(ysecond_search_index + (ME_FILTER_TAP >> 1));
    second_search_region_index_posh     = (int32_t)(xsecond_search_index + (ME_FILTER_TAP >> 1) - 1) + (int32_t)context_ptr->interpolated_stride * (int32_t)(ysecond_search_index + (ME_FILTER_TAP >> 1) - 1);
    second_search_region_index_posj     = (int32_t)(xsecond_search_index + (ME_FILTER_TAP >> 1) - 1) + (int32_t)context_ptr->interpolated_stride * (int32_t)(ysecond_search_index + (ME_FILTER_TAP >> 1) - 1);

    uint32_t n_index =   pu_index >  20   ?   eb_vp9_tab8x8[pu_index-21]  + 21   :
        pu_index >   4   ?   eb_vp9_tab32x32[pu_index-5] + 5    :  pu_index;
    context_ptr->p_sb_bipred_sad[n_index] =

        bi_pred_averging(
            context_ptr,
            me_candidate,
            pu_index,
            &(context_ptr->sb_src_ptr[pu_sb_buffer_index]),
            context_ptr->sb_src_stride,
            first_ref_frac_pos,
            second_ref_frac_pos,
            partition_width[pu_index],
            partition_height[pu_index],
            &(context_ptr->integer_buffer_ptr[first_list][0][first_search_region_index_pos_integ]),
            &(context_ptr->posb_buffer[first_list][0][first_search_region_index_posb]),
            &(context_ptr->posh_buffer[first_list][0][first_search_region_index_posh]),
            &(context_ptr->posj_buffer[first_list][0][first_search_region_index_posj]),
            &(context_ptr->integer_buffer_ptr[second_list][0][second_search_region_index_pos_integ]),
            &(context_ptr->posb_buffer[second_list][0][second_search_region_index_posb]),
            &(context_ptr->posh_buffer[second_list][0][second_search_region_index_posh]),
            &(context_ptr->posj_buffer[second_list][0][second_search_region_index_posj]),
            context_ptr->interpolated_stride,
            context_ptr->interpolated_full_stride[first_list][0],
            context_ptr->interpolated_full_stride[second_list][0],
            &(context_ptr->one_d_intermediate_results_buf0[0]),
            &(context_ptr->one_d_intermediate_results_buf1[0]));

    return return_error;
}

/*******************************************
 * bi_prediction_search
 *   performs Bi-Prediction Search (LCU)
 *******************************************/
// This function enables all 16 Bipred candidates when MRP is ON
EbErrorType  bi_prediction_search(
    MeContext               *context_ptr,
    uint32_t                 pu_index,
    uint8_t                  candidate_index,
    uint32_t                 active_ref_pic_first_lis_num,
    uint32_t                 active_ref_pic_second_lis_num,
    uint8_t                 *total_me_candidate_index,
    PictureParentControlSet *picture_control_set_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    uint32_t first_list_ref_pictdx;
    uint32_t second_list_ref_pictdx;

    (void)picture_control_set_ptr;
    uint32_t n_index =   pu_index >  20   ?   eb_vp9_tab8x8[pu_index-21]  + 21   :
        pu_index >   4   ?   eb_vp9_tab32x32[pu_index-5] + 5    :  pu_index;
    for (first_list_ref_pictdx = 0; first_list_ref_pictdx < active_ref_pic_first_lis_num; first_list_ref_pictdx++) {
        for (second_list_ref_pictdx = 0; second_list_ref_pictdx < active_ref_pic_second_lis_num; second_list_ref_pictdx++) {

            {

                bi_prediction_compensation(
                    context_ptr,
                    pu_index,
                    &(context_ptr->me_candidate[candidate_index].pu[pu_index]),
                    REFERENCE_PIC_LIST_0,
                    context_ptr->p_sb_best_mv[REFERENCE_PIC_LIST_0][first_list_ref_pictdx][n_index],
                    REFERENCE_PIC_LIST_1,
                    context_ptr->p_sb_best_mv[REFERENCE_PIC_LIST_1][second_list_ref_pictdx][n_index]);

                candidate_index++;

            }
        }
    }

    *total_me_candidate_index = candidate_index;

    return return_error;
}

#define NSET_CAND(me_pu_result, num, dist, dir) \
     (me_pu_result)->distortion_direction[(num)].distortion = (dist); \
     (me_pu_result)->distortion_direction[(num)].direction = (dir)  ;

int8_t sort3_elements(uint32_t a, uint32_t b, uint32_t c){

    uint8_t sortCode = 0;
    if (a <= b && a <= c){
        if (b <= c) {
            sortCode =   a_b_c;
        }else{
            sortCode =   a_c_b;
        }
    }else if (b <= a &&  b <= c) {
        if (a <= c) {
            sortCode =   b_a_c;
        }else {
            sortCode =   b_c_a;
        }

    }else if (a <= b){
        sortCode =  c_a_b;
    } else{
        sortCode =  c_b_a;
    }

    return sortCode;
}

EbErrorType check_zero_zero_center(
    EbPictureBufferDesc *ref_pic_ptr,
    MeContext           *context_ptr,
    uint32_t             sb_origin_x,
    uint32_t             sb_origin_y,
    uint32_t             sb_width,
    uint32_t             sb_height,
    int16_t             *x_search_center,
    int16_t             *y_search_center)

{
    EbErrorType return_error = EB_ErrorNone;
#if 1 // Amir-Hsan: to fix rate estimation or to use SAD only
    uint32_t       search_region_index, zero_mv_sad, hme_mv_sad;
#else
    uint32_t       search_region_index, zero_mv_sad, hme_mv_sad, hme_mvd_rate;
#endif
    uint64_t       hme_mv_cost, zero_mv_cost, search_center_cost;
    int16_t       origin_x = (int16_t)sb_origin_x;
    int16_t       origin_y = (int16_t)sb_origin_y;
    uint32_t       subsample_sad = 1;
#if HME_ENHANCED_CENTER_SEARCH
    int16_t                  pad_width = (int16_t)MAX_SB_SIZE - 1;
    int16_t                  pad_height = (int16_t)MAX_SB_SIZE - 1;
#endif
    search_region_index = (int16_t)ref_pic_ptr->origin_x + origin_x +
        ((int16_t)ref_pic_ptr->origin_y + origin_y) * ref_pic_ptr->stride_y;

    zero_mv_sad = n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][sb_width >> 3](
        context_ptr->sb_src_ptr,
        context_ptr->sb_src_stride << subsample_sad,
        &(ref_pic_ptr->buffer_y[search_region_index]),
        ref_pic_ptr->stride_y << subsample_sad,
        sb_height >> subsample_sad,
        sb_width);

    zero_mv_sad = zero_mv_sad << subsample_sad;
#if HME_ENHANCED_CENTER_SEARCH
    // FIX
    // Correct the left edge of the Search Area if it is not on the reference Picture
    *x_search_center = ((origin_x + *x_search_center) < -pad_width) ?
        -pad_width - origin_x :
        *x_search_center;
    // Correct the right edge of the Search Area if its not on the reference Picture
    *x_search_center = ((origin_x + *x_search_center) > (int16_t)ref_pic_ptr->width - 1) ?
        *x_search_center - ((origin_x + *x_search_center) - ((int16_t)ref_pic_ptr->width - 1)) :
        *x_search_center;
    // Correct the top edge of the Search Area if it is not on the reference Picture
    *y_search_center = ((origin_y + *y_search_center) < -pad_height) ?
        -pad_height - origin_y :
        *y_search_center;
    // Correct the bottom edge of the Search Area if its not on the reference Picture
    *y_search_center = ((origin_y + *y_search_center) > (int16_t)ref_pic_ptr->height - 1) ?
        *y_search_center - ((origin_y + *y_search_center) - ((int16_t)ref_pic_ptr->height - 1)) :
        *y_search_center;
#endif
#if 1 // Amir-Hsan: to fix rate estimation or to use SAD only
    zero_mv_cost = zero_mv_sad;
#else
    zero_mv_cost = zero_mv_sad << COST_PRECISION;
#endif
    search_region_index = (int16_t)(ref_pic_ptr->origin_x + origin_x) + *x_search_center +
        ((int16_t)(ref_pic_ptr->origin_y + origin_y) + *y_search_center) * ref_pic_ptr->stride_y;

    hme_mv_sad = n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][sb_width >> 3](
        context_ptr->sb_src_ptr,
        context_ptr->sb_src_stride << subsample_sad,
        &(ref_pic_ptr->buffer_y[search_region_index]),
        ref_pic_ptr->stride_y << subsample_sad,
        sb_height >> subsample_sad,
        sb_width);

    hme_mv_sad = hme_mv_sad << subsample_sad;

#if 1 // Amir-Hsan: to fix rate estimation or to use SAD only
    hme_mv_cost = hme_mv_sad;
#else
    hme_mvd_rate = 0;
    MeGetMvdFractionBits(
        ABS(*x_search_center << 2),
        ABS(*y_search_center << 2),
        context_ptr->mvdBitsArray,
        &hme_mvd_rate);

    hme_mv_cost = (hme_mv_sad << COST_PRECISION) + (((context_ptr->lambda * hme_mvd_rate) + MD_OFFSET) >> MD_SHIFT);
#endif
    search_center_cost = MIN(zero_mv_cost, hme_mv_cost);

    *x_search_center = (search_center_cost == zero_mv_cost) ? 0 : *x_search_center;
    *y_search_center = (search_center_cost == zero_mv_cost) ? 0 : *y_search_center;

    return return_error;
}

EbErrorType su_pel_enable(
    MeContext               *context_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    uint32_t list_index,
    uint32_t ref_pic_index,
    EB_BOOL *enable_half_pel32x32,
    EB_BOOL *enable_half_pel16x16,
    EB_BOOL *enable_half_pel8x8)
{
    EbErrorType return_error = EB_ErrorNone;

    uint32_t mv_mag32x32  = 0;
    uint32_t mv_mag16x16  = 0;
    uint32_t mv_mag8x8    = 0;
    uint32_t avg_sad32x32 = 0;
    uint32_t avg_sad16x16 = 0;
    uint32_t avg_sad8x8   = 0;
    uint32_t avg_mvx32x32 = 0;
    uint32_t avg_mvy32x32 = 0;
    uint32_t avg_mvx16x16 = 0;
    uint32_t avg_mvy16x16 = 0;
    uint32_t avg_mvx8x8   = 0;
    uint32_t avg_mvy8x8   = 0;

    avg_mvx32x32 = (_MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_32x32_0]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_32x32_1]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_32x32_2]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_32x32_3])) >> 2;
    avg_mvy32x32 = (_MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_32x32_0]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_32x32_1]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_32x32_2]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_32x32_3])) >> 2;
    mv_mag32x32 = SQR(avg_mvx32x32) + SQR(avg_mvy32x32);

    avg_mvx16x16 = (_MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_0]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_1]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_2]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_3])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_4]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_5]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_6]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_7])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_8]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_9]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_10]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_11])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_12]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_13]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_14]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_15])
        ) >> 4;
    avg_mvy16x16 = (_MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_0]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_1]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_2]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_3])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_4]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_5]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_6]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_7])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_8]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_9]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_10]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_11])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_12]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_13]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_14]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_15])
        ) >> 4;
    mv_mag16x16 = SQR(avg_mvx16x16) + SQR(avg_mvy16x16);

    avg_mvx8x8 = (_MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_0]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_1]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_2]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_3])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_4]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_5]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_6]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_7])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_8]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_9]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_10]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_11])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_12]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_13]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_14]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_15])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_16]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_17]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_18]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_19])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_20]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_21]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_22]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_23])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_24]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_25]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_26]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_27])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_28]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_29]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_30]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_31])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_32]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_33]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_34]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_35])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_36]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_37]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_38]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_39])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_40]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_41]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_42]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_43])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_44]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_45]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_46]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_47])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_48]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_49]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_50]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_51])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_52]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_53]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_54]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_55])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_56]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_57]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_58]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_59])
        + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_60]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_61]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_62]) + _MVXT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_63])
        ) >> 6;
    avg_mvy8x8 = (_MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_0]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_1]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_2]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_3])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_4]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_5]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_6]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_7])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_8]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_9]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_10]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_11])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_12]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_13]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_14]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_15])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_16]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_17]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_18]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_19])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_20]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_21]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_22]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_23])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_24]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_25]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_26]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_27])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_28]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_29]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_30]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_31])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_32]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_33]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_34]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_35])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_36]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_37]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_38]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_39])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_40]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_41]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_42]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_43])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_44]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_45]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_46]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_47])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_48]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_49]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_50]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_51])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_52]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_53]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_54]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_55])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_56]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_57]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_58]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_59])
        + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_60]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_61]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_62]) + _MVYT(context_ptr->p_sb_best_mv[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_63])
        ) >> 6;
    mv_mag8x8 = SQR(avg_mvx8x8) + SQR(avg_mvy8x8);

    avg_sad32x32 = (context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_32x32_0] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_32x32_1] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_32x32_2] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_32x32_3]) >> 2;
    avg_sad16x16 = (context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_0] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_1] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_2] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_3]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_4] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_5] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_6] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_7]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_8] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_9] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_10] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_11]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_12] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_13] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_14] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_16x16_15]
        ) >> 4;
    avg_sad8x8 = (context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_0] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_1] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_2] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_3]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_4] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_5] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_6] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_7]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_8] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_9] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_10] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_11]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_12] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_13] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_14] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_15]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_16] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_17] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_18] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_19]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_20] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_21] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_22] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_23]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_24] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_25] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_26] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_27]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_28] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_29] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_30] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_31]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_32] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_33] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_34] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_35]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_36] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_37] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_38] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_39]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_40] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_41] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_42] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_43]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_44] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_45] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_46] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_47]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_48] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_49] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_50] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_51]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_52] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_53] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_54] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_55]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_56] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_57] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_58] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_59]
        + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_60] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_61] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_62] + context_ptr->p_sb_best_sad[list_index][ref_pic_index][ME_TIER_ZERO_PU_8x8_63]
        ) >> 6;

        if (picture_control_set_ptr->temporal_layer_index == 0)
        {
            //32x32
            if ((mv_mag32x32 < SQR(48)) && (avg_sad32x32 < 32 * 32 * 6))
            {
                *enable_half_pel32x32 = EB_TRUE; //CLASS_0
            }
            else if ((mv_mag32x32 < SQR(48)) && !(avg_sad32x32 < 32 * 32 * 6))
            {
                *enable_half_pel32x32 = EB_FALSE; //CLASS_1
            }
            else if (!(mv_mag32x32 < SQR(48)) && (avg_sad32x32 < 32 * 32 * 6))
            {
                *enable_half_pel32x32 = EB_TRUE; //CLASS_2
            }
            else
            {
                *enable_half_pel32x32 = EB_FALSE; //CLASS_3
            }
            //16x16
            if ((mv_mag16x16 < SQR(48)) && (avg_sad16x16 < 16 * 16 * 2))
            {
                *enable_half_pel16x16 = EB_FALSE; //CLASS_0
            }
            else if ((mv_mag16x16 < SQR(48)) && !(avg_sad16x16 < 16 * 16 * 2))
            {
                *enable_half_pel16x16 = EB_TRUE; //CLASS_1
            }
            else if (!(mv_mag16x16 < SQR(48)) && (avg_sad16x16 < 16 * 16 * 2))
            {
                *enable_half_pel16x16 = EB_FALSE; //CLASS_2
            }
            else
            {
                *enable_half_pel16x16 = EB_TRUE; //CLASS_3
            }
            //8x8
            if ((mv_mag8x8 < SQR(48)) && (avg_sad8x8 < 8 * 8 * 2))
            {
                *enable_half_pel8x8 = EB_FALSE; //CLASS_0
            }
            else if ((mv_mag8x8 < SQR(48)) && !(avg_sad8x8 < 8 * 8 * 2))
            {
                *enable_half_pel8x8 = EB_TRUE; //CLASS_1
            }
            else if (!(mv_mag8x8 < SQR(48)) && (avg_sad8x8 < 8 * 8 * 2))
            {
                *enable_half_pel8x8 = EB_FALSE; //CLASS_2
            }
            else
            {
                *enable_half_pel8x8 = EB_TRUE; //CLASS_3
            }

        }

        else if (picture_control_set_ptr->temporal_layer_index == 1)
        {
            //32x32
            if ((mv_mag32x32 < SQR(32)) && (avg_sad32x32 < 32 * 32 * 6))
            {
                *enable_half_pel32x32 = EB_TRUE; //CLASS_0
            }
            else if ((mv_mag32x32 < SQR(32)) && !(avg_sad32x32 < 32 * 32 * 6))
            {
                *enable_half_pel32x32 = EB_FALSE; //CLASS_1
            }
            else if (!(mv_mag32x32 < SQR(32)) && (avg_sad32x32 < 32 * 32 * 6))
            {
                *enable_half_pel32x32 = EB_TRUE; //CLASS_2
            }
            else
            {
                *enable_half_pel32x32 = EB_TRUE; //CLASS_3
            }
            //16x16
            if ((mv_mag16x16 < SQR(32)) && (avg_sad16x16 < 16 * 16 * 2))
            {
                *enable_half_pel16x16 = EB_FALSE; //CLASS_0
            }
            else if ((mv_mag16x16 < SQR(32)) && !(avg_sad16x16 < 16 * 16 * 2))
            {
                *enable_half_pel16x16 = EB_TRUE; //CLASS_1
            }
            else if (!(mv_mag16x16 < SQR(32)) && (avg_sad16x16 < 16 * 16 * 2))
            {
                *enable_half_pel16x16 = EB_FALSE; //CLASS_2
            }
            else
            {
                *enable_half_pel16x16 = EB_TRUE; //CLASS_3
            }
            //8x8
            if ((mv_mag8x8 < SQR(32)) && (avg_sad8x8 < 8 * 8 * 2))
            {
                *enable_half_pel8x8 = EB_FALSE; //CLASS_0
            }
            else if ((mv_mag8x8 < SQR(32)) && !(avg_sad8x8 < 8 * 8 * 2))
            {
                *enable_half_pel8x8 = EB_TRUE; //CLASS_1
            }
            else if (!(mv_mag8x8 < SQR(32)) && (avg_sad8x8 < 8 * 8 * 2))
            {
                *enable_half_pel8x8 = EB_FALSE; //CLASS_2
            }
            else
            {
                *enable_half_pel8x8 = EB_TRUE; //CLASS_3
            }

        }
        else if (picture_control_set_ptr->temporal_layer_index == 2)
        {
            //32x32
            if ((mv_mag32x32 < SQR(80)) && (avg_sad32x32 < 32 * 32 * 6))
            {
                *enable_half_pel32x32 = EB_TRUE; //CLASS_0
            }
            else if ((mv_mag32x32 < SQR(80)) && !(avg_sad32x32 < 32 * 32 * 6))
            {
                *enable_half_pel32x32 = EB_FALSE; //CLASS_1

            }
            else if (!(mv_mag32x32 < SQR(80)) && (avg_sad32x32 < 32 * 32 * 6))
            {
                *enable_half_pel32x32 = EB_TRUE; //CLASS_2
            }
            else
            {
                *enable_half_pel32x32 = EB_FALSE; //CLASS_3
            }
            //16x16
            if ((mv_mag16x16 < SQR(80)) && (avg_sad16x16 < 16 * 16 * 2))
            {
                *enable_half_pel16x16 = EB_FALSE; //CLASS_0
            }
            else if ((mv_mag16x16 < SQR(80)) && !(avg_sad16x16 < 16 * 16 * 2))
            {
                *enable_half_pel16x16 = EB_TRUE; //CLASS_1
            }
            else if (!(mv_mag16x16 < SQR(80)) && (avg_sad16x16 < 16 * 16 * 2))
            {
                *enable_half_pel16x16 = EB_FALSE; //CLASS_2
            }
            else
            {
                *enable_half_pel16x16 = EB_TRUE; //CLASS_3
            }
            //8x8
            if ((mv_mag8x8 < SQR(80)) && (avg_sad8x8 < 8 * 8 * 2))
            {
                *enable_half_pel8x8 = EB_FALSE; //CLASS_0
            }
            else if ((mv_mag8x8 < SQR(80)) && !(avg_sad8x8 < 8 * 8 * 2))
            {
                *enable_half_pel8x8 = EB_TRUE; //CLASS_1
            }
            else if (!(mv_mag8x8 < SQR(80)) && (avg_sad8x8 < 8 * 8 * 2))
            {
                *enable_half_pel8x8 = EB_FALSE; //CLASS_2
            }
            else
            {
                *enable_half_pel8x8 = EB_TRUE; //CLASS_3
            }

        }
        else
        {
            //32x32
            if ((mv_mag32x32 < SQR(48)) && (avg_sad32x32 < 32 * 32 * 6))
            {
                *enable_half_pel32x32 = EB_TRUE; //CLASS_0
            }
            else if ((mv_mag32x32 < SQR(48)) && !(avg_sad32x32 < 32 * 32 * 6))
            {
                *enable_half_pel32x32 = EB_TRUE; //CLASS_1
            }
            else if (!(mv_mag32x32 < SQR(48)) && (avg_sad32x32 < 32 * 32 * 6))
            {
                *enable_half_pel32x32 = EB_TRUE; //CLASS_2
            }
            else
            {
                *enable_half_pel32x32 = EB_FALSE; //CLASS_3
            }
            //16x16
            if ((mv_mag16x16 < SQR(48)) && (avg_sad16x16 < 16 * 16 * 2))
            {
                *enable_half_pel16x16 = EB_FALSE; //CLASS_0
            }
            else if ((mv_mag16x16 < SQR(48)) && !(avg_sad16x16 < 16 * 16 * 2))
            {
                *enable_half_pel16x16 = EB_TRUE; //CLASS_1
            }
            else if (!(mv_mag16x16 < SQR(48)) && (avg_sad16x16 < 16 * 16 * 2))
            {
                *enable_half_pel16x16 = EB_FALSE; //CLASS_2
            }
            else
            {
                *enable_half_pel16x16 = EB_TRUE; //CLASS_3
            }
            //8x8
            if ((mv_mag8x8 < SQR(48)) && (avg_sad8x8 < 8 * 8 * 2))
            {
                *enable_half_pel8x8 = EB_FALSE; //CLASS_0
            }
            else if ((mv_mag8x8 < SQR(48)) && !(avg_sad8x8 < 8 * 8 * 2))
            {
                *enable_half_pel8x8 = EB_TRUE; //CLASS_1
            }
            else if (!(mv_mag8x8 < SQR(48)) && (avg_sad8x8 < 8 * 8 * 2))
            {
                *enable_half_pel8x8 = EB_FALSE; //CLASS_2
            }
            else
            {
                *enable_half_pel8x8 = EB_FALSE;// EB_TRUE; //CLASS_3
            }

        }

    return return_error;
}

static void test_search_area_bounds(
    EbPictureBufferDesc *ref_pic_ptr,
    MeContext           *context_ptr,
    int16_t             *xsc,
    int16_t             *ysc,
    uint32_t             list_index,
    int16_t              origin_x,
    int16_t              origin_y,
    uint32_t             sb_width,
    uint32_t             sb_height)
{
    // Search for (-srx/2, 0),  (+srx/2, 0),  (0, -sry/2), (0, +sry/2),
    /*
    |------------C-------------|
    |--------------------------|
    |--------------------------|
    A            0             B
    |--------------------------|
    |--------------------------|
    |------------D-------------|
    */
    uint32_t search_region_index;
    int16_t x_search_center = *xsc;
    int16_t y_search_center = *ysc;
    uint64_t best_cost;
    uint64_t direct_mv_cost = 0xFFFFFFFFFFFFF;
    uint8_t  sparce_scale = 1;
    int16_t pad_width = (int16_t)MAX_SB_SIZE - 1;
    int16_t pad_height = (int16_t)MAX_SB_SIZE - 1;
    // O pos

    search_region_index = (int16_t)ref_pic_ptr->origin_x + origin_x +
        ((int16_t)ref_pic_ptr->origin_y + origin_y) * ref_pic_ptr->stride_y;

    uint32_t subsample_sad = 1;
    uint64_t zero_mv_sad = n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][sb_width >> 3](
        context_ptr->sb_src_ptr,
        context_ptr->sb_src_stride << subsample_sad,
        &(ref_pic_ptr->buffer_y[search_region_index]),
        ref_pic_ptr->stride_y << subsample_sad,
        sb_height >> subsample_sad,
        sb_width);

    zero_mv_sad = zero_mv_sad << subsample_sad;
#if 1 // Amir-Hsan: to fix rate estimation or to use SAD only
    uint64_t zero_mv_cost = zero_mv_sad;
#else
    uint64_t zero_mv_cost = zero_mv_sad << COST_PRECISION;
#endif
    //A pos
    x_search_center = 0 - (context_ptr->hme_level0_total_search_area_width*sparce_scale);
    y_search_center = 0;

    // Correct the left edge of the Search Area if it is not on the reference Picture
    x_search_center = ((origin_x + x_search_center) < -pad_width) ?
        -pad_width - origin_x :
        x_search_center;
    // Correct the right edge of the Search Area if its not on the reference Picture
    x_search_center = ((origin_x + x_search_center) > (int16_t)ref_pic_ptr->width - 1) ?
        x_search_center - ((origin_x + x_search_center) - ((int16_t)ref_pic_ptr->width - 1)) :
        x_search_center;
    // Correct the top edge of the Search Area if it is not on the reference Picture
    y_search_center = ((origin_y + y_search_center) < -pad_height) ?
        -pad_height - origin_y :
        y_search_center;

    // Correct the bottom edge of the Search Area if its not on the reference Picture
    y_search_center = ((origin_y + y_search_center) > (int16_t)ref_pic_ptr->height - 1) ?
        y_search_center - ((origin_y + y_search_center) - ((int16_t)ref_pic_ptr->height - 1)) :
        y_search_center;

    uint64_t mv_a_sad = n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][sb_width >> 3](
        context_ptr->sb_src_ptr,
        context_ptr->sb_src_stride << subsample_sad,
        &(ref_pic_ptr->buffer_y[search_region_index]),
        ref_pic_ptr->stride_y << subsample_sad,
        sb_height >> subsample_sad,
        sb_width);

    mv_a_sad = mv_a_sad << subsample_sad;
#if 1 // Amir-Hsan: to fix rate estimation or to use SAD only
    uint64_t mv_a_cost = mv_a_sad;
#else
    uint32_t MvAdRate = 0;
    MeGetMvdFractionBits(
        ABS(x_search_center << 2),
        ABS(y_search_center << 2),
        context_ptr->mvdBitsArray,
        &MvAdRate);

    uint64_t mv_a_cost = (mv_a_sad << COST_PRECISION) + (MD_OFFSET >> MD_SHIFT);
#endif

    //B pos
    x_search_center = (context_ptr->hme_level0_total_search_area_width*sparce_scale);
    y_search_center = 0;
    ///////////////// correct
    // Correct the left edge of the Search Area if it is not on the reference Picture
    x_search_center = ((origin_x + x_search_center) < -pad_width) ?
        -pad_width - origin_x :
        x_search_center;
    // Correct the right edge of the Search Area if its not on the reference Picture
    x_search_center = ((origin_x + x_search_center) > (int16_t)ref_pic_ptr->width - 1) ?
        x_search_center - ((origin_x + x_search_center) - ((int16_t)ref_pic_ptr->width - 1)) :
        x_search_center;
    // Correct the top edge of the Search Area if it is not on the reference Picture
    y_search_center = ((origin_y + y_search_center) < -pad_height) ?
        -pad_height - origin_y :
        y_search_center;
    // Correct the bottom edge of the Search Area if its not on the reference Picture
    y_search_center = ((origin_y + y_search_center) > (int16_t)ref_pic_ptr->height - 1) ?
        y_search_center - ((origin_y + y_search_center) - ((int16_t)ref_pic_ptr->height - 1)) :
        y_search_center;

    search_region_index = (int16_t)(ref_pic_ptr->origin_x + origin_x) + x_search_center +
        ((int16_t)(ref_pic_ptr->origin_y + origin_y) + y_search_center) * ref_pic_ptr->stride_y;

    uint64_t mv_b_sad = n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][sb_width >> 3](
        context_ptr->sb_src_ptr,
        context_ptr->sb_src_stride << subsample_sad,
        &(ref_pic_ptr->buffer_y[search_region_index]),
        ref_pic_ptr->stride_y << subsample_sad,
        sb_height >> subsample_sad,
        sb_width);

    mv_b_sad = mv_b_sad << subsample_sad;

#if 1 // Amir-Hsan: to fix rate estimation or to use SAD only
    uint64_t mv_b_cost = mv_b_sad;
#else
    uint32_t MvBdRate = 0;
    MeGetMvdFractionBits(
        ABS(x_search_center << 2),
        ABS(y_search_center << 2),
        context_ptr->mvdBitsArray,
        &MvBdRate);

    uint64_t mv_b_cost = (mv_b_sad << COST_PRECISION) + (MD_OFFSET >> MD_SHIFT);
#endif
    //C pos
    x_search_center = 0;
    y_search_center = 0 - (context_ptr->hme_level0_total_search_area_height * sparce_scale);
    ///////////////// correct
    // Correct the left edge of the Search Area if it is not on the reference Picture
    x_search_center = ((origin_x + x_search_center) < -pad_width) ?
        -pad_width - origin_x :
        x_search_center;

    // Correct the right edge of the Search Area if its not on the reference Picture
    x_search_center = ((origin_x + x_search_center) > (int16_t)ref_pic_ptr->width - 1) ?
        x_search_center - ((origin_x + x_search_center) - ((int16_t)ref_pic_ptr->width - 1)) :
        x_search_center;

    // Correct the top edge of the Search Area if it is not on the reference Picture
    y_search_center = ((origin_y + y_search_center) < -pad_height) ?
        -pad_height - origin_y :
        y_search_center;

    // Correct the bottom edge of the Search Area if its not on the reference Picture
    y_search_center = ((origin_y + y_search_center) > (int16_t)ref_pic_ptr->height - 1) ?
        y_search_center - ((origin_y + y_search_center) - ((int16_t)ref_pic_ptr->height - 1)) :
        y_search_center;

    search_region_index = (int16_t)(ref_pic_ptr->origin_x + origin_x) + x_search_center +
        ((int16_t)(ref_pic_ptr->origin_y + origin_y) + y_search_center) * ref_pic_ptr->stride_y;

    uint64_t mv_c_sad = n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][sb_width >> 3](
        context_ptr->sb_src_ptr,
        context_ptr->sb_src_stride << subsample_sad,
        &(ref_pic_ptr->buffer_y[search_region_index]),
        ref_pic_ptr->stride_y << subsample_sad,
        sb_height >> subsample_sad,
        sb_width);

    mv_c_sad = mv_c_sad << subsample_sad;

#if 1 // Amir-Hsan: to fix rate estimation or to use SAD only
    uint64_t mv_c_cost = mv_c_sad;
#else
    uint32_t MvCdRate = 0;
    MeGetMvdFractionBits(
        ABS(x_search_center << 2),
        ABS(y_search_center << 2),
        context_ptr->mvdBitsArray,
        &MvCdRate);

    uint64_t mv_c_cost = (mv_c_sad << COST_PRECISION) + (MD_OFFSET >> MD_SHIFT);
#endif
    //D pos
    x_search_center = 0;
    y_search_center = (context_ptr->hme_level0_total_search_area_height * sparce_scale);
    // Correct the left edge of the Search Area if it is not on the reference Picture
    x_search_center = ((origin_x + x_search_center) < -pad_width) ?
        -pad_width - origin_x :
        x_search_center;
    // Correct the right edge of the Search Area if its not on the reference Picture
    x_search_center = ((origin_x + x_search_center) > (int16_t)ref_pic_ptr->width - 1) ?
        x_search_center - ((origin_x + x_search_center) - ((int16_t)ref_pic_ptr->width - 1)) :
        x_search_center;
    // Correct the top edge of the Search Area if it is not on the reference Picture
    y_search_center = ((origin_y + y_search_center) < -pad_height) ?
        -pad_height - origin_y :
        y_search_center;
    // Correct the bottom edge of the Search Area if its not on the reference Picture
    y_search_center = ((origin_y + y_search_center) > (int16_t)ref_pic_ptr->height - 1) ?
        y_search_center - ((origin_y + y_search_center) - ((int16_t)ref_pic_ptr->height - 1)) :
        y_search_center;
    search_region_index = (int16_t)(ref_pic_ptr->origin_x + origin_x) + x_search_center +
        ((int16_t)(ref_pic_ptr->origin_y + origin_y) + y_search_center) * ref_pic_ptr->stride_y;
    uint64_t mv_d_sad = n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][sb_width >> 3](
        context_ptr->sb_src_ptr,
        context_ptr->sb_src_stride << subsample_sad,
        &(ref_pic_ptr->buffer_y[search_region_index]),
        ref_pic_ptr->stride_y << subsample_sad,
        sb_height >> subsample_sad,
        sb_width);
    mv_d_sad = mv_d_sad << subsample_sad;
#if 1 // Amir-Hsan: to fix rate estimation or to use SAD only
    uint64_t mv_d_cost = mv_d_sad;
#else
    uint32_t MvDdRate = 0;
    MeGetMvdFractionBits(
        ABS(x_search_center << 2),
        ABS(y_search_center << 2),
        context_ptr->mvdBitsArray,
        &MvDdRate);

    uint64_t mv_d_cost = (mv_d_sad << COST_PRECISION) + (MD_OFFSET >> MD_SHIFT);
#endif
    if (list_index == 1) {

        x_search_center = list_index ? 0 - (_MVXT(context_ptr->p_sb_best_mv[0][0][0]) >> 2) : 0;
        y_search_center = list_index ? 0 - (_MVYT(context_ptr->p_sb_best_mv[0][0][0]) >> 2) : 0;
        ///////////////// correct
        // Correct the left edge of the Search Area if it is not on the reference Picture
        x_search_center = ((origin_x + x_search_center) < -pad_width) ?
            -pad_width - origin_x :
            x_search_center;
        // Correct the right edge of the Search Area if its not on the reference Picture
        x_search_center = ((origin_x + x_search_center) > (int16_t)ref_pic_ptr->width - 1) ?
            x_search_center - ((origin_x + x_search_center) - ((int16_t)ref_pic_ptr->width - 1)) :
            x_search_center;
        // Correct the top edge of the Search Area if it is not on the reference Picture
        y_search_center = ((origin_y + y_search_center) < -pad_height) ?
            -pad_height - origin_y :
            y_search_center;
        // Correct the bottom edge of the Search Area if its not on the reference Picture
        y_search_center = ((origin_y + y_search_center) > (int16_t)ref_pic_ptr->height - 1) ?
            y_search_center - ((origin_y + y_search_center) - ((int16_t)ref_pic_ptr->height - 1)) :
            y_search_center;

        search_region_index = (int16_t)(ref_pic_ptr->origin_x + origin_x) + x_search_center +
            ((int16_t)(ref_pic_ptr->origin_y + origin_y) + y_search_center) * ref_pic_ptr->stride_y;

        uint64_t direct_mv_sad = n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][sb_width >> 3](
            context_ptr->sb_src_ptr,
            context_ptr->sb_src_stride << subsample_sad,
            &(ref_pic_ptr->buffer_y[search_region_index]),
            ref_pic_ptr->stride_y << subsample_sad,
            sb_height >> subsample_sad,
            sb_width);

        direct_mv_sad = direct_mv_sad << subsample_sad;

#if 1 // Amir-Hsan: to fix rate estimation or to use SAD only
        direct_mv_cost = direct_mv_sad;
#else
        uint32_t direcMvdRate = 0;
        MeGetMvdFractionBits(
            ABS(x_search_center << 2),
            ABS(y_search_center << 2),
            context_ptr->mvdBitsArray,
            &direcMvdRate);

        direct_mv_cost = (direct_mv_sad << COST_PRECISION) + (MD_OFFSET >> MD_SHIFT);
#endif
    }

    best_cost = MIN(zero_mv_cost, MIN(mv_a_cost, MIN(mv_b_cost, MIN(mv_c_cost, MIN(mv_d_cost, direct_mv_cost)))));

    if (best_cost == zero_mv_cost) {
        x_search_center = 0;
        y_search_center = 0;
    }
    else if (best_cost == mv_a_cost) {
        x_search_center = 0 - (context_ptr->hme_level0_total_search_area_width*sparce_scale);
        y_search_center = 0;
    }
    else if (best_cost == mv_b_cost) {
        x_search_center = (context_ptr->hme_level0_total_search_area_width*sparce_scale);
        y_search_center = 0;
    }
    else if (best_cost == mv_c_cost) {
        x_search_center = 0;
        y_search_center = 0 - (context_ptr->hme_level0_total_search_area_height * sparce_scale);
    }
    else if (best_cost == direct_mv_cost) {
        x_search_center = list_index ? 0 - (_MVXT(context_ptr->p_sb_best_mv[0][0][0]) >> 2) : 0;
        y_search_center = list_index ? 0 - (_MVYT(context_ptr->p_sb_best_mv[0][0][0]) >> 2) : 0;
    }
    else if (best_cost == mv_d_cost) {
        x_search_center = 0;
        y_search_center = (context_ptr->hme_level0_total_search_area_height * sparce_scale);
    }

    else {
        SVT_LOG("error no center selected");
    }

    *xsc = x_search_center;
    *ysc = y_search_center;
}

/*******************************************
 * motion_estimate_sb
 *   performs ME (SB)
 *******************************************/
EbErrorType motion_estimate_sb(
    PictureParentControlSet *picture_control_set_ptr,  // input parameter, Picture Control Set Ptr
    uint32_t                 lblock_index,             // input parameter, LCU Index
    uint32_t                 sb_origin_x,              // input parameter, LCU Origin X
    uint32_t                 sb_origin_y,              // input parameter, LCU Origin X
    MeContext               *context_ptr,              // input parameter, ME Context Ptr, used to store decimated/interpolated LCU/SR
    EbPictureBufferDesc     *input_ptr)                // input parameter, source Picture Ptr
{
    EbErrorType return_error = EB_ErrorNone;

    SequenceControlSet      *sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;

    int16_t                  x_top_left_search_region;
    int16_t                  y_top_left_search_region;
    uint32_t                 search_region_index;
    int16_t                  picture_width = (int16_t)((SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr)->luma_width;
    int16_t                  picture_height = (int16_t)((SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr)->luma_height;
    uint32_t                 sb_width = (input_ptr->width - sb_origin_x) < MAX_SB_SIZE ? input_ptr->width - sb_origin_x : MAX_SB_SIZE;
    uint32_t                 sb_height = (input_ptr->height - sb_origin_y) < MAX_SB_SIZE ? input_ptr->height - sb_origin_y : MAX_SB_SIZE;
    int16_t                  pad_width = (int16_t)MAX_SB_SIZE - 1;
    int16_t                  pad_height = (int16_t)MAX_SB_SIZE - 1;
    int16_t                  search_area_width;
    int16_t                  search_area_height;
    int16_t                  x_search_area_origin;
    int16_t                  y_search_area_origin;
    int16_t                  origin_x = (int16_t)sb_origin_x;
    int16_t                  origin_y = (int16_t)sb_origin_y;

    // HME
    uint32_t                 search_region_number_in_width = 0;
    uint32_t                 search_region_number_in_height = 0;
    int16_t                  x_hme_level0_search_center[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    int16_t                  y_hme_level0_search_center[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    uint64_t                 hme_level0_sad[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    int16_t                  x_hme_level1_search_center[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    int16_t                  y_hme_level1_search_center[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    uint64_t                 hme_level1_sad[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    int16_t                  x_hme_level2_search_center[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    int16_t                  y_hme_level2_search_center[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    uint64_t                 hme_level2_sad[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];

    // Hierarchical ME Search Center
    int16_t                  x_hme_search_center = 0;
    int16_t                  y_hme_search_center = 0;

    // Final ME Search Center
    int16_t                  x_search_center = 0;
    int16_t                  y_search_center = 0;

    // Search Center SADs
    uint64_t                 hme_mv_sad = 0;

    uint32_t                 pu_index;

    uint32_t                 max_number_of_pus_per_sb = picture_control_set_ptr->max_number_of_pus_per_sb;

    uint32_t                 num_of_list_to_search;
    uint32_t                 list_index;
    uint8_t                  candidate_index = 0;
    uint8_t                  total_me_candidate_index = 0;
    EbPaReferenceObject     *reference_object;  // input parameter, reference Object Ptr

    EbPictureBufferDesc     *ref_pic_ptr;
    EbPictureBufferDesc     *quarter_ref_pic_ptr;
    EbPictureBufferDesc     *sixteenth_ref_pic_ptr;

    int16_t                  temp_x_hme_search_center = 0;
    int16_t                  temp_y_hme_search_center = 0;

    uint32_t                 num_quad_in_width;
    uint32_t                 total_me_quad;
    uint32_t                 quad_index;
    uint32_t                 next_quad_index;
    uint64_t                 temp_x_hme_distortion;

    uint64_t                 ref0_poc = 0;
    uint64_t                 ref1_poc = 0;

    uint64_t                 i;

    int16_t                  hme_level1_search_area_in_width;
    int16_t                  hme_level1_search_area_in_height;

    // Configure HME level 0, level 1 and level 2 from static config parameters
    EB_BOOL                 enable_hme_level_0_flag = picture_control_set_ptr->enable_hme_level_0_flag;
    EB_BOOL                 enable_hme_level_1_flag = picture_control_set_ptr->enable_hme_level_1_flag;
    EB_BOOL                 enable_hme_level_2_flag = picture_control_set_ptr->enable_hme_level_2_flag;
    EB_BOOL                 enable_half_pel32x32 = EB_FALSE;
    EB_BOOL                 enable_half_pel16x16 = EB_FALSE;
    EB_BOOL                 enable_half_pel8x8 = EB_FALSE;
    EB_BOOL                 enable_quarter_pel = EB_FALSE;

    num_of_list_to_search = (picture_control_set_ptr->slice_type == P_SLICE) ? (uint32_t)REF_LIST_0 : (uint32_t)REF_LIST_1;

    reference_object = (EbPaReferenceObject*)picture_control_set_ptr->ref_pa_pic_ptr_array[0]->object_ptr;
    ref0_poc = picture_control_set_ptr->ref_pic_poc_array[0];

    if (num_of_list_to_search) {

        reference_object = (EbPaReferenceObject*)picture_control_set_ptr->ref_pa_pic_ptr_array[1]->object_ptr;
        ref1_poc = picture_control_set_ptr->ref_pic_poc_array[1];
    }

    // Uni-Prediction motion estimation loop
    // List Loop
    for (list_index = REF_LIST_0; list_index <= num_of_list_to_search; ++list_index) {

        // Ref Picture Loop
        {

            reference_object = (EbPaReferenceObject*)picture_control_set_ptr->ref_pa_pic_ptr_array[list_index]->object_ptr;
            ref_pic_ptr = (EbPictureBufferDesc*)reference_object->input_padded_picture_ptr;
            quarter_ref_pic_ptr = (EbPictureBufferDesc*)reference_object->quarter_decimated_picture_ptr;
            sixteenth_ref_pic_ptr = (EbPictureBufferDesc*)reference_object->sixteenth_decimated_picture_ptr;

            if (picture_control_set_ptr->temporal_layer_index > 0 || list_index == 0) {
                // A - The MV center for Tier0 search could be either (0,0), or HME
                // A - Set HME MV Center
#if HME_ENHANCED_CENTER_SEARCH // TEST1
                test_search_area_bounds(
                    ref_pic_ptr,
                    context_ptr,
                    &x_search_center,
                    &y_search_center,
                    list_index,
                    origin_x,
                    origin_y,
                    sb_width,
                    sb_height);

#else
                x_search_center = 0;
                y_search_center = 0;
#endif
                // B - NO HME in boundaries
                // C - Skip HME

                if (picture_control_set_ptr->enable_hme_flag && /*B*/sb_height == MAX_SB_SIZE) {//(searchCenterSad > sequence_control_set_ptr->static_config.skipTier0HmeTh)) {

                    while (search_region_number_in_height < context_ptr->number_hme_search_region_in_height) {
                        while (search_region_number_in_width < context_ptr->number_hme_search_region_in_width) {
#if HME_ENHANCED_CENTER_SEARCH
                            x_hme_level0_search_center[search_region_number_in_width][search_region_number_in_height] = x_search_center >> 2;
                            y_hme_level0_search_center[search_region_number_in_width][search_region_number_in_height] = y_search_center >> 2;

                            x_hme_level1_search_center[search_region_number_in_width][search_region_number_in_height] = x_search_center >> 1;
                            y_hme_level1_search_center[search_region_number_in_width][search_region_number_in_height] = y_search_center >> 1;

#else
                            x_hme_level0_search_center[search_region_number_in_width][search_region_number_in_height] = x_search_center;
                            y_hme_level0_search_center[search_region_number_in_width][search_region_number_in_height] = y_search_center;

                            x_hme_level1_search_center[search_region_number_in_width][search_region_number_in_height] = x_search_center;
                            y_hme_level1_search_center[search_region_number_in_width][search_region_number_in_height] = y_search_center;
#endif
                            x_hme_level2_search_center[search_region_number_in_width][search_region_number_in_height] = x_search_center;
                            y_hme_level2_search_center[search_region_number_in_width][search_region_number_in_height] = y_search_center;

                            search_region_number_in_width++;
                        }
                        search_region_number_in_width = 0;
                        search_region_number_in_height++;
                    }

                    // HME: Level0 search
                    if (enable_hme_level_0_flag) {

                        if (context_ptr->single_hme_quadrant && !enable_hme_level_1_flag && !enable_hme_level_2_flag) {

                            search_region_number_in_height = 0;
                            search_region_number_in_width = 0;

                            single_hme_quadrant_level0(
                                context_ptr,
                                origin_x >> 2,
                                origin_y >> 2,
                                sb_width >> 2,
                                sb_height >> 2,
                                x_search_center >> 2,
                                y_search_center >> 2,
                                sixteenth_ref_pic_ptr,
                                &(hme_level0_sad[search_region_number_in_width][search_region_number_in_height]),
                                &(x_hme_level0_search_center[search_region_number_in_width][search_region_number_in_height]),
                                &(y_hme_level0_search_center[search_region_number_in_width][search_region_number_in_height]),
                                hme_level_0_search_area_multiplier_x[picture_control_set_ptr->hierarchical_levels][picture_control_set_ptr->temporal_layer_index],
                                hme_level_0_search_area_multiplier_y[picture_control_set_ptr->hierarchical_levels][picture_control_set_ptr->temporal_layer_index]);

                        }
                        else {

                            search_region_number_in_height = 0;
                            search_region_number_in_width = 0;

                            while (search_region_number_in_height < context_ptr->number_hme_search_region_in_height) {
                                while (search_region_number_in_width < context_ptr->number_hme_search_region_in_width) {

                                    hme_level0(
                                        context_ptr,
                                        origin_x >> 2,
                                        origin_y >> 2,
                                        sb_width >> 2,
                                        sb_height >> 2,
                                        x_search_center >> 2,
                                        y_search_center >> 2,
                                        sixteenth_ref_pic_ptr,
                                        search_region_number_in_width,
                                        search_region_number_in_height,
                                        &(hme_level0_sad[search_region_number_in_width][search_region_number_in_height]),
                                        &(x_hme_level0_search_center[search_region_number_in_width][search_region_number_in_height]),
                                        &(y_hme_level0_search_center[search_region_number_in_width][search_region_number_in_height]),
                                        hme_level_0_search_area_multiplier_x[picture_control_set_ptr->hierarchical_levels][picture_control_set_ptr->temporal_layer_index],
                                        hme_level_0_search_area_multiplier_y[picture_control_set_ptr->hierarchical_levels][picture_control_set_ptr->temporal_layer_index]);

                                    search_region_number_in_width++;
                                }
                                search_region_number_in_width = 0;
                                search_region_number_in_height++;
                            }
                        }
                    }

                    // HME: Level1 search
                    if (enable_hme_level_1_flag) {
                        search_region_number_in_height = 0;
                        search_region_number_in_width = 0;

                        while (search_region_number_in_height < context_ptr->number_hme_search_region_in_height) {
                            while (search_region_number_in_width < context_ptr->number_hme_search_region_in_width) {

                                // When HME level 0 has been disabled, increase the search area width and height for level 1 to (32x12)
                                hme_level1_search_area_in_width = (int16_t)context_ptr->hme_level1_search_area_in_width_array[search_region_number_in_width];
                                hme_level1_search_area_in_height = (int16_t)context_ptr->hme_level1_search_area_in_height_array[search_region_number_in_height];
                                hme_level1(
                                    context_ptr,
                                    origin_x >> 1,
                                    origin_y >> 1,
                                    sb_width >> 1,
                                    sb_height >> 1,
                                    quarter_ref_pic_ptr,
                                    hme_level1_search_area_in_width,
                                    hme_level1_search_area_in_height,
                                    x_hme_level0_search_center[search_region_number_in_width][search_region_number_in_height] >> 1,
                                    y_hme_level0_search_center[search_region_number_in_width][search_region_number_in_height] >> 1,
                                    &(hme_level1_sad[search_region_number_in_width][search_region_number_in_height]),
                                    &(x_hme_level1_search_center[search_region_number_in_width][search_region_number_in_height]),
                                    &(y_hme_level1_search_center[search_region_number_in_width][search_region_number_in_height]));

                                search_region_number_in_width++;
                            }
                            search_region_number_in_width = 0;
                            search_region_number_in_height++;
                        }
                    }

                    // HME: Level2 search
                    if (enable_hme_level_2_flag) {

                        search_region_number_in_height = 0;
                        search_region_number_in_width = 0;

                        while (search_region_number_in_height < context_ptr->number_hme_search_region_in_height) {
                            while (search_region_number_in_width < context_ptr->number_hme_search_region_in_width) {

                                hme_level2(
                                    context_ptr,
                                    origin_x,
                                    origin_y,
                                    sb_width,
                                    sb_height,
                                    ref_pic_ptr,
                                    search_region_number_in_width,
                                    search_region_number_in_height,
                                    x_hme_level1_search_center[search_region_number_in_width][search_region_number_in_height],
                                    y_hme_level1_search_center[search_region_number_in_width][search_region_number_in_height],
                                    &(hme_level2_sad[search_region_number_in_width][search_region_number_in_height]),
                                    &(x_hme_level2_search_center[search_region_number_in_width][search_region_number_in_height]),
                                    &(y_hme_level2_search_center[search_region_number_in_width][search_region_number_in_height]));

                                search_region_number_in_width++;
                            }
                            search_region_number_in_width = 0;
                            search_region_number_in_height++;
                        }
                    }

                    // Hierarchical ME - Search Center
                    if (enable_hme_level_0_flag && !enable_hme_level_1_flag && !enable_hme_level_2_flag) {

                        if (context_ptr->single_hme_quadrant)
                        {
                            x_hme_search_center = x_hme_level0_search_center[0][0];
                            y_hme_search_center = y_hme_level0_search_center[0][0];
                            hme_mv_sad = hme_level0_sad[0][0];
                        }
                        else {

                            x_hme_search_center = x_hme_level0_search_center[0][0];
                            y_hme_search_center = y_hme_level0_search_center[0][0];
                            hme_mv_sad = hme_level0_sad[0][0];

                            search_region_number_in_width = 1;
                            search_region_number_in_height = 0;

                            while (search_region_number_in_height < context_ptr->number_hme_search_region_in_height) {
                                while (search_region_number_in_width < context_ptr->number_hme_search_region_in_width) {

                                    x_hme_search_center = (hme_level0_sad[search_region_number_in_width][search_region_number_in_height] < hme_mv_sad) ? x_hme_level0_search_center[search_region_number_in_width][search_region_number_in_height] : x_hme_search_center;
                                    y_hme_search_center = (hme_level0_sad[search_region_number_in_width][search_region_number_in_height] < hme_mv_sad) ? y_hme_level0_search_center[search_region_number_in_width][search_region_number_in_height] : y_hme_search_center;
                                    hme_mv_sad = (hme_level0_sad[search_region_number_in_width][search_region_number_in_height] < hme_mv_sad) ? hme_level0_sad[search_region_number_in_width][search_region_number_in_height] : hme_mv_sad;
                                    search_region_number_in_width++;
                                }
                                search_region_number_in_width = 0;
                                search_region_number_in_height++;
                            }
                        }

                    }

                    if (enable_hme_level_1_flag && !enable_hme_level_2_flag) {
                        x_hme_search_center = x_hme_level1_search_center[0][0];
                        y_hme_search_center = y_hme_level1_search_center[0][0];
                        hme_mv_sad = hme_level1_sad[0][0];

                        search_region_number_in_width = 1;
                        search_region_number_in_height = 0;

                        while (search_region_number_in_height < context_ptr->number_hme_search_region_in_height) {
                            while (search_region_number_in_width < context_ptr->number_hme_search_region_in_width) {

                                x_hme_search_center = (hme_level1_sad[search_region_number_in_width][search_region_number_in_height] < hme_mv_sad) ? x_hme_level1_search_center[search_region_number_in_width][search_region_number_in_height] : x_hme_search_center;
                                y_hme_search_center = (hme_level1_sad[search_region_number_in_width][search_region_number_in_height] < hme_mv_sad) ? y_hme_level1_search_center[search_region_number_in_width][search_region_number_in_height] : y_hme_search_center;
                                hme_mv_sad = (hme_level1_sad[search_region_number_in_width][search_region_number_in_height] < hme_mv_sad) ? hme_level1_sad[search_region_number_in_width][search_region_number_in_height] : hme_mv_sad;
                                search_region_number_in_width++;
                            }
                            search_region_number_in_width = 0;
                            search_region_number_in_height++;
                        }
                    }

                    if (enable_hme_level_2_flag) {
                        x_hme_search_center = x_hme_level2_search_center[0][0];
                        y_hme_search_center = y_hme_level2_search_center[0][0];
                        hme_mv_sad = hme_level2_sad[0][0];

                        search_region_number_in_width = 1;
                        search_region_number_in_height = 0;

                        while (search_region_number_in_height < context_ptr->number_hme_search_region_in_height) {
                            while (search_region_number_in_width < context_ptr->number_hme_search_region_in_width) {

                                x_hme_search_center = (hme_level2_sad[search_region_number_in_width][search_region_number_in_height] < hme_mv_sad) ? x_hme_level2_search_center[search_region_number_in_width][search_region_number_in_height] : x_hme_search_center;
                                y_hme_search_center = (hme_level2_sad[search_region_number_in_width][search_region_number_in_height] < hme_mv_sad) ? y_hme_level2_search_center[search_region_number_in_width][search_region_number_in_height] : y_hme_search_center;
                                hme_mv_sad = (hme_level2_sad[search_region_number_in_width][search_region_number_in_height] < hme_mv_sad) ? hme_level2_sad[search_region_number_in_width][search_region_number_in_height] : hme_mv_sad;
                                search_region_number_in_width++;
                            }
                            search_region_number_in_width = 0;
                            search_region_number_in_height++;
                        }

                        num_quad_in_width = context_ptr->number_hme_search_region_in_width;
                        total_me_quad = context_ptr->number_hme_search_region_in_height * context_ptr->number_hme_search_region_in_width;

                        if ((ref0_poc == ref1_poc) && (list_index == 1) && (total_me_quad > 1)) {

                            for (quad_index = 0; quad_index < total_me_quad - 1; ++quad_index) {
                                for (next_quad_index = quad_index + 1; next_quad_index < total_me_quad; ++next_quad_index) {

                                    if (hme_level2_sad[quad_index / num_quad_in_width][quad_index%num_quad_in_width] > hme_level2_sad[next_quad_index / num_quad_in_width][next_quad_index%num_quad_in_width]) {

                                        temp_x_hme_search_center = x_hme_level2_search_center[quad_index / num_quad_in_width][quad_index%num_quad_in_width];
                                        temp_y_hme_search_center = y_hme_level2_search_center[quad_index / num_quad_in_width][quad_index%num_quad_in_width];
                                        temp_x_hme_distortion = hme_level2_sad[quad_index / num_quad_in_width][quad_index%num_quad_in_width];

                                        x_hme_level2_search_center[quad_index / num_quad_in_width][quad_index%num_quad_in_width] = x_hme_level2_search_center[next_quad_index / num_quad_in_width][next_quad_index%num_quad_in_width];
                                        y_hme_level2_search_center[quad_index / num_quad_in_width][quad_index%num_quad_in_width] = y_hme_level2_search_center[next_quad_index / num_quad_in_width][next_quad_index%num_quad_in_width];
                                        hme_level2_sad[quad_index / num_quad_in_width][quad_index%num_quad_in_width] = hme_level2_sad[next_quad_index / num_quad_in_width][next_quad_index%num_quad_in_width];

                                        x_hme_level2_search_center[next_quad_index / num_quad_in_width][next_quad_index%num_quad_in_width] = temp_x_hme_search_center;
                                        y_hme_level2_search_center[next_quad_index / num_quad_in_width][next_quad_index%num_quad_in_width] = temp_y_hme_search_center;
                                        hme_level2_sad[next_quad_index / num_quad_in_width][next_quad_index%num_quad_in_width] = temp_x_hme_distortion;
                                    }
                                }
                            }

                            x_hme_search_center = x_hme_level2_search_center[0][1];
                            y_hme_search_center = y_hme_level2_search_center[0][1];
                        }
                    }

                    x_search_center = x_hme_search_center;
                    y_search_center = y_hme_search_center;
                }
            }
            else {
                x_search_center = 0;
                y_search_center = 0;
            }

            search_area_width = (int16_t)MIN(context_ptr->search_area_width, 127);
            search_area_height = (int16_t)MIN(context_ptr->search_area_height, 127);

            if (x_search_center != 0 || y_search_center != 0) {
                check_zero_zero_center(
                    ref_pic_ptr,
                    context_ptr,
                    sb_origin_x,
                    sb_origin_y,
                    sb_width,
                    sb_height,
                    &x_search_center,
                    &y_search_center);
            }

            x_search_area_origin = x_search_center - (search_area_width >> 1);
            y_search_area_origin = y_search_center - (search_area_height >> 1);

            // Correct the left edge of the Search Area if it is not on the reference Picture
            x_search_area_origin = ((origin_x + x_search_area_origin) < -pad_width) ?
                -pad_width - origin_x :
                x_search_area_origin;

            search_area_width = ((origin_x + x_search_area_origin) < -pad_width) ?
                search_area_width - (-pad_width - (origin_x + x_search_area_origin)) :
                search_area_width;

            // Correct the right edge of the Search Area if its not on the reference Picture
            x_search_area_origin = ((origin_x + x_search_area_origin) > picture_width - 1) ?
                x_search_area_origin - ((origin_x + x_search_area_origin) - (picture_width - 1)) :
                x_search_area_origin;

            search_area_width = ((origin_x + x_search_area_origin + search_area_width) > picture_width) ?
                MAX(1, search_area_width - ((origin_x + x_search_area_origin + search_area_width) - picture_width)) :
                search_area_width;

            // Correct the top edge of the Search Area if it is not on the reference Picture
            y_search_area_origin = ((origin_y + y_search_area_origin) < -pad_height) ?
                -pad_height - origin_y :
                y_search_area_origin;

            search_area_height = ((origin_y + y_search_area_origin) < -pad_height) ?
                search_area_height - (-pad_height - (origin_y + y_search_area_origin)) :
                search_area_height;

            // Correct the bottom edge of the Search Area if its not on the reference Picture
            y_search_area_origin = ((origin_y + y_search_area_origin) > picture_height - 1) ?
                y_search_area_origin - ((origin_y + y_search_area_origin) - (picture_height - 1)) :
                y_search_area_origin;

            search_area_height = (origin_y + y_search_area_origin + search_area_height > picture_height) ?
                MAX(1, search_area_height - ((origin_y + y_search_area_origin + search_area_height) - picture_height)) :
                search_area_height;

            context_ptr->x_search_area_origin[list_index][0] = x_search_area_origin;
            context_ptr->y_search_area_origin[list_index][0] = y_search_area_origin;

            x_top_left_search_region = (int16_t)(ref_pic_ptr->origin_x + sb_origin_x) - (ME_FILTER_TAP >> 1) + x_search_area_origin;
            y_top_left_search_region = (int16_t)(ref_pic_ptr->origin_y + sb_origin_y) - (ME_FILTER_TAP >> 1) + y_search_area_origin;
            search_region_index = (x_top_left_search_region)+(y_top_left_search_region)* ref_pic_ptr->stride_y;

            context_ptr->integer_buffer_ptr[list_index][0] = &(ref_pic_ptr->buffer_y[search_region_index]);
            context_ptr->interpolated_full_stride[list_index][0] = ref_pic_ptr->stride_y;

            // Move to the top left of the search region
            x_top_left_search_region = (int16_t)(ref_pic_ptr->origin_x + sb_origin_x) + x_search_area_origin;
            y_top_left_search_region = (int16_t)(ref_pic_ptr->origin_y + sb_origin_y) + y_search_area_origin;
            search_region_index = x_top_left_search_region + y_top_left_search_region * ref_pic_ptr->stride_y;

            {
                {

                    eb_vp9_initialize_buffer_32bits_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](context_ptr->p_sb_best_sad[list_index][0], 21, 1, MAX_SAD_VALUE);
                    context_ptr->p_best_sad64x64 = &(context_ptr->p_sb_best_sad[list_index][0][ME_TIER_ZERO_PU_64x64]);
                    context_ptr->p_best_sad32x32 = &(context_ptr->p_sb_best_sad[list_index][0][ME_TIER_ZERO_PU_32x32_0]);
                    context_ptr->p_best_sad16x16 = &(context_ptr->p_sb_best_sad[list_index][0][ME_TIER_ZERO_PU_16x16_0]);
                    context_ptr->p_best_sad8x8 = &(context_ptr->p_sb_best_sad[list_index][0][ME_TIER_ZERO_PU_8x8_0]);

                    context_ptr->p_best_mv64x64 = &(context_ptr->p_sb_best_mv[list_index][0][ME_TIER_ZERO_PU_64x64]);
                    context_ptr->p_best_mv32x32 = &(context_ptr->p_sb_best_mv[list_index][0][ME_TIER_ZERO_PU_32x32_0]);
                    context_ptr->p_best_mv16x16 = &(context_ptr->p_sb_best_mv[list_index][0][ME_TIER_ZERO_PU_16x16_0]);
                    context_ptr->p_best_mv8x8 = &(context_ptr->p_sb_best_mv[list_index][0][ME_TIER_ZERO_PU_8x8_0]);

                    context_ptr->p_best_ssd64x64 = &(context_ptr->p_sb_best_ssd[list_index][0][ME_TIER_ZERO_PU_64x64]);
                    context_ptr->p_best_ssd32x32 = &(context_ptr->p_sb_best_ssd[list_index][0][ME_TIER_ZERO_PU_32x32_0]);
                    context_ptr->p_best_ssd16x16 = &(context_ptr->p_sb_best_ssd[list_index][0][ME_TIER_ZERO_PU_16x16_0]);
                    context_ptr->p_best_ssd8x8 = &(context_ptr->p_sb_best_ssd[list_index][0][ME_TIER_ZERO_PU_8x8_0]);

                    full_pel_search_sb(
                        context_ptr,
                        list_index,
                        x_search_area_origin,
                        y_search_area_origin,
                        search_area_width,
                        search_area_height
                    );

                }

                if (context_ptr->fractional_search_model == 0) {
                    enable_half_pel32x32 = EB_TRUE;
                    enable_half_pel16x16 = EB_TRUE;
                    enable_half_pel8x8 = EB_TRUE;
                    enable_quarter_pel = EB_TRUE;
                }
                else if (context_ptr->fractional_search_model == 1) {
                    su_pel_enable(
                        context_ptr,
                        picture_control_set_ptr,
                        list_index,
                        0,
                        &enable_half_pel32x32,
                        &enable_half_pel16x16,
                        &enable_half_pel8x8);
                    enable_quarter_pel = EB_TRUE;

                }
                else {
                    enable_half_pel32x32 = EB_FALSE;
                    enable_half_pel16x16 = EB_FALSE;
                    enable_half_pel8x8 = EB_FALSE;
                    enable_quarter_pel = EB_FALSE;

                }

                if (enable_half_pel32x32 || enable_half_pel16x16 || enable_half_pel8x8 || enable_quarter_pel) {

                    // Move to the top left of the search region
                    x_top_left_search_region = (int16_t)(ref_pic_ptr->origin_x + sb_origin_x) + x_search_area_origin;
                    y_top_left_search_region = (int16_t)(ref_pic_ptr->origin_y + sb_origin_y) + y_search_area_origin;
                    search_region_index = x_top_left_search_region + y_top_left_search_region * ref_pic_ptr->stride_y;

                    // Interpolate the search region for Half-Pel Refinements
                    // H - AVC Style
                    interpolate_search_region_avc(
                        context_ptr,
                        list_index,
                        context_ptr->integer_buffer_ptr[list_index][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * context_ptr->interpolated_full_stride[list_index][0]),
                        context_ptr->interpolated_full_stride[list_index][0],
                        (uint32_t)search_area_width + (MAX_SB_SIZE - 1),
                        (uint32_t)search_area_height + (MAX_SB_SIZE - 1));

                    // Half-Pel Refinement [8 search positions]
                    half_pel_search_sb(
                        sequence_control_set_ptr,
                        context_ptr,
                        context_ptr->integer_buffer_ptr[list_index][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * context_ptr->interpolated_full_stride[list_index][0]),
                        context_ptr->interpolated_full_stride[list_index][0],
                        &(context_ptr->posb_buffer[list_index][0][(ME_FILTER_TAP >> 1) * context_ptr->interpolated_stride]),
                        &(context_ptr->posh_buffer[list_index][0][1]),
                        &(context_ptr->posj_buffer[list_index][0][0]),
                        x_search_area_origin,
                        y_search_area_origin,
                        picture_control_set_ptr->cu8x8_mode == CU_8x8_MODE_1,
                        enable_half_pel32x32,
                        enable_half_pel16x16 && picture_control_set_ptr->cu16x16_mode == CU_16x16_MODE_0,
                        enable_half_pel8x8);

                    // Quarter-Pel Refinement [8 search positions]
                    quarter_pel_search_sb(
                        context_ptr,
                        context_ptr->integer_buffer_ptr[list_index][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * context_ptr->interpolated_full_stride[list_index][0]),
                        context_ptr->interpolated_full_stride[list_index][0],
                        &(context_ptr->posb_buffer[list_index][0][(ME_FILTER_TAP >> 1) * context_ptr->interpolated_stride]),  //points to b position of the figure above
                        &(context_ptr->posh_buffer[list_index][0][1]),                                                      //points to h position of the figure above
                        &(context_ptr->posj_buffer[list_index][0][0]),                                                      //points to j position of the figure above
                        x_search_area_origin,
                        y_search_area_origin,
                        picture_control_set_ptr->cu8x8_mode == CU_8x8_MODE_1,
                        enable_half_pel32x32,
                        enable_half_pel16x16 && picture_control_set_ptr->cu16x16_mode == CU_16x16_MODE_0,
                        enable_half_pel8x8,
                        enable_quarter_pel);
                }
            }
        }
    }

    // Bi-Prediction motion estimation loop
    for (pu_index = 0; pu_index < max_number_of_pus_per_sb; ++pu_index) {

        candidate_index = 0;
        uint32_t n_idx = pu_index > 20 ? eb_vp9_tab8x8[pu_index - 21] + 21 :
            pu_index > 4 ? eb_vp9_tab32x32[pu_index - 5] + 5 : pu_index;

        for (list_index = REF_LIST_0; list_index <= num_of_list_to_search; ++list_index) {
            candidate_index++;
        }

        total_me_candidate_index = candidate_index;

        if (num_of_list_to_search) {

            EB_BOOL condition = (EB_BOOL)((picture_control_set_ptr->cu8x8_mode == CU_8x8_MODE_0 || pu_index < 21) && (picture_control_set_ptr->cu16x16_mode == CU_16x16_MODE_0 || pu_index < 5));

            if (condition) {

                bi_prediction_search(
                    context_ptr,
                    pu_index,
                    candidate_index,
                    1,
                    1,
                    &total_me_candidate_index,
                    picture_control_set_ptr);
            }
        }

        MeCuResults * me_pu_result = &picture_control_set_ptr->me_results[lblock_index][pu_index];
        me_pu_result->total_me_candidate_index = total_me_candidate_index;

        if (total_me_candidate_index == 3) {

            uint32_t l0_sad = context_ptr->p_sb_best_sad[0][0][n_idx];
            uint32_t l1_sad = context_ptr->p_sb_best_sad[1][0][n_idx];
            uint32_t bi_sad = context_ptr->p_sb_bipred_sad[n_idx];

            me_pu_result->x_mv_l0 = _MVXT(context_ptr->p_sb_best_mv[0][0][n_idx]);
            me_pu_result->y_mv_l0 = _MVYT(context_ptr->p_sb_best_mv[0][0][n_idx]);
            me_pu_result->x_mv_l1 = _MVXT(context_ptr->p_sb_best_mv[1][0][n_idx]);
            me_pu_result->y_mv_l1 = _MVYT(context_ptr->p_sb_best_mv[1][0][n_idx]);

            uint32_t order = sort3_elements(l0_sad, l1_sad, bi_sad);

            switch (order) {
                // a = l0_sad, b= l1_sad, c= bi_sad
            case a_b_c:

                NSET_CAND(me_pu_result, 0, l0_sad, UNI_PRED_LIST_0)
                    NSET_CAND(me_pu_result, 1, l1_sad, UNI_PRED_LIST_1)
                    NSET_CAND(me_pu_result, 2, bi_sad, BI_PRED)
                    break;

            case a_c_b:

                NSET_CAND(me_pu_result, 0, l0_sad, UNI_PRED_LIST_0)
                    NSET_CAND(me_pu_result, 1, bi_sad, BI_PRED)
                    NSET_CAND(me_pu_result, 2, l1_sad, UNI_PRED_LIST_1)
                    break;

            case b_a_c:

                NSET_CAND(me_pu_result, 0, l1_sad, UNI_PRED_LIST_1)
                    NSET_CAND(me_pu_result, 1, l0_sad, UNI_PRED_LIST_0)
                    NSET_CAND(me_pu_result, 2, bi_sad, BI_PRED)
                    break;

            case b_c_a:

                NSET_CAND(me_pu_result, 0, l1_sad, UNI_PRED_LIST_1)
                    NSET_CAND(me_pu_result, 1, bi_sad, BI_PRED)
                    NSET_CAND(me_pu_result, 2, l0_sad, UNI_PRED_LIST_0)
                    break;

            case c_a_b:

                NSET_CAND(me_pu_result, 0, bi_sad, BI_PRED)
                    NSET_CAND(me_pu_result, 1, l0_sad, UNI_PRED_LIST_0)
                    NSET_CAND(me_pu_result, 2, l1_sad, UNI_PRED_LIST_1)
                    break;

            case c_b_a:

                NSET_CAND(me_pu_result, 0, bi_sad, BI_PRED)
                    NSET_CAND(me_pu_result, 1, l1_sad, UNI_PRED_LIST_1)
                    NSET_CAND(me_pu_result, 2, l0_sad, UNI_PRED_LIST_0)
                    break;

            default:
                SVT_LOG("Err in sorting");
                break;
            }

        }
        else if (total_me_candidate_index == 2) {

            uint32_t l0_sad = context_ptr->p_sb_best_sad[0][0][n_idx];
            uint32_t l1_sad = context_ptr->p_sb_best_sad[1][0][n_idx];

            me_pu_result->x_mv_l0 = _MVXT(context_ptr->p_sb_best_mv[0][0][n_idx]);
            me_pu_result->y_mv_l0 = _MVYT(context_ptr->p_sb_best_mv[0][0][n_idx]);
            me_pu_result->x_mv_l1 = _MVXT(context_ptr->p_sb_best_mv[1][0][n_idx]);
            me_pu_result->y_mv_l1 = _MVYT(context_ptr->p_sb_best_mv[1][0][n_idx]);

            if (l0_sad <= l1_sad) {
                NSET_CAND(me_pu_result, 0, l0_sad, UNI_PRED_LIST_0)
                    NSET_CAND(me_pu_result, 1, l1_sad, UNI_PRED_LIST_1)
            }
            else {
                NSET_CAND(me_pu_result, 0, l1_sad, UNI_PRED_LIST_1)
                    NSET_CAND(me_pu_result, 1, l0_sad, UNI_PRED_LIST_0)
            }

        }
        else {
            uint32_t l0_sad = context_ptr->p_sb_best_sad[0][0][n_idx];
            me_pu_result->x_mv_l0 = _MVXT(context_ptr->p_sb_best_mv[0][0][n_idx]);
            me_pu_result->y_mv_l0 = _MVYT(context_ptr->p_sb_best_mv[0][0][n_idx]);
            me_pu_result->x_mv_l1 = _MVXT(context_ptr->p_sb_best_mv[1][0][n_idx]);
            me_pu_result->y_mv_l1 = _MVYT(context_ptr->p_sb_best_mv[1][0][n_idx]);
            NSET_CAND(me_pu_result, 0, l0_sad, UNI_PRED_LIST_0)
        }

    }

    if (sequence_control_set_ptr->static_config.rate_control_mode) {

        // Compute the sum of the distortion of all 16 16x16 (best) blocks in the LCU
        picture_control_set_ptr->rcme_distortion[lblock_index] = 0;
        for (i = 0; i < 16; i++) {
            picture_control_set_ptr->rcme_distortion[lblock_index] += picture_control_set_ptr->me_results[lblock_index][5 + i].distortion_direction[0].distortion;
        }

    }

    return return_error;
}
