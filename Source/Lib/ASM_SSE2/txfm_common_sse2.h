/*
 *  Copyright (c) 2015 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VPX_DSP_X86_TXFM_COMMON_SSE2_H_
#define VPX_VPX_DSP_X86_TXFM_COMMON_SSE2_H_

#include <emmintrin.h>
 //#include "vpx/vpx_integer.h"

#define pair_set_epi16(a, b) \
  _mm_set1_epi32((int)((uint16_t)(a) | ((uint32_t)(b) << 16) ))

#define pair_set_epi32(a, b) \
  _mm_set_epi32((int)(b), (int)(a), (int)(b), (int)(a))

#define dual_set_epi16(a, b)                                            \
  _mm_set_epi16((int16_t)(b), (int16_t)(b), (int16_t)(b), (int16_t)(b), \
                (int16_t)(a), (int16_t)(a), (int16_t)(a), (int16_t)(a))

#define octa_set_epi16(a, b, c, d, e, f, g, h)                           \
  _mm_setr_epi16((int16_t)(a), (int16_t)(b), (int16_t)(c), (int16_t)(d), \
                 (int16_t)(e), (int16_t)(f), (int16_t)(g), (int16_t)(h))

static INLINE __m128i dct_const_round_shift_sse2(const __m128i in) {
    const __m128i t = _mm_add_epi32(in, _mm_set1_epi32(DCT_CONST_ROUNDING));
    return _mm_srai_epi32(t, DCT_CONST_BITS);
}

static INLINE __m128i idct_madd_round_shift_sse2(const __m128i in,
    const __m128i cospi) {
    const __m128i t = _mm_madd_epi16(in, cospi);
    return dct_const_round_shift_sse2(t);
}

// Calculate the dot product between in0/1 and x and wrap to short.
static INLINE __m128i idct_calc_wraplow_sse2(const __m128i in0,
    const __m128i in1,
    const __m128i x) {
    const __m128i t0 = idct_madd_round_shift_sse2(in0, x);
    const __m128i t1 = idct_madd_round_shift_sse2(in1, x);
    return _mm_packs_epi32(t0, t1);
}

// Multiply elements by constants and add them together.
static INLINE void butterfly(const __m128i in0, const __m128i in1, const int c0,
    const int c1, __m128i *const out0,
    __m128i *const out1) {
    const __m128i cst0 = pair_set_epi16(c0, -c1);
    const __m128i cst1 = pair_set_epi16(c1, c0);
    const __m128i lo = _mm_unpacklo_epi16(in0, in1);
    const __m128i hi = _mm_unpackhi_epi16(in0, in1);
    *out0 = idct_calc_wraplow_sse2(lo, hi, cst0);
    *out1 = idct_calc_wraplow_sse2(lo, hi, cst1);
}

static INLINE void recon_and_store_16(const __m128i in0, const __m128i in1,
    uint8_t *const dest) {
    const __m128i zero = _mm_setzero_si128();
    const __m128i d = _mm_loadu_si128((__m128i *)dest);
    const __m128i d0 = _mm_unpacklo_epi8(d, zero);
    const __m128i d1 = _mm_unpackhi_epi8(d, zero);
    const __m128i d2 = _mm_add_epi16(in0, d0);
    const __m128i d3 = _mm_add_epi16(in1, d1);
    const __m128i dd = _mm_packus_epi16(d2, d3);
    _mm_storeu_si128((__m128i *)dest, dd);
}

#endif  // VPX_VPX_DSP_X86_TXFM_COMMON_SSE2_H_
