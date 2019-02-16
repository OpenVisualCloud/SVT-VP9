/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbPictureOperators_SSE4_1.h"
#include "smmintrin.h"

uint64_t spatialfull_distortion_kernel4x4_ssse3_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *recon,
    uint32_t   recon_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{
    uint64_t spatial_distortion = 0;
    int32_t row_count;
    __m128i sum = _mm_setzero_si128();

    row_count = 4;
    do
    {
        __m128i x0;
        __m128i y0;
        x0 = _mm_setr_epi32(*((uint32_t *)input), 0, 0, 0);
        y0 = _mm_setr_epi32(*((uint32_t *)recon), 0, 0, 0);
        input += input_stride;
        recon += recon_stride;
        x0 = _mm_unpacklo_epi8(x0, _mm_setzero_si128());
        y0 = _mm_unpacklo_epi8(y0, _mm_setzero_si128());
        x0 = _mm_sub_epi16(x0, y0);
        x0 = _mm_sign_epi16(x0, x0);
        x0 = _mm_madd_epi16(x0, x0);
        sum = _mm_add_epi32(sum, x0);
    } while (--row_count);

    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0xe1)); // 11100001
    spatial_distortion = _mm_extract_epi32(sum, 0);

    (void)area_width;
    (void)area_height;
    return spatial_distortion;

};

uint64_t spatialfull_distortion_kernel8x8_ssse3_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *recon,
    uint32_t   recon_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{
    uint64_t spatial_distortion = 0;
    int32_t row_count;
    __m128i sum = _mm_setzero_si128();

    row_count = 8;
    do
    {
        __m128i x0;//, x1;
        __m128i y0;
        x0 = _mm_loadl_epi64/*_mm_loadu_si128*/((__m128i *)(input + 0x00));
        y0 = _mm_loadl_epi64((__m128i *)(recon + 0x00));
        input += input_stride;
        recon += recon_stride;
        x0 = _mm_unpacklo_epi8(x0, _mm_setzero_si128());
        y0 = _mm_unpacklo_epi8(y0, _mm_setzero_si128());
        x0 = _mm_sub_epi16(x0, y0);
        x0 = _mm_sign_epi16(x0, x0);
        x0 = _mm_madd_epi16(x0, x0);
        sum = _mm_add_epi32(sum, x0);
    } while (--row_count);

    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4e)); // 01001110
    sum = _mm_unpacklo_epi32(sum, sum);
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4e)); // 01001110
    spatial_distortion = _mm_extract_epi32(sum, 0);

    (void)area_width;
    (void)area_height;
    return spatial_distortion;

};

uint64_t spatialfull_distortion_kernel16_mx_n_ssse3_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *recon,
    uint32_t   recon_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{
    uint64_t  spatial_distortion = 0;
    int32_t row_count, col_count;
    __m128i sum = _mm_setzero_si128();
    __m128i x0, y0, x0_L, x0_H, y0_L, y0_H;

    col_count = area_width;
    do
    {
        uint8_t *coeff_temp = input;
        uint8_t *reconCoeffTemp = recon;

        row_count = area_height;
        do
        {
            x0 = _mm_loadu_si128((__m128i *)(coeff_temp + 0x00));
            y0 = _mm_loadu_si128((__m128i *)(reconCoeffTemp + 0x00));
            coeff_temp += input_stride;
            reconCoeffTemp += recon_stride;

            x0_L = _mm_unpacklo_epi8(x0, _mm_setzero_si128());
            x0_H = _mm_unpackhi_epi8(x0, _mm_setzero_si128());

            y0_L = _mm_unpacklo_epi8(y0, _mm_setzero_si128());
            y0_H = _mm_unpackhi_epi8(y0, _mm_setzero_si128());

            x0_L = _mm_sub_epi16(x0_L, y0_L);
            x0_H = _mm_sub_epi16(x0_H, y0_H);
            x0_L = _mm_sign_epi16(x0_L, x0_L);
            x0_H = _mm_sign_epi16(x0_H, x0_H);

            x0_L = _mm_madd_epi16(x0_L, x0_L);
            x0_H = _mm_madd_epi16(x0_H, x0_H);

            sum = _mm_add_epi32(sum, _mm_add_epi32(x0_L, x0_H));
        } while (--row_count);

        input += 16;
        recon += 16;
        col_count -= 16;
    } while (col_count > 0);

    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4e)); // 01001110
    sum = _mm_unpacklo_epi32(sum, sum);
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4e)); // 01001110
    spatial_distortion = _mm_extract_epi32(sum, 0);

    return spatial_distortion;

};
