/*
 *  Copyright (c) 2015 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <emmintrin.h> // SSE2

#include "vpx_dsp_rtcd.h"
#include "inv_txfm_sse2.h"
//#include "transpose_sse2.h"
//#include "txfm_common_sse2.h"

static inline void transpose_16bit_4(__m128i *res) {
    const __m128i tr0_0 = _mm_unpacklo_epi16(res[0], res[1]);
    const __m128i tr0_1 = _mm_unpackhi_epi16(res[0], res[1]);

    res[0] = _mm_unpacklo_epi16(tr0_0, tr0_1);
    res[1] = _mm_unpackhi_epi16(tr0_0, tr0_1);
}

void eb_vp9_idct4x4_16_add_sse2(const tran_low_t *input, uint8_t *dest, int stride) {
    const __m128i eight = _mm_set1_epi16(8);
    __m128i       in[2];

    // Rows
    in[0] = load_input_data8(input);
    in[1] = load_input_data8(input + 8);
    eb_vp9_idct4_sse2(in);

    // Columns
    eb_vp9_idct4_sse2(in);

    // Final round and shift
    in[0] = _mm_add_epi16(in[0], eight);
    in[1] = _mm_add_epi16(in[1], eight);
    in[0] = _mm_srai_epi16(in[0], 4);
    in[1] = _mm_srai_epi16(in[1], 4);

    recon_and_store4x4_sse2(in, dest, stride);
}

void eb_vp9_idct4x4_1_add_sse2(const tran_low_t *input, uint8_t *dest, int stride) {
    const __m128i zero = _mm_setzero_si128();
    int           a;
    __m128i       dc_value, d[2];

    a = (int)dct_const_round_shift((int16_t)input[0] * cospi_16_64);
    a = (int)dct_const_round_shift(a * cospi_16_64);
    a = ROUND_POWER_OF_TWO(a, 4);

    dc_value = _mm_set1_epi16(a);

    // Reconstruction and Store
    d[0] = _mm_cvtsi32_si128(*(const int *)(dest));
    d[1] = _mm_cvtsi32_si128(*(const int *)(dest + stride * 3));
    d[0] = _mm_unpacklo_epi32(d[0], _mm_cvtsi32_si128(*(const int *)(dest + stride)));
    d[1] = _mm_unpacklo_epi32(_mm_cvtsi32_si128(*(const int *)(dest + stride * 2)), d[1]);
    d[0] = _mm_unpacklo_epi8(d[0], zero);
    d[1] = _mm_unpacklo_epi8(d[1], zero);
    d[0] = _mm_add_epi16(d[0], dc_value);
    d[1] = _mm_add_epi16(d[1], dc_value);
    d[0] = _mm_packus_epi16(d[0], d[1]);

    *(int *)dest                = _mm_cvtsi128_si32(d[0]);
    d[0]                        = _mm_srli_si128(d[0], 4);
    *(int *)(dest + stride)     = _mm_cvtsi128_si32(d[0]);
    d[0]                        = _mm_srli_si128(d[0], 4);
    *(int *)(dest + stride * 2) = _mm_cvtsi128_si32(d[0]);
    d[0]                        = _mm_srli_si128(d[0], 4);
    *(int *)(dest + stride * 3) = _mm_cvtsi128_si32(d[0]);
}

void eb_vp9_idct4_sse2(__m128i *const in) {
    const __m128i k__cospi_p16_p16 = pair_set_epi16(cospi_16_64, cospi_16_64);
    const __m128i k__cospi_p16_m16 = pair_set_epi16(cospi_16_64, -cospi_16_64);
    const __m128i k__cospi_p24_m08 = pair_set_epi16(cospi_24_64, -cospi_8_64);
    const __m128i k__cospi_p08_p24 = pair_set_epi16(cospi_8_64, cospi_24_64);
    __m128i       u[2];

    transpose_16bit_4(in);
    // stage 1
    u[0] = _mm_unpacklo_epi16(in[0], in[1]);
    u[1] = _mm_unpackhi_epi16(in[0], in[1]);
    u[0] = idct_calc_wraplow_sse2(k__cospi_p16_p16, k__cospi_p16_m16, u[0]);
    u[1] = idct_calc_wraplow_sse2(k__cospi_p08_p24, k__cospi_p24_m08, u[1]);

    // stage 2
    in[0] = _mm_add_epi16(u[0], u[1]);
    in[1] = _mm_sub_epi16(u[0], u[1]);
    in[1] = _mm_shuffle_epi32(in[1], 0x4E);
}

void eb_vp9_iadst4_sse2(__m128i *const in) {
    const __m128i k__sinpi_1_3   = pair_set_epi16(sinpi_1_9, sinpi_3_9);
    const __m128i k__sinpi_4_2   = pair_set_epi16(sinpi_4_9, sinpi_2_9);
    const __m128i k__sinpi_2_3   = pair_set_epi16(sinpi_2_9, sinpi_3_9);
    const __m128i k__sinpi_1_4   = pair_set_epi16(sinpi_1_9, sinpi_4_9);
    const __m128i k__sinpi_12_n3 = pair_set_epi16(sinpi_1_9 + sinpi_2_9, -sinpi_3_9);
    __m128i       u[4], v[5];

    // 00 01 20 21  02 03 22 23
    // 10 11 30 31  12 13 32 33
    const __m128i tr0_0 = _mm_unpacklo_epi32(in[0], in[1]);
    const __m128i tr0_1 = _mm_unpackhi_epi32(in[0], in[1]);

    // 00 01 10 11  20 21 30 31
    // 02 03 12 13  22 23 32 33
    in[0] = _mm_unpacklo_epi32(tr0_0, tr0_1);
    in[1] = _mm_unpackhi_epi32(tr0_0, tr0_1);

    v[0]  = _mm_madd_epi16(in[0], k__sinpi_1_3); // s_1 * x0 + s_3 * x1
    v[1]  = _mm_madd_epi16(in[1], k__sinpi_4_2); // s_4 * x2 + s_2 * x3
    v[2]  = _mm_madd_epi16(in[0], k__sinpi_2_3); // s_2 * x0 + s_3 * x1
    v[3]  = _mm_madd_epi16(in[1], k__sinpi_1_4); // s_1 * x2 + s_4 * x3
    v[4]  = _mm_madd_epi16(in[0], k__sinpi_12_n3); // (s_1 + s_2) * x0 - s_3 * x1
    in[0] = _mm_sub_epi16(in[0], in[1]); // x0 - x2
    in[1] = _mm_srli_epi32(in[1], 16);
    in[0] = _mm_add_epi16(in[0], in[1]);
    in[0] = _mm_slli_epi32(in[0], 16); // x0 - x2 + x3

    u[0] = _mm_add_epi32(v[0], v[1]);
    u[1] = _mm_sub_epi32(v[2], v[3]);
    u[2] = _mm_madd_epi16(in[0], k__sinpi_1_3);
    u[3] = _mm_sub_epi32(v[1], v[3]);
    u[3] = _mm_add_epi32(u[3], v[4]);

    u[0] = dct_const_round_shift_sse2(u[0]);
    u[1] = dct_const_round_shift_sse2(u[1]);
    u[2] = dct_const_round_shift_sse2(u[2]);
    u[3] = dct_const_round_shift_sse2(u[3]);

    in[0] = _mm_packs_epi32(u[0], u[1]);
    in[1] = _mm_packs_epi32(u[2], u[3]);
}

void eb_vp9_idct8x8_64_add_sse2(const tran_low_t *input, uint8_t *dest, int stride) {
    __m128i in[8];
    int     i;

    // Load input data.
    load_buffer_8x8(input, 8, in);

    // 2-D
    for (i = 0; i < 2; i++) { eb_vp9_idct8_sse2(in); }

    write_buffer_8x8(in, dest, stride);
}

static inline void recon_and_store_8_dual(const __m128i in, uint8_t *const dest, const int stride) {
    const __m128i zero = _mm_setzero_si128();
    __m128i       d0, d1;

    d0 = _mm_loadl_epi64((__m128i *)(dest + 0 * stride));
    d1 = _mm_loadl_epi64((__m128i *)(dest + 1 * stride));
    d0 = _mm_unpacklo_epi8(d0, zero);
    d1 = _mm_unpacklo_epi8(d1, zero);
    d0 = _mm_add_epi16(in, d0);
    d1 = _mm_add_epi16(in, d1);
    d0 = _mm_packus_epi16(d0, d1);
    _mm_storel_epi64((__m128i *)(dest + 0 * stride), d0);
    _mm_storeh_pi((__m64 *)(dest + 1 * stride), _mm_castsi128_ps(d0));
}

void eb_vp9_idct8x8_1_add_sse2(const tran_low_t *input, uint8_t *dest, int stride) {
    __m128i     dc_value;
    tran_high_t a1;
    tran_low_t  out = WRAPLOW(dct_const_round_shift((int16_t)input[0] * cospi_16_64));

    out      = WRAPLOW(dct_const_round_shift(out * cospi_16_64));
    a1       = ROUND_POWER_OF_TWO(out, 5);
    dc_value = _mm_set1_epi16((int16_t)a1);

    recon_and_store_8_dual(dc_value, dest, stride);
    dest += 2 * stride;
    recon_and_store_8_dual(dc_value, dest, stride);
    dest += 2 * stride;
    recon_and_store_8_dual(dc_value, dest, stride);
    dest += 2 * stride;
    recon_and_store_8_dual(dc_value, dest, stride);
}

void eb_vp9_idct8_sse2(__m128i *const in) {
    transpose_16bit_8x8(in, in);

    // 4-stage 1D idct8x8
    idct8(in, in);
}

void eb_vp9_iadst8_sse2(__m128i *const in) {
    const __m128i k__cospi_p02_p30 = pair_set_epi16(cospi_2_64, cospi_30_64);
    const __m128i k__cospi_p30_m02 = pair_set_epi16(cospi_30_64, -cospi_2_64);
    const __m128i k__cospi_p10_p22 = pair_set_epi16(cospi_10_64, cospi_22_64);
    const __m128i k__cospi_p22_m10 = pair_set_epi16(cospi_22_64, -cospi_10_64);
    const __m128i k__cospi_p18_p14 = pair_set_epi16(cospi_18_64, cospi_14_64);
    const __m128i k__cospi_p14_m18 = pair_set_epi16(cospi_14_64, -cospi_18_64);
    const __m128i k__cospi_p26_p06 = pair_set_epi16(cospi_26_64, cospi_6_64);
    const __m128i k__cospi_p06_m26 = pair_set_epi16(cospi_6_64, -cospi_26_64);
    const __m128i k__cospi_p08_p24 = pair_set_epi16(cospi_8_64, cospi_24_64);
    const __m128i k__cospi_p24_m08 = pair_set_epi16(cospi_24_64, -cospi_8_64);
    const __m128i k__cospi_m24_p08 = pair_set_epi16(-cospi_24_64, cospi_8_64);
    const __m128i k__cospi_p16_m16 = pair_set_epi16(cospi_16_64, -cospi_16_64);
    const __m128i k__cospi_p16_p16 = _mm_set1_epi16(cospi_16_64);
    const __m128i kZero            = _mm_set1_epi16(0);
    __m128i       s[8], u[16], v[8], w[16];

    // transpose
    transpose_16bit_8x8(in, in);

    // column transformation
    // stage 1
    // interleave and multiply/add into 32-bit integer
    s[0] = _mm_unpacklo_epi16(in[7], in[0]);
    s[1] = _mm_unpackhi_epi16(in[7], in[0]);
    s[2] = _mm_unpacklo_epi16(in[5], in[2]);
    s[3] = _mm_unpackhi_epi16(in[5], in[2]);
    s[4] = _mm_unpacklo_epi16(in[3], in[4]);
    s[5] = _mm_unpackhi_epi16(in[3], in[4]);
    s[6] = _mm_unpacklo_epi16(in[1], in[6]);
    s[7] = _mm_unpackhi_epi16(in[1], in[6]);

    u[0]  = _mm_madd_epi16(s[0], k__cospi_p02_p30);
    u[1]  = _mm_madd_epi16(s[1], k__cospi_p02_p30);
    u[2]  = _mm_madd_epi16(s[0], k__cospi_p30_m02);
    u[3]  = _mm_madd_epi16(s[1], k__cospi_p30_m02);
    u[4]  = _mm_madd_epi16(s[2], k__cospi_p10_p22);
    u[5]  = _mm_madd_epi16(s[3], k__cospi_p10_p22);
    u[6]  = _mm_madd_epi16(s[2], k__cospi_p22_m10);
    u[7]  = _mm_madd_epi16(s[3], k__cospi_p22_m10);
    u[8]  = _mm_madd_epi16(s[4], k__cospi_p18_p14);
    u[9]  = _mm_madd_epi16(s[5], k__cospi_p18_p14);
    u[10] = _mm_madd_epi16(s[4], k__cospi_p14_m18);
    u[11] = _mm_madd_epi16(s[5], k__cospi_p14_m18);
    u[12] = _mm_madd_epi16(s[6], k__cospi_p26_p06);
    u[13] = _mm_madd_epi16(s[7], k__cospi_p26_p06);
    u[14] = _mm_madd_epi16(s[6], k__cospi_p06_m26);
    u[15] = _mm_madd_epi16(s[7], k__cospi_p06_m26);

    // addition
    w[0]  = _mm_add_epi32(u[0], u[8]);
    w[1]  = _mm_add_epi32(u[1], u[9]);
    w[2]  = _mm_add_epi32(u[2], u[10]);
    w[3]  = _mm_add_epi32(u[3], u[11]);
    w[4]  = _mm_add_epi32(u[4], u[12]);
    w[5]  = _mm_add_epi32(u[5], u[13]);
    w[6]  = _mm_add_epi32(u[6], u[14]);
    w[7]  = _mm_add_epi32(u[7], u[15]);
    w[8]  = _mm_sub_epi32(u[0], u[8]);
    w[9]  = _mm_sub_epi32(u[1], u[9]);
    w[10] = _mm_sub_epi32(u[2], u[10]);
    w[11] = _mm_sub_epi32(u[3], u[11]);
    w[12] = _mm_sub_epi32(u[4], u[12]);
    w[13] = _mm_sub_epi32(u[5], u[13]);
    w[14] = _mm_sub_epi32(u[6], u[14]);
    w[15] = _mm_sub_epi32(u[7], u[15]);

    // shift and rounding
    u[0]  = dct_const_round_shift_sse2(w[0]);
    u[1]  = dct_const_round_shift_sse2(w[1]);
    u[2]  = dct_const_round_shift_sse2(w[2]);
    u[3]  = dct_const_round_shift_sse2(w[3]);
    u[4]  = dct_const_round_shift_sse2(w[4]);
    u[5]  = dct_const_round_shift_sse2(w[5]);
    u[6]  = dct_const_round_shift_sse2(w[6]);
    u[7]  = dct_const_round_shift_sse2(w[7]);
    u[8]  = dct_const_round_shift_sse2(w[8]);
    u[9]  = dct_const_round_shift_sse2(w[9]);
    u[10] = dct_const_round_shift_sse2(w[10]);
    u[11] = dct_const_round_shift_sse2(w[11]);
    u[12] = dct_const_round_shift_sse2(w[12]);
    u[13] = dct_const_round_shift_sse2(w[13]);
    u[14] = dct_const_round_shift_sse2(w[14]);
    u[15] = dct_const_round_shift_sse2(w[15]);

    // back to 16-bit and pack 8 integers into __m128i
    in[0] = _mm_packs_epi32(u[0], u[1]);
    in[1] = _mm_packs_epi32(u[2], u[3]);
    in[2] = _mm_packs_epi32(u[4], u[5]);
    in[3] = _mm_packs_epi32(u[6], u[7]);
    in[4] = _mm_packs_epi32(u[8], u[9]);
    in[5] = _mm_packs_epi32(u[10], u[11]);
    in[6] = _mm_packs_epi32(u[12], u[13]);
    in[7] = _mm_packs_epi32(u[14], u[15]);

    // stage 2
    s[0] = _mm_add_epi16(in[0], in[2]);
    s[1] = _mm_add_epi16(in[1], in[3]);
    s[2] = _mm_sub_epi16(in[0], in[2]);
    s[3] = _mm_sub_epi16(in[1], in[3]);
    u[0] = _mm_unpacklo_epi16(in[4], in[5]);
    u[1] = _mm_unpackhi_epi16(in[4], in[5]);
    u[2] = _mm_unpacklo_epi16(in[6], in[7]);
    u[3] = _mm_unpackhi_epi16(in[6], in[7]);

    v[0] = _mm_madd_epi16(u[0], k__cospi_p08_p24);
    v[1] = _mm_madd_epi16(u[1], k__cospi_p08_p24);
    v[2] = _mm_madd_epi16(u[0], k__cospi_p24_m08);
    v[3] = _mm_madd_epi16(u[1], k__cospi_p24_m08);
    v[4] = _mm_madd_epi16(u[2], k__cospi_m24_p08);
    v[5] = _mm_madd_epi16(u[3], k__cospi_m24_p08);
    v[6] = _mm_madd_epi16(u[2], k__cospi_p08_p24);
    v[7] = _mm_madd_epi16(u[3], k__cospi_p08_p24);

    w[0] = _mm_add_epi32(v[0], v[4]);
    w[1] = _mm_add_epi32(v[1], v[5]);
    w[2] = _mm_add_epi32(v[2], v[6]);
    w[3] = _mm_add_epi32(v[3], v[7]);
    w[4] = _mm_sub_epi32(v[0], v[4]);
    w[5] = _mm_sub_epi32(v[1], v[5]);
    w[6] = _mm_sub_epi32(v[2], v[6]);
    w[7] = _mm_sub_epi32(v[3], v[7]);

    u[0] = dct_const_round_shift_sse2(w[0]);
    u[1] = dct_const_round_shift_sse2(w[1]);
    u[2] = dct_const_round_shift_sse2(w[2]);
    u[3] = dct_const_round_shift_sse2(w[3]);
    u[4] = dct_const_round_shift_sse2(w[4]);
    u[5] = dct_const_round_shift_sse2(w[5]);
    u[6] = dct_const_round_shift_sse2(w[6]);
    u[7] = dct_const_round_shift_sse2(w[7]);

    // back to 16-bit intergers
    s[4] = _mm_packs_epi32(u[0], u[1]);
    s[5] = _mm_packs_epi32(u[2], u[3]);
    s[6] = _mm_packs_epi32(u[4], u[5]);
    s[7] = _mm_packs_epi32(u[6], u[7]);

    // stage 3
    u[0] = _mm_unpacklo_epi16(s[2], s[3]);
    u[1] = _mm_unpackhi_epi16(s[2], s[3]);
    u[2] = _mm_unpacklo_epi16(s[6], s[7]);
    u[3] = _mm_unpackhi_epi16(s[6], s[7]);

    s[2] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_p16_p16);
    s[3] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_p16_m16);
    s[6] = idct_calc_wraplow_sse2(u[2], u[3], k__cospi_p16_p16);
    s[7] = idct_calc_wraplow_sse2(u[2], u[3], k__cospi_p16_m16);

    in[0] = s[0];
    in[1] = _mm_sub_epi16(kZero, s[4]);
    in[2] = s[6];
    in[3] = _mm_sub_epi16(kZero, s[2]);
    in[4] = s[3];
    in[5] = _mm_sub_epi16(kZero, s[7]);
    in[6] = s[5];
    in[7] = _mm_sub_epi16(kZero, s[1]);
}

void eb_vp9_idct16x16_256_add_sse2(const tran_low_t *input, uint8_t *dest, int stride) {
    __m128i l[16], r[16], out[16], *in;
    int     i;

    in = l;
    for (i = 0; i < 2; i++) {
        load_transpose_16bit_8x8(input, 16, in);
        load_transpose_16bit_8x8(input + 8, 16, in + 8);
        idct16_8col(in, in);
        in = r;
        input += 128;
    }

    for (i = 0; i < 16; i += 8) {
        int j;
        transpose_16bit_8x8(l + i, out);
        transpose_16bit_8x8(r + i, out + 8);
        idct16_8col(out, out);

        for (j = 0; j < 16; ++j) { write_buffer_8x1(dest + j * stride, out[j]); }

        dest += 8;
    }
}

void eb_vp9_idct16x16_38_add_sse2(const tran_low_t *input, uint8_t *dest, int stride) {
    __m128i in[16], temp[16], out[16];
    int     i;

    load_transpose_16bit_8x8(input, 16, in);

    for (i = 8; i < 16; i++) { in[i] = _mm_setzero_si128(); }
    idct16_8col(in, temp);

    for (i = 0; i < 16; i += 8) {
        int j;
        transpose_16bit_8x8(temp + i, in);
        idct16_8col(in, out);

        for (j = 0; j < 16; ++j) { write_buffer_8x1(dest + j * stride, out[j]); }

        dest += 8;
    }
}

void eb_vp9_idct16x16_10_add_sse2(const tran_low_t *input, uint8_t *dest, int stride) {
    __m128i in[16], l[16];
    int     i;

    // First 1-D inverse DCT
    // Load input data.
    in[0] = load_input_data4(input + 0 * 16);
    in[1] = load_input_data4(input + 1 * 16);
    in[2] = load_input_data4(input + 2 * 16);
    in[3] = load_input_data4(input + 3 * 16);

    idct16x16_10_pass1(in, l);

    // Second 1-D inverse transform, performed per 8x16 block
    for (i = 0; i < 16; i += 8) {
        int j;
        idct16x16_10_pass2(l + i, in);

        for (j = 0; j < 16; ++j) { write_buffer_8x1(dest + j * stride, in[j]); }

        dest += 8;
    }
}

void eb_vp9_idct16x16_1_add_sse2(const tran_low_t *input, uint8_t *dest, int stride) {
    __m128i     dc_value;
    int         i;
    tran_high_t a1;
    tran_low_t  out = WRAPLOW(dct_const_round_shift((int16_t)input[0] * cospi_16_64));

    out      = WRAPLOW(dct_const_round_shift(out * cospi_16_64));
    a1       = ROUND_POWER_OF_TWO(out, 6);
    dc_value = _mm_set1_epi16((int16_t)a1);

    for (i = 0; i < 16; ++i) {
        recon_and_store_16(dc_value, dc_value, dest);
        dest += stride;
    }
}
