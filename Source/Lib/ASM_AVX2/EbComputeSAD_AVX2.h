/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbComputeSAD_AVX2_h
#define EbComputeSAD_AVX2_h

#include "EbDefinitions.h"
#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

uint32_t compute4x_m_sad_avx2_intrin(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                     // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                     // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width);                         // input parameter, block width (N)

uint32_t compute8x_m_sad_avx2_intrin(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                     // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                     // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width);                         // input parameter, block width (N)

uint32_t compute16x_m_sad_avx2_intrin(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                     // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                     // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width);                         // input parameter, block width (N)

uint32_t compute24x_m_sad_avx2_intrin(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                     // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                     // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width);                         // input parameter, block width (N)

uint32_t compute32x_m_sad_avx2_intrin(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                     // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                     // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width);                         // input parameter, block width (N)

uint32_t compute48x_m_sad_avx2_intrin(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                     // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                     // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width);                         // input parameter, block width (N)

uint32_t compute64x_m_sad_avx2_intrin(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                     // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                     // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width);                         // input parameter, block width (N)

void eb_vp9_sad_loop_kernel_avx2_intrin(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                     // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                     // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width,                          // input parameter, block width (N)
    uint64_t *best_sad,
    int16_t  *x_search_center,
    int16_t  *y_search_center,
    uint32_t  src_stride_raw,                 // input parameter, source stride (no line skipping)
    int16_t   search_area_width,
    int16_t   search_area_height);

#ifdef DISABLE_AVX512
void eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu_avx2_intrin(
    uint8_t   *src,
    uint32_t   src_stride,
    uint8_t   *ref,
    uint32_t   ref_stride,
    uint32_t  *p_best_sad8x8,
    uint32_t  *p_best_mv8x8,
    uint32_t  *p_best_sad16x16,
    uint32_t  *p_best_mv16x16,
    uint32_t   mv,
    uint16_t  *p_sad16x16);

void eb_vp9_get_eight_horizontal_search_point_results_32x32_64x64_pu_avx2_intrin(
    uint16_t  *p_sad16x16,
    uint32_t  *p_best_sad32x32,
    uint32_t  *p_best_sad64x64,
    uint32_t  *p_best_mv32x32,
    uint32_t  *p_best_mv64x64,
    uint32_t   mv);
#endif

#ifdef __cplusplus
}
#endif
#endif // EbComputeSAD_AVX2_h
