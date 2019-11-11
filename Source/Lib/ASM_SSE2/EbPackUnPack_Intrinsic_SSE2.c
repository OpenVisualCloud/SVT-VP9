/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbPackUnPack_SSE2.h"
#include <emmintrin.h>
#include "stdint.h"

void eb_enc_un_pack8_bit_data_safe_sub_sse2_intrin(
    uint16_t *in16_bit_buffer,
    uint32_t  in_stride,
    uint8_t  *out8_bit_buffer,
    uint32_t  out8_stride,
    uint32_t  width,
    uint32_t  height
    )
{

    uint32_t x, y;

    __m128i xmm_00FF, in_pixel0, in_pixel1, inPixel1_shftR_2_U8, inPixel0_shftR_2_U8, inPixel0_shftR_2, inPixel1_shftR_2;

    xmm_00FF = _mm_set1_epi16(0x00FF);

    if (width == 8)
    {
        for (y = 0; y < height; y += 2)
        {
            in_pixel0 = _mm_loadu_si128((__m128i*) in16_bit_buffer);
            in_pixel1 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride));

            inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF);
            inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF);

            inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
            inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

            _mm_storel_epi64((__m128i*)out8_bit_buffer, inPixel0_shftR_2_U8);
            _mm_storel_epi64((__m128i*)(out8_bit_buffer + out8_stride), inPixel1_shftR_2_U8);

            out8_bit_buffer += 2 * out8_stride;
            in16_bit_buffer += 2 * in_stride;
        }

    }
    else if (width == 16)
    {
        __m128i in_pixel2, inPixel3;

        for (y = 0; y < height; y += 2)
        {
            in_pixel0 = _mm_loadu_si128((__m128i*) in16_bit_buffer);
            in_pixel1 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + 8));
            in_pixel2 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride));
            inPixel3 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride + 8));

            inPixel0_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
            inPixel1_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));

            _mm_storeu_si128((__m128i*)out8_bit_buffer, inPixel0_shftR_2_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride), inPixel1_shftR_2_U8);

            out8_bit_buffer += 2 * out8_stride;
            in16_bit_buffer += 2 * in_stride;
        }

    }
    else if (width == 32)
    {
         __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
         __m128i  out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

        for (y = 0; y < height; y += 2)
        {
            in_pixel0 = _mm_loadu_si128((__m128i*)in16_bit_buffer);
            in_pixel1 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 8));
            in_pixel2 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 24));
            in_pixel4 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + in_stride));
            in_pixel5 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + in_stride + 8));
            in_pixel6 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride + 16));
            in_pixel7 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride + 24));

            out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
            out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
            out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel5, 2), xmm_00FF));
            out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel7, 2), xmm_00FF));

            _mm_storeu_si128((__m128i*)out8_bit_buffer, out8_0_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + 16), out8_1_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride), out8_2_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride + 16), out8_3_U8);

            out8_bit_buffer += 2 * out8_stride;
            in16_bit_buffer += 2 * in_stride;
        }

    }
    else if (width == 64)
    {
        __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
        __m128i  out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

        for (y = 0; y < height; ++y)
        {
            in_pixel0 = _mm_loadu_si128((__m128i*)in16_bit_buffer);
            in_pixel1 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 8));
            in_pixel2 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 24));
            in_pixel4 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 32));
            in_pixel5 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 40));
            in_pixel6 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 48));
            in_pixel7 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 56));

            out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
            out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
            out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel5, 2), xmm_00FF));
            out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel7, 2), xmm_00FF));

            _mm_storeu_si128((__m128i*)out8_bit_buffer, out8_0_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + 16), out8_1_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + 32), out8_2_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + 48), out8_3_U8);

            out8_bit_buffer += out8_stride;
            in16_bit_buffer += in_stride;
        }

    } else
    {
        uint32_t in_stride_diff = (2 * in_stride) - width;
        uint32_t out8_stride_diff = (2 * out8_stride) - width;

        uint32_t in_stride_diff64 = in_stride - width;
        uint32_t out8_stride_diff64 = out8_stride - width;

        if (!(width & 63))
        {
            __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
            __m128i out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

            for (x = 0; x < height; x += 1){
                for (y = 0; y < width; y += 64){

                    in_pixel0 = _mm_loadu_si128((__m128i*)in16_bit_buffer);
                    in_pixel1 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 8));
                    in_pixel2 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 16));
                    inPixel3 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 24));
                    in_pixel4 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 32));
                    in_pixel5 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 40));
                    in_pixel6 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 48));
                    in_pixel7 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 56));

                    out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
                    out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
                    out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel5, 2), xmm_00FF));
                    out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel7, 2), xmm_00FF));

                    _mm_storeu_si128((__m128i*)out8_bit_buffer, out8_0_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + 16), out8_1_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + 32), out8_2_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + 48), out8_3_U8);

                    out8_bit_buffer += 64;
                    in16_bit_buffer += 64;
                }
                in16_bit_buffer += in_stride_diff64;
                out8_bit_buffer += out8_stride_diff64;
            }
        }
        else if (!(width & 31))
        {
            __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
            __m128i out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 32)
                {
                    in_pixel0 = _mm_loadu_si128((__m128i*)in16_bit_buffer);
                    in_pixel1 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 8));
                    in_pixel2 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 16));
                    inPixel3 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 24));
                    in_pixel4 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + in_stride));
                    in_pixel5 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + in_stride + 8));
                    in_pixel6 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride + 16));
                    in_pixel7 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride + 24));

                    out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
                    out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
                    out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel5, 2), xmm_00FF));
                    out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel7, 2), xmm_00FF));

                    _mm_storeu_si128((__m128i*)out8_bit_buffer, out8_0_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + 16), out8_1_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride), out8_2_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride + 16), out8_3_U8);

                    out8_bit_buffer += 32;
                    in16_bit_buffer += 32;
                }
                in16_bit_buffer += in_stride_diff;
                out8_bit_buffer += out8_stride_diff;
            }
        }
        else if (!(width & 15))
        {
            __m128i in_pixel2, inPixel3;

            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 16)
                {
                    in_pixel0 = _mm_loadu_si128((__m128i*) in16_bit_buffer);
                    in_pixel1 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + 8));
                    in_pixel2 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride));
                    inPixel3 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride + 8));

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));

                    _mm_storeu_si128((__m128i*)out8_bit_buffer, inPixel0_shftR_2_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride), inPixel1_shftR_2_U8);

                    out8_bit_buffer += 16;
                    in16_bit_buffer += 16;
                }
                in16_bit_buffer += in_stride_diff;
                out8_bit_buffer += out8_stride_diff;
            }
        }
        else if (!(width & 7))
        {
            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 8)
                {
                    in_pixel0 = _mm_loadu_si128((__m128i*) in16_bit_buffer);
                    in_pixel1 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride));

                    inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF);
                    inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF);

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

                    _mm_storel_epi64((__m128i*)out8_bit_buffer, inPixel0_shftR_2_U8);
                    _mm_storel_epi64((__m128i*)(out8_bit_buffer + out8_stride), inPixel1_shftR_2_U8);

                    out8_bit_buffer += 8;
                    in16_bit_buffer += 8;
                }
                in16_bit_buffer += in_stride_diff;
                out8_bit_buffer += out8_stride_diff;
            }
        }
        else
        {
            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 4)
                {
                    in_pixel0 = _mm_loadl_epi64((__m128i*)in16_bit_buffer);
                    in_pixel1 = _mm_loadl_epi64((__m128i*)(in16_bit_buffer + in_stride));

                    inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF);
                    inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF);

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

                    *(uint32_t*)out8_bit_buffer = _mm_cvtsi128_si32(inPixel0_shftR_2_U8);
                    *(uint32_t*)(out8_bit_buffer + out8_stride) = _mm_cvtsi128_si32(inPixel1_shftR_2_U8);

                    out8_bit_buffer += 4;
                    in16_bit_buffer += 4;
                }
                in16_bit_buffer += in_stride_diff;
                out8_bit_buffer += out8_stride_diff;
            }
        }

    }

    return;
}

void eb_enc_un_pack8_bit_data_sse2_intrin(
    uint16_t  *in16_bit_buffer,
    uint32_t   in_stride,
    uint8_t   *out8_bit_buffer,
    uint32_t   out8_stride,
    uint32_t   width,
    uint32_t   height)
{
    uint32_t x, y;

    __m128i xmm_00FF, in_pixel0, in_pixel1, inPixel1_shftR_2_U8, inPixel0_shftR_2_U8, inPixel0_shftR_2, inPixel1_shftR_2;
//    __m128i tempPixel0_U8, tempPixel1_U8;

    xmm_00FF = _mm_set1_epi16(0x00FF);

    if (width == 4)
    {
        for (y = 0; y < height; y += 2)
        {
            in_pixel0 = _mm_loadl_epi64((__m128i*)in16_bit_buffer);
            in_pixel1 = _mm_loadl_epi64((__m128i*)(in16_bit_buffer + in_stride));

            //tempPixel0 = _mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6);
            //tempPixel1 = _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6);
            //
            //tempPixel0_U8 = _mm_packus_epi16(tempPixel0, tempPixel0);
            //tempPixel1_U8 = _mm_packus_epi16(tempPixel1, tempPixel1);

            inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF);
            inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF);

            inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
            inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

            //*(uint32_t*)outn_bit_buffer = _mm_cvtsi128_si32(tempPixel0_U8);
            //*(uint32_t*)(outn_bit_buffer + outn_stride) = _mm_cvtsi128_si32(tempPixel1_U8);
            *(uint32_t*)out8_bit_buffer = _mm_cvtsi128_si32(inPixel0_shftR_2_U8);
            *(uint32_t*)(out8_bit_buffer + out8_stride) = _mm_cvtsi128_si32(inPixel1_shftR_2_U8);

            //outn_bit_buffer += 2 * outn_stride;
            out8_bit_buffer += 2 * out8_stride;
            in16_bit_buffer += 2 * in_stride;
        }
    }
    else if (width == 8)
    {
        for (y = 0; y < height; y += 2)
        {
            in_pixel0 = _mm_loadu_si128((__m128i*) in16_bit_buffer);
            in_pixel1 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride));

            //tempPixel0 = _mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6);
            //tempPixel1 = _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6);
            //
            //tempPixel0_U8 = _mm_packus_epi16(tempPixel0, tempPixel0);
            //tempPixel1_U8 = _mm_packus_epi16(tempPixel1, tempPixel1);

            inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF);
            inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF);

            inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
            inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

            //_mm_storel_epi64((__m128i*)outn_bit_buffer, tempPixel0_U8);
            //_mm_storel_epi64((__m128i*)(outn_bit_buffer + outn_stride), tempPixel1_U8);
            _mm_storel_epi64((__m128i*)out8_bit_buffer, inPixel0_shftR_2_U8);
            _mm_storel_epi64((__m128i*)(out8_bit_buffer + out8_stride), inPixel1_shftR_2_U8);

            //outn_bit_buffer += 2 * outn_stride;
            out8_bit_buffer += 2 * out8_stride;
            in16_bit_buffer += 2 * in_stride;
        }
    }
    else if (width == 16)
    {
        __m128i in_pixel2, inPixel3;

        for (y = 0; y < height; y += 2)
        {
            in_pixel0 = _mm_loadu_si128((__m128i*) in16_bit_buffer);
            in_pixel1 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + 8));
            in_pixel2 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride));
            inPixel3 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride + 8));

            //tempPixel0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6));
            //tempPixel1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));

            inPixel0_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
            inPixel1_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));

            //_mm_storeu_si128((__m128i*)outn_bit_buffer, tempPixel0_U8);
            //_mm_storeu_si128((__m128i*)(outn_bit_buffer + outn_stride), tempPixel1_U8);
            _mm_storeu_si128((__m128i*)out8_bit_buffer, inPixel0_shftR_2_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride), inPixel1_shftR_2_U8);

            //outn_bit_buffer += 2 * outn_stride;
            out8_bit_buffer += 2 * out8_stride;
            in16_bit_buffer += 2 * in_stride;
        }
    }
    else if (width == 32)
    {
         __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
         __m128i /*outn0_U8, outn1_U8, outn2_U8, outn3_U8,*/ out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

        for (y = 0; y < height; y += 2)
        {
            in_pixel0 = _mm_loadu_si128((__m128i*)in16_bit_buffer);
            in_pixel1 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 8));
            in_pixel2 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 24));
            in_pixel4 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + in_stride));
            in_pixel5 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + in_stride + 8));
            in_pixel6 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride + 16));
            in_pixel7 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride + 24));

            //outn0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6));
            //outn1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
            //outn2_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel4, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel5, xmm_3), 6));
            //outn3_U8 = _mm_packus_epi16(_mm_and_si128(in_pixel6, xmm_3), _mm_and_si128(in_pixel7, xmm_3));

            out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
            out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
            out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel5, 2), xmm_00FF));
            out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel7, 2), xmm_00FF));

            //_mm_storeu_si128((__m128i*)outn_bit_buffer, outn0_U8);
            //_mm_storeu_si128((__m128i*)(outn_bit_buffer + 16), outn1_U8);
            //_mm_storeu_si128((__m128i*)(outn_bit_buffer + outn_stride), outn2_U8);
            //_mm_storeu_si128((__m128i*)(outn_bit_buffer + outn_stride + 16), outn3_U8);

            _mm_storeu_si128((__m128i*)out8_bit_buffer, out8_0_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + 16), out8_1_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride), out8_2_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride + 16), out8_3_U8);

            //outn_bit_buffer += 2 * outn_stride;
            out8_bit_buffer += 2 * out8_stride;
            in16_bit_buffer += 2 * in_stride;
        }
    }
    else if (width == 64)
    {
        __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
        __m128i /*outn0_U8, outn1_U8, outn2_U8, outn3_U8,*/ out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

        for (y = 0; y < height; ++y)
        {
            in_pixel0 = _mm_loadu_si128((__m128i*)in16_bit_buffer);
            in_pixel1 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 8));
            in_pixel2 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 24));
            in_pixel4 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 32));
            in_pixel5 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 40));
            in_pixel6 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 48));
            in_pixel7 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 56));

            //outn0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6));
            //outn1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
            //outn2_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel4, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel5, xmm_3), 6));
            //outn3_U8 = _mm_packus_epi16(_mm_and_si128(in_pixel6, xmm_3), _mm_and_si128(in_pixel7, xmm_3));

            out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
            out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
            out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel5, 2), xmm_00FF));
            out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel7, 2), xmm_00FF));

            //_mm_storeu_si128((__m128i*)outn_bit_buffer, outn0_U8);
            //_mm_storeu_si128((__m128i*)(outn_bit_buffer + 16), outn1_U8);
            //_mm_storeu_si128((__m128i*)(outn_bit_buffer + 32), outn2_U8);
            //_mm_storeu_si128((__m128i*)(outn_bit_buffer + 48), outn3_U8);

            _mm_storeu_si128((__m128i*)out8_bit_buffer, out8_0_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + 16), out8_1_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + 32), out8_2_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + 48), out8_3_U8);

            //outn_bit_buffer += outn_stride;
            out8_bit_buffer += out8_stride;
            in16_bit_buffer += in_stride;
        }
    }
    else
    {
        uint32_t in_stride_diff = (2 * in_stride) - width;
        uint32_t out8_stride_diff = (2 * out8_stride) - width;
        //uint32_t outnStrideDiff = (2 * outn_stride) - width;

        uint32_t in_stride_diff64 = in_stride - width;
        uint32_t out8_stride_diff64 = out8_stride - width;
        //uint32_t outnStrideDiff64 = outn_stride - width;

        if (!(width & 63))
        {
            __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
            __m128i /*outn0_U8, outn1_U8, outn2_U8, outn3_U8, */out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

            for (x = 0; x < height; x += 1){
                for (y = 0; y < width; y += 64){

                    in_pixel0 = _mm_loadu_si128((__m128i*)in16_bit_buffer);
                    in_pixel1 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 8));
                    in_pixel2 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 16));
                    inPixel3 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 24));
                    in_pixel4 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 32));
                    in_pixel5 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 40));
                    in_pixel6 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 48));
                    in_pixel7 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 56));

                    //outn0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6));
                    //outn1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
                    //outn2_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel4, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel5, xmm_3), 6));
                    //outn3_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel6, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel7, xmm_3), 6));

                    out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
                    out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
                    out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel5, 2), xmm_00FF));
                    out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel7, 2), xmm_00FF));

                    //_mm_storeu_si128((__m128i*)outn_bit_buffer, outn0_U8);
                    //_mm_storeu_si128((__m128i*)(outn_bit_buffer + 16), outn1_U8);
                    //_mm_storeu_si128((__m128i*)(outn_bit_buffer + 32), outn2_U8);
                    //_mm_storeu_si128((__m128i*)(outn_bit_buffer + 48), outn3_U8);

                    _mm_storeu_si128((__m128i*)out8_bit_buffer, out8_0_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + 16), out8_1_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + 32), out8_2_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + 48), out8_3_U8);

                    //outn_bit_buffer += 64;
                    out8_bit_buffer += 64;
                    in16_bit_buffer += 64;
                }
                in16_bit_buffer += in_stride_diff64;
                //outn_bit_buffer += outnStrideDiff64;
                out8_bit_buffer += out8_stride_diff64;
            }
        }
        else if (!(width & 31))
        {
            __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
            __m128i /*outn0_U8, outn1_U8, outn2_U8, outn3_U8,*/ out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 32)
                {
                    in_pixel0 = _mm_loadu_si128((__m128i*)in16_bit_buffer);
                    in_pixel1 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 8));
                    in_pixel2 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 16));
                    inPixel3 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 24));
                    in_pixel4 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + in_stride));
                    in_pixel5 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + in_stride + 8));
                    in_pixel6 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride + 16));
                    in_pixel7 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride + 24));

                    //outn0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6));
                    //outn1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
                    //outn2_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel4, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel5, xmm_3), 6));
                    //outn3_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel6, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel7, xmm_3), 6));

                    out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
                    out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
                    out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel5, 2), xmm_00FF));
                    out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel7, 2), xmm_00FF));

                    //_mm_storeu_si128((__m128i*)outn_bit_buffer, outn0_U8);
                    //_mm_storeu_si128((__m128i*)(outn_bit_buffer + 16), outn1_U8);
                    //_mm_storeu_si128((__m128i*)(outn_bit_buffer + outn_stride), outn2_U8);
                    //_mm_storeu_si128((__m128i*)(outn_bit_buffer + outn_stride + 16), outn3_U8);

                    _mm_storeu_si128((__m128i*)out8_bit_buffer, out8_0_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + 16), out8_1_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride), out8_2_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride + 16), out8_3_U8);

                    //outn_bit_buffer += 32;
                    out8_bit_buffer += 32;
                    in16_bit_buffer += 32;
                }
                in16_bit_buffer += in_stride_diff;
                //outn_bit_buffer += outnStrideDiff;
                out8_bit_buffer += out8_stride_diff;
            }
        }
        else if (!(width & 15))
        {
            __m128i in_pixel2, inPixel3;

            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 16)
                {
                    in_pixel0 = _mm_loadu_si128((__m128i*) in16_bit_buffer);
                    in_pixel1 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + 8));
                    in_pixel2 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride));
                    inPixel3 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride + 8));

                    //tempPixel0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6));
                    //tempPixel1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
                    //
                    inPixel0_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));

                    //_mm_storeu_si128((__m128i*)outn_bit_buffer, tempPixel0_U8);
                    //_mm_storeu_si128((__m128i*)(outn_bit_buffer + outn_stride), tempPixel1_U8);
                    _mm_storeu_si128((__m128i*)out8_bit_buffer, inPixel0_shftR_2_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride), inPixel1_shftR_2_U8);

                    //outn_bit_buffer += 16;
                    out8_bit_buffer += 16;
                    in16_bit_buffer += 16;
                }
                in16_bit_buffer += in_stride_diff;
                //outn_bit_buffer += outnStrideDiff;
                out8_bit_buffer += out8_stride_diff;
            }
        }
        else if (!(width & 7))
        {
            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 8)
                {
                    in_pixel0 = _mm_loadu_si128((__m128i*) in16_bit_buffer);
                    in_pixel1 = _mm_loadu_si128((__m128i*) (in16_bit_buffer + in_stride));

                    //tempPixel0_U8 = _mm_packus_epi16(tempPixel0, tempPixel0);
                    //tempPixel1_U8 = _mm_packus_epi16(tempPixel1, tempPixel1);

                    inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF);
                    inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF);

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

                    //_mm_storel_epi64((__m128i*)outn_bit_buffer, tempPixel0_U8);
                    //_mm_storel_epi64((__m128i*)(outn_bit_buffer + outn_stride), tempPixel1_U8);
                    _mm_storel_epi64((__m128i*)out8_bit_buffer, inPixel0_shftR_2_U8);
                    _mm_storel_epi64((__m128i*)(out8_bit_buffer + out8_stride), inPixel1_shftR_2_U8);

                    //outn_bit_buffer += 8;
                    out8_bit_buffer += 8;
                    in16_bit_buffer += 8;
                }
                in16_bit_buffer += in_stride_diff;
                //outn_bit_buffer += outnStrideDiff;
                out8_bit_buffer += out8_stride_diff;
            }
        }
        else
        {
            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 4)
                {
                    in_pixel0 = _mm_loadl_epi64((__m128i*)in16_bit_buffer);
                    in_pixel1 = _mm_loadl_epi64((__m128i*)(in16_bit_buffer + in_stride));

                    //tempPixel0_U8 = _mm_packus_epi16(tempPixel0, tempPixel0);
                    //tempPixel1_U8 = _mm_packus_epi16(tempPixel1, tempPixel1);

                    inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF);
                    inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF);

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

                    //*(uint32_t*)outn_bit_buffer = _mm_cvtsi128_si32(tempPixel0_U8);
                    //*(uint32_t*)(outn_bit_buffer + outn_stride) = _mm_cvtsi128_si32(tempPixel1_U8);
                    *(uint32_t*)out8_bit_buffer = _mm_cvtsi128_si32(inPixel0_shftR_2_U8);
                    *(uint32_t*)(out8_bit_buffer + out8_stride) = _mm_cvtsi128_si32(inPixel1_shftR_2_U8);

                    //outn_bit_buffer += 4;
                    out8_bit_buffer += 4;
                    in16_bit_buffer += 4;
                }
                in16_bit_buffer += in_stride_diff;
                //outn_bit_buffer += outnStrideDiff;
                out8_bit_buffer += out8_stride_diff;
            }
        }
    }

    return;
}
void eb_vp9_unpack_avg_sse2_intrin(
    uint16_t *ref16_l0,
    uint32_t  ref_l0_stride,
    uint16_t *ref16_l1,
    uint32_t  ref_l1_stride,
    uint8_t  *dst_ptr,
    uint32_t  dst_stride,
    uint32_t  width,
    uint32_t  height)
{

    uint32_t  y;
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

            _mm_storeu_si128((__m128i*) dst_ptr      , avg8_0_U8);

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

            _mm_storeu_si128((__m128i*)(dst_ptr  + dst_stride      ) , avg8_2_U8);

            dst_ptr  += 2*dst_stride;
            ref16_l0 += 2*ref_l0_stride;
            ref16_l1 += 2*ref_l1_stride;

        }

    }
    else if (width == 32)
    {
        __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
        __m128i out8_0_U8_L0, out8_1_U8_L0, out8_2_U8_L0, out8_3_U8_L0;
        __m128i out8_0_U8_L1, out8_1_U8_L1, out8_2_U8_L1, out8_3_U8_L1;
        __m128i avg8_0_U8, avg8_1_U8, avg8_2_U8, avg8_3_U8;

       for (y = 0; y < height; y += 2)
        {
            //--------
            //Line One
            //--------

            //List0

            in_pixel0 =  _mm_loadu_si128((__m128i*)  ref16_l0);
            in_pixel1 =  _mm_loadu_si128((__m128i*) (ref16_l0 + 8));
            in_pixel2 =  _mm_loadu_si128((__m128i*) (ref16_l0 + 16));
            inPixel3 =  _mm_loadu_si128((__m128i*) (ref16_l0 + 24));

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

            _mm_storeu_si128((__m128i*) dst_ptr      , avg8_0_U8);
            _mm_storeu_si128((__m128i*)(dst_ptr + 16), avg8_1_U8);

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

            _mm_storeu_si128((__m128i*)(dst_ptr  + dst_stride      ) , avg8_2_U8);
            _mm_storeu_si128((__m128i*)(dst_ptr  + dst_stride + 16 ) , avg8_3_U8);

            dst_ptr  += 2*dst_stride;
            ref16_l0 += 2*ref_l0_stride;
            ref16_l1 += 2*ref_l1_stride;

        }

    }
    else if (width == 64)
    {
        __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
        __m128i out8_0_U8_L0, out8_1_U8_L0, out8_2_U8_L0, out8_3_U8_L0;
        __m128i out8_0_U8_L1, out8_1_U8_L1, out8_2_U8_L1, out8_3_U8_L1;
        __m128i avg8_0_U8, avg8_1_U8, avg8_2_U8, avg8_3_U8;

        for (y = 0; y < height; ++y)
        {
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

            _mm_storeu_si128((__m128i*) dst_ptr      , avg8_0_U8);
            _mm_storeu_si128((__m128i*)(dst_ptr + 16), avg8_1_U8);
            _mm_storeu_si128((__m128i*)(dst_ptr + 32), avg8_2_U8);
            _mm_storeu_si128((__m128i*)(dst_ptr + 48), avg8_3_U8);

            dst_ptr  += dst_stride;
            ref16_l0 += ref_l0_stride;
            ref16_l1 += ref_l1_stride;
        }
    }

    return;
}
