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

static __m128i _mm_loadhi_epi64(__m128i x, __m128i *p)
{
    return _mm_castpd_si128(_mm_loadh_pd(_mm_castsi128_pd(x), (double *)p));
}

void eb_vp9_compressed_packmsb_avx2_intrin(
    uint8_t     *in8_bit_buffer,
    uint32_t     in8_stride,
    uint8_t     *inn_bit_buffer,
    uint16_t    *out16_bit_buffer,
    uint32_t     inn_stride,
    uint32_t     out_stride,
    uint32_t     width,
    uint32_t     height)
{

    uint32_t y;

    if (width == 8)
    {

        __m128i in2_bit, ext0, ext1, ext2, ext3, ext01, ext23, ext0_15, ext16_31;
        __m128i msk0, firstfour_2bit, secondfour_2bit, thirdfour_2bit, fourthfour_2bit;
        __m128i concat0_128, concat1_128, concat2_128, concat3_128, in_8bit_128_0, in_8bit_128_1, in_8bit_stride_128, in_8bit_128;
        msk0 = _mm_set1_epi8((signed char)0xC0);//1100.000

                                                //processing 2 lines for chroma
        for (y = 0; y < height; y += 4)
        {

            firstfour_2bit = _mm_loadl_epi64((__m128i*)inn_bit_buffer);
            secondfour_2bit = _mm_loadl_epi64((__m128i*)(inn_bit_buffer + inn_stride));
            thirdfour_2bit = _mm_loadl_epi64((__m128i*)(inn_bit_buffer + 2 * inn_stride));
            fourthfour_2bit = _mm_loadl_epi64((__m128i*)(inn_bit_buffer + 3 * inn_stride));
            in2_bit = _mm_unpacklo_epi32(_mm_unpacklo_epi16(firstfour_2bit, secondfour_2bit), _mm_unpacklo_epi16(thirdfour_2bit, fourthfour_2bit));

            ext0 = _mm_and_si128(in2_bit, msk0);
            ext1 = _mm_and_si128(_mm_slli_epi16(in2_bit, 2), msk0);
            ext2 = _mm_and_si128(_mm_slli_epi16(in2_bit, 4), msk0);
            ext3 = _mm_and_si128(_mm_slli_epi16(in2_bit, 6), msk0);

            ext01 = _mm_unpacklo_epi8(ext0, ext1);
            ext23 = _mm_unpacklo_epi8(ext2, ext3);
            ext0_15 = _mm_unpacklo_epi16(ext01, ext23);
            ext16_31 = _mm_unpackhi_epi16(ext01, ext23);

            in_8bit_128_0 = _mm_loadl_epi64((__m128i*)in8_bit_buffer);
            in_8bit_128_1 = _mm_loadl_epi64((__m128i*)(in8_bit_buffer + in8_stride));
            in_8bit_128 = _mm_unpacklo_epi64(in_8bit_128_0, in_8bit_128_1);
            in_8bit_128_0 = _mm_loadl_epi64((__m128i*)(in8_bit_buffer + 2 * in8_stride));
            in_8bit_128_1 = _mm_loadl_epi64((__m128i*)(in8_bit_buffer + 3 * in8_stride));
            in_8bit_stride_128 = _mm_unpacklo_epi64(in_8bit_128_0, in_8bit_128_1);

            //(out_pixel | n_bit_pixel) concatenation is done with unpacklo_epi8 and unpackhi_epi8
            concat0_128 = _mm_srli_epi16(_mm_unpacklo_epi8(ext0_15, in_8bit_128), 6);
            concat1_128 = _mm_srli_epi16(_mm_unpackhi_epi8(ext0_15, in_8bit_128), 6);
            concat2_128 = _mm_srli_epi16(_mm_unpacklo_epi8(ext16_31, in_8bit_stride_128), 6);
            concat3_128 = _mm_srli_epi16(_mm_unpackhi_epi8(ext16_31, in_8bit_stride_128), 6);

            _mm_store_si128((__m128i*) out16_bit_buffer, concat0_128);
            _mm_store_si128((__m128i*) (out16_bit_buffer + out_stride), concat1_128);
            _mm_store_si128((__m128i*) (out16_bit_buffer + 2 * out_stride), concat2_128);
            _mm_store_si128((__m128i*) (out16_bit_buffer + 3 * out_stride), concat3_128);

            in8_bit_buffer += in8_stride << 2;
            inn_bit_buffer += inn_stride << 2;
            out16_bit_buffer += out_stride << 2;
        }
    }
    if (width == 16)
    {
        __m256i in_n_bit, in_n_bit_stride, in8_bit, in8_bit_stride, concat0, concat1, concat2, concat3;
        __m128i in2_bit, ext0, ext1, ext2, ext3, ext01, ext23, ext01h, ext23h, ext0_15, ext16_31, ext32_47, ext48_63;
        __m128i msk0, firstfour_2bit, secondfour_2bit, thirdfour_2bit, fourthfour_2bit;
        __m128i  in_8bit_128_0, in_8bit_128_1;
        msk0 = _mm_set1_epi8((signed char)0xC0);//1100.000

                                                //processing 2 lines for chroma
        for (y = 0; y < height; y += 4)
        {

            firstfour_2bit = _mm_loadl_epi64((__m128i*)inn_bit_buffer);
            secondfour_2bit = _mm_loadl_epi64((__m128i*)(inn_bit_buffer + inn_stride));
            thirdfour_2bit = _mm_loadl_epi64((__m128i*)(inn_bit_buffer + 2 * inn_stride));
            fourthfour_2bit = _mm_loadl_epi64((__m128i*)(inn_bit_buffer + 3 * inn_stride));
            in2_bit = _mm_unpacklo_epi64(_mm_unpacklo_epi32(firstfour_2bit, secondfour_2bit), _mm_unpacklo_epi32(thirdfour_2bit, fourthfour_2bit));

            ext0 = _mm_and_si128(in2_bit, msk0);
            ext1 = _mm_and_si128(_mm_slli_epi16(in2_bit, 2), msk0);
            ext2 = _mm_and_si128(_mm_slli_epi16(in2_bit, 4), msk0);
            ext3 = _mm_and_si128(_mm_slli_epi16(in2_bit, 6), msk0);

            ext01 = _mm_unpacklo_epi8(ext0, ext1);
            ext23 = _mm_unpacklo_epi8(ext2, ext3);
            ext0_15 = _mm_unpacklo_epi16(ext01, ext23);
            ext16_31 = _mm_unpackhi_epi16(ext01, ext23);

            ext01h = _mm_unpackhi_epi8(ext0, ext1);
            ext23h = _mm_unpackhi_epi8(ext2, ext3);
            ext32_47 = _mm_unpacklo_epi16(ext01h, ext23h);
            ext48_63 = _mm_unpackhi_epi16(ext01h, ext23h);

            in_n_bit = _mm256_set_m128i(ext16_31, ext0_15);
            in_n_bit_stride = _mm256_set_m128i(ext48_63, ext32_47);

            in_8bit_128_0 = _mm_loadu_si128((__m128i*)in8_bit_buffer);
            in_8bit_128_1 = _mm_loadu_si128((__m128i*)(in8_bit_buffer + in8_stride));
            in8_bit = _mm256_set_m128i(in_8bit_128_1, in_8bit_128_0);
            in_8bit_128_0 = _mm_loadu_si128((__m128i*)(in8_bit_buffer + 2 * in8_stride));
            in_8bit_128_1 = _mm_loadu_si128((__m128i*)(in8_bit_buffer + 3 * in8_stride));
            in8_bit_stride = _mm256_set_m128i(in_8bit_128_1, in_8bit_128_0);

            //(out_pixel | n_bit_pixel) concatenation is done with unpacklo_epi8 and unpackhi_epi8
            concat0 = _mm256_srli_epi16(_mm256_unpacklo_epi8(in_n_bit, in8_bit), 6);
            concat1 = _mm256_srli_epi16(_mm256_unpackhi_epi8(in_n_bit, in8_bit), 6);
            concat2 = _mm256_srli_epi16(_mm256_unpacklo_epi8(in_n_bit_stride, in8_bit_stride), 6);
            concat3 = _mm256_srli_epi16(_mm256_unpackhi_epi8(in_n_bit_stride, in8_bit_stride), 6);
            _mm_store_si128((__m128i*) out16_bit_buffer, _mm256_castsi256_si128(concat0));
            _mm_store_si128((__m128i*) (out16_bit_buffer + 8), _mm256_castsi256_si128(concat1));
            _mm_store_si128((__m128i*) (out16_bit_buffer + out_stride), _mm256_extracti128_si256(concat0, 1));
            _mm_store_si128((__m128i*) (out16_bit_buffer + out_stride + 8), _mm256_extracti128_si256(concat1, 1));

            _mm_store_si128((__m128i*) (out16_bit_buffer + 2 * out_stride), _mm256_castsi256_si128(concat2));
            _mm_store_si128((__m128i*) (out16_bit_buffer + 2 * out_stride + 8), _mm256_castsi256_si128(concat3));
            _mm_store_si128((__m128i*) (out16_bit_buffer + 3 * out_stride), _mm256_extracti128_si256(concat2, 1));
            _mm_store_si128((__m128i*) (out16_bit_buffer + 3 * out_stride + 8), _mm256_extracti128_si256(concat3, 1));
            in8_bit_buffer += in8_stride << 2;
            inn_bit_buffer += inn_stride << 2;
            out16_bit_buffer += out_stride << 2;
        }
    }
    if (width == 32)
    {
        __m256i in_n_bit, in8_bit, in_n_bit_stride, in8_bit_stride, concat0, concat1, concat2, concat3;
        __m128i in2_bit, ext0, ext1, ext2, ext3, ext01, ext23, ext01h, ext23h, ext0_15, ext16_31, ext32_47, ext48_63;
        __m128i msk0, firstfour_2bit, secondfour_2bit;

        msk0 = _mm_set1_epi8((signed char)0xC0);//1100.000

                                                //processing 2 lines for chroma
        for (y = 0; y < height; y += 2)
        {
            firstfour_2bit = _mm_loadl_epi64((__m128i*)inn_bit_buffer);
            secondfour_2bit = _mm_loadl_epi64((__m128i*)(inn_bit_buffer + inn_stride));
            in2_bit = _mm_unpacklo_epi64(firstfour_2bit, secondfour_2bit);

            ext0 = _mm_and_si128(in2_bit, msk0);
            ext1 = _mm_and_si128(_mm_slli_epi16(in2_bit, 2), msk0);
            ext2 = _mm_and_si128(_mm_slli_epi16(in2_bit, 4), msk0);
            ext3 = _mm_and_si128(_mm_slli_epi16(in2_bit, 6), msk0);

            ext01 = _mm_unpacklo_epi8(ext0, ext1);
            ext23 = _mm_unpacklo_epi8(ext2, ext3);
            ext0_15 = _mm_unpacklo_epi16(ext01, ext23);
            ext16_31 = _mm_unpackhi_epi16(ext01, ext23);

            ext01h = _mm_unpackhi_epi8(ext0, ext1);
            ext23h = _mm_unpackhi_epi8(ext2, ext3);
            ext32_47 = _mm_unpacklo_epi16(ext01h, ext23h);
            ext48_63 = _mm_unpackhi_epi16(ext01h, ext23h);

            in_n_bit = _mm256_set_m128i(ext16_31, ext0_15);
            in_n_bit_stride = _mm256_set_m128i(ext48_63, ext32_47);

            in8_bit = _mm256_loadu_si256((__m256i*)in8_bit_buffer);
            in8_bit_stride = _mm256_loadu_si256((__m256i*)(in8_bit_buffer + in8_stride));

            //(out_pixel | n_bit_pixel) concatenation is done with unpacklo_epi8 and unpackhi_epi8
            concat0 = _mm256_srli_epi16(_mm256_unpacklo_epi8(in_n_bit, in8_bit), 6);
            concat1 = _mm256_srli_epi16(_mm256_unpackhi_epi8(in_n_bit, in8_bit), 6);
            concat2 = _mm256_srli_epi16(_mm256_unpacklo_epi8(in_n_bit_stride, in8_bit_stride), 6);
            concat3 = _mm256_srli_epi16(_mm256_unpackhi_epi8(in_n_bit_stride, in8_bit_stride), 6);
            _mm_store_si128((__m128i*) out16_bit_buffer, _mm256_castsi256_si128(concat0));
            _mm_store_si128((__m128i*) (out16_bit_buffer + 8), _mm256_castsi256_si128(concat1));
            _mm_store_si128((__m128i*) (out16_bit_buffer + 16), _mm256_extracti128_si256(concat0, 1));
            _mm_store_si128((__m128i*) (out16_bit_buffer + 24), _mm256_extracti128_si256(concat1, 1));

            _mm_store_si128((__m128i*) (out16_bit_buffer + out_stride), _mm256_castsi256_si128(concat2));
            _mm_store_si128((__m128i*) (out16_bit_buffer + out_stride + 8), _mm256_castsi256_si128(concat3));
            _mm_store_si128((__m128i*) (out16_bit_buffer + out_stride + 16), _mm256_extracti128_si256(concat2, 1));
            _mm_store_si128((__m128i*) (out16_bit_buffer + out_stride + 24), _mm256_extracti128_si256(concat3, 1));

            in8_bit_buffer += in8_stride << 1;
            inn_bit_buffer += inn_stride << 1;
            out16_bit_buffer += out_stride << 1;
        }
    }
    else if (width == 64)
    {
        __m256i in_n_bit, in8_bit, in_n_bit32, in8_bit32;
        __m256i concat0, concat1, concat2, concat3;
        __m128i in2_bit, ext0, ext1, ext2, ext3, ext01, ext23, ext01h, ext23h, ext0_15, ext16_31, ext32_47, ext48_63;
        __m128i msk;

        msk = _mm_set1_epi8((signed char)0xC0);//1100.000

                                               //One row per iter
        for (y = 0; y < height; y++)
        {

            in2_bit = _mm_loadu_si128((__m128i*)inn_bit_buffer);

            ext0 = _mm_and_si128(in2_bit, msk);
            ext1 = _mm_and_si128(_mm_slli_epi16(in2_bit, 2), msk);
            ext2 = _mm_and_si128(_mm_slli_epi16(in2_bit, 4), msk);
            ext3 = _mm_and_si128(_mm_slli_epi16(in2_bit, 6), msk);

            ext01 = _mm_unpacklo_epi8(ext0, ext1);
            ext23 = _mm_unpacklo_epi8(ext2, ext3);
            ext0_15 = _mm_unpacklo_epi16(ext01, ext23);
            ext16_31 = _mm_unpackhi_epi16(ext01, ext23);

            ext01h = _mm_unpackhi_epi8(ext0, ext1);
            ext23h = _mm_unpackhi_epi8(ext2, ext3);
            ext32_47 = _mm_unpacklo_epi16(ext01h, ext23h);
            ext48_63 = _mm_unpackhi_epi16(ext01h, ext23h);

            in_n_bit = _mm256_set_m128i(ext16_31, ext0_15);
            in_n_bit32 = _mm256_set_m128i(ext48_63, ext32_47);

            in8_bit = _mm256_loadu_si256((__m256i*)in8_bit_buffer);
            in8_bit32 = _mm256_loadu_si256((__m256i*)(in8_bit_buffer + 32));

            //(out_pixel | n_bit_pixel) concatenation
            concat0 = _mm256_srli_epi16(_mm256_unpacklo_epi8(in_n_bit, in8_bit), 6);
            concat1 = _mm256_srli_epi16(_mm256_unpackhi_epi8(in_n_bit, in8_bit), 6);
            concat2 = _mm256_srli_epi16(_mm256_unpacklo_epi8(in_n_bit32, in8_bit32), 6);
            concat3 = _mm256_srli_epi16(_mm256_unpackhi_epi8(in_n_bit32, in8_bit32), 6);

            _mm_storeu_si128((__m128i*) out16_bit_buffer, _mm256_castsi256_si128(concat0));
            _mm_storeu_si128((__m128i*) (out16_bit_buffer + 8), _mm256_castsi256_si128(concat1));
            _mm_storeu_si128((__m128i*) (out16_bit_buffer + 16), _mm256_extracti128_si256(concat0, 1));
            _mm_storeu_si128((__m128i*) (out16_bit_buffer + 24), _mm256_extracti128_si256(concat1, 1));

            _mm_storeu_si128((__m128i*) (out16_bit_buffer + 32), _mm256_castsi256_si128(concat2));
            _mm_storeu_si128((__m128i*) (out16_bit_buffer + 40), _mm256_castsi256_si128(concat3));
            _mm_storeu_si128((__m128i*) (out16_bit_buffer + 48), _mm256_extracti128_si256(concat2, 1));
            _mm_storeu_si128((__m128i*) (out16_bit_buffer + 56), _mm256_extracti128_si256(concat3, 1));

            in8_bit_buffer += in8_stride;
            inn_bit_buffer += inn_stride;
            out16_bit_buffer += out_stride;

        }

    }

}

void eb_vp9_c_pack_avx2_intrin(
    const uint8_t     *inn_bit_buffer,
    uint32_t           inn_stride,
    uint8_t           *in_compn_bit_buffer,
    uint32_t           out_stride,
    uint8_t           *local_cache,
    uint32_t           width,
    uint32_t           height)
{

    uint32_t y;

    if (width == 32)
    {
        __m256i in_n_bit;

        __m256i ext0, ext1, ext2, ext3, ext0123, ext0123n, extp;
        __m256i msk0, msk1, msk2, msk3;

        msk0 = _mm256_set1_epi32(0x000000C0);//1100.0000
        msk1 = _mm256_set1_epi32(0x00000030);//0011.0000
        msk2 = _mm256_set1_epi32(0x0000000C);//0000.1100
        msk3 = _mm256_set1_epi32(0x00000003);//0000.0011

        //One row per iter
        for (y = 0; y < height; y++)
        {

            in_n_bit = _mm256_loadu_si256((__m256i*)inn_bit_buffer);

            ext0 = _mm256_and_si256(in_n_bit, msk0);
            ext1 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 1 * 8 + 2), msk1);
            ext2 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 2 * 8 + 4), msk2);
            ext3 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 3 * 8 + 6), msk3);

            ext0123 = _mm256_or_si256(_mm256_or_si256(ext0, ext1), _mm256_or_si256(ext2, ext3));

            ext0123n = _mm256_castsi128_si256(_mm256_extracti128_si256(ext0123, 1));

            extp = _mm256_packus_epi32(ext0123, ext0123n);
            extp = _mm256_packus_epi16(extp, extp);

            _mm_storel_epi64((__m128i*) in_compn_bit_buffer, _mm256_castsi256_si128(extp));
            in_compn_bit_buffer += 8;
            inn_bit_buffer += inn_stride;

        }

    }
    else if (width == 64)
    {
        __m256i in_n_bit;
        __m256i ext0, ext1, ext2, ext3, ext0123, ext0123n, extp, extp1;
        __m256i msk0, msk1, msk2, msk3;

        msk0 = _mm256_set1_epi32(0x000000C0);//1100.0000
        msk1 = _mm256_set1_epi32(0x00000030);//0011.0000
        msk2 = _mm256_set1_epi32(0x0000000C);//0000.1100
        msk3 = _mm256_set1_epi32(0x00000003);//0000.0011
        if (height == 64)
        {

            uint8_t* local_ptr = local_cache;

            for (y = 0; y < height; y++)
            {

                in_n_bit = _mm256_loadu_si256((__m256i*)inn_bit_buffer);

                ext0 = _mm256_and_si256(in_n_bit, msk0);
                ext1 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 1 * 8 + 2), msk1);
                ext2 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 2 * 8 + 4), msk2);
                ext3 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 3 * 8 + 6), msk3);

                ext0123 = _mm256_or_si256(_mm256_or_si256(ext0, ext1), _mm256_or_si256(ext2, ext3));

                ext0123n = _mm256_castsi128_si256(_mm256_extracti128_si256(ext0123, 1));

                extp = _mm256_packus_epi32(ext0123, ext0123n);
                extp = _mm256_packus_epi16(extp, extp);

                in_n_bit = _mm256_loadu_si256((__m256i*)(inn_bit_buffer + 32));

                ext0 = _mm256_and_si256(in_n_bit, msk0);
                ext1 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 1 * 8 + 2), msk1);
                ext2 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 2 * 8 + 4), msk2);
                ext3 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 3 * 8 + 6), msk3);

                ext0123 = _mm256_or_si256(_mm256_or_si256(ext0, ext1), _mm256_or_si256(ext2, ext3));

                ext0123n = _mm256_castsi128_si256(_mm256_extracti128_si256(ext0123, 1));

                extp1 = _mm256_packus_epi32(ext0123, ext0123n);
                extp1 = _mm256_packus_epi16(extp1, extp1);

                extp = _mm256_unpacklo_epi64(extp, extp1);

                _mm_storeu_si128((__m128i*)  (local_ptr + 16 * (y & 3)), _mm256_castsi256_si128(extp));

                if ((y & 3) == 3)
                {
                    __m256i c0 = _mm256_loadu_si256((__m256i*)(local_ptr));
                    __m256i c1 = _mm256_loadu_si256((__m256i*)(local_ptr + 32));
                    _mm256_stream_si256((__m256i*)&in_compn_bit_buffer[0], c0);
                    _mm256_stream_si256((__m256i*)&in_compn_bit_buffer[32], c1);
                    in_compn_bit_buffer += 4 * out_stride;
                }

                inn_bit_buffer += inn_stride;

            }

        }
        else{

            //One row per iter
            for (y = 0; y < height; y++)
            {

                in_n_bit = _mm256_loadu_si256((__m256i*)inn_bit_buffer);

                ext0 = _mm256_and_si256(in_n_bit, msk0);
                ext1 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 1 * 8 + 2), msk1);
                ext2 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 2 * 8 + 4), msk2);
                ext3 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 3 * 8 + 6), msk3);

                ext0123 = _mm256_or_si256(_mm256_or_si256(ext0, ext1), _mm256_or_si256(ext2, ext3));

                ext0123n = _mm256_castsi128_si256(_mm256_extracti128_si256(ext0123, 1));

                extp = _mm256_packus_epi32(ext0123, ext0123n);
                extp = _mm256_packus_epi16(extp, extp);

                in_n_bit = _mm256_loadu_si256((__m256i*)(inn_bit_buffer + 32));

                ext0 = _mm256_and_si256(in_n_bit, msk0);
                ext1 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 1 * 8 + 2), msk1);
                ext2 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 2 * 8 + 4), msk2);
                ext3 = _mm256_and_si256(_mm256_srli_epi32(in_n_bit, 3 * 8 + 6), msk3);

                ext0123 = _mm256_or_si256(_mm256_or_si256(ext0, ext1), _mm256_or_si256(ext2, ext3));

                ext0123n = _mm256_castsi128_si256(_mm256_extracti128_si256(ext0123, 1));

                extp1 = _mm256_packus_epi32(ext0123, ext0123n);
                extp1 = _mm256_packus_epi16(extp1, extp1);

                extp = _mm256_unpacklo_epi64(extp, extp1);

                _mm_storeu_si128((__m128i*)  in_compn_bit_buffer, _mm256_castsi256_si128(extp));

                in_compn_bit_buffer += out_stride;

                inn_bit_buffer += inn_stride;

            }

        }

    }

}

void eb_vp9_enc_msb_pack2_d_avx2_intrin_al(
    uint8_t     *in8_bit_buffer,
    uint32_t     in8_stride,
    uint8_t     *inn_bit_buffer,
    uint16_t    *out16_bit_buffer,
    uint32_t     inn_stride,
    uint32_t     out_stride,
    uint32_t     width,
    uint32_t     height)
{
    //(out_pixel | n_bit_pixel) concatenation is done with unpacklo_epi8 and unpackhi_epi8

    uint32_t y, x;

    __m128i out0, out1;

    if (width == 4)
    {
        for (y = 0; y < height; y += 2){

            out0 = _mm_srli_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*(uint32_t *)inn_bit_buffer), _mm_cvtsi32_si128(*(uint32_t *)in8_bit_buffer)), 6);
            out1 = _mm_srli_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*(uint32_t *)(inn_bit_buffer + inn_stride)), _mm_cvtsi32_si128(*(uint32_t *)(in8_bit_buffer + in8_stride))), 6);

            _mm_storel_epi64((__m128i*) out16_bit_buffer, out0);
            _mm_storel_epi64((__m128i*) (out16_bit_buffer + out_stride), out1);

            in8_bit_buffer += in8_stride << 1;
            inn_bit_buffer += inn_stride << 1;
            out16_bit_buffer += out_stride << 1;
        }
    }
    else if (width == 8)
    {
        for (y = 0; y < height; y += 2){

            out0 = _mm_srli_epi16(_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)inn_bit_buffer), _mm_loadl_epi64((__m128i*)in8_bit_buffer)), 6);
            out1 = _mm_srli_epi16(_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(inn_bit_buffer + inn_stride)), _mm_loadl_epi64((__m128i*)(in8_bit_buffer + in8_stride))), 6);

            _mm_storeu_si128((__m128i*) out16_bit_buffer, out0);
            _mm_storeu_si128((__m128i*) (out16_bit_buffer + out_stride), out1);

            in8_bit_buffer += in8_stride << 1;
            inn_bit_buffer += inn_stride << 1;
            out16_bit_buffer += out_stride << 1;
        }
    }
    else if (width == 16)
    {
        __m128i in_n_bit, in8_bit, in_n_bit_stride, in8_bit_stride, out0, out1, out2, out3;

        for (y = 0; y < height; y += 2){
            in_n_bit = _mm_loadu_si128((__m128i*)inn_bit_buffer);
            in8_bit = _mm_loadu_si128((__m128i*)in8_bit_buffer);
            in_n_bit_stride = _mm_loadu_si128((__m128i*)(inn_bit_buffer + inn_stride));
            in8_bit_stride = _mm_loadu_si128((__m128i*)(in8_bit_buffer + in8_stride));

            out0 = _mm_srli_epi16(_mm_unpacklo_epi8(in_n_bit, in8_bit), 6);
            out1 = _mm_srli_epi16(_mm_unpackhi_epi8(in_n_bit, in8_bit), 6);
            out2 = _mm_srli_epi16(_mm_unpacklo_epi8(in_n_bit_stride, in8_bit_stride), 6);
            out3 = _mm_srli_epi16(_mm_unpackhi_epi8(in_n_bit_stride, in8_bit_stride), 6);

            _mm_storeu_si128((__m128i*) out16_bit_buffer, out0);
            _mm_storeu_si128((__m128i*) (out16_bit_buffer + 8), out1);
            _mm_storeu_si128((__m128i*) (out16_bit_buffer + out_stride), out2);
            _mm_storeu_si128((__m128i*) (out16_bit_buffer + out_stride + 8), out3);

            in8_bit_buffer += in8_stride << 1;
            inn_bit_buffer += inn_stride << 1;
            out16_bit_buffer += out_stride << 1;
        }
    }
    else if (width == 32)
    {
        __m256i in_n_bit, in8_bit, in_n_bit_stride, in8_bit_stride, concat0, concat1, concat2, concat3;
        __m256i out0_15, out16_31, out_s0_s15, out_s16_s31;

        for (y = 0; y < height; y += 2){

            in_n_bit = _mm256_loadu_si256((__m256i*)inn_bit_buffer);
            in8_bit = _mm256_loadu_si256((__m256i*)in8_bit_buffer);
            in_n_bit_stride = _mm256_loadu_si256((__m256i*)(inn_bit_buffer + inn_stride));
            in8_bit_stride = _mm256_loadu_si256((__m256i*)(in8_bit_buffer + in8_stride));

            //(out_pixel | n_bit_pixel) concatenation is done with unpacklo_epi8 and unpackhi_epi8
            concat0 = _mm256_srli_epi16(_mm256_unpacklo_epi8(in_n_bit, in8_bit), 6);
            concat1 = _mm256_srli_epi16(_mm256_unpackhi_epi8(in_n_bit, in8_bit), 6);
            concat2 = _mm256_srli_epi16(_mm256_unpacklo_epi8(in_n_bit_stride, in8_bit_stride), 6);
            concat3 = _mm256_srli_epi16(_mm256_unpackhi_epi8(in_n_bit_stride, in8_bit_stride), 6);

            //Re-organize the packing for writing to the out buffer
            out0_15 = _mm256_inserti128_si256(concat0, _mm256_extracti128_si256(concat1, 0), 1);
            out16_31 = _mm256_inserti128_si256(concat1, _mm256_extracti128_si256(concat0, 1), 0);
            out_s0_s15 = _mm256_inserti128_si256(concat2, _mm256_extracti128_si256(concat3, 0), 1);
            out_s16_s31 = _mm256_inserti128_si256(concat3, _mm256_extracti128_si256(concat2, 1), 0);

            _mm256_store_si256((__m256i*) out16_bit_buffer, out0_15);
            _mm256_store_si256((__m256i*) (out16_bit_buffer + 16), out16_31);
            _mm256_store_si256((__m256i*) (out16_bit_buffer + out_stride), out_s0_s15);
            _mm256_store_si256((__m256i*) (out16_bit_buffer + out_stride + 16), out_s16_s31);

            in8_bit_buffer += in8_stride << 1;
            //inn_bit_buffer += inn_stride << 1;
            inn_bit_buffer += inn_stride *2;
            out16_bit_buffer += out_stride << 1;
        }
    }
    else if (width == 64)
    {
        __m256i in_n_bit, in8_bit, in_n_bit_stride, in8_bit_stride, in_n_bit32, in8_bit32, inNBitStride32, in8BitStride32;
        __m256i concat0, concat1, concat2, concat3, concat4, concat5, concat6, concat7;
        __m256i out_0_15, out16_31, out32_47, out_48_63, out_s0_s15, out_s16_s31, out_s32_s47, out_s48_s63;

        for (y = 0; y < height; y += 2){

            in_n_bit = _mm256_loadu_si256((__m256i*)inn_bit_buffer);
            in8_bit = _mm256_loadu_si256((__m256i*)in8_bit_buffer);
            in_n_bit32 = _mm256_loadu_si256((__m256i*)(inn_bit_buffer + 32));
            in8_bit32 = _mm256_loadu_si256((__m256i*)(in8_bit_buffer + 32));
            in_n_bit_stride = _mm256_loadu_si256((__m256i*)(inn_bit_buffer + inn_stride));
            in8_bit_stride = _mm256_loadu_si256((__m256i*)(in8_bit_buffer + in8_stride));
            inNBitStride32 = _mm256_loadu_si256((__m256i*)(inn_bit_buffer + inn_stride + 32));
            in8BitStride32 = _mm256_loadu_si256((__m256i*)(in8_bit_buffer + in8_stride + 32));
            //(out_pixel | n_bit_pixel) concatenation is done with unpacklo_epi8 and unpackhi_epi8
            concat0 = _mm256_srli_epi16(_mm256_unpacklo_epi8(in_n_bit, in8_bit), 6);
            concat1 = _mm256_srli_epi16(_mm256_unpackhi_epi8(in_n_bit, in8_bit), 6);
            concat2 = _mm256_srli_epi16(_mm256_unpacklo_epi8(in_n_bit32, in8_bit32), 6);
            concat3 = _mm256_srli_epi16(_mm256_unpackhi_epi8(in_n_bit32, in8_bit32), 6);
            concat4 = _mm256_srli_epi16(_mm256_unpacklo_epi8(in_n_bit_stride, in8_bit_stride), 6);
            concat5 = _mm256_srli_epi16(_mm256_unpackhi_epi8(in_n_bit_stride, in8_bit_stride), 6);
            concat6 = _mm256_srli_epi16(_mm256_unpacklo_epi8(inNBitStride32, in8BitStride32), 6);
            concat7 = _mm256_srli_epi16(_mm256_unpackhi_epi8(inNBitStride32, in8BitStride32), 6);

            //Re-organize the packing for writing to the out buffer
            out_0_15 = _mm256_inserti128_si256(concat0, _mm256_extracti128_si256(concat1, 0), 1);
            out16_31 = _mm256_inserti128_si256(concat1, _mm256_extracti128_si256(concat0, 1), 0);
            out32_47 = _mm256_inserti128_si256(concat2, _mm256_extracti128_si256(concat3, 0), 1);
            out_48_63 = _mm256_inserti128_si256(concat3, _mm256_extracti128_si256(concat2, 1), 0);
            out_s0_s15 = _mm256_inserti128_si256(concat4, _mm256_extracti128_si256(concat5, 0), 1);
            out_s16_s31 = _mm256_inserti128_si256(concat5, _mm256_extracti128_si256(concat4, 1), 0);
            out_s32_s47 = _mm256_inserti128_si256(concat6, _mm256_extracti128_si256(concat7, 0), 1);
            out_s48_s63 = _mm256_inserti128_si256(concat7, _mm256_extracti128_si256(concat6, 1), 0);

            _mm256_store_si256((__m256i*) out16_bit_buffer, out_0_15);
            _mm256_store_si256((__m256i*) (out16_bit_buffer + 16), out16_31);
            _mm256_store_si256((__m256i*) (out16_bit_buffer + 32), out32_47);
            _mm256_store_si256((__m256i*) (out16_bit_buffer + 48), out_48_63);

            _mm256_store_si256((__m256i*) (out16_bit_buffer + out_stride), out_s0_s15);
            _mm256_store_si256((__m256i*) (out16_bit_buffer + out_stride + 16), out_s16_s31);
            _mm256_store_si256((__m256i*) (out16_bit_buffer + out_stride + 32), out_s32_s47);
            _mm256_store_si256((__m256i*) (out16_bit_buffer + out_stride + 48), out_s48_s63);

            in8_bit_buffer += in8_stride << 1;
            //inn_bit_buffer += inn_stride << 1;
            inn_bit_buffer += inn_stride *2;
            out16_bit_buffer += out_stride << 1;
        }
    }
    else
    {
        uint32_t inn_stride_diff = 2 * inn_stride;
        uint32_t in8_stride_diff = 2 * in8_stride;
        uint32_t out_stride_diff = 2 * out_stride;
        inn_stride_diff -= width;
        in8_stride_diff -= width;
        out_stride_diff -= width;

        if (!(width & 7)){

            for (x = 0; x < height; x += 2){
                for (y = 0; y < width; y += 8){

                    out0 = _mm_srli_epi16(_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)inn_bit_buffer), _mm_loadl_epi64((__m128i*)in8_bit_buffer)), 6);
                    out1 = _mm_srli_epi16(_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(inn_bit_buffer + inn_stride)), _mm_loadl_epi64((__m128i*)(in8_bit_buffer + in8_stride))), 6);

                    _mm_storeu_si128((__m128i*) out16_bit_buffer, out0);
                    _mm_storeu_si128((__m128i*) (out16_bit_buffer + out_stride), out1);

                    in8_bit_buffer += 8;
                    inn_bit_buffer += 8;
                    out16_bit_buffer += 8;
                }
                in8_bit_buffer += in8_stride_diff;
                inn_bit_buffer += inn_stride_diff;
                out16_bit_buffer += out_stride_diff;
            }
        }
        else{
            for (x = 0; x < height; x += 2){
                for (y = 0; y < width; y += 4){

                    out0 = _mm_srli_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*(uint32_t *)inn_bit_buffer), _mm_cvtsi32_si128(*(uint32_t *)in8_bit_buffer)), 6);
                    out1 = _mm_srli_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*(uint32_t *)(inn_bit_buffer + inn_stride)), _mm_cvtsi32_si128(*(uint32_t *)(in8_bit_buffer + in8_stride))), 6);

                    _mm_storel_epi64((__m128i*) out16_bit_buffer, out0);
                    _mm_storel_epi64((__m128i*) (out16_bit_buffer + out_stride), out1);

                    in8_bit_buffer += 4;
                    inn_bit_buffer += 4;
                    out16_bit_buffer += 4;
                }
                in8_bit_buffer += in8_stride_diff;
                inn_bit_buffer += inn_stride_diff;
                out16_bit_buffer += out_stride_diff;
            }
        }
    }
}

#define ALSTORE  1
#define B256     1

void eb_vp9_unpack_avg_avx2_intrin(
        uint16_t *ref16_l0,
        uint32_t  ref_l0_stride,
        uint16_t *ref16_l1,
        uint32_t  ref_l1_stride,
        uint8_t  *dst_ptr,
        uint32_t  dst_stride,
        uint32_t  width,
        uint32_t  height)
{

    uint32_t   y;
    __m128i in_pixel0, in_pixel1;

    if (width == 4)
    {
        __m128i out8_0_U8_L0, out8_0_U8_L1;
        __m128i avg8_0_U8;

        for (y = 0; y < height; y += 2)
        {
            //--------
            //Line One
            //--------

            //List0
            in_pixel0 = _mm_loadl_epi64((__m128i*)ref16_l0);
            in_pixel1 = _mm_srli_epi16(in_pixel0, 2) ;
            out8_0_U8_L0 = _mm_packus_epi16( in_pixel1 , in_pixel1 );

            //List1
            in_pixel0 = _mm_loadl_epi64((__m128i*)ref16_l1);
            in_pixel1 = _mm_srli_epi16(in_pixel0, 2) ;
            out8_0_U8_L1 = _mm_packus_epi16( in_pixel1 , in_pixel1 );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);

            *(uint32_t*)dst_ptr = _mm_cvtsi128_si32(avg8_0_U8);

            //--------
            //Line Two
            //--------

            //List0
            in_pixel0 = _mm_loadl_epi64((__m128i*)(ref16_l0+ ref_l0_stride) );
            in_pixel1 = _mm_srli_epi16(in_pixel0, 2) ;
            out8_0_U8_L0 = _mm_packus_epi16( in_pixel1 , in_pixel1 );

            //List1

            in_pixel0 = _mm_loadl_epi64((__m128i*)(ref16_l1+ ref_l1_stride) );
            in_pixel1 = _mm_srli_epi16(in_pixel0, 2) ;
            out8_0_U8_L1 = _mm_packus_epi16( in_pixel1 , in_pixel1 );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);

            *(uint32_t*)(dst_ptr+dst_stride) = _mm_cvtsi128_si32(avg8_0_U8);

            dst_ptr  += 2*dst_stride;
            ref16_l0 += 2*ref_l0_stride;
            ref16_l1 += 2*ref_l1_stride;

        }

    }
    else if (width == 8)
    {

        __m128i out8_0_U8_L0, out8_0_U8_L1, out8_2_U8_L0,out8_2_U8_L1;
        __m128i avg8_0_U8,avg8_2_U8;

        for (y = 0; y < height; y += 2)
        {
            //--------
            //Line One
            //--------

            //List0

            in_pixel0 = _mm_loadu_si128((__m128i*) ref16_l0);

            in_pixel1 = _mm_srli_epi16(in_pixel0, 2) ;
            out8_0_U8_L0 = _mm_packus_epi16( in_pixel1 , in_pixel1 );

             //List1

            in_pixel0 = _mm_loadu_si128((__m128i*) ref16_l1);

            in_pixel1 = _mm_srli_epi16(in_pixel0, 2) ;
            out8_0_U8_L1 = _mm_packus_epi16( in_pixel1 , in_pixel1 );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);

            _mm_storel_epi64((__m128i*) dst_ptr      , avg8_0_U8);

            //--------
            //Line Two
            //--------

            //List0

            in_pixel0 = _mm_loadu_si128((__m128i*)(ref16_l0 + ref_l0_stride) );

            in_pixel1 = _mm_srli_epi16(in_pixel0, 2) ;
            out8_2_U8_L0 = _mm_packus_epi16( in_pixel1 , in_pixel1 );

            //List1

            in_pixel0 = _mm_loadu_si128((__m128i*)(ref16_l1 + ref_l1_stride) );

            in_pixel1 = _mm_srli_epi16(in_pixel0, 2) ;
            out8_2_U8_L1 = _mm_packus_epi16( in_pixel1 , in_pixel1 );

            //AVG
            avg8_2_U8 =  _mm_avg_epu8 (out8_2_U8_L0 , out8_2_U8_L1);

            _mm_storel_epi64((__m128i*)(dst_ptr +dst_stride)    , avg8_2_U8);

            dst_ptr  += 2*dst_stride;
            ref16_l0 += 2*ref_l0_stride;
            ref16_l1 += 2*ref_l1_stride;
        }

    }
    else if (width == 16)
    {

        __m128i in_pixel4, in_pixel5;
        __m128i out8_0_U8_L0, out8_0_U8_L1, out8_2_U8_L0,out8_2_U8_L1;
        __m128i avg8_0_U8,avg8_2_U8;

        for (y = 0; y < height; y += 2)
        {
            //--------
            //Line One
            //--------

            //List0

            in_pixel0 = _mm_loadu_si128((__m128i*)  ref16_l0);
            in_pixel1 = _mm_loadu_si128((__m128i*) (ref16_l0 + 8));

            out8_0_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel0, 2)  ,  _mm_srli_epi16(in_pixel1, 2)  );

             //List1

            in_pixel0 = _mm_loadu_si128((__m128i*) ref16_l1);
            in_pixel1 = _mm_loadu_si128((__m128i*)(ref16_l1 + 8));

            out8_0_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel0, 2)  ,  _mm_srli_epi16(in_pixel1, 2)  );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);
#if ALSTORE
            _mm_store_si128((__m128i*) dst_ptr      , avg8_0_U8);
#else
            _mm_storeu_si128((__m128i*) dst_ptr      , avg8_0_U8);
#endif

            //--------
            //Line Two
            //--------

            //List0

            in_pixel4 = _mm_loadu_si128((__m128i*) (ref16_l0 + ref_l0_stride));
            in_pixel5 = _mm_loadu_si128((__m128i*) (ref16_l0 + ref_l0_stride + 8));

               out8_2_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel4, 2)  ,  _mm_srli_epi16(in_pixel5, 2)  );

            //List1

            in_pixel4 = _mm_loadu_si128((__m128i*) (ref16_l1 + ref_l1_stride));
            in_pixel5 = _mm_loadu_si128((__m128i*) (ref16_l1 + ref_l1_stride + 8));

               out8_2_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel4, 2)  ,  _mm_srli_epi16(in_pixel5, 2)  );

            //AVG
            avg8_2_U8 =  _mm_avg_epu8 (out8_2_U8_L0 , out8_2_U8_L1);
#if ALSTORE
            _mm_store_si128((__m128i*)(dst_ptr  + dst_stride      ) , avg8_2_U8);
#else
            _mm_storeu_si128((__m128i*)(dst_ptr  + dst_stride      ) , avg8_2_U8);
#endif
            dst_ptr  += 2*dst_stride;
            ref16_l0 += 2*ref_l0_stride;
            ref16_l1 += 2*ref_l1_stride;

        }

    }
    else if (width == 32)
    {

#if B256
        __m256i in_val16b0,in_val16b1;
        __m256i data8b_32_0_L0,data8b_32_0_L1;
        __m256i avg8b_32_0;
#else
        __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
        __m128i out8_0_U8_L0, out8_1_U8_L0, out8_2_U8_L0, out8_3_U8_L0;
        __m128i out8_0_U8_L1, out8_1_U8_L1, out8_2_U8_L1, out8_3_U8_L1;
        __m128i avg8_0_U8, avg8_1_U8, avg8_2_U8, avg8_3_U8;
#endif

       for (y = 0; y < height; y += 2)
        {

#if B256
            //--------
            //Line One
            //--------

            //List0
            in_val16b0 = _mm256_loadu_si256((__m256i*) ref16_l0);
            in_val16b1 = _mm256_loadu_si256((__m256i*)(ref16_l0 + 16));
            data8b_32_0_L0 = _mm256_packus_epi16(  _mm256_srli_epi16 (in_val16b0, 2) ,  _mm256_srli_epi16 (in_val16b1, 2));
            //List1
            in_val16b0 = _mm256_loadu_si256((__m256i*) ref16_l1);
            in_val16b1 = _mm256_loadu_si256((__m256i*)(ref16_l1 + 16));
            data8b_32_0_L1 = _mm256_packus_epi16(  _mm256_srli_epi16 (in_val16b0, 2) ,  _mm256_srli_epi16 (in_val16b1, 2));

            //Avg
            avg8b_32_0 = _mm256_avg_epu8(data8b_32_0_L0,data8b_32_0_L1);

            avg8b_32_0 = _mm256_permute4x64_epi64(avg8b_32_0, 216);

            _mm256_storeu_si256((__m256i *)(dst_ptr     ), avg8b_32_0);

            //--------
            //Line Two
            //--------
              //List0
            in_val16b0 = _mm256_loadu_si256((__m256i*)(ref16_l0 + ref_l0_stride     ));
            in_val16b1 = _mm256_loadu_si256((__m256i*)(ref16_l0 + ref_l0_stride + 16));

            data8b_32_0_L0 = _mm256_packus_epi16(  _mm256_srli_epi16 (in_val16b0, 2) ,  _mm256_srli_epi16 (in_val16b1, 2));

            //List1
            in_val16b0 = _mm256_loadu_si256((__m256i*)(ref16_l1 + ref_l1_stride     ));
            in_val16b1 = _mm256_loadu_si256((__m256i*)(ref16_l1 + ref_l1_stride + 16));

            data8b_32_0_L1 = _mm256_packus_epi16(  _mm256_srli_epi16 (in_val16b0, 2) ,  _mm256_srli_epi16 (in_val16b1, 2));

            //Avg
            avg8b_32_0 = _mm256_avg_epu8(data8b_32_0_L0,data8b_32_0_L1);

            avg8b_32_0 = _mm256_permute4x64_epi64(avg8b_32_0, 216);

            _mm256_storeu_si256((__m256i *)(dst_ptr + dst_stride   ), avg8b_32_0);

#else
            //--------
            //Line One
            //--------

            //List0

            in_pixel0 = _mm_loadu_si128((__m128i*)  ref16_l0);
            in_pixel1 = _mm_loadu_si128((__m128i*) (ref16_l0 + 8));
            in_pixel2 = _mm_loadu_si128((__m128i*) (ref16_l0 + 16));
            inPixel3 = _mm_loadu_si128((__m128i*) (ref16_l0 + 24));

            out8_0_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel0, 2)  ,  _mm_srli_epi16(in_pixel1, 2)  );
            out8_1_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel2, 2)  ,  _mm_srli_epi16(inPixel3, 2)  );

             //List1

            in_pixel0 = _mm_loadu_si128((__m128i*) ref16_l1);
            in_pixel1 = _mm_loadu_si128((__m128i*)(ref16_l1 + 8));
            in_pixel2 = _mm_loadu_si128((__m128i*)(ref16_l1 + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(ref16_l1 + 24));

            out8_0_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel0, 2)  ,  _mm_srli_epi16(in_pixel1, 2)  );
            out8_1_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel2, 2)  ,  _mm_srli_epi16(inPixel3, 2)  );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);
            avg8_1_U8 =  _mm_avg_epu8 (out8_1_U8_L0 , out8_1_U8_L1);
#if ALSTORE
            _mm_store_si128((__m128i*) dst_ptr      , avg8_0_U8);
            _mm_store_si128((__m128i*)(dst_ptr + 16), avg8_1_U8);
#else
            _mm_storeu_si128((__m128i*) dst_ptr      , avg8_0_U8);
            _mm_storeu_si128((__m128i*)(dst_ptr + 16), avg8_1_U8);
#endif

            //--------
            //Line Two
            //--------

            //List0

            in_pixel4 = _mm_loadu_si128((__m128i*) (ref16_l0 + ref_l0_stride));
            in_pixel5 = _mm_loadu_si128((__m128i*) (ref16_l0 + ref_l0_stride + 8));
            in_pixel6 = _mm_loadu_si128((__m128i*) (ref16_l0 + ref_l0_stride + 16));
            in_pixel7 = _mm_loadu_si128((__m128i*) (ref16_l0 + ref_l0_stride + 24));

               out8_2_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel4, 2)  ,  _mm_srli_epi16(in_pixel5, 2)  );
            out8_3_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel6, 2)  ,  _mm_srli_epi16(in_pixel7, 2)  );

            //List1

            in_pixel4 = _mm_loadu_si128((__m128i*) (ref16_l1 + ref_l1_stride));
            in_pixel5 = _mm_loadu_si128((__m128i*) (ref16_l1 + ref_l1_stride + 8));
            in_pixel6 = _mm_loadu_si128((__m128i*) (ref16_l1 + ref_l1_stride + 16));
            in_pixel7 = _mm_loadu_si128((__m128i*) (ref16_l1 + ref_l1_stride + 24));

               out8_2_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel4, 2)  ,  _mm_srli_epi16(in_pixel5, 2)  );
            out8_3_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel6, 2)  ,  _mm_srli_epi16(in_pixel7, 2)  );

            //AVG
            avg8_2_U8 =  _mm_avg_epu8 (out8_2_U8_L0 , out8_2_U8_L1);
            avg8_3_U8 =  _mm_avg_epu8 (out8_3_U8_L0 , out8_3_U8_L1);
#if ALSTORE
            _mm_store_si128((__m128i*)(dst_ptr  + dst_stride      ) , avg8_2_U8);
            _mm_store_si128((__m128i*)(dst_ptr  + dst_stride + 16 ) , avg8_3_U8);
#else
            _mm_storeu_si128((__m128i*)(dst_ptr  + dst_stride      ) , avg8_2_U8);
            _mm_storeu_si128((__m128i*)(dst_ptr  + dst_stride + 16 ) , avg8_3_U8);
#endif

#endif
            dst_ptr  += 2*dst_stride;
            ref16_l0 += 2*ref_l0_stride;
            ref16_l1 += 2*ref_l1_stride;

        }

    }
    else if (width == 64)
    {

#if B256
        __m256i in_val16b0,in_val16b1,inVal16b_2,inVal16b_3;
        __m256i data8b_32_0_L0,data8b_32_1_L0,data8b_32_0_L1,data8b_32_1_L1;
        __m256i avg8b_32_0,avg8b_32_1;
#else
        __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
        __m128i out8_0_U8_L0, out8_1_U8_L0, out8_2_U8_L0, out8_3_U8_L0;
        __m128i out8_0_U8_L1, out8_1_U8_L1, out8_2_U8_L1, out8_3_U8_L1;
        __m128i avg8_0_U8, avg8_1_U8, avg8_2_U8, avg8_3_U8;

#endif

        for (y = 0; y < height; ++y)
        {

#if B256       // _mm256_lddqu_si256

            //List0
            in_val16b0 = _mm256_loadu_si256((__m256i*) ref16_l0);
            in_val16b1 = _mm256_loadu_si256((__m256i*)(ref16_l0 + 16));
            inVal16b_2 = _mm256_loadu_si256((__m256i*)(ref16_l0 + 32));
            inVal16b_3 = _mm256_loadu_si256((__m256i*)(ref16_l0 + 48));
            data8b_32_0_L0 = _mm256_packus_epi16(  _mm256_srli_epi16 (in_val16b0, 2) ,  _mm256_srli_epi16 (in_val16b1, 2));
            data8b_32_1_L0 = _mm256_packus_epi16(  _mm256_srli_epi16 (inVal16b_2, 2) ,  _mm256_srli_epi16 (inVal16b_3, 2));
            //List1
            in_val16b0 = _mm256_loadu_si256((__m256i*) ref16_l1);
            in_val16b1 = _mm256_loadu_si256((__m256i*)(ref16_l1 + 16));
            inVal16b_2 = _mm256_loadu_si256((__m256i*)(ref16_l1 + 32));
            inVal16b_3 = _mm256_loadu_si256((__m256i*)(ref16_l1 + 48));
            data8b_32_0_L1 = _mm256_packus_epi16(  _mm256_srli_epi16 (in_val16b0, 2) ,  _mm256_srli_epi16 (in_val16b1, 2));
            data8b_32_1_L1 = _mm256_packus_epi16(  _mm256_srli_epi16 (inVal16b_2, 2) ,  _mm256_srli_epi16 (inVal16b_3, 2));

            //Avg
            avg8b_32_0 = _mm256_avg_epu8(data8b_32_0_L0,data8b_32_0_L1);
            avg8b_32_1 = _mm256_avg_epu8(data8b_32_1_L0,data8b_32_1_L1);

            avg8b_32_0 = _mm256_permute4x64_epi64(avg8b_32_0, 216);
            avg8b_32_1 = _mm256_permute4x64_epi64(avg8b_32_1, 216);

            _mm256_storeu_si256((__m256i *)(dst_ptr     ), avg8b_32_0);
            _mm256_storeu_si256((__m256i *)(dst_ptr + 32), avg8b_32_1);
#else
            //List0
            in_pixel0 = _mm_loadu_si128((__m128i*) ref16_l0);
            in_pixel1 = _mm_loadu_si128((__m128i*)(ref16_l0 + 8));
            in_pixel2 = _mm_loadu_si128((__m128i*)(ref16_l0 + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(ref16_l0 + 24));
            in_pixel4 = _mm_loadu_si128((__m128i*)(ref16_l0 + 32));
            in_pixel5 = _mm_loadu_si128((__m128i*)(ref16_l0 + 40));
            in_pixel6 = _mm_loadu_si128((__m128i*)(ref16_l0 + 48));
            in_pixel7 = _mm_loadu_si128((__m128i*)(ref16_l0 + 56));

            out8_0_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel0, 2)  ,  _mm_srli_epi16(in_pixel1, 2)  );
            out8_1_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel2, 2)  ,  _mm_srli_epi16(inPixel3, 2)  );
            out8_2_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel4, 2)  ,  _mm_srli_epi16(in_pixel5, 2)  );
            out8_3_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel6, 2)  ,  _mm_srli_epi16(in_pixel7, 2)  );

            //List1
            in_pixel0 = _mm_loadu_si128((__m128i*) ref16_l1);
            in_pixel1 = _mm_loadu_si128((__m128i*)(ref16_l1 + 8));
            in_pixel2 = _mm_loadu_si128((__m128i*)(ref16_l1 + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(ref16_l1 + 24));
            in_pixel4 = _mm_loadu_si128((__m128i*)(ref16_l1 + 32));
            in_pixel5 = _mm_loadu_si128((__m128i*)(ref16_l1 + 40));
            in_pixel6 = _mm_loadu_si128((__m128i*)(ref16_l1 + 48));
            in_pixel7 = _mm_loadu_si128((__m128i*)(ref16_l1 + 56));

            //Note: old Version used to use _mm_and_si128 to mask the MSB bits of the pixels
            out8_0_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel0, 2)  ,  _mm_srli_epi16(in_pixel1, 2)  );
            out8_1_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel2, 2)  ,  _mm_srli_epi16(inPixel3, 2)  );
            out8_2_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel4, 2)  ,  _mm_srli_epi16(in_pixel5, 2)  );
            out8_3_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel6, 2)  ,  _mm_srli_epi16(in_pixel7, 2)  );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);
            avg8_1_U8 =  _mm_avg_epu8 (out8_1_U8_L0 , out8_1_U8_L1);
            avg8_2_U8 =  _mm_avg_epu8 (out8_2_U8_L0 , out8_2_U8_L1);
            avg8_3_U8 =  _mm_avg_epu8 (out8_3_U8_L0 , out8_3_U8_L1);
#if ALSTORE
            _mm_store_si128((__m128i*) dst_ptr      , avg8_0_U8);
            _mm_store_si128((__m128i*)(dst_ptr + 16), avg8_1_U8);
            _mm_store_si128((__m128i*)(dst_ptr + 32), avg8_2_U8);
            _mm_store_si128((__m128i*)(dst_ptr + 48), avg8_3_U8);
#else
            _mm_storeu_si128((__m128i*) dst_ptr      , avg8_0_U8);
            _mm_storeu_si128((__m128i*)(dst_ptr + 16), avg8_1_U8);
            _mm_storeu_si128((__m128i*)(dst_ptr + 32), avg8_2_U8);
            _mm_storeu_si128((__m128i*)(dst_ptr + 48), avg8_3_U8);
#endif

#endif
            dst_ptr  += dst_stride;
            ref16_l0 += ref_l0_stride;
            ref16_l1 += ref_l1_stride;
        }
    }

    return;
}

int32_t  eb_vp9_sum_residual8bit_avx2_intrin(
                     int16_t * in_ptr,
                     uint32_t   size,
                     uint32_t   stride_in )
{

   int32_t  sum_block;

   __m128i in0, in1,in01, in2, in3, in23, sum, sum_l, sum_h;
   __m256i sum0, sum1, sum2, sum3, sum0L, sum0H, sumT, sum01, sum_t_perm;
   uint32_t  row_index;

   //Assumption: 9bit or 11bit residual data . for bigger block sizes or bigger bit depths , re-asses the dynamic range of the internal calculation

   if(size==4){ //SSSE3

        __m128i zer = _mm_setzero_si128();

       in0  = _mm_loadl_epi64((__m128i*)in_ptr);
       in1  = _mm_loadl_epi64((__m128i*)(in_ptr+stride_in));
       in1  = _mm_shuffle_epi32 (in1, 0x4A ); //01.00.10.10
       in01 = _mm_or_si128 (in1, in0);

       in2  = _mm_loadl_epi64((__m128i*)(in_ptr+2*stride_in));
       in3  = _mm_loadl_epi64((__m128i*)(in_ptr+3*stride_in));
       in3  = _mm_shuffle_epi32 (in3, 0x4A ); //01.00.10.10
       in23 = _mm_or_si128 (in3, in2);

       sum  = _mm_add_epi16 (in01, in23);
       sum  = _mm_hadd_epi16 (sum, zer);
       sum  = _mm_hadd_epi16 (sum, zer);
       sum  = _mm_hadd_epi16 (sum, zer);

       sum      = _mm_cvtepi16_epi32(sum);
       sum_block = _mm_cvtsi128_si32 (sum);

       return sum_block;

   }else if(size==8){//SSSE3

        __m128i zer = _mm_setzero_si128();

        sum  =  _mm_add_epi16 (_mm_loadu_si128((__m128i*)(in_ptr+0*stride_in)),  _mm_loadu_si128((__m128i*)(in_ptr+1*stride_in)));
        sum  =  _mm_add_epi16 (sum, _mm_loadu_si128((__m128i*)(in_ptr+2*stride_in)));
        sum  =  _mm_add_epi16 (sum, _mm_loadu_si128((__m128i*)(in_ptr+3*stride_in)));
        sum  =  _mm_add_epi16 (sum, _mm_loadu_si128((__m128i*)(in_ptr+4*stride_in)));
        sum  =  _mm_add_epi16 (sum, _mm_loadu_si128((__m128i*)(in_ptr+5*stride_in)));
        sum  =  _mm_add_epi16 (sum, _mm_loadu_si128((__m128i*)(in_ptr+6*stride_in)));
        sum  =  _mm_add_epi16 (sum, _mm_loadu_si128((__m128i*)(in_ptr+7*stride_in)));

        sum  = _mm_hadd_epi16 (sum, zer);
        sum  = _mm_hadd_epi16 (sum, zer);
        sum  = _mm_hadd_epi16 (sum, zer);

        sum      = _mm_cvtepi16_epi32(sum); //the sum is on 16bit, for negative values, we need to extend the sign to the next 16bit, so that the next extraction to int is fine.
        sum_block = _mm_cvtsi128_si32 (sum);

        return sum_block;

   }else if(size==16){//AVX2

         sum0  =  _mm256_add_epi16 (_mm256_loadu_si256((__m256i *)(in_ptr+0*stride_in)),  _mm256_loadu_si256((__m256i *)(in_ptr+1*stride_in)));
         sum0  =  _mm256_add_epi16 (sum0, _mm256_loadu_si256((__m256i *)(in_ptr+2*stride_in)));
         sum0  =  _mm256_add_epi16 (sum0, _mm256_loadu_si256((__m256i *)(in_ptr+3*stride_in)));
         sum0  =  _mm256_add_epi16 (sum0, _mm256_loadu_si256((__m256i *)(in_ptr+4*stride_in)));
         sum0  =  _mm256_add_epi16 (sum0, _mm256_loadu_si256((__m256i *)(in_ptr+5*stride_in)));
         sum0  =  _mm256_add_epi16 (sum0, _mm256_loadu_si256((__m256i *)(in_ptr+6*stride_in)));
         sum0  =  _mm256_add_epi16 (sum0, _mm256_loadu_si256((__m256i *)(in_ptr+7*stride_in)));

         in_ptr+=8*stride_in;
         sum1  =  _mm256_add_epi16 (_mm256_loadu_si256((__m256i *)(in_ptr+0*stride_in)),  _mm256_loadu_si256((__m256i *)(in_ptr+1*stride_in)));
         sum1  =  _mm256_add_epi16 (sum1, _mm256_loadu_si256((__m256i *)(in_ptr+2*stride_in)));
         sum1  =  _mm256_add_epi16 (sum1, _mm256_loadu_si256((__m256i *)(in_ptr+3*stride_in)));
         sum1  =  _mm256_add_epi16 (sum1, _mm256_loadu_si256((__m256i *)(in_ptr+4*stride_in)));
         sum1  =  _mm256_add_epi16 (sum1, _mm256_loadu_si256((__m256i *)(in_ptr+5*stride_in)));
         sum1  =  _mm256_add_epi16 (sum1, _mm256_loadu_si256((__m256i *)(in_ptr+6*stride_in)));
         sum1  =  _mm256_add_epi16 (sum1, _mm256_loadu_si256((__m256i *)(in_ptr+7*stride_in)));

         sum01  =  _mm256_add_epi16 (sum0,sum1);

         //go from 16bit to 32bit (to support big values)
         sum_l   = _mm256_castsi256_si128(sum01);
         sum_h   = _mm256_extracti128_si256(sum01,1);
         sum0L  = _mm256_cvtepi16_epi32(sum_l);
         sum0H  = _mm256_cvtepi16_epi32(sum_h);

         sumT     = _mm256_add_epi32(sum0L,sum0H);

         sumT     = _mm256_hadd_epi32(sumT,sumT);
         sumT     = _mm256_hadd_epi32(sumT,sumT);
         sum_t_perm = _mm256_permute4x64_epi64(sumT, 2); //00.00.00.10
         sumT     = _mm256_add_epi32(sumT,sum_t_perm);

         sum      = _mm256_castsi256_si128(sumT);
         sum_block = _mm_cvtsi128_si32 (sum);

         return sum_block;

   }
   else if (size == 32){//AVX2
       int16_t *in_ptr_temp = in_ptr;

       sum0 = sum1 = sum2 = sum3 = _mm256_setzero_si256();
       for (row_index = 0; row_index < size; row_index += 2) { // Parse every two rows
           sum0 = _mm256_add_epi16(sum0, _mm256_loadu_si256((__m256i *)(in_ptr_temp)));
           sum1 = _mm256_add_epi16(sum1, _mm256_loadu_si256((__m256i *)(in_ptr_temp + 16)));
           in_ptr_temp += stride_in;
           sum2 = _mm256_add_epi16(sum2, _mm256_loadu_si256((__m256i *)(in_ptr_temp)));
           sum3 = _mm256_add_epi16(sum3, _mm256_loadu_si256((__m256i *)(in_ptr_temp + 16)));
           in_ptr_temp += stride_in;

       }
       //go from 16bit to 32bit (to support big values)
       sum_l = _mm256_castsi256_si128(sum0);
       sum_h = _mm256_extracti128_si256(sum0, 1);
       sum0L = _mm256_cvtepi16_epi32(sum_l);
       sum0H = _mm256_cvtepi16_epi32(sum_h);
       sumT = _mm256_add_epi32(sum0L, sum0H);

       sum_l = _mm256_castsi256_si128(sum1);
       sum_h = _mm256_extracti128_si256(sum1, 1);
       sum0L = _mm256_cvtepi16_epi32(sum_l);
       sum0H = _mm256_cvtepi16_epi32(sum_h);
       sumT = _mm256_add_epi32(sumT, sum0L);
       sumT = _mm256_add_epi32(sumT, sum0H);

       sum_l = _mm256_castsi256_si128(sum2);
       sum_h = _mm256_extracti128_si256(sum2, 1);
       sum0L = _mm256_cvtepi16_epi32(sum_l);
       sum0H = _mm256_cvtepi16_epi32(sum_h);
       sumT = _mm256_add_epi32(sumT, sum0L);
       sumT = _mm256_add_epi32(sumT, sum0H);

       sum_l = _mm256_castsi256_si128(sum3);
       sum_h = _mm256_extracti128_si256(sum3, 1);
       sum0L = _mm256_cvtepi16_epi32(sum_l);
       sum0H = _mm256_cvtepi16_epi32(sum_h);
       sumT = _mm256_add_epi32(sumT, sum0L);
       sumT = _mm256_add_epi32(sumT, sum0H);

       sumT = _mm256_hadd_epi32(sumT, sumT);
       sumT = _mm256_hadd_epi32(sumT, sumT);
       sum_t_perm = _mm256_permute4x64_epi64(sumT, 2); //00.00.00.10
       sumT = _mm256_add_epi32(sumT, sum_t_perm);

       sum = _mm256_castsi256_si128(sumT);
       sum_block = _mm_cvtsi128_si32(sum);

       return sum_block;
   }

   else{
       SVT_LOG("\n add the rest \n");
       return 0;
   }

}

void eb_vp9_eb_vp9_memset16bit_block_avx2_intrin (
                    int16_t * in_ptr,
                    uint32_t   stride_in,
                    uint32_t   size,
                    int16_t   value
    )
{

   if(size==4){

        __m128i line =  _mm_set1_epi16 (value);

        _mm_storel_epi64 ((__m128i *)(in_ptr + 0*stride_in), line);
        _mm_storel_epi64 ((__m128i *)(in_ptr + 1*stride_in), line);
        _mm_storel_epi64 ((__m128i *)(in_ptr + 2*stride_in), line);
        _mm_storel_epi64 ((__m128i *)(in_ptr + 3*stride_in), line);

   }else if(size==8){

       __m128i line =  _mm_set1_epi16 (value);

        _mm_storeu_si128((__m128i *)(in_ptr + 0*stride_in), line);
        _mm_storeu_si128((__m128i *)(in_ptr + 1*stride_in), line);
        _mm_storeu_si128((__m128i *)(in_ptr + 2*stride_in), line);
        _mm_storeu_si128((__m128i *)(in_ptr + 3*stride_in), line);
        _mm_storeu_si128((__m128i *)(in_ptr + 4*stride_in), line);
        _mm_storeu_si128((__m128i *)(in_ptr + 5*stride_in), line);
        _mm_storeu_si128((__m128i *)(in_ptr + 6*stride_in), line);
        _mm_storeu_si128((__m128i *)(in_ptr + 7*stride_in), line);

   }else if(size==16){

       __m256i line =  _mm256_set1_epi16(value);

       _mm256_storeu_si256((__m256i *)(in_ptr + 0*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 1*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 2*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 3*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 4*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 5*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 6*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 7*stride_in), line);

       in_ptr+=8*stride_in;

       _mm256_storeu_si256((__m256i *)(in_ptr + 0*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 1*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 2*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 3*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 4*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 5*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 6*stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 7*stride_in), line);

   }
   else if (size == 32){

       __m256i line = _mm256_set1_epi16(value);

       _mm256_storeu_si256((__m256i *)(in_ptr + 0 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 0 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 1 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 1 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 2 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 2 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 3 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 3 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 4 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 4 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 5 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 5 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 6 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 6 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 7 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 7 * stride_in + 16), line);

       in_ptr += 8 * stride_in;

       _mm256_storeu_si256((__m256i *)(in_ptr + 0 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 0 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 1 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 1 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 2 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 2 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 3 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 3 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 4 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 4 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 5 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 5 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 6 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 6 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 7 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 7 * stride_in + 16), line);

       in_ptr += 8 * stride_in;

       _mm256_storeu_si256((__m256i *)(in_ptr + 0 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 0 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 1 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 1 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 2 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 2 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 3 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 3 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 4 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 4 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 5 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 5 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 6 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 6 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 7 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 7 * stride_in + 16), line);

       in_ptr += 8 * stride_in;

       _mm256_storeu_si256((__m256i *)(in_ptr + 0 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 0 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 1 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 1 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 2 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 2 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 3 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 3 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 4 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 4 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 5 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 5 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 6 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 6 * stride_in + 16), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 7 * stride_in), line);
       _mm256_storeu_si256((__m256i *)(in_ptr + 7 * stride_in + 16), line);

   }

   else{
       SVT_LOG("\n add the rest \n");
   }

}

void eb_vp9_unpack_avg_safe_sub_avx2_intrin(
        uint16_t *ref16_l0,
        uint32_t  ref_l0_stride,
        uint16_t *ref16_l1,
        uint32_t  ref_l1_stride,
        uint8_t  *dst_ptr,
        uint32_t  dst_stride,
        uint32_t  width,
        uint32_t  height)
{

    uint32_t   y;
    __m128i in_pixel0, in_pixel1;

    if (width == 8)
    {

        __m128i out8_0_U8_L0, out8_0_U8_L1, out8_2_U8_L0,out8_2_U8_L1;
        __m128i avg8_0_U8,avg8_2_U8;

        for (y = 0; y < height; y += 2)
        {
            //--------
            //Line One
            //--------

            //List0

            in_pixel0 = _mm_loadu_si128((__m128i*) ref16_l0);

            in_pixel1 = _mm_srli_epi16(in_pixel0, 2) ;
            out8_0_U8_L0 = _mm_packus_epi16( in_pixel1 , in_pixel1 );

             //List1

            in_pixel0 = _mm_loadu_si128((__m128i*) ref16_l1);

            in_pixel1 = _mm_srli_epi16(in_pixel0, 2) ;
            out8_0_U8_L1 = _mm_packus_epi16( in_pixel1 , in_pixel1 );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);

            _mm_storel_epi64((__m128i*) dst_ptr      , avg8_0_U8);

            //--------
            //Line Two
            //--------

            //List0

            in_pixel0 = _mm_loadu_si128((__m128i*)(ref16_l0 + ref_l0_stride) );

            in_pixel1 = _mm_srli_epi16(in_pixel0, 2) ;
            out8_2_U8_L0 = _mm_packus_epi16( in_pixel1 , in_pixel1 );

            //List1

            in_pixel0 = _mm_loadu_si128((__m128i*)(ref16_l1 + ref_l1_stride) );

            in_pixel1 = _mm_srli_epi16(in_pixel0, 2) ;
            out8_2_U8_L1 = _mm_packus_epi16( in_pixel1 , in_pixel1 );

            //AVG
            avg8_2_U8 =  _mm_avg_epu8 (out8_2_U8_L0 , out8_2_U8_L1);

            _mm_storel_epi64((__m128i*)(dst_ptr +dst_stride)    , avg8_2_U8);

            dst_ptr  += 2*dst_stride;
            ref16_l0 += 2*ref_l0_stride;
            ref16_l1 += 2*ref_l1_stride;
        }

    }
    else if (width == 16)
    {

        __m128i in_pixel4, in_pixel5;
        __m128i out8_0_U8_L0, out8_0_U8_L1, out8_2_U8_L0,out8_2_U8_L1;
        __m128i avg8_0_U8,avg8_2_U8;

        for (y = 0; y < height; y += 2)
        {

            //--------
            //Line One
            //--------

            //List0

            in_pixel0 = _mm_loadu_si128((__m128i*)  ref16_l0);
            in_pixel1 = _mm_loadu_si128((__m128i*) (ref16_l0 + 8));

            out8_0_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel0, 2)  ,  _mm_srli_epi16(in_pixel1, 2)  );

             //List1

            in_pixel0 = _mm_loadu_si128((__m128i*) ref16_l1);
            in_pixel1 = _mm_loadu_si128((__m128i*)(ref16_l1 + 8));

            out8_0_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel0, 2)  ,  _mm_srli_epi16(in_pixel1, 2)  );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);

            _mm_store_si128((__m128i*) dst_ptr      , avg8_0_U8);

            //--------
            //Line Two
            //--------

            //List0

            in_pixel4 = _mm_loadu_si128((__m128i*) (ref16_l0 + ref_l0_stride));
            in_pixel5 = _mm_loadu_si128((__m128i*) (ref16_l0 + ref_l0_stride + 8));

               out8_2_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel4, 2)  ,  _mm_srli_epi16(in_pixel5, 2)  );

            //List1

            in_pixel4 = _mm_loadu_si128((__m128i*) (ref16_l1 + ref_l1_stride));
            in_pixel5 = _mm_loadu_si128((__m128i*) (ref16_l1 + ref_l1_stride + 8));

               out8_2_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(in_pixel4, 2)  ,  _mm_srli_epi16(in_pixel5, 2)  );

            //AVG
            avg8_2_U8 =  _mm_avg_epu8 (out8_2_U8_L0 , out8_2_U8_L1);

            _mm_store_si128((__m128i*)(dst_ptr  + dst_stride      ) , avg8_2_U8);

            dst_ptr  += 2*dst_stride;
            ref16_l0 += 2*ref_l0_stride;
            ref16_l1 += 2*ref_l1_stride;

        }

    }
    else if (width == 32)
    {

        __m256i in_val16b0,in_val16b1;
        __m256i data8b_32_0_L0,data8b_32_0_L1;
        __m256i avg8b_32_0;

       for (y = 0; y < height; y += 2)
        {

            //--------
            //Line One
            //--------

            //List0
            in_val16b0 = _mm256_loadu_si256((__m256i*) ref16_l0);
            in_val16b1 = _mm256_loadu_si256((__m256i*)(ref16_l0 + 16));
            data8b_32_0_L0 = _mm256_packus_epi16(  _mm256_srli_epi16 (in_val16b0, 2) ,  _mm256_srli_epi16 (in_val16b1, 2));
            //List1
            in_val16b0 = _mm256_loadu_si256((__m256i*) ref16_l1);
            in_val16b1 = _mm256_loadu_si256((__m256i*)(ref16_l1 + 16));
            data8b_32_0_L1 = _mm256_packus_epi16(  _mm256_srli_epi16 (in_val16b0, 2) ,  _mm256_srli_epi16 (in_val16b1, 2));

            //Avg
            avg8b_32_0 = _mm256_avg_epu8(data8b_32_0_L0,data8b_32_0_L1);

            avg8b_32_0 = _mm256_permute4x64_epi64(avg8b_32_0, 216);

            _mm256_storeu_si256((__m256i *)(dst_ptr     ), avg8b_32_0);

            //--------
            //Line Two
            //--------
              //List0
            in_val16b0 = _mm256_loadu_si256((__m256i*)(ref16_l0 + ref_l0_stride     ));
            in_val16b1 = _mm256_loadu_si256((__m256i*)(ref16_l0 + ref_l0_stride + 16));

            data8b_32_0_L0 = _mm256_packus_epi16(  _mm256_srli_epi16 (in_val16b0, 2) ,  _mm256_srli_epi16 (in_val16b1, 2));

            //List1
            in_val16b0 = _mm256_loadu_si256((__m256i*)(ref16_l1 + ref_l1_stride     ));
            in_val16b1 = _mm256_loadu_si256((__m256i*)(ref16_l1 + ref_l1_stride + 16));

            data8b_32_0_L1 = _mm256_packus_epi16(  _mm256_srli_epi16 (in_val16b0, 2) ,  _mm256_srli_epi16 (in_val16b1, 2));

            //Avg
            avg8b_32_0 = _mm256_avg_epu8(data8b_32_0_L0,data8b_32_0_L1);

            avg8b_32_0 = _mm256_permute4x64_epi64(avg8b_32_0, 216);

            _mm256_storeu_si256((__m256i *)(dst_ptr + dst_stride   ), avg8b_32_0);

            dst_ptr  += 2*dst_stride;
            ref16_l0 += 2*ref_l0_stride;
            ref16_l1 += 2*ref_l1_stride;

        }

    }
    else if (width == 64)
    {
        __m256i in_val16b0,in_val16b1,inVal16b_2,inVal16b_3;
        __m256i data8b_32_0_L0,data8b_32_1_L0,data8b_32_0_L1,data8b_32_1_L1;
        __m256i avg8b_32_0,avg8b_32_1;

        for (y = 0; y < height; ++y)
        {

            //List0
            in_val16b0 = _mm256_loadu_si256((__m256i*) ref16_l0);
            in_val16b1 = _mm256_loadu_si256((__m256i*)(ref16_l0 + 16));
            inVal16b_2 = _mm256_loadu_si256((__m256i*)(ref16_l0 + 32));
            inVal16b_3 = _mm256_loadu_si256((__m256i*)(ref16_l0 + 48));
            data8b_32_0_L0 = _mm256_packus_epi16(  _mm256_srli_epi16 (in_val16b0, 2) ,  _mm256_srli_epi16 (in_val16b1, 2));
            data8b_32_1_L0 = _mm256_packus_epi16(  _mm256_srli_epi16 (inVal16b_2, 2) ,  _mm256_srli_epi16 (inVal16b_3, 2));
            //List1
            in_val16b0 = _mm256_loadu_si256((__m256i*) ref16_l1);
            in_val16b1 = _mm256_loadu_si256((__m256i*)(ref16_l1 + 16));
            inVal16b_2 = _mm256_loadu_si256((__m256i*)(ref16_l1 + 32));
            inVal16b_3 = _mm256_loadu_si256((__m256i*)(ref16_l1 + 48));
            data8b_32_0_L1 = _mm256_packus_epi16(  _mm256_srli_epi16 (in_val16b0, 2) ,  _mm256_srli_epi16 (in_val16b1, 2));
            data8b_32_1_L1 = _mm256_packus_epi16(  _mm256_srli_epi16 (inVal16b_2, 2) ,  _mm256_srli_epi16 (inVal16b_3, 2));

            //Avg
            avg8b_32_0 = _mm256_avg_epu8(data8b_32_0_L0,data8b_32_0_L1);
            avg8b_32_1 = _mm256_avg_epu8(data8b_32_1_L0,data8b_32_1_L1);

            avg8b_32_0 = _mm256_permute4x64_epi64(avg8b_32_0, 216);
            avg8b_32_1 = _mm256_permute4x64_epi64(avg8b_32_1, 216);

            _mm256_storeu_si256((__m256i *)(dst_ptr     ), avg8b_32_0);
            _mm256_storeu_si256((__m256i *)(dst_ptr + 32), avg8b_32_1);

            dst_ptr  += dst_stride;
            ref16_l0 += ref_l0_stride;
            ref16_l1 += ref_l1_stride;
        }

    }

    return;
}

void full_distortion_kernel_eob_zero_4x4_32bit_bt_avx2(
    int16_t  *coeff,
    uint32_t   coeff_stride,
    int16_t  *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height)
{
    __m256i sum = _mm256_setzero_si256();
    __m128i m0, m1;
    __m256i x, y;
    m0 = _mm_loadl_epi64((__m128i *)coeff); coeff += coeff_stride;
    m0 = _mm_loadhi_epi64(m0, (__m128i *)coeff); coeff += coeff_stride;
    m1 = _mm_loadl_epi64((__m128i *)coeff); coeff += coeff_stride;
    m1 = _mm_loadhi_epi64(m1, (__m128i *)coeff); coeff += coeff_stride;
    x = _mm256_set_m128i(m1, m0);
    y = _mm256_madd_epi16(x, x);
    sum = _mm256_add_epi32(sum, y);
    sum = _mm256_add_epi32(sum, _mm256_shuffle_epi32(sum, 0x4e)); // 01001110
    sum = _mm256_unpacklo_epi32(sum, sum);
    sum = _mm256_add_epi32(sum, _mm256_shuffle_epi32(sum, 0x4e)); // 01001110
    m0 = _mm256_extracti128_si256(sum, 0);
    m1 = _mm256_extracti128_si256(sum, 1);
    m0 = _mm_add_epi32(m0, m1);
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(m0, _mm_setzero_si128()));

    (void)area_width;
    (void)area_height;
    (void)recon_coeff;
    (void)recon_coeff_stride;
}

void full_distortion_kernel_eob_zero_8x8_32bit_bt_avx2(
    int16_t  *coeff,
    uint32_t   coeff_stride,
    int16_t  *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height)
{
    int32_t  row_count;
    __m256i sum = _mm256_setzero_si256();
    __m128i temp1, temp2;

    row_count = 4;
    do
    {
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

    sum = _mm256_add_epi32(sum, _mm256_shuffle_epi32(sum, 0x4e)); // 01001110
    sum = _mm256_unpacklo_epi32(sum, sum);
    sum = _mm256_add_epi32(sum, _mm256_shuffle_epi32(sum, 0x4e)); // 01001110
    temp1 = _mm256_extracti128_si256(sum, 0);
    temp2 = _mm256_extracti128_si256(sum, 1);
    temp1 = _mm_add_epi32(temp1, temp2);
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(temp1, _mm_setzero_si128()));

    (void)area_width;
    (void)area_height;
    (void)recon_coeff;
    (void)recon_coeff_stride;
}

void full_distortion_kernel_eob_zero16_mxn_32bit_bt_avx2(
    int16_t  *coeff,
    uint32_t   coeff_stride,
    int16_t  *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height)
{
    int32_t row_count, col_count;
    __m256i sum = _mm256_setzero_si256();
    __m128i temp1, temp2;

    col_count = area_width;
    do
    {
        int16_t *coeff_temp = coeff;

        row_count = area_height;
        do
        {
            __m256i x0, z0;
            x0 = _mm256_loadu_si256((__m256i *)(coeff_temp));
            coeff_temp += coeff_stride;
            z0 = _mm256_madd_epi16(x0, x0);
            sum = _mm256_add_epi32(sum, z0);
        } while (--row_count);

        coeff += 16;
        recon_coeff += 16;
        col_count -= 16;
    } while (col_count > 0);

    sum = _mm256_add_epi32(sum, _mm256_shuffle_epi32(sum, 0x4e)); // 01001110
    sum = _mm256_unpacklo_epi32(sum, sum);
    sum = _mm256_add_epi32(sum, _mm256_shuffle_epi32(sum, 0x4e)); // 01001110
    temp1 = _mm256_extracti128_si256(sum, 0);
    temp2 = _mm256_extracti128_si256(sum, 1);
    temp1 = _mm_add_epi32(temp1, temp2);
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(temp1, _mm_setzero_si128()));
    (void)recon_coeff_stride;
}

void full_distortion_kernel_4x4_32bit_bt_avx2(
    int16_t  *coeff,
    uint32_t   coeff_stride,
    int16_t  *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height)
{
    __m256i sum1 = _mm256_setzero_si256();
    __m256i sum2 = _mm256_setzero_si256();
    __m128i m0, m1;
    __m256i x, y, z;
    m0 = _mm_loadl_epi64((__m128i *)coeff); coeff += coeff_stride;
    m0 = _mm_loadhi_epi64(m0, (__m128i *)coeff); coeff += coeff_stride;
    m1 = _mm_loadl_epi64((__m128i *)coeff); coeff += coeff_stride;
    m1 = _mm_loadhi_epi64(m1, (__m128i *)coeff); coeff += coeff_stride;
    x = _mm256_set_m128i(m1, m0);
    m0 = _mm_loadl_epi64((__m128i *)recon_coeff); recon_coeff += recon_coeff_stride;
    m0 = _mm_loadhi_epi64(m0, (__m128i *)recon_coeff); recon_coeff += recon_coeff_stride;
    m1 = _mm_loadl_epi64((__m128i *)recon_coeff); recon_coeff += recon_coeff_stride;
    m1 = _mm_loadhi_epi64(m1, (__m128i *)recon_coeff); recon_coeff += recon_coeff_stride;
    y = _mm256_set_m128i(m1, m0);
    z = _mm256_madd_epi16(x, x);
    sum2 = _mm256_add_epi32(sum2, z);
    x = _mm256_sub_epi16(x, y);
    x = _mm256_madd_epi16(x, x);
    sum1 = _mm256_add_epi32(sum1, x);
    sum1 = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4e)); // 01001110
    sum2 = _mm256_add_epi32(sum2, _mm256_shuffle_epi32(sum2, 0x4e)); // 01001110
    sum1 = _mm256_unpacklo_epi32(sum1, sum2);
    sum1 = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4e)); // 01001110
    m0 = _mm256_extracti128_si256(sum1, 0);
    m1 = _mm256_extracti128_si256(sum1, 1);
    m0 = _mm_add_epi32(m0, m1);
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(m0, _mm_setzero_si128()));

    (void)area_width;
    (void)area_height;
}

void full_distortion_kernel_8x8_32bit_bt_avx2(
    int16_t  *coeff,
    uint32_t   coeff_stride,
    int16_t  *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height)
{
    int32_t row_count;

    __m256i sum1 = _mm256_setzero_si256();
    __m256i sum2 = _mm256_setzero_si256();
    __m128i temp1, temp2;

    row_count = 4;
    do
    {
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

        z = _mm256_madd_epi16(x, x);
        sum2 = _mm256_add_epi32(sum2, z);
        x = _mm256_sub_epi16(x, y);
        x = _mm256_madd_epi16(x, x);
        sum1 = _mm256_add_epi32(sum1, x);
    } while (--row_count);

    sum1 = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4e)); // 01001110
    sum2 = _mm256_add_epi32(sum2, _mm256_shuffle_epi32(sum2, 0x4e)); // 01001110
    sum1 = _mm256_unpacklo_epi32(sum1, sum2);
    sum1 = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4e)); // 01001110
    temp1 = _mm256_extracti128_si256(sum1, 0);
    temp2 = _mm256_extracti128_si256(sum1, 1);
    temp1 = _mm_add_epi32(temp1, temp2);
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(temp1, _mm_setzero_si128()));

    (void)area_width;
    (void)area_height;
}

void full_distortion_kernel_16_32_bit_bt_avx2(
    int16_t  *coeff,
    uint32_t   coeff_stride,
    int16_t  *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height)
{
    int32_t row_count, col_count;
    __m256i sum1 = _mm256_setzero_si256();
    __m256i sum2 = _mm256_setzero_si256();
    __m128i temp1, temp2;

    col_count = area_width;
    do
    {
        int16_t *coeff_temp = coeff;
        int16_t *reconCoeffTemp = recon_coeff;

        row_count = area_height;
        do
        {
            __m256i x, y, z;
            x = _mm256_loadu_si256((__m256i *)(coeff_temp));
            y = _mm256_loadu_si256((__m256i *)(reconCoeffTemp));
            coeff_temp += coeff_stride;
            reconCoeffTemp += recon_coeff_stride;

            z = _mm256_madd_epi16(x, x);
            sum2 = _mm256_add_epi32(sum2, z);
            x = _mm256_sub_epi16(x, y);
            x = _mm256_madd_epi16(x, x);
            sum1 = _mm256_add_epi32(sum1, x);
        } while (--row_count);

        coeff += 16;
        recon_coeff += 16;
        col_count -= 16;
    } while (col_count > 0);

    sum1 = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4e)); // 01001110
    sum2 = _mm256_add_epi32(sum2, _mm256_shuffle_epi32(sum2, 0x4e)); // 01001110
    sum1 = _mm256_unpacklo_epi32(sum1, sum2);
    sum1 = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4e)); // 01001110
    temp1 = _mm256_extracti128_si256(sum1, 0);
    temp2 = _mm256_extracti128_si256(sum1, 1);
    temp1 = _mm_add_epi32(temp1, temp2);
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(temp1, _mm_setzero_si128()));
}

void eb_vp9_picture_average_kernel_avx2_intrin(
    EbByte                  src0,
    uint32_t                   src0_stride,
    EbByte                  src1,
    uint32_t                   src1_stride,
    EbByte                  dst,
    uint32_t                   dst_stride,
    uint32_t                   area_width,
    uint32_t                   area_height)
{
    __m128i xmm_avg1, xmm_avg2, xmm_avg3, xmm_avg4, temp1, temp2, temp3, temp4;
    __m256i ymm_avg1, ymm_avg2, ymm_avg3, ymm_avg4;
    uint32_t y;
    if (area_width >= 16)
    {
        if (area_width == 16)
        {
            for (y = 0; y < area_height; y += 2) {
                temp1 = _mm_loadu_si128((__m128i*)src0);
                temp2 = _mm_loadu_si128((__m128i*)(src0 + src0_stride));
                temp3 = _mm_loadu_si128((__m128i*)src1);
                temp4 = _mm_loadu_si128((__m128i*)(src1 + src1_stride));
                ymm_avg1 = _mm256_avg_epu8(_mm256_set_m128i(temp2, temp1), _mm256_set_m128i(temp4, temp3));
                xmm_avg1 = _mm256_extracti128_si256(ymm_avg1, 0);
                xmm_avg2 = _mm256_extracti128_si256(ymm_avg1, 1);
                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storeu_si128((__m128i*) (dst + dst_stride), xmm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
        else if (area_width == 24)
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

                ymm_avg1 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)src0), _mm256_loadu_si256((__m256i*)src1));
                ymm_avg2 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + src0_stride)), _mm256_loadu_si256((__m256i*)(src1 + src1_stride)));

                _mm256_storeu_si256((__m256i*) dst, ymm_avg1);
                _mm256_storeu_si256((__m256i*) (dst + dst_stride), ymm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
        else if (area_width == 48)
        {
            for (y = 0; y < area_height; y += 2) {
                ymm_avg1 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)src0), _mm256_loadu_si256((__m256i*)src1));
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 32)), _mm_loadu_si128((__m128i*)(src1 + 32)));

                ymm_avg2 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + src0_stride)), _mm256_loadu_si256((__m256i*)(src1 + src1_stride)));
                xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0_stride + 32)), _mm_loadu_si128((__m128i*)(src1 + src1_stride + 32)));

                _mm256_storeu_si256((__m256i*) dst, ymm_avg1);
                _mm_storeu_si128((__m128i*) (dst + 32), xmm_avg1);
                _mm256_storeu_si256((__m256i*) (dst + dst_stride), ymm_avg2);
                _mm_storeu_si128((__m128i*) (dst + dst_stride + 32), xmm_avg2);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;

            }
        }
        else
        {
            for (y = 0; y < area_height; y += 2) {
                ymm_avg1 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)src0), _mm256_loadu_si256((__m256i*)src1));
                ymm_avg2 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + 32)), _mm256_loadu_si256((__m256i*)(src1 + 32)));
                ymm_avg3 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + src0_stride)), _mm256_loadu_si256((__m256i*)(src1 + src1_stride)));
                ymm_avg4 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + src0_stride + 32)), _mm256_loadu_si256((__m256i*)(src1 + src1_stride + 32)));

                _mm256_storeu_si256((__m256i*) dst, ymm_avg1);
                _mm256_storeu_si256((__m256i*) (dst + 32), ymm_avg2);
                _mm256_storeu_si256((__m256i*) (dst + dst_stride), ymm_avg3);
                _mm256_storeu_si256((__m256i*) (dst + dst_stride + 32), ymm_avg4);

                src0 += src0_stride << 1;
                src1 += src1_stride << 1;
                dst += dst_stride << 1;
            }
        }
    }
    else
    {
        if (area_width == 4)
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

void eb_vp9_residual_kernel4x4_avx2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t  *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{
    const __m256i zero = _mm256_setzero_si256();
    const __m256i in = load8bit_4x4_avx2(input, input_stride);
    const __m256i pr = load8bit_4x4_avx2(pred, pred_stride);
    const __m256i in_lo = _mm256_unpacklo_epi8(in, zero);
    const __m256i pr_lo = _mm256_unpacklo_epi8(pr, zero);
    const __m256i re_lo = _mm256_sub_epi16(in_lo, pr_lo);
    const __m128i re_01 = _mm256_extracti128_si256(re_lo, 0);
    const __m128i re_23 = _mm256_extracti128_si256(re_lo, 1);
    (void)area_width;
    (void)area_height;

    _mm_storel_epi64((__m128i*)(residual + 0 * residual_stride), re_01);
    _mm_storeh_epi64((__m128i*)(residual + 1 * residual_stride), re_01);
    _mm_storel_epi64((__m128i*)(residual + 2 * residual_stride), re_23);
    _mm_storeh_epi64((__m128i*)(residual + 3 * residual_stride), re_23);
}

void eb_vp9_residual_kernel8x8_avx2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t  *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{
    const __m256i zero = _mm256_setzero_si256();
    uint32_t y;
    (void)area_width;
    (void)area_height;

    for (y = 0; y < 8; y += 4) {
        const __m256i in = load8bit_8x4_avx2(input, input_stride);
        const __m256i pr = load8bit_8x4_avx2(pred, pred_stride);
        const __m256i in_lo = _mm256_unpacklo_epi8(in, zero);
        const __m256i in_hi = _mm256_unpackhi_epi8(in, zero);
        const __m256i pr_lo = _mm256_unpacklo_epi8(pr, zero);
        const __m256i pr_hi = _mm256_unpackhi_epi8(pr, zero);
        const __m256i re_lo = _mm256_sub_epi16(in_lo, pr_lo);
        const __m256i re_hi = _mm256_sub_epi16(in_hi, pr_hi);
        const __m128i re_0 = _mm256_extracti128_si256(re_lo, 0);
        const __m128i re_1 = _mm256_extracti128_si256(re_hi, 0);
        const __m128i re_2 = _mm256_extracti128_si256(re_lo, 1);
        const __m128i re_3 = _mm256_extracti128_si256(re_hi, 1);

        _mm_storeu_si128((__m128i*)(residual + 0 * residual_stride), re_0);
        _mm_storeu_si128((__m128i*)(residual + 1 * residual_stride), re_1);
        _mm_storeu_si128((__m128i*)(residual + 2 * residual_stride), re_2);
        _mm_storeu_si128((__m128i*)(residual + 3 * residual_stride), re_3);
        input += 4 * input_stride;
        pred += 4 * pred_stride;
        residual += 4 * residual_stride;
    }
}

void eb_vp9_residual_kernel16x16_avx2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t  *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{
    const __m256i zero = _mm256_setzero_si256();
    uint32_t y;
    (void)area_width;
    (void)area_height;

    for (y = 0; y < 16; y += 2) {
        const __m256i in0 = load8bit_16x2_unaligned_avx2(input, input_stride);
        const __m256i pr0 = load8bit_16x2_unaligned_avx2(pred, pred_stride);
        const __m256i in1 = _mm256_permute4x64_epi64(in0, 0xD8);
        const __m256i pr1 = _mm256_permute4x64_epi64(pr0, 0xD8);
        const __m256i in_lo = _mm256_unpacklo_epi8(in1, zero);
        const __m256i in_hi = _mm256_unpackhi_epi8(in1, zero);
        const __m256i pr_lo = _mm256_unpacklo_epi8(pr1, zero);
        const __m256i pr_hi = _mm256_unpackhi_epi8(pr1, zero);
        const __m256i re_lo = _mm256_sub_epi16(in_lo, pr_lo);
        const __m256i re_hi = _mm256_sub_epi16(in_hi, pr_hi);

        _mm256_storeu_si256((__m256i*)(residual + 0 * residual_stride), re_lo);
        _mm256_storeu_si256((__m256i*)(residual + 1 * residual_stride), re_hi);
        input += 2 * input_stride;
        pred += 2 * pred_stride;
        residual += 2 * residual_stride;
    }
}

static INLINE void eb_vp9_residual_kernel32_avx2(
    const uint8_t *const input,
    const uint8_t *const pred,
    int16_t *const      residual)
{
    const __m256i zero = _mm256_setzero_si256();
    const __m256i in0 = _mm256_loadu_si256((__m256i *)input);
    const __m256i pr0 = _mm256_loadu_si256((__m256i *)pred);
    const __m256i in1 = _mm256_permute4x64_epi64(in0, 0xD8);
    const __m256i pr1 = _mm256_permute4x64_epi64(pr0, 0xD8);
    const __m256i in_lo = _mm256_unpacklo_epi8(in1, zero);
    const __m256i in_hi = _mm256_unpackhi_epi8(in1, zero);
    const __m256i pr_lo = _mm256_unpacklo_epi8(pr1, zero);
    const __m256i pr_hi = _mm256_unpackhi_epi8(pr1, zero);
    const __m256i re_lo = _mm256_sub_epi16(in_lo, pr_lo);
    const __m256i re_hi = _mm256_sub_epi16(in_hi, pr_hi);
    _mm256_storeu_si256((__m256i*)(residual + 0x00), re_lo);
    _mm256_storeu_si256((__m256i*)(residual + 0x10), re_hi);
}

void eb_vp9_residual_kernel32x32_avx2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t  *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{
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

void eb_vp9_residual_kernel64x64_avx2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t  *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{
    uint32_t y;
    (void)area_width;
    (void)area_height;

    for (y = 0; y < 64; ++y) {
        eb_vp9_residual_kernel32_avx2(input + 0x00, pred + 0x00, residual + 0x00);
        eb_vp9_residual_kernel32_avx2(input + 0x20, pred + 0x20, residual + 0x20);
        input    += input_stride;
        pred     += pred_stride;
        residual += residual_stride;
    }
}
