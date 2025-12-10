/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbPictureOperators_SSE2.h"
#include <emmintrin.h>
#include "EbDefinitions.h"

// Note: maximum energy within 1 TU considered to be 2^30-e
// All functions can accumulate up to 4 TUs to stay within 32-bit unsigned range

void full_distortion_kernel_8x8_32bit_bt_sse2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                              uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                              uint32_t area_width, uint32_t area_height) {
    int32_t row_count;

    __m128i sum = _mm_setzero_si128();

    row_count = 8;
    do {
        __m128i x0;
        __m128i y0;

        x0 = _mm_loadu_si128((__m128i *)(coeff + 0x00));
        y0 = _mm_loadu_si128((__m128i *)(recon_coeff + 0x00));
        coeff += coeff_stride;
        recon_coeff += recon_coeff_stride;

        x0 = _mm_sub_epi16(x0, y0);

        x0 = _mm_madd_epi16(x0, x0);

        sum = _mm_add_epi32(sum, x0);
    } while (--row_count);

    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4e)); // 01001110
    sum = _mm_unpacklo_epi32(sum, sum);
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4e)); // 01001110
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(sum, _mm_setzero_si128()));

    (void)area_width;
    (void)area_height;
}

void full_distortion_kernel_intra_16_mxn_32_bit_bt_sse2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                                        uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                                        uint32_t area_width, uint32_t area_height) {
    int32_t row_count, col_count;
    __m128i sum = _mm_setzero_si128();

    col_count = area_width;
    do {
        int16_t *coeff_temp     = coeff;
        int16_t *reconCoeffTemp = recon_coeff;

        row_count = area_height;
        do {
            __m128i x0, x1;
            __m128i y0, y1;

            x0 = _mm_loadu_si128((__m128i *)(coeff_temp + 0x00));
            x1 = _mm_loadu_si128((__m128i *)(coeff_temp + 0x08));
            y0 = _mm_loadu_si128((__m128i *)(reconCoeffTemp + 0x00));
            y1 = _mm_loadu_si128((__m128i *)(reconCoeffTemp + 0x08));
            coeff_temp += coeff_stride;
            reconCoeffTemp += recon_coeff_stride;

            x0 = _mm_sub_epi16(x0, y0);
            x1 = _mm_sub_epi16(x1, y1);

            x0 = _mm_madd_epi16(x0, x0);
            x1 = _mm_madd_epi16(x1, x1);

            sum = _mm_add_epi32(sum, x0);
            sum = _mm_add_epi32(sum, x1);
        } while (--row_count);

        coeff += 16;
        recon_coeff += 16;
        col_count -= 16;
    } while (col_count > 0);

    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4e)); // 01001110
    sum = _mm_unpacklo_epi32(sum, sum);
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4e)); // 01001110
    _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(sum, _mm_setzero_si128()));
}

/*******************************************************************************
                         PictureCopyKernel_INTRIN
*******************************************************************************/
void eb_vp9_picture_copy_kernel4x4_sse_intrin(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride,
                                              uint32_t area_width, uint32_t area_height) {
    *(uint32_t *)dst                       = *(uint32_t *)src;
    *(uint32_t *)(dst + dst_stride)        = *(uint32_t *)(src + src_stride);
    *(uint32_t *)(dst + (dst_stride << 1)) = *(uint32_t *)(src + (src_stride << 1));
    *(uint32_t *)(dst + (dst_stride * 3))  = *(uint32_t *)(src + (src_stride * 3));

    (void)area_width;
    (void)area_height;

    return;
}

void eb_vp9_picture_copy_kernel8x8_sse2_intrin(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride,
                                               uint32_t area_width, uint32_t area_height) {
    _mm_storel_epi64((__m128i *)dst, _mm_cvtsi64_si128(*(uint64_t *)src));
    _mm_storel_epi64((__m128i *)(dst + dst_stride), _mm_cvtsi64_si128(*(uint64_t *)(src + src_stride)));
    _mm_storel_epi64((__m128i *)(dst + (dst_stride << 1)), _mm_cvtsi64_si128(*(uint64_t *)(src + (src_stride << 1))));
    _mm_storel_epi64((__m128i *)(dst + 3 * dst_stride), _mm_cvtsi64_si128(*(uint64_t *)(src + 3 * src_stride)));

    src += (src_stride << 2);
    dst += (dst_stride << 2);

    _mm_storel_epi64((__m128i *)dst, _mm_cvtsi64_si128(*(uint64_t *)src));
    _mm_storel_epi64((__m128i *)(dst + dst_stride), _mm_cvtsi64_si128(*(uint64_t *)(src + src_stride)));
    _mm_storel_epi64((__m128i *)(dst + (dst_stride << 1)), _mm_cvtsi64_si128(*(uint64_t *)(src + (src_stride << 1))));
    _mm_storel_epi64((__m128i *)(dst + 3 * dst_stride), _mm_cvtsi64_si128(*(uint64_t *)(src + 3 * src_stride)));

    (void)area_width;
    (void)area_height;

    return;
}

void eb_vp9_picture_copy_kernel16x16_sse2_intrin(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride,
                                                 uint32_t area_width, uint32_t area_height) {
    _mm_storeu_si128((__m128i *)dst, _mm_loadu_si128((__m128i *)src));
    _mm_storeu_si128((__m128i *)(dst + dst_stride), _mm_loadu_si128((__m128i *)(src + src_stride)));
    _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i *)(src + (src_stride << 1))));
    _mm_storeu_si128((__m128i *)(dst + (dst_stride * 3)), _mm_loadu_si128((__m128i *)(src + (src_stride * 3))));

    src += (src_stride << 2);
    dst += (dst_stride << 2);

    _mm_storeu_si128((__m128i *)dst, _mm_loadu_si128((__m128i *)src));
    _mm_storeu_si128((__m128i *)(dst + dst_stride), _mm_loadu_si128((__m128i *)(src + src_stride)));
    _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i *)(src + (src_stride << 1))));
    _mm_storeu_si128((__m128i *)(dst + (dst_stride * 3)), _mm_loadu_si128((__m128i *)(src + (src_stride * 3))));

    src += (src_stride << 2);
    dst += (dst_stride << 2);

    _mm_storeu_si128((__m128i *)dst, _mm_loadu_si128((__m128i *)src));
    _mm_storeu_si128((__m128i *)(dst + dst_stride), _mm_loadu_si128((__m128i *)(src + src_stride)));
    _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i *)(src + (src_stride << 1))));
    _mm_storeu_si128((__m128i *)(dst + (dst_stride * 3)), _mm_loadu_si128((__m128i *)(src + (src_stride * 3))));

    src += (src_stride << 2);
    dst += (dst_stride << 2);

    _mm_storeu_si128((__m128i *)dst, _mm_loadu_si128((__m128i *)src));
    _mm_storeu_si128((__m128i *)(dst + dst_stride), _mm_loadu_si128((__m128i *)(src + src_stride)));
    _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i *)(src + (src_stride << 1))));
    _mm_storeu_si128((__m128i *)(dst + (dst_stride * 3)), _mm_loadu_si128((__m128i *)(src + (src_stride * 3))));

    src += (src_stride << 2);
    dst += (dst_stride << 2);

    (void)area_width;
    (void)area_height;

    return;
}

void eb_vp9_picture_copy_kernel32x32_sse2_intrin(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride,
                                                 uint32_t area_width, uint32_t area_height) {
    uint32_t y;

    for (y = 0; y < 4; ++y) {
        _mm_storeu_si128((__m128i *)dst, _mm_loadu_si128((__m128i *)src));
        _mm_storeu_si128((__m128i *)(dst + 16), _mm_loadu_si128((__m128i *)(src + 16)));
        _mm_storeu_si128((__m128i *)(dst + dst_stride), _mm_loadu_si128((__m128i *)(src + src_stride)));
        _mm_storeu_si128((__m128i *)(dst + dst_stride + 16), _mm_loadu_si128((__m128i *)(src + src_stride + 16)));
        _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i *)(src + (src_stride << 1))));
        _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1) + 16),
                         _mm_loadu_si128((__m128i *)(src + (src_stride << 1) + 16)));
        _mm_storeu_si128((__m128i *)(dst + 3 * dst_stride), _mm_loadu_si128((__m128i *)(src + 3 * src_stride)));
        _mm_storeu_si128((__m128i *)(dst + 3 * dst_stride + 16),
                         _mm_loadu_si128((__m128i *)(src + 3 * src_stride + 16)));

        src += (src_stride << 2);
        dst += (dst_stride << 2);

        _mm_storeu_si128((__m128i *)dst, _mm_loadu_si128((__m128i *)src));
        _mm_storeu_si128((__m128i *)(dst + 16), _mm_loadu_si128((__m128i *)(src + 16)));
        _mm_storeu_si128((__m128i *)(dst + dst_stride), _mm_loadu_si128((__m128i *)(src + src_stride)));
        _mm_storeu_si128((__m128i *)(dst + dst_stride + 16), _mm_loadu_si128((__m128i *)(src + src_stride + 16)));
        _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i *)(src + (src_stride << 1))));
        _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1) + 16),
                         _mm_loadu_si128((__m128i *)(src + (src_stride << 1) + 16)));
        _mm_storeu_si128((__m128i *)(dst + 3 * dst_stride), _mm_loadu_si128((__m128i *)(src + 3 * src_stride)));
        _mm_storeu_si128((__m128i *)(dst + 3 * dst_stride + 16),
                         _mm_loadu_si128((__m128i *)(src + 3 * src_stride + 16)));

        src += (src_stride << 2);
        dst += (dst_stride << 2);
    }
    (void)area_width;
    (void)area_height;

    return;
}

void eb_vp9_picture_copy_kernel64x64_sse2_intrin(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride,
                                                 uint32_t area_width, uint32_t area_height) {
    uint32_t y;

    for (y = 0; y < 8; ++y) {
        _mm_storeu_si128((__m128i *)dst, _mm_loadu_si128((__m128i *)src));
        _mm_storeu_si128((__m128i *)(dst + 16), _mm_loadu_si128((__m128i *)(src + 16)));
        _mm_storeu_si128((__m128i *)(dst + 32), _mm_loadu_si128((__m128i *)(src + 32)));
        _mm_storeu_si128((__m128i *)(dst + 48), _mm_loadu_si128((__m128i *)(src + 48)));
        _mm_storeu_si128((__m128i *)(dst + dst_stride), _mm_loadu_si128((__m128i *)(src + src_stride)));
        _mm_storeu_si128((__m128i *)(dst + dst_stride + 16), _mm_loadu_si128((__m128i *)(src + src_stride + 16)));
        _mm_storeu_si128((__m128i *)(dst + dst_stride + 32), _mm_loadu_si128((__m128i *)(src + src_stride + 32)));
        _mm_storeu_si128((__m128i *)(dst + dst_stride + 48), _mm_loadu_si128((__m128i *)(src + src_stride + 48)));
        _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i *)(src + (src_stride << 1))));
        _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1) + 16),
                         _mm_loadu_si128((__m128i *)(src + (src_stride << 1) + 16)));
        _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1) + 32),
                         _mm_loadu_si128((__m128i *)(src + (src_stride << 1) + 32)));
        _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1) + 48),
                         _mm_loadu_si128((__m128i *)(src + (src_stride << 1) + 48)));
        _mm_storeu_si128((__m128i *)(dst + 3 * dst_stride), _mm_loadu_si128((__m128i *)(src + 3 * src_stride)));
        _mm_storeu_si128((__m128i *)(dst + 3 * dst_stride + 16),
                         _mm_loadu_si128((__m128i *)(src + 3 * src_stride + 16)));
        _mm_storeu_si128((__m128i *)(dst + 3 * dst_stride + 32),
                         _mm_loadu_si128((__m128i *)(src + 3 * src_stride + 32)));
        _mm_storeu_si128((__m128i *)(dst + 3 * dst_stride + 48),
                         _mm_loadu_si128((__m128i *)(src + 3 * src_stride + 48)));

        src += (src_stride << 2);
        dst += (dst_stride << 2);

        _mm_storeu_si128((__m128i *)dst, _mm_loadu_si128((__m128i *)src));
        _mm_storeu_si128((__m128i *)(dst + 16), _mm_loadu_si128((__m128i *)(src + 16)));
        _mm_storeu_si128((__m128i *)(dst + 32), _mm_loadu_si128((__m128i *)(src + 32)));
        _mm_storeu_si128((__m128i *)(dst + 48), _mm_loadu_si128((__m128i *)(src + 48)));
        _mm_storeu_si128((__m128i *)(dst + dst_stride), _mm_loadu_si128((__m128i *)(src + src_stride)));
        _mm_storeu_si128((__m128i *)(dst + dst_stride + 16), _mm_loadu_si128((__m128i *)(src + src_stride + 16)));
        _mm_storeu_si128((__m128i *)(dst + dst_stride + 32), _mm_loadu_si128((__m128i *)(src + src_stride + 32)));
        _mm_storeu_si128((__m128i *)(dst + dst_stride + 48), _mm_loadu_si128((__m128i *)(src + src_stride + 48)));
        _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i *)(src + (src_stride << 1))));
        _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1) + 16),
                         _mm_loadu_si128((__m128i *)(src + (src_stride << 1) + 16)));
        _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1) + 32),
                         _mm_loadu_si128((__m128i *)(src + (src_stride << 1) + 32)));
        _mm_storeu_si128((__m128i *)(dst + (dst_stride << 1) + 48),
                         _mm_loadu_si128((__m128i *)(src + (src_stride << 1) + 48)));
        _mm_storeu_si128((__m128i *)(dst + 3 * dst_stride), _mm_loadu_si128((__m128i *)(src + 3 * src_stride)));
        _mm_storeu_si128((__m128i *)(dst + 3 * dst_stride + 16),
                         _mm_loadu_si128((__m128i *)(src + 3 * src_stride + 16)));
        _mm_storeu_si128((__m128i *)(dst + 3 * dst_stride + 32),
                         _mm_loadu_si128((__m128i *)(src + 3 * src_stride + 32)));
        _mm_storeu_si128((__m128i *)(dst + 3 * dst_stride + 48),
                         _mm_loadu_si128((__m128i *)(src + 3 * src_stride + 48)));

        src += (src_stride << 2);
        dst += (dst_stride << 2);
    }
    (void)area_width;
    (void)area_height;

    return;
}
