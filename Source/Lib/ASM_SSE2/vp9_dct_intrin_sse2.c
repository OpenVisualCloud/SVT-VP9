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
