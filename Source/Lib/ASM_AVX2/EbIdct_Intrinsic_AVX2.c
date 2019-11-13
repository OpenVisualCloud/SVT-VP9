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
#include "inv_txfm_sse2.h"
#include "tmmintrin.h"

static INLINE __m256i load_input_data16_avx2(
    const tran_low_t *const data)
{
#if CONFIG_VP9_HIGHBITDEPTH
    const __m256i in0 = _mm256_load_si256((const __m256i *)(data + 0));
    const __m256i in1 = _mm256_load_si256((const __m256i *)(data + 8));
    const __m256i in = _mm256_packs_epi32(in0, in1);
    return _mm256_permute4x64_epi64(in, 0xD8);
#else
    return _mm256_load_si256((const __m256i *)data);
#endif
}

static INLINE void load_buffer_8x16(
    const tran_low_t *const input,
    const int               stride,
    __m128i *const          in)
{
    load_buffer_8x8(input + 0 * 16, stride, in + 0);
    load_buffer_8x8(input + 8 * 16, stride, in + 8);
}

static INLINE void load_buffer_16x16_avx2(
    const tran_low_t *const input,
    const int               stride,
    __m256i *const          in)
{
    in[0]  = load_input_data16_avx2(input + 0 * stride);
    in[1]  = load_input_data16_avx2(input + 1 * stride);
    in[2]  = load_input_data16_avx2(input + 2 * stride);
    in[3]  = load_input_data16_avx2(input + 3 * stride);
    in[4]  = load_input_data16_avx2(input + 4 * stride);
    in[5]  = load_input_data16_avx2(input + 5 * stride);
    in[6]  = load_input_data16_avx2(input + 6 * stride);
    in[7]  = load_input_data16_avx2(input + 7 * stride);
    in[8]  = load_input_data16_avx2(input + 8 * stride);
    in[9]  = load_input_data16_avx2(input + 9 * stride);
    in[10] = load_input_data16_avx2(input + 10 * stride);
    in[11] = load_input_data16_avx2(input + 11 * stride);
    in[12] = load_input_data16_avx2(input + 12 * stride);
    in[13] = load_input_data16_avx2(input + 13 * stride);
    in[14] = load_input_data16_avx2(input + 14 * stride);
    in[15] = load_input_data16_avx2(input + 15 * stride);
}

static INLINE void recon_and_store_avx2(
    const __m256i  in,
    uint8_t *const dest)
{
    const __m128i zero = _mm_setzero_si128();
    __m128i d = _mm_loadu_si128((__m128i *)dest);
    __m128i d0, d1;
    __m256i dd;
    d0 = _mm_unpacklo_epi8(d, zero);
    d1 = _mm_unpackhi_epi8(d, zero);
    dd = _mm256_setr_m128i(d0, d1);
    dd = _mm256_add_epi16(in, dd);
    dd = _mm256_packus_epi16(dd, dd);
    dd = _mm256_permute4x64_epi64(dd, 0xD8);
    _mm_storeu_si128((__m128i *)dest, _mm256_extracti128_si256(dd, 0));
}

static INLINE void write_buffer_16x1_avx2(
    const __m256i  in,
    uint8_t *const dest)
{
    const __m256i final_rounding = _mm256_set1_epi16(1 << 5);
    __m256i out;
    out = _mm256_adds_epi16(in, final_rounding);
    out = _mm256_srai_epi16(out, 6);
    recon_and_store_avx2(out, dest);
}

static INLINE void write_buffer_16_avx2(
    const __m256i in,
    uint8_t *const dest)
{
    const __m256i final_rounding = _mm256_set1_epi16(1 << 5);
    const __m256i d0 = _mm256_adds_epi16(in, final_rounding);
    const __m256i d1 = _mm256_srai_epi16(d0, 6);
    recon_and_store_avx2(d1, dest);
}

static INLINE void write_buffer_16x16_avx2(
    __m256i *const in,
    uint8_t *const dest,
    const int      stride)
{
    // Final rounding and shift
    write_buffer_16x1_avx2(in[0],  dest + 0 *  stride);
    write_buffer_16x1_avx2(in[1],  dest + 1 *  stride);
    write_buffer_16x1_avx2(in[2],  dest + 2 *  stride);
    write_buffer_16x1_avx2(in[3],  dest + 3 *  stride);
    write_buffer_16x1_avx2(in[4],  dest + 4 *  stride);
    write_buffer_16x1_avx2(in[5],  dest + 5 *  stride);
    write_buffer_16x1_avx2(in[6],  dest + 6 *  stride);
    write_buffer_16x1_avx2(in[7],  dest + 7 *  stride);
    write_buffer_16x1_avx2(in[8],  dest + 8 *  stride);
    write_buffer_16x1_avx2(in[9],  dest + 9 *  stride);
    write_buffer_16x1_avx2(in[10], dest + 10 * stride);
    write_buffer_16x1_avx2(in[11], dest + 11 * stride);
    write_buffer_16x1_avx2(in[12], dest + 12 * stride);
    write_buffer_16x1_avx2(in[13], dest + 13 * stride);
    write_buffer_16x1_avx2(in[14], dest + 14 * stride);
    write_buffer_16x1_avx2(in[15], dest + 15 * stride);
}

// Only do addition and subtraction butterfly, size = 16, 32
static INLINE void add_sub_butterfly_avx2(
    const __m256i *const in,
    __m256i *const       out,
    const int            size)
{
    int i           = 0;
    const int num   = size >> 1;
    const int bound = size - 1;
    while (i < num) {
        out[i]         = _mm256_add_epi16(in[i], in[bound - i]);
        out[bound - i] = _mm256_sub_epi16(in[i], in[bound - i]);
        i++;
    }
}

static INLINE void idct16_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i step1[16], step2[16];

    // stage 2
    butterfly_avx2(in[1], in[15], cospi_30_64, cospi_2_64, &step2[8], &step2[15]);
    butterfly_avx2(in[9], in[7], cospi_14_64, cospi_18_64, &step2[9], &step2[14]);
    butterfly_avx2(in[5], in[11], cospi_22_64, cospi_10_64, &step2[10], &step2[13]);
    butterfly_avx2(in[13], in[3], cospi_6_64, cospi_26_64, &step2[11], &step2[12]);

    // stage 3
    butterfly_avx2(in[2], in[14], cospi_28_64, cospi_4_64, &step1[4], &step1[7]);
    butterfly_avx2(in[10], in[6], cospi_12_64, cospi_20_64, &step1[5], &step1[6]);
    step1[8]  = _mm256_add_epi16(step2[8], step2[9]);
    step1[9]  = _mm256_sub_epi16(step2[8], step2[9]);
    step1[10] = _mm256_sub_epi16(step2[11], step2[10]);
    step1[11] = _mm256_add_epi16(step2[10], step2[11]);
    step1[12] = _mm256_add_epi16(step2[12], step2[13]);
    step1[13] = _mm256_sub_epi16(step2[12], step2[13]);
    step1[14] = _mm256_sub_epi16(step2[15], step2[14]);
    step1[15] = _mm256_add_epi16(step2[14], step2[15]);

    // stage 4
    butterfly_avx2(in[0], in[8], cospi_16_64, cospi_16_64, &step2[1], &step2[0]);
    butterfly_avx2(in[4], in[12], cospi_24_64, cospi_8_64, &step2[2], &step2[3]);

    butterfly_avx2(step1[14], step1[9], cospi_24_64, cospi_8_64, &step2[9], &step2[14]);
    butterfly_avx2(step1[10], step1[13], -cospi_8_64, -cospi_24_64, &step2[13], &step2[10]);

    step2[5]  = _mm256_sub_epi16(step1[4], step1[5]);
    step1[4]  = _mm256_add_epi16(step1[4], step1[5]);
    step2[6]  = _mm256_sub_epi16(step1[7], step1[6]);
    step1[7]  = _mm256_add_epi16(step1[6], step1[7]);
    step2[8]  = step1[8];
    step2[11] = step1[11];
    step2[12] = step1[12];
    step2[15] = step1[15];

    // stage 5
    add_sub_butterfly_avx2(step2 + 0, step1 + 0, 4);
    butterfly_avx2(step2[6], step2[5], cospi_16_64, cospi_16_64, &step1[5], &step1[6]);
    add_sub_butterfly_avx2(step2 + 8, step1 + 8, 4);
    step1[12] = _mm256_sub_epi16(step2[15], step2[12]);
    step1[13] = _mm256_sub_epi16(step2[14], step2[13]);
    step1[14] = _mm256_add_epi16(step2[14], step2[13]);
    step1[15] = _mm256_add_epi16(step2[15], step2[12]);

    // stage 6
    add_sub_butterfly_avx2(step1, step2, 8);
    step2[8] = step1[8];
    step2[9] = step1[9];
    butterfly_avx2(step1[13], step1[10], cospi_16_64, cospi_16_64, &step2[10],
        &step2[13]);
    butterfly_avx2(step1[12], step1[11], cospi_16_64, cospi_16_64, &step2[11],
        &step2[12]);
    step2[14] = step1[14];
    step2[15] = step1[15];

    // stage 7
    add_sub_butterfly_avx2(step2, out, 16);
}

static INLINE void iadst16_kernel_avx2(
    const __m256i     in0,
    const __m256i     in1,
    const __m256i     in2,
    const __m256i     in3,
    const tran_coef_t c0,
    const tran_coef_t c1,
    const tran_coef_t c2,
    const tran_coef_t c3,
    __m256i *const    out0,
    __m256i *const    out1,
    __m256i *const    out2,
    __m256i *const    out3)
{
    const __m256i cst0 = pair_set_epi16_avx2(c0, c1);
    const __m256i cst1 = pair_set_epi16_avx2(c1, -c0);
    const __m256i cst2 = pair_set_epi16_avx2(c2, c3);
    const __m256i cst3 = pair_set_epi16_avx2(c3, -c2);
    __m256i u[8], v[8];

    u[0]  = _mm256_unpacklo_epi16(in0, in1);
    u[1]  = _mm256_unpackhi_epi16(in0, in1);
    u[2]  = _mm256_unpacklo_epi16(in2, in3);
    u[3]  = _mm256_unpackhi_epi16(in2, in3);

    v[0]  = _mm256_madd_epi16(u[0], cst0);
    v[1]  = _mm256_madd_epi16(u[1], cst0);
    v[2]  = _mm256_madd_epi16(u[0], cst1);
    v[3]  = _mm256_madd_epi16(u[1], cst1);

    v[4]  = _mm256_madd_epi16(u[2], cst2);
    v[5]  = _mm256_madd_epi16(u[3], cst2);
    v[6]  = _mm256_madd_epi16(u[2], cst3);
    v[7]  = _mm256_madd_epi16(u[3], cst3);

    u[0]  = _mm256_add_epi32(v[0], v[4]);
    u[1]  = _mm256_add_epi32(v[1], v[5]);
    u[2]  = _mm256_add_epi32(v[2], v[6]);
    u[3]  = _mm256_add_epi32(v[3], v[7]);
    u[4]  = _mm256_sub_epi32(v[0], v[4]);
    u[5]  = _mm256_sub_epi32(v[1], v[5]);
    u[6]  = _mm256_sub_epi32(v[2], v[6]);
    u[7]  = _mm256_sub_epi32(v[3], v[7]);

    u[0]  = dct_const_round_shift_avx2(u[0]);
    u[1]  = dct_const_round_shift_avx2(u[1]);
    u[2]  = dct_const_round_shift_avx2(u[2]);
    u[3]  = dct_const_round_shift_avx2(u[3]);
    u[4]  = dct_const_round_shift_avx2(u[4]);
    u[5]  = dct_const_round_shift_avx2(u[5]);
    u[6]  = dct_const_round_shift_avx2(u[6]);
    u[7]  = dct_const_round_shift_avx2(u[7]);

    *out0 = _mm256_packs_epi32(u[0], u[1]);
    *out1 = _mm256_packs_epi32(u[2], u[3]);
    *out2 = _mm256_packs_epi32(u[4], u[5]);
    *out3 = _mm256_packs_epi32(u[6], u[7]);
}

void iadst16_avx2(__m256i *const in)
{
    const __m256i kZero = _mm256_set1_epi16(0);
    __m256i s[16], x[16];

    // stage 1
    iadst16_kernel_avx2(in[15], in[0], in[7], in[8], cospi_1_64, cospi_31_64, cospi_17_64, cospi_15_64, &s[0], &s[1], &s[8], &s[9]);
    iadst16_kernel_avx2(in[13], in[2], in[5], in[10], cospi_5_64, cospi_27_64, cospi_21_64, cospi_11_64, &s[2], &s[3], &s[10], &s[11]);
    iadst16_kernel_avx2(in[11], in[4], in[3], in[12], cospi_9_64, cospi_23_64, cospi_25_64, cospi_7_64, &s[4], &s[5], &s[12], &s[13]);
    iadst16_kernel_avx2(in[9] , in[6], in[1], in[14], cospi_13_64, cospi_19_64, cospi_29_64, cospi_3_64, &s[6], &s[7], &s[14], &s[15]);

    // stage 2
    x[0]   = _mm256_add_epi16(s[0], s[4]);
    x[1]   = _mm256_add_epi16(s[1], s[5]);
    x[2]   = _mm256_add_epi16(s[2], s[6]);
    x[3]   = _mm256_add_epi16(s[3], s[7]);
    x[4]   = _mm256_sub_epi16(s[0], s[4]);
    x[5]   = _mm256_sub_epi16(s[1], s[5]);
    x[6]   = _mm256_sub_epi16(s[2], s[6]);
    x[7]   = _mm256_sub_epi16(s[3], s[7]);
    iadst16_kernel_avx2(s[8], s[9], s[12], s[13], cospi_4_64, cospi_28_64, -cospi_28_64, cospi_4_64, &x[8], &x[9], &x[12], &x[13]);
    iadst16_kernel_avx2(s[10], s[11], s[14], s[15], cospi_20_64, cospi_12_64, -cospi_12_64, cospi_20_64, &x[10], &x[11], &x[14], &x[15]);

    // stage 3
    s[0]   = _mm256_add_epi16(x[0], x[2]);
    s[1]   = _mm256_add_epi16(x[1], x[3]);
    s[2]   = _mm256_sub_epi16(x[0], x[2]);
    s[3]   = _mm256_sub_epi16(x[1], x[3]);
    iadst16_kernel_avx2(x[4], x[5], x[6], x[7], cospi_8_64, cospi_24_64, -cospi_24_64, cospi_8_64, &s[4], &s[5], &s[6], &s[7]);
    s[8]   = _mm256_add_epi16(x[8], x[10]);
    s[9]   = _mm256_add_epi16(x[9], x[11]);
    s[10]  = _mm256_sub_epi16(x[8], x[10]);
    s[11]  = _mm256_sub_epi16(x[9], x[11]);
    iadst16_kernel_avx2(x[12], x[13], x[14], x[15], cospi_8_64, cospi_24_64, -cospi_24_64, cospi_8_64, &s[12], &s[13], &s[14], &s[15]);

    // stage 4
    in[0]  = s[0];
    in[1]  = _mm256_sub_epi16(kZero, s[8]);
    in[2]  = s[12];
    in[3]  = _mm256_sub_epi16(kZero, s[4]);
    butterfly_avx2(s[6], s[7], cospi_16_64, -cospi_16_64, &in[4], &in[11]);
    butterfly_avx2(s[14], s[15], -cospi_16_64, cospi_16_64, &in[5], &in[10]);
    butterfly_avx2(s[10], s[11], cospi_16_64, -cospi_16_64, &in[6], &in[9]);
    butterfly_avx2(s[2], s[3], -cospi_16_64, cospi_16_64, &in[7], &in[8]);
    in[12] = s[5];
    in[13] = _mm256_sub_epi16(kZero, s[13]);
    in[14] = s[9];
    in[15] = _mm256_sub_epi16(kZero, s[1]);
}

void transpose_idct16_avx2(__m256i *const in)
{
    transpose_16bit_16x16_avx2(in, in);
    idct16_avx2(in, in);
}

void transpose_iadst16_avx2(__m256i *const in)
{
    transpose_16bit_16x16_avx2(in, in);
    iadst16_avx2(in);
}

void vp9_iht16x16_256_add_avx2(
    const tran_low_t *input,
    uint8_t          *dest,
    int               stride,
    int               tx_type)
{
    __m256i in[16];

    load_buffer_16x16_avx2(input, 16, in);

    switch (tx_type) {
    case DCT_DCT:
        transpose_idct16_avx2(in);
        transpose_idct16_avx2(in);
        break;
    case ADST_DCT:
        transpose_idct16_avx2(in);
        transpose_iadst16_avx2(in);
        break;
    case DCT_ADST:
        transpose_iadst16_avx2(in);
        transpose_idct16_avx2(in);
        break;
    default:
        assert(tx_type == ADST_ADST);
        transpose_iadst16_avx2(in);
        transpose_iadst16_avx2(in);
        break;
    }

    write_buffer_16x16_avx2(in, dest, stride);
}

//------------------------------------------------------------------------------

static INLINE void recon_and_store_32_avx2(
    const __m256i  in,
    uint8_t *const dest)
{
    const __m256i zero = _mm256_setzero_si256();
    __m256i d0, d1;

    d0 = _mm256_loadu_si256((__m256i *)dest);
    d1 = _mm256_unpackhi_epi8(d0, zero);
    d0 = _mm256_unpacklo_epi8(d0, zero);
    d0 = _mm256_add_epi16(in, d0);
    d1 = _mm256_add_epi16(in, d1);
    d0 = _mm256_packus_epi16(d0, d1);
    _mm256_storeu_si256((__m256i *)dest, d0);
}

void vpx_idct32x32_1_add_avx2(
    const tran_low_t *input,
    uint8_t          *dest,
    int               stride)
{
    __m256i dc_value;
    int j;
    tran_high_t a1;
    tran_low_t out = WRAPLOW(dct_const_round_shift((int16_t)input[0] * cospi_16_64));

    out            = WRAPLOW(dct_const_round_shift(out * cospi_16_64));
    a1             = ROUND_POWER_OF_TWO(out, 6);
    dc_value       = _mm256_set1_epi16((int16_t)a1);

    for (j = 0; j < 32; ++j) {
        recon_and_store_32_avx2(dc_value, dest + j * stride);
    }
}

//------------------------------------------------------------------------------

static INLINE void load_transpose_16bit_16x16(
    const tran_low_t *const input,
    const int               stride,
    __m256i *const          in)
{
    load_buffer_16x16_avx2(input, stride, in);
    transpose_16bit_16x16_avx2(in, in);
}

static INLINE void partial_butterfly_avx2(
    const __m256i  in,
    const int      c0,
    const int      c1,
    __m256i *const out0,
    __m256i *const out1)
{
    const __m256i cst0 = _mm256_set1_epi16(2 * c0);
    const __m256i cst1 = _mm256_set1_epi16(2 * c1);
    *out0              = _mm256_mulhrs_epi16(in, cst0);
    *out1              = _mm256_mulhrs_epi16(in, cst1);
}

static INLINE __m256i partial_butterfly_cospi16_avx2(const __m256i in)
{
    const __m256i coef_pair = _mm256_set1_epi16(2 * cospi_16_64);
    return _mm256_mulhrs_epi16(in, coef_pair);
}

// Group the coefficient calculation into smaller functions to prevent stack
// spillover in 32x32 idct optimizations:
// quarter_1: 0-7
// quarter_2: 8-15
// quarter_3_4: 16-23, 24-31

static INLINE void idct32_8x32_quarter_2_stage_4_to_6_avx2(
    __m256i *const step1,
    __m256i *const out)
{
    __m256i step2[32];

    // stage 4
    step2[8]  = step1[8];
    step2[15] = step1[15];
    butterfly_avx2(step1[14], step1[9], cospi_24_64, cospi_8_64, &step2[9],
        &step2[14]);
    butterfly_avx2(step1[13], step1[10], -cospi_8_64, cospi_24_64, &step2[10],
        &step2[13]);
    step2[11] = step1[11];
    step2[12] = step1[12];

    // stage 5
    step1[8]  = _mm256_add_epi16(step2[8], step2[11]);
    step1[9]  = _mm256_add_epi16(step2[9], step2[10]);
    step1[10] = _mm256_sub_epi16(step2[9], step2[10]);
    step1[11] = _mm256_sub_epi16(step2[8], step2[11]);
    step1[12] = _mm256_sub_epi16(step2[15], step2[12]);
    step1[13] = _mm256_sub_epi16(step2[14], step2[13]);
    step1[14] = _mm256_add_epi16(step2[14], step2[13]);
    step1[15] = _mm256_add_epi16(step2[15], step2[12]);

    // stage 6
    out[8]    = step1[8];
    out[9]    = step1[9];
    butterfly_avx2(step1[13], step1[10], cospi_16_64, cospi_16_64, &out[10], &out[13]);
    butterfly_avx2(step1[12], step1[11], cospi_16_64, cospi_16_64, &out[11], &out[12]);
    out[14]   = step1[14];
    out[15]   = step1[15];
}

static INLINE void idct32_8x32_quarter_3_4_stage_4_to_7_avx2(
    __m256i *const step1,
    __m256i *const out)
{
    __m256i step2[32];

    // stage 4
    step2[16] = _mm256_add_epi16(step1[16], step1[19]);
    step2[17] = _mm256_add_epi16(step1[17], step1[18]);
    step2[18] = _mm256_sub_epi16(step1[17], step1[18]);
    step2[19] = _mm256_sub_epi16(step1[16], step1[19]);
    step2[20] = _mm256_sub_epi16(step1[23], step1[20]);
    step2[21] = _mm256_sub_epi16(step1[22], step1[21]);
    step2[22] = _mm256_add_epi16(step1[22], step1[21]);
    step2[23] = _mm256_add_epi16(step1[23], step1[20]);

    step2[24] = _mm256_add_epi16(step1[24], step1[27]);
    step2[25] = _mm256_add_epi16(step1[25], step1[26]);
    step2[26] = _mm256_sub_epi16(step1[25], step1[26]);
    step2[27] = _mm256_sub_epi16(step1[24], step1[27]);
    step2[28] = _mm256_sub_epi16(step1[31], step1[28]);
    step2[29] = _mm256_sub_epi16(step1[30], step1[29]);
    step2[30] = _mm256_add_epi16(step1[29], step1[30]);
    step2[31] = _mm256_add_epi16(step1[28], step1[31]);

    // stage 5
    step1[16] = step2[16];
    step1[17] = step2[17];
    butterfly_avx2(step2[29], step2[18], cospi_24_64, cospi_8_64, &step1[18],
        &step1[29]);
    butterfly_avx2(step2[28], step2[19], cospi_24_64, cospi_8_64, &step1[19],
        &step1[28]);
    butterfly_avx2(step2[27], step2[20], -cospi_8_64, cospi_24_64, &step1[20],
        &step1[27]);
    butterfly_avx2(step2[26], step2[21], -cospi_8_64, cospi_24_64, &step1[21],
        &step1[26]);
    step1[22] = step2[22];
    step1[23] = step2[23];
    step1[24] = step2[24];
    step1[25] = step2[25];
    step1[30] = step2[30];
    step1[31] = step2[31];

    // stage 6
    out[16]   = _mm256_add_epi16(step1[16], step1[23]);
    out[17]   = _mm256_add_epi16(step1[17], step1[22]);
    out[18]   = _mm256_add_epi16(step1[18], step1[21]);
    out[19]   = _mm256_add_epi16(step1[19], step1[20]);
    step2[20] = _mm256_sub_epi16(step1[19], step1[20]);
    step2[21] = _mm256_sub_epi16(step1[18], step1[21]);
    step2[22] = _mm256_sub_epi16(step1[17], step1[22]);
    step2[23] = _mm256_sub_epi16(step1[16], step1[23]);

    step2[24] = _mm256_sub_epi16(step1[31], step1[24]);
    step2[25] = _mm256_sub_epi16(step1[30], step1[25]);
    step2[26] = _mm256_sub_epi16(step1[29], step1[26]);
    step2[27] = _mm256_sub_epi16(step1[28], step1[27]);
    out[28]   = _mm256_add_epi16(step1[27], step1[28]);
    out[29]   = _mm256_add_epi16(step1[26], step1[29]);
    out[30]   = _mm256_add_epi16(step1[25], step1[30]);
    out[31]   = _mm256_add_epi16(step1[24], step1[31]);

    // stage 7
    butterfly_avx2(step2[27], step2[20], cospi_16_64, cospi_16_64, &out[20], &out[27]);
    butterfly_avx2(step2[26], step2[21], cospi_16_64, cospi_16_64, &out[21], &out[26]);
    butterfly_avx2(step2[25], step2[22], cospi_16_64, cospi_16_64, &out[22], &out[25]);
    butterfly_avx2(step2[24], step2[23], cospi_16_64, cospi_16_64, &out[23], &out[24]);
}

// For each 8x32 block __m256i in[32],
// Input with index, 0, 4
// output pixels: 0-7 in __m256i out[32]
static INLINE void idct32_34_16x32_quarter_1_avx2(
    const __m256i *const in,
    __m256i *const out)
{
    __m256i step1[8], step2[8];

    // stage 3
    partial_butterfly_avx2(in[4], cospi_28_64, cospi_4_64, &step1[4], &step1[7]);

    // stage 4
    step2[0] = partial_butterfly_cospi16_avx2(in[0]);
    step2[4] = step1[4];
    step2[5] = step1[4];
    step2[6] = step1[7];
    step2[7] = step1[7];

    // stage 5
    step1[0] = step2[0];
    step1[1] = step2[0];
    step1[2] = step2[0];
    step1[3] = step2[0];
    step1[4] = step2[4];
    butterfly_avx2(step2[6], step2[5], cospi_16_64, cospi_16_64, &step1[5], &step1[6]);
    step1[7] = step2[7];

    // stage 6
    out[0]   = _mm256_add_epi16(step1[0], step1[7]);
    out[1]   = _mm256_add_epi16(step1[1], step1[6]);
    out[2]   = _mm256_add_epi16(step1[2], step1[5]);
    out[3]   = _mm256_add_epi16(step1[3], step1[4]);
    out[4]   = _mm256_sub_epi16(step1[3], step1[4]);
    out[5]   = _mm256_sub_epi16(step1[2], step1[5]);
    out[6]   = _mm256_sub_epi16(step1[1], step1[6]);
    out[7]   = _mm256_sub_epi16(step1[0], step1[7]);
}

// For each 8x32 block __m256i in[32],
// Input with index, 2, 6
// output pixels: 8-15 in __m256i out[32]
static INLINE void idct32_34_16x32_quarter_2_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i step1[16], step2[16];

    // stage 2
    partial_butterfly_avx2(in[2], cospi_30_64, cospi_2_64, &step2[8],
        &step2[15]);
    partial_butterfly_avx2(in[6], -cospi_26_64, cospi_6_64, &step2[11],
        &step2[12]);

    // stage 3
    step1[8]  = step2[8];
    step1[9]  = step2[8];
    step1[14] = step2[15];
    step1[15] = step2[15];
    step1[10] = step2[11];
    step1[11] = step2[11];
    step1[12] = step2[12];
    step1[13] = step2[12];

    idct32_8x32_quarter_2_stage_4_to_6_avx2(step1, out);
}

static INLINE void idct32_34_16x32_quarter_1_2_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i temp[16];
    idct32_34_16x32_quarter_1_avx2(in, temp);
    idct32_34_16x32_quarter_2_avx2(in, temp);
    // stage 7
    add_sub_butterfly_avx2(temp, out, 16);
}

// For each 8x32 block __m256i in[32],
// Input with odd index, 1, 3, 5, 7
// output pixels: 16-23, 24-31 in __m256i out[32]
static INLINE void idct32_34_16x32_quarter_3_4_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i step1[32];

    // stage 1
    partial_butterfly_avx2(in[1], cospi_31_64, cospi_1_64, &step1[16],
        &step1[31]);
    partial_butterfly_avx2(in[7], -cospi_25_64, cospi_7_64, &step1[19],
        &step1[28]);
    partial_butterfly_avx2(in[5], cospi_27_64, cospi_5_64, &step1[20],
        &step1[27]);
    partial_butterfly_avx2(in[3], -cospi_29_64, cospi_3_64, &step1[23],
        &step1[24]);

    // stage 3
    butterfly_avx2(step1[31], step1[16], cospi_28_64, cospi_4_64, &step1[17],
        &step1[30]);
    butterfly_avx2(step1[28], step1[19], -cospi_4_64, cospi_28_64, &step1[18],
        &step1[29]);
    butterfly_avx2(step1[27], step1[20], cospi_12_64, cospi_20_64, &step1[21],
        &step1[26]);
    butterfly_avx2(step1[24], step1[23], -cospi_20_64, cospi_12_64, &step1[22],
        &step1[25]);

    idct32_8x32_quarter_3_4_stage_4_to_7_avx2(step1, out);
}

static void idct32_34_16x32_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i temp[32];

    idct32_34_16x32_quarter_1_2_avx2(in, temp);
    idct32_34_16x32_quarter_3_4_avx2(in, temp);
    // final stage
    add_sub_butterfly_avx2(temp, out, 32);
}

// Only upper-left 8x8 has non-zero coeff
void eb_vp9_idct32x32_34_add_avx2(
    const tran_low_t *input,
    uint8_t          *dest,
    int               stride)
{
    __m128i in[32], col[32];
    __m256i io[32];
    int i;

    // Load input data. Only need to load the top left 8x8 block.
    load_transpose_16bit_8x8(input, 32, in);
    eb_vp9_idct32_34_8x32_ssse3(in, col);

    for (i = 0; i < 32; i += 16) {
        int j;
        transpose_16bit_8x16_avx2(col + i, io);
        idct32_34_16x32_avx2(io, io);

        for (j = 0; j < 32; ++j) {
            write_buffer_16x1_avx2(io[j], dest + j * stride);
        }

        dest += 16;
    }
}

//------------------------------------------------------------------------------

// For each 16x32 block __m256i in[32],
// Input with index, 0, 4, 8, 12
// output pixels: 0-7 in __m256i out[32]
static INLINE void idct32_135_16x32_quarter_1_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i step1[8], step2[8];

    // stage 3
    partial_butterfly_avx2(in[4], cospi_28_64, cospi_4_64, &step1[4], &step1[7]);
    partial_butterfly_avx2(in[12], -cospi_20_64, cospi_12_64, &step1[5],
        &step1[6]);

    // stage 4
    step2[0] = partial_butterfly_cospi16_avx2(in[0]);
    partial_butterfly_avx2(in[8], cospi_24_64, cospi_8_64, &step2[2], &step2[3]);
    step2[4] = _mm256_add_epi16(step1[4], step1[5]);
    step2[5] = _mm256_sub_epi16(step1[4], step1[5]);
    step2[6] = _mm256_sub_epi16(step1[7], step1[6]);
    step2[7] = _mm256_add_epi16(step1[7], step1[6]);

    // stage 5
    step1[0] = _mm256_add_epi16(step2[0], step2[3]);
    step1[1] = _mm256_add_epi16(step2[0], step2[2]);
    step1[2] = _mm256_sub_epi16(step2[0], step2[2]);
    step1[3] = _mm256_sub_epi16(step2[0], step2[3]);
    step1[4] = step2[4];
    butterfly_avx2(step2[6], step2[5], cospi_16_64, cospi_16_64, &step1[5], &step1[6]);
    step1[7] = step2[7];

    // stage 6
    out[0]   = _mm256_add_epi16(step1[0], step1[7]);
    out[1]   = _mm256_add_epi16(step1[1], step1[6]);
    out[2]   = _mm256_add_epi16(step1[2], step1[5]);
    out[3]   = _mm256_add_epi16(step1[3], step1[4]);
    out[4]   = _mm256_sub_epi16(step1[3], step1[4]);
    out[5]   = _mm256_sub_epi16(step1[2], step1[5]);
    out[6]   = _mm256_sub_epi16(step1[1], step1[6]);
    out[7]   = _mm256_sub_epi16(step1[0], step1[7]);
}

// For each 16x32 block __m256i in[32],
// Input with index, 2, 6, 10, 14
// output pixels: 8-15 in __m256i out[32]
static INLINE void idct32_135_16x32_quarter_2_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i step1[16], step2[16];

    // stage 2
    partial_butterfly_avx2(in[2], cospi_30_64, cospi_2_64, &step2[8],
        &step2[15]);
    partial_butterfly_avx2(in[14], -cospi_18_64, cospi_14_64, &step2[9],
        &step2[14]);
    partial_butterfly_avx2(in[10], cospi_22_64, cospi_10_64, &step2[10],
        &step2[13]);
    partial_butterfly_avx2(in[6], -cospi_26_64, cospi_6_64, &step2[11],
        &step2[12]);

    // stage 3
    step1[8] = _mm256_add_epi16(step2[8], step2[9]);
    step1[9] = _mm256_sub_epi16(step2[8], step2[9]);
    step1[10] = _mm256_sub_epi16(step2[11], step2[10]);
    step1[11] = _mm256_add_epi16(step2[11], step2[10]);
    step1[12] = _mm256_add_epi16(step2[12], step2[13]);
    step1[13] = _mm256_sub_epi16(step2[12], step2[13]);
    step1[14] = _mm256_sub_epi16(step2[15], step2[14]);
    step1[15] = _mm256_add_epi16(step2[15], step2[14]);

    idct32_8x32_quarter_2_stage_4_to_6_avx2(step1, out);
}

static INLINE void idct32_135_16x32_quarter_1_2_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i temp[16];
    idct32_135_16x32_quarter_1_avx2(in, temp);
    idct32_135_16x32_quarter_2_avx2(in, temp);
    // stage 7
    add_sub_butterfly_avx2(temp, out, 16);
}

// For each 16x32 block __m256i in[32],
// Input with odd index,
// 1, 3, 5, 7, 9, 11, 13, 15
// output pixels: 16-23, 24-31 in __m256i out[32]
static INLINE void idct32_135_16x32_quarter_3_4_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i step1[32], step2[32];

    // stage 1
    partial_butterfly_avx2(in[1], cospi_31_64, cospi_1_64, &step1[16],
        &step1[31]);
    partial_butterfly_avx2(in[15], -cospi_17_64, cospi_15_64, &step1[17],
        &step1[30]);
    partial_butterfly_avx2(in[9], cospi_23_64, cospi_9_64, &step1[18],
        &step1[29]);
    partial_butterfly_avx2(in[7], -cospi_25_64, cospi_7_64, &step1[19],
        &step1[28]);

    partial_butterfly_avx2(in[5], cospi_27_64, cospi_5_64, &step1[20],
        &step1[27]);
    partial_butterfly_avx2(in[11], -cospi_21_64, cospi_11_64, &step1[21],
        &step1[26]);

    partial_butterfly_avx2(in[13], cospi_19_64, cospi_13_64, &step1[22],
        &step1[25]);
    partial_butterfly_avx2(in[3], -cospi_29_64, cospi_3_64, &step1[23],
        &step1[24]);

    // stage 2
    step2[16] = _mm256_add_epi16(step1[16], step1[17]);
    step2[17] = _mm256_sub_epi16(step1[16], step1[17]);
    step2[18] = _mm256_sub_epi16(step1[19], step1[18]);
    step2[19] = _mm256_add_epi16(step1[19], step1[18]);
    step2[20] = _mm256_add_epi16(step1[20], step1[21]);
    step2[21] = _mm256_sub_epi16(step1[20], step1[21]);
    step2[22] = _mm256_sub_epi16(step1[23], step1[22]);
    step2[23] = _mm256_add_epi16(step1[23], step1[22]);

    step2[24] = _mm256_add_epi16(step1[24], step1[25]);
    step2[25] = _mm256_sub_epi16(step1[24], step1[25]);
    step2[26] = _mm256_sub_epi16(step1[27], step1[26]);
    step2[27] = _mm256_add_epi16(step1[27], step1[26]);
    step2[28] = _mm256_add_epi16(step1[28], step1[29]);
    step2[29] = _mm256_sub_epi16(step1[28], step1[29]);
    step2[30] = _mm256_sub_epi16(step1[31], step1[30]);
    step2[31] = _mm256_add_epi16(step1[31], step1[30]);

    // stage 3
    step1[16] = step2[16];
    step1[31] = step2[31];
    butterfly_avx2(step2[30], step2[17], cospi_28_64, cospi_4_64, &step1[17],
        &step1[30]);
    butterfly_avx2(step2[29], step2[18], -cospi_4_64, cospi_28_64, &step1[18],
        &step1[29]);
    step1[19] = step2[19];
    step1[20] = step2[20];
    butterfly_avx2(step2[26], step2[21], cospi_12_64, cospi_20_64, &step1[21],
        &step1[26]);
    butterfly_avx2(step2[25], step2[22], -cospi_20_64, cospi_12_64, &step1[22],
        &step1[25]);
    step1[23] = step2[23];
    step1[24] = step2[24];
    step1[27] = step2[27];
    step1[28] = step2[28];

    idct32_8x32_quarter_3_4_stage_4_to_7_avx2(step1, out);
}

static void idct32_135_16x32_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i temp[32];
    idct32_135_16x32_quarter_1_2_avx2(in, temp);
    idct32_135_16x32_quarter_3_4_avx2(in, temp);
    // final stage
    add_sub_butterfly_avx2(temp, out, 32);
}

static INLINE void store_buffer_16x32_avx2(
    __m256i *const in,
    uint8_t       *dst,
    const int      stride)
{
    const __m256i final_rounding = _mm256_set1_epi16(1 << 5);
    int j = 0;
    while (j < 32) {
        in[j] = _mm256_adds_epi16(in[j], final_rounding);
        in[j + 1] = _mm256_adds_epi16(in[j + 1], final_rounding);

        in[j] = _mm256_srai_epi16(in[j], 6);
        in[j + 1] = _mm256_srai_epi16(in[j + 1], 6);

        recon_and_store_avx2(in[j], dst);
        dst += stride;
        recon_and_store_avx2(in[j + 1], dst);
        dst += stride;
        j += 2;
    }
}

void eb_vp9_idct32x32_135_add_avx2(
    const tran_low_t *input,
    uint8_t          *dest,
    int               stride)
{
    __m256i col[32], io[32];
    int i;

    // rows
    load_transpose_16bit_16x16(input, 32, io);
    idct32_135_16x32_avx2(io, col);

    // columns
    for (i = 0; i < 32; i += 16) {
        transpose_16bit_16x16_avx2(col + i, io);
        idct32_135_16x32_avx2(io, io);
        store_buffer_16x32_avx2(io, dest, stride);
        dest += 16;
    }
}

//------------------------------------------------------------------------------

// For each 16x32 block __m256i in[32],
// Input with index, 0, 4, 8, 12, 16, 20, 24, 28
// output pixels: 0-7 in __m256i out[32]
static INLINE void idct32_1024_16x32_quarter_1_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i step1[8], step2[8];

    // stage 3
    butterfly_avx2(in[4], in[28], cospi_28_64, cospi_4_64, &step1[4], &step1[7]);
    butterfly_avx2(in[20], in[12], cospi_12_64, cospi_20_64, &step1[5], &step1[6]);

    // stage 4
    butterfly_avx2(in[0], in[16], cospi_16_64, cospi_16_64, &step2[1], &step2[0]);
    butterfly_avx2(in[8], in[24], cospi_24_64, cospi_8_64, &step2[2], &step2[3]);
    step2[4] = _mm256_add_epi16(step1[4], step1[5]);
    step2[5] = _mm256_sub_epi16(step1[4], step1[5]);
    step2[6] = _mm256_sub_epi16(step1[7], step1[6]);
    step2[7] = _mm256_add_epi16(step1[7], step1[6]);

    // stage 5
    step1[0] = _mm256_add_epi16(step2[0], step2[3]);
    step1[1] = _mm256_add_epi16(step2[1], step2[2]);
    step1[2] = _mm256_sub_epi16(step2[1], step2[2]);
    step1[3] = _mm256_sub_epi16(step2[0], step2[3]);
    step1[4] = step2[4];
    butterfly_avx2(step2[6], step2[5], cospi_16_64, cospi_16_64, &step1[5], &step1[6]);
    step1[7] = step2[7];

    // stage 6
    out[0]   = _mm256_add_epi16(step1[0], step1[7]);
    out[1]   = _mm256_add_epi16(step1[1], step1[6]);
    out[2]   = _mm256_add_epi16(step1[2], step1[5]);
    out[3]   = _mm256_add_epi16(step1[3], step1[4]);
    out[4]   = _mm256_sub_epi16(step1[3], step1[4]);
    out[5]   = _mm256_sub_epi16(step1[2], step1[5]);
    out[6]   = _mm256_sub_epi16(step1[1], step1[6]);
    out[7]   = _mm256_sub_epi16(step1[0], step1[7]);
}

// For each 16x32 block __m256i in[32],
// Input with index, 2, 6, 10, 14, 18, 22, 26, 30
// output pixels: 8-15 in __m256i out[32]
static INLINE void idct32_1024_16x32_quarter_2_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i step1[16], step2[16];

    // stage 2
    butterfly_avx2(in[2], in[30], cospi_30_64, cospi_2_64, &step2[8], &step2[15]);
    butterfly_avx2(in[18], in[14], cospi_14_64, cospi_18_64, &step2[9], &step2[14]);
    butterfly_avx2(in[10], in[22], cospi_22_64, cospi_10_64, &step2[10], &step2[13]);
    butterfly_avx2(in[26], in[6], cospi_6_64, cospi_26_64, &step2[11], &step2[12]);

    // stage 3
    step1[8] = _mm256_add_epi16(step2[8], step2[9]);
    step1[9] = _mm256_sub_epi16(step2[8], step2[9]);
    step1[10] = _mm256_sub_epi16(step2[11], step2[10]);
    step1[11] = _mm256_add_epi16(step2[11], step2[10]);
    step1[12] = _mm256_add_epi16(step2[12], step2[13]);
    step1[13] = _mm256_sub_epi16(step2[12], step2[13]);
    step1[14] = _mm256_sub_epi16(step2[15], step2[14]);
    step1[15] = _mm256_add_epi16(step2[15], step2[14]);

    idct32_8x32_quarter_2_stage_4_to_6_avx2(step1, out);
}

static INLINE void idct32_1024_16x32_quarter_1_2_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i temp[16];
    idct32_1024_16x32_quarter_1_avx2(in, temp);
    idct32_1024_16x32_quarter_2_avx2(in, temp);
    // stage 7
    add_sub_butterfly_avx2(temp, out, 16);
}

// For each 16x32 block __m256i in[32],
// Input with odd index,
// 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31
// output pixels: 16-23, 24-31 in __m256i out[32]
static INLINE void idct32_1024_16x32_quarter_3_4_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i step1[32], step2[32];

    // stage 1
    butterfly_avx2(in[1], in[31], cospi_31_64, cospi_1_64, &step1[16], &step1[31]);
    butterfly_avx2(in[17], in[15], cospi_15_64, cospi_17_64, &step1[17], &step1[30]);
    butterfly_avx2(in[9], in[23], cospi_23_64, cospi_9_64, &step1[18], &step1[29]);
    butterfly_avx2(in[25], in[7], cospi_7_64, cospi_25_64, &step1[19], &step1[28]);

    butterfly_avx2(in[5], in[27], cospi_27_64, cospi_5_64, &step1[20], &step1[27]);
    butterfly_avx2(in[21], in[11], cospi_11_64, cospi_21_64, &step1[21], &step1[26]);

    butterfly_avx2(in[13], in[19], cospi_19_64, cospi_13_64, &step1[22], &step1[25]);
    butterfly_avx2(in[29], in[3], cospi_3_64, cospi_29_64, &step1[23], &step1[24]);

    // stage 2
    step2[16] = _mm256_add_epi16(step1[16], step1[17]);
    step2[17] = _mm256_sub_epi16(step1[16], step1[17]);
    step2[18] = _mm256_sub_epi16(step1[19], step1[18]);
    step2[19] = _mm256_add_epi16(step1[19], step1[18]);
    step2[20] = _mm256_add_epi16(step1[20], step1[21]);
    step2[21] = _mm256_sub_epi16(step1[20], step1[21]);
    step2[22] = _mm256_sub_epi16(step1[23], step1[22]);
    step2[23] = _mm256_add_epi16(step1[23], step1[22]);

    step2[24] = _mm256_add_epi16(step1[24], step1[25]);
    step2[25] = _mm256_sub_epi16(step1[24], step1[25]);
    step2[26] = _mm256_sub_epi16(step1[27], step1[26]);
    step2[27] = _mm256_add_epi16(step1[27], step1[26]);
    step2[28] = _mm256_add_epi16(step1[28], step1[29]);
    step2[29] = _mm256_sub_epi16(step1[28], step1[29]);
    step2[30] = _mm256_sub_epi16(step1[31], step1[30]);
    step2[31] = _mm256_add_epi16(step1[31], step1[30]);

    // stage 3
    step1[16] = step2[16];
    step1[31] = step2[31];
    butterfly_avx2(step2[30], step2[17], cospi_28_64, cospi_4_64, &step1[17],
        &step1[30]);
    butterfly_avx2(step2[29], step2[18], -cospi_4_64, cospi_28_64, &step1[18],
        &step1[29]);
    step1[19] = step2[19];
    step1[20] = step2[20];
    butterfly_avx2(step2[26], step2[21], cospi_12_64, cospi_20_64, &step1[21],
        &step1[26]);
    butterfly_avx2(step2[25], step2[22], -cospi_20_64, cospi_12_64, &step1[22],
        &step1[25]);
    step1[23] = step2[23];
    step1[24] = step2[24];
    step1[27] = step2[27];
    step1[28] = step2[28];

    idct32_8x32_quarter_3_4_stage_4_to_7_avx2(step1, out);
}

static void idct32_1024_16x32_avx2(
    const __m256i *const in,
    __m256i *const       out)
{
    __m256i temp[32];

    idct32_1024_16x32_quarter_1_2_avx2(in, temp);
    idct32_1024_16x32_quarter_3_4_avx2(in, temp);
    // final stage
    add_sub_butterfly_avx2(temp, out, 32);
}

void vpx_idct32x32_1024_add_avx2(
    const tran_low_t *input,
    uint8_t          *dest,
    int               stride)
{
    __m256i col[2][32], io[32];
    int i;

    // rows
    for (i = 0; i < 2; i++) {
        load_transpose_16bit_16x16(input + 0, 32, io + 0);
        load_transpose_16bit_16x16(input + 16, 32, io + 16);
        idct32_1024_16x32_avx2(io, col[i]);
        input += 32 << 4;
    }

    // columns
    for (i = 0; i < 32; i += 16) {
        // Transpose 32x16 block to 16x32 block
        transpose_16bit_16x16_avx2(col[0] + i, io + 0);
        transpose_16bit_16x16_avx2(col[1] + i, io + 16);

        idct32_1024_16x32_avx2(io, io);
        store_buffer_16x32_avx2(io, dest, stride);
        dest += 16;
    }
}
