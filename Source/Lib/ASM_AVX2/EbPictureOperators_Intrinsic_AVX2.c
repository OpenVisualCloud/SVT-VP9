/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdio.h>
#include <immintrin.h>
#include "EbMemory_AVX2.h"
#include "EbDefinitions.h"
#include "EbPictureOperators_AVX2.h"
#include "mem_sse2.h"

static __m128i _mm_loadhi_epi64(__m128i x, __m128i *p) {
    return _mm_castpd_si128(_mm_loadh_pd(_mm_castsi128_pd(x), (double *)p));
}

void full_distortion_kernel_eob_zero_4x4_32bit_bt_avx2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                                       uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                                       uint32_t area_width, uint32_t area_height) {
    __m256i sum = _mm256_setzero_si256();
    __m128i m0, m1;
    __m256i x, y;
    m0 = _mm_loadl_epi64((__m128i *)coeff);
    coeff += coeff_stride;
    m0 = _mm_loadhi_epi64(m0, (__m128i *)coeff);
    coeff += coeff_stride;
    m1 = _mm_loadl_epi64((__m128i *)coeff);
    coeff += coeff_stride;
    m1 = _mm_loadhi_epi64(m1, (__m128i *)coeff);
    coeff += coeff_stride;
    x   = _mm256_set_m128i(m1, m0);
    y   = _mm256_madd_epi16(x, x);
    sum = _mm256_add_epi32(sum, y);
    sum = _mm256_add_epi32(sum, _mm256_shuffle_epi32(sum, 0x4e)); // 01001110
    sum = _mm256_unpacklo_epi32(sum, sum);
    sum = _mm256_add_epi32(sum, _mm256_shuffle_epi32(sum, 0x4e)); // 01001110
    m0  = _mm256_extracti128_si256(sum, 0);
    m1  = _mm256_extracti128_si256(sum, 1);
    m0  = _mm_add_epi32(m0, m1);
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(m0, _mm_setzero_si128()));

    (void)area_width;
    (void)area_height;
    (void)recon_coeff;
    (void)recon_coeff_stride;
}

void full_distortion_kernel_eob_zero_8x8_32bit_bt_avx2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                                       uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                                       uint32_t area_width, uint32_t area_height) {
    int32_t row_count;
    __m256i sum = _mm256_setzero_si256();
    __m128i temp1, temp2;

    row_count = 4;
    do {
        __m128i m0, m1;
        __m256i x, y;

        m0 = _mm_loadu_si128((__m128i *)(coeff));
        coeff += coeff_stride;
        m1 = _mm_loadu_si128((__m128i *)(coeff));
        coeff += coeff_stride;
        x = _mm256_set_m128i(m1, m0);
        y = _mm256_madd_epi16(x, x);

        sum = _mm256_add_epi32(sum, y);
    } while (--row_count);

    sum   = _mm256_add_epi32(sum, _mm256_shuffle_epi32(sum, 0x4e)); // 01001110
    sum   = _mm256_unpacklo_epi32(sum, sum);
    sum   = _mm256_add_epi32(sum, _mm256_shuffle_epi32(sum, 0x4e)); // 01001110
    temp1 = _mm256_extracti128_si256(sum, 0);
    temp2 = _mm256_extracti128_si256(sum, 1);
    temp1 = _mm_add_epi32(temp1, temp2);
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(temp1, _mm_setzero_si128()));

    (void)area_width;
    (void)area_height;
    (void)recon_coeff;
    (void)recon_coeff_stride;
}

void full_distortion_kernel_eob_zero16_mxn_32bit_bt_avx2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                                         uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                                         uint32_t area_width, uint32_t area_height) {
    int32_t row_count, col_count;
    __m256i sum = _mm256_setzero_si256();
    __m128i temp1, temp2;
    UNUSED(recon_coeff);
    UNUSED(recon_coeff_stride);

    col_count = area_width;
    do {
        int16_t *coeff_temp = coeff;

        row_count = area_height;
        do {
            __m256i x0, z0;
            x0 = _mm256_loadu_si256((__m256i *)(coeff_temp));
            coeff_temp += coeff_stride;
            z0  = _mm256_madd_epi16(x0, x0);
            sum = _mm256_add_epi32(sum, z0);
        } while (--row_count);

        coeff += 16;
        recon_coeff += 16;
        col_count -= 16;
    } while (col_count > 0);

    sum   = _mm256_add_epi32(sum, _mm256_shuffle_epi32(sum, 0x4e)); // 01001110
    sum   = _mm256_unpacklo_epi32(sum, sum);
    sum   = _mm256_add_epi32(sum, _mm256_shuffle_epi32(sum, 0x4e)); // 01001110
    temp1 = _mm256_extracti128_si256(sum, 0);
    temp2 = _mm256_extracti128_si256(sum, 1);
    temp1 = _mm_add_epi32(temp1, temp2);
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(temp1, _mm_setzero_si128()));
}

void full_distortion_kernel_4x4_32bit_bt_avx2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                              uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                              uint32_t area_width, uint32_t area_height) {
    __m256i sum1 = _mm256_setzero_si256();
    __m256i sum2 = _mm256_setzero_si256();
    __m128i m0, m1;
    __m256i x, y, z;
    m0 = _mm_loadl_epi64((__m128i *)coeff);
    coeff += coeff_stride;
    m0 = _mm_loadhi_epi64(m0, (__m128i *)coeff);
    coeff += coeff_stride;
    m1 = _mm_loadl_epi64((__m128i *)coeff);
    coeff += coeff_stride;
    m1 = _mm_loadhi_epi64(m1, (__m128i *)coeff);
    coeff += coeff_stride;
    x  = _mm256_set_m128i(m1, m0);
    m0 = _mm_loadl_epi64((__m128i *)recon_coeff);
    recon_coeff += recon_coeff_stride;
    m0 = _mm_loadhi_epi64(m0, (__m128i *)recon_coeff);
    recon_coeff += recon_coeff_stride;
    m1 = _mm_loadl_epi64((__m128i *)recon_coeff);
    recon_coeff += recon_coeff_stride;
    m1 = _mm_loadhi_epi64(m1, (__m128i *)recon_coeff);
    recon_coeff += recon_coeff_stride;
    y    = _mm256_set_m128i(m1, m0);
    z    = _mm256_madd_epi16(x, x);
    sum2 = _mm256_add_epi32(sum2, z);
    x    = _mm256_sub_epi16(x, y);
    x    = _mm256_madd_epi16(x, x);
    sum1 = _mm256_add_epi32(sum1, x);
    sum1 = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4e)); // 01001110
    sum2 = _mm256_add_epi32(sum2, _mm256_shuffle_epi32(sum2, 0x4e)); // 01001110
    sum1 = _mm256_unpacklo_epi32(sum1, sum2);
    sum1 = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4e)); // 01001110
    m0   = _mm256_extracti128_si256(sum1, 0);
    m1   = _mm256_extracti128_si256(sum1, 1);
    m0   = _mm_add_epi32(m0, m1);
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(m0, _mm_setzero_si128()));

    (void)area_width;
    (void)area_height;
}

void full_distortion_kernel_8x8_32bit_bt_avx2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                              uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                              uint32_t area_width, uint32_t area_height) {
    int32_t row_count;

    __m256i sum1 = _mm256_setzero_si256();
    __m256i sum2 = _mm256_setzero_si256();
    __m128i temp1, temp2;

    row_count = 4;
    do {
        __m128i m0, m1;
        __m256i x, y, z;

        m0 = _mm_loadu_si128((__m128i *)(coeff));
        coeff += coeff_stride;
        m1 = _mm_loadu_si128((__m128i *)(coeff));
        coeff += coeff_stride;
        x = _mm256_set_m128i(m1, m0);

        m0 = _mm_loadu_si128((__m128i *)(recon_coeff));
        recon_coeff += recon_coeff_stride;
        m1 = _mm_loadu_si128((__m128i *)(recon_coeff));
        recon_coeff += recon_coeff_stride;
        y = _mm256_set_m128i(m1, m0);

        z    = _mm256_madd_epi16(x, x);
        sum2 = _mm256_add_epi32(sum2, z);
        x    = _mm256_sub_epi16(x, y);
        x    = _mm256_madd_epi16(x, x);
        sum1 = _mm256_add_epi32(sum1, x);
    } while (--row_count);

    sum1  = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4e)); // 01001110
    sum2  = _mm256_add_epi32(sum2, _mm256_shuffle_epi32(sum2, 0x4e)); // 01001110
    sum1  = _mm256_unpacklo_epi32(sum1, sum2);
    sum1  = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4e)); // 01001110
    temp1 = _mm256_extracti128_si256(sum1, 0);
    temp2 = _mm256_extracti128_si256(sum1, 1);
    temp1 = _mm_add_epi32(temp1, temp2);
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(temp1, _mm_setzero_si128()));

    (void)area_width;
    (void)area_height;
}

void full_distortion_kernel_16_32_bit_bt_avx2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                              uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                              uint32_t area_width, uint32_t area_height) {
    int32_t row_count, col_count;
    __m256i sum1 = _mm256_setzero_si256();
    __m256i sum2 = _mm256_setzero_si256();
    __m128i temp1, temp2;

    col_count = area_width;
    do {
        int16_t *coeff_temp     = coeff;
        int16_t *reconCoeffTemp = recon_coeff;

        row_count = area_height;
        do {
            __m256i x, y, z;
            x = _mm256_loadu_si256((__m256i *)(coeff_temp));
            y = _mm256_loadu_si256((__m256i *)(reconCoeffTemp));
            coeff_temp += coeff_stride;
            reconCoeffTemp += recon_coeff_stride;

            z    = _mm256_madd_epi16(x, x);
            sum2 = _mm256_add_epi32(sum2, z);
            x    = _mm256_sub_epi16(x, y);
            x    = _mm256_madd_epi16(x, x);
            sum1 = _mm256_add_epi32(sum1, x);
        } while (--row_count);

        coeff += 16;
        recon_coeff += 16;
        col_count -= 16;
    } while (col_count > 0);

    sum1  = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4e)); // 01001110
    sum2  = _mm256_add_epi32(sum2, _mm256_shuffle_epi32(sum2, 0x4e)); // 01001110
    sum1  = _mm256_unpacklo_epi32(sum1, sum2);
    sum1  = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4e)); // 01001110
    temp1 = _mm256_extracti128_si256(sum1, 0);
    temp2 = _mm256_extracti128_si256(sum1, 1);
    temp1 = _mm_add_epi32(temp1, temp2);
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(temp1, _mm_setzero_si128()));
}

void eb_vp9_picture_average_kernel_avx2_intrin(EbByte src0, uint32_t src0_stride, EbByte src1, uint32_t src1_stride,
                                               EbByte dst, uint32_t dst_stride, uint32_t area_width,
                                               uint32_t area_height) {
    __m128i  xmm_avg1, xmm_avg2, xmm_avg3, xmm_avg4, temp1, temp2, temp3, temp4;
    __m256i  ymm_avg1, ymm_avg2, ymm_avg3, ymm_avg4;
    uint32_t y;
    if (area_width >= 16) {
        if (area_width == 16) {
            for (y = 0; y < area_height; y += 2) {
                temp1    = _mm_loadu_si128((__m128i *)src0);
                temp2    = _mm_loadu_si128((__m128i *)(src0 + src0_stride));
                temp3    = _mm_loadu_si128((__m128i *)src1);
                temp4    = _mm_loadu_si128((__m128i *)(src1 + src1_stride));
                ymm_avg1 = _mm256_avg_epu8(_mm256_set_m128i(temp2, temp1), _mm256_set_m128i(temp4, temp3));
                xmm_avg1 = _mm256_extracti128_si256(ymm_avg1, 0);
                xmm_avg2 = _mm256_extracti128_si256(ymm_avg1, 1);
                _mm_storeu_si128((__m128i *)dst, xmm_avg1);
                _mm_storeu_si128((__m128i *)(dst + dst_stride), xmm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        } else if (area_width == 24) {
            for (y = 0; y < area_height; y += 2) {
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i *)src0), _mm_loadu_si128((__m128i *)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadl_epi64((__m128i *)(src0 + 16)),
                                        _mm_loadl_epi64((__m128i *)(src1 + 16)));
                xmm_avg3 = _mm_avg_epu8(_mm_loadu_si128((__m128i *)(src0 + src0_stride)),
                                        _mm_loadu_si128((__m128i *)(src1 + src1_stride)));
                xmm_avg4 = _mm_avg_epu8(_mm_loadl_epi64((__m128i *)(src0 + src0_stride + 16)),
                                        _mm_loadl_epi64((__m128i *)(src1 + src1_stride + 16)));

                _mm_storeu_si128((__m128i *)dst, xmm_avg1);
                _mm_storel_epi64((__m128i *)(dst + 16), xmm_avg2);
                _mm_storeu_si128((__m128i *)(dst + dst_stride), xmm_avg3);
                _mm_storel_epi64((__m128i *)(dst + dst_stride + 16), xmm_avg4);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        } else if (area_width == 32) {
            for (y = 0; y < area_height; y += 2) {
                ymm_avg1 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i *)src0), _mm256_loadu_si256((__m256i *)src1));
                ymm_avg2 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i *)(src0 + src0_stride)),
                                           _mm256_loadu_si256((__m256i *)(src1 + src1_stride)));

                _mm256_storeu_si256((__m256i *)dst, ymm_avg1);
                _mm256_storeu_si256((__m256i *)(dst + dst_stride), ymm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        } else if (area_width == 48) {
            for (y = 0; y < area_height; y += 2) {
                ymm_avg1 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i *)src0), _mm256_loadu_si256((__m256i *)src1));
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i *)(src0 + 32)),
                                        _mm_loadu_si128((__m128i *)(src1 + 32)));

                ymm_avg2 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i *)(src0 + src0_stride)),
                                           _mm256_loadu_si256((__m256i *)(src1 + src1_stride)));
                xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i *)(src0 + src0_stride + 32)),
                                        _mm_loadu_si128((__m128i *)(src1 + src1_stride + 32)));

                _mm256_storeu_si256((__m256i *)dst, ymm_avg1);
                _mm_storeu_si128((__m128i *)(dst + 32), xmm_avg1);
                _mm256_storeu_si256((__m256i *)(dst + dst_stride), ymm_avg2);
                _mm_storeu_si128((__m128i *)(dst + dst_stride + 32), xmm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        } else {
            for (y = 0; y < area_height; y += 2) {
                ymm_avg1 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i *)src0), _mm256_loadu_si256((__m256i *)src1));
                ymm_avg2 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i *)(src0 + 32)),
                                           _mm256_loadu_si256((__m256i *)(src1 + 32)));
                ymm_avg3 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i *)(src0 + src0_stride)),
                                           _mm256_loadu_si256((__m256i *)(src1 + src1_stride)));
                ymm_avg4 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i *)(src0 + src0_stride + 32)),
                                           _mm256_loadu_si256((__m256i *)(src1 + src1_stride + 32)));

                _mm256_storeu_si256((__m256i *)dst, ymm_avg1);
                _mm256_storeu_si256((__m256i *)(dst + 32), ymm_avg2);
                _mm256_storeu_si256((__m256i *)(dst + dst_stride), ymm_avg3);
                _mm256_storeu_si256((__m256i *)(dst + dst_stride + 32), ymm_avg4);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
    } else {
        if (area_width == 4) {
            for (y = 0; y < area_height; y += 2) {
                xmm_avg1 = _mm_avg_epu8(_mm_cvtsi32_si128(*(uint32_t *)src0), _mm_cvtsi32_si128(*(uint32_t *)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_cvtsi32_si128(*(uint32_t *)(src0 + src0_stride)),
                                        _mm_cvtsi32_si128(*(uint32_t *)(src1 + src1_stride)));

                *(uint32_t *)dst                = _mm_cvtsi128_si32(xmm_avg1);
                *(uint32_t *)(dst + dst_stride) = _mm_cvtsi128_si32(xmm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        } else if (area_width == 8) {
            for (y = 0; y < area_height; y += 2) {
                xmm_avg1 = _mm_avg_epu8(_mm_loadl_epi64((__m128i *)src0), _mm_loadl_epi64((__m128i *)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadl_epi64((__m128i *)(src0 + src0_stride)),
                                        _mm_loadl_epi64((__m128i *)(src1 + src1_stride)));

                _mm_storel_epi64((__m128i *)dst, xmm_avg1);
                _mm_storel_epi64((__m128i *)(dst + dst_stride), xmm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        } else {
            for (y = 0; y < area_height; y += 2) {
                xmm_avg1 = _mm_avg_epu8(_mm_loadl_epi64((__m128i *)src0), _mm_loadl_epi64((__m128i *)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_cvtsi32_si128(*(uint32_t *)(src0 + 8)),
                                        _mm_cvtsi32_si128(*(uint32_t *)(src1 + 8)));

                xmm_avg3 = _mm_avg_epu8(_mm_loadl_epi64((__m128i *)(src0 + src0_stride)),
                                        _mm_loadl_epi64((__m128i *)(src1 + src1_stride)));
                xmm_avg4 = _mm_avg_epu8(_mm_cvtsi32_si128(*(uint32_t *)(src0 + src0_stride + 8)),
                                        _mm_cvtsi32_si128(*(uint32_t *)(src1 + src1_stride + 8)));

                _mm_storel_epi64((__m128i *)dst, xmm_avg1);
                *(uint32_t *)(dst + 8) = _mm_cvtsi128_si32(xmm_avg2);
                _mm_storel_epi64((__m128i *)(dst + dst_stride), xmm_avg3);
                *(uint32_t *)(dst + dst_stride + 8) = _mm_cvtsi128_si32(xmm_avg4);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
    }
}

void eb_vp9_residual_kernel4x4_avx2_intrin(uint8_t *input, uint32_t input_stride, uint8_t *pred, uint32_t pred_stride,
                                           int16_t *residual, uint32_t residual_stride, uint32_t area_width,
                                           uint32_t area_height) {
    const __m256i zero  = _mm256_setzero_si256();
    const __m256i in    = load8bit_4x4_avx2(input, input_stride);
    const __m256i pr    = load8bit_4x4_avx2(pred, pred_stride);
    const __m256i in_lo = _mm256_unpacklo_epi8(in, zero);
    const __m256i pr_lo = _mm256_unpacklo_epi8(pr, zero);
    const __m256i re_lo = _mm256_sub_epi16(in_lo, pr_lo);
    const __m128i re_01 = _mm256_extracti128_si256(re_lo, 0);
    const __m128i re_23 = _mm256_extracti128_si256(re_lo, 1);
    (void)area_width;
    (void)area_height;

    _mm_storel_epi64((__m128i *)(residual + 0 * residual_stride), re_01);
    _mm_storeh_epi64((__m128i *)(residual + 1 * residual_stride), re_01);
    _mm_storel_epi64((__m128i *)(residual + 2 * residual_stride), re_23);
    _mm_storeh_epi64((__m128i *)(residual + 3 * residual_stride), re_23);
}

void eb_vp9_residual_kernel8x8_avx2_intrin(uint8_t *input, uint32_t input_stride, uint8_t *pred, uint32_t pred_stride,
                                           int16_t *residual, uint32_t residual_stride, uint32_t area_width,
                                           uint32_t area_height) {
    const __m256i zero = _mm256_setzero_si256();
    uint32_t      y;
    (void)area_width;
    (void)area_height;

    for (y = 0; y < 8; y += 4) {
        const __m256i in    = load8bit_8x4_avx2(input, input_stride);
        const __m256i pr    = load8bit_8x4_avx2(pred, pred_stride);
        const __m256i in_lo = _mm256_unpacklo_epi8(in, zero);
        const __m256i in_hi = _mm256_unpackhi_epi8(in, zero);
        const __m256i pr_lo = _mm256_unpacklo_epi8(pr, zero);
        const __m256i pr_hi = _mm256_unpackhi_epi8(pr, zero);
        const __m256i re_lo = _mm256_sub_epi16(in_lo, pr_lo);
        const __m256i re_hi = _mm256_sub_epi16(in_hi, pr_hi);
        const __m128i re_0  = _mm256_extracti128_si256(re_lo, 0);
        const __m128i re_1  = _mm256_extracti128_si256(re_hi, 0);
        const __m128i re_2  = _mm256_extracti128_si256(re_lo, 1);
        const __m128i re_3  = _mm256_extracti128_si256(re_hi, 1);

        _mm_storeu_si128((__m128i *)(residual + 0 * residual_stride), re_0);
        _mm_storeu_si128((__m128i *)(residual + 1 * residual_stride), re_1);
        _mm_storeu_si128((__m128i *)(residual + 2 * residual_stride), re_2);
        _mm_storeu_si128((__m128i *)(residual + 3 * residual_stride), re_3);
        input += 4 * input_stride;
        pred += 4 * pred_stride;
        residual += 4 * residual_stride;
    }
}

void eb_vp9_residual_kernel16x16_avx2_intrin(uint8_t *input, uint32_t input_stride, uint8_t *pred, uint32_t pred_stride,
                                             int16_t *residual, uint32_t residual_stride, uint32_t area_width,
                                             uint32_t area_height) {
    const __m256i zero = _mm256_setzero_si256();
    uint32_t      y;
    (void)area_width;
    (void)area_height;

    for (y = 0; y < 16; y += 2) {
        const __m256i in0   = load8bit_16x2_unaligned_avx2(input, input_stride);
        const __m256i pr0   = load8bit_16x2_unaligned_avx2(pred, pred_stride);
        const __m256i in1   = _mm256_permute4x64_epi64(in0, 0xD8);
        const __m256i pr1   = _mm256_permute4x64_epi64(pr0, 0xD8);
        const __m256i in_lo = _mm256_unpacklo_epi8(in1, zero);
        const __m256i in_hi = _mm256_unpackhi_epi8(in1, zero);
        const __m256i pr_lo = _mm256_unpacklo_epi8(pr1, zero);
        const __m256i pr_hi = _mm256_unpackhi_epi8(pr1, zero);
        const __m256i re_lo = _mm256_sub_epi16(in_lo, pr_lo);
        const __m256i re_hi = _mm256_sub_epi16(in_hi, pr_hi);

        _mm256_storeu_si256((__m256i *)(residual + 0 * residual_stride), re_lo);
        _mm256_storeu_si256((__m256i *)(residual + 1 * residual_stride), re_hi);
        input += 2 * input_stride;
        pred += 2 * pred_stride;
        residual += 2 * residual_stride;
    }
}

static inline void eb_vp9_residual_kernel32_avx2(const uint8_t *const input, const uint8_t *const pred,
                                                 int16_t *const residual) {
    const __m256i zero  = _mm256_setzero_si256();
    const __m256i in0   = _mm256_loadu_si256((__m256i *)input);
    const __m256i pr0   = _mm256_loadu_si256((__m256i *)pred);
    const __m256i in1   = _mm256_permute4x64_epi64(in0, 0xD8);
    const __m256i pr1   = _mm256_permute4x64_epi64(pr0, 0xD8);
    const __m256i in_lo = _mm256_unpacklo_epi8(in1, zero);
    const __m256i in_hi = _mm256_unpackhi_epi8(in1, zero);
    const __m256i pr_lo = _mm256_unpacklo_epi8(pr1, zero);
    const __m256i pr_hi = _mm256_unpackhi_epi8(pr1, zero);
    const __m256i re_lo = _mm256_sub_epi16(in_lo, pr_lo);
    const __m256i re_hi = _mm256_sub_epi16(in_hi, pr_hi);
    _mm256_storeu_si256((__m256i *)(residual + 0x00), re_lo);
    _mm256_storeu_si256((__m256i *)(residual + 0x10), re_hi);
}

void eb_vp9_residual_kernel32x32_avx2_intrin(uint8_t *input, uint32_t input_stride, uint8_t *pred, uint32_t pred_stride,
                                             int16_t *residual, uint32_t residual_stride, uint32_t area_width,
                                             uint32_t area_height) {
    uint32_t y;
    (void)area_width;
    (void)area_height;

    for (y = 0; y < 32; ++y) {
        eb_vp9_residual_kernel32_avx2(input, pred, residual);
        input += input_stride;
        pred += pred_stride;
        residual += residual_stride;
    }
}

void eb_vp9_residual_kernel64x64_avx2_intrin(uint8_t *input, uint32_t input_stride, uint8_t *pred, uint32_t pred_stride,
                                             int16_t *residual, uint32_t residual_stride, uint32_t area_width,
                                             uint32_t area_height) {
    uint32_t y;
    (void)area_width;
    (void)area_height;

    for (y = 0; y < 64; ++y) {
        eb_vp9_residual_kernel32_avx2(input + 0x00, pred + 0x00, residual + 0x00);
        eb_vp9_residual_kernel32_avx2(input + 0x20, pred + 0x20, residual + 0x20);
        input += input_stride;
        pred += pred_stride;
        residual += residual_stride;
    }
}
