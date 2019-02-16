/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBAVCSTYLEMCP_SSSE3_H
#define EBAVCSTYLEMCP_SSSE3_H

#include "bitops.h"
#include "EbDefinitions.h"
#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

static EB_ALIGN(16) const int8_t avc_style_luma_if_coeff8_ssse3[] = {
    -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25,
     9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,
    -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18,
    18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2,
    -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9,
    25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1
};

static INLINE void avc_style_luma_interpolation_filter8(const __m128i ref0,
    const __m128i ref1, const __m128i ref2, const __m128i ref3,
    const __m128i if_offset, const __m128i IFCoeff_1_0,
    const __m128i if_coeff_3_2, EbByte const dst)
{
    const __m128i ref01 = _mm_unpacklo_epi8(ref0, ref1);
    const __m128i ref23 = _mm_unpacklo_epi8(ref2, ref3);
    const __m128i sum01 = _mm_maddubs_epi16(ref01, IFCoeff_1_0);
    const __m128i sum23 = _mm_maddubs_epi16(ref23, if_coeff_3_2);
    const __m128i sum0123 = _mm_add_epi16(sum01, sum23);
    const __m128i sum_Offset = _mm_add_epi16(sum0123, if_offset);
    const __m128i sum = _mm_srai_epi16(sum_Offset, 5);
    const __m128i sum_clip_U8 = _mm_packus_epi16(sum, sum);
    _mm_storel_epi64((__m128i *)dst, sum_clip_U8);
}

static INLINE void avc_style_luma_interpolation_filter16(const __m128i ref0,
    const __m128i ref1, const __m128i ref2, const __m128i ref3,
    const __m128i if_offset, const __m128i IFCoeff_1_0,
    const __m128i if_coeff_3_2, const EbByte dst)
{
    const __m128i ref01_lo = _mm_unpacklo_epi8(ref0, ref1);
    const __m128i ref01_hi = _mm_unpackhi_epi8(ref0, ref1);
    const __m128i ref23_lo = _mm_unpacklo_epi8(ref2, ref3);
    const __m128i ref23_hi = _mm_unpackhi_epi8(ref2, ref3);
    const __m128i sum01_lo = _mm_maddubs_epi16(ref01_lo, IFCoeff_1_0);
    const __m128i sum01_hi = _mm_maddubs_epi16(ref01_hi, IFCoeff_1_0);
    const __m128i sum23_lo = _mm_maddubs_epi16(ref23_lo, if_coeff_3_2);
    const __m128i sum23_hi = _mm_maddubs_epi16(ref23_hi, if_coeff_3_2);
    const __m128i sum0123_lo = _mm_add_epi16(sum01_lo, sum23_lo);
    const __m128i sum0123_hi = _mm_add_epi16(sum01_hi, sum23_hi);
    const __m128i sum_Offset_lo = _mm_add_epi16(sum0123_lo, if_offset);
    const __m128i sum_Offset_hi = _mm_add_epi16(sum0123_hi, if_offset);
    const __m128i sum_lo = _mm_srai_epi16(sum_Offset_lo, 5);
    const __m128i sum_hi = _mm_srai_epi16(sum_Offset_hi, 5);
    const __m128i sum_clip_U8 = _mm_packus_epi16(sum_lo, sum_hi);

    _mm_storeu_si128((__m128i *)dst, sum_clip_U8);
}

#ifdef __cplusplus
}
#endif
#endif //EBAVCSTYLEMCP_H
