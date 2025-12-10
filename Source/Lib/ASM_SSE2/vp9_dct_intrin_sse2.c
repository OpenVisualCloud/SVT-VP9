/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>
#include <emmintrin.h> // SSE2

#include "vp9_rtcd.h"
#include "vpx_dsp_rtcd.h"
#include "txfm_common.h"
#include "fwd_txfm_sse2.h"
#include "mem_sse2.h"
#include "transpose_sse2.h"
#include "txfm_common_sse2.h"

// TODO(jingning) The high bit-depth version needs re-work for performance.
// The current SSE2 implementation also causes cross reference to the static
// functions in the C implementation file.
#define ADD_EPI16 _mm_add_epi16
#define SUB_EPI16 _mm_sub_epi16
static inline void load_buffer_4x4(const int16_t *input, __m128i *in, int stride) {
    const __m128i k__nonzero_bias_a = _mm_setr_epi16(0, 1, 1, 1, 1, 1, 1, 1);
    const __m128i k__nonzero_bias_b = _mm_setr_epi16(1, 0, 0, 0, 0, 0, 0, 0);
    __m128i       mask;

    in[0] = _mm_loadl_epi64((const __m128i *)(input + 0 * stride));
    in[1] = _mm_loadl_epi64((const __m128i *)(input + 1 * stride));
    in[2] = _mm_loadl_epi64((const __m128i *)(input + 2 * stride));
    in[3] = _mm_loadl_epi64((const __m128i *)(input + 3 * stride));

    in[0] = _mm_slli_epi16(in[0], 4);
    in[1] = _mm_slli_epi16(in[1], 4);
    in[2] = _mm_slli_epi16(in[2], 4);
    in[3] = _mm_slli_epi16(in[3], 4);

    mask  = _mm_cmpeq_epi16(in[0], k__nonzero_bias_a);
    in[0] = _mm_add_epi16(in[0], mask);
    in[0] = _mm_add_epi16(in[0], k__nonzero_bias_b);
}

static inline void write_buffer_4x4(tran_low_t *output, __m128i *res) {
    const __m128i kOne  = _mm_set1_epi16(1);
    __m128i       in01  = _mm_unpacklo_epi64(res[0], res[1]);
    __m128i       in23  = _mm_unpacklo_epi64(res[2], res[3]);
    __m128i       out01 = _mm_add_epi16(in01, kOne);
    __m128i       out23 = _mm_add_epi16(in23, kOne);
    out01               = _mm_srai_epi16(out01, 2);
    out23               = _mm_srai_epi16(out23, 2);
    store_output(out01, output + 0 * 8);
    store_output(out23, output + 1 * 8);
}

static inline void transpose_4x4(__m128i *res) {
    // Combine and transpose
    // 00 01 02 03 20 21 22 23
    // 10 11 12 13 30 31 32 33
    const __m128i tr0_0 = _mm_unpacklo_epi16(res[0], res[1]);
    const __m128i tr0_1 = _mm_unpackhi_epi16(res[0], res[1]);

    // 00 10 01 11 02 12 03 13
    // 20 30 21 31 22 32 23 33
    res[0] = _mm_unpacklo_epi32(tr0_0, tr0_1);
    res[2] = _mm_unpackhi_epi32(tr0_0, tr0_1);

    // 00 10 20 30 01 11 21 31
    // 02 12 22 32 03 13 23 33
    // only use the first 4 16-bit integers
    res[1] = _mm_unpackhi_epi64(res[0], res[0]);
    res[3] = _mm_unpackhi_epi64(res[2], res[2]);
}

static void fdct4_sse2(__m128i *in) {
    const __m128i k__cospi_p16_p16      = _mm_set1_epi16(cospi_16_64);
    const __m128i k__cospi_p16_m16      = pair_set_epi16(cospi_16_64, -cospi_16_64);
    const __m128i k__cospi_p08_p24      = pair_set_epi16(cospi_8_64, cospi_24_64);
    const __m128i k__cospi_p24_m08      = pair_set_epi16(cospi_24_64, -cospi_8_64);
    const __m128i k__DCT_CONST_ROUNDING = _mm_set1_epi32(DCT_CONST_ROUNDING);

    __m128i u[4], v[4];
    u[0] = _mm_unpacklo_epi16(in[0], in[1]);
    u[1] = _mm_unpacklo_epi16(in[3], in[2]);

    v[0] = _mm_add_epi16(u[0], u[1]);
    v[1] = _mm_sub_epi16(u[0], u[1]);

    u[0] = _mm_madd_epi16(v[0], k__cospi_p16_p16); // 0
    u[1] = _mm_madd_epi16(v[0], k__cospi_p16_m16); // 2
    u[2] = _mm_madd_epi16(v[1], k__cospi_p08_p24); // 1
    u[3] = _mm_madd_epi16(v[1], k__cospi_p24_m08); // 3

    v[0] = _mm_add_epi32(u[0], k__DCT_CONST_ROUNDING);
    v[1] = _mm_add_epi32(u[1], k__DCT_CONST_ROUNDING);
    v[2] = _mm_add_epi32(u[2], k__DCT_CONST_ROUNDING);
    v[3] = _mm_add_epi32(u[3], k__DCT_CONST_ROUNDING);
    u[0] = _mm_srai_epi32(v[0], DCT_CONST_BITS);
    u[1] = _mm_srai_epi32(v[1], DCT_CONST_BITS);
    u[2] = _mm_srai_epi32(v[2], DCT_CONST_BITS);
    u[3] = _mm_srai_epi32(v[3], DCT_CONST_BITS);

    in[0] = _mm_packs_epi32(u[0], u[1]);
    in[1] = _mm_packs_epi32(u[2], u[3]);
    transpose_4x4(in);
}

static void fadst4_sse2(__m128i *in) {
    const __m128i k__sinpi_p01_p02      = pair_set_epi16(sinpi_1_9, sinpi_2_9);
    const __m128i k__sinpi_p04_m01      = pair_set_epi16(sinpi_4_9, -sinpi_1_9);
    const __m128i k__sinpi_p03_p04      = pair_set_epi16(sinpi_3_9, sinpi_4_9);
    const __m128i k__sinpi_m03_p02      = pair_set_epi16(-sinpi_3_9, sinpi_2_9);
    const __m128i k__sinpi_p03_p03      = _mm_set1_epi16((int16_t)sinpi_3_9);
    const __m128i kZero                 = _mm_set1_epi16(0);
    const __m128i k__DCT_CONST_ROUNDING = _mm_set1_epi32(DCT_CONST_ROUNDING);
    __m128i       u[8], v[8];
    __m128i       in7 = _mm_add_epi16(in[0], in[1]);

    u[0] = _mm_unpacklo_epi16(in[0], in[1]);
    u[1] = _mm_unpacklo_epi16(in[2], in[3]);
    u[2] = _mm_unpacklo_epi16(in7, kZero);
    u[3] = _mm_unpacklo_epi16(in[2], kZero);
    u[4] = _mm_unpacklo_epi16(in[3], kZero);

    v[0] = _mm_madd_epi16(u[0], k__sinpi_p01_p02); // s0 + s2
    v[1] = _mm_madd_epi16(u[1], k__sinpi_p03_p04); // s4 + s5
    v[2] = _mm_madd_epi16(u[2], k__sinpi_p03_p03); // x1
    v[3] = _mm_madd_epi16(u[0], k__sinpi_p04_m01); // s1 - s3
    v[4] = _mm_madd_epi16(u[1], k__sinpi_m03_p02); // -s4 + s6
    v[5] = _mm_madd_epi16(u[3], k__sinpi_p03_p03); // s4
    v[6] = _mm_madd_epi16(u[4], k__sinpi_p03_p03);

    u[0] = _mm_add_epi32(v[0], v[1]);
    u[1] = _mm_sub_epi32(v[2], v[6]);
    u[2] = _mm_add_epi32(v[3], v[4]);
    u[3] = _mm_sub_epi32(u[2], u[0]);
    u[4] = _mm_slli_epi32(v[5], 2);
    u[5] = _mm_sub_epi32(u[4], v[5]);
    u[6] = _mm_add_epi32(u[3], u[5]);

    v[0] = _mm_add_epi32(u[0], k__DCT_CONST_ROUNDING);
    v[1] = _mm_add_epi32(u[1], k__DCT_CONST_ROUNDING);
    v[2] = _mm_add_epi32(u[2], k__DCT_CONST_ROUNDING);
    v[3] = _mm_add_epi32(u[6], k__DCT_CONST_ROUNDING);

    u[0] = _mm_srai_epi32(v[0], DCT_CONST_BITS);
    u[1] = _mm_srai_epi32(v[1], DCT_CONST_BITS);
    u[2] = _mm_srai_epi32(v[2], DCT_CONST_BITS);
    u[3] = _mm_srai_epi32(v[3], DCT_CONST_BITS);

    in[0] = _mm_packs_epi32(u[0], u[2]);
    in[1] = _mm_packs_epi32(u[1], u[3]);
    transpose_4x4(in);
}
void eb_vp9_fdct4x4_sse2(const int16_t *input, tran_low_t *output, int stride) {
    // This 2D transform implements 4 vertical 1D transforms followed
    // by 4 horizontal 1D transforms.  The multiplies and adds are as given
    // by Chen, Smith and Fralick ('77).  The commands for moving the data
    // around have been minimized by hand.
    // For the purposes of the comments, the 16 inputs are referred to at i0
    // through iF (in raster order), intermediate variables are a0, b0, c0
    // through f, and correspond to the in-place computations mapped to input
    // locations.  The outputs, o0 through oF are labeled according to the
    // output locations.

    // Constants
    // These are the coefficients used for the multiplies.
    // In the comments, pN means cos(N pi /64) and mN is -cos(N pi /64),
    // where cospi_N_64 = cos(N pi /64)
    const __m128i k__cospi_A = octa_set_epi16(
        cospi_16_64, cospi_16_64, cospi_16_64, cospi_16_64, cospi_16_64, -cospi_16_64, cospi_16_64, -cospi_16_64);
    const __m128i k__cospi_B = octa_set_epi16(
        cospi_16_64, -cospi_16_64, cospi_16_64, -cospi_16_64, cospi_16_64, cospi_16_64, cospi_16_64, cospi_16_64);
    const __m128i k__cospi_C = octa_set_epi16(
        cospi_8_64, cospi_24_64, cospi_8_64, cospi_24_64, cospi_24_64, -cospi_8_64, cospi_24_64, -cospi_8_64);
    const __m128i k__cospi_D = octa_set_epi16(
        cospi_24_64, -cospi_8_64, cospi_24_64, -cospi_8_64, cospi_8_64, cospi_24_64, cospi_8_64, cospi_24_64);
    const __m128i k__cospi_E = octa_set_epi16(
        cospi_16_64, cospi_16_64, cospi_16_64, cospi_16_64, cospi_16_64, cospi_16_64, cospi_16_64, cospi_16_64);
    const __m128i k__cospi_F = octa_set_epi16(
        cospi_16_64, -cospi_16_64, cospi_16_64, -cospi_16_64, cospi_16_64, -cospi_16_64, cospi_16_64, -cospi_16_64);
    const __m128i k__cospi_G = octa_set_epi16(
        cospi_8_64, cospi_24_64, cospi_8_64, cospi_24_64, -cospi_8_64, -cospi_24_64, -cospi_8_64, -cospi_24_64);
    const __m128i k__cospi_H = octa_set_epi16(
        cospi_24_64, -cospi_8_64, cospi_24_64, -cospi_8_64, -cospi_24_64, cospi_8_64, -cospi_24_64, cospi_8_64);

    const __m128i k__DCT_CONST_ROUNDING = _mm_set1_epi32(DCT_CONST_ROUNDING);
    // This second rounding constant saves doing some extra adds at the end
    const __m128i k__DCT_CONST_ROUNDING2 = _mm_set1_epi32(DCT_CONST_ROUNDING + (DCT_CONST_ROUNDING << 1));
    const int     DCT_CONST_BITS2        = DCT_CONST_BITS + 2;
    const __m128i k__nonzero_bias_a      = _mm_setr_epi16(0, 1, 1, 1, 1, 1, 1, 1);
    const __m128i k__nonzero_bias_b      = _mm_setr_epi16(1, 0, 0, 0, 0, 0, 0, 0);
    __m128i       in0, in1;

    // Load inputs.
    in0 = _mm_loadl_epi64((const __m128i *)(input + 0 * stride));
    in1 = _mm_loadl_epi64((const __m128i *)(input + 1 * stride));
    in1 = _mm_unpacklo_epi64(in1, _mm_loadl_epi64((const __m128i *)(input + 2 * stride)));
    in0 = _mm_unpacklo_epi64(in0, _mm_loadl_epi64((const __m128i *)(input + 3 * stride)));
    // in0 = [i0 i1 i2 i3 iC iD iE iF]
    // in1 = [i4 i5 i6 i7 i8 i9 iA iB]

    // multiply by 16 to give some extra precision
    in0 = _mm_slli_epi16(in0, 4);
    in1 = _mm_slli_epi16(in1, 4);
    // if (i == 0 && input[0]) input[0] += 1;
    // add 1 to the upper left pixel if it is non-zero, which helps reduce
    // the round-trip error
    {
        // The mask will only contain whether the first value is zero, all
        // other comparison will fail as something shifted by 4 (above << 4)
        // can never be equal to one. To increment in the non-zero case, we
        // add the mask and one for the first element:
        //   - if zero, mask = -1, v = v - 1 + 1 = v
        //   - if non-zero, mask = 0, v = v + 0 + 1 = v + 1
        __m128i mask = _mm_cmpeq_epi16(in0, k__nonzero_bias_a);
        in0          = _mm_add_epi16(in0, mask);
        in0          = _mm_add_epi16(in0, k__nonzero_bias_b);
    }
    // There are 4 total stages, alternating between an add/subtract stage
    // followed by an multiply-and-add stage.
    {
        // Stage 1: Add/subtract

        // in0 = [i0 i1 i2 i3 iC iD iE iF]
        // in1 = [i4 i5 i6 i7 i8 i9 iA iB]
        const __m128i r0 = _mm_unpacklo_epi16(in0, in1);
        const __m128i r1 = _mm_unpackhi_epi16(in0, in1);
        // r0 = [i0 i4 i1 i5 i2 i6 i3 i7]
        // r1 = [iC i8 iD i9 iE iA iF iB]
        const __m128i r2 = _mm_shuffle_epi32(r0, 0xB4);
        const __m128i r3 = _mm_shuffle_epi32(r1, 0xB4);
        // r2 = [i0 i4 i1 i5 i3 i7 i2 i6]
        // r3 = [iC i8 iD i9 iF iB iE iA]

        const __m128i t0 = _mm_add_epi16(r2, r3);
        const __m128i t1 = _mm_sub_epi16(r2, r3);
        // t0 = [a0 a4 a1 a5 a3 a7 a2 a6]
        // t1 = [aC a8 aD a9 aF aB aE aA]

        // Stage 2: multiply by constants (which gets us into 32 bits).
        // The constants needed here are:
        // k__cospi_A = [p16 p16 p16 p16 p16 m16 p16 m16]
        // k__cospi_B = [p16 m16 p16 m16 p16 p16 p16 p16]
        // k__cospi_C = [p08 p24 p08 p24 p24 m08 p24 m08]
        // k__cospi_D = [p24 m08 p24 m08 p08 p24 p08 p24]
        const __m128i u0 = _mm_madd_epi16(t0, k__cospi_A);
        const __m128i u2 = _mm_madd_epi16(t0, k__cospi_B);
        const __m128i u1 = _mm_madd_epi16(t1, k__cospi_C);
        const __m128i u3 = _mm_madd_epi16(t1, k__cospi_D);
        // Then add and right-shift to get back to 16-bit range
        const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
        const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
        const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
        const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
        const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
        const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
        const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
        const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
        // w0 = [b0 b1 b7 b6]
        // w1 = [b8 b9 bF bE]
        // w2 = [b4 b5 b3 b2]
        // w3 = [bC bD bB bA]
        const __m128i x0 = _mm_packs_epi32(w0, w1);
        const __m128i x1 = _mm_packs_epi32(w2, w3);
        // x0 = [b0 b1 b7 b6 b8 b9 bF bE]
        // x1 = [b4 b5 b3 b2 bC bD bB bA]
        in0 = _mm_shuffle_epi32(x0, 0xD8);
        in1 = _mm_shuffle_epi32(x1, 0x8D);
        // in0 = [b0 b1 b8 b9 b7 b6 bF bE]
        // in1 = [b3 b2 bB bA b4 b5 bC bD]
    }
    {
        // vertical DCTs finished. Now we do the horizontal DCTs.
        // Stage 3: Add/subtract

        const __m128i t0 = ADD_EPI16(in0, in1);
        const __m128i t1 = SUB_EPI16(in0, in1);
        // t0 = [c0 c1 c8 c9  c4  c5  cC  cD]
        // t1 = [c3 c2 cB cA -c7 -c6 -cF -cE]

        // Stage 4: multiply by constants (which gets us into 32 bits).
        {
            // The constants needed here are:
            // k__cospi_E = [p16 p16 p16 p16 p16 p16 p16 p16]
            // k__cospi_F = [p16 m16 p16 m16 p16 m16 p16 m16]
            // k__cospi_G = [p08 p24 p08 p24 m08 m24 m08 m24]
            // k__cospi_H = [p24 m08 p24 m08 m24 p08 m24 p08]
            const __m128i u0 = _mm_madd_epi16(t0, k__cospi_E);
            const __m128i u1 = _mm_madd_epi16(t0, k__cospi_F);
            const __m128i u2 = _mm_madd_epi16(t1, k__cospi_G);
            const __m128i u3 = _mm_madd_epi16(t1, k__cospi_H);
            // Then add and right-shift to get back to 16-bit range
            // but this combines the final right-shift as well to save operations
            // This unusual rounding operations is to maintain bit-accurate
            // compatibility with the c version of this function which has two
            // rounding steps in a row.
            const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING2);
            const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING2);
            const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING2);
            const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING2);
            const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS2);
            const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS2);
            const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS2);
            const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS2);
            // w0 = [o0 o4 o8 oC]
            // w1 = [o2 o6 oA oE]
            // w2 = [o1 o5 o9 oD]
            // w3 = [o3 o7 oB oF]
            // remember the o's are numbered according to the correct output location
            const __m128i x0 = _mm_packs_epi32(w0, w1);
            const __m128i x1 = _mm_packs_epi32(w2, w3);
            {
                // x0 = [o0 o4 o8 oC o2 o6 oA oE]
                // x1 = [o1 o5 o9 oD o3 o7 oB oF]
                const __m128i y0 = _mm_unpacklo_epi16(x0, x1);
                const __m128i y1 = _mm_unpackhi_epi16(x0, x1);
                // y0 = [o0 o1 o4 o5 o8 o9 oC oD]
                // y1 = [o2 o3 o6 o7 oA oB oE oF]
                in0 = _mm_unpacklo_epi32(y0, y1);
                // in0 = [o0 o1 o2 o3 o4 o5 o6 o7]
                in1 = _mm_unpackhi_epi32(y0, y1);
                // in1 = [o8 o9 oA oB oC oD oE oF]
            }
        }
    }
    // Post-condition (v + 1) >> 2 is now incorporated into previous
    // add and right-shift commands.  Only 2 store instructions needed
    // because we are using the fact that 1/3 are stored just after 0/2.
    storeu_output(in0, output + 0 * 4);
    storeu_output(in1, output + 2 * 4);
}

void eb_vp9_fht4x4_sse2(const int16_t *input, tran_low_t *output, int stride, int tx_type) {
    __m128i in[4];

    switch (tx_type) {
    case DCT_DCT: eb_vp9_fdct4x4_sse2(input, output, stride); break;
    case ADST_DCT:
        load_buffer_4x4(input, in, stride);
        fadst4_sse2(in);
        fdct4_sse2(in);
        write_buffer_4x4(output, in);
        break;
    case DCT_ADST:
        load_buffer_4x4(input, in, stride);
        fdct4_sse2(in);
        fadst4_sse2(in);
        write_buffer_4x4(output, in);
        break;
    default:
        assert(tx_type == ADST_ADST);
        load_buffer_4x4(input, in, stride);
        fadst4_sse2(in);
        fadst4_sse2(in);
        write_buffer_4x4(output, in);
        break;
    }
}

void eb_vp9_fdct8x8_quant_sse2(const int16_t *input, int stride, int16_t *coeff_ptr, intptr_t n_coeffs, int skip_block,
                               const int16_t *round_ptr, const int16_t *quant_ptr, int16_t *qcoeff_ptr,
                               int16_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr,
                               const int16_t *scan_ptr, const int16_t *iscan_ptr) {
    __m128i zero;
    int     pass;

    // Constants
    //    When we use them, in one case, they are all the same. In all others
    //    it's a pair of them that we need to repeat four times. This is done
    //    by constructing the 32 bit constant corresponding to that pair.
    const __m128i k__cospi_p16_p16      = _mm_set1_epi16(cospi_16_64);
    const __m128i k__cospi_p16_m16      = pair_set_epi16(cospi_16_64, -cospi_16_64);
    const __m128i k__cospi_p24_p08      = pair_set_epi16(cospi_24_64, cospi_8_64);
    const __m128i k__cospi_m08_p24      = pair_set_epi16(-cospi_8_64, cospi_24_64);
    const __m128i k__cospi_p28_p04      = pair_set_epi16(cospi_28_64, cospi_4_64);
    const __m128i k__cospi_m04_p28      = pair_set_epi16(-cospi_4_64, cospi_28_64);
    const __m128i k__cospi_p12_p20      = pair_set_epi16(cospi_12_64, cospi_20_64);
    const __m128i k__cospi_m20_p12      = pair_set_epi16(-cospi_20_64, cospi_12_64);
    const __m128i k__DCT_CONST_ROUNDING = _mm_set1_epi32(DCT_CONST_ROUNDING);
    // Load input
    __m128i  in0 = _mm_load_si128((const __m128i *)(input + 0 * stride));
    __m128i  in1 = _mm_load_si128((const __m128i *)(input + 1 * stride));
    __m128i  in2 = _mm_load_si128((const __m128i *)(input + 2 * stride));
    __m128i  in3 = _mm_load_si128((const __m128i *)(input + 3 * stride));
    __m128i  in4 = _mm_load_si128((const __m128i *)(input + 4 * stride));
    __m128i  in5 = _mm_load_si128((const __m128i *)(input + 5 * stride));
    __m128i  in6 = _mm_load_si128((const __m128i *)(input + 6 * stride));
    __m128i  in7 = _mm_load_si128((const __m128i *)(input + 7 * stride));
    __m128i *in[8];
    int      index = 0;

    (void)scan_ptr;
    (void)coeff_ptr;

    // Pre-condition input (shift by two)
    in0 = _mm_slli_epi16(in0, 2);
    in1 = _mm_slli_epi16(in1, 2);
    in2 = _mm_slli_epi16(in2, 2);
    in3 = _mm_slli_epi16(in3, 2);
    in4 = _mm_slli_epi16(in4, 2);
    in5 = _mm_slli_epi16(in5, 2);
    in6 = _mm_slli_epi16(in6, 2);
    in7 = _mm_slli_epi16(in7, 2);

    in[0] = &in0;
    in[1] = &in1;
    in[2] = &in2;
    in[3] = &in3;
    in[4] = &in4;
    in[5] = &in5;
    in[6] = &in6;
    in[7] = &in7;

    // We do two passes, first the columns, then the rows. The results of the
    // first pass are transposed so that the same column code can be reused. The
    // results of the second pass are also transposed so that the rows (processed
    // as columns) are put back in row positions.
    for (pass = 0; pass < 2; pass++) {
        // To store results of each pass before the transpose.
        __m128i res0, res1, res2, res3, res4, res5, res6, res7;
        // Add/subtract
        const __m128i q0 = _mm_add_epi16(in0, in7);
        const __m128i q1 = _mm_add_epi16(in1, in6);
        const __m128i q2 = _mm_add_epi16(in2, in5);
        const __m128i q3 = _mm_add_epi16(in3, in4);
        const __m128i q4 = _mm_sub_epi16(in3, in4);
        const __m128i q5 = _mm_sub_epi16(in2, in5);
        const __m128i q6 = _mm_sub_epi16(in1, in6);
        const __m128i q7 = _mm_sub_epi16(in0, in7);
        // Work on first four results
        {
            // Add/subtract
            const __m128i r0 = _mm_add_epi16(q0, q3);
            const __m128i r1 = _mm_add_epi16(q1, q2);
            const __m128i r2 = _mm_sub_epi16(q1, q2);
            const __m128i r3 = _mm_sub_epi16(q0, q3);
            // Interleave to do the multiply by constants which gets us into 32bits
            const __m128i t0 = _mm_unpacklo_epi16(r0, r1);
            const __m128i t1 = _mm_unpackhi_epi16(r0, r1);
            const __m128i t2 = _mm_unpacklo_epi16(r2, r3);
            const __m128i t3 = _mm_unpackhi_epi16(r2, r3);
            const __m128i u0 = _mm_madd_epi16(t0, k__cospi_p16_p16);
            const __m128i u1 = _mm_madd_epi16(t1, k__cospi_p16_p16);
            const __m128i u2 = _mm_madd_epi16(t0, k__cospi_p16_m16);
            const __m128i u3 = _mm_madd_epi16(t1, k__cospi_p16_m16);
            const __m128i u4 = _mm_madd_epi16(t2, k__cospi_p24_p08);
            const __m128i u5 = _mm_madd_epi16(t3, k__cospi_p24_p08);
            const __m128i u6 = _mm_madd_epi16(t2, k__cospi_m08_p24);
            const __m128i u7 = _mm_madd_epi16(t3, k__cospi_m08_p24);
            // dct_const_round_shift
            const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
            const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
            const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
            const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
            const __m128i v4 = _mm_add_epi32(u4, k__DCT_CONST_ROUNDING);
            const __m128i v5 = _mm_add_epi32(u5, k__DCT_CONST_ROUNDING);
            const __m128i v6 = _mm_add_epi32(u6, k__DCT_CONST_ROUNDING);
            const __m128i v7 = _mm_add_epi32(u7, k__DCT_CONST_ROUNDING);
            const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
            const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
            const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
            const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
            const __m128i w4 = _mm_srai_epi32(v4, DCT_CONST_BITS);
            const __m128i w5 = _mm_srai_epi32(v5, DCT_CONST_BITS);
            const __m128i w6 = _mm_srai_epi32(v6, DCT_CONST_BITS);
            const __m128i w7 = _mm_srai_epi32(v7, DCT_CONST_BITS);
            // Combine
            res0 = _mm_packs_epi32(w0, w1);
            res4 = _mm_packs_epi32(w2, w3);
            res2 = _mm_packs_epi32(w4, w5);
            res6 = _mm_packs_epi32(w6, w7);
        }
        // Work on next four results
        {
            // Interleave to do the multiply by constants which gets us into 32bits
            const __m128i d0 = _mm_unpacklo_epi16(q6, q5);
            const __m128i d1 = _mm_unpackhi_epi16(q6, q5);
            const __m128i e0 = _mm_madd_epi16(d0, k__cospi_p16_m16);
            const __m128i e1 = _mm_madd_epi16(d1, k__cospi_p16_m16);
            const __m128i e2 = _mm_madd_epi16(d0, k__cospi_p16_p16);
            const __m128i e3 = _mm_madd_epi16(d1, k__cospi_p16_p16);
            // dct_const_round_shift
            const __m128i f0 = _mm_add_epi32(e0, k__DCT_CONST_ROUNDING);
            const __m128i f1 = _mm_add_epi32(e1, k__DCT_CONST_ROUNDING);
            const __m128i f2 = _mm_add_epi32(e2, k__DCT_CONST_ROUNDING);
            const __m128i f3 = _mm_add_epi32(e3, k__DCT_CONST_ROUNDING);
            const __m128i s0 = _mm_srai_epi32(f0, DCT_CONST_BITS);
            const __m128i s1 = _mm_srai_epi32(f1, DCT_CONST_BITS);
            const __m128i s2 = _mm_srai_epi32(f2, DCT_CONST_BITS);
            const __m128i s3 = _mm_srai_epi32(f3, DCT_CONST_BITS);
            // Combine
            const __m128i r0 = _mm_packs_epi32(s0, s1);
            const __m128i r1 = _mm_packs_epi32(s2, s3);
            // Add/subtract
            const __m128i x0 = _mm_add_epi16(q4, r0);
            const __m128i x1 = _mm_sub_epi16(q4, r0);
            const __m128i x2 = _mm_sub_epi16(q7, r1);
            const __m128i x3 = _mm_add_epi16(q7, r1);
            // Interleave to do the multiply by constants which gets us into 32bits
            const __m128i t0 = _mm_unpacklo_epi16(x0, x3);
            const __m128i t1 = _mm_unpackhi_epi16(x0, x3);
            const __m128i t2 = _mm_unpacklo_epi16(x1, x2);
            const __m128i t3 = _mm_unpackhi_epi16(x1, x2);
            const __m128i u0 = _mm_madd_epi16(t0, k__cospi_p28_p04);
            const __m128i u1 = _mm_madd_epi16(t1, k__cospi_p28_p04);
            const __m128i u2 = _mm_madd_epi16(t0, k__cospi_m04_p28);
            const __m128i u3 = _mm_madd_epi16(t1, k__cospi_m04_p28);
            const __m128i u4 = _mm_madd_epi16(t2, k__cospi_p12_p20);
            const __m128i u5 = _mm_madd_epi16(t3, k__cospi_p12_p20);
            const __m128i u6 = _mm_madd_epi16(t2, k__cospi_m20_p12);
            const __m128i u7 = _mm_madd_epi16(t3, k__cospi_m20_p12);
            // dct_const_round_shift
            const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
            const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
            const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
            const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
            const __m128i v4 = _mm_add_epi32(u4, k__DCT_CONST_ROUNDING);
            const __m128i v5 = _mm_add_epi32(u5, k__DCT_CONST_ROUNDING);
            const __m128i v6 = _mm_add_epi32(u6, k__DCT_CONST_ROUNDING);
            const __m128i v7 = _mm_add_epi32(u7, k__DCT_CONST_ROUNDING);
            const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
            const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
            const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
            const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
            const __m128i w4 = _mm_srai_epi32(v4, DCT_CONST_BITS);
            const __m128i w5 = _mm_srai_epi32(v5, DCT_CONST_BITS);
            const __m128i w6 = _mm_srai_epi32(v6, DCT_CONST_BITS);
            const __m128i w7 = _mm_srai_epi32(v7, DCT_CONST_BITS);
            // Combine
            res1 = _mm_packs_epi32(w0, w1);
            res7 = _mm_packs_epi32(w2, w3);
            res5 = _mm_packs_epi32(w4, w5);
            res3 = _mm_packs_epi32(w6, w7);
        }
        // Transpose the 8x8.
        {
            // 00 01 02 03 04 05 06 07
            // 10 11 12 13 14 15 16 17
            // 20 21 22 23 24 25 26 27
            // 30 31 32 33 34 35 36 37
            // 40 41 42 43 44 45 46 47
            // 50 51 52 53 54 55 56 57
            // 60 61 62 63 64 65 66 67
            // 70 71 72 73 74 75 76 77
            const __m128i tr0_0 = _mm_unpacklo_epi16(res0, res1);
            const __m128i tr0_1 = _mm_unpacklo_epi16(res2, res3);
            const __m128i tr0_2 = _mm_unpackhi_epi16(res0, res1);
            const __m128i tr0_3 = _mm_unpackhi_epi16(res2, res3);
            const __m128i tr0_4 = _mm_unpacklo_epi16(res4, res5);
            const __m128i tr0_5 = _mm_unpacklo_epi16(res6, res7);
            const __m128i tr0_6 = _mm_unpackhi_epi16(res4, res5);
            const __m128i tr0_7 = _mm_unpackhi_epi16(res6, res7);
            // 00 10 01 11 02 12 03 13
            // 20 30 21 31 22 32 23 33
            // 04 14 05 15 06 16 07 17
            // 24 34 25 35 26 36 27 37
            // 40 50 41 51 42 52 43 53
            // 60 70 61 71 62 72 63 73
            // 54 54 55 55 56 56 57 57
            // 64 74 65 75 66 76 67 77
            const __m128i tr1_0 = _mm_unpacklo_epi32(tr0_0, tr0_1);
            const __m128i tr1_1 = _mm_unpacklo_epi32(tr0_2, tr0_3);
            const __m128i tr1_2 = _mm_unpackhi_epi32(tr0_0, tr0_1);
            const __m128i tr1_3 = _mm_unpackhi_epi32(tr0_2, tr0_3);
            const __m128i tr1_4 = _mm_unpacklo_epi32(tr0_4, tr0_5);
            const __m128i tr1_5 = _mm_unpacklo_epi32(tr0_6, tr0_7);
            const __m128i tr1_6 = _mm_unpackhi_epi32(tr0_4, tr0_5);
            const __m128i tr1_7 = _mm_unpackhi_epi32(tr0_6, tr0_7);
            // 00 10 20 30 01 11 21 31
            // 40 50 60 70 41 51 61 71
            // 02 12 22 32 03 13 23 33
            // 42 52 62 72 43 53 63 73
            // 04 14 24 34 05 15 21 36
            // 44 54 64 74 45 55 61 76
            // 06 16 26 36 07 17 27 37
            // 46 56 66 76 47 57 67 77
            in0 = _mm_unpacklo_epi64(tr1_0, tr1_4);
            in1 = _mm_unpackhi_epi64(tr1_0, tr1_4);
            in2 = _mm_unpacklo_epi64(tr1_2, tr1_6);
            in3 = _mm_unpackhi_epi64(tr1_2, tr1_6);
            in4 = _mm_unpacklo_epi64(tr1_1, tr1_5);
            in5 = _mm_unpackhi_epi64(tr1_1, tr1_5);
            in6 = _mm_unpacklo_epi64(tr1_3, tr1_7);
            in7 = _mm_unpackhi_epi64(tr1_3, tr1_7);
            // 00 10 20 30 40 50 60 70
            // 01 11 21 31 41 51 61 71
            // 02 12 22 32 42 52 62 72
            // 03 13 23 33 43 53 63 73
            // 04 14 24 34 44 54 64 74
            // 05 15 25 35 45 55 65 75
            // 06 16 26 36 46 56 66 76
            // 07 17 27 37 47 57 67 77
        }
    }
    // Post-condition output and store it
    {
        // Post-condition (division by two)
        //    division of two 16 bits signed numbers using shifts
        //    n / 2 = (n - (n >> 15)) >> 1
        const __m128i sign_in0 = _mm_srai_epi16(in0, 15);
        const __m128i sign_in1 = _mm_srai_epi16(in1, 15);
        const __m128i sign_in2 = _mm_srai_epi16(in2, 15);
        const __m128i sign_in3 = _mm_srai_epi16(in3, 15);
        const __m128i sign_in4 = _mm_srai_epi16(in4, 15);
        const __m128i sign_in5 = _mm_srai_epi16(in5, 15);
        const __m128i sign_in6 = _mm_srai_epi16(in6, 15);
        const __m128i sign_in7 = _mm_srai_epi16(in7, 15);
        in0                    = _mm_sub_epi16(in0, sign_in0);
        in1                    = _mm_sub_epi16(in1, sign_in1);
        in2                    = _mm_sub_epi16(in2, sign_in2);
        in3                    = _mm_sub_epi16(in3, sign_in3);
        in4                    = _mm_sub_epi16(in4, sign_in4);
        in5                    = _mm_sub_epi16(in5, sign_in5);
        in6                    = _mm_sub_epi16(in6, sign_in6);
        in7                    = _mm_sub_epi16(in7, sign_in7);
        in0                    = _mm_srai_epi16(in0, 1);
        in1                    = _mm_srai_epi16(in1, 1);
        in2                    = _mm_srai_epi16(in2, 1);
        in3                    = _mm_srai_epi16(in3, 1);
        in4                    = _mm_srai_epi16(in4, 1);
        in5                    = _mm_srai_epi16(in5, 1);
        in6                    = _mm_srai_epi16(in6, 1);
        in7                    = _mm_srai_epi16(in7, 1);
    }

    iscan_ptr += n_coeffs;
    qcoeff_ptr += n_coeffs;
    dqcoeff_ptr += n_coeffs;
    n_coeffs = -n_coeffs;
    zero     = _mm_setzero_si128();

    if (!skip_block) {
        __m128i eob;
        __m128i round, quant, dequant;
        {
            __m128i coeff0, coeff1;

            // Setup global values
            {
                round   = _mm_load_si128((const __m128i *)round_ptr);
                quant   = _mm_load_si128((const __m128i *)quant_ptr);
                dequant = _mm_load_si128((const __m128i *)dequant_ptr);
            }

            {
                __m128i coeff0_sign, coeff1_sign;
                __m128i qcoeff0, qcoeff1;
                __m128i qtmp0, qtmp1;
                // Do DC and first 15 AC
                coeff0 = *in[0];
                coeff1 = *in[1];

                // Poor man's sign extract
                coeff0_sign = _mm_srai_epi16(coeff0, 15);
                coeff1_sign = _mm_srai_epi16(coeff1, 15);
                qcoeff0     = _mm_xor_si128(coeff0, coeff0_sign);
                qcoeff1     = _mm_xor_si128(coeff1, coeff1_sign);
                qcoeff0     = _mm_sub_epi16(qcoeff0, coeff0_sign);
                qcoeff1     = _mm_sub_epi16(qcoeff1, coeff1_sign);

                qcoeff0 = _mm_adds_epi16(qcoeff0, round);
                round   = _mm_unpackhi_epi64(round, round);
                qcoeff1 = _mm_adds_epi16(qcoeff1, round);
                qtmp0   = _mm_mulhi_epi16(qcoeff0, quant);
                quant   = _mm_unpackhi_epi64(quant, quant);
                qtmp1   = _mm_mulhi_epi16(qcoeff1, quant);

                // Reinsert signs
                qcoeff0 = _mm_xor_si128(qtmp0, coeff0_sign);
                qcoeff1 = _mm_xor_si128(qtmp1, coeff1_sign);
                qcoeff0 = _mm_sub_epi16(qcoeff0, coeff0_sign);
                qcoeff1 = _mm_sub_epi16(qcoeff1, coeff1_sign);

                _mm_store_si128((__m128i *)(qcoeff_ptr + n_coeffs), qcoeff0);
                _mm_store_si128((__m128i *)(qcoeff_ptr + n_coeffs) + 1, qcoeff1);

                coeff0  = _mm_mullo_epi16(qcoeff0, dequant);
                dequant = _mm_unpackhi_epi64(dequant, dequant);
                coeff1  = _mm_mullo_epi16(qcoeff1, dequant);

                _mm_store_si128((__m128i *)(dqcoeff_ptr + n_coeffs), coeff0);
                _mm_store_si128((__m128i *)(dqcoeff_ptr + n_coeffs) + 1, coeff1);
            }

            {
                // Scan for eob
                __m128i zero_coeff0, zero_coeff1;
                __m128i nzero_coeff0, nzero_coeff1;
                __m128i iscan0, iscan1;
                __m128i eob1;
                zero_coeff0  = _mm_cmpeq_epi16(coeff0, zero);
                zero_coeff1  = _mm_cmpeq_epi16(coeff1, zero);
                nzero_coeff0 = _mm_cmpeq_epi16(zero_coeff0, zero);
                nzero_coeff1 = _mm_cmpeq_epi16(zero_coeff1, zero);
                iscan0       = _mm_load_si128((const __m128i *)(iscan_ptr + n_coeffs));
                iscan1       = _mm_load_si128((const __m128i *)(iscan_ptr + n_coeffs) + 1);
                // Add one to convert from indices to counts
                iscan0 = _mm_sub_epi16(iscan0, nzero_coeff0);
                iscan1 = _mm_sub_epi16(iscan1, nzero_coeff1);
                eob    = _mm_and_si128(iscan0, nzero_coeff0);
                eob1   = _mm_and_si128(iscan1, nzero_coeff1);
                eob    = _mm_max_epi16(eob, eob1);
            }
            n_coeffs += 8 * 2;
        }

        // AC only loop
        index = 2;
        while (n_coeffs < 0) {
            __m128i coeff0, coeff1;
            {
                __m128i coeff0_sign, coeff1_sign;
                __m128i qcoeff0, qcoeff1;
                __m128i qtmp0, qtmp1;

                assert(index < (int)(sizeof(in) / sizeof(in[0])) - 1);
                coeff0 = *in[index];
                coeff1 = *in[index + 1];

                // Poor man's sign extract
                coeff0_sign = _mm_srai_epi16(coeff0, 15);
                coeff1_sign = _mm_srai_epi16(coeff1, 15);
                qcoeff0     = _mm_xor_si128(coeff0, coeff0_sign);
                qcoeff1     = _mm_xor_si128(coeff1, coeff1_sign);
                qcoeff0     = _mm_sub_epi16(qcoeff0, coeff0_sign);
                qcoeff1     = _mm_sub_epi16(qcoeff1, coeff1_sign);

                qcoeff0 = _mm_adds_epi16(qcoeff0, round);
                qcoeff1 = _mm_adds_epi16(qcoeff1, round);
                qtmp0   = _mm_mulhi_epi16(qcoeff0, quant);
                qtmp1   = _mm_mulhi_epi16(qcoeff1, quant);

                // Reinsert signs
                qcoeff0 = _mm_xor_si128(qtmp0, coeff0_sign);
                qcoeff1 = _mm_xor_si128(qtmp1, coeff1_sign);
                qcoeff0 = _mm_sub_epi16(qcoeff0, coeff0_sign);
                qcoeff1 = _mm_sub_epi16(qcoeff1, coeff1_sign);

                _mm_store_si128((__m128i *)(qcoeff_ptr + n_coeffs), qcoeff0);
                _mm_store_si128((__m128i *)(qcoeff_ptr + n_coeffs) + 1, qcoeff1);

                coeff0 = _mm_mullo_epi16(qcoeff0, dequant);
                coeff1 = _mm_mullo_epi16(qcoeff1, dequant);

                _mm_store_si128((__m128i *)(dqcoeff_ptr + n_coeffs), coeff0);
                _mm_store_si128((__m128i *)(dqcoeff_ptr + n_coeffs) + 1, coeff1);
            }

            {
                // Scan for eob
                __m128i zero_coeff0, zero_coeff1;
                __m128i nzero_coeff0, nzero_coeff1;
                __m128i iscan0, iscan1;
                __m128i eob0, eob1;
                zero_coeff0  = _mm_cmpeq_epi16(coeff0, zero);
                zero_coeff1  = _mm_cmpeq_epi16(coeff1, zero);
                nzero_coeff0 = _mm_cmpeq_epi16(zero_coeff0, zero);
                nzero_coeff1 = _mm_cmpeq_epi16(zero_coeff1, zero);
                iscan0       = _mm_load_si128((const __m128i *)(iscan_ptr + n_coeffs));
                iscan1       = _mm_load_si128((const __m128i *)(iscan_ptr + n_coeffs) + 1);
                // Add one to convert from indices to counts
                iscan0 = _mm_sub_epi16(iscan0, nzero_coeff0);
                iscan1 = _mm_sub_epi16(iscan1, nzero_coeff1);
                eob0   = _mm_and_si128(iscan0, nzero_coeff0);
                eob1   = _mm_and_si128(iscan1, nzero_coeff1);
                eob0   = _mm_max_epi16(eob0, eob1);
                eob    = _mm_max_epi16(eob, eob0);
            }
            n_coeffs += 8 * 2;
            index += 2;
        }

        // Accumulate EOB
        {
            __m128i eob_shuffled;
            eob_shuffled = _mm_shuffle_epi32(eob, 0xe);
            eob          = _mm_max_epi16(eob, eob_shuffled);
            eob_shuffled = _mm_shufflelo_epi16(eob, 0xe);
            eob          = _mm_max_epi16(eob, eob_shuffled);
            eob_shuffled = _mm_shufflelo_epi16(eob, 0x1);
            eob          = _mm_max_epi16(eob, eob_shuffled);
            *eob_ptr     = _mm_extract_epi16(eob, 1);
        }
    } else {
        do {
            _mm_store_si128((__m128i *)(dqcoeff_ptr + n_coeffs), zero);
            _mm_store_si128((__m128i *)(dqcoeff_ptr + n_coeffs) + 1, zero);
            _mm_store_si128((__m128i *)(qcoeff_ptr + n_coeffs), zero);
            _mm_store_si128((__m128i *)(qcoeff_ptr + n_coeffs) + 1, zero);
            n_coeffs += 8 * 2;
        } while (n_coeffs < 0);
        *eob_ptr = 0;
    }
}
