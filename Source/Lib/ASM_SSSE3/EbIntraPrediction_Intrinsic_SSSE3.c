/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include "stddef.h"
#include "stdint.h"
#include "tmmintrin.h"
#include "vpx_dsp_rtcd.h"

#if 0
// Keep these 2 functions which are the code base of x86 optimizations.
static INLINE void d117_pred(uint8_t *dst, ptrdiff_t stride, int bs,
    const uint8_t *above, const uint8_t *left)
{
    EB_ALIGN(16) uint8_t ref[32 + 32 + 1];
    uint8_t border[32 + 32 - 1];  // outer border from bottom-left to top-right
    uint8_t border0[64];          // for even rows
    uint8_t border1[64];          // for odd rows

    // reverse left
    for (int i = 1; i < bs; ++i) {
        ref[i] = left[bs - 1 - i];
    }

    // copy above
    for (int i = bs; i <= 2 * bs; ++i) {
        ref[i] = above[i - bs - 1];
    }

    // average
    for (int i = 1; i < 2 * bs - 1; ++i) {
        border[i] = AVG3(ref[i], ref[i + 1], ref[i + 2]);
    }

    // shuffle
    for (int i = 0; i < bs / 2 - 1; ++i) {
        border1[i] = border[2 * i + 1];
        border0[i] = border[2 * i + 2];
    }

    for (int i = 0; i < bs; ++i) {
        border0[bs / 2 - 1 + i] = AVG2(ref[bs + i], ref[bs + i + 1]);
        border1[bs / 2 - 1 + i] = border[bs - 1 + i];
    }

    // store
    for (int i = 0; i < bs / 2; ++i) {
        EB_MEMCPY(dst + (2 * i + 0) * stride, border0 + bs / 2 - 1 - i, bs);
        EB_MEMCPY(dst + (2 * i + 1) * stride, border1 + bs / 2 - 1 - i, bs);
    }
}

static void d135_pred(uint8_t *dst, ptrdiff_t stride, int bs,
    const uint8_t *above, const uint8_t *left)
{
    EB_ALIGN(16) uint8_t ref[32 + 32 + 1];
    uint8_t border[32 + 32 - 1];  // outer border from bottom-left to top-right

    // reverse left
    for (int i = 0; i < bs; ++i) {
        ref[i] = left[bs - 1 - i];
    }

    // copy above
    for (int i = bs; i <= 2 * bs; ++i) {
        ref[i] = above[i - bs - 1];
    }

    // average
    for (int i = 0; i < 2 * bs - 1; ++i) {
        border[i] = AVG3(ref[i], ref[i + 1], ref[i + 2]);
    }

    // store
    for (int i = 0; i < bs; ++i) {
        EB_MEMCPY(dst + i * stride, border + bs - 1 - i, bs);
    }
}
#endif

// D117

static INLINE __m128i avg3_sse2(const __m128i a, const __m128i b,
    const __m128i c)
{
    const __m128i c1 = _mm_set1_epi8(1);
    __m128i t0, t1;
    t0 = _mm_xor_si128(a, c);   // a ^ c
    t0 = _mm_and_si128(t0, c1); // (a ^ c) & 1
    t1 = _mm_avg_epu8(a, c);    // (a + c + 1) >> 1
    // ((a + c + 1) >> 1) - ((a ^ c) & 1)
    t1 = _mm_sub_epi8(t1, t0);
    // (((a + c + 1) >> 1) - ((a ^ c) & 1) + b + 1) >> 1
    // = (a + 2 * b + c + 2) >> 2
    t1 = _mm_avg_epu8(t1, b);
    return t1;
}

static INLINE void d117_avg3_4(const __m128i r0, __m128i *const b0h,
    __m128i *const b)
{
    const __m128i r1 = _mm_srli_si128(r0, 1);
    const __m128i r2 = _mm_srli_si128(r0, 2);
    *b0h = _mm_avg_epu8(r0, r1);
    *b0h = _mm_srli_si128(*b0h, 4);
    *b = avg3_sse2(r0, r1, r2);
}

static INLINE void d117_avg3_8(const uint8_t *const ref, __m128i *const b0h,
    __m128i *const b)
{
    const __m128i r0 = _mm_load_si128((const __m128i *)ref);
    const __m128i r1 = _mm_srli_si128(r0, 1);
    const __m128i r2 = _mm_srli_si128(r0, 2);
    *b0h = _mm_avg_epu8(r0, r1);
    *b0h = _mm_srli_si128(*b0h, 7);
    *b = avg3_sse2(r0, r1, r2);
}

static INLINE __m128i d117_avg3(const uint8_t *const ref) {
    const __m128i r0 = _mm_load_si128((const __m128i *)(ref + 0));
    const __m128i r1 = _mm_loadu_si128((const __m128i *)(ref + 1));
    const __m128i r2 = _mm_loadu_si128((const __m128i *)(ref + 2));
    return avg3_sse2(r0, r1, r2);
}

static INLINE void d117_avg3_last(const uint8_t *const ref, __m128i *const b0h,
    __m128i *const b)
{
    const __m128i r0 = _mm_loadu_si128((const __m128i *)(ref - 1));
    const __m128i r1 = _mm_load_si128((const __m128i *)ref);
    const __m128i r2 = _mm_srli_si128(r1, 1);
    *b0h = _mm_avg_epu8(r0, r1);
    *b = avg3_sse2(r0, r1, r2);
}

void eb_vp9_d117_predictor_4x4_ssse3(uint8_t *dst, ptrdiff_t stride,
    const uint8_t *above, const uint8_t *left)
{
    // reverse left
    // copy above
    const __m128i c_rev =
        _mm_setr_epi8(3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0);
    const __m128i l = _mm_cvtsi32_si128(*(const int *)left);
    const __m128i l_rev = _mm_shuffle_epi8(l, c_rev);
    const __m128i a = _mm_cvtsi32_si128(*(const int *)(above - 1));
    const __m128i r8 = _mm_cvtsi32_si128(above[3]);
    const __m128i la = _mm_unpacklo_epi32(l_rev, a);
    const __m128i r = _mm_unpacklo_epi64(la, r8);

    // average
    __m128i b, b0h;
    d117_avg3_4(r, &b0h, &b);

    // shuffle
    const __m128i b0l = _mm_shuffle_epi8(b,
        _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0));
    const __m128i b1 = _mm_shuffle_epi8(b,
        _mm_setr_epi8(0, 1, 3, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
    const __m128i b0 = _mm_unpacklo_epi64(b0l, b0h);

    // store
    *(int *)(dst + 0 * stride) = _mm_cvtsi128_si32(_mm_srli_si128(b0, 8));
    *(int *)(dst + 1 * stride) = _mm_cvtsi128_si32(_mm_srli_si128(b1, 2));
    *(int *)(dst + 2 * stride) = _mm_cvtsi128_si32(_mm_srli_si128(b0, 7));
    *(int *)(dst + 3 * stride) = _mm_cvtsi128_si32(_mm_srli_si128(b1, 1));
}

void eb_vp9_d117_predictor_8x8_ssse3(uint8_t *dst, ptrdiff_t stride,
    const uint8_t *above, const uint8_t *left)
{
    EB_ALIGN(16) uint8_t ref[16];

    // reverse left
    // copy above
    const __m128i c_rev = _mm_setr_epi8(6, 5, 4, 3, 2, 1, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0);
    const __m128i l = _mm_loadl_epi64((const __m128i *)left);
    const __m128i l_rev = _mm_shuffle_epi8(l, c_rev);
    const __m128i a = _mm_loadl_epi64((const __m128i *)(above - 1));

    _mm_storel_epi64((__m128i *)(ref + 0), l_rev);
    _mm_storel_epi64((__m128i *)(ref + 7), a);
    ref[15] = above[7];

    // average
    __m128i b, b0h;
    d117_avg3_8(ref, &b0h, &b);

    // shuffle
    const __m128i b0l = _mm_shuffle_epi8(b,
        _mm_setr_epi8(0, 0, 0, 0, 0, 1, 3, 5, 0, 0, 0, 0, 0, 0, 0, 0));
    const __m128i b1 = _mm_shuffle_epi8(b,
        _mm_setr_epi8(0, 2, 4, 6, 7, 8, 9, 10, 11, 12, 13, 0, 0, 0, 0, 0));
    const __m128i b0 = _mm_unpacklo_epi64(b0l, b0h);

    // store
    _mm_storel_epi64((__m128i *)(dst + 0 * stride), _mm_srli_si128(b0, 8));
    _mm_storel_epi64((__m128i *)(dst + 1 * stride), _mm_srli_si128(b1, 3));
    _mm_storel_epi64((__m128i *)(dst + 2 * stride), _mm_srli_si128(b0, 7));
    _mm_storel_epi64((__m128i *)(dst + 3 * stride), _mm_srli_si128(b1, 2));
    _mm_storel_epi64((__m128i *)(dst + 4 * stride), _mm_srli_si128(b0, 6));
    _mm_storel_epi64((__m128i *)(dst + 5 * stride), _mm_srli_si128(b1, 1));
    _mm_storel_epi64((__m128i *)(dst + 6 * stride), _mm_srli_si128(b0, 5));
    _mm_storel_epi64((__m128i *)(dst + 7 * stride), b1);
}

void eb_vp9_d117_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t stride,
    const uint8_t *above, const uint8_t *left)
{
    EB_ALIGN(16) uint8_t ref[32];
    EB_ALIGN(16) uint8_t border0[32];  // for even rows
    EB_ALIGN(16) uint8_t border1[32];  // for odd rows

    // reverse left
    // copy above
    const __m128i c_rev =
        _mm_setr_epi8(14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0);
    const __m128i l = _mm_loadu_si128((const __m128i *)left);
    const __m128i l_rev = _mm_shuffle_epi8(l, c_rev);
    const __m128i a = _mm_loadu_si128((const __m128i *)(above - 1));
    __m128i b[2];

    _mm_store_si128((__m128i *)(ref + 0), l_rev);
    _mm_storeu_si128((__m128i *)(ref + 15), a);
    ref[31] = above[15];

    // average
    b[0] = d117_avg3(ref);
    __m128i b0h;
    d117_avg3_last(ref + 16, &b0h, &b[1]);

    // shuffle
    const __m128i b0l = _mm_shuffle_epi8(b[0],
        _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 5, 7, 9, 11, 13));
    const __m128i b1l = _mm_shuffle_epi8(b[0],
        _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 6, 8, 10, 12, 14));
    _mm_store_si128((__m128i *)(border0 + 0x00), b0l);
    _mm_store_si128((__m128i *)(border0 + 0x10), b0h);
    _mm_store_si128((__m128i *)(border1 + 0x00), b1l);
    _mm_store_si128((__m128i *)(border1 + 0x10), b[1]);

    // store
    _mm_store_si128((__m128i *)(dst + 0 * stride), _mm_load_si128((const __m128i *)(border0 + 16)));
    _mm_store_si128((__m128i *)(dst + 1 * stride), _mm_loadu_si128((const __m128i *)(border1 + 15)));
    _mm_store_si128((__m128i *)(dst + 2 * stride), _mm_loadu_si128((const __m128i *)(border0 + 15)));
    _mm_store_si128((__m128i *)(dst + 3 * stride), _mm_loadu_si128((const __m128i *)(border1 + 14)));
    _mm_store_si128((__m128i *)(dst + 4 * stride), _mm_loadu_si128((const __m128i *)(border0 + 14)));
    _mm_store_si128((__m128i *)(dst + 5 * stride), _mm_loadu_si128((const __m128i *)(border1 + 13)));
    _mm_store_si128((__m128i *)(dst + 6 * stride), _mm_loadu_si128((const __m128i *)(border0 + 13)));
    _mm_store_si128((__m128i *)(dst + 7 * stride), _mm_loadu_si128((const __m128i *)(border1 + 12)));
    _mm_store_si128((__m128i *)(dst + 8 * stride), _mm_loadu_si128((const __m128i *)(border0 + 12)));
    _mm_store_si128((__m128i *)(dst + 9 * stride), _mm_loadu_si128((const __m128i *)(border1 + 11)));
    _mm_store_si128((__m128i *)(dst + 10 * stride), _mm_loadu_si128((const __m128i *)(border0 + 11)));
    _mm_store_si128((__m128i *)(dst + 11 * stride), _mm_loadu_si128((const __m128i *)(border1 + 10)));
    _mm_store_si128((__m128i *)(dst + 12 * stride), _mm_loadu_si128((const __m128i *)(border0 + 10)));
    _mm_store_si128((__m128i *)(dst + 13 * stride), _mm_loadu_si128((const __m128i *)(border1 + 9)));
    _mm_store_si128((__m128i *)(dst + 14 * stride), _mm_loadu_si128((const __m128i *)(border0 + 9)));
    _mm_store_si128((__m128i *)(dst + 15 * stride), _mm_loadu_si128((const __m128i *)(border1 + 8)));
}

// -----------------------------------------------------------------------------
// D135

static INLINE __m128i d135_avg3_4(const __m128i r0) {
    const __m128i r1 = _mm_srli_si128(r0, 1);
    const __m128i r2 = _mm_srli_si128(r0, 2);
    return avg3_sse2(r0, r1, r2);
}

static INLINE __m128i d135_avg3(const uint8_t *const ref) {
    const __m128i r0 = _mm_load_si128((const __m128i *)(ref + 0));
    const __m128i r1 = _mm_loadu_si128((const __m128i *)(ref + 1));
    const __m128i r2 = _mm_loadu_si128((const __m128i *)(ref + 2));
    return avg3_sse2(r0, r1, r2);
}

static INLINE __m128i d135_avg3_last(const uint8_t *const ref) {
    const __m128i r0 = _mm_load_si128((const __m128i *)(ref + 0));
    const __m128i r1 = _mm_loadu_si128((const __m128i *)(ref + 1));
    const __m128i r2 = _mm_srli_si128(r1, 1);
    return avg3_sse2(r0, r1, r2);
}

void eb_vp9_d135_predictor_4x4_ssse3(uint8_t *dst, ptrdiff_t stride,
    const uint8_t *above, const uint8_t *left)
{
    // reverse left
    // copy above
    const __m128i c_rev =
        _mm_setr_epi8(3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0);
    const __m128i l = _mm_cvtsi32_si128(*(const int *)left);
    const __m128i l_rev = _mm_shuffle_epi8(l, c_rev);
    const __m128i a = _mm_cvtsi32_si128(*(const int *)(above - 1));
    const __m128i r8 = _mm_cvtsi32_si128(above[3]);
    const __m128i la = _mm_unpacklo_epi32(l_rev, a);
    const __m128i r = _mm_unpacklo_epi64(la, r8);

    // average
    const __m128i b = d135_avg3_4(r);

    // store
    *(int *)(dst + 0 * stride) = _mm_cvtsi128_si32(_mm_srli_si128(b, 3));
    *(int *)(dst + 1 * stride) = _mm_cvtsi128_si32(_mm_srli_si128(b, 2));
    *(int *)(dst + 2 * stride) = _mm_cvtsi128_si32(_mm_srli_si128(b, 1));
    *(int *)(dst + 3 * stride) = _mm_cvtsi128_si32(b);
}

void eb_vp9_d135_predictor_8x8_ssse3(uint8_t *dst, ptrdiff_t stride,
    const uint8_t *above, const uint8_t *left)
{
    EB_ALIGN(16) uint8_t ref[17];

    // reverse left
    // copy above
    const __m128i c_rev =
        _mm_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 7, 6, 5, 4, 3, 2, 1, 0);
    const __m128i l = _mm_loadl_epi64((const __m128i *)left);
    const __m128i l_rev = _mm_shuffle_epi8(l, c_rev);
    const __m128i a = _mm_loadl_epi64((const __m128i *)(above - 1));
    const __m128i r = _mm_unpacklo_epi64(l_rev, a);

    _mm_store_si128((__m128i *)ref, r);
    ref[16] = above[7];

    // average
    const __m128i b = d135_avg3_last(ref);

    // store
    _mm_storel_epi64((__m128i *)(dst + 0 * stride), _mm_srli_si128(b, 7));
    _mm_storel_epi64((__m128i *)(dst + 1 * stride), _mm_srli_si128(b, 6));
    _mm_storel_epi64((__m128i *)(dst + 2 * stride), _mm_srli_si128(b, 5));
    _mm_storel_epi64((__m128i *)(dst + 3 * stride), _mm_srli_si128(b, 4));
    _mm_storel_epi64((__m128i *)(dst + 4 * stride), _mm_srli_si128(b, 3));
    _mm_storel_epi64((__m128i *)(dst + 5 * stride), _mm_srli_si128(b, 2));
    _mm_storel_epi64((__m128i *)(dst + 6 * stride), _mm_srli_si128(b, 1));
    _mm_storel_epi64((__m128i *)(dst + 7 * stride), b);
}

void eb_vp9_d135_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t stride,
    const uint8_t *above, const uint8_t *left)
{
    EB_ALIGN(16) uint8_t ref[33];
    // border[] has an extra element to facilitate optimization.
    EB_ALIGN(16) uint8_t border[32];  // outer border from bottom-left to top-right

    // reverse left
    // copy above
    const __m128i c_rev =
        _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    const __m128i l = _mm_loadu_si128((const __m128i *)left);
    const __m128i l_rev = _mm_shuffle_epi8(l, c_rev);
    const __m128i a = _mm_loadu_si128((const __m128i *)(above - 1));
    __m128i b[2];

    _mm_store_si128((__m128i *)(ref + 0x00), l_rev);
    _mm_store_si128((__m128i *)(ref + 0x10), a);
    ref[32] = above[15];

    // average
    b[0] = d135_avg3(ref);
    b[1] = d135_avg3_last(ref + 16);
    _mm_store_si128((__m128i *)(border + 0x00), b[0]);
    _mm_store_si128((__m128i *)(border + 0x10), b[1]);

    // store
    _mm_store_si128((__m128i *)(dst + 0 * stride), _mm_loadu_si128((const __m128i *)(border + 15)));
    _mm_store_si128((__m128i *)(dst + 1 * stride), _mm_loadu_si128((const __m128i *)(border + 14)));
    _mm_store_si128((__m128i *)(dst + 2 * stride), _mm_loadu_si128((const __m128i *)(border + 13)));
    _mm_store_si128((__m128i *)(dst + 3 * stride), _mm_loadu_si128((const __m128i *)(border + 12)));
    _mm_store_si128((__m128i *)(dst + 4 * stride), _mm_loadu_si128((const __m128i *)(border + 11)));
    _mm_store_si128((__m128i *)(dst + 5 * stride), _mm_loadu_si128((const __m128i *)(border + 10)));
    _mm_store_si128((__m128i *)(dst + 6 * stride), _mm_loadu_si128((const __m128i *)(border + 9)));
    _mm_store_si128((__m128i *)(dst + 7 * stride), _mm_loadu_si128((const __m128i *)(border + 8)));
    _mm_store_si128((__m128i *)(dst + 8 * stride), _mm_loadu_si128((const __m128i *)(border + 7)));
    _mm_store_si128((__m128i *)(dst + 9 * stride), _mm_loadu_si128((const __m128i *)(border + 6)));
    _mm_store_si128((__m128i *)(dst + 10 * stride), _mm_loadu_si128((const __m128i *)(border + 5)));
    _mm_store_si128((__m128i *)(dst + 11 * stride), _mm_loadu_si128((const __m128i *)(border + 4)));
    _mm_store_si128((__m128i *)(dst + 12 * stride), _mm_loadu_si128((const __m128i *)(border + 3)));
    _mm_store_si128((__m128i *)(dst + 13 * stride), _mm_loadu_si128((const __m128i *)(border + 2)));
    _mm_store_si128((__m128i *)(dst + 14 * stride), _mm_loadu_si128((const __m128i *)(border + 1)));
    _mm_store_si128((__m128i *)(dst + 15 * stride), _mm_load_si128((const __m128i *)(border + 0)));
}
