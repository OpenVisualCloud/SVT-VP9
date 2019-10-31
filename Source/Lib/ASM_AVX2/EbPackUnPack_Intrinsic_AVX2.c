/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <immintrin.h>

#include "EbPackUnPack_Intrinsic_AVX2.h"
#include "EbDefinitions.h"

#ifndef DISABLE_AVX512
AVX512_FUNC_TARGET
void eb_enc_msb_un_pack2_d_avx512_intrin(
    uint16_t      *in16_bit_buffer,
    uint32_t       in_stride,
    uint8_t       *out8_bit_buffer,
    uint8_t       *outn_bit_buffer,
    uint32_t       out8_stride,
    uint32_t       outn_stride,
    uint32_t       width,
    uint32_t       height)
{
    const uint32_t CHUNK_SIZE = 64;
    __m512i zmm_3 = _mm512_set1_epi16(0x0003);
    __m512i zmm_perm = _mm512_set_epi64(7, 5, 3, 1, 6, 4, 2, 0);

    uint32_t x, y;

    uint32_t out8_bit_buffer_align = CHUNK_SIZE - ((size_t)out8_bit_buffer & (CHUNK_SIZE - 1));
    uint32_t outn_bit_buffer_align = CHUNK_SIZE - ((size_t)outn_bit_buffer & (CHUNK_SIZE - 1));

    // We are going to use non temporal stores for both output buffers if they are aligned or can be aligned using same offset
    const int b_use_nt_store_for_both = (out8_bit_buffer_align == outn_bit_buffer_align) && (out8_stride == outn_stride);

    for (x = 0; x < height; x++) {
            // Find offset to the aligned store address
            uint32_t offset = CHUNK_SIZE - ((size_t)out8_bit_buffer & (CHUNK_SIZE - 1));
            uint32_t compl_offset = (width - offset) & (CHUNK_SIZE - 1);
            uint32_t num_chunks = (width - (offset + compl_offset)) / CHUNK_SIZE;

            // Process the unaligned output data pixel by pixel
            for (y = 0; y < offset; y++)
            {
                uint16_t in_pixel0 = in16_bit_buffer[y];
                out8_bit_buffer[y] = (uint8_t)(in_pixel0 >> 2);

                uint8_t tmp_pixel0 = (uint8_t)(in_pixel0 << 6);
                outn_bit_buffer[y] = tmp_pixel0;
            }
            in16_bit_buffer += offset;
            outn_bit_buffer += offset;
            out8_bit_buffer += offset;

            for (y = 0; y < num_chunks; y++) {
                // We process scan lines by elements of CHUNK_SIZE starting from an offset and after that we process the residual (or complimentary offset) going pixel by pixel

                __m512i in_pixel0 = _mm512_loadu_si512((__m512i*)in16_bit_buffer);
                __m512i in_pixel1 = _mm512_loadu_si512((__m512i*)(in16_bit_buffer + 32));

                __m512i outn0_U8 = _mm512_packus_epi16(_mm512_slli_epi16(_mm512_and_si512(in_pixel0, zmm_3), 6), _mm512_slli_epi16(_mm512_and_si512(in_pixel1, zmm_3), 6));
                __m512i out8_0_U8 = _mm512_packus_epi16(_mm512_srli_epi16(in_pixel0, 2), _mm512_srli_epi16(in_pixel1, 2));

                outn0_U8 = _mm512_permutexvar_epi64(zmm_perm, outn0_U8);
                out8_0_U8 = _mm512_permutexvar_epi64(zmm_perm, out8_0_U8);

                _mm512_stream_si512((__m512i*)out8_bit_buffer, out8_0_U8);
                if (b_use_nt_store_for_both)
                    _mm512_stream_si512((__m512i*)outn_bit_buffer, outn0_U8);
                else
                    _mm512_storeu_si512((__m512i*)outn_bit_buffer, outn0_U8);
                outn_bit_buffer += CHUNK_SIZE;
                out8_bit_buffer += CHUNK_SIZE;
                in16_bit_buffer += CHUNK_SIZE;
        }

        // Process the tail of a scan line
        for (y = 0; y < compl_offset; y++)
        {
            uint16_t in_pixel2 = in16_bit_buffer[y];
            out8_bit_buffer[y] = (uint8_t)(in_pixel2 >> 2);

            uint8_t tmp_pixel2 = (uint8_t)(in_pixel2 << 6);
            outn_bit_buffer[y] = tmp_pixel2;
        }

        in16_bit_buffer += in_stride - width + compl_offset;
        outn_bit_buffer += outn_stride - width + compl_offset;
        out8_bit_buffer += out8_stride - width + compl_offset;
    }

    return;
}
#else
void eb_enc_msb_un_pack2_d_sse2_intrin(
    uint16_t      *in16_bit_buffer,
    uint32_t       in_stride,
    uint8_t       *out8_bit_buffer,
    uint8_t       *outn_bit_buffer,
    uint32_t       out8_stride,
    uint32_t       outn_stride,
    uint32_t       width,
    uint32_t       height)
{
    uint32_t x, y;

    __m128i xmm_3, xmm_00FF, in_pixel0, in_pixel1, tempPixel0, tempPixel1, inPixel1_shftR_2_U8, inPixel0_shftR_2_U8, inPixel0_shftR_2, inPixel1_shftR_2;
    __m128i tempPixel0_U8, tempPixel1_U8;

    xmm_3 = _mm_set1_epi16(0x0003);
    xmm_00FF = _mm_set1_epi16(0x00FF);

    if (width == 4)
    {
        for (y = 0; y < height; y += 2)
        {
            in_pixel0 = _mm_loadl_epi64((__m128i*)in16_bit_buffer);
            in_pixel1 = _mm_loadl_epi64((__m128i*)(in16_bit_buffer + in_stride));

            tempPixel0 = _mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6);
            tempPixel1 = _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6);

            tempPixel0_U8 = _mm_packus_epi16(tempPixel0, tempPixel0);
            tempPixel1_U8 = _mm_packus_epi16(tempPixel1, tempPixel1);

            inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF);
            inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF);

            inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
            inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

            *(uint32_t*)outn_bit_buffer = _mm_cvtsi128_si32(tempPixel0_U8);
            *(uint32_t*)(outn_bit_buffer + outn_stride) = _mm_cvtsi128_si32(tempPixel1_U8);
            *(uint32_t*)out8_bit_buffer = _mm_cvtsi128_si32(inPixel0_shftR_2_U8);
            *(uint32_t*)(out8_bit_buffer + out8_stride) = _mm_cvtsi128_si32(inPixel1_shftR_2_U8);

            outn_bit_buffer += 2 * outn_stride;
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

            tempPixel0 = _mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6);
            tempPixel1 = _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6);

            tempPixel0_U8 = _mm_packus_epi16(tempPixel0, tempPixel0);
            tempPixel1_U8 = _mm_packus_epi16(tempPixel1, tempPixel1);

            inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF);
            inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF);

            inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
            inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

            _mm_storel_epi64((__m128i*)outn_bit_buffer, tempPixel0_U8);
            _mm_storel_epi64((__m128i*)(outn_bit_buffer + outn_stride), tempPixel1_U8);
            _mm_storel_epi64((__m128i*)out8_bit_buffer, inPixel0_shftR_2_U8);
            _mm_storel_epi64((__m128i*)(out8_bit_buffer + out8_stride), inPixel1_shftR_2_U8);

            outn_bit_buffer += 2 * outn_stride;
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

            tempPixel0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6));
            tempPixel1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));

            inPixel0_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
            inPixel1_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));

            _mm_storeu_si128((__m128i*)outn_bit_buffer, tempPixel0_U8);
            _mm_storeu_si128((__m128i*)(outn_bit_buffer + outn_stride), tempPixel1_U8);
            _mm_storeu_si128((__m128i*)out8_bit_buffer, inPixel0_shftR_2_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride), inPixel1_shftR_2_U8);

            outn_bit_buffer += 2 * outn_stride;
            out8_bit_buffer += 2 * out8_stride;
            in16_bit_buffer += 2 * in_stride;
        }
    }
    else if (width == 32)
    {
        __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
        __m128i outn0_U8, outn1_U8, outn2_U8, outn3_U8, out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

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

            outn0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6));
            outn1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
            outn2_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel4, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel5, xmm_3), 6));
            outn3_U8 = _mm_packus_epi16(_mm_and_si128(in_pixel6, xmm_3), _mm_and_si128(in_pixel7, xmm_3));

            out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
            out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
            out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel5, 2), xmm_00FF));
            out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel7, 2), xmm_00FF));

            _mm_storeu_si128((__m128i*)outn_bit_buffer, outn0_U8);
            _mm_storeu_si128((__m128i*)(outn_bit_buffer + 16), outn1_U8);
            _mm_storeu_si128((__m128i*)(outn_bit_buffer + outn_stride), outn2_U8);
            _mm_storeu_si128((__m128i*)(outn_bit_buffer + outn_stride + 16), outn3_U8);

            _mm_storeu_si128((__m128i*)out8_bit_buffer, out8_0_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + 16), out8_1_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride), out8_2_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride + 16), out8_3_U8);

            outn_bit_buffer += 2 * outn_stride;
            out8_bit_buffer += 2 * out8_stride;
            in16_bit_buffer += 2 * in_stride;
        }
    }
    else if (width == 64)
    {
        __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
        __m128i outn0_U8, outn1_U8, outn2_U8, outn3_U8, out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

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

            outn0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6));
            outn1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
            outn2_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel4, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel5, xmm_3), 6));
            outn3_U8 = _mm_packus_epi16(_mm_and_si128(in_pixel6, xmm_3), _mm_and_si128(in_pixel7, xmm_3));

            out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
            out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
            out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel5, 2), xmm_00FF));
            out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel7, 2), xmm_00FF));

            _mm_storeu_si128((__m128i*)outn_bit_buffer, outn0_U8);
            _mm_storeu_si128((__m128i*)(outn_bit_buffer + 16), outn1_U8);
            _mm_storeu_si128((__m128i*)(outn_bit_buffer + 32), outn2_U8);
            _mm_storeu_si128((__m128i*)(outn_bit_buffer + 48), outn3_U8);

            _mm_storeu_si128((__m128i*)out8_bit_buffer, out8_0_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + 16), out8_1_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + 32), out8_2_U8);
            _mm_storeu_si128((__m128i*)(out8_bit_buffer + 48), out8_3_U8);

            outn_bit_buffer += outn_stride;
            out8_bit_buffer += out8_stride;
            in16_bit_buffer += in_stride;
        }
    }
    else
    {
        uint32_t in_stride_diff = (2 * in_stride) - width;
        uint32_t out8_stride_diff = (2 * out8_stride) - width;
        uint32_t outnStrideDiff = (2 * outn_stride) - width;

        uint32_t in_stride_diff64 = in_stride - width;
        uint32_t out8_stride_diff64 = out8_stride - width;
        uint32_t outnStrideDiff64 = outn_stride - width;

        if (!(width & 63))
        {
            __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
            __m128i outn0_U8, outn1_U8, outn2_U8, outn3_U8, out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

            for (x = 0; x < height; x += 1) {
                for (y = 0; y < width; y += 64) {

                    in_pixel0 = _mm_loadu_si128((__m128i*)in16_bit_buffer);
                    in_pixel1 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 8));
                    in_pixel2 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 16));
                    inPixel3 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 24));
                    in_pixel4 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 32));
                    in_pixel5 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 40));
                    in_pixel6 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 48));
                    in_pixel7 = _mm_loadu_si128((__m128i*)(in16_bit_buffer + 56));

                    outn0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6));
                    outn1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
                    outn2_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel4, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel5, xmm_3), 6));
                    outn3_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel6, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel7, xmm_3), 6));

                    out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
                    out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
                    out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel5, 2), xmm_00FF));
                    out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel7, 2), xmm_00FF));

                    _mm_storeu_si128((__m128i*)outn_bit_buffer, outn0_U8);
                    _mm_storeu_si128((__m128i*)(outn_bit_buffer + 16), outn1_U8);
                    _mm_storeu_si128((__m128i*)(outn_bit_buffer + 32), outn2_U8);
                    _mm_storeu_si128((__m128i*)(outn_bit_buffer + 48), outn3_U8);

                    _mm_storeu_si128((__m128i*)out8_bit_buffer, out8_0_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + 16), out8_1_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + 32), out8_2_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + 48), out8_3_U8);

                    outn_bit_buffer += 64;
                    out8_bit_buffer += 64;
                    in16_bit_buffer += 64;
                }
                in16_bit_buffer += in_stride_diff64;
                outn_bit_buffer += outnStrideDiff64;
                out8_bit_buffer += out8_stride_diff64;
            }
        }
        else if (!(width & 31))
        {
            __m128i in_pixel2, inPixel3, in_pixel4, in_pixel5, in_pixel6, in_pixel7;
            __m128i outn0_U8, outn1_U8, outn2_U8, outn3_U8, out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

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

                    outn0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6));
                    outn1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
                    outn2_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel4, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel5, xmm_3), 6));
                    outn3_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel6, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel7, xmm_3), 6));

                    out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
                    out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
                    out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel5, 2), xmm_00FF));
                    out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel7, 2), xmm_00FF));

                    _mm_storeu_si128((__m128i*)outn_bit_buffer, outn0_U8);
                    _mm_storeu_si128((__m128i*)(outn_bit_buffer + 16), outn1_U8);
                    _mm_storeu_si128((__m128i*)(outn_bit_buffer + outn_stride), outn2_U8);
                    _mm_storeu_si128((__m128i*)(outn_bit_buffer + outn_stride + 16), outn3_U8);

                    _mm_storeu_si128((__m128i*)out8_bit_buffer, out8_0_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + 16), out8_1_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride), out8_2_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride + 16), out8_3_U8);

                    outn_bit_buffer += 32;
                    out8_bit_buffer += 32;
                    in16_bit_buffer += 32;
                }
                in16_bit_buffer += in_stride_diff;
                outn_bit_buffer += outnStrideDiff;
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

                    tempPixel0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6));
                    tempPixel1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(in_pixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF));
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(in_pixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));

                    _mm_storeu_si128((__m128i*)outn_bit_buffer, tempPixel0_U8);
                    _mm_storeu_si128((__m128i*)(outn_bit_buffer + outn_stride), tempPixel1_U8);
                    _mm_storeu_si128((__m128i*)out8_bit_buffer, inPixel0_shftR_2_U8);
                    _mm_storeu_si128((__m128i*)(out8_bit_buffer + out8_stride), inPixel1_shftR_2_U8);

                    outn_bit_buffer += 16;
                    out8_bit_buffer += 16;
                    in16_bit_buffer += 16;
                }
                in16_bit_buffer += in_stride_diff;
                outn_bit_buffer += outnStrideDiff;
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

                    tempPixel0 = _mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6);
                    tempPixel1 = _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6);

                    tempPixel0_U8 = _mm_packus_epi16(tempPixel0, tempPixel0);
                    tempPixel1_U8 = _mm_packus_epi16(tempPixel1, tempPixel1);

                    inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF);
                    inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF);

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

                    _mm_storel_epi64((__m128i*)outn_bit_buffer, tempPixel0_U8);
                    _mm_storel_epi64((__m128i*)(outn_bit_buffer + outn_stride), tempPixel1_U8);
                    _mm_storel_epi64((__m128i*)out8_bit_buffer, inPixel0_shftR_2_U8);
                    _mm_storel_epi64((__m128i*)(out8_bit_buffer + out8_stride), inPixel1_shftR_2_U8);

                    outn_bit_buffer += 8;
                    out8_bit_buffer += 8;
                    in16_bit_buffer += 8;
                }
                in16_bit_buffer += in_stride_diff;
                outn_bit_buffer += outnStrideDiff;
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

                    tempPixel0 = _mm_slli_epi16(_mm_and_si128(in_pixel0, xmm_3), 6);
                    tempPixel1 = _mm_slli_epi16(_mm_and_si128(in_pixel1, xmm_3), 6);

                    tempPixel0_U8 = _mm_packus_epi16(tempPixel0, tempPixel0);
                    tempPixel1_U8 = _mm_packus_epi16(tempPixel1, tempPixel1);

                    inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel0, 2), xmm_00FF);
                    inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(in_pixel1, 2), xmm_00FF);

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

                    *(uint32_t*)outn_bit_buffer = _mm_cvtsi128_si32(tempPixel0_U8);
                    *(uint32_t*)(outn_bit_buffer + outn_stride) = _mm_cvtsi128_si32(tempPixel1_U8);
                    *(uint32_t*)out8_bit_buffer = _mm_cvtsi128_si32(inPixel0_shftR_2_U8);
                    *(uint32_t*)(out8_bit_buffer + out8_stride) = _mm_cvtsi128_si32(inPixel1_shftR_2_U8);

                    outn_bit_buffer += 4;
                    out8_bit_buffer += 4;
                    in16_bit_buffer += 4;
                }
                in16_bit_buffer += in_stride_diff;
                outn_bit_buffer += outnStrideDiff;
                out8_bit_buffer += out8_stride_diff;
            }
        }
    }
    return;
}

#endif
