/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbComputeSAD_h
#define EbComputeSAD_h

#include "EbDefinitions.h"

#include "EbCombinedAveragingSAD_Intrinsic_AVX2.h"
#include "EbComputeSAD_C.h"
#include "EbComputeSAD_SSE2.h"
#include "EbComputeSAD_SSE4_1.h"
#include "EbComputeSAD_AVX2.h"
#include "EbComputeSAD_SadLoopKernel_AVX512.h"

#include "EbUtility.h"

#ifdef __cplusplus
extern "C" {
#endif

    /***************************************
    * Function Ptr Types
    ***************************************/
    typedef uint32_t(*EbSadkernelNxMType)(
        uint8_t  *src,
        uint32_t  src_stride,
        uint8_t  *ref,
        uint32_t  ref_stride,
        uint32_t  height,
        uint32_t  width);

    static void nx_m_sad_kernel_void_func() {}

    typedef void(*EbSadloopkernelNxMType)(
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

    typedef uint32_t(*EbSadavgkernelNxMType)(
        uint8_t  *src,
        uint32_t  src_stride,
        uint8_t  *ref1,
        uint32_t  ref1_stride,
        uint8_t  *ref2,
        uint32_t  ref2_stride,
        uint32_t  height,
        uint32_t  width);

    typedef void(*EbGeteightsaD8x8)(
        uint8_t  *src,
        uint32_t  src_stride,
        uint8_t  *ref,
        uint32_t  ref_stride,
        uint32_t *p_best_sad8x8,
        uint32_t *p_best_mv8x8,
        uint32_t *p_best_sad16x16,
        uint32_t *p_best_mv16x16,
        uint32_t  mv,
        uint16_t *p_sad16x16);

    typedef void(*EbGeteightsaD32x32)(
        uint16_t *p_sad16x16,
        uint32_t *p_best_sad32x32,
        uint32_t *p_best_sad64x64,
        uint32_t *p_best_mv32x32,
        uint32_t *p_best_mv64x64,
        uint32_t  mv);

    /***************************************
    * Function Tables
    ***************************************/
    static EbSadkernelNxMType FUNC_TABLE n_x_m_sad_kernel_func_ptr_array[ASM_TYPE_TOTAL][9] =   // [eb_vp9_ASM_TYPES][SAD - block height]
    {
        // C_DEFAULT
        {
            /*0 4xM  */ eb_vp9_fast_loop_nx_m_sad_kernel,
            /*1 8xM  */ eb_vp9_fast_loop_nx_m_sad_kernel,
            /*2 16xM */ eb_vp9_fast_loop_nx_m_sad_kernel,
            /*3 24xM */ eb_vp9_fast_loop_nx_m_sad_kernel,
            /*4 32xM */ eb_vp9_fast_loop_nx_m_sad_kernel,
            /*5      */ eb_vp9_fast_loop_nx_m_sad_kernel,
            /*6 48xM */ eb_vp9_fast_loop_nx_m_sad_kernel,
            /*7      */ eb_vp9_fast_loop_nx_m_sad_kernel,
            /*8 64xM */ eb_vp9_fast_loop_nx_m_sad_kernel
        },
        // AVX2
        {
            /*0 4xM  */ compute4x_m_sad_avx2_intrin,
            /*1 8xM  */ compute8x_m_sad_avx2_intrin,
            /*2 16xM */ compute16x_m_sad_avx2_intrin,
            /*3 24xM */ compute24x_m_sad_avx2_intrin,
            /*4 32xM */ compute32x_m_sad_avx2_intrin,
            /*5      */ (EbSadkernelNxMType)nx_m_sad_kernel_void_func,
            /*6 48xM */ compute48x_m_sad_avx2_intrin,
            /*7      */ (EbSadkernelNxMType)nx_m_sad_kernel_void_func,
            /*8 64xM */ compute64x_m_sad_avx2_intrin,
        },
    };

    static EbSadavgkernelNxMType FUNC_TABLE nx_m_sad_averaging_kernel_func_ptr_array[ASM_TYPE_TOTAL][9] =   // [eb_vp9_ASM_TYPES][SAD - block height]
    {
        // C_DEFAULT
        {
            /*0 4xM  */ eb_vp9_combined_averaging_sad,
            /*1 8xM  */ eb_vp9_combined_averaging_sad,
            /*2 16xM */ eb_vp9_combined_averaging_sad,
            /*3 24xM */ eb_vp9_combined_averaging_sad,
            /*4 32xM */ eb_vp9_combined_averaging_sad,
            /*5      */ (EbSadavgkernelNxMType)nx_m_sad_kernel_void_func,
            /*6 48xM */ eb_vp9_combined_averaging_sad,
            /*7      */ (EbSadavgkernelNxMType)nx_m_sad_kernel_void_func,
            /*8 64xM */ eb_vp9_combined_averaging_sad
        },
        // AVX2
        {
            /*0 4xM  */ eb_vp9_combined_averaging4x_msad_sse2_intrin,
            /*1 8xM  */ eb_vp9_combined_averaging8x_msad_avx2_intrin,
            /*2 16xM */ eb_vp9_combined_averaging16x_msad_avx2_intrin,
            /*3 24xM */ eb_vp9_combined_averaging24x_msad_avx2_intrin,
            /*4 32xM */ eb_vp9_combined_averaging32x_msad_avx2_intrin,
            /*5      */ (EbSadavgkernelNxMType)nx_m_sad_kernel_void_func,
            /*6 48xM */ eb_vp9_combined_averaging48x_msad_avx2_intrin,
            /*7      */ (EbSadavgkernelNxMType)nx_m_sad_kernel_void_func,
            /*8 64xM */ eb_vp9_combined_averaging64x_msad_avx2_intrin
        },
    };

    static EbSadloopkernelNxMType FUNC_TABLE nx_m_sad_loop_kernel_func_ptr_array[ASM_TYPE_TOTAL] =
    {
        // C_DEFAULT
        eb_vp9_sad_loop_kernel,
        // AVX2
        eb_vp9_sad_loop_kernel_avx2_intrin,
    };

#ifdef DISABLE_AVX512
    static EbGeteightsaD8x8 FUNC_TABLE eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_func_ptr_array[ASM_TYPE_TOTAL] =
    {
        // C_DEFAULT
        eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu,
        // AVX2
        eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu_avx2_intrin,
    };

    static EbGeteightsaD32x32 FUNC_TABLE eb_vp9_get_eight_horizontal_search_point_results_32x32_64x64_func_ptr_array[ASM_TYPE_TOTAL] =
    {
        // C_DEFAULT
        eb_vp9_get_eight_horizontal_search_point_results_32x32_64x64,
        // AVX2
        eb_vp9_get_eight_horizontal_search_point_results_32x32_64x64_pu_avx2_intrin,
    };
#endif

#ifdef __cplusplus
}
#endif

#endif
