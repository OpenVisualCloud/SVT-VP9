/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include "EbCombinedAveragingSAD_Intrinsic_AVX512.h"
#ifdef _WIN32
#include <intrin.h>
#endif

#ifndef DISABLE_AVX512
//BiPredAveragingOnthefly
AVX512_FUNC_TARGET
void bi_pred_average_kernel_avx512_intrin(
    EbByte                  src0,
    uint32_t                   src0_stride,
    EbByte                  src1,
    uint32_t                   src1_stride,
    EbByte                  dst,
    uint32_t                   dst_stride,
    uint32_t                   area_width,
    uint32_t                   area_height)
{
    __m128i xmm_avg1, xmm_avg2, xmm_avg3, xmm_avg4, xmm_avg5, xmm_avg6;
    __m256i xmm_avg1_256,  xmm_avg3_256;
    uint32_t y;

    if (area_width > 16)
    {
        if (area_width == 24)
        {
            for (y = 0; y < area_height; y += 2) {
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + 16)), _mm_loadl_epi64((__m128i*)(src1 + 16)));
                xmm_avg3 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0_stride)), _mm_loadu_si128((__m128i*)(src1 + src1_stride)));
                xmm_avg4 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + src0_stride + 16)), _mm_loadl_epi64((__m128i*)(src1 + src1_stride + 16)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storel_epi64((__m128i*) (dst + 16), xmm_avg2);
                _mm_storeu_si128((__m128i*) (dst + dst_stride), xmm_avg3);
                _mm_storel_epi64((__m128i*) (dst + dst_stride + 16), xmm_avg4);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
        else if (area_width == 32)
        {
            for (y = 0; y < area_height; y += 2) {

                xmm_avg1_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)src0), _mm256_loadu_si256((__m256i*)src1));
                xmm_avg3_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + src0_stride)), _mm256_loadu_si256((__m256i*)(src1 + src1_stride)));

                _mm256_storeu_si256((__m256i *) dst, xmm_avg1_256);
                _mm256_storeu_si256((__m256i *) (dst + dst_stride), xmm_avg3_256);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
        else if (area_width == 48)
        {
            for (y = 0; y < area_height; y += 2) {
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 16)), _mm_loadu_si128((__m128i*)(src1 + 16)));
                xmm_avg3 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 32)), _mm_loadu_si128((__m128i*)(src1 + 32)));

                xmm_avg4 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0_stride)), _mm_loadu_si128((__m128i*)(src1 + src1_stride)));
                xmm_avg5 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0_stride + 16)), _mm_loadu_si128((__m128i*)(src1 + src1_stride + 16)));
                xmm_avg6 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0_stride + 32)), _mm_loadu_si128((__m128i*)(src1 + src1_stride + 32)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storeu_si128((__m128i*) (dst + 16), xmm_avg2);
                _mm_storeu_si128((__m128i*) (dst + 32), xmm_avg3);
                _mm_storeu_si128((__m128i*) (dst + dst_stride), xmm_avg4);
                _mm_storeu_si128((__m128i*) (dst + dst_stride + 16), xmm_avg5);
                _mm_storeu_si128((__m128i*) (dst + dst_stride + 32), xmm_avg6);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;

            }
        }
        else
        {
            uint32_t src0Stride1 = src0_stride << 1;
            uint32_t src1Stride1 = src1_stride << 1;
            uint32_t dstStride1 = dst_stride << 1;

            __m512i xmm_avg1_512, xmm_avg3_512;

            for (uint32_t y = 0; y < area_height; y += 2, src0 += src0Stride1, src1 += src1Stride1, dst += dstStride1) {
                xmm_avg1_512 = _mm512_avg_epu8(_mm512_loadu_si512((__m512i *)src0), _mm512_loadu_si512((__m512i *)src1));
                xmm_avg3_512 = _mm512_avg_epu8(_mm512_loadu_si512((__m512i *)(src0 + src0_stride)), _mm512_loadu_si512((__m512i *)(src1 + src1_stride)));

                _mm512_storeu_si512((__m512i *)dst, xmm_avg1_512);
                _mm512_storeu_si512((__m512i *)(dst + dst_stride), xmm_avg3_512);
            }

        }
    }
    else
    {
        if (area_width == 16)
        {
            for (y = 0; y < area_height; y += 2) {
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0_stride)), _mm_loadu_si128((__m128i*)(src1 + src1_stride)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storeu_si128((__m128i*) (dst + dst_stride), xmm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
        else if (area_width == 4)
        {
            for (y = 0; y < area_height; y += 2) {

                xmm_avg1 = _mm_avg_epu8(_mm_cvtsi32_si128(*(uint32_t *)src0), _mm_cvtsi32_si128(*(uint32_t *)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_cvtsi32_si128(*(uint32_t *)(src0 + src0_stride)), _mm_cvtsi32_si128(*(uint32_t *)(src1 + src1_stride)));

                *(uint32_t *)dst = _mm_cvtsi128_si32(xmm_avg1);
                *(uint32_t *)(dst + dst_stride) = _mm_cvtsi128_si32(xmm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
        else if (area_width == 8)
        {
            for (y = 0; y < area_height; y += 2) {

                xmm_avg1 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)src0), _mm_loadl_epi64((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + src0_stride)), _mm_loadl_epi64((__m128i*)(src1 + src1_stride)));

                _mm_storel_epi64((__m128i*) dst, xmm_avg1);
                _mm_storel_epi64((__m128i*) (dst + dst_stride), xmm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
        else
        {
            for (y = 0; y < area_height; y += 2) {

                xmm_avg1 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)src0), _mm_loadl_epi64((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_cvtsi32_si128(*(uint32_t *)(src0 + 8)), _mm_cvtsi32_si128(*(uint32_t *)(src1 + 8)));

                xmm_avg3 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + src0_stride)), _mm_loadl_epi64((__m128i*)(src1 + src1_stride)));
                xmm_avg4 = _mm_avg_epu8(_mm_cvtsi32_si128(*(uint32_t *)(src0 + src0_stride + 8)), _mm_cvtsi32_si128(*(uint32_t *)(src1 + src1_stride + 8)));

                _mm_storel_epi64((__m128i*) dst, xmm_avg1);
                *(uint32_t *)(dst + 8) = _mm_cvtsi128_si32(xmm_avg2);
                _mm_storel_epi64((__m128i*) (dst + dst_stride), xmm_avg3);
                *(uint32_t *)(dst + dst_stride + 8) = _mm_cvtsi128_si32(xmm_avg4);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
    }

}
#else
//BiPredAveragingOnthefly
void bi_pred_average_kernel_avx2_intrin(
    EbByte                  src0,
    uint32_t                   src0_stride,
    EbByte                  src1,
    uint32_t                   src1_stride,
    EbByte                  dst,
    uint32_t                   dst_stride,
    uint32_t                   area_width,
    uint32_t                   area_height)
{
    __m128i xmm_avg1, xmm_avg2, xmm_avg3, xmm_avg4, xmm_avg5, xmm_avg6;
    __m256i xmm_avg1_256, xmm_avg2_256, xmm_avg3_256, xmm_avg4_256;
    uint32_t y;

    if (area_width > 16)
    {
        if (area_width == 24)
        {
            for (y = 0; y < area_height; y += 2) {
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + 16)), _mm_loadl_epi64((__m128i*)(src1 + 16)));
                xmm_avg3 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0_stride)), _mm_loadu_si128((__m128i*)(src1 + src1_stride)));
                xmm_avg4 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + src0_stride + 16)), _mm_loadl_epi64((__m128i*)(src1 + src1_stride + 16)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storel_epi64((__m128i*) (dst + 16), xmm_avg2);
                _mm_storeu_si128((__m128i*) (dst + dst_stride), xmm_avg3);
                _mm_storel_epi64((__m128i*) (dst + dst_stride + 16), xmm_avg4);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
        else if (area_width == 32)
        {
            for (y = 0; y < area_height; y += 2) {

                xmm_avg1_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)src0), _mm256_loadu_si256((__m256i*)src1));
                xmm_avg3_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + src0_stride)), _mm256_loadu_si256((__m256i*)(src1 + src1_stride)));

                _mm256_storeu_si256((__m256i *) dst, xmm_avg1_256);
                _mm256_storeu_si256((__m256i *) (dst + dst_stride), xmm_avg3_256);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
        else if (area_width == 48)
        {
            for (y = 0; y < area_height; y += 2) {
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 16)), _mm_loadu_si128((__m128i*)(src1 + 16)));
                xmm_avg3 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 32)), _mm_loadu_si128((__m128i*)(src1 + 32)));

                xmm_avg4 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0_stride)), _mm_loadu_si128((__m128i*)(src1 + src1_stride)));
                xmm_avg5 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0_stride + 16)), _mm_loadu_si128((__m128i*)(src1 + src1_stride + 16)));
                xmm_avg6 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0_stride + 32)), _mm_loadu_si128((__m128i*)(src1 + src1_stride + 32)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storeu_si128((__m128i*) (dst + 16), xmm_avg2);
                _mm_storeu_si128((__m128i*) (dst + 32), xmm_avg3);
                _mm_storeu_si128((__m128i*) (dst + dst_stride), xmm_avg4);
                _mm_storeu_si128((__m128i*) (dst + dst_stride + 16), xmm_avg5);
                _mm_storeu_si128((__m128i*) (dst + dst_stride + 32), xmm_avg6);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;

            }
        }
        else
        {
            for (y = 0; y < area_height; y += 2) {
                xmm_avg1_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)src0), _mm256_loadu_si256((__m256i*)src1));
                xmm_avg2_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + 32)), _mm256_loadu_si256((__m256i*)(src1 + 32)));
                xmm_avg3_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + src0_stride)), _mm256_loadu_si256((__m256i*)(src1 + src1_stride)));
                xmm_avg4_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + src0_stride + 32)), _mm256_loadu_si256((__m256i*)(src1 + src1_stride + 32)));

                _mm256_storeu_si256((__m256i *) dst, xmm_avg1_256);
                _mm256_storeu_si256((__m256i *) (dst + 32), xmm_avg2_256);

                _mm256_storeu_si256((__m256i *) (dst + dst_stride), xmm_avg3_256);
                _mm256_storeu_si256((__m256i *) (dst + dst_stride + 32), xmm_avg4_256);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
    }
    else
    {
        if (area_width == 16)
        {
            for (y = 0; y < area_height; y += 2) {
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0_stride)), _mm_loadu_si128((__m128i*)(src1 + src1_stride)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storeu_si128((__m128i*) (dst + dst_stride), xmm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
        else if (area_width == 4)
        {
            for (y = 0; y < area_height; y += 2) {

                xmm_avg1 = _mm_avg_epu8(_mm_cvtsi32_si128(*(uint32_t *)src0), _mm_cvtsi32_si128(*(uint32_t *)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_cvtsi32_si128(*(uint32_t *)(src0 + src0_stride)), _mm_cvtsi32_si128(*(uint32_t *)(src1 + src1_stride)));

                *(uint32_t *)dst = _mm_cvtsi128_si32(xmm_avg1);
                *(uint32_t *)(dst + dst_stride) = _mm_cvtsi128_si32(xmm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
        else if (area_width == 8)
        {
            for (y = 0; y < area_height; y += 2) {

                xmm_avg1 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)src0), _mm_loadl_epi64((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + src0_stride)), _mm_loadl_epi64((__m128i*)(src1 + src1_stride)));

                _mm_storel_epi64((__m128i*) dst, xmm_avg1);
                _mm_storel_epi64((__m128i*) (dst + dst_stride), xmm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
        else
        {
            for (y = 0; y < area_height; y += 2) {

                xmm_avg1 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)src0), _mm_loadl_epi64((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_cvtsi32_si128(*(uint32_t *)(src0 + 8)), _mm_cvtsi32_si128(*(uint32_t *)(src1 + 8)));

                xmm_avg3 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + src0_stride)), _mm_loadl_epi64((__m128i*)(src1 + src1_stride)));
                xmm_avg4 = _mm_avg_epu8(_mm_cvtsi32_si128(*(uint32_t *)(src0 + src0_stride + 8)), _mm_cvtsi32_si128(*(uint32_t *)(src1 + src1_stride + 8)));

                _mm_storel_epi64((__m128i*) dst, xmm_avg1);
                *(uint32_t *)(dst + 8) = _mm_cvtsi128_si32(xmm_avg2);
                _mm_storel_epi64((__m128i*) (dst + dst_stride), xmm_avg3);
                *(uint32_t *)(dst + dst_stride + 8) = _mm_cvtsi128_si32(xmm_avg4);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
    }

}
#endif
