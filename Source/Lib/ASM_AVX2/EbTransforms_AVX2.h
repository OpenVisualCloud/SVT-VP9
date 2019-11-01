/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTransforms_AVX2_h
#define EbTransforms_AVX2_h

#include "immintrin.h"

#include "EbDefinitions.h"
#include "vpx_dsp_rtcd.h"
#include "txfm_common.h"
#include "EbMemory_AVX2.h"
#ifdef __cplusplus
extern "C" {
#endif

    static INLINE __m256i dct_const_round_shift_avx2(const __m256i in) {
        const __m256i t =
            _mm256_add_epi32(in, _mm256_set1_epi32(DCT_CONST_ROUNDING));
        return _mm256_srai_epi32(t, DCT_CONST_BITS);
    }

    static INLINE __m256i idct_madd_round_shift_avx2(const __m256i in,
        const __m256i cospi) {
        const __m256i t = _mm256_madd_epi16(in, cospi);
        return dct_const_round_shift_avx2(t);
    }

    // Calculate the dot product between in0/1 and x and wrap to short.
    static INLINE __m256i idct_calc_wraplow_avx2(const __m256i in0,
        const __m256i in1, const __m256i x) {
        const __m256i t0 = idct_madd_round_shift_avx2(in0, x);
        const __m256i t1 = idct_madd_round_shift_avx2(in1, x);
        return _mm256_packs_epi32(t0, t1);
    }

    // Multiply elements by constants and add them together.
    static INLINE void butterfly_avx2(const __m256i in0, const __m256i in1,
        const int c0, const int c1, __m256i *const out0,
        __m256i *const out1) {
        const __m256i cst0 = pair_set_epi16_avx2(c0, -c1);
        const __m256i cst1 = pair_set_epi16_avx2(c1, c0);
        const __m256i lo = _mm256_unpacklo_epi16(in0, in1);
        const __m256i hi = _mm256_unpackhi_epi16(in0, in1);
        *out0 = idct_calc_wraplow_avx2(lo, hi, cst0);
        *out1 = idct_calc_wraplow_avx2(lo, hi, cst1);
    }

#ifdef __cplusplus
}
#endif
#endif // EbTransforms_AVX2_h
