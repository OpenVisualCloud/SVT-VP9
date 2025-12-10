/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp9_rtcd.h"
#include "inv_txfm_sse2.h"

void eb_vp9_iht4x4_16_add_sse2(const tran_low_t *input, uint8_t *dest, int stride, int tx_type) {
    __m128i       in[2];
    const __m128i eight = _mm_set1_epi16(8);

    in[0] = load_input_data8(input);
    in[1] = load_input_data8(input + 8);

    switch (tx_type) {
    case DCT_DCT:
        eb_vp9_idct4_sse2(in);
        eb_vp9_idct4_sse2(in);
        break;
    case ADST_DCT:
        eb_vp9_idct4_sse2(in);
        eb_vp9_iadst4_sse2(in);
        break;
    case DCT_ADST:
        eb_vp9_iadst4_sse2(in);
        eb_vp9_idct4_sse2(in);
        break;
    default:
        assert(tx_type == ADST_ADST);
        eb_vp9_iadst4_sse2(in);
        eb_vp9_iadst4_sse2(in);
        break;
    }

    // Final round and shift
    in[0] = _mm_add_epi16(in[0], eight);
    in[1] = _mm_add_epi16(in[1], eight);

    in[0] = _mm_srai_epi16(in[0], 4);
    in[1] = _mm_srai_epi16(in[1], 4);

    recon_and_store4x4_sse2(in, dest, stride);
}

void eb_vp9_iht8x8_64_add_sse2(const tran_low_t *input, uint8_t *dest, int stride, int tx_type) {
    __m128i       in[8];
    const __m128i final_rounding = _mm_set1_epi16(1 << 4);

    // load input data
    in[0] = load_input_data8(input);
    in[1] = load_input_data8(input + 8 * 1);
    in[2] = load_input_data8(input + 8 * 2);
    in[3] = load_input_data8(input + 8 * 3);
    in[4] = load_input_data8(input + 8 * 4);
    in[5] = load_input_data8(input + 8 * 5);
    in[6] = load_input_data8(input + 8 * 6);
    in[7] = load_input_data8(input + 8 * 7);

    switch (tx_type) {
    case DCT_DCT:
        eb_vp9_idct8_sse2(in);
        eb_vp9_idct8_sse2(in);
        break;
    case ADST_DCT:
        eb_vp9_idct8_sse2(in);
        eb_vp9_iadst8_sse2(in);
        break;
    case DCT_ADST:
        eb_vp9_iadst8_sse2(in);
        eb_vp9_idct8_sse2(in);
        break;
    default:
        assert(tx_type == ADST_ADST);
        eb_vp9_iadst8_sse2(in);
        eb_vp9_iadst8_sse2(in);
        break;
    }

    // Final rounding and shift
    in[0] = _mm_adds_epi16(in[0], final_rounding);
    in[1] = _mm_adds_epi16(in[1], final_rounding);
    in[2] = _mm_adds_epi16(in[2], final_rounding);
    in[3] = _mm_adds_epi16(in[3], final_rounding);
    in[4] = _mm_adds_epi16(in[4], final_rounding);
    in[5] = _mm_adds_epi16(in[5], final_rounding);
    in[6] = _mm_adds_epi16(in[6], final_rounding);
    in[7] = _mm_adds_epi16(in[7], final_rounding);

    in[0] = _mm_srai_epi16(in[0], 5);
    in[1] = _mm_srai_epi16(in[1], 5);
    in[2] = _mm_srai_epi16(in[2], 5);
    in[3] = _mm_srai_epi16(in[3], 5);
    in[4] = _mm_srai_epi16(in[4], 5);
    in[5] = _mm_srai_epi16(in[5], 5);
    in[6] = _mm_srai_epi16(in[6], 5);
    in[7] = _mm_srai_epi16(in[7], 5);

    recon_and_store(in[0], dest + 0 * stride);
    recon_and_store(in[1], dest + 1 * stride);
    recon_and_store(in[2], dest + 2 * stride);
    recon_and_store(in[3], dest + 3 * stride);
    recon_and_store(in[4], dest + 4 * stride);
    recon_and_store(in[5], dest + 5 * stride);
    recon_and_store(in[6], dest + 6 * stride);
    recon_and_store(in[7], dest + 7 * stride);
}
