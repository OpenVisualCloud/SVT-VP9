/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbCombinedAveragingSAD_AVX512_h
#define EbCombinedAveragingSAD_AVX512_h

#include "EbDefinitions.h"
#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

uint64_t eb_vp9_compute_mean8x8_avx2_intrin(
    uint8_t   *input_samples,       // input parameter, input samples Ptr
    uint32_t   input_stride,        // input parameter, input stride
    uint32_t   input_area_width,    // input parameter, input area width
    uint32_t   input_area_height);

void eb_vp9_compute_interm_var_four8x8_avx2_intrin(
    uint8_t   *input_samples,
    uint16_t   input_stride,
    uint64_t  *mean_of8x8_blocks,      // mean of four  8x8
    uint64_t  *mean_of_squared8x8_blocks);

#ifndef DISABLE_AVX512
void bi_pred_average_kernel_avx512_intrin(
    EbByte    src0,
    uint32_t  src0_stride,
    EbByte    src1,
    uint32_t  src1_stride,
    EbByte    dst,
    uint32_t  dst_stride,
    uint32_t  area_width,
    uint32_t  area_height);
#else
void bi_pred_average_kernel_avx2_intrin(
    EbByte    src0,
    uint32_t  src0_stride,
    EbByte    src1,
    uint32_t  src1_stride,
    EbByte    dst,
    uint32_t  dst_stride,
    uint32_t  area_width,
    uint32_t  area_height);
#endif

#ifdef __cplusplus
}
#endif
#endif
