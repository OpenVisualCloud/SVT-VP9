/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include "stddef.h"
#include "stdint.h"
#include "immintrin.h"
#include "vpx_dsp_rtcd.h"

#if 0
static void _mm_storeh_epi64(__m128i * p, __m128i x)
{
  _mm_storeh_pd((double *)p, _mm_castsi128_pd(x));
}
#endif

static INLINE __m256i avg3_avx2(const __m256i a, const __m256i b,
    const __m256i c)
{
    const __m256i c1 = _mm256_set1_epi8(1);
    __m256i t0, t1;
    t0               = _mm256_xor_si256(a, c);   // a ^ c
    t0               = _mm256_and_si256(t0, c1); // (a ^ c) & 1
    t1               = _mm256_avg_epu8(a, c);    // (a + c + 1) >> 1
    // ((a + c + 1) >> 1) - ((a ^ c) & 1)
    t1               = _mm256_sub_epi8(t1, t0);
    // (((a + c + 1) >> 1) - ((a ^ c) & 1) + b + 1) >> 1
    //               = (a + 2 * b + c + 2) >> 2
    t1               = _mm256_avg_epu8(t1, b);
    return t1;
}

// D117
static INLINE __m256i d117_avg3(const uint8_t *const ref) {
    const __m256i r0 = _mm256_load_si256((const __m256i *)(ref + 0));
    const __m256i r1 = _mm256_loadu_si256((const __m256i *)(ref + 1));
    const __m256i r2 = _mm256_loadu_si256((const __m256i *)(ref + 2));
    return avg3_avx2(r0, r1, r2);
}

static INLINE void d117_avg3_last(
    const uint8_t *const ref,
    __m256i *const       b0h,
    __m256i *const       b)
{
    const __m256i r0 = _mm256_load_si256((const __m256i *)(ref + 0));
    const __m256i r1 = _mm256_loadu_si256((const __m256i *)(ref + 1));
    const __m256i r2 = _mm256_loadu_si256((const __m256i *)(ref + 2));
    *b0h = _mm256_avg_epu8(r0, r1);
    *b = avg3_avx2(r0, r1, r2);
}

void eb_vp9_d117_predictor_32x32_avx2(
    uint8_t       *dst,
    ptrdiff_t      stride,
    const uint8_t *above,
    const uint8_t *left)
{
    // ref[] has an extra element to facilitate optimization.
    EB_ALIGN(32) uint8_t ref[65 + 1];
    EB_ALIGN(32) uint8_t border0[64];  // for even rows
    EB_ALIGN(32) uint8_t border1[64];  // for odd rows

    // reverse left
    // copy above
    const __m256i c_rev = _mm256_setr_epi8(
        15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
        15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    const __m256i l = _mm256_loadu_si256((const __m256i *)left);
    const __m256i l_r = _mm256_shuffle_epi8(l, c_rev);
    const __m256i l_rev = _mm256_inserti128_si256(
        _mm256_castsi128_si256(_mm256_extracti128_si256(l_r, 1)),
        _mm256_extracti128_si256(l_r, 0), 1);
    const __m256i a = _mm256_loadu_si256((const __m256i *)(above - 1));
    __m256i b[2];

    _mm256_store_si256((__m256i *)(ref + 0x00), l_rev);
    _mm256_store_si256((__m256i *)(ref + 0x20), a);
    ref[64] = above[31];
    ref[65] = 0; // Useless. Initialize to avoid warning.

    // average
    b[0] = d117_avg3(ref);
    __m256i b0h;
    d117_avg3_last(ref + 32, &b0h, &b[1]);

    // shuffle
    const __m256i b0l = _mm256_shuffle_epi8(b[0],
        _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 6, 8, 10, 12, 14,
            0, 2, 4, 6, 8, 10, 12, 14, 0, 0, 0, 0, 0, 0, 0, 0));
    const __m256i b1l = _mm256_shuffle_epi8(b[0],
        _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 5, 7, 9, 11, 13, 15,
            1, 3, 5, 7, 9, 11, 13, 15, 0, 0, 0, 0, 0, 0, 0, 0));
    _mm256_store_si256 ((__m256i *)(border0 + 0x00), b0l);
    _mm256_storeu_si256((__m256i *)(border0 + 0x18), b0h);
    _mm256_store_si256 ((__m256i *)(border1 + 0x00), b1l);
    _mm256_storeu_si256((__m256i *)(border1 + 0x18), b[1]);

    // store
    _mm256_store_si256((__m256i *)(dst + 0 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 24)));
    _mm256_store_si256((__m256i *)(dst + 1 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 23)));
    _mm256_store_si256((__m256i *)(dst + 2 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 23)));
    _mm256_store_si256((__m256i *)(dst + 3 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 22)));
    _mm256_store_si256((__m256i *)(dst + 4 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 22)));
    _mm256_store_si256((__m256i *)(dst + 5 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 21)));
    _mm256_store_si256((__m256i *)(dst + 6 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 21)));
    _mm256_store_si256((__m256i *)(dst + 7 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 20)));
    _mm256_store_si256((__m256i *)(dst + 8 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 20)));
    _mm256_store_si256((__m256i *)(dst + 9 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 19)));
    _mm256_store_si256((__m256i *)(dst + 10 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 19)));
    _mm256_store_si256((__m256i *)(dst + 11 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 18)));
    _mm256_store_si256((__m256i *)(dst + 12 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 18)));
    _mm256_store_si256((__m256i *)(dst + 13 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 17)));
    _mm256_store_si256((__m256i *)(dst + 14 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 17)));
    _mm256_store_si256((__m256i *)(dst + 15 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 16)));
    _mm256_store_si256((__m256i *)(dst + 16 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 16)));
    _mm256_store_si256((__m256i *)(dst + 17 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 15)));
    _mm256_store_si256((__m256i *)(dst + 18 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 15)));
    _mm256_store_si256((__m256i *)(dst + 19 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 14)));
    _mm256_store_si256((__m256i *)(dst + 20 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 14)));
    _mm256_store_si256((__m256i *)(dst + 21 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 13)));
    _mm256_store_si256((__m256i *)(dst + 22 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 13)));
    _mm256_store_si256((__m256i *)(dst + 23 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 12)));
    _mm256_store_si256((__m256i *)(dst + 24 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 12)));
    _mm256_store_si256((__m256i *)(dst + 25 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 11)));
    _mm256_store_si256((__m256i *)(dst + 26 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 11)));
    _mm256_store_si256((__m256i *)(dst + 27 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 10)));
    _mm256_store_si256((__m256i *)(dst + 28 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 10)));
    _mm256_store_si256((__m256i *)(dst + 29 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 9)));
    _mm256_store_si256((__m256i *)(dst + 30 * stride), _mm256_loadu_si256((const __m256i *)(border0 + 9)));
    _mm256_store_si256((__m256i *)(dst + 31 * stride), _mm256_loadu_si256((const __m256i *)(border1 + 8)));
}

static INLINE __m256i d135_avg3(const uint8_t *const ref) {
    const __m256i r0 = _mm256_load_si256 ((const __m256i *)(ref + 0));
    const __m256i r1 = _mm256_loadu_si256((const __m256i *)(ref + 1));
    const __m256i r2 = _mm256_loadu_si256((const __m256i *)(ref + 2));
    return avg3_avx2(r0, r1, r2);
}

void eb_vp9_d135_predictor_32x32_avx2(
    uint8_t       *dst,
    ptrdiff_t      stride,
    const uint8_t *above,
    const uint8_t *left)
{
    // ref[] has an extra element to facilitate optimization.
    EB_ALIGN(32) uint8_t ref[65 + 1];
    // border[] has an extra element to facilitate optimization.
    EB_ALIGN(32) uint8_t border[64];  // outer border from bottom-left to top-right

    // reverse left
    // copy above
    const __m256i c_rev = _mm256_setr_epi8(
        15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
        15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    const __m256i l = _mm256_loadu_si256((const __m256i *)left);
    const __m256i l_r = _mm256_shuffle_epi8(l, c_rev);
    const __m256i l_rev = _mm256_inserti128_si256(
        _mm256_castsi128_si256(_mm256_extracti128_si256(l_r, 1)),
        _mm256_extracti128_si256(l_r, 0), 1);
    const __m256i a = _mm256_loadu_si256((const __m256i *)(above - 1));
    __m256i b[2];

    _mm256_store_si256((__m256i *)(ref + 0x00), l_rev);
    _mm256_store_si256((__m256i *)(ref + 0x20), a);
    ref[64] = above[31];
    ref[65] = 0; // Useless. Initialize to avoid warning.

    // average
    b[0] = d135_avg3(ref);
    b[1] = d135_avg3(ref + 32);
    _mm256_store_si256((__m256i *)(border + 0x00), b[0]);
    _mm256_store_si256((__m256i *)(border + 0x20), b[1]);

    // store
    _mm256_store_si256((__m256i *)(dst + 0 * stride), _mm256_loadu_si256((const __m256i *)(border + 31)));
    _mm256_store_si256((__m256i *)(dst + 1 * stride), _mm256_loadu_si256((const __m256i *)(border + 30)));
    _mm256_store_si256((__m256i *)(dst + 2 * stride), _mm256_loadu_si256((const __m256i *)(border + 29)));
    _mm256_store_si256((__m256i *)(dst + 3 * stride), _mm256_loadu_si256((const __m256i *)(border + 28)));
    _mm256_store_si256((__m256i *)(dst + 4 * stride), _mm256_loadu_si256((const __m256i *)(border + 27)));
    _mm256_store_si256((__m256i *)(dst + 5 * stride), _mm256_loadu_si256((const __m256i *)(border + 26)));
    _mm256_store_si256((__m256i *)(dst + 6 * stride), _mm256_loadu_si256((const __m256i *)(border + 25)));
    _mm256_store_si256((__m256i *)(dst + 7 * stride), _mm256_loadu_si256((const __m256i *)(border + 24)));
    _mm256_store_si256((__m256i *)(dst + 8 * stride), _mm256_loadu_si256((const __m256i *)(border + 23)));
    _mm256_store_si256((__m256i *)(dst + 9 * stride), _mm256_loadu_si256((const __m256i *)(border + 22)));
    _mm256_store_si256((__m256i *)(dst + 10 * stride), _mm256_loadu_si256((const __m256i *)(border + 21)));
    _mm256_store_si256((__m256i *)(dst + 11 * stride), _mm256_loadu_si256((const __m256i *)(border + 20)));
    _mm256_store_si256((__m256i *)(dst + 12 * stride), _mm256_loadu_si256((const __m256i *)(border + 19)));
    _mm256_store_si256((__m256i *)(dst + 13 * stride), _mm256_loadu_si256((const __m256i *)(border + 18)));
    _mm256_store_si256((__m256i *)(dst + 14 * stride), _mm256_loadu_si256((const __m256i *)(border + 17)));
    _mm256_store_si256((__m256i *)(dst + 15 * stride), _mm256_loadu_si256((const __m256i *)(border + 16)));
    _mm256_store_si256((__m256i *)(dst + 16 * stride), _mm256_loadu_si256((const __m256i *)(border + 15)));
    _mm256_store_si256((__m256i *)(dst + 17 * stride), _mm256_loadu_si256((const __m256i *)(border + 14)));
    _mm256_store_si256((__m256i *)(dst + 18 * stride), _mm256_loadu_si256((const __m256i *)(border + 13)));
    _mm256_store_si256((__m256i *)(dst + 19 * stride), _mm256_loadu_si256((const __m256i *)(border + 12)));
    _mm256_store_si256((__m256i *)(dst + 20 * stride), _mm256_loadu_si256((const __m256i *)(border + 11)));
    _mm256_store_si256((__m256i *)(dst + 21 * stride), _mm256_loadu_si256((const __m256i *)(border + 10)));
    _mm256_store_si256((__m256i *)(dst + 22 * stride), _mm256_loadu_si256((const __m256i *)(border + 9)));
    _mm256_store_si256((__m256i *)(dst + 23 * stride), _mm256_loadu_si256((const __m256i *)(border + 8)));
    _mm256_store_si256((__m256i *)(dst + 24 * stride), _mm256_loadu_si256((const __m256i *)(border + 7)));
    _mm256_store_si256((__m256i *)(dst + 25 * stride), _mm256_loadu_si256((const __m256i *)(border + 6)));
    _mm256_store_si256((__m256i *)(dst + 26 * stride), _mm256_loadu_si256((const __m256i *)(border + 5)));
    _mm256_store_si256((__m256i *)(dst + 27 * stride), _mm256_loadu_si256((const __m256i *)(border + 4)));
    _mm256_store_si256((__m256i *)(dst + 28 * stride), _mm256_loadu_si256((const __m256i *)(border + 3)));
    _mm256_store_si256((__m256i *)(dst + 29 * stride), _mm256_loadu_si256((const __m256i *)(border + 2)));
    _mm256_store_si256((__m256i *)(dst + 30 * stride), _mm256_loadu_si256((const __m256i *)(border + 1)));
    _mm256_store_si256((__m256i *)(dst + 31 * stride), _mm256_load_si256((const __m256i *)(border + 0)));
}
