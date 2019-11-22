/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <immintrin.h>

#include "mem_sse2.h"
#include "vpx_dsp_rtcd.h"
#include "convolve.h"
//#include "vpx_dsp/x86/convolve_avx2.h"
#include "mem.h"

void vpx_convolve_copy_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst,
    ptrdiff_t dst_stride, const InterpKernel *filter,
    int x0_q4, int x_step_q4, int y0_q4, int y_step_q4,
    int w, int h) {
    (void)filter;
    (void)x0_q4;
    (void)x_step_q4;
    (void)y0_q4;
    (void)y_step_q4;

    if (w == 4) {
        do {
            *(int32_t *)(dst + 0 * dst_stride) = *(int32_t *)(src + 0 * src_stride);
            *(int32_t *)(dst + 1 * dst_stride) = *(int32_t *)(src + 1 * src_stride);
            *(int32_t *)(dst + 2 * dst_stride) = *(int32_t *)(src + 2 * src_stride);
            *(int32_t *)(dst + 3 * dst_stride) = *(int32_t *)(src + 3 * src_stride);
            src += 4 * src_stride;
            dst += 4 * dst_stride;
            h -= 4;
        } while (h);
    } else if (w == 8) {
        do {
            _mm_storel_epi64((__m128i *)(dst + 0 * dst_stride), _mm_loadl_epi64((__m128i *)(src + 0 * src_stride)));
            _mm_storel_epi64((__m128i *)(dst + 1 * dst_stride), _mm_loadl_epi64((__m128i *)(src + 1 * src_stride)));
            _mm_storel_epi64((__m128i *)(dst + 2 * dst_stride), _mm_loadl_epi64((__m128i *)(src + 2 * src_stride)));
            _mm_storel_epi64((__m128i *)(dst + 3 * dst_stride), _mm_loadl_epi64((__m128i *)(src + 3 * src_stride)));
            src += 4 * src_stride;
            dst += 4 * dst_stride;
            h -= 4;
        } while (h);
    } else if (w == 16) {
        do {
            _mm_store_si128((__m128i *)(dst + 0 * dst_stride), _mm_loadu_si128((__m128i *)(src + 0 * src_stride)));
            _mm_store_si128((__m128i *)(dst + 1 * dst_stride), _mm_loadu_si128((__m128i *)(src + 1 * src_stride)));
            _mm_store_si128((__m128i *)(dst + 2 * dst_stride), _mm_loadu_si128((__m128i *)(src + 2 * src_stride)));
            _mm_store_si128((__m128i *)(dst + 3 * dst_stride), _mm_loadu_si128((__m128i *)(src + 3 * src_stride)));
            src += 4 * src_stride;
            dst += 4 * dst_stride;
            h -= 4;
        } while (h);
    } else if (w == 32) {
        do {
            _mm256_store_si256((__m256i *)(dst + 0 * dst_stride), _mm256_loadu_si256((__m256i *)(src + 0 * src_stride)));
            _mm256_store_si256((__m256i *)(dst + 1 * dst_stride), _mm256_loadu_si256((__m256i *)(src + 1 * src_stride)));
            _mm256_store_si256((__m256i *)(dst + 2 * dst_stride), _mm256_loadu_si256((__m256i *)(src + 2 * src_stride)));
            _mm256_store_si256((__m256i *)(dst + 3 * dst_stride), _mm256_loadu_si256((__m256i *)(src + 3 * src_stride)));
            src += 4 * src_stride;
            dst += 4 * dst_stride;
            h -= 4;
        } while (h);
    } else {
        do {
            _mm256_store_si256((__m256i *)(dst + 0 * dst_stride + 0x00), _mm256_loadu_si256((__m256i *)(src + 0 * src_stride + 0x00)));
            _mm256_store_si256((__m256i *)(dst + 0 * dst_stride + 0x20), _mm256_loadu_si256((__m256i *)(src + 0 * src_stride + 0x20)));
            _mm256_store_si256((__m256i *)(dst + 1 * dst_stride + 0x00), _mm256_loadu_si256((__m256i *)(src + 1 * src_stride + 0x00)));
            _mm256_store_si256((__m256i *)(dst + 1 * dst_stride + 0x20), _mm256_loadu_si256((__m256i *)(src + 1 * src_stride + 0x20)));
            src += 2 * src_stride;
            dst += 2 * dst_stride;
            h -= 2;
        } while (h);
    }
}

static INLINE void convolve_avg4(const uint8_t *const src, uint8_t *const dst)
{
    const __m128i s = _mm_cvtsi32_si128(*(const int32_t *)src);
    const __m128i d = _mm_cvtsi32_si128(*(const int32_t *)dst);
    const __m128i a = _mm_avg_epu8(s, d);
    *(int32_t *)dst = _mm_cvtsi128_si32(a);
}

static INLINE void convolve_avg8(const uint8_t *const src, uint8_t *const dst)
{
    const __m128i s = _mm_loadl_epi64((const __m128i *)src);
    const __m128i d = _mm_loadl_epi64((const __m128i *)dst);
    const __m128i a = _mm_avg_epu8(s, d);
    _mm_storel_epi64((__m128i *)dst, a);
}

static INLINE void convolve_avg16(const uint8_t *const src, uint8_t *const dst)
{
    const __m128i s = _mm_loadu_si128((const __m128i *)src);
    const __m128i d = _mm_load_si128((const __m128i *)dst);
    const __m128i a = _mm_avg_epu8(s, d);
    _mm_store_si128((__m128i *)dst, a);
}

static INLINE void convolve_avg32(const uint8_t *const src, uint8_t *const dst)
{
    const __m256i s = _mm256_loadu_si256((const __m256i *)src);
    const __m256i d = _mm256_load_si256((const __m256i *)dst);
    const __m256i a = _mm256_avg_epu8(s, d);
    _mm256_store_si256((__m256i *)dst, a);
}

void vpx_convolve_avg_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst,
    ptrdiff_t dst_stride, const InterpKernel *filter,
    int x0_q4, int x_step_q4, int y0_q4, int y_step_q4,
    int w, int h) {
    (void)filter;
    (void)x0_q4;
    (void)x_step_q4;
    (void)y0_q4;
    (void)y_step_q4;

    if (w == 4) {
        do {
            convolve_avg4(src + 0 * src_stride, dst + 0 * dst_stride);
            convolve_avg4(src + 1 * src_stride, dst + 1 * dst_stride);
            convolve_avg4(src + 2 * src_stride, dst + 2 * dst_stride);
            convolve_avg4(src + 3 * src_stride, dst + 3 * dst_stride);
            src += 4 * src_stride;
            dst += 4 * dst_stride;
            h -= 4;
        } while (h);
    } else if (w == 8) {
        do {
            convolve_avg8(src + 0 * src_stride, dst + 0 * dst_stride);
            convolve_avg8(src + 1 * src_stride, dst + 1 * dst_stride);
            convolve_avg8(src + 2 * src_stride, dst + 2 * dst_stride);
            convolve_avg8(src + 3 * src_stride, dst + 3 * dst_stride);
            src += 4 * src_stride;
            dst += 4 * dst_stride;
            h -= 4;
        } while (h);
    } else if (w == 16) {
        do {
            convolve_avg16(src + 0 * src_stride, dst + 0 * dst_stride);
            convolve_avg16(src + 1 * src_stride, dst + 1 * dst_stride);
            convolve_avg16(src + 2 * src_stride, dst + 2 * dst_stride);
            convolve_avg16(src + 3 * src_stride, dst + 3 * dst_stride);
            src += 4 * src_stride;
            dst += 4 * dst_stride;
            h -= 4;
        } while (h);
    } else if (w == 32) {
        do {
            convolve_avg32(src + 0 * src_stride, dst + 0 * dst_stride);
            convolve_avg32(src + 1 * src_stride, dst + 1 * dst_stride);
            convolve_avg32(src + 2 * src_stride, dst + 2 * dst_stride);
            convolve_avg32(src + 3 * src_stride, dst + 3 * dst_stride);
            src += 4 * src_stride;
            dst += 4 * dst_stride;
            h -= 4;
        } while (h);
    } else if (w == 64) {
        do {
            convolve_avg32(src + 0 * src_stride + 0x00, dst + 0 * dst_stride + 0x00);
            convolve_avg32(src + 0 * src_stride + 0x20, dst + 0 * dst_stride + 0x20);
            convolve_avg32(src + 1 * src_stride + 0x00, dst + 1 * dst_stride + 0x00);
            convolve_avg32(src + 1 * src_stride + 0x20, dst + 1 * dst_stride + 0x20);
            src += 2 * src_stride;
            dst += 2 * dst_stride;
            h -= 2;
        } while (h);
    } else {
        assert(w == 128);
        do {
            convolve_avg32(src + 0x00, dst + 0x00);
            convolve_avg32(src + 0x20, dst + 0x20);
            convolve_avg32(src + 0x40, dst + 0x40);
            convolve_avg32(src + 0x60, dst + 0x60);
            src += src_stride;
            dst += dst_stride;
        } while (--h);
    }
}

static INLINE void shuffle_filter_ssse3(const int16_t *const filter,
    __m128i *const f) {
    const __m128i f_values = _mm_load_si128((const __m128i *)filter);
    // pack and duplicate the filter values
    f[0] = _mm_shuffle_epi8(f_values, _mm_set1_epi16(0x0200u));
    f[1] = _mm_shuffle_epi8(f_values, _mm_set1_epi16(0x0604u));
    f[2] = _mm_shuffle_epi8(f_values, _mm_set1_epi16(0x0a08u));
    f[3] = _mm_shuffle_epi8(f_values, _mm_set1_epi16(0x0e0cu));
}

static INLINE __m128i convolve8_8_ssse3(const __m128i *const s,
    const __m128i *const f) {
    // multiply 2 adjacent elements with the filter and add the result
    const __m128i k_64 = _mm_set1_epi16(1 << 6);
    const __m128i x0 = _mm_maddubs_epi16(s[0], f[0]);
    const __m128i x1 = _mm_maddubs_epi16(s[1], f[1]);
    const __m128i x2 = _mm_maddubs_epi16(s[2], f[2]);
    const __m128i x3 = _mm_maddubs_epi16(s[3], f[3]);
    __m128i sum1, sum2;

    // sum the results together, saturating only on the final step
    // adding x0 with x2 and x1 with x3 is the only order that prevents
    // outranges for all filters
    sum1 = _mm_add_epi16(x0, x2);
    sum2 = _mm_add_epi16(x1, x3);
    // add the rounding offset early to avoid another saturated add
    sum1 = _mm_add_epi16(sum1, k_64);
    sum1 = _mm_adds_epi16(sum1, sum2);
    // shift by 7 bit each 16 bit
    sum1 = _mm_srai_epi16(sum1, 7);
    return sum1;
}

#if defined(__clang__)
#if (__clang_major__ > 0 && __clang_major__ < 3) ||            \
    (__clang_major__ == 3 && __clang_minor__ <= 3) ||          \
    (defined(__APPLE__) && defined(__apple_build_version__) && \
     ((__clang_major__ == 4 && __clang_minor__ <= 2) ||        \
      (__clang_major__ == 5 && __clang_minor__ == 0)))
#define MM256_BROADCASTSI128_SI256(x) \
  _mm_broadcastsi128_si256((__m128i const *)&(x))
#else  // clang > 3.3, and not 5.0 on macosx.
#define MM256_BROADCASTSI128_SI256(x) _mm256_broadcastsi128_si256(x)
#endif  // clang <= 3.3
#elif defined(__GNUC__)
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ <= 6)
#define MM256_BROADCASTSI128_SI256(x) \
  _mm_broadcastsi128_si256((__m128i const *)&(x))
#elif __GNUC__ == 4 && __GNUC_MINOR__ == 7
#define MM256_BROADCASTSI128_SI256(x) _mm_broadcastsi128_si256(x)
#else  // gcc > 4.7
#define MM256_BROADCASTSI128_SI256(x) _mm256_broadcastsi128_si256(x)
#endif  // gcc <= 4.6
#else   // !(gcc || clang)
#define MM256_BROADCASTSI128_SI256(x) _mm256_broadcastsi128_si256(x)
#endif  // __clang__

static INLINE void shuffle_filter_avx2(const int16_t *const filter,
    __m256i *const f) {
    const __m256i f_values =
        MM256_BROADCASTSI128_SI256(_mm_load_si128((const __m128i *)filter));
    // pack and duplicate the filter values
    f[0] = _mm256_shuffle_epi8(f_values, _mm256_set1_epi16(0x0200u));
    f[1] = _mm256_shuffle_epi8(f_values, _mm256_set1_epi16(0x0604u));
    f[2] = _mm256_shuffle_epi8(f_values, _mm256_set1_epi16(0x0a08u));
    f[3] = _mm256_shuffle_epi8(f_values, _mm256_set1_epi16(0x0e0cu));
}

static INLINE __m256i convolve8_16_avx2(const __m256i *const s,
    const __m256i *const f) {
    // multiply 2 adjacent elements with the filter and add the result
    const __m256i k_64 = _mm256_set1_epi16(1 << 6);
    const __m256i x0 = _mm256_maddubs_epi16(s[0], f[0]);
    const __m256i x1 = _mm256_maddubs_epi16(s[1], f[1]);
    const __m256i x2 = _mm256_maddubs_epi16(s[2], f[2]);
    const __m256i x3 = _mm256_maddubs_epi16(s[3], f[3]);
    __m256i sum1, sum2;

    // sum the results together, saturating only on the final step
    // adding x0 with x2 and x1 with x3 is the only order that prevents
    // outranges for all filters
    sum1 = _mm256_add_epi16(x0, x2);
    sum2 = _mm256_add_epi16(x1, x3);
    // add the rounding offset early to avoid another saturated add
    sum1 = _mm256_add_epi16(sum1, k_64);
    sum1 = _mm256_adds_epi16(sum1, sum2);
    // round and shift by 7 bit each 16 bit
    sum1 = _mm256_srai_epi16(sum1, 7);
    return sum1;
}

static INLINE __m128i convolve8_8_avx2(const __m256i *const s,
    const __m256i *const f) {
    // multiply 2 adjacent elements with the filter and add the result
    const __m128i k_64 = _mm_set1_epi16(1 << 6);
    const __m128i x0 = _mm_maddubs_epi16(_mm256_castsi256_si128(s[0]),
        _mm256_castsi256_si128(f[0]));
    const __m128i x1 = _mm_maddubs_epi16(_mm256_castsi256_si128(s[1]),
        _mm256_castsi256_si128(f[1]));
    const __m128i x2 = _mm_maddubs_epi16(_mm256_castsi256_si128(s[2]),
        _mm256_castsi256_si128(f[2]));
    const __m128i x3 = _mm_maddubs_epi16(_mm256_castsi256_si128(s[3]),
        _mm256_castsi256_si128(f[3]));
    __m128i sum1, sum2;

    // sum the results together, saturating only on the final step
    // adding x0 with x2 and x1 with x3 is the only order that prevents
    // outranges for all filters
    sum1 = _mm_add_epi16(x0, x2);
    sum2 = _mm_add_epi16(x1, x3);
    // add the rounding offset early to avoid another saturated add
    sum1 = _mm_add_epi16(sum1, k_64);
    sum1 = _mm_adds_epi16(sum1, sum2);
    // shift by 7 bit each 16 bit
    sum1 = _mm_srai_epi16(sum1, 7);
    return sum1;
}

// filters for 16_h8
DECLARE_ALIGNED(32, static const uint8_t, filt1_global_avx2[32]) = {
  0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8,
  0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8
};

DECLARE_ALIGNED(32, static const uint8_t, filt2_global_avx2[32]) = {
  2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
  2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10
};

DECLARE_ALIGNED(32, static const uint8_t, filt3_global_avx2[32]) = {
  4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12,
  4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12
};

DECLARE_ALIGNED(32, static const uint8_t, filt4_global_avx2[32]) = {
  6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14,
  6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14
};

static INLINE void vpx_filter_block1d4_h8_x_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_pixels_per_line, uint8_t *output_ptr,
    ptrdiff_t output_pitch, uint32_t output_height, const int16_t *filter,
    const int avg) {
    __m128i outReg1, outReg2;
    __m256i outReg32b1;
    unsigned int i;
    ptrdiff_t src_stride, dst_stride;
    __m256i f[4], filt[4], s[4];

    shuffle_filter_avx2(filter, f);
    filt[0] = _mm256_load_si256((__m256i const *)filt1_global_avx2);
    filt[1] = _mm256_load_si256((__m256i const *)filt2_global_avx2);
    filt[2] = _mm256_load_si256((__m256i const *)filt3_global_avx2);
    filt[3] = _mm256_load_si256((__m256i const *)filt4_global_avx2);

    // multiple the size of the source and destination stride by two
    src_stride = src_pixels_per_line << 1;
    dst_stride = output_pitch << 1;
    for (i = output_height; i > 1; i -= 2) {
        __m256i srcReg;

        // load the 2 strides of source
        srcReg =
            _mm256_castsi128_si256(_mm_loadu_si128((const __m128i *)(src_ptr - 3)));
        srcReg = _mm256_inserti128_si256(
            srcReg,
            _mm_loadu_si128((const __m128i *)(src_ptr + src_pixels_per_line - 3)),
            1);

        // filter the source buffer
        s[0] = _mm256_shuffle_epi8(srcReg, filt[0]);
        s[1] = _mm256_shuffle_epi8(srcReg, filt[1]);
        s[2] = _mm256_shuffle_epi8(srcReg, filt[2]);
        s[3] = _mm256_shuffle_epi8(srcReg, filt[3]);
        outReg32b1 = convolve8_16_avx2(s, f);

        // shrink to 8 bit each 16 bits
        outReg32b1 = _mm256_permute4x64_epi64(outReg32b1, 0xD8);
        outReg1 = _mm256_castsi256_si128(outReg32b1);
        outReg1 = _mm_packus_epi16(outReg1, outReg1);

        src_ptr += src_stride;

        // average if necessary
        if (avg) {
            outReg2 = _mm_cvtsi32_si128(*(const int32_t *)output_ptr);
            outReg2 = _mm_insert_epi32(outReg2, *(const int32_t *)(output_ptr + output_pitch), 1);
            outReg1 = _mm_avg_epu8(outReg1, outReg2);
        }

        // save 4 bytes
        *(int32_t *)output_ptr = _mm_cvtsi128_si32(outReg1);

        // save the next 4 bytes
        *(int32_t *)(output_ptr + output_pitch) = _mm_extract_epi32(outReg1, 1);

        output_ptr += dst_stride;
    }

    // if the number of strides is odd.
    // process only 8 bytes
    if (i > 0) {
        __m128i srcReg;

        // load the first 16 bytes of the last row
        srcReg = _mm_loadu_si128((const __m128i *)(src_ptr - 3));

        // filter the source buffer
        s[0] = _mm256_castsi128_si256(
            _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[0])));
        s[1] = _mm256_castsi128_si256(
            _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[1])));
        s[2] = _mm256_castsi128_si256(
            _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[2])));
        s[3] = _mm256_castsi128_si256(
            _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[3])));
        outReg1 = convolve8_8_avx2(s, f);

        // shrink to 8 bit each 16 bits
        outReg1 = _mm_packus_epi16(outReg1, outReg1);

        // average if necessary
        if (avg) {
            outReg1 = _mm_avg_epu8(outReg1, _mm_cvtsi32_si128(*(const int32_t *)output_ptr));
        }

        // save 4 bytes
        *(int32_t *)output_ptr = _mm_cvtsi128_si32(outReg1);
    }
}

static INLINE void vpx_filter_block1d8_h8_x_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_pixels_per_line, uint8_t *output_ptr,
    ptrdiff_t output_pitch, uint32_t output_height, const int16_t *filter,
    const int avg) {
    __m128i outReg1, outReg2;
    __m256i outReg32b1;
    unsigned int i;
    ptrdiff_t src_stride, dst_stride;
    __m256i f[4], filt[4], s[4];

    shuffle_filter_avx2(filter, f);
    filt[0] = _mm256_load_si256((__m256i const *)filt1_global_avx2);
    filt[1] = _mm256_load_si256((__m256i const *)filt2_global_avx2);
    filt[2] = _mm256_load_si256((__m256i const *)filt3_global_avx2);
    filt[3] = _mm256_load_si256((__m256i const *)filt4_global_avx2);

    // multiple the size of the source and destination stride by two
    src_stride = src_pixels_per_line << 1;
    dst_stride = output_pitch << 1;
    for (i = output_height; i > 1; i -= 2) {
        __m256i srcReg;

        // load the 2 strides of source
        srcReg =
            _mm256_castsi128_si256(_mm_loadu_si128((const __m128i *)(src_ptr - 3)));
        srcReg = _mm256_inserti128_si256(
            srcReg,
            _mm_loadu_si128((const __m128i *)(src_ptr + src_pixels_per_line - 3)),
            1);

        // filter the source buffer
        s[0] = _mm256_shuffle_epi8(srcReg, filt[0]);
        s[1] = _mm256_shuffle_epi8(srcReg, filt[1]);
        s[2] = _mm256_shuffle_epi8(srcReg, filt[2]);
        s[3] = _mm256_shuffle_epi8(srcReg, filt[3]);
        outReg32b1 = convolve8_16_avx2(s, f);

        // shrink to 8 bit each 16 bits
        outReg1 = _mm256_castsi256_si128(outReg32b1);
        outReg2 = _mm256_extractf128_si256(outReg32b1, 1);
        outReg1 = _mm_packus_epi16(outReg1, outReg2);

        src_ptr += src_stride;

        // average if necessary
        if (avg) {
            outReg2 = _mm_loadl_epi64((__m128i *)output_ptr);
            outReg2 = loadh_epi64(outReg2, (__m128i *)(output_ptr + output_pitch));
            outReg1 = _mm_avg_epu8(outReg1, outReg2);
        }

        // save 8 bytes
        _mm_storel_epi64((__m128i *)output_ptr, outReg1);

        // save the next 8 bytes
        _mm_storeh_epi64((__m128i *)(output_ptr + output_pitch), outReg1);

        output_ptr += dst_stride;
    }

    // if the number of strides is odd.
    // process only 8 bytes
    if (i > 0) {
        __m128i srcReg;

        // load the first 16 bytes of the last row
        srcReg = _mm_loadu_si128((const __m128i *)(src_ptr - 3));

        // filter the source buffer
        s[0] = _mm256_castsi128_si256(
            _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[0])));
        s[1] = _mm256_castsi128_si256(
            _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[1])));
        s[2] = _mm256_castsi128_si256(
            _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[2])));
        s[3] = _mm256_castsi128_si256(
            _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[3])));
        outReg1 = convolve8_8_avx2(s, f);

        // shrink to 8 bit each 16 bits
        outReg1 = _mm_packus_epi16(outReg1, outReg1);

        // average if necessary
        if (avg) {
            outReg1 = _mm_avg_epu8(outReg1, _mm_loadl_epi64((__m128i *)output_ptr));
        }

        // save 8 bytes
        _mm_storel_epi64((__m128i *)output_ptr, outReg1);
    }
}

static INLINE void vpx_filter_block1d16_h8_x_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_pixels_per_line, uint8_t *output_ptr,
    ptrdiff_t output_pitch, uint32_t output_height, const int16_t *filter,
    const int avg) {
  __m128i outReg1, outReg2;
  __m256i outReg32b1, outReg32b2;
  unsigned int i;
  ptrdiff_t src_stride, dst_stride;
  __m256i f[4], filt[4], s[4];

  shuffle_filter_avx2(filter, f);
  filt[0] = _mm256_load_si256((__m256i const *)filt1_global_avx2);
  filt[1] = _mm256_load_si256((__m256i const *)filt2_global_avx2);
  filt[2] = _mm256_load_si256((__m256i const *)filt3_global_avx2);
  filt[3] = _mm256_load_si256((__m256i const *)filt4_global_avx2);

  // multiple the size of the source and destination stride by two
  src_stride = src_pixels_per_line << 1;
  dst_stride = output_pitch << 1;
  for (i = output_height; i > 1; i -= 2) {
    __m256i srcReg;

    // load the 2 strides of source
    srcReg =
        _mm256_castsi128_si256(_mm_loadu_si128((const __m128i *)(src_ptr - 3)));
    srcReg = _mm256_inserti128_si256(
        srcReg,
        _mm_loadu_si128((const __m128i *)(src_ptr + src_pixels_per_line - 3)),
        1);

    // filter the source buffer
    s[0] = _mm256_shuffle_epi8(srcReg, filt[0]);
    s[1] = _mm256_shuffle_epi8(srcReg, filt[1]);
    s[2] = _mm256_shuffle_epi8(srcReg, filt[2]);
    s[3] = _mm256_shuffle_epi8(srcReg, filt[3]);
    outReg32b1 = convolve8_16_avx2(s, f);

    // reading 2 strides of the next 16 bytes
    // (part of it was being read by earlier read)
    srcReg =
        _mm256_castsi128_si256(_mm_loadu_si128((const __m128i *)(src_ptr + 5)));
    srcReg = _mm256_inserti128_si256(
        srcReg,
        _mm_loadu_si128((const __m128i *)(src_ptr + src_pixels_per_line + 5)),
        1);

    // filter the source buffer
    s[0] = _mm256_shuffle_epi8(srcReg, filt[0]);
    s[1] = _mm256_shuffle_epi8(srcReg, filt[1]);
    s[2] = _mm256_shuffle_epi8(srcReg, filt[2]);
    s[3] = _mm256_shuffle_epi8(srcReg, filt[3]);
    outReg32b2 = convolve8_16_avx2(s, f);

    // shrink to 8 bit each 16 bits, the low and high 64-bits of each lane
    // contain the first and second convolve result respectively
    outReg32b1 = _mm256_packus_epi16(outReg32b1, outReg32b2);

    src_ptr += src_stride;

    // average if necessary
    outReg1 = _mm256_castsi256_si128(outReg32b1);
    outReg2 = _mm256_extractf128_si256(outReg32b1, 1);
    if (avg) {
      outReg1 = _mm_avg_epu8(outReg1, _mm_load_si128((__m128i *)output_ptr));
      outReg2 = _mm_avg_epu8(
          outReg2, _mm_load_si128((__m128i *)(output_ptr + output_pitch)));
    }

    // save 16 bytes
    _mm_store_si128((__m128i *)output_ptr, outReg1);

    // save the next 16 bits
    _mm_store_si128((__m128i *)(output_ptr + output_pitch), outReg2);

    output_ptr += dst_stride;
  }

  // if the number of strides is odd.
  // process only 16 bytes
  if (i > 0) {
    __m128i srcReg;

    // load the first 16 bytes of the last row
    srcReg = _mm_loadu_si128((const __m128i *)(src_ptr - 3));

    // filter the source buffer
    s[0] = _mm256_castsi128_si256(
        _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[0])));
    s[1] = _mm256_castsi128_si256(
        _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[1])));
    s[2] = _mm256_castsi128_si256(
        _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[2])));
    s[3] = _mm256_castsi128_si256(
        _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[3])));
    outReg1 = convolve8_8_avx2(s, f);

    // reading the next 16 bytes
    // (part of it was being read by earlier read)
    srcReg = _mm_loadu_si128((const __m128i *)(src_ptr + 5));

    // filter the source buffer
    s[0] = _mm256_castsi128_si256(
        _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[0])));
    s[1] = _mm256_castsi128_si256(
        _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[1])));
    s[2] = _mm256_castsi128_si256(
        _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[2])));
    s[3] = _mm256_castsi128_si256(
        _mm_shuffle_epi8(srcReg, _mm256_castsi256_si128(filt[3])));
    outReg2 = convolve8_8_avx2(s, f);

    // shrink to 8 bit each 16 bits, the low and high 64-bits of each lane
    // contain the first and second convolve result respectively
    outReg1 = _mm_packus_epi16(outReg1, outReg2);

    // average if necessary
    if (avg) {
      outReg1 = _mm_avg_epu8(outReg1, _mm_load_si128((__m128i *)output_ptr));
    }

    // save 16 bytes
    _mm_store_si128((__m128i *)output_ptr, outReg1);
  }
}

static void vpx_filter_block1d4_h8_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_stride, uint8_t *output_ptr,
    ptrdiff_t dst_stride, uint32_t output_height, const int16_t *filter) {
    vpx_filter_block1d4_h8_x_avx2(src_ptr, src_stride, output_ptr, dst_stride,
        output_height, filter, 0);
}

static void vpx_filter_block1d4_h8_avg_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_stride, uint8_t *output_ptr,
    ptrdiff_t dst_stride, uint32_t output_height, const int16_t *filter) {
    vpx_filter_block1d4_h8_x_avx2(src_ptr, src_stride, output_ptr, dst_stride,
        output_height, filter, 1);
}

static void vpx_filter_block1d8_h8_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_stride, uint8_t *output_ptr,
    ptrdiff_t dst_stride, uint32_t output_height, const int16_t *filter) {
    vpx_filter_block1d8_h8_x_avx2(src_ptr, src_stride, output_ptr, dst_stride,
        output_height, filter, 0);
}

static void vpx_filter_block1d8_h8_avg_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_stride, uint8_t *output_ptr,
    ptrdiff_t dst_stride, uint32_t output_height, const int16_t *filter) {
    vpx_filter_block1d8_h8_x_avx2(src_ptr, src_stride, output_ptr, dst_stride,
        output_height, filter, 1);
}

static void vpx_filter_block1d16_h8_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_stride, uint8_t *output_ptr,
    ptrdiff_t dst_stride, uint32_t output_height, const int16_t *filter) {
  vpx_filter_block1d16_h8_x_avx2(src_ptr, src_stride, output_ptr, dst_stride,
                                 output_height, filter, 0);
}

static void vpx_filter_block1d16_h8_avg_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_stride, uint8_t *output_ptr,
    ptrdiff_t dst_stride, uint32_t output_height, const int16_t *filter) {
  vpx_filter_block1d16_h8_x_avx2(src_ptr, src_stride, output_ptr, dst_stride,
                                 output_height, filter, 1);
}

static INLINE void vpx_filter_block1d4_v8_x_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
    ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter,
    const int avg) {
    __m128i outReg1, outReg2;
    unsigned int i;
    __m128i s[10], s45, s56;
    __m256i f[4], s1[4];

    shuffle_filter_avx2(filter, f);

    {
        __m256i s32b[4];
        __m128i s01, s12, s23, s34;

        // load 4 bytes 7 times in stride of src_pitch
        s[0] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 0 * src_pitch));
        s[1] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 1 * src_pitch));
        s[2] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 2 * src_pitch));
        s[3] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 3 * src_pitch));
        s[4] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 4 * src_pitch));
        s[5] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 5 * src_pitch));
        s[6] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 6 * src_pitch));

        // have each consecutive loads on the same 256 register
        s01 = _mm_unpacklo_epi32(s[0], s[1]);
        s12 = _mm_unpacklo_epi32(s[1], s[2]);
        s23 = _mm_unpacklo_epi32(s[2], s[3]);
        s34 = _mm_unpacklo_epi32(s[3], s[4]);
        s45 = _mm_unpacklo_epi32(s[4], s[5]);
        s56 = _mm_unpacklo_epi32(s[5], s[6]);
        s32b[0] = _mm256_inserti128_si256(_mm256_castsi128_si256(s01), s23, 1);
        s32b[1] = _mm256_inserti128_si256(_mm256_castsi128_si256(s12), s34, 1);
        s32b[2] = _mm256_inserti128_si256(_mm256_castsi128_si256(s23), s45, 1);
        s32b[3] = _mm256_inserti128_si256(_mm256_castsi128_si256(s34), s56, 1);

        // merge every four consecutive registers
        s1[0] = _mm256_unpacklo_epi8(s32b[0], s32b[1]);
        s1[1] = _mm256_unpacklo_epi8(s32b[2], s32b[3]);
    }

    for (i = output_height; i > 3; i -= 4) {
        __m128i s67, s78;
        __m256i s32b[4];

        // load the next 4 loads of 4 bytes and have every four
        // consecutive loads in the same 256 bit register
        s[7] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 7 * src_pitch));
        s[8] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 8 * src_pitch));
        s67 = _mm_unpacklo_epi32(s[6], s[7]);
        s78 = _mm_unpacklo_epi32(s[7], s[8]);
        s[9] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 9 * src_pitch));
        s[6] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 10 * src_pitch));
        s32b[0] = _mm256_inserti128_si256(_mm256_castsi128_si256(s45), s67, 1);
        s32b[1] = _mm256_inserti128_si256(_mm256_castsi128_si256(s56), s78, 1);
        s45 = _mm_unpacklo_epi32(s[8], s[9]);
        s56 = _mm_unpacklo_epi32(s[9], s[6]);
        s32b[2] = _mm256_inserti128_si256(_mm256_castsi128_si256(s67), s45, 1);
        s32b[3] = _mm256_inserti128_si256(_mm256_castsi128_si256(s78), s56, 1);

        // merge every four consecutive registers
        s1[2] = _mm256_unpacklo_epi8(s32b[0], s32b[1]);
        s1[3] = _mm256_unpacklo_epi8(s32b[2], s32b[3]);

        s1[0] = convolve8_16_avx2(s1, f);

        // shrink to 8 bit each 16 bits
        outReg1 = _mm256_castsi256_si128(s1[0]);
        outReg2 = _mm256_extractf128_si256(s1[0], 1);
        outReg1 = _mm_packus_epi16(outReg1, outReg2);

        src_ptr += 4 * src_pitch;

        // average if necessary
        if (avg) {
            outReg2 = _mm_cvtsi32_si128(*(const int32_t *)(output_ptr + 0 * out_pitch));
            outReg2 = _mm_insert_epi32(outReg2, *(const int32_t *)(output_ptr + 1 * out_pitch), 1);
            outReg2 = _mm_insert_epi32(outReg2, *(const int32_t *)(output_ptr + 2 * out_pitch), 2);
            outReg2 = _mm_insert_epi32(outReg2, *(const int32_t *)(output_ptr + 3 * out_pitch), 3);
            outReg1 = _mm_avg_epu8(outReg1, outReg2);
        }

        // save 4x4 bytes
        *(int32_t *)(output_ptr + 0 * out_pitch) = _mm_cvtsi128_si32(outReg1);
        *(int32_t *)(output_ptr + 1 * out_pitch) = _mm_extract_epi32(outReg1, 1);
        *(int32_t *)(output_ptr + 2 * out_pitch) = _mm_extract_epi32(outReg1, 2);
        *(int32_t *)(output_ptr + 3 * out_pitch) = _mm_extract_epi32(outReg1, 3);

        output_ptr += 4 * out_pitch;

        // shift down by two rows
        s1[0] = s1[2];
        s1[1] = s1[3];
    }

    // process the last 1, 2 or 3 rows
    if (i) {
        __m128i s67, s78;
        __m256i s32b[4];

        // load the next 4 loads of 4 bytes and have every four
        // consecutive loads in the same 256 bit register
        s[7] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 7 * src_pitch));
        if (i > 1) {
            s[8] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 8 * src_pitch));
            if (i > 2) {
                s[9] = _mm_cvtsi32_si128(*(const int32_t *)(src_ptr + 9 * src_pitch));
            }
        }
        s67 = _mm_unpacklo_epi32(s[6], s[7]);
        s78 = _mm_unpacklo_epi32(s[7], s[8]);
        s32b[0] = _mm256_inserti128_si256(_mm256_castsi128_si256(s45), s67, 1);
        s32b[1] = _mm256_inserti128_si256(_mm256_castsi128_si256(s56), s78, 1);
        s45 = _mm_unpacklo_epi32(s[8], s[9]);
        s56 = _mm_unpacklo_epi32(s[9], s[6]);
        s32b[2] = _mm256_inserti128_si256(_mm256_castsi128_si256(s67), s45, 1);
        s32b[3] = _mm256_inserti128_si256(_mm256_castsi128_si256(s78), s56, 1);

        // merge every four consecutive registers
        s1[2] = _mm256_unpacklo_epi8(s32b[0], s32b[1]);
        s1[3] = _mm256_unpacklo_epi8(s32b[2], s32b[3]);

        s1[0] = convolve8_16_avx2(s1, f);

        // shrink to 8 bit each 16 bits
        outReg1 = _mm256_castsi256_si128(s1[0]);
        outReg2 = _mm256_extractf128_si256(s1[0], 1);
        outReg1 = _mm_packus_epi16(outReg1, outReg2);

        // average if necessary
        if (avg) {
            outReg2 = _mm_cvtsi32_si128(*(const int32_t *)(output_ptr + 0 * out_pitch));
            if (i > 1) {
                outReg2 = _mm_insert_epi32(outReg2, *(const int32_t *)(output_ptr + 1 * out_pitch), 1);
                if (i > 2) {
                    outReg2 = _mm_insert_epi32(outReg2, *(const int32_t *)(output_ptr + 2 * out_pitch), 2);
                }
            }
            outReg1 = _mm_avg_epu8(outReg1, outReg2);
        }

        // save 4 bytes
        *(int32_t *)(output_ptr + 0 * out_pitch) = _mm_cvtsi128_si32(outReg1);
        if (i > 1) {
            *(int32_t *)(output_ptr + 1 * out_pitch) = _mm_extract_epi32(outReg1, 1);
            if (i > 2) {
                *(int32_t *)(output_ptr + 2 * out_pitch) = _mm_extract_epi32(outReg1, 2);
            }
        }
    }
}

static INLINE void vpx_filter_block1d8_v8_x_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
    ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter,
    const int avg) {
    __m128i outReg1, outReg2;
    __m256i srcRegHead1;
    unsigned int i;
    ptrdiff_t src_stride, dst_stride;
    __m256i f[4], s1[4];

    shuffle_filter_avx2(filter, f);

    // multiple the size of the source and destination stride by two
    src_stride = src_pitch << 1;
    dst_stride = out_pitch << 1;

    {
        __m128i s[6];
        __m256i s32b[6];

        // load 8 bytes 7 times in stride of src_pitch
        s[0] = _mm_loadl_epi64((const __m128i *)(src_ptr + 0 * src_pitch));
        s[1] = _mm_loadl_epi64((const __m128i *)(src_ptr + 1 * src_pitch));
        s[2] = _mm_loadl_epi64((const __m128i *)(src_ptr + 2 * src_pitch));
        s[3] = _mm_loadl_epi64((const __m128i *)(src_ptr + 3 * src_pitch));
        s[4] = _mm_loadl_epi64((const __m128i *)(src_ptr + 4 * src_pitch));
        s[5] = _mm_loadl_epi64((const __m128i *)(src_ptr + 5 * src_pitch));
        srcRegHead1 = _mm256_castsi128_si256(
            _mm_loadl_epi64((const __m128i *)(src_ptr + 6 * src_pitch)));

        // have each consecutive loads on the same 256 register
        s32b[0] = _mm256_inserti128_si256(_mm256_castsi128_si256(s[0]), s[1], 1);
        s32b[1] = _mm256_inserti128_si256(_mm256_castsi128_si256(s[1]), s[2], 1);
        s32b[2] = _mm256_inserti128_si256(_mm256_castsi128_si256(s[2]), s[3], 1);
        s32b[3] = _mm256_inserti128_si256(_mm256_castsi128_si256(s[3]), s[4], 1);
        s32b[4] = _mm256_inserti128_si256(_mm256_castsi128_si256(s[4]), s[5], 1);
        s32b[5] = _mm256_inserti128_si256(_mm256_castsi128_si256(s[5]),
            _mm256_castsi256_si128(srcRegHead1), 1);

        // merge every two consecutive registers except the last one
        // the first lanes contain values for filtering odd rows (1,3,5...) and
        // the second lanes contain values for filtering even rows (2,4,6...)
        s1[0] = _mm256_unpacklo_epi8(s32b[0], s32b[1]);
        s1[1] = _mm256_unpacklo_epi8(s32b[2], s32b[3]);
        s1[2] = _mm256_unpacklo_epi8(s32b[4], s32b[5]);
    }

    for (i = output_height; i > 1; i -= 2) {
        __m256i srcRegHead2, srcRegHead3;

        // load the next 2 loads of 8 bytes and have every two
        // consecutive loads in the same 256 bit register
        srcRegHead2 = _mm256_castsi128_si256(
            _mm_loadl_epi64((const __m128i *)(src_ptr + 7 * src_pitch)));
        srcRegHead1 = _mm256_inserti128_si256(
            srcRegHead1, _mm256_castsi256_si128(srcRegHead2), 1);
        srcRegHead3 = _mm256_castsi128_si256(
            _mm_loadl_epi64((const __m128i *)(src_ptr + 8 * src_pitch)));
        srcRegHead2 = _mm256_inserti128_si256(
            srcRegHead2, _mm256_castsi256_si128(srcRegHead3), 1);

        // merge the two new consecutive registers
        // the first lane contain values for filtering odd rows (1,3,5...) and
        // the second lane contain values for filtering even rows (2,4,6...)
        s1[3] = _mm256_unpacklo_epi8(srcRegHead1, srcRegHead2);

        s1[0] = convolve8_16_avx2(s1, f);

        // shrink to 8 bit each 16 bits, the low and high 64-bits of each lane
        // contain the first and second convolve result respectively
        outReg1 = _mm256_castsi256_si128(s1[0]);
        outReg2 = _mm256_extractf128_si256(s1[0], 1);
        outReg1 = _mm_packus_epi16(outReg1, outReg2);

        src_ptr += src_stride;

        // average if necessary
        if (avg) {
            outReg2 = _mm_loadl_epi64((__m128i *)output_ptr);
            outReg2 = loadh_epi64(outReg2, (__m128i *)(output_ptr + out_pitch));
            outReg1 = _mm_avg_epu8(outReg1, outReg2);
        }

        // save 8 bytes
        _mm_storel_epi64((__m128i *)output_ptr, outReg1);

        // save the next 8 bytes
        _mm_storeh_epi64((__m128i *)(output_ptr + out_pitch), outReg1);

        output_ptr += dst_stride;

        // shift down by two rows
        s1[0] = s1[1];
        s1[1] = s1[2];
        s1[2] = s1[3];
        srcRegHead1 = srcRegHead3;
    }

    // if the number of strides is odd.
    // process only 8 bytes
    if (i > 0) {
        // load the last 8 bytes
        const __m128i srcRegHead2 =
            _mm_loadl_epi64((const __m128i *)(src_ptr + src_pitch * 7));

        // merge the last 2 results together
        s1[3] = _mm256_castsi128_si256(
            _mm_unpacklo_epi8(_mm256_castsi256_si128(srcRegHead1), srcRegHead2));

        outReg1 = convolve8_8_avx2(s1, f);

        // shrink to 8 bit each 16 bits, the low and high 64-bits of each lane
        // contain the first and second convolve result respectively
        outReg1 = _mm_packus_epi16(outReg1, outReg1);

        // average if necessary
        if (avg) {
            outReg1 = _mm_avg_epu8(outReg1, _mm_loadl_epi64((__m128i *)output_ptr));
        }

        // save 8 bytes
        _mm_storel_epi64((__m128i *)output_ptr, outReg1);
    }
}

static INLINE void vpx_filter_block1d16_v8_x_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
    ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter,
    const int avg) {
  __m128i outReg1, outReg2;
  __m256i srcRegHead1;
  unsigned int i;
  ptrdiff_t src_stride, dst_stride;
  __m256i f[4], s1[4], s2[4];

  shuffle_filter_avx2(filter, f);

  // multiple the size of the source and destination stride by two
  src_stride = src_pitch << 1;
  dst_stride = out_pitch << 1;

  {
    __m128i s[6];
    __m256i s32b[6];

    // load 16 bytes 7 times in stride of src_pitch
    s[0] = _mm_loadu_si128((const __m128i *)(src_ptr + 0 * src_pitch));
    s[1] = _mm_loadu_si128((const __m128i *)(src_ptr + 1 * src_pitch));
    s[2] = _mm_loadu_si128((const __m128i *)(src_ptr + 2 * src_pitch));
    s[3] = _mm_loadu_si128((const __m128i *)(src_ptr + 3 * src_pitch));
    s[4] = _mm_loadu_si128((const __m128i *)(src_ptr + 4 * src_pitch));
    s[5] = _mm_loadu_si128((const __m128i *)(src_ptr + 5 * src_pitch));
    srcRegHead1 = _mm256_castsi128_si256(
        _mm_loadu_si128((const __m128i *)(src_ptr + 6 * src_pitch)));

    // have each consecutive loads on the same 256 register
    s32b[0] = _mm256_inserti128_si256(_mm256_castsi128_si256(s[0]), s[1], 1);
    s32b[1] = _mm256_inserti128_si256(_mm256_castsi128_si256(s[1]), s[2], 1);
    s32b[2] = _mm256_inserti128_si256(_mm256_castsi128_si256(s[2]), s[3], 1);
    s32b[3] = _mm256_inserti128_si256(_mm256_castsi128_si256(s[3]), s[4], 1);
    s32b[4] = _mm256_inserti128_si256(_mm256_castsi128_si256(s[4]), s[5], 1);
    s32b[5] = _mm256_inserti128_si256(_mm256_castsi128_si256(s[5]),
                                      _mm256_castsi256_si128(srcRegHead1), 1);

    // merge every two consecutive registers except the last one
    // the first lanes contain values for filtering odd rows (1,3,5...) and
    // the second lanes contain values for filtering even rows (2,4,6...)
    s1[0] = _mm256_unpacklo_epi8(s32b[0], s32b[1]);
    s2[0] = _mm256_unpackhi_epi8(s32b[0], s32b[1]);
    s1[1] = _mm256_unpacklo_epi8(s32b[2], s32b[3]);
    s2[1] = _mm256_unpackhi_epi8(s32b[2], s32b[3]);
    s1[2] = _mm256_unpacklo_epi8(s32b[4], s32b[5]);
    s2[2] = _mm256_unpackhi_epi8(s32b[4], s32b[5]);
  }

  for (i = output_height; i > 1; i -= 2) {
    __m256i srcRegHead2, srcRegHead3;

    // load the next 2 loads of 16 bytes and have every two
    // consecutive loads in the same 256 bit register
    srcRegHead2 = _mm256_castsi128_si256(
        _mm_loadu_si128((const __m128i *)(src_ptr + 7 * src_pitch)));
    srcRegHead1 = _mm256_inserti128_si256(
        srcRegHead1, _mm256_castsi256_si128(srcRegHead2), 1);
    srcRegHead3 = _mm256_castsi128_si256(
        _mm_loadu_si128((const __m128i *)(src_ptr + 8 * src_pitch)));
    srcRegHead2 = _mm256_inserti128_si256(
        srcRegHead2, _mm256_castsi256_si128(srcRegHead3), 1);

    // merge the two new consecutive registers
    // the first lane contain values for filtering odd rows (1,3,5...) and
    // the second lane contain values for filtering even rows (2,4,6...)
    s1[3] = _mm256_unpacklo_epi8(srcRegHead1, srcRegHead2);
    s2[3] = _mm256_unpackhi_epi8(srcRegHead1, srcRegHead2);

    s1[0] = convolve8_16_avx2(s1, f);
    s2[0] = convolve8_16_avx2(s2, f);

    // shrink to 8 bit each 16 bits, the low and high 64-bits of each lane
    // contain the first and second convolve result respectively
    s1[0] = _mm256_packus_epi16(s1[0], s2[0]);

    src_ptr += src_stride;

    // average if necessary
    outReg1 = _mm256_castsi256_si128(s1[0]);
    outReg2 = _mm256_extractf128_si256(s1[0], 1);
    if (avg) {
      outReg1 = _mm_avg_epu8(outReg1, _mm_load_si128((__m128i *)output_ptr));
      outReg2 = _mm_avg_epu8(
          outReg2, _mm_load_si128((__m128i *)(output_ptr + out_pitch)));
    }

    // save 16 bytes
    _mm_store_si128((__m128i *)output_ptr, outReg1);

    // save the next 16 bits
    _mm_store_si128((__m128i *)(output_ptr + out_pitch), outReg2);

    output_ptr += dst_stride;

    // shift down by two rows
    s1[0] = s1[1];
    s2[0] = s2[1];
    s1[1] = s1[2];
    s2[1] = s2[2];
    s1[2] = s1[3];
    s2[2] = s2[3];
    srcRegHead1 = srcRegHead3;
  }

  // if the number of strides is odd.
  // process only 16 bytes
  if (i > 0) {
    // load the last 16 bytes
    const __m128i srcRegHead2 =
        _mm_loadu_si128((const __m128i *)(src_ptr + src_pitch * 7));

    // merge the last 2 results together
    s1[3] = _mm256_castsi128_si256(
        _mm_unpacklo_epi8(_mm256_castsi256_si128(srcRegHead1), srcRegHead2));
    s2[3] = _mm256_castsi128_si256(
        _mm_unpackhi_epi8(_mm256_castsi256_si128(srcRegHead1), srcRegHead2));

    outReg1 = convolve8_8_avx2(s1, f);
    outReg2 = convolve8_8_avx2(s2, f);

    // shrink to 8 bit each 16 bits, the low and high 64-bits of each lane
    // contain the first and second convolve result respectively
    outReg1 = _mm_packus_epi16(outReg1, outReg2);

    // average if necessary
    if (avg) {
      outReg1 = _mm_avg_epu8(outReg1, _mm_load_si128((__m128i *)output_ptr));
    }

    // save 16 bytes
    _mm_store_si128((__m128i *)output_ptr, outReg1);
  }
}

static void vpx_filter_block1d4_v8_avx2(const uint8_t *src_ptr,
    ptrdiff_t src_stride, uint8_t *dst_ptr,
    ptrdiff_t dst_stride, uint32_t height,
    const int16_t *filter) {
    vpx_filter_block1d4_v8_x_avx2(src_ptr, src_stride, dst_ptr, dst_stride,
        height, filter, 0);
}

static void vpx_filter_block1d4_v8_avg_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_stride, uint8_t *dst_ptr,
    ptrdiff_t dst_stride, uint32_t height, const int16_t *filter) {
    vpx_filter_block1d4_v8_x_avx2(src_ptr, src_stride, dst_ptr, dst_stride,
        height, filter, 1);
}

static void vpx_filter_block1d8_v8_avx2(const uint8_t *src_ptr,
    ptrdiff_t src_stride, uint8_t *dst_ptr,
    ptrdiff_t dst_stride, uint32_t height,
    const int16_t *filter) {
    vpx_filter_block1d8_v8_x_avx2(src_ptr, src_stride, dst_ptr, dst_stride,
        height, filter, 0);
}

static void vpx_filter_block1d8_v8_avg_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_stride, uint8_t *dst_ptr,
    ptrdiff_t dst_stride, uint32_t height, const int16_t *filter) {
    vpx_filter_block1d8_v8_x_avx2(src_ptr, src_stride, dst_ptr, dst_stride,
        height, filter, 1);
}

static void vpx_filter_block1d16_v8_avx2(const uint8_t *src_ptr,
                                         ptrdiff_t src_stride, uint8_t *dst_ptr,
                                         ptrdiff_t dst_stride, uint32_t height,
                                         const int16_t *filter) {
  vpx_filter_block1d16_v8_x_avx2(src_ptr, src_stride, dst_ptr, dst_stride,
                                 height, filter, 0);
}

static void vpx_filter_block1d16_v8_avg_avx2(
    const uint8_t *src_ptr, ptrdiff_t src_stride, uint8_t *dst_ptr,
    ptrdiff_t dst_stride, uint32_t height, const int16_t *filter) {
  vpx_filter_block1d16_v8_x_avx2(src_ptr, src_stride, dst_ptr, dst_stride,
                                 height, filter, 1);
}

filter8_1dfunction eb_vp9_filter_block1d16_v2_ssse3;
filter8_1dfunction eb_vp9_filter_block1d16_h2_ssse3;
filter8_1dfunction eb_vp9_filter_block1d8_v2_ssse3;
filter8_1dfunction eb_vp9_filter_block1d8_h2_ssse3;
filter8_1dfunction eb_vp9_filter_block1d4_v2_ssse3;
filter8_1dfunction eb_vp9_filter_block1d4_h2_ssse3;
#define vpx_filter_block1d16_v2_avx2 eb_vp9_filter_block1d16_v2_ssse3
#define vpx_filter_block1d16_h2_avx2 eb_vp9_filter_block1d16_h2_ssse3
#define vpx_filter_block1d8_v2_avx2 eb_vp9_filter_block1d8_v2_ssse3
#define vpx_filter_block1d8_h2_avx2 eb_vp9_filter_block1d8_h2_ssse3
#define vpx_filter_block1d4_v2_avx2 eb_vp9_filter_block1d4_v2_ssse3
#define vpx_filter_block1d4_h2_avx2 eb_vp9_filter_block1d4_h2_ssse3
filter8_1dfunction eb_vp9_filter_block1d16_v2_avg_ssse3;
filter8_1dfunction eb_vp9_filter_block1d16_h2_avg_ssse3;
filter8_1dfunction eb_vp9_filter_block1d8_v2_avg_ssse3;
filter8_1dfunction eb_vp9_filter_block1d8_h2_avg_ssse3;
filter8_1dfunction eb_vp9_filter_block1d4_v2_avg_ssse3;
filter8_1dfunction eb_vp9_filter_block1d4_h2_avg_ssse3;
#define vpx_filter_block1d16_v2_avg_avx2 eb_vp9_filter_block1d16_v2_avg_ssse3
#define vpx_filter_block1d16_h2_avg_avx2 eb_vp9_filter_block1d16_h2_avg_ssse3
#define vpx_filter_block1d8_v2_avg_avx2 eb_vp9_filter_block1d8_v2_avg_ssse3
#define vpx_filter_block1d8_h2_avg_avx2 eb_vp9_filter_block1d8_h2_avg_ssse3
#define vpx_filter_block1d4_v2_avg_avx2 eb_vp9_filter_block1d4_v2_avg_ssse3
#define vpx_filter_block1d4_h2_avg_avx2 eb_vp9_filter_block1d4_h2_avg_ssse3
// void eb_vp9_convolve8_horiz_avx2(const uint8_t *src, ptrdiff_t src_stride,
//                                uint8_t *dst, ptrdiff_t dst_stride,
//                                const InterpKernel *filter, int x0_q4,
//                                int32_t x_step_q4, int y0_q4, int y_step_q4,
//                                int w, int h);
// void eb_vp9_convolve8_vert_avx2(const uint8_t *src, ptrdiff_t src_stride,
//                               uint8_t *dst, ptrdiff_t dst_stride,
//                               const InterpKernel *filter, int x0_q4,
//                               int32_t x_step_q4, int y0_q4, int y_step_q4,
//                               int w, int h);
// void eb_vp9_convolve8_avg_horiz_avx2(const uint8_t *src, ptrdiff_t src_stride,
//                                    uint8_t *dst, ptrdiff_t dst_stride,
//                                    const InterpKernel *filter, int x0_q4,
//                                    int32_t x_step_q4, int y0_q4,
//                                    int y_step_q4, int w, int h);
// void eb_vp9_convolve8_avg_vert_avx2(const uint8_t *src, ptrdiff_t src_stride,
//                                   uint8_t *dst, ptrdiff_t dst_stride,
//                                   const InterpKernel *filter, int x0_q4,
//                                   int32_t x_step_q4, int y0_q4,
//                                   int y_step_q4, int w, int h);
FUN_CONV_1D(horiz, x0_q4, x_step_q4, h, src, , avx2);
FUN_CONV_1D(vert, y0_q4, y_step_q4, v, src - src_stride * 3, , avx2);
FUN_CONV_1D(avg_horiz, x0_q4, x_step_q4, h, src, avg_, avx2);
FUN_CONV_1D(avg_vert, y0_q4, y_step_q4, v, src - src_stride * 3, avg_, avx2);

// void eb_vp9_convolve8_avx2(const uint8_t *src, ptrdiff_t src_stride,
//                          uint8_t *dst, ptrdiff_t dst_stride,
//                          const InterpKernel *filter, int x0_q4,
//                          int32_t x_step_q4, int y0_q4, int y_step_q4,
//                          int w, int h);
// void eb_vp9_convolve8_avg_avx2(const uint8_t *src, ptrdiff_t src_stride,
//                              uint8_t *dst, ptrdiff_t dst_stride,
//                              const InterpKernel *filter, int x0_q4,
//                              int32_t x_step_q4, int y0_q4, int y_step_q4,
//                              int w, int h);
FUN_CONV_2D(, avx2);
FUN_CONV_2D(avg_, avx2);
