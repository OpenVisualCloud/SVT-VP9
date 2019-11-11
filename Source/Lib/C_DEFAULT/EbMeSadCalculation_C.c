/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbMeSadCalculation_C.h"
#include "EbComputeSAD_C.h"
#include "EbDefinitions.h"
#include "EbUtility.h"

/*******************************************
Calcualte SAD for 16x16 and its 8x8 sublcoks
and check if there is improvment, if yes keep
the best SAD+MV
*******************************************/
void eb_vp9_sad_calculation_8x8_16x16(
    uint8_t   *src,
    uint32_t   src_stride,
    uint8_t   *ref,
    uint32_t   ref_stride,
    uint32_t  *p_best_sad8x8,
    uint32_t  *p_best_sad16x16,
    uint32_t  *p_best_mv8x8,
    uint32_t  *p_best_mv16x16,
    uint32_t   mv,
    uint32_t  *p_sad16x16)
{
    uint64_t sad8x8_0, sad8x8_1, sad8x8_2, sad8x8_3;
    uint64_t sad16x16;

    uint32_t   src_stride_sub = (src_stride << 1); //TODO get these from outside
    uint32_t   ref_stride_sub = (ref_stride << 1);

    sad8x8_0 = (compute8x4_sad_kernel(src, src_stride_sub, ref, ref_stride_sub)) << 1;
    if (sad8x8_0 < p_best_sad8x8[0]) {
        p_best_sad8x8[0] = (uint32_t)sad8x8_0;
        p_best_mv8x8[0] = mv;
    }

    sad8x8_1 = (compute8x4_sad_kernel(src + 8, src_stride_sub, ref + 8, ref_stride_sub)) << 1;
    if (sad8x8_1 < p_best_sad8x8[1]){
        p_best_sad8x8[1] = (uint32_t)sad8x8_1;
        p_best_mv8x8[1] = mv;
    }

    sad8x8_2 = (compute8x4_sad_kernel(src + (src_stride << 3), src_stride_sub, ref + (ref_stride << 3), ref_stride_sub)) << 1;
    if (sad8x8_2 < p_best_sad8x8[2]){
        p_best_sad8x8[2] = (uint32_t)sad8x8_2;
        p_best_mv8x8[2] = mv;
    }

    sad8x8_3 = (compute8x4_sad_kernel(src + (src_stride << 3) + 8, src_stride_sub, ref + (ref_stride << 3) + 8, ref_stride_sub)) << 1;
    if (sad8x8_3 < p_best_sad8x8[3]){
        p_best_sad8x8[3] = (uint32_t)sad8x8_3;
        p_best_mv8x8[3] = mv;
    }

    sad16x16 = sad8x8_0 + sad8x8_1 + sad8x8_2 + sad8x8_3;
    if (sad16x16 < p_best_sad16x16[0]){
        p_best_sad16x16[0] = (uint32_t)sad16x16;
        p_best_mv16x16[0] = mv;
    }

    *p_sad16x16 = (uint32_t)sad16x16;
}

/*******************************************
Calcualte SAD for 32x32,64x64 from 16x16
and check if there is improvment, if yes keep
the best SAD+MV
*******************************************/
void eb_vp9_sad_calculation_32x32_64x64(
    uint32_t  *p_sad16x16,
    uint32_t  *p_best_sad32x32,
    uint32_t  *p_best_sad64x64,
    uint32_t  *p_best_mv32x32,
    uint32_t  *p_best_mv64x64,
    uint32_t   mv)
{

    uint32_t sad32x32_0, sad32x32_1, sad32x32_2, sad32x32_3, sad64x64;

    sad32x32_0 = p_sad16x16[0] + p_sad16x16[1] + p_sad16x16[2] + p_sad16x16[3];
    if (sad32x32_0<p_best_sad32x32[0]){
        p_best_sad32x32[0] = sad32x32_0;
        p_best_mv32x32[0] = mv;
    }

    sad32x32_1 = p_sad16x16[4] + p_sad16x16[5] + p_sad16x16[6] + p_sad16x16[7];
    if (sad32x32_1<p_best_sad32x32[1]){
        p_best_sad32x32[1] = sad32x32_1;
        p_best_mv32x32[1] = mv;
    }

    sad32x32_2 = p_sad16x16[8] + p_sad16x16[9] + p_sad16x16[10] + p_sad16x16[11];
    if (sad32x32_2<p_best_sad32x32[2]){
        p_best_sad32x32[2] = sad32x32_2;
        p_best_mv32x32[2] = mv;
    }

    sad32x32_3 = p_sad16x16[12] + p_sad16x16[13] + p_sad16x16[14] + p_sad16x16[15];
    if (sad32x32_3<p_best_sad32x32[3]){
        p_best_sad32x32[3] = sad32x32_3;
        p_best_mv32x32[3] = mv;
    }

    sad64x64 = sad32x32_0 + sad32x32_1 + sad32x32_2 + sad32x32_3;
    if (sad64x64<p_best_sad64x64[0]){
        p_best_sad64x64[0] = sad64x64;
        p_best_mv64x64[0] = mv;
    }
}

void eb_vp9_initialize_buffer_32bits(
    uint32_t *pointer,
    uint32_t  count128,
    uint32_t  count32,
    uint32_t  value) {

    uint32_t  block_index;

    for (block_index = 0; block_index < ((count128 << 2) + count32); block_index++) {

        pointer[block_index] = value;
    }
}

/*******************************************
* weight_search_region
*   Apply weight and offsets for reference
*   samples and sotre them in local buffer
*******************************************/
void weight_search_region(
    uint8_t  *input_buffer,
    uint32_t  input_stride,
    uint8_t  *dst_buffer,
    uint32_t  dst_stride,
    uint32_t  search_area_width,
    uint32_t  search_area_height,
    int16_t   wp_weight,
    int16_t   wp_offset,
    uint32_t  wp_luma_weight_denominator_shift)
{
    uint32_t y_search_index = 0;
    uint32_t x_search_index = 0;
    uint32_t input_region_index;
    uint32_t dst_region_index;
    int32_t wp_luma_weight_denominator_offset = (1 << (wp_luma_weight_denominator_shift)) >> 1;

    while (y_search_index < search_area_height) {
        while (x_search_index < search_area_width) {

            input_region_index = x_search_index + y_search_index * input_stride;
            dst_region_index = x_search_index + y_search_index * dst_stride;

            dst_buffer[dst_region_index] =
                (uint8_t)CLIP3(0, MAX_SAMPLE_VALUE, (((wp_weight * input_buffer[input_region_index]) + wp_luma_weight_denominator_offset) >> wp_luma_weight_denominator_shift) + wp_offset);

            // Next x position
            x_search_index++;
        }
        // Intilize x position to 0
        x_search_index = 0;
        // Next y position
        y_search_index++;
    }
}
