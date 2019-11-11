/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbComputeSAD_C.h"
#include "EbUtility.h"
#include "EbDefinitions.h"

/*******************************************
* eb_vp9_combined_averaging_sad
*
*******************************************/
uint32_t eb_vp9_combined_averaging_sad(
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
    uint32_t sad = 0;
    uint8_t avgpel;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            avgpel = (ref1[x] + ref2[x] + 1) >> 1;
            sad += EB_ABS_DIFF(src[x], avgpel);
        }
        src += src_stride;
        ref1 += ref1_stride;
        ref2 += ref2_stride;
    }

    return sad;
}

/*******************************************
* Compute8x4SAD_Default
*   Unoptimized 8x4 SAD
*******************************************/
uint32_t compute8x4_sad_kernel(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                      // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride)                      // input parameter, reference stride
{
    uint32_t row_number_in_block8x4;
    uint32_t sad_block8x4 = 0;

    for (row_number_in_block8x4 = 0; row_number_in_block8x4 < 4; ++row_number_in_block8x4) {
        sad_block8x4 += EB_ABS_DIFF(src[0x00], ref[0x00]);
        sad_block8x4 += EB_ABS_DIFF(src[0x01], ref[0x01]);
        sad_block8x4 += EB_ABS_DIFF(src[0x02], ref[0x02]);
        sad_block8x4 += EB_ABS_DIFF(src[0x03], ref[0x03]);
        sad_block8x4 += EB_ABS_DIFF(src[0x04], ref[0x04]);
        sad_block8x4 += EB_ABS_DIFF(src[0x05], ref[0x05]);
        sad_block8x4 += EB_ABS_DIFF(src[0x06], ref[0x06]);
        sad_block8x4 += EB_ABS_DIFF(src[0x07], ref[0x07]);
        src += src_stride;
        ref += ref_stride;
    }

    return sad_block8x4;
}

void eb_vp9_sad_loop_kernel_sparse(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                      // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                      // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width,                          // input parameter, block width (N)
    uint64_t *best_sad,
    int16_t  *x_search_center,
    int16_t  *y_search_center,
    uint32_t  src_stride_raw,                   // input parameter, source stride (no line skipping)
    int16_t  search_area_width,
    int16_t  search_area_height)
{
    int16_t x_search_index;
    int16_t y_search_index;

    *best_sad = 0xffffff;

    for (y_search_index = 0; y_search_index < search_area_height; y_search_index++)
    {

        for (x_search_index = 0; x_search_index < search_area_width; x_search_index++)
        {

            uint8_t doThisPoint = 0;
            uint32_t group = (x_search_index / 8);
            if ((group & 1) == (y_search_index & 1))
                doThisPoint = 1;

            if (doThisPoint) {
                uint32_t x, y;
                uint32_t sad = 0;

                for (y = 0; y < height; y++)
                {
                    for (x = 0; x < width; x++)
                    {
                        sad += EB_ABS_DIFF(src[y*src_stride + x], ref[x_search_index + y*ref_stride + x]);
                    }

                }

                // Update results
                if (sad < *best_sad)
                {
                    *best_sad = sad;
                    *x_search_center = x_search_index;
                    *y_search_center = y_search_index;
                }
            }

        }

        ref += src_stride_raw;
    }

    return;
}

/*******************************************
*   returns NxM Sum of Absolute Differences
Note: moved from picture operators.
keep this function here for profiling
issues.
*******************************************/
uint32_t eb_vp9_fast_loop_nx_m_sad_kernel(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                      // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                      // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width)                          // input parameter, block width (N)
{
    uint32_t x, y;
    uint32_t sad = 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            sad += EB_ABS_DIFF(src[x], ref[x]);
        }
        src += src_stride;
        ref += ref_stride;
    }

    return sad;
}

void eb_vp9_sad_loop_kernel(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                      // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                      // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width,                          // input parameter, block width (N)
    uint64_t *best_sad,
    int16_t  *x_search_center,
    int16_t  *y_search_center,
    uint32_t  src_stride_raw,                   // input parameter, source stride (no line skipping)
    int16_t  search_area_width,
    int16_t  search_area_height)
{
    int16_t x_search_index;
    int16_t y_search_index;

    *best_sad = 0xffffff;

    for (y_search_index = 0; y_search_index < search_area_height; y_search_index++)
    {
        for (x_search_index = 0; x_search_index < search_area_width; x_search_index++)
        {
            uint32_t x, y;
            uint32_t sad = 0;

            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; x++)
                {
                    sad += EB_ABS_DIFF(src[y*src_stride + x], ref[x_search_index + y*ref_stride + x]);
                }

            }

            // Update results
            if (sad < *best_sad)
            {
                *best_sad = sad;
                *x_search_center = x_search_index;
                *y_search_center = y_search_index;
            }
        }

        ref += src_stride_raw;
    }

    return;
}

//compute a 8x4 SAD
static uint32_t Subsad8x8(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                      // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride)                     // input parameter, reference stride
{
    uint32_t x, y;
    uint32_t sad_block8x4 = 0;

    src_stride = src_stride * 2;
    ref_stride = ref_stride * 2;

    for (y = 0; y < 4; y++)
    {
        for (x = 0; x < 8; x++)
        {
            sad_block8x4 += EB_ABS_DIFF(src[y*src_stride + x], ref[y*ref_stride + x]);
        }

    }

    return sad_block8x4;
}

/*******************************************
* eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu
*******************************************/
void eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu(
    uint8_t   *src,
    uint32_t   src_stride,
    uint8_t   *ref,
    uint32_t   ref_stride,
    uint32_t  *p_best_sad8x8,
    uint32_t  *p_best_mv8x8,
    uint32_t  *p_best_sad16x16,
    uint32_t  *p_best_mv16x16,
    uint32_t   mv,
    uint16_t  *p_sad16x16)
{
    uint32_t x_search_index;
    int16_t x_mv, y_mv;
    uint32_t sad8x8_0, sad8x8_1, sad8x8_2, sad8x8_3;
    uint16_t sad16x16;

    /*
    -------------------------------------   -----------------------------------
    | 8x8_00 | 8x8_01 | 8x8_04 | 8x8_05 |   8x8_16 | 8x8_17 | 8x8_20 | 8x8_21 |
    -------------------------------------   -----------------------------------
    | 8x8_02 | 8x8_03 | 8x8_06 | 8x8_07 |   8x8_18 | 8x8_19 | 8x8_22 | 8x8_23 |
    -----------------------   -----------   ----------------------   ----------
    | 8x8_08 | 8x8_09 | 8x8_12 | 8x8_13 |   8x8_24 | 8x8_25 | 8x8_29 | 8x8_29 |
    ----------------------    -----------   ---------------------    ----------
    | 8x8_10 | 8x8_11 | 8x8_14 | 8x8_15 |   8x8_26 | 8x8_27 | 8x8_30 | 8x8_31 |
    -------------------------------------   -----------------------------------

    -------------------------------------   -----------------------------------
    | 8x8_32 | 8x8_33 | 8x8_36 | 8x8_37 |   8x8_48 | 8x8_49 | 8x8_52 | 8x8_53 |
    -------------------------------------   -----------------------------------
    | 8x8_34 | 8x8_35 | 8x8_38 | 8x8_39 |   8x8_50 | 8x8_51 | 8x8_54 | 8x8_55 |
    -----------------------   -----------   ----------------------   ----------
    | 8x8_40 | 8x8_41 | 8x8_44 | 8x8_45 |   8x8_56 | 8x8_57 | 8x8_60 | 8x8_61 |
    ----------------------    -----------   ---------------------    ----------
    | 8x8_42 | 8x8_43 | 8x8_46 | 8x8_48 |   8x8_58 | 8x8_59 | 8x8_62 | 8x8_63 |
    -------------------------------------   -----------------------------------
    */

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

    for (x_search_index = 0; x_search_index < 8; x_search_index++)
    {

        //8x8_0
        sad8x8_0 = Subsad8x8(src, src_stride, ref + x_search_index, ref_stride);
        if (2 * sad8x8_0 < p_best_sad8x8[0]) {
            p_best_sad8x8[0] = 2 * sad8x8_0;
            x_mv = _MVXT(mv) + (int16_t)x_search_index * 4;
            y_mv = _MVYT(mv);
            p_best_mv8x8[0] = ((uint16_t)y_mv << 16) | ((uint16_t)x_mv);
        }

        //8x8_1
        sad8x8_1 = Subsad8x8(src + 8, src_stride, ref + x_search_index + 8, ref_stride);
        if (2 * sad8x8_1 < p_best_sad8x8[1]) {
            p_best_sad8x8[1] = 2 * sad8x8_1;
            x_mv = _MVXT(mv) + (int16_t)x_search_index * 4;
            y_mv = _MVYT(mv);
            p_best_mv8x8[1] = ((uint16_t)y_mv << 16) | ((uint16_t)x_mv);
        }

        //8x8_2
        sad8x8_2 = Subsad8x8(src + 8 * src_stride, src_stride, ref + x_search_index + 8 * ref_stride, ref_stride);
        if (2 * sad8x8_2 < p_best_sad8x8[2]) {
            p_best_sad8x8[2] = 2 * sad8x8_2;
            x_mv = _MVXT(mv) + (int16_t)x_search_index * 4;
            y_mv = _MVYT(mv);
            p_best_mv8x8[2] = ((uint16_t)y_mv << 16) | ((uint16_t)x_mv);
        }

        //8x8_3
        sad8x8_3 = Subsad8x8(src + 8 + 8 * src_stride, src_stride, ref + 8 + 8 * ref_stride + x_search_index, ref_stride);
        if (2 * sad8x8_3 < p_best_sad8x8[3]) {
            p_best_sad8x8[3] = 2 * sad8x8_3;
            x_mv = _MVXT(mv) + (int16_t)x_search_index * 4;
            y_mv = _MVYT(mv);
            p_best_mv8x8[3] = ((uint16_t)y_mv << 16) | ((uint16_t)x_mv);
        }

        //16x16
        sad16x16 = (uint16_t)(sad8x8_0 + sad8x8_1 + sad8x8_2 + sad8x8_3);
        p_sad16x16[x_search_index] = sad16x16;  //store the intermediate 16x16 SAD for 32x32 in subsampled form.
        if ((uint32_t)(2 * sad16x16) < p_best_sad16x16[0]) {
            p_best_sad16x16[0] = 2 * sad16x16;
            x_mv = _MVXT(mv) + (int16_t)x_search_index * 4;
            y_mv = _MVYT(mv);
            p_best_mv16x16[0] = ((uint16_t)y_mv << 16) | ((uint16_t)x_mv);
        }

    }
}

/*******************************************
Calcualte SAD for 32x32,64x64 from 16x16
and check if there is improvement, if yes keep
the best SAD+MV
*******************************************/
void eb_vp9_get_eight_horizontal_search_point_results_32x32_64x64(
    uint16_t  *p_sad16x16,
    uint32_t  *p_best_sad32x32,
    uint32_t  *p_best_sad64x64,
    uint32_t  *p_best_mv32x32,
    uint32_t  *p_best_mv64x64,
    uint32_t   mv)
{
    int16_t x_mv, y_mv;
    uint32_t sad32x32_0, sad32x32_1, sad32x32_2, sad32x32_3, sad64x64;
    uint32_t x_search_index;

    /*--------------------
    |  32x32_0  |  32x32_1
    ----------------------
    |  32x32_2  |  32x32_3
    ----------------------*/

    /*  data ordering in p_sad16x16 buffer

    Search    Search            Search
    Point 0   Point 1           Point 7
    ---------------------------------------
    16x16_0    |    x    |    x    | ...... |    x    |
    ---------------------------------------
    16x16_1    |    x    |    x    | ...... |    x    |

    16x16_n    |    x    |    x    | ...... |    x    |

    ---------------------------------------
    16x16_15   |    x    |    x    | ...... |    x    |
    ---------------------------------------
    */

    for (x_search_index = 0; x_search_index < 8; x_search_index++)
    {

        //32x32_0
        sad32x32_0 = p_sad16x16[0 * 8 + x_search_index] + p_sad16x16[1 * 8 + x_search_index] + p_sad16x16[2 * 8 + x_search_index] + p_sad16x16[3 * 8 + x_search_index];

        if (2 * sad32x32_0 < p_best_sad32x32[0]){
            p_best_sad32x32[0] = 2 * sad32x32_0;
            x_mv = _MVXT(mv) + (int16_t)x_search_index * 4;
            y_mv = _MVYT(mv);
            p_best_mv32x32[0] = ((uint16_t)y_mv << 16) | ((uint16_t)x_mv);
        }

        //32x32_1
        sad32x32_1 = p_sad16x16[4 * 8 + x_search_index] + p_sad16x16[5 * 8 + x_search_index] + p_sad16x16[6 * 8 + x_search_index] + p_sad16x16[7 * 8 + x_search_index];

        if (2 * sad32x32_1 < p_best_sad32x32[1]){
            p_best_sad32x32[1] = 2 * sad32x32_1;
            x_mv = _MVXT(mv) + (int16_t)x_search_index * 4;
            y_mv = _MVYT(mv);
            p_best_mv32x32[1] = ((uint16_t)y_mv << 16) | ((uint16_t)x_mv);
        }

        //32x32_2
        sad32x32_2 = p_sad16x16[8 * 8 + x_search_index] + p_sad16x16[9 * 8 + x_search_index] + p_sad16x16[10 * 8 + x_search_index] + p_sad16x16[11 * 8 + x_search_index];

        if (2 * sad32x32_2 < p_best_sad32x32[2]){
            p_best_sad32x32[2] = 2 * sad32x32_2;
            x_mv = _MVXT(mv) + (int16_t)x_search_index * 4;
            y_mv = _MVYT(mv);
            p_best_mv32x32[2] = ((uint16_t)y_mv << 16) | ((uint16_t)x_mv);
        }

        //32x32_3
        sad32x32_3 = p_sad16x16[12 * 8 + x_search_index] + p_sad16x16[13 * 8 + x_search_index] + p_sad16x16[14 * 8 + x_search_index] + p_sad16x16[15 * 8 + x_search_index];

        if (2 * sad32x32_3 < p_best_sad32x32[3]){
            p_best_sad32x32[3] = 2 * sad32x32_3;
            x_mv = _MVXT(mv) + (int16_t)x_search_index * 4;
            y_mv = _MVYT(mv);
            p_best_mv32x32[3] = ((uint16_t)y_mv << 16) | ((uint16_t)x_mv);
        }

        //64x64
        sad64x64 = sad32x32_0 + sad32x32_1 + sad32x32_2 + sad32x32_3;
        if (2 * sad64x64 < p_best_sad64x64[0]){
            p_best_sad64x64[0] = 2 * sad64x64;
            x_mv = _MVXT(mv) + (int16_t)x_search_index * 4;
            y_mv = _MVYT(mv);
            p_best_mv64x64[0] = ((uint16_t)y_mv << 16) | ((uint16_t)x_mv);

        }

    }

}
