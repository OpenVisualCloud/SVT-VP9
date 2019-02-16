/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <assert.h>
#include "immintrin.h"

#include "vp9_rtcd.h"
#include "vpx_dsp_rtcd.h"
#include "txfm_common.h"
#include "EbMemory_AVX2.h"
#include "EbTransforms_AVX2.h"
#include "EbTranspose_AVX2.h"
#include "fwd_txfm_sse2.h"
#include "txfm_common_sse2.h"

#if DCT_HIGH_BIT_DEPTH
#define ADD_EPI16 _mm_adds_epi16
#define SUB_EPI16 _mm_subs_epi16
#define ADD_EPI16_AVX2 _mm256_adds_epi16
#define SUB_EPI16_AVX2 _mm256_subs_epi16
#else
#define ADD_EPI16 _mm_add_epi16
#define SUB_EPI16 _mm_sub_epi16
#define ADD_EPI16_AVX2 _mm256_add_epi16
#define SUB_EPI16_AVX2 _mm256_sub_epi16
#endif

static INLINE int check_epi16_overflow_x1_avx2(const __m256i reg) {
    const __m256i max_overflow = _mm256_set1_epi16(0x7fff);
    const __m256i min_overflow = _mm256_set1_epi16((short)0x8000);
    const __m256i cmp = _mm256_or_si256(_mm256_cmpeq_epi16(reg, max_overflow),
        _mm256_cmpeq_epi16(reg, min_overflow));
    return _mm256_movemask_epi8(cmp);
}

static INLINE int check_epi16_overflow_x2_avx2(const __m256i *const preg) {
    const int res0 = check_epi16_overflow_x1_avx2(preg[0]);
    const int res1 = check_epi16_overflow_x1_avx2(preg[1]);
    return res0 + res1;
}

static INLINE int check_epi16_overflow_x4_avx2(const __m256i *const preg) {
    const int res0 = check_epi16_overflow_x2_avx2(preg + 0);
    const int res1 = check_epi16_overflow_x2_avx2(preg + 4);
    return res0 + res1;
}

static INLINE int check_epi16_overflow_x8_avx2(const __m256i *const preg) {
    const int res0 = check_epi16_overflow_x4_avx2(preg + 0);
    const int res1 = check_epi16_overflow_x4_avx2(preg + 4);
    return res0 + res1;
}

static INLINE void transpose_16bit_fdct_8x8_avx2(const __m256i *const in,
    __m256i *const out)
{
    const __m256i idx_transform0 = _mm256_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7);
    const __m256i idx_transform1 = _mm256_setr_epi32(2, 6, 3, 7, 0, 4, 1, 5);

    // Unpack 16 bit elements. Goes from:
    // in[0]: 00 01 02 03 04 05 06 07  20 21 22 23 24 25 26 27
    // in[1]: 40 41 42 43 44 45 46 47  60 61 62 63 64 65 66 67
    // in[2]: 10 11 12 13 14 15 16 17  30 31 32 33 34 35 36 37
    // in[3]: 50 51 52 53 54 55 56 57  70 71 72 73 74 75 76 77
    // to:
    // a0:    00 10 01 11 02 12 03 13  20 30 21 31 22 32 23 33
    // a1:    40 50 41 51 42 52 43 53  60 70 61 71 62 72 63 73
    // a2:    04 14 05 15 06 16 07 17  24 34 25 35 26 36 27 37
    // a3:    44 54 45 55 46 56 47 57  64 74 65 75 66 76 67 77
    const __m256i a0 = _mm256_unpacklo_epi16(in[0], in[2]);
    const __m256i a1 = _mm256_unpacklo_epi16(in[1], in[3]);
    const __m256i a2 = _mm256_unpackhi_epi16(in[0], in[2]);
    const __m256i a3 = _mm256_unpackhi_epi16(in[1], in[3]);

    // Unpack 32 bit elements resulting in:
    // b0: 00 10 40 50 01 11 41 51  20 30 60 70 21 31 61 71
    // b1: 02 12 42 52 03 13 43 53  22 32 62 72 23 33 63 73
    // b2: 04 14 44 54 05 15 45 55  24 34 64 74 25 35 65 75
    // b3: 06 16 46 56 07 17 47 57  26 36 66 76 27 37 67 77
    const __m256i b0 = _mm256_unpacklo_epi32(a0, a1);
    const __m256i b1 = _mm256_unpackhi_epi32(a0, a1);
    const __m256i b2 = _mm256_unpacklo_epi32(a2, a3);
    const __m256i b3 = _mm256_unpackhi_epi32(a2, a3);

    // Permute 32 bit elements resulting in:
    // out[0]: 00 10 20 30 40 50 60 70  01 11 21 31 41 51 61 71
    // out[1]: 02 12 22 32 42 52 62 72  03 13 23 33 43 53 63 73
    // out[2]: 05 15 25 35 45 55 65 75  04 14 24 34 44 54 64 74
    // out[3]: 07 17 27 37 47 57 67 77  06 16 26 36 46 56 66 76
    out[0] = _mm256_permutevar8x32_epi32(b0, idx_transform0);
    out[1] = _mm256_permutevar8x32_epi32(b1, idx_transform0);
    out[2] = _mm256_permutevar8x32_epi32(b2, idx_transform1);
    out[3] = _mm256_permutevar8x32_epi32(b3, idx_transform1);
}

static INLINE void store_output_8x2_avx2(const __m256i output,
    tran_low_t *const dst_ptr, const int stride)
{
#if CONFIG_VP9_HIGHBITDEPTH
    const __m256i sign_bits = _mm256_srai_epi16(output, 15);
    const __m256i out = _mm256_permute4x64_epi64(output, 0xD8);
    const __m256i out0 = _mm256_unpacklo_epi16(out, sign_bits);
    const __m256i out1 = _mm256_unpackhi_epi16(out, sign_bits);
    _mm256_store_si256((__m256i *)(dst_ptr + 0 * stride), out0);
    _mm256_store_si256((__m256i *)(dst_ptr + 1 * stride), out1);
#else
    store16bit_signed_8x2_avx2(output, dst_ptr, stride);
#endif  // CONFIG_VP9_HIGHBITDEPTH
}

static INLINE void store_output_avx2(const __m256i output,
    tran_low_t *const dst_ptr)
{
#if CONFIG_VP9_HIGHBITDEPTH
    const __m256i sign_bits = _mm256_srai_epi16(output, 15);
    const __m256i out = _mm256_permute4x64_epi64(output, 0xD8);
    const __m256i out0 = _mm256_unpacklo_epi16(out, sign_bits);
    const __m256i out1 = _mm256_unpackhi_epi16(out, sign_bits);
    _mm256_store_si256((__m256i *)(dst_ptr + 0), out0);
    _mm256_store_si256((__m256i *)(dst_ptr + 8), out1);
#else
    _mm256_store_si256((__m256i *)dst_ptr, output);
#endif  // CONFIG_VP9_HIGHBITDEPTH
}

 // load 8x8 array
static INLINE void load_buffer_left_shift2_fdct_8x8_avx2(
    const int16_t *const input, const int stride, __m256i *const in)
{
    in[0] = load16bit_signed_8x2_avx2(input + 0 * stride, stride);
    in[1] = load16bit_signed_8x2_avx2(input + 2 * stride, stride);
    in[2] = load16bit_signed_8x2_rev_avx2(input + 4 * stride, stride);
    in[3] = load16bit_signed_8x2_rev_avx2(input + 6 * stride, stride);

    in[0] = _mm256_slli_epi16(in[0], 2);
    in[1] = _mm256_slli_epi16(in[1], 2);
    in[2] = _mm256_slli_epi16(in[2], 2);
    in[3] = _mm256_slli_epi16(in[3], 2);
}

// load 8x8 array
static INLINE void load_buffer_left_shift2_fadst_8x8(
    const int16_t *const input, const int stride, __m256i *const in)
{
    __m128i in128[8];

    in128[0] = _mm_load_si128((const __m128i *)(input + 0 * stride));
    in128[1] = _mm_load_si128((const __m128i *)(input + 1 * stride));
    in128[2] = _mm_load_si128((const __m128i *)(input + 2 * stride));
    in128[3] = _mm_load_si128((const __m128i *)(input + 3 * stride));
    in128[4] = _mm_load_si128((const __m128i *)(input + 4 * stride));
    in128[5] = _mm_load_si128((const __m128i *)(input + 5 * stride));
    in128[6] = _mm_load_si128((const __m128i *)(input + 6 * stride));
    in128[7] = _mm_load_si128((const __m128i *)(input + 7 * stride));

    in[0] = _mm256_setr_m128i(in128[0], in128[4]);
    in[1] = _mm256_setr_m128i(in128[2], in128[6]);
    in[2] = _mm256_setr_m128i(in128[5], in128[1]);
    in[3] = _mm256_setr_m128i(in128[7], in128[3]);

    in[0] = _mm256_slli_epi16(in[0], 2);
    in[1] = _mm256_slli_epi16(in[1], 2);
    in[2] = _mm256_slli_epi16(in[2], 2);
    in[3] = _mm256_slli_epi16(in[3], 2);
}

static INLINE void add_sub_8x4x2_avx2(const __m256i *const in,
    __m256i *const out256)
{
    const __m256i in1 = _mm256_permute4x64_epi64(in[1], 0x4E);
    out256[0] = _mm256_add_epi16(in[0], in1);
    out256[1] = _mm256_sub_epi16(in[0], in1);
}

static INLINE void add_sub_8x8x2_avx2(const __m256i *const in,
    __m256i *const out256)
{
    out256[0] = _mm256_add_epi16(in[0], in[3]);
    out256[1] = _mm256_add_epi16(in[1], in[2]);
    out256[2] = _mm256_sub_epi16(in[1], in[2]);
    out256[3] = _mm256_sub_epi16(in[0], in[3]);
}

static INLINE void highbd_add_sub_8x4x2_avx2(const __m256i *const in,
    __m256i *const out256)
{
    const __m256i in0 = _mm256_permute4x64_epi64(in[1], 0x4E);
    out256[0] = ADD_EPI16_AVX2(in[0], in0);
    out256[1] = SUB_EPI16_AVX2(in[0], in0);
}

static INLINE void highbd_add_sub_8x8x2_avx2(const __m256i *const in,
    __m256i *const out256)
{
    out256[0] = ADD_EPI16_AVX2(in[0], in[3]);
    out256[1] = ADD_EPI16_AVX2(in[1], in[2]);
    out256[2] = SUB_EPI16_AVX2(in[1], in[2]);
    out256[3] = SUB_EPI16_AVX2(in[0], in[3]);
}

static INLINE __m256i fdct8_kernel_avx2(const __m128i in0, const __m128i in1,
    const __m128i c0, const __m128i c1)
{
    const __m128i k__DCT_CONST_ROUNDING = _mm_set1_epi32(DCT_CONST_ROUNDING);
    __m128i u[4], v[4];

    u[0] = _mm_unpacklo_epi16(in0, in1);
    u[1] = _mm_unpackhi_epi16(in0, in1);
    v[0] = _mm_madd_epi16(u[0], c0);
    v[1] = _mm_madd_epi16(u[1], c0);
    v[2] = _mm_madd_epi16(u[0], c1);
    v[3] = _mm_madd_epi16(u[1], c1);
    u[0] = _mm_add_epi32(v[0], k__DCT_CONST_ROUNDING);
    u[1] = _mm_add_epi32(v[1], k__DCT_CONST_ROUNDING);
    u[2] = _mm_add_epi32(v[2], k__DCT_CONST_ROUNDING);
    u[3] = _mm_add_epi32(v[3], k__DCT_CONST_ROUNDING);
    v[0] = _mm_srai_epi32(u[0], DCT_CONST_BITS);
    v[1] = _mm_srai_epi32(u[1], DCT_CONST_BITS);
    v[2] = _mm_srai_epi32(u[2], DCT_CONST_BITS);
    v[3] = _mm_srai_epi32(u[3], DCT_CONST_BITS);

    u[0] = _mm_packs_epi32(v[0], v[1]);
    u[1] = _mm_packs_epi32(v[2], v[3]);
    return _mm256_setr_m128i(u[1], u[0]);
}

static INLINE void dual_fdct8_kernel_avx2(const __m256i in0, const __m256i in1,
    const __m256i c0, const __m256i c1, __m256i *const out0,
    __m256i *const out1)
{
    const __m256i k__DCT_CONST_ROUNDING = _mm256_set1_epi32(DCT_CONST_ROUNDING);
    const __m256i in_lo = _mm256_inserti128_si256(in0, _mm256_extracti128_si256(in1, 0), 1);
    const __m256i in_hi = _mm256_inserti128_si256(in1, _mm256_extracti128_si256(in0, 1), 0);
    __m256i u[4], v[4];

    u[0] = _mm256_unpacklo_epi16(in_lo, in_hi);
    u[1] = _mm256_unpackhi_epi16(in_lo, in_hi);
    v[0] = _mm256_madd_epi16(u[0], c0);
    v[1] = _mm256_madd_epi16(u[1], c0);
    v[2] = _mm256_madd_epi16(u[0], c1);
    v[3] = _mm256_madd_epi16(u[1], c1);
    u[0] = _mm256_add_epi32(v[0], k__DCT_CONST_ROUNDING);
    u[1] = _mm256_add_epi32(v[1], k__DCT_CONST_ROUNDING);
    u[2] = _mm256_add_epi32(v[2], k__DCT_CONST_ROUNDING);
    u[3] = _mm256_add_epi32(v[3], k__DCT_CONST_ROUNDING);
    v[0] = _mm256_srai_epi32(u[0], DCT_CONST_BITS);
    v[1] = _mm256_srai_epi32(u[1], DCT_CONST_BITS);
    v[2] = _mm256_srai_epi32(u[2], DCT_CONST_BITS);
    v[3] = _mm256_srai_epi32(u[3], DCT_CONST_BITS);

    *out0 = _mm256_packs_epi32(v[0], v[1]);
    *out1 = _mm256_packs_epi32(v[2], v[3]);
}

static void fdct8_avx2(__m256i *const in) {
    // constants
    const __m128i k__cospi_p16_p16 = _mm_set1_epi16(cospi_16_64);
    const __m128i k__cospi_p16_m16 = pair_set_epi16(cospi_16_64, -cospi_16_64);
    const __m128i k__cospi_p08_p24 = pair_set_epi16(cospi_8_64, cospi_24_64);
    const __m128i k__cospi_p24_m08 = pair_set_epi16(cospi_24_64, -cospi_8_64);
    const __m128i k__cospi_p04_p28 = pair_set_epi16(cospi_4_64, cospi_28_64);
    const __m128i k__cospi_p28_m04 = pair_set_epi16(cospi_28_64, -cospi_4_64);
    const __m128i k__cospi_p20_p12 = pair_set_epi16(cospi_20_64, cospi_12_64);
    const __m128i k__cospi_p12_m20 = pair_set_epi16(cospi_12_64, -cospi_20_64);
    const __m256i k__cospi_p16_p16_p08_p24 = _mm256_setr_m128i(k__cospi_p16_p16, k__cospi_p08_p24);
    const __m256i k__cospi_p16_m16_p24_m08 = _mm256_setr_m128i(k__cospi_p16_m16, k__cospi_p24_m08);
    const __m256i k__cospi_p04_p28_p12_m20 = _mm256_setr_m128i(k__cospi_p04_p28, k__cospi_p12_m20);
    const __m256i k__cospi_p28_m04_p20_p12 = _mm256_setr_m128i(k__cospi_p28_m04, k__cospi_p20_p12);
    __m256i s[4], x[2], t;
    __m128i s5, s6;

    // stage 1
    add_sub_8x8x2_avx2(in, s);
    add_sub_8x4x2_avx2(s, x);
    dual_fdct8_kernel_avx2(x[0], x[1], k__cospi_p16_p16_p08_p24, k__cospi_p16_m16_p24_m08, &in[0], &in[1]);

    // stage 2
    s5 = _mm256_extracti128_si256(s[2], 0);
    s6 = _mm256_extracti128_si256(s[3], 1);
    t = fdct8_kernel_avx2(s6, s5, k__cospi_p16_m16, k__cospi_p16_p16);

    // stage 3
    s[2] = _mm256_inserti128_si256(s[3], _mm256_extracti128_si256(s[2], 1), 1);
    x[0] = _mm256_add_epi16(s[2], t);
    x[1] = _mm256_sub_epi16(s[2], t);

    // stage 4
    dual_fdct8_kernel_avx2(x[0], x[1], k__cospi_p04_p28_p12_m20, k__cospi_p28_m04_p20_p12, &in[2], &in[3]);
    in[3] = _mm256_permute4x64_epi64(in[3], 0x4E);

    // transpose
    transpose_16bit_fdct_8x8_avx2(in, in);
}

static int fdct8_overflow_avx2(__m256i *const in, const int pass)
{
    // constants
    const __m128i k__cospi_p16_p16 = _mm_set1_epi16(cospi_16_64);
    const __m128i k__cospi_p16_m16 = pair_set_epi16(cospi_16_64, -cospi_16_64);
    const __m128i k__cospi_p08_p24 = pair_set_epi16(cospi_8_64, cospi_24_64);
    const __m128i k__cospi_p24_m08 = pair_set_epi16(cospi_24_64, -cospi_8_64);
    const __m128i k__cospi_p04_p28 = pair_set_epi16(cospi_4_64, cospi_28_64);
    const __m128i k__cospi_p28_m04 = pair_set_epi16(cospi_28_64, -cospi_4_64);
    const __m128i k__cospi_p20_p12 = pair_set_epi16(cospi_20_64, cospi_12_64);
    const __m128i k__cospi_p12_m20 = pair_set_epi16(cospi_12_64, -cospi_20_64);
    const __m256i k__cospi_p16_p16_p08_p24 = _mm256_setr_m128i(k__cospi_p16_p16, k__cospi_p08_p24);
    const __m256i k__cospi_p16_m16_p24_m08 = _mm256_setr_m128i(k__cospi_p16_m16, k__cospi_p24_m08);
    const __m256i k__cospi_p04_p28_p12_m20 = _mm256_setr_m128i(k__cospi_p04_p28, k__cospi_p12_m20);
    const __m256i k__cospi_p28_m04_p20_p12 = _mm256_setr_m128i(k__cospi_p28_m04, k__cospi_p20_p12);
    int overflow = 0;
    __m256i s[4], x[2], t;
    __m128i s5, s6;

    // stage 1
    highbd_add_sub_8x8x2_avx2(in, s);
    highbd_add_sub_8x4x2_avx2(s, x);

#if DCT_HIGH_BIT_DEPTH
    overflow |= (pass == 1) && check_epi16_overflow_x4_avx2(s);
    overflow |= check_epi16_overflow_x2_avx2(x);
#else
    (void)pass;
#endif  // DCT_HIGH_BIT_DEPTH

    dual_fdct8_kernel_avx2(x[0], x[1], k__cospi_p16_p16_p08_p24, k__cospi_p16_m16_p24_m08, &in[0], &in[1]);

    // stage 2
    s5 = _mm256_extracti128_si256(s[2], 0);
    s6 = _mm256_extracti128_si256(s[3], 1);
    t = fdct8_kernel_avx2(s6, s5, k__cospi_p16_m16, k__cospi_p16_p16);

    // stage 3
    s[2] = _mm256_inserti128_si256(s[3], _mm256_extracti128_si256(s[2], 1), 1);
    x[0] = ADD_EPI16_AVX2(s[2], t);
    x[1] = SUB_EPI16_AVX2(s[2], t);

    // stage 4
    dual_fdct8_kernel_avx2(x[0], x[1], k__cospi_p04_p28_p12_m20, k__cospi_p28_m04_p20_p12, &in[2], &in[3]);
    in[3] = _mm256_permute4x64_epi64(in[3], 0x4E);

#if DCT_HIGH_BIT_DEPTH
    overflow |= check_epi16_overflow_x4_avx2(in);
    overflow |= check_epi16_overflow_x1_avx2(t);
    overflow |= check_epi16_overflow_x2_avx2(x);
#endif  // DCT_HIGH_BIT_DEPTH

    // transpose
    transpose_16bit_fdct_8x8_avx2(in, in);

    return overflow;
}

static INLINE __m256i fadst8_core_avx2(const __m256i *const in, const __m256i k)
{
    const __m256i k__DCT_CONST_ROUNDING = _mm256_set1_epi32(DCT_CONST_ROUNDING);
    __m256i u[2], v[2];

    u[0] = _mm256_madd_epi16(in[0], k);
    u[1] = _mm256_madd_epi16(in[1], k);

    v[0] = _mm256_inserti128_si256(u[0], _mm256_extracti128_si256(u[1], 0), 1);
    v[1] = _mm256_inserti128_si256(u[1], _mm256_extracti128_si256(u[0], 1), 0);

    // addition
    u[0] = _mm256_add_epi32(v[0], v[1]);
    u[1] = _mm256_sub_epi32(v[0], v[1]);

    // shift and rounding
    u[0] = _mm256_add_epi32(u[0], k__DCT_CONST_ROUNDING);
    u[1] = _mm256_add_epi32(u[1], k__DCT_CONST_ROUNDING);

    u[0] = _mm256_srai_epi32(u[0], DCT_CONST_BITS);
    u[1] = _mm256_srai_epi32(u[1], DCT_CONST_BITS);

    // back to 16-bit and pack 8 integers into __m256i
    u[0] = _mm256_packs_epi32(u[0], u[1]);
    return _mm256_permute4x64_epi64(u[0], 0xD8);
}

static INLINE void fadst8_kernel_avx2(const __m256i in0, const __m256i in1,
    const tran_coef_t c0, const tran_coef_t c1, const tran_coef_t c2,
    const tran_coef_t c3, __m256i *const out0, __m256i *const out1)
{
    const __m128i k0 = pair_set_epi16(c0, c1);
    const __m128i k1 = pair_set_epi16(c1, -c0);
    const __m128i k2 = pair_set_epi16(c2, c3);
    const __m128i k3 = pair_set_epi16(c3, -c2);
    const __m256i cst0 = _mm256_setr_m128i(k0, k2);
    const __m256i cst1 = _mm256_setr_m128i(k1, k3);
    __m256i s[2];

    s[0] = _mm256_unpacklo_epi16(in0, in1); // 0 2
    s[1] = _mm256_unpackhi_epi16(in0, in1); // 1 3

    *out0 = fadst8_core_avx2(s, cst0);
    *out1 = fadst8_core_avx2(s, cst1);
}

static void fadst8_avx2(__m256i *const in) {
    // Constants
    const __m256i k__const_0 = _mm256_set1_epi16(0);
    const __m128i k__cospi_p16_p16 = _mm_set1_epi16(cospi_16_64);
    const __m128i k__cospi_p16_m16 = pair_set_epi16(cospi_16_64, -cospi_16_64);
    const __m256i k__cospi_p16_p16_p16_p16 = _mm256_setr_m128i(k__cospi_p16_p16, k__cospi_p16_p16);
    const __m256i k__cospi_p16_m16_p16_m16 = _mm256_setr_m128i(k__cospi_p16_m16, k__cospi_p16_m16);
    __m256i x[4];

    // stage 1
    fadst8_kernel_avx2(in[3], in[0], cospi_2_64, cospi_30_64, cospi_18_64, cospi_14_64, &x[0], &x[1]);
    fadst8_kernel_avx2(in[2], in[1], cospi_10_64, cospi_22_64, cospi_26_64, cospi_6_64, &x[2], &x[3]);

    // stage 2
    in[0] = _mm256_inserti128_si256(x[0], _mm256_extracti128_si256(x[1], 0), 1);
    in[1] = _mm256_inserti128_si256(x[2], _mm256_extracti128_si256(x[3], 0), 1);
    in[2] = _mm256_inserti128_si256(x[2], _mm256_extracti128_si256(x[0], 1), 0);
    in[3] = _mm256_inserti128_si256(x[3], _mm256_extracti128_si256(x[1], 1), 0);
    x[0] = _mm256_add_epi16(in[0], in[1]);
    x[1] = _mm256_sub_epi16(in[0], in[1]);

    fadst8_kernel_avx2(in[2], in[3], cospi_8_64, cospi_24_64, -cospi_24_64, cospi_8_64, &x[2], &in[3]);

    // stage 3
    x[3] = _mm256_inserti128_si256(in[3], _mm256_extracti128_si256(x[2], 1), 0);
    dual_fdct8_kernel_avx2(x[1], x[3], k__cospi_p16_p16_p16_p16, k__cospi_p16_m16_p16_m16, &x[1], &x[3]);

    in[0] = _mm256_inserti128_si256(x[0], _mm256_extracti128_si256(x[1], 1), 1);
    in[1] = _mm256_inserti128_si256(x[3], _mm256_extracti128_si256(in[3], 0), 1);
    in[2] = _mm256_inserti128_si256(x[2], _mm256_extracti128_si256(x[1], 0), 1);
    in[3] = _mm256_inserti128_si256(x[0], _mm256_extracti128_si256(x[3], 1), 0);
    in[2] = _mm256_sub_epi16(k__const_0, in[2]);
    in[3] = _mm256_sub_epi16(k__const_0, in[3]);

    // transpose
    transpose_16bit_fdct_8x8_avx2(in, in);
}

static INLINE void right_shift_write_buffer_8x8(const __m256i *const in,
    tran_low_t *const output)
{
    __m256i sign[4], res[4];
    __m128i out[8];

    // right shift and rounding
    sign[0] = _mm256_srai_epi16(in[0], 15);
    sign[1] = _mm256_srai_epi16(in[1], 15);
    sign[2] = _mm256_srai_epi16(in[2], 15);
    sign[3] = _mm256_srai_epi16(in[3], 15);

    res[0] = _mm256_sub_epi16(in[0], sign[0]);
    res[1] = _mm256_sub_epi16(in[1], sign[1]);
    res[2] = _mm256_sub_epi16(in[2], sign[2]);
    res[3] = _mm256_sub_epi16(in[3], sign[3]);

    res[0] = _mm256_srai_epi16(res[0], 1);
    res[1] = _mm256_srai_epi16(res[1], 1);
    res[2] = _mm256_srai_epi16(res[2], 1);
    res[3] = _mm256_srai_epi16(res[3], 1);

    // write 8x8 array
    out[0] = _mm256_extracti128_si256(res[0], 0);
    out[1] = _mm256_extracti128_si256(res[0], 1);
    out[2] = _mm256_extracti128_si256(res[1], 0);
    out[3] = _mm256_extracti128_si256(res[1], 1);
    out[4] = _mm256_extracti128_si256(res[2], 1);
    out[5] = _mm256_extracti128_si256(res[2], 0);
    out[6] = _mm256_extracti128_si256(res[3], 1);
    out[7] = _mm256_extracti128_si256(res[3], 0);

    store_output(out[0], (output + 0 * 8));
    store_output(out[1], (output + 1 * 8));
    store_output(out[2], (output + 2 * 8));
    store_output(out[3], (output + 3 * 8));
    store_output(out[4], (output + 4 * 8));
    store_output(out[5], (output + 5 * 8));
    store_output(out[6], (output + 6 * 8));
    store_output(out[7], (output + 7 * 8));
}

void vpx_fdct8x8_avx2(const int16_t *input, tran_low_t *output, int stride) {
    int overflow;
    __m256i in[4];

    load_buffer_left_shift2_fdct_8x8_avx2(input, stride, in);
    overflow = fdct8_overflow_avx2(in, 0);
    overflow |= fdct8_overflow_avx2(in, 1);
    right_shift_write_buffer_8x8(in, output);

#if DCT_HIGH_BIT_DEPTH
    if (overflow) {
        vpx_highbd_fdct8x8_c(input, output, stride);
    }
#else
    (void)overflow;
#endif  // DCT_HIGH_BIT_DEPTH
}

void vp9_fht8x8_avx2(const int16_t *input, tran_low_t *output, int stride,
    int tx_type) {
    int overflow = 0;
    __m256i in[4], x[4];

    switch (tx_type) {
    case DCT_DCT:
        load_buffer_left_shift2_fdct_8x8_avx2(input, stride, in);
        overflow = fdct8_overflow_avx2(in, 0);
        overflow |= fdct8_overflow_avx2(in, 1);
        break;

    case ADST_DCT:
        load_buffer_left_shift2_fadst_8x8(input, stride, in);
        fadst8_avx2(in);
        fdct8_avx2(in);
        break;

    case DCT_ADST:
        load_buffer_left_shift2_fdct_8x8_avx2(input, stride, x);
        fdct8_avx2(x);
        in[0] = _mm256_inserti128_si256(x[0], _mm256_extracti128_si256(x[2], 1), 1);
        in[1] = _mm256_inserti128_si256(x[1], _mm256_extracti128_si256(x[3], 1), 1);
        in[2] = _mm256_inserti128_si256(x[2], _mm256_extracti128_si256(x[0], 1), 1);
        in[3] = _mm256_inserti128_si256(x[3], _mm256_extracti128_si256(x[1], 1), 1);
        fadst8_avx2(in);
        break;

    default:
        assert(tx_type == ADST_ADST);
        load_buffer_left_shift2_fadst_8x8(input, stride, x);
        fadst8_avx2(x);
        in[0] = _mm256_inserti128_si256(x[0], _mm256_extracti128_si256(x[2], 1), 1);
        in[1] = _mm256_inserti128_si256(x[1], _mm256_extracti128_si256(x[3], 1), 1);
        in[2] = _mm256_inserti128_si256(x[0], _mm256_extracti128_si256(x[2], 0), 0);
        in[3] = _mm256_inserti128_si256(x[1], _mm256_extracti128_si256(x[3], 0), 0);
        fadst8_avx2(in);
        break;
    }

    right_shift_write_buffer_8x8(in, output);

#if DCT_HIGH_BIT_DEPTH
    if (overflow) {
        vpx_highbd_fdct8x8_c(input, output, stride);
    }
#else
    (void)overflow;
#endif  // DCT_HIGH_BIT_DEPTH
}

//------------------------------------------------------------------------------

 // load 16x8 array
static INLINE void load_buffer_left_shift2_16x8_avx2(const int16_t *const input,
    const int stride, __m256i *const in)
{
    in[0] = _mm256_loadu_si256((const __m256i *)(input + 0 * stride));
    in[1] = _mm256_loadu_si256((const __m256i *)(input + 1 * stride));
    in[2] = _mm256_loadu_si256((const __m256i *)(input + 2 * stride));
    in[3] = _mm256_loadu_si256((const __m256i *)(input + 3 * stride));
    in[4] = _mm256_loadu_si256((const __m256i *)(input + 4 * stride));
    in[5] = _mm256_loadu_si256((const __m256i *)(input + 5 * stride));
    in[6] = _mm256_loadu_si256((const __m256i *)(input + 6 * stride));
    in[7] = _mm256_loadu_si256((const __m256i *)(input + 7 * stride));

    in[0] = _mm256_slli_epi16(in[0], 2);
    in[1] = _mm256_slli_epi16(in[1], 2);
    in[2] = _mm256_slli_epi16(in[2], 2);
    in[3] = _mm256_slli_epi16(in[3], 2);
    in[4] = _mm256_slli_epi16(in[4], 2);
    in[5] = _mm256_slli_epi16(in[5], 2);
    in[6] = _mm256_slli_epi16(in[6], 2);
    in[7] = _mm256_slli_epi16(in[7], 2);
}

static INLINE void load_buffer_left_shift2_16x16_avx2(
    const int16_t *const input, const int stride, __m256i *const in) {
    load_buffer_left_shift2_16x8_avx2(input + 0 * stride, stride, in + 0);
    load_buffer_left_shift2_16x8_avx2(input + 8 * stride, stride, in + 8);
}

static INLINE void add_sub_16x4_avx2(const __m256i *const in,
    __m256i *const out)
{
    out[0] = _mm256_add_epi16(in[0], in[3]);
    out[1] = _mm256_add_epi16(in[1], in[2]);
    out[2] = _mm256_sub_epi16(in[1], in[2]);
    out[3] = _mm256_sub_epi16(in[0], in[3]);
}

static INLINE void add_sub_16x8_avx2(const __m256i *const in,
    __m256i *const out)
{
    out[0] = _mm256_add_epi16(in[0], in[7]);
    out[1] = _mm256_add_epi16(in[1], in[6]);
    out[2] = _mm256_add_epi16(in[2], in[5]);
    out[3] = _mm256_add_epi16(in[3], in[4]);
    out[4] = _mm256_sub_epi16(in[3], in[4]);
    out[5] = _mm256_sub_epi16(in[2], in[5]);
    out[6] = _mm256_sub_epi16(in[1], in[6]);
    out[7] = _mm256_sub_epi16(in[0], in[7]);
}

static INLINE void highbd_add_sub_16x4_avx2(const __m256i *const in,
    __m256i *const out)
{
    out[0] = ADD_EPI16_AVX2(in[0], in[3]);
    out[1] = ADD_EPI16_AVX2(in[1], in[2]);
    out[2] = SUB_EPI16_AVX2(in[1], in[2]);
    out[3] = SUB_EPI16_AVX2(in[0], in[3]);
}

static INLINE void highbd_add_sub_16x8_avx2(const __m256i *const in,
    __m256i *const out)
{
    out[0] = ADD_EPI16_AVX2(in[0], in[7]);
    out[1] = ADD_EPI16_AVX2(in[1], in[6]);
    out[2] = ADD_EPI16_AVX2(in[2], in[5]);
    out[3] = ADD_EPI16_AVX2(in[3], in[4]);
    out[4] = SUB_EPI16_AVX2(in[3], in[4]);
    out[5] = SUB_EPI16_AVX2(in[2], in[5]);
    out[6] = SUB_EPI16_AVX2(in[1], in[6]);
    out[7] = SUB_EPI16_AVX2(in[0], in[7]);
}

// right shift and rounding
static INLINE void right_shift2_16x8_avx2(__m256i *const in) {
    const __m256i const_rounding = _mm256_set1_epi16(1);

    // x = (x + 1) >> 2
    in[0] = _mm256_add_epi16(in[0], const_rounding);
    in[1] = _mm256_add_epi16(in[1], const_rounding);
    in[2] = _mm256_add_epi16(in[2], const_rounding);
    in[3] = _mm256_add_epi16(in[3], const_rounding);
    in[4] = _mm256_add_epi16(in[4], const_rounding);
    in[5] = _mm256_add_epi16(in[5], const_rounding);
    in[6] = _mm256_add_epi16(in[6], const_rounding);
    in[7] = _mm256_add_epi16(in[7], const_rounding);

    in[0] = _mm256_srai_epi16(in[0], 2);
    in[1] = _mm256_srai_epi16(in[1], 2);
    in[2] = _mm256_srai_epi16(in[2], 2);
    in[3] = _mm256_srai_epi16(in[3], 2);
    in[4] = _mm256_srai_epi16(in[4], 2);
    in[5] = _mm256_srai_epi16(in[5], 2);
    in[6] = _mm256_srai_epi16(in[6], 2);
    in[7] = _mm256_srai_epi16(in[7], 2);
}

static INLINE void right_shift_16x8_avx2(__m256i *const res) {
    const __m256i const_rounding = _mm256_set1_epi16(1);
    __m256i sign[8];

    sign[0] = _mm256_srai_epi16(res[0], 15);
    sign[1] = _mm256_srai_epi16(res[1], 15);
    sign[2] = _mm256_srai_epi16(res[2], 15);
    sign[3] = _mm256_srai_epi16(res[3], 15);
    sign[4] = _mm256_srai_epi16(res[4], 15);
    sign[5] = _mm256_srai_epi16(res[5], 15);
    sign[6] = _mm256_srai_epi16(res[6], 15);
    sign[7] = _mm256_srai_epi16(res[7], 15);

    res[0] = _mm256_add_epi16(res[0], const_rounding);
    res[1] = _mm256_add_epi16(res[1], const_rounding);
    res[2] = _mm256_add_epi16(res[2], const_rounding);
    res[3] = _mm256_add_epi16(res[3], const_rounding);
    res[4] = _mm256_add_epi16(res[4], const_rounding);
    res[5] = _mm256_add_epi16(res[5], const_rounding);
    res[6] = _mm256_add_epi16(res[6], const_rounding);
    res[7] = _mm256_add_epi16(res[7], const_rounding);

    res[0] = _mm256_sub_epi16(res[0], sign[0]);
    res[1] = _mm256_sub_epi16(res[1], sign[1]);
    res[2] = _mm256_sub_epi16(res[2], sign[2]);
    res[3] = _mm256_sub_epi16(res[3], sign[3]);
    res[4] = _mm256_sub_epi16(res[4], sign[4]);
    res[5] = _mm256_sub_epi16(res[5], sign[5]);
    res[6] = _mm256_sub_epi16(res[6], sign[6]);
    res[7] = _mm256_sub_epi16(res[7], sign[7]);

    res[0] = _mm256_srai_epi16(res[0], 2);
    res[1] = _mm256_srai_epi16(res[1], 2);
    res[2] = _mm256_srai_epi16(res[2], 2);
    res[3] = _mm256_srai_epi16(res[3], 2);
    res[4] = _mm256_srai_epi16(res[4], 2);
    res[5] = _mm256_srai_epi16(res[5], 2);
    res[6] = _mm256_srai_epi16(res[6], 2);
    res[7] = _mm256_srai_epi16(res[7], 2);
}

static INLINE void right_shift_fdct_16x16_avx2(__m256i *const res) {
    right_shift2_16x8_avx2(res + 0);
    right_shift2_16x8_avx2(res + 8);
}

static INLINE void right_shift_16x16_avx2(__m256i *const res) {
    // perform rounding operations
    right_shift_16x8_avx2(res + 0);
    right_shift_16x8_avx2(res + 8);
}

// write 8x8 array
static INLINE void write_buffer_16x8_avx2(const __m256i *const res,
    tran_low_t *const output, const int stride) {
    store_output_avx2(res[0], (output + 0 * stride));
    store_output_avx2(res[1], (output + 1 * stride));
    store_output_avx2(res[2], (output + 2 * stride));
    store_output_avx2(res[3], (output + 3 * stride));
    store_output_avx2(res[4], (output + 4 * stride));
    store_output_avx2(res[5], (output + 5 * stride));
    store_output_avx2(res[6], (output + 6 * stride));
    store_output_avx2(res[7], (output + 7 * stride));
}

static INLINE void write_buffer_16x16_avx2(const __m256i *const res,
    tran_low_t *const output, const int stride) {
    write_buffer_16x8_avx2(res + 0, output + 0 * stride, stride);
    write_buffer_16x8_avx2(res + 8, output + 8 * stride, stride);
}

static INLINE void fadst16_core_avx2(const __m256i *const in, const __m256i c0,
    const __m256i c1, __m256i *const out0, __m256i *const out1)
{
    const __m256i k__DCT_CONST_ROUNDING = _mm256_set1_epi32(DCT_CONST_ROUNDING);
    __m256i u[4], v[4];

    u[0] = _mm256_madd_epi16(in[0], c0);
    u[1] = _mm256_madd_epi16(in[1], c0);
    u[2] = _mm256_madd_epi16(in[2], c1);
    u[3] = _mm256_madd_epi16(in[3], c1);

    // addition
    v[0] = _mm256_add_epi32(u[0], u[2]);
    v[1] = _mm256_add_epi32(u[1], u[3]);
    v[2] = _mm256_sub_epi32(u[0], u[2]);
    v[3] = _mm256_sub_epi32(u[1], u[3]);

    // shift and rounding
    v[0] = _mm256_add_epi32(v[0], k__DCT_CONST_ROUNDING);
    v[1] = _mm256_add_epi32(v[1], k__DCT_CONST_ROUNDING);
    v[2] = _mm256_add_epi32(v[2], k__DCT_CONST_ROUNDING);
    v[3] = _mm256_add_epi32(v[3], k__DCT_CONST_ROUNDING);

    v[0] = _mm256_srai_epi32(v[0], DCT_CONST_BITS);
    v[1] = _mm256_srai_epi32(v[1], DCT_CONST_BITS);
    v[2] = _mm256_srai_epi32(v[2], DCT_CONST_BITS);
    v[3] = _mm256_srai_epi32(v[3], DCT_CONST_BITS);

    // back to 16-bit and pack 8 integers into __m256i
    *out0 = _mm256_packs_epi32(v[0], v[1]);
    *out1 = _mm256_packs_epi32(v[2], v[3]);
}

static INLINE void fadst16_kernel_avx2(const __m256i in0, const __m256i in1,
    const __m256i in2, const __m256i in3, const tran_coef_t c0,
    const tran_coef_t c1, const tran_coef_t c2, const tran_coef_t c3,
    __m256i *const out0, __m256i *const out1, __m256i *const out2,
    __m256i *const out3)
{
    const __m256i cst0 = pair_set_epi16_avx2(c0, c1);
    const __m256i cst1 = pair_set_epi16_avx2(c1, -c0);
    const __m256i cst2 = pair_set_epi16_avx2(c2, c3);
    const __m256i cst3 = pair_set_epi16_avx2(c3, -c2);
    __m256i s[4];

    s[0] = _mm256_unpacklo_epi16(in0, in1);
    s[1] = _mm256_unpackhi_epi16(in0, in1);
    s[2] = _mm256_unpacklo_epi16(in2, in3);
    s[3] = _mm256_unpackhi_epi16(in2, in3);

    fadst16_core_avx2(s, cst0, cst2, out0, out1);
    fadst16_core_avx2(s, cst1, cst3, out2, out3);
}

static void fdct16_avx2(__m256i *const in) {
    __m256i in_high[8], step1[8], step2[8], step3[8], s[8], t[4], x[4];

    // Calculate input for the first 8 results.
    in_high[0] = _mm256_add_epi16(in[0], in[15]);
    in_high[1] = _mm256_add_epi16(in[1], in[14]);
    in_high[2] = _mm256_add_epi16(in[2], in[13]);
    in_high[3] = _mm256_add_epi16(in[3], in[12]);
    in_high[4] = _mm256_add_epi16(in[4], in[11]);
    in_high[5] = _mm256_add_epi16(in[5], in[10]);
    in_high[6] = _mm256_add_epi16(in[6], in[9]);
    in_high[7] = _mm256_add_epi16(in[7], in[8]);

    // Calculate input for the next 8 results.
    step1[0] = _mm256_sub_epi16(in[7], in[8]);
    step1[1] = _mm256_sub_epi16(in[6], in[9]);
    step1[2] = _mm256_sub_epi16(in[5], in[10]);
    step1[3] = _mm256_sub_epi16(in[4], in[11]);
    step1[4] = _mm256_sub_epi16(in[3], in[12]);
    step1[5] = _mm256_sub_epi16(in[2], in[13]);
    step1[6] = _mm256_sub_epi16(in[1], in[14]);
    step1[7] = _mm256_sub_epi16(in[0], in[15]);

    // Work on the first eight values; fdct8(input, even_results);

    // stage 1
    add_sub_16x8_avx2(in_high, s);
    add_sub_16x4_avx2(s, x);

    butterfly_avx2(x[0], x[1], cospi_16_64, cospi_16_64, &in[8], &in[0]);
    butterfly_avx2(x[2], x[3], cospi_24_64, -cospi_8_64, &in[4], &in[12]);

    // stage 2
    butterfly_avx2(s[6], s[5], cospi_16_64, cospi_16_64, &t[2], &t[3]);

    // stage 3
    x[0] = _mm256_add_epi16(s[4], t[2]);
    x[1] = _mm256_sub_epi16(s[4], t[2]);
    x[2] = _mm256_sub_epi16(s[7], t[3]);
    x[3] = _mm256_add_epi16(s[7], t[3]);

    // stage 4
    butterfly_avx2(x[3], x[0], cospi_28_64, cospi_4_64, &in[14], &in[2]);
    butterfly_avx2(x[2], x[1], cospi_12_64, cospi_20_64, &in[6], &in[10]);

    // Work on the next eight values; step1 -> odd_results

    // step 2
    butterfly_avx2(step1[5], step1[2], cospi_16_64, cospi_16_64, &step2[2], &step2[5]);
    butterfly_avx2(step1[4], step1[3], cospi_16_64, cospi_16_64, &step2[3], &step2[4]);

    // step 3
    step3[0] = _mm256_add_epi16(step1[0], step2[3]);
    step3[1] = _mm256_add_epi16(step1[1], step2[2]);
    step3[2] = _mm256_sub_epi16(step1[1], step2[2]);
    step3[3] = _mm256_sub_epi16(step1[0], step2[3]);
    step3[4] = _mm256_sub_epi16(step1[7], step2[4]);
    step3[5] = _mm256_sub_epi16(step1[6], step2[5]);
    step3[6] = _mm256_add_epi16(step1[6], step2[5]);
    step3[7] = _mm256_add_epi16(step1[7], step2[4]);

    // step 4
    butterfly_avx2(step3[1], step3[6], cospi_24_64, -cospi_8_64, &step2[6], &step2[1]);
    butterfly_avx2(step3[2], step3[5], cospi_8_64, cospi_24_64, &step2[5], &step2[2]);

    // step 5
    step1[0] = _mm256_add_epi16(step3[0], step2[1]);
    step1[1] = _mm256_sub_epi16(step3[0], step2[1]);
    step1[2] = _mm256_add_epi16(step3[3], step2[2]);
    step1[3] = _mm256_sub_epi16(step3[3], step2[2]);
    step1[4] = _mm256_sub_epi16(step3[4], step2[5]);
    step1[5] = _mm256_add_epi16(step3[4], step2[5]);
    step1[6] = _mm256_sub_epi16(step3[7], step2[6]);
    step1[7] = _mm256_add_epi16(step3[7], step2[6]);

    // step 6
    butterfly_avx2(step1[0], step1[7], cospi_30_64, -cospi_2_64, &in[1], &in[15]);
    butterfly_avx2(step1[1], step1[6], cospi_14_64, -cospi_18_64, &in[9], &in[7]);
    butterfly_avx2(step1[2], step1[5], cospi_22_64, -cospi_10_64, &in[5], &in[11]);
    butterfly_avx2(step1[3], step1[4], cospi_6_64, -cospi_26_64, &in[13], &in[3]);

    // transpose
    transpose_16bit_16x16_avx2(in, in);
}

static int fdct16_overflow_avx2(__m256i *const in) {
    int overflow = 0;
    __m256i in_high[8], step1[8], step2[8], step3[8], s[8], t[4], x[4];

    // Calculate input for the first 8 results.
    in_high[0] = ADD_EPI16_AVX2(in[0], in[15]);
    in_high[1] = ADD_EPI16_AVX2(in[1], in[14]);
    in_high[2] = ADD_EPI16_AVX2(in[2], in[13]);
    in_high[3] = ADD_EPI16_AVX2(in[3], in[12]);
    in_high[4] = ADD_EPI16_AVX2(in[4], in[11]);
    in_high[5] = ADD_EPI16_AVX2(in[5], in[10]);
    in_high[6] = ADD_EPI16_AVX2(in[6], in[9]);
    in_high[7] = ADD_EPI16_AVX2(in[7], in[8]);

    // Calculate input for the next 8 results.
    step1[0] = SUB_EPI16_AVX2(in[7], in[8]);
    step1[1] = SUB_EPI16_AVX2(in[6], in[9]);
    step1[2] = SUB_EPI16_AVX2(in[5], in[10]);
    step1[3] = SUB_EPI16_AVX2(in[4], in[11]);
    step1[4] = SUB_EPI16_AVX2(in[3], in[12]);
    step1[5] = SUB_EPI16_AVX2(in[2], in[13]);
    step1[6] = SUB_EPI16_AVX2(in[1], in[14]);
    step1[7] = SUB_EPI16_AVX2(in[0], in[15]);

    // Work on the first eight values; fdct8(input, even_results);

    // stage 1
    highbd_add_sub_16x8_avx2(in_high, s);
    highbd_add_sub_16x4_avx2(s, x);

    butterfly_avx2(x[0], x[1], cospi_16_64, cospi_16_64, &in[8], &in[0]);
    butterfly_avx2(x[2], x[3], cospi_24_64, -cospi_8_64, &in[4], &in[12]);

    // stage 2
    butterfly_avx2(s[6], s[5], cospi_16_64, cospi_16_64, &t[2], &t[3]);

#if DCT_HIGH_BIT_DEPTH
    overflow |= check_epi16_overflow_x8_avx2(in_high);
    overflow |= check_epi16_overflow_x8_avx2(step1);
    overflow |= check_epi16_overflow_x8_avx2(s);
    overflow |= check_epi16_overflow_x4_avx2(x);
    overflow |= check_epi16_overflow_x2_avx2(t);
#endif  // DCT_HIGH_BIT_DEPTH

    // stage 3
    x[0] = ADD_EPI16_AVX2(s[4], t[2]);
    x[1] = SUB_EPI16_AVX2(s[4], t[2]);
    x[2] = SUB_EPI16_AVX2(s[7], t[3]);
    x[3] = ADD_EPI16_AVX2(s[7], t[3]);

    // stage 4
    butterfly_avx2(x[3], x[0], cospi_28_64, cospi_4_64, &in[14], &in[2]);
    butterfly_avx2(x[2], x[1], cospi_12_64, cospi_20_64, &in[6], &in[10]);

    // Work on the next eight values; step1 -> odd_results

    // step 2
    butterfly_avx2(step1[5], step1[2], cospi_16_64, cospi_16_64, &step2[2], &step2[5]);
    butterfly_avx2(step1[4], step1[3], cospi_16_64, cospi_16_64, &step2[3], &step2[4]);

    // step 3
    step3[0] = ADD_EPI16_AVX2(step1[0], step2[3]);
    step3[1] = ADD_EPI16_AVX2(step1[1], step2[2]);
    step3[2] = SUB_EPI16_AVX2(step1[1], step2[2]);
    step3[3] = SUB_EPI16_AVX2(step1[0], step2[3]);
    step3[4] = SUB_EPI16_AVX2(step1[7], step2[4]);
    step3[5] = SUB_EPI16_AVX2(step1[6], step2[5]);
    step3[6] = ADD_EPI16_AVX2(step1[6], step2[5]);
    step3[7] = ADD_EPI16_AVX2(step1[7], step2[4]);

    // step 4
    butterfly_avx2(step3[1], step3[6], cospi_24_64, -cospi_8_64, &step2[6], &step2[1]);
    butterfly_avx2(step3[2], step3[5], cospi_8_64, cospi_24_64, &step2[5], &step2[2]);

    // step 5
    step1[0] = ADD_EPI16_AVX2(step3[0], step2[1]);
    step1[1] = SUB_EPI16_AVX2(step3[0], step2[1]);
    step1[2] = ADD_EPI16_AVX2(step3[3], step2[2]);
    step1[3] = SUB_EPI16_AVX2(step3[3], step2[2]);
    step1[4] = SUB_EPI16_AVX2(step3[4], step2[5]);
    step1[5] = ADD_EPI16_AVX2(step3[4], step2[5]);
    step1[6] = SUB_EPI16_AVX2(step3[7], step2[6]);
    step1[7] = ADD_EPI16_AVX2(step3[7], step2[6]);

    // step 6
    butterfly_avx2(step1[0], step1[7], cospi_30_64, -cospi_2_64, &in[1], &in[15]);
    butterfly_avx2(step1[1], step1[6], cospi_14_64, -cospi_18_64, &in[9], &in[7]);
    butterfly_avx2(step1[2], step1[5], cospi_22_64, -cospi_10_64, &in[5], &in[11]);
    butterfly_avx2(step1[3], step1[4], cospi_6_64, -cospi_26_64, &in[13], &in[3]);

#if DCT_HIGH_BIT_DEPTH
    overflow |= check_epi16_overflow_x4_avx2(x);
    overflow |= check_epi16_overflow_x8_avx2(in + 0);
    overflow |= check_epi16_overflow_x8_avx2(in + 8);
    overflow |= check_epi16_overflow_x8_avx2(step1);
    overflow |= check_epi16_overflow_x8_avx2(step2);
    overflow |= check_epi16_overflow_x8_avx2(step3);
#endif  // DCT_HIGH_BIT_DEPTH

    // transpose
    transpose_16bit_16x16_avx2(in, in);

    return overflow;
}

static void fadst16_avx2(__m256i *const in) {
    const __m256i kZero = _mm256_set1_epi16(0);
    __m256i s[16], x[16];

    fadst16_kernel_avx2(in[15], in[0], in[7], in[8], cospi_1_64, cospi_31_64, cospi_17_64, cospi_15_64, &s[0], &s[8], &s[1], &s[9]);
    fadst16_kernel_avx2(in[13], in[2], in[5], in[10], cospi_5_64, cospi_27_64, cospi_21_64, cospi_11_64, &s[2], &s[10], &s[3], &s[11]);
    fadst16_kernel_avx2(in[11], in[4], in[3], in[12], cospi_9_64, cospi_23_64, cospi_25_64, cospi_7_64, &s[4], &s[12], &s[5], &s[13]);
    fadst16_kernel_avx2(in[9], in[6], in[1], in[14], cospi_13_64, cospi_19_64, cospi_29_64, cospi_3_64, &s[6], &s[14], &s[7], &s[15]);

    // stage 2
    x[0] = _mm256_add_epi16(s[0], s[4]);
    x[1] = _mm256_add_epi16(s[1], s[5]);
    x[2] = _mm256_add_epi16(s[2], s[6]);
    x[3] = _mm256_add_epi16(s[3], s[7]);
    x[4] = _mm256_sub_epi16(s[0], s[4]);
    x[5] = _mm256_sub_epi16(s[1], s[5]);
    x[6] = _mm256_sub_epi16(s[2], s[6]);
    x[7] = _mm256_sub_epi16(s[3], s[7]);
    fadst16_kernel_avx2(s[8], s[9], s[12], s[13], cospi_4_64, cospi_28_64, -cospi_28_64, cospi_4_64, &x[8], &x[12], &x[9], &x[13]);
    fadst16_kernel_avx2(s[10], s[11], s[14], s[15], cospi_20_64, cospi_12_64, -cospi_12_64, cospi_20_64, &x[10], &x[14], &x[11], &x[15]);

    // stage 3
    s[0] = _mm256_add_epi16(x[0], x[2]);
    s[1] = _mm256_add_epi16(x[1], x[3]);
    s[2] = _mm256_sub_epi16(x[0], x[2]);
    s[3] = _mm256_sub_epi16(x[1], x[3]);
    fadst16_kernel_avx2(x[4], x[5], x[6], x[7], cospi_8_64, cospi_24_64, -cospi_24_64, cospi_8_64, &s[4], &s[6], &s[5], &s[7]);
    s[8] = _mm256_add_epi16(x[8], x[10]);
    s[9] = _mm256_add_epi16(x[9], x[11]);
    s[10] = _mm256_sub_epi16(x[8], x[10]);
    s[11] = _mm256_sub_epi16(x[9], x[11]);
    fadst16_kernel_avx2(x[12], x[13], x[14], x[15], cospi_8_64, cospi_24_64, -cospi_24_64, cospi_8_64, &s[12], &s[14], &s[13], &s[15]);

    // stage 4
    in[0] = s[0];
    in[1] = _mm256_sub_epi16(kZero, s[8]);
    in[2] = s[12];
    in[3] = _mm256_sub_epi16(kZero, s[4]);
    butterfly_avx2(s[7], s[6], cospi_16_64, cospi_16_64, &in[11], &in[4]);
    butterfly_avx2(s[14], s[15], -cospi_16_64, cospi_16_64, &in[5], &in[10]);
    butterfly_avx2(s[11], s[10], cospi_16_64, cospi_16_64, &in[9], &in[6]);
    butterfly_avx2(s[2], s[3], -cospi_16_64, cospi_16_64, &in[7], &in[8]);
    in[12] = s[5];
    in[13] = _mm256_sub_epi16(kZero, s[13]);
    in[14] = s[9];
    in[15] = _mm256_sub_epi16(kZero, s[1]);

    // transpose
    transpose_16bit_16x16_avx2(in, in);
}

void vpx_fdct16x16_avx2(const int16_t *input, tran_low_t *output, int stride) {
    int overflow;
    __m256i in[16];

    load_buffer_left_shift2_16x16_avx2(input, stride, in);
    overflow = fdct16_overflow_avx2(in);
    right_shift_fdct_16x16_avx2(in);
    overflow |= fdct16_overflow_avx2(in);
    write_buffer_16x16_avx2(in, output, 16);

#if DCT_HIGH_BIT_DEPTH
    if (overflow) {
        vpx_highbd_fdct16x16_c(input, output, stride);
    }
#endif  // DCT_HIGH_BIT_DEPTH
}

void vp9_fht16x16_avx2(const int16_t *input, tran_low_t *output, int stride,
    int tx_type) {
    int overflow = 0;
    __m256i in[16];

    load_buffer_left_shift2_16x16_avx2(input, stride, in);

    switch (tx_type) {
    case DCT_DCT:
        overflow = fdct16_overflow_avx2(in);
        right_shift_fdct_16x16_avx2(in);
        overflow |= fdct16_overflow_avx2(in);
        break;

    case ADST_DCT:
        fadst16_avx2(in);
        right_shift_16x16_avx2(in);
        fdct16_avx2(in);
        break;

    case DCT_ADST:
        fdct16_avx2(in);
        right_shift_16x16_avx2(in);
        fadst16_avx2(in);
        break;

    default:
        assert(tx_type == ADST_ADST);
        fadst16_avx2(in);
        right_shift_16x16_avx2(in);
        fadst16_avx2(in);
        break;
    }

    write_buffer_16x16_avx2(in, output, 16);

#if DCT_HIGH_BIT_DEPTH
    if (overflow) {
        vpx_highbd_fdct16x16_c(input, output, stride);
    }
#else
    (void)overflow;
#endif  // DCT_HIGH_BIT_DEPTH
}
