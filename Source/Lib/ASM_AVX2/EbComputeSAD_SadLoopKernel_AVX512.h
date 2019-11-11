/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbComputeSAD_AVX512_h
#define EbComputeSAD_AVX512_h

#include "EbComputeSAD_AVX2.h"
#include "EbMotionEstimation.h"
#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifndef DISABLE_AVX512
void eb_vp9_sad_loop_kernel_avx512_hme_l0_intrin(
    uint8_t  *src,                             // input parameter, source samples Ptr
    uint32_t  src_stride,                      // input parameter, source stride
    uint8_t  *ref,                             // input parameter, reference samples Ptr
    uint32_t  ref_stride,                      // input parameter, reference stride
    uint32_t  height,                          // input parameter, block height (M)
    uint32_t  width,                           // input parameter, block width (N)
    uint64_t *best_sad,
    int16_t  *x_search_center,
    int16_t  *y_search_center,
    uint32_t  src_stride_raw,                  // input parameter, source stride (no line skipping)
    int16_t   search_area_width,
    int16_t   search_area_height);
#else
    void eb_vp9_sad_loop_kernel_avx2_hme_l0_intrin(
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
#endif

#ifndef DISABLE_AVX512
extern void eb_vp9_get_eight_horizontal_search_point_results_all85_p_us_avx512_intrin(
        MeContext *context_ptr,
        uint32_t   list_index,
        uint32_t   search_region_index,
        uint32_t   x_search_index,
        uint32_t   y_search_index
    );
#endif

#ifdef __cplusplus
}
#endif
#endif // EbComputeSAD_AVX512_h
