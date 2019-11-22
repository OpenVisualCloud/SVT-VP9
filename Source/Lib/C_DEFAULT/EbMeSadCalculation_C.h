/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMeSadCalculation_C_h
#define EbMeSadCalculation_C_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

extern void eb_vp9_sad_calculation_8x8_16x16(
    uint8_t   *src,
    uint32_t   src_stride,
    uint8_t   *ref,
    uint32_t   ref_stride,
    uint32_t  *p_best_sad8x8,
    uint32_t  *p_best_sad16x16,
    uint32_t  *p_best_mv8x8,
    uint32_t  *p_best_mv16x16,
    uint32_t   mv,
    uint32_t  *p_sad16x16);

extern void eb_vp9_sad_calculation_32x32_64x64(
    uint32_t  *p_sad16x16,
    uint32_t  *p_best_sad32x32,
    uint32_t  *p_best_sad64x64,
    uint32_t  *p_best_mv32x32,
    uint32_t  *p_best_mv64x64,
    uint32_t   mv);

extern void eb_vp9_initialize_buffer_32bits(
    uint32_t  *pointer,
    uint32_t   count128,
    uint32_t   count32,
    uint32_t   value);

void weight_search_region(
    uint8_t   *input_buffer,
    uint32_t   input_stride,
    uint8_t   *dst_buffer,
    uint32_t   dst_stride,
    uint32_t   search_area_width,
    uint32_t   search_area_height,
    int16_t    wp_weight,
    int16_t    wp_offset,
    uint32_t   wp_luma_weight_denominator_shift);

#ifdef __cplusplus
}
#endif
#endif // EbMeSadCalculation_C_h
