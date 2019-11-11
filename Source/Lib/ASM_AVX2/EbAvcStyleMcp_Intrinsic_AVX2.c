/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <assert.h>
#include "EbAvcStyleMcp_AVX2.h"
#include "EbAvcStyleMcp_SSSE3.h"
#include "EbDefinitions.h"
#include "emmintrin.h"

static EB_ALIGN(16) const int8_t avc_style_luma_if_coeff8_avx2[] = {
    -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25,
    -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25,
     9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,
     9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,
    -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18,
    -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18,
    18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2,
    18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2, 18, -2,
    -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9,
    -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9, -1,  9,
    25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1,
    25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1, 25, -1
};

static INLINE void avc_style_luma_interpolation_filter32(const __m256i ref0,
    const __m256i ref1, const __m256i ref2, const __m256i ref3,
    const __m256i if_offset, const __m256i IFCoeff_1_0,
    const __m256i if_coeff_3_2, const EbByte dst)
{
    const __m256i ref01_lo      = _mm256_unpacklo_epi8(ref0, ref1);
    const __m256i ref01_hi      = _mm256_unpackhi_epi8(ref0, ref1);
    const __m256i ref23_lo      = _mm256_unpacklo_epi8(ref2, ref3);
    const __m256i ref23_hi      = _mm256_unpackhi_epi8(ref2, ref3);
    const __m256i sum01_lo      = _mm256_maddubs_epi16(ref01_lo, IFCoeff_1_0);
    const __m256i sum01_hi      = _mm256_maddubs_epi16(ref01_hi, IFCoeff_1_0);
    const __m256i sum23_lo      = _mm256_maddubs_epi16(ref23_lo, if_coeff_3_2);
    const __m256i sum23_hi      = _mm256_maddubs_epi16(ref23_hi, if_coeff_3_2);
    const __m256i sum0123_lo    = _mm256_add_epi16(sum01_lo, sum23_lo);
    const __m256i sum0123_hi    = _mm256_add_epi16(sum01_hi, sum23_hi);
    const __m256i sum_Offset_lo = _mm256_add_epi16(sum0123_lo, if_offset);
    const __m256i sum_Offset_hi = _mm256_add_epi16(sum0123_hi, if_offset);
    const __m256i sum_lo        = _mm256_srai_epi16(sum_Offset_lo, 5);
    const __m256i sum_hi        = _mm256_srai_epi16(sum_Offset_hi, 5);
    const __m256i sum_clip_U8   = _mm256_packus_epi16(sum_lo, sum_hi);

    _mm256_storeu_si256((__m256i *)dst, sum_clip_U8);
}

void eb_vp9_avc_style_luma_interpolation_filter_horizontal_avx2_intrin(
    EbByte ref_pic,
    uint32_t src_stride,
    EbByte dst,
    uint32_t dst_stride,
    uint32_t pu_width,
    uint32_t pu_height,
    EbByte temp_buf,
    uint32_t frac_pos)
{
    const __m128i if_offset    = _mm_set1_epi16(16);
    const __m128i IFCoeff_1_0 = _mm_load_si128((__m128i *)(avc_style_luma_if_coeff8_ssse3 + 32 * frac_pos - 32));
    const __m128i if_coeff_3_2 = _mm_load_si128((__m128i *)(avc_style_luma_if_coeff8_ssse3 + 32 * frac_pos - 16));
    uint32_t width_cnt, height_cnt;

    assert(!(pu_width & 7));
    (void)temp_buf;

    if (pu_width & 8) { // first 8
        EbByte refPic_t = ref_pic;
        EbByte dst_t = dst;

        height_cnt = pu_height;
        do {
            const __m128i ref0 = _mm_loadl_epi64((__m128i *)(refPic_t - 1));
            const __m128i ref1 = _mm_loadl_epi64((__m128i *)(refPic_t + 0));
            const __m128i ref2 = _mm_loadl_epi64((__m128i *)(refPic_t + 1));
            const __m128i ref3 = _mm_loadl_epi64((__m128i *)(refPic_t + 2));
            avc_style_luma_interpolation_filter8(ref0, ref1, ref2, ref3, if_offset,
                IFCoeff_1_0, if_coeff_3_2, dst_t);
            refPic_t += src_stride;
            dst_t += dst_stride;
        } while (--height_cnt);

        pu_width -= 8;
        ref_pic += 8;
        dst += 8;
    }

    if (pu_width & 16) { // first 16
        EbByte refPic_t = ref_pic;
        EbByte dst_t = dst;

        height_cnt = pu_height;
        do {
            const __m128i ref0 = _mm_loadu_si128((__m128i *)(refPic_t - 1));
            const __m128i ref1 = _mm_loadu_si128((__m128i *)(refPic_t + 0));
            const __m128i ref2 = _mm_loadu_si128((__m128i *)(refPic_t + 1));
            const __m128i ref3 = _mm_loadu_si128((__m128i *)(refPic_t + 2));
            avc_style_luma_interpolation_filter16(ref0, ref1, ref2, ref3,
                if_offset, IFCoeff_1_0, if_coeff_3_2, dst_t);
            refPic_t += src_stride;
            dst_t += dst_stride;
        } while (--height_cnt);

        pu_width -= 16;
        ref_pic += 16;
        dst += 16;
    }

    if (pu_width) { // 32x
        const __m256i IFOffset_avx2 = _mm256_set1_epi16(16);
        const __m256i IFCoeff_1_0_avx2 = _mm256_loadu_si256((__m256i *)(avc_style_luma_if_coeff8_avx2 + 64 * frac_pos - 64));
        const __m256i IFCoeff_3_2_avx2 = _mm256_loadu_si256((__m256i *)(avc_style_luma_if_coeff8_avx2 + 64 * frac_pos - 32));

        height_cnt = pu_height;
        do {
            width_cnt = 0;
            do {
                const __m256i ref0 = _mm256_loadu_si256((__m256i *)(ref_pic + width_cnt - 1));
                const __m256i ref1 = _mm256_loadu_si256((__m256i *)(ref_pic + width_cnt + 0));
                const __m256i ref2 = _mm256_loadu_si256((__m256i *)(ref_pic + width_cnt + 1));
                const __m256i ref3 = _mm256_loadu_si256((__m256i *)(ref_pic + width_cnt + 2));
                avc_style_luma_interpolation_filter32(ref0, ref1, ref2, ref3,
                    IFOffset_avx2, IFCoeff_1_0_avx2, IFCoeff_3_2_avx2,
                    dst + width_cnt);
                width_cnt += 32;
            } while (width_cnt < pu_width);

            ref_pic += src_stride;
            dst += dst_stride;
        } while (--height_cnt);
    }
}

void eb_vp9_avc_style_luma_interpolation_filter_vertical_avx2_intrin(
    EbByte ref_pic,
    uint32_t src_stride,
    EbByte dst,
    uint32_t dst_stride,
    uint32_t pu_width,
    uint32_t pu_height,
    EbByte temp_buf,
    uint32_t frac_pos)
{
    const __m128i if_offset = _mm_set1_epi16(16);
    const __m128i IFCoeff_1_0 = _mm_load_si128((__m128i *)(avc_style_luma_if_coeff8_ssse3 + 32 * frac_pos - 32));
    const __m128i if_coeff_3_2 = _mm_load_si128((__m128i *)(avc_style_luma_if_coeff8_ssse3 + 32 * frac_pos - 16));
    uint32_t width_cnt, height_cnt;

    assert(!(pu_width & 7));
    (void)temp_buf;

    if (pu_width & 8) { // first 8
        EbByte refPic_t = ref_pic + 2 * src_stride;
        EbByte dst_t = dst;
        __m128i ref0 = _mm_loadl_epi64((__m128i *)(ref_pic - 1 * src_stride));
        __m128i ref1 = _mm_loadl_epi64((__m128i *)(ref_pic + 0 * src_stride));
        __m128i ref2 = _mm_loadl_epi64((__m128i *)(ref_pic + 1 * src_stride));

        height_cnt = pu_height;
        do {
            const __m128i ref3 = _mm_loadl_epi64((__m128i *)refPic_t);
            avc_style_luma_interpolation_filter8(ref0, ref1, ref2, ref3, if_offset,
                IFCoeff_1_0, if_coeff_3_2, dst_t);
            ref0 = ref1;
            ref1 = ref2;
            ref2 = ref3;
            refPic_t += src_stride;
            dst_t += dst_stride;
        } while (--height_cnt);

        pu_width -= 8;
        ref_pic += 8;
        dst += 8;
    }

    if (pu_width & 16) { // first 16
        EbByte refPic_t = ref_pic + 2 * src_stride;
        EbByte dst_t = dst;
        __m128i ref0 = _mm_loadu_si128((__m128i *)(ref_pic - 1 * src_stride));
        __m128i ref1 = _mm_loadu_si128((__m128i *)(ref_pic + 0 * src_stride));
        __m128i ref2 = _mm_loadu_si128((__m128i *)(ref_pic + 1 * src_stride));

        height_cnt = pu_height;
        do {
            const __m128i ref3 = _mm_loadu_si128((__m128i *)refPic_t);
            avc_style_luma_interpolation_filter16(ref0, ref1, ref2, ref3, if_offset,
                IFCoeff_1_0, if_coeff_3_2, dst_t);
            ref0 = ref1;
            ref1 = ref2;
            ref2 = ref3;
            refPic_t += src_stride;
            dst_t += dst_stride;
        } while (--height_cnt);

        pu_width -= 16;
        ref_pic += 16;
        dst += 16;
    }

    if (pu_width) { // 32x
        const __m256i IFOffset_avx2 = _mm256_set1_epi16(16);
        const __m256i IFCoeff_1_0_avx2 = _mm256_loadu_si256((__m256i *)(avc_style_luma_if_coeff8_avx2 + 64 * frac_pos - 64));
        const __m256i IFCoeff_3_2_avx2 = _mm256_loadu_si256((__m256i *)(avc_style_luma_if_coeff8_avx2 + 64 * frac_pos - 32));

        width_cnt = 0;
        do {
            EbByte refPic_t = ref_pic + 2 * src_stride + width_cnt;
            EbByte dst_t = dst + width_cnt;
            __m256i ref0 = _mm256_loadu_si256((__m256i *)(refPic_t - 3 * src_stride));
            __m256i ref1 = _mm256_loadu_si256((__m256i *)(refPic_t - 2 * src_stride));
            __m256i ref2 = _mm256_loadu_si256((__m256i *)(refPic_t - 1 * src_stride));

            height_cnt = pu_height;
            do {
                const __m256i ref3 = _mm256_loadu_si256((__m256i *)refPic_t);
                avc_style_luma_interpolation_filter32(ref0, ref1, ref2, ref3,
                    IFOffset_avx2, IFCoeff_1_0_avx2, IFCoeff_3_2_avx2, dst_t);
                ref0 = ref1;
                ref1 = ref2;
                ref2 = ref3;
                refPic_t += src_stride;
                dst_t += dst_stride;
            } while (--height_cnt);

            width_cnt += 32;
        } while (width_cnt < pu_width);
    }
}
