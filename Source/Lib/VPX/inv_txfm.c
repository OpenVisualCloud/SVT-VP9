/*
 *  Copyright (c) 2015 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <string.h>
#include "vpx_dsp_rtcd.h"
#include "inv_txfm.h"

void eb_vp9_iadst4_c(const tran_low_t *input, tran_low_t *output) {
    tran_high_t s0, s1, s2, s3, s4, s5, s6, s7;
    tran_low_t  x0 = input[0];
    tran_low_t  x1 = input[1];
    tran_low_t  x2 = input[2];
    tran_low_t  x3 = input[3];

    if (!(x0 | x1 | x2 | x3)) {
        memset(output, 0, 4 * sizeof(*output));
        return;
    }

    // 32-bit result is enough for the following multiplications.
    s0 = sinpi_1_9 * x0;
    s1 = sinpi_2_9 * x0;
    s2 = sinpi_3_9 * x1;
    s3 = sinpi_4_9 * x2;
    s4 = sinpi_1_9 * x2;
    s5 = sinpi_2_9 * x3;
    s6 = sinpi_4_9 * x3;
    s7 = WRAPLOW(x0 - x2 + x3);

    s0 = s0 + s3 + s5;
    s1 = s1 - s4 - s6;
    s3 = s2;
    s2 = sinpi_3_9 * s7;

    // 1-D transform scaling factor is sqrt(2).
    // The overall dynamic range is 14b (input) + 14b (multiplication scaling)
    // + 1b (addition) = 29b.
    // Hence the output bit depth is 15b.
    output[0] = (int16_t)WRAPLOW(dct_const_round_shift(s0 + s3));
    output[1] = (int16_t)WRAPLOW(dct_const_round_shift(s1 + s3));
    output[2] = (int16_t)WRAPLOW(dct_const_round_shift(s2));
    output[3] = (int16_t)WRAPLOW(dct_const_round_shift(s0 + s1 - s3));
}

void eb_vp9_idct4_c(const tran_low_t *input, tran_low_t *output) {
    int16_t     step[4];
    tran_high_t temp1, temp2;

    // stage 1
    temp1   = ((int16_t)input[0] + (int16_t)input[2]) * cospi_16_64;
    temp2   = ((int16_t)input[0] - (int16_t)input[2]) * cospi_16_64;
    step[0] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step[1] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1   = (int16_t)input[1] * cospi_24_64 - (int16_t)input[3] * cospi_8_64;
    temp2   = (int16_t)input[1] * cospi_8_64 + (int16_t)input[3] * cospi_24_64;
    step[2] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step[3] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    // stage 2
    output[0] = (int16_t)WRAPLOW(step[0] + step[3]);
    output[1] = (int16_t)WRAPLOW(step[1] + step[2]);
    output[2] = (int16_t)WRAPLOW(step[1] - step[2]);
    output[3] = (int16_t)WRAPLOW(step[0] - step[3]);
}

void eb_vp9_idct4x4_16_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i, j;
    tran_low_t  out[4 * 4];
    tran_low_t *out_ptr = out;
    tran_low_t  temp_in[4], temp_out[4];

    // Rows
    for (i = 0; i < 4; ++i) {
        eb_vp9_idct4_c(input, out_ptr);
        input += 4;
        out_ptr += 4;
    }

    // Columns
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) temp_in[j] = out[j * 4 + i];
        eb_vp9_idct4_c(temp_in, temp_out);
        for (j = 0; j < 4; ++j) {
            dest[j * stride + i] = clip_pixel_add(dest[j * stride + i], ROUND_POWER_OF_TWO(temp_out[j], 4));
        }
    }
}

void eb_vp9_idct4x4_1_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i;
    tran_high_t a1;
    tran_low_t  out = (int16_t)WRAPLOW(dct_const_round_shift((int16_t)input[0] * cospi_16_64));

    out = (int16_t)WRAPLOW(dct_const_round_shift(out * cospi_16_64));
    a1  = ROUND_POWER_OF_TWO(out, 4);

    for (i = 0; i < 4; i++) {
        dest[0] = clip_pixel_add(dest[0], a1);
        dest[1] = clip_pixel_add(dest[1], a1);
        dest[2] = clip_pixel_add(dest[2], a1);
        dest[3] = clip_pixel_add(dest[3], a1);
        dest += stride;
    }
}

void eb_vp9_iadst8_c(const tran_low_t *input, tran_low_t *output) {
    int         s0, s1, s2, s3, s4, s5, s6, s7;
    tran_high_t x0 = input[7];
    tran_high_t x1 = input[0];
    tran_high_t x2 = input[5];
    tran_high_t x3 = input[2];
    tran_high_t x4 = input[3];
    tran_high_t x5 = input[4];
    tran_high_t x6 = input[1];
    tran_high_t x7 = input[6];

    if (!(x0 | x1 | x2 | x3 | x4 | x5 | x6 | x7)) {
        memset(output, 0, 8 * sizeof(*output));
        return;
    }

    // stage 1
    s0 = (int)(cospi_2_64 * x0 + cospi_30_64 * x1);
    s1 = (int)(cospi_30_64 * x0 - cospi_2_64 * x1);
    s2 = (int)(cospi_10_64 * x2 + cospi_22_64 * x3);
    s3 = (int)(cospi_22_64 * x2 - cospi_10_64 * x3);
    s4 = (int)(cospi_18_64 * x4 + cospi_14_64 * x5);
    s5 = (int)(cospi_14_64 * x4 - cospi_18_64 * x5);
    s6 = (int)(cospi_26_64 * x6 + cospi_6_64 * x7);
    s7 = (int)(cospi_6_64 * x6 - cospi_26_64 * x7);

    x0 = WRAPLOW(dct_const_round_shift(s0 + s4));
    x1 = WRAPLOW(dct_const_round_shift(s1 + s5));
    x2 = WRAPLOW(dct_const_round_shift(s2 + s6));
    x3 = WRAPLOW(dct_const_round_shift(s3 + s7));
    x4 = WRAPLOW(dct_const_round_shift(s0 - s4));
    x5 = WRAPLOW(dct_const_round_shift(s1 - s5));
    x6 = WRAPLOW(dct_const_round_shift(s2 - s6));
    x7 = WRAPLOW(dct_const_round_shift(s3 - s7));

    // stage 2
    s0 = (int)x0;
    s1 = (int)x1;
    s2 = (int)x2;
    s3 = (int)x3;
    s4 = (int)(cospi_8_64 * x4 + cospi_24_64 * x5);
    s5 = (int)(cospi_24_64 * x4 - cospi_8_64 * x5);
    s6 = (int)(-cospi_24_64 * x6 + cospi_8_64 * x7);
    s7 = (int)(cospi_8_64 * x6 + cospi_24_64 * x7);

    x0 = WRAPLOW(s0 + s2);
    x1 = WRAPLOW(s1 + s3);
    x2 = WRAPLOW(s0 - s2);
    x3 = WRAPLOW(s1 - s3);
    x4 = WRAPLOW(dct_const_round_shift(s4 + s6));
    x5 = WRAPLOW(dct_const_round_shift(s5 + s7));
    x6 = WRAPLOW(dct_const_round_shift(s4 - s6));
    x7 = WRAPLOW(dct_const_round_shift(s5 - s7));

    // stage 3
    s2 = (int)(cospi_16_64 * (x2 + x3));
    s3 = (int)(cospi_16_64 * (x2 - x3));
    s6 = (int)(cospi_16_64 * (x6 + x7));
    s7 = (int)(cospi_16_64 * (x6 - x7));

    x2 = WRAPLOW(dct_const_round_shift(s2));
    x3 = WRAPLOW(dct_const_round_shift(s3));
    x6 = WRAPLOW(dct_const_round_shift(s6));
    x7 = WRAPLOW(dct_const_round_shift(s7));

    output[0] = (int16_t)WRAPLOW(x0);
    output[1] = (int16_t)WRAPLOW(-x4);
    output[2] = (int16_t)WRAPLOW(x6);
    output[3] = (int16_t)WRAPLOW(-x2);
    output[4] = (int16_t)WRAPLOW(x3);
    output[5] = (int16_t)WRAPLOW(-x7);
    output[6] = (int16_t)WRAPLOW(x5);
    output[7] = (int16_t)WRAPLOW(-x1);
}

void eb_vp9_idct8_c(const tran_low_t *input, tran_low_t *output) {
    int16_t     step1[8], step2[8];
    tran_high_t temp1, temp2;

    // stage 1
    step1[0] = (int16_t)input[0];
    step1[2] = (int16_t)input[4];
    step1[1] = (int16_t)input[2];
    step1[3] = (int16_t)input[6];
    temp1    = (int16_t)input[1] * cospi_28_64 - (int16_t)input[7] * cospi_4_64;
    temp2    = (int16_t)input[1] * cospi_4_64 + (int16_t)input[7] * cospi_28_64;
    step1[4] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[7] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1    = (int16_t)input[5] * cospi_12_64 - (int16_t)input[3] * cospi_20_64;
    temp2    = (int16_t)input[5] * cospi_20_64 + (int16_t)input[3] * cospi_12_64;
    step1[5] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[6] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    // stage 2
    temp1    = (step1[0] + step1[2]) * cospi_16_64;
    temp2    = (step1[0] - step1[2]) * cospi_16_64;
    step2[0] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[1] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1    = step1[1] * cospi_24_64 - step1[3] * cospi_8_64;
    temp2    = step1[1] * cospi_8_64 + step1[3] * cospi_24_64;
    step2[2] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[3] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step2[4] = (int16_t)WRAPLOW(step1[4] + step1[5]);
    step2[5] = (int16_t)WRAPLOW(step1[4] - step1[5]);
    step2[6] = (int16_t)WRAPLOW(-step1[6] + step1[7]);
    step2[7] = (int16_t)WRAPLOW(step1[6] + step1[7]);

    // stage 3
    step1[0] = (int16_t)WRAPLOW(step2[0] + step2[3]);
    step1[1] = (int16_t)WRAPLOW(step2[1] + step2[2]);
    step1[2] = (int16_t)WRAPLOW(step2[1] - step2[2]);
    step1[3] = (int16_t)WRAPLOW(step2[0] - step2[3]);
    step1[4] = step2[4];
    temp1    = (step2[6] - step2[5]) * cospi_16_64;
    temp2    = (step2[5] + step2[6]) * cospi_16_64;
    step1[5] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[6] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step1[7] = step2[7];

    // stage 4
    output[0] = (int16_t)WRAPLOW(step1[0] + step1[7]);
    output[1] = (int16_t)WRAPLOW(step1[1] + step1[6]);
    output[2] = (int16_t)WRAPLOW(step1[2] + step1[5]);
    output[3] = (int16_t)WRAPLOW(step1[3] + step1[4]);
    output[4] = (int16_t)WRAPLOW(step1[3] - step1[4]);
    output[5] = (int16_t)WRAPLOW(step1[2] - step1[5]);
    output[6] = (int16_t)WRAPLOW(step1[1] - step1[6]);
    output[7] = (int16_t)WRAPLOW(step1[0] - step1[7]);
}

void eb_vp9_idct8x8_64_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i, j;
    tran_low_t  out[8 * 8];
    tran_low_t *out_ptr = out;
    tran_low_t  temp_in[8], temp_out[8];

    // First transform rows
    for (i = 0; i < 8; ++i) {
        eb_vp9_idct8_c(input, out_ptr);
        input += 8;
        out_ptr += 8;
    }

    // Then transform columns
    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) temp_in[j] = out[j * 8 + i];
        eb_vp9_idct8_c(temp_in, temp_out);
        for (j = 0; j < 8; ++j) {
            dest[j * stride + i] = clip_pixel_add(dest[j * stride + i], ROUND_POWER_OF_TWO(temp_out[j], 5));
        }
    }
}

void eb_vp9_idct8x8_12_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i, j;
    tran_low_t  out[8 * 8] = {0};
    tran_low_t *out_ptr    = out;
    tran_low_t  temp_in[8], temp_out[8];

    // First transform rows
    // Only first 4 row has non-zero coefs
    for (i = 0; i < 4; ++i) {
        eb_vp9_idct8_c(input, out_ptr);
        input += 8;
        out_ptr += 8;
    }

    // Then transform columns
    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) temp_in[j] = out[j * 8 + i];
        eb_vp9_idct8_c(temp_in, temp_out);
        for (j = 0; j < 8; ++j) {
            dest[j * stride + i] = clip_pixel_add(dest[j * stride + i], ROUND_POWER_OF_TWO(temp_out[j], 5));
        }
    }
}

void eb_vp9_idct8x8_1_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i, j;
    tran_high_t a1;
    tran_low_t  out = (int16_t)WRAPLOW(dct_const_round_shift((int16_t)input[0] * cospi_16_64));

    out = (int16_t)WRAPLOW(dct_const_round_shift(out * cospi_16_64));
    a1  = ROUND_POWER_OF_TWO(out, 5);
    for (j = 0; j < 8; ++j) {
        for (i = 0; i < 8; ++i) dest[i] = clip_pixel_add(dest[i], a1);
        dest += stride;
    }
}

void eb_vp9_iadst16_c(const tran_low_t *input, tran_low_t *output) {
    tran_high_t s0, s1, s2, s3, s4, s5, s6, s7, s8;
    tran_high_t s9, s10, s11, s12, s13, s14, s15;
    tran_high_t x0  = input[15];
    tran_high_t x1  = input[0];
    tran_high_t x2  = input[13];
    tran_high_t x3  = input[2];
    tran_high_t x4  = input[11];
    tran_high_t x5  = input[4];
    tran_high_t x6  = input[9];
    tran_high_t x7  = input[6];
    tran_high_t x8  = input[7];
    tran_high_t x9  = input[8];
    tran_high_t x10 = input[5];
    tran_high_t x11 = input[10];
    tran_high_t x12 = input[3];
    tran_high_t x13 = input[12];
    tran_high_t x14 = input[1];
    tran_high_t x15 = input[14];

    if (!(x0 | x1 | x2 | x3 | x4 | x5 | x6 | x7 | x8 | x9 | x10 | x11 | x12 | x13 | x14 | x15)) {
        memset(output, 0, 16 * sizeof(*output));
        return;
    }

    // stage 1
    s0  = x0 * cospi_1_64 + x1 * cospi_31_64;
    s1  = x0 * cospi_31_64 - x1 * cospi_1_64;
    s2  = x2 * cospi_5_64 + x3 * cospi_27_64;
    s3  = x2 * cospi_27_64 - x3 * cospi_5_64;
    s4  = x4 * cospi_9_64 + x5 * cospi_23_64;
    s5  = x4 * cospi_23_64 - x5 * cospi_9_64;
    s6  = x6 * cospi_13_64 + x7 * cospi_19_64;
    s7  = x6 * cospi_19_64 - x7 * cospi_13_64;
    s8  = x8 * cospi_17_64 + x9 * cospi_15_64;
    s9  = x8 * cospi_15_64 - x9 * cospi_17_64;
    s10 = x10 * cospi_21_64 + x11 * cospi_11_64;
    s11 = x10 * cospi_11_64 - x11 * cospi_21_64;
    s12 = x12 * cospi_25_64 + x13 * cospi_7_64;
    s13 = x12 * cospi_7_64 - x13 * cospi_25_64;
    s14 = x14 * cospi_29_64 + x15 * cospi_3_64;
    s15 = x14 * cospi_3_64 - x15 * cospi_29_64;

    x0  = WRAPLOW(dct_const_round_shift(s0 + s8));
    x1  = WRAPLOW(dct_const_round_shift(s1 + s9));
    x2  = WRAPLOW(dct_const_round_shift(s2 + s10));
    x3  = WRAPLOW(dct_const_round_shift(s3 + s11));
    x4  = WRAPLOW(dct_const_round_shift(s4 + s12));
    x5  = WRAPLOW(dct_const_round_shift(s5 + s13));
    x6  = WRAPLOW(dct_const_round_shift(s6 + s14));
    x7  = WRAPLOW(dct_const_round_shift(s7 + s15));
    x8  = WRAPLOW(dct_const_round_shift(s0 - s8));
    x9  = WRAPLOW(dct_const_round_shift(s1 - s9));
    x10 = WRAPLOW(dct_const_round_shift(s2 - s10));
    x11 = WRAPLOW(dct_const_round_shift(s3 - s11));
    x12 = WRAPLOW(dct_const_round_shift(s4 - s12));
    x13 = WRAPLOW(dct_const_round_shift(s5 - s13));
    x14 = WRAPLOW(dct_const_round_shift(s6 - s14));
    x15 = WRAPLOW(dct_const_round_shift(s7 - s15));

    // stage 2
    s0  = x0;
    s1  = x1;
    s2  = x2;
    s3  = x3;
    s4  = x4;
    s5  = x5;
    s6  = x6;
    s7  = x7;
    s8  = x8 * cospi_4_64 + x9 * cospi_28_64;
    s9  = x8 * cospi_28_64 - x9 * cospi_4_64;
    s10 = x10 * cospi_20_64 + x11 * cospi_12_64;
    s11 = x10 * cospi_12_64 - x11 * cospi_20_64;
    s12 = -x12 * cospi_28_64 + x13 * cospi_4_64;
    s13 = x12 * cospi_4_64 + x13 * cospi_28_64;
    s14 = -x14 * cospi_12_64 + x15 * cospi_20_64;
    s15 = x14 * cospi_20_64 + x15 * cospi_12_64;

    x0  = WRAPLOW(s0 + s4);
    x1  = WRAPLOW(s1 + s5);
    x2  = WRAPLOW(s2 + s6);
    x3  = WRAPLOW(s3 + s7);
    x4  = WRAPLOW(s0 - s4);
    x5  = WRAPLOW(s1 - s5);
    x6  = WRAPLOW(s2 - s6);
    x7  = WRAPLOW(s3 - s7);
    x8  = WRAPLOW(dct_const_round_shift(s8 + s12));
    x9  = WRAPLOW(dct_const_round_shift(s9 + s13));
    x10 = WRAPLOW(dct_const_round_shift(s10 + s14));
    x11 = WRAPLOW(dct_const_round_shift(s11 + s15));
    x12 = WRAPLOW(dct_const_round_shift(s8 - s12));
    x13 = WRAPLOW(dct_const_round_shift(s9 - s13));
    x14 = WRAPLOW(dct_const_round_shift(s10 - s14));
    x15 = WRAPLOW(dct_const_round_shift(s11 - s15));

    // stage 3
    s0  = x0;
    s1  = x1;
    s2  = x2;
    s3  = x3;
    s4  = x4 * cospi_8_64 + x5 * cospi_24_64;
    s5  = x4 * cospi_24_64 - x5 * cospi_8_64;
    s6  = -x6 * cospi_24_64 + x7 * cospi_8_64;
    s7  = x6 * cospi_8_64 + x7 * cospi_24_64;
    s8  = x8;
    s9  = x9;
    s10 = x10;
    s11 = x11;
    s12 = x12 * cospi_8_64 + x13 * cospi_24_64;
    s13 = x12 * cospi_24_64 - x13 * cospi_8_64;
    s14 = -x14 * cospi_24_64 + x15 * cospi_8_64;
    s15 = x14 * cospi_8_64 + x15 * cospi_24_64;

    x0  = WRAPLOW(s0 + s2);
    x1  = WRAPLOW(s1 + s3);
    x2  = WRAPLOW(s0 - s2);
    x3  = WRAPLOW(s1 - s3);
    x4  = WRAPLOW(dct_const_round_shift(s4 + s6));
    x5  = WRAPLOW(dct_const_round_shift(s5 + s7));
    x6  = WRAPLOW(dct_const_round_shift(s4 - s6));
    x7  = WRAPLOW(dct_const_round_shift(s5 - s7));
    x8  = WRAPLOW(s8 + s10);
    x9  = WRAPLOW(s9 + s11);
    x10 = WRAPLOW(s8 - s10);
    x11 = WRAPLOW(s9 - s11);
    x12 = WRAPLOW(dct_const_round_shift(s12 + s14));
    x13 = WRAPLOW(dct_const_round_shift(s13 + s15));
    x14 = WRAPLOW(dct_const_round_shift(s12 - s14));
    x15 = WRAPLOW(dct_const_round_shift(s13 - s15));

    // stage 4
    s2  = (-cospi_16_64) * (x2 + x3);
    s3  = cospi_16_64 * (x2 - x3);
    s6  = cospi_16_64 * (x6 + x7);
    s7  = cospi_16_64 * (-x6 + x7);
    s10 = cospi_16_64 * (x10 + x11);
    s11 = cospi_16_64 * (-x10 + x11);
    s14 = (-cospi_16_64) * (x14 + x15);
    s15 = cospi_16_64 * (x14 - x15);

    x2  = WRAPLOW(dct_const_round_shift(s2));
    x3  = WRAPLOW(dct_const_round_shift(s3));
    x6  = WRAPLOW(dct_const_round_shift(s6));
    x7  = WRAPLOW(dct_const_round_shift(s7));
    x10 = WRAPLOW(dct_const_round_shift(s10));
    x11 = WRAPLOW(dct_const_round_shift(s11));
    x14 = WRAPLOW(dct_const_round_shift(s14));
    x15 = WRAPLOW(dct_const_round_shift(s15));

    output[0]  = (int16_t)WRAPLOW(x0);
    output[1]  = (int16_t)WRAPLOW(-x8);
    output[2]  = (int16_t)WRAPLOW(x12);
    output[3]  = (int16_t)WRAPLOW(-x4);
    output[4]  = (int16_t)WRAPLOW(x6);
    output[5]  = (int16_t)WRAPLOW(x14);
    output[6]  = (int16_t)WRAPLOW(x10);
    output[7]  = (int16_t)WRAPLOW(x2);
    output[8]  = (int16_t)WRAPLOW(x3);
    output[9]  = (int16_t)WRAPLOW(x11);
    output[10] = (int16_t)WRAPLOW(x15);
    output[11] = (int16_t)WRAPLOW(x7);
    output[12] = (int16_t)WRAPLOW(x5);
    output[13] = (int16_t)WRAPLOW(-x13);
    output[14] = (int16_t)WRAPLOW(x9);
    output[15] = (int16_t)WRAPLOW(-x1);
}

void eb_vp9_idct16_c(const tran_low_t *input, tran_low_t *output) {
    int16_t     step1[16], step2[16];
    tran_high_t temp1, temp2;

    // stage 1
    step1[0]  = (int16_t)input[0 / 2];
    step1[1]  = (int16_t)input[16 / 2];
    step1[2]  = (int16_t)input[8 / 2];
    step1[3]  = (int16_t)input[24 / 2];
    step1[4]  = (int16_t)input[4 / 2];
    step1[5]  = (int16_t)input[20 / 2];
    step1[6]  = (int16_t)input[12 / 2];
    step1[7]  = (int16_t)input[28 / 2];
    step1[8]  = (int16_t)input[2 / 2];
    step1[9]  = (int16_t)input[18 / 2];
    step1[10] = (int16_t)input[10 / 2];
    step1[11] = (int16_t)input[26 / 2];
    step1[12] = (int16_t)input[6 / 2];
    step1[13] = (int16_t)input[22 / 2];
    step1[14] = (int16_t)input[14 / 2];
    step1[15] = (int16_t)input[30 / 2];

    // stage 2
    step2[0] = step1[0];
    step2[1] = step1[1];
    step2[2] = step1[2];
    step2[3] = step1[3];
    step2[4] = step1[4];
    step2[5] = step1[5];
    step2[6] = step1[6];
    step2[7] = step1[7];

    temp1     = step1[8] * cospi_30_64 - step1[15] * cospi_2_64;
    temp2     = step1[8] * cospi_2_64 + step1[15] * cospi_30_64;
    step2[8]  = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[15] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = step1[9] * cospi_14_64 - step1[14] * cospi_18_64;
    temp2     = step1[9] * cospi_18_64 + step1[14] * cospi_14_64;
    step2[9]  = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[14] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = step1[10] * cospi_22_64 - step1[13] * cospi_10_64;
    temp2     = step1[10] * cospi_10_64 + step1[13] * cospi_22_64;
    step2[10] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[13] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = step1[11] * cospi_6_64 - step1[12] * cospi_26_64;
    temp2     = step1[11] * cospi_26_64 + step1[12] * cospi_6_64;
    step2[11] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[12] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    // stage 3
    step1[0] = step2[0];
    step1[1] = step2[1];
    step1[2] = step2[2];
    step1[3] = step2[3];

    temp1    = step2[4] * cospi_28_64 - step2[7] * cospi_4_64;
    temp2    = step2[4] * cospi_4_64 + step2[7] * cospi_28_64;
    step1[4] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[7] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1    = step2[5] * cospi_12_64 - step2[6] * cospi_20_64;
    temp2    = step2[5] * cospi_20_64 + step2[6] * cospi_12_64;
    step1[5] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[6] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    step1[8]  = (int16_t)WRAPLOW(step2[8] + step2[9]);
    step1[9]  = (int16_t)WRAPLOW(step2[8] - step2[9]);
    step1[10] = (int16_t)WRAPLOW(-step2[10] + step2[11]);
    step1[11] = (int16_t)WRAPLOW(step2[10] + step2[11]);
    step1[12] = (int16_t)WRAPLOW(step2[12] + step2[13]);
    step1[13] = (int16_t)WRAPLOW(step2[12] - step2[13]);
    step1[14] = (int16_t)WRAPLOW(-step2[14] + step2[15]);
    step1[15] = (int16_t)WRAPLOW(step2[14] + step2[15]);

    // stage 4
    temp1    = (step1[0] + step1[1]) * cospi_16_64;
    temp2    = (step1[0] - step1[1]) * cospi_16_64;
    step2[0] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[1] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1    = step1[2] * cospi_24_64 - step1[3] * cospi_8_64;
    temp2    = step1[2] * cospi_8_64 + step1[3] * cospi_24_64;
    step2[2] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[3] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step2[4] = (int16_t)WRAPLOW(step1[4] + step1[5]);
    step2[5] = (int16_t)WRAPLOW(step1[4] - step1[5]);
    step2[6] = (int16_t)WRAPLOW(-step1[6] + step1[7]);
    step2[7] = (int16_t)WRAPLOW(step1[6] + step1[7]);

    step2[8]  = step1[8];
    step2[15] = step1[15];
    temp1     = -step1[9] * cospi_8_64 + step1[14] * cospi_24_64;
    temp2     = step1[9] * cospi_24_64 + step1[14] * cospi_8_64;
    step2[9]  = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[14] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1     = -step1[10] * cospi_24_64 - step1[13] * cospi_8_64;
    temp2     = -step1[10] * cospi_8_64 + step1[13] * cospi_24_64;
    step2[10] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[13] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step2[11] = step1[11];
    step2[12] = step1[12];

    // stage 5
    step1[0] = (int16_t)WRAPLOW(step2[0] + step2[3]);
    step1[1] = (int16_t)WRAPLOW(step2[1] + step2[2]);
    step1[2] = (int16_t)WRAPLOW(step2[1] - step2[2]);
    step1[3] = (int16_t)WRAPLOW(step2[0] - step2[3]);
    step1[4] = step2[4];
    temp1    = (step2[6] - step2[5]) * cospi_16_64;
    temp2    = (step2[5] + step2[6]) * cospi_16_64;
    step1[5] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[6] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step1[7] = step2[7];

    step1[8]  = (int16_t)WRAPLOW(step2[8] + step2[11]);
    step1[9]  = (int16_t)WRAPLOW(step2[9] + step2[10]);
    step1[10] = (int16_t)WRAPLOW(step2[9] - step2[10]);
    step1[11] = (int16_t)WRAPLOW(step2[8] - step2[11]);
    step1[12] = (int16_t)WRAPLOW(-step2[12] + step2[15]);
    step1[13] = (int16_t)WRAPLOW(-step2[13] + step2[14]);
    step1[14] = (int16_t)WRAPLOW(step2[13] + step2[14]);
    step1[15] = (int16_t)WRAPLOW(step2[12] + step2[15]);

    // stage 6
    step2[0]  = (int16_t)WRAPLOW(step1[0] + step1[7]);
    step2[1]  = (int16_t)WRAPLOW(step1[1] + step1[6]);
    step2[2]  = (int16_t)WRAPLOW(step1[2] + step1[5]);
    step2[3]  = (int16_t)WRAPLOW(step1[3] + step1[4]);
    step2[4]  = (int16_t)WRAPLOW(step1[3] - step1[4]);
    step2[5]  = (int16_t)WRAPLOW(step1[2] - step1[5]);
    step2[6]  = (int16_t)WRAPLOW(step1[1] - step1[6]);
    step2[7]  = (int16_t)WRAPLOW(step1[0] - step1[7]);
    step2[8]  = step1[8];
    step2[9]  = step1[9];
    temp1     = (-step1[10] + step1[13]) * cospi_16_64;
    temp2     = (step1[10] + step1[13]) * cospi_16_64;
    step2[10] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[13] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1     = (-step1[11] + step1[12]) * cospi_16_64;
    temp2     = (step1[11] + step1[12]) * cospi_16_64;
    step2[11] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[12] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step2[14] = step1[14];
    step2[15] = step1[15];

    // stage 7
    output[0]  = (int16_t)WRAPLOW(step2[0] + step2[15]);
    output[1]  = (int16_t)WRAPLOW(step2[1] + step2[14]);
    output[2]  = (int16_t)WRAPLOW(step2[2] + step2[13]);
    output[3]  = (int16_t)WRAPLOW(step2[3] + step2[12]);
    output[4]  = (int16_t)WRAPLOW(step2[4] + step2[11]);
    output[5]  = (int16_t)WRAPLOW(step2[5] + step2[10]);
    output[6]  = (int16_t)WRAPLOW(step2[6] + step2[9]);
    output[7]  = (int16_t)WRAPLOW(step2[7] + step2[8]);
    output[8]  = (int16_t)WRAPLOW(step2[7] - step2[8]);
    output[9]  = (int16_t)WRAPLOW(step2[6] - step2[9]);
    output[10] = (int16_t)WRAPLOW(step2[5] - step2[10]);
    output[11] = (int16_t)WRAPLOW(step2[4] - step2[11]);
    output[12] = (int16_t)WRAPLOW(step2[3] - step2[12]);
    output[13] = (int16_t)WRAPLOW(step2[2] - step2[13]);
    output[14] = (int16_t)WRAPLOW(step2[1] - step2[14]);
    output[15] = (int16_t)WRAPLOW(step2[0] - step2[15]);
}

void eb_vp9_idct16x16_256_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i, j;
    tran_low_t  out[16 * 16];
    tran_low_t *out_ptr = out;
    tran_low_t  temp_in[16], temp_out[16];

    // First transform rows
    for (i = 0; i < 16; ++i) {
        eb_vp9_idct16_c(input, out_ptr);
        input += 16;
        out_ptr += 16;
    }

    // Then transform columns
    for (i = 0; i < 16; ++i) {
        for (j = 0; j < 16; ++j) temp_in[j] = out[j * 16 + i];
        eb_vp9_idct16_c(temp_in, temp_out);
        for (j = 0; j < 16; ++j) {
            dest[j * stride + i] = clip_pixel_add(dest[j * stride + i], ROUND_POWER_OF_TWO(temp_out[j], 6));
        }
    }
}

void eb_vp9_idct16x16_38_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i, j;
    tran_low_t  out[16 * 16] = {0};
    tran_low_t *out_ptr      = out;
    tran_low_t  temp_in[16], temp_out[16];

    // First transform rows. Since all non-zero dct coefficients are in
    // upper-left 8x8 area, we only need to calculate first 8 rows here.
    for (i = 0; i < 8; ++i) {
        eb_vp9_idct16_c(input, out_ptr);
        input += 16;
        out_ptr += 16;
    }

    // Then transform columns
    for (i = 0; i < 16; ++i) {
        for (j = 0; j < 16; ++j) temp_in[j] = out[j * 16 + i];
        eb_vp9_idct16_c(temp_in, temp_out);
        for (j = 0; j < 16; ++j) {
            dest[j * stride + i] = clip_pixel_add(dest[j * stride + i], ROUND_POWER_OF_TWO(temp_out[j], 6));
        }
    }
}

void eb_vp9_idct16x16_10_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i, j;
    tran_low_t  out[16 * 16] = {0};
    tran_low_t *out_ptr      = out;
    tran_low_t  temp_in[16], temp_out[16];

    // First transform rows. Since all non-zero dct coefficients are in
    // upper-left 4x4 area, we only need to calculate first 4 rows here.
    for (i = 0; i < 4; ++i) {
        eb_vp9_idct16_c(input, out_ptr);
        input += 16;
        out_ptr += 16;
    }

    // Then transform columns
    for (i = 0; i < 16; ++i) {
        for (j = 0; j < 16; ++j) temp_in[j] = out[j * 16 + i];
        eb_vp9_idct16_c(temp_in, temp_out);
        for (j = 0; j < 16; ++j) {
            dest[j * stride + i] = clip_pixel_add(dest[j * stride + i], ROUND_POWER_OF_TWO(temp_out[j], 6));
        }
    }
}

void eb_vp9_idct16x16_1_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i, j;
    tran_high_t a1;
    tran_low_t  out = (int16_t)WRAPLOW(dct_const_round_shift((int16_t)input[0] * cospi_16_64));

    out = (int16_t)WRAPLOW(dct_const_round_shift(out * cospi_16_64));
    a1  = ROUND_POWER_OF_TWO(out, 6);
    for (j = 0; j < 16; ++j) {
        for (i = 0; i < 16; ++i) dest[i] = clip_pixel_add(dest[i], a1);
        dest += stride;
    }
}

static void idct32_c(const tran_low_t *input, tran_low_t *output) {
    int16_t     step1[32], step2[32];
    tran_high_t temp1, temp2;

    // stage 1
    step1[0]  = (int16_t)input[0];
    step1[1]  = (int16_t)input[16];
    step1[2]  = (int16_t)input[8];
    step1[3]  = (int16_t)input[24];
    step1[4]  = (int16_t)input[4];
    step1[5]  = (int16_t)input[20];
    step1[6]  = (int16_t)input[12];
    step1[7]  = (int16_t)input[28];
    step1[8]  = (int16_t)input[2];
    step1[9]  = (int16_t)input[18];
    step1[10] = (int16_t)input[10];
    step1[11] = (int16_t)input[26];
    step1[12] = (int16_t)input[6];
    step1[13] = (int16_t)input[22];
    step1[14] = (int16_t)input[14];
    step1[15] = (int16_t)input[30];

    temp1     = (int16_t)input[1] * cospi_31_64 - (int16_t)input[31] * cospi_1_64;
    temp2     = (int16_t)input[1] * cospi_1_64 + (int16_t)input[31] * cospi_31_64;
    step1[16] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[31] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = (int16_t)input[17] * cospi_15_64 - (int16_t)input[15] * cospi_17_64;
    temp2     = (int16_t)input[17] * cospi_17_64 + (int16_t)input[15] * cospi_15_64;
    step1[17] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[30] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = (int16_t)input[9] * cospi_23_64 - (int16_t)input[23] * cospi_9_64;
    temp2     = (int16_t)input[9] * cospi_9_64 + (int16_t)input[23] * cospi_23_64;
    step1[18] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[29] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = (int16_t)input[25] * cospi_7_64 - (int16_t)input[7] * cospi_25_64;
    temp2     = (int16_t)input[25] * cospi_25_64 + (int16_t)input[7] * cospi_7_64;
    step1[19] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[28] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = (int16_t)input[5] * cospi_27_64 - (int16_t)input[27] * cospi_5_64;
    temp2     = (int16_t)input[5] * cospi_5_64 + (int16_t)input[27] * cospi_27_64;
    step1[20] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[27] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = (int16_t)input[21] * cospi_11_64 - (int16_t)input[11] * cospi_21_64;
    temp2     = (int16_t)input[21] * cospi_21_64 + (int16_t)input[11] * cospi_11_64;
    step1[21] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[26] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = (int16_t)input[13] * cospi_19_64 - (int16_t)input[19] * cospi_13_64;
    temp2     = (int16_t)input[13] * cospi_13_64 + (int16_t)input[19] * cospi_19_64;
    step1[22] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[25] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = (int16_t)input[29] * cospi_3_64 - (int16_t)input[3] * cospi_29_64;
    temp2     = (int16_t)input[29] * cospi_29_64 + (int16_t)input[3] * cospi_3_64;
    step1[23] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[24] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    // stage 2
    step2[0] = step1[0];
    step2[1] = step1[1];
    step2[2] = step1[2];
    step2[3] = step1[3];
    step2[4] = step1[4];
    step2[5] = step1[5];
    step2[6] = step1[6];
    step2[7] = step1[7];

    temp1     = step1[8] * cospi_30_64 - step1[15] * cospi_2_64;
    temp2     = step1[8] * cospi_2_64 + step1[15] * cospi_30_64;
    step2[8]  = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[15] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = step1[9] * cospi_14_64 - step1[14] * cospi_18_64;
    temp2     = step1[9] * cospi_18_64 + step1[14] * cospi_14_64;
    step2[9]  = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[14] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = step1[10] * cospi_22_64 - step1[13] * cospi_10_64;
    temp2     = step1[10] * cospi_10_64 + step1[13] * cospi_22_64;
    step2[10] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[13] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    temp1     = step1[11] * cospi_6_64 - step1[12] * cospi_26_64;
    temp2     = step1[11] * cospi_26_64 + step1[12] * cospi_6_64;
    step2[11] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[12] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    step2[16] = (int16_t)WRAPLOW(step1[16] + step1[17]);
    step2[17] = (int16_t)WRAPLOW(step1[16] - step1[17]);
    step2[18] = (int16_t)WRAPLOW(-step1[18] + step1[19]);
    step2[19] = (int16_t)WRAPLOW(step1[18] + step1[19]);
    step2[20] = (int16_t)WRAPLOW(step1[20] + step1[21]);
    step2[21] = (int16_t)WRAPLOW(step1[20] - step1[21]);
    step2[22] = (int16_t)WRAPLOW(-step1[22] + step1[23]);
    step2[23] = (int16_t)WRAPLOW(step1[22] + step1[23]);
    step2[24] = (int16_t)WRAPLOW(step1[24] + step1[25]);
    step2[25] = (int16_t)WRAPLOW(step1[24] - step1[25]);
    step2[26] = (int16_t)WRAPLOW(-step1[26] + step1[27]);
    step2[27] = (int16_t)WRAPLOW(step1[26] + step1[27]);
    step2[28] = (int16_t)WRAPLOW(step1[28] + step1[29]);
    step2[29] = (int16_t)WRAPLOW(step1[28] - step1[29]);
    step2[30] = (int16_t)WRAPLOW(-step1[30] + step1[31]);
    step2[31] = (int16_t)WRAPLOW(step1[30] + step1[31]);

    // stage 3
    step1[0] = step2[0];
    step1[1] = step2[1];
    step1[2] = step2[2];
    step1[3] = step2[3];

    temp1    = step2[4] * cospi_28_64 - step2[7] * cospi_4_64;
    temp2    = step2[4] * cospi_4_64 + step2[7] * cospi_28_64;
    step1[4] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[7] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1    = step2[5] * cospi_12_64 - step2[6] * cospi_20_64;
    temp2    = step2[5] * cospi_20_64 + step2[6] * cospi_12_64;
    step1[5] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[6] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));

    step1[8]  = (int16_t)WRAPLOW(step2[8] + step2[9]);
    step1[9]  = (int16_t)WRAPLOW(step2[8] - step2[9]);
    step1[10] = (int16_t)WRAPLOW(-step2[10] + step2[11]);
    step1[11] = (int16_t)WRAPLOW(step2[10] + step2[11]);
    step1[12] = (int16_t)WRAPLOW(step2[12] + step2[13]);
    step1[13] = (int16_t)WRAPLOW(step2[12] - step2[13]);
    step1[14] = (int16_t)WRAPLOW(-step2[14] + step2[15]);
    step1[15] = (int16_t)WRAPLOW(step2[14] + step2[15]);

    step1[16] = step2[16];
    step1[31] = step2[31];
    temp1     = -step2[17] * cospi_4_64 + step2[30] * cospi_28_64;
    temp2     = step2[17] * cospi_28_64 + step2[30] * cospi_4_64;
    step1[17] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[30] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1     = -step2[18] * cospi_28_64 - step2[29] * cospi_4_64;
    temp2     = -step2[18] * cospi_4_64 + step2[29] * cospi_28_64;
    step1[18] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[29] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step1[19] = step2[19];
    step1[20] = step2[20];
    temp1     = -step2[21] * cospi_20_64 + step2[26] * cospi_12_64;
    temp2     = step2[21] * cospi_12_64 + step2[26] * cospi_20_64;
    step1[21] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[26] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1     = -step2[22] * cospi_12_64 - step2[25] * cospi_20_64;
    temp2     = -step2[22] * cospi_20_64 + step2[25] * cospi_12_64;
    step1[22] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[25] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step1[23] = step2[23];
    step1[24] = step2[24];
    step1[27] = step2[27];
    step1[28] = step2[28];

    // stage 4
    temp1    = (step1[0] + step1[1]) * cospi_16_64;
    temp2    = (step1[0] - step1[1]) * cospi_16_64;
    step2[0] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[1] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1    = step1[2] * cospi_24_64 - step1[3] * cospi_8_64;
    temp2    = step1[2] * cospi_8_64 + step1[3] * cospi_24_64;
    step2[2] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[3] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step2[4] = (int16_t)WRAPLOW(step1[4] + step1[5]);
    step2[5] = (int16_t)WRAPLOW(step1[4] - step1[5]);
    step2[6] = (int16_t)WRAPLOW(-step1[6] + step1[7]);
    step2[7] = (int16_t)WRAPLOW(step1[6] + step1[7]);

    step2[8]  = step1[8];
    step2[15] = step1[15];
    temp1     = -step1[9] * cospi_8_64 + step1[14] * cospi_24_64;
    temp2     = step1[9] * cospi_24_64 + step1[14] * cospi_8_64;
    step2[9]  = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[14] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1     = -step1[10] * cospi_24_64 - step1[13] * cospi_8_64;
    temp2     = -step1[10] * cospi_8_64 + step1[13] * cospi_24_64;
    step2[10] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[13] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step2[11] = step1[11];
    step2[12] = step1[12];

    step2[16] = (int16_t)WRAPLOW(step1[16] + step1[19]);
    step2[17] = (int16_t)WRAPLOW(step1[17] + step1[18]);
    step2[18] = (int16_t)WRAPLOW(step1[17] - step1[18]);
    step2[19] = (int16_t)WRAPLOW(step1[16] - step1[19]);
    step2[20] = (int16_t)WRAPLOW(-step1[20] + step1[23]);
    step2[21] = (int16_t)WRAPLOW(-step1[21] + step1[22]);
    step2[22] = (int16_t)WRAPLOW(step1[21] + step1[22]);
    step2[23] = (int16_t)WRAPLOW(step1[20] + step1[23]);

    step2[24] = (int16_t)WRAPLOW(step1[24] + step1[27]);
    step2[25] = (int16_t)WRAPLOW(step1[25] + step1[26]);
    step2[26] = (int16_t)WRAPLOW(step1[25] - step1[26]);
    step2[27] = (int16_t)WRAPLOW(step1[24] - step1[27]);
    step2[28] = (int16_t)WRAPLOW(-step1[28] + step1[31]);
    step2[29] = (int16_t)WRAPLOW(-step1[29] + step1[30]);
    step2[30] = (int16_t)WRAPLOW(step1[29] + step1[30]);
    step2[31] = (int16_t)WRAPLOW(step1[28] + step1[31]);

    // stage 5
    step1[0] = (int16_t)WRAPLOW(step2[0] + step2[3]);
    step1[1] = (int16_t)WRAPLOW(step2[1] + step2[2]);
    step1[2] = (int16_t)WRAPLOW(step2[1] - step2[2]);
    step1[3] = (int16_t)WRAPLOW(step2[0] - step2[3]);
    step1[4] = step2[4];
    temp1    = (step2[6] - step2[5]) * cospi_16_64;
    temp2    = (step2[5] + step2[6]) * cospi_16_64;
    step1[5] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[6] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step1[7] = step2[7];

    step1[8]  = (int16_t)WRAPLOW(step2[8] + step2[11]);
    step1[9]  = (int16_t)WRAPLOW(step2[9] + step2[10]);
    step1[10] = (int16_t)WRAPLOW(step2[9] - step2[10]);
    step1[11] = (int16_t)WRAPLOW(step2[8] - step2[11]);
    step1[12] = (int16_t)WRAPLOW(-step2[12] + step2[15]);
    step1[13] = (int16_t)WRAPLOW(-step2[13] + step2[14]);
    step1[14] = (int16_t)WRAPLOW(step2[13] + step2[14]);
    step1[15] = (int16_t)WRAPLOW(step2[12] + step2[15]);

    step1[16] = step2[16];
    step1[17] = step2[17];
    temp1     = -step2[18] * cospi_8_64 + step2[29] * cospi_24_64;
    temp2     = step2[18] * cospi_24_64 + step2[29] * cospi_8_64;
    step1[18] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[29] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1     = -step2[19] * cospi_8_64 + step2[28] * cospi_24_64;
    temp2     = step2[19] * cospi_24_64 + step2[28] * cospi_8_64;
    step1[19] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[28] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1     = -step2[20] * cospi_24_64 - step2[27] * cospi_8_64;
    temp2     = -step2[20] * cospi_8_64 + step2[27] * cospi_24_64;
    step1[20] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[27] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1     = -step2[21] * cospi_24_64 - step2[26] * cospi_8_64;
    temp2     = -step2[21] * cospi_8_64 + step2[26] * cospi_24_64;
    step1[21] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[26] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step1[22] = step2[22];
    step1[23] = step2[23];
    step1[24] = step2[24];
    step1[25] = step2[25];
    step1[30] = step2[30];
    step1[31] = step2[31];

    // stage 6
    step2[0]  = (int16_t)WRAPLOW(step1[0] + step1[7]);
    step2[1]  = (int16_t)WRAPLOW(step1[1] + step1[6]);
    step2[2]  = (int16_t)WRAPLOW(step1[2] + step1[5]);
    step2[3]  = (int16_t)WRAPLOW(step1[3] + step1[4]);
    step2[4]  = (int16_t)WRAPLOW(step1[3] - step1[4]);
    step2[5]  = (int16_t)WRAPLOW(step1[2] - step1[5]);
    step2[6]  = (int16_t)WRAPLOW(step1[1] - step1[6]);
    step2[7]  = (int16_t)WRAPLOW(step1[0] - step1[7]);
    step2[8]  = step1[8];
    step2[9]  = step1[9];
    temp1     = (-step1[10] + step1[13]) * cospi_16_64;
    temp2     = (step1[10] + step1[13]) * cospi_16_64;
    step2[10] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[13] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1     = (-step1[11] + step1[12]) * cospi_16_64;
    temp2     = (step1[11] + step1[12]) * cospi_16_64;
    step2[11] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step2[12] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step2[14] = step1[14];
    step2[15] = step1[15];

    step2[16] = (int16_t)WRAPLOW(step1[16] + step1[23]);
    step2[17] = (int16_t)WRAPLOW(step1[17] + step1[22]);
    step2[18] = (int16_t)WRAPLOW(step1[18] + step1[21]);
    step2[19] = (int16_t)WRAPLOW(step1[19] + step1[20]);
    step2[20] = (int16_t)WRAPLOW(step1[19] - step1[20]);
    step2[21] = (int16_t)WRAPLOW(step1[18] - step1[21]);
    step2[22] = (int16_t)WRAPLOW(step1[17] - step1[22]);
    step2[23] = (int16_t)WRAPLOW(step1[16] - step1[23]);

    step2[24] = (int16_t)WRAPLOW(-step1[24] + step1[31]);
    step2[25] = (int16_t)WRAPLOW(-step1[25] + step1[30]);
    step2[26] = (int16_t)WRAPLOW(-step1[26] + step1[29]);
    step2[27] = (int16_t)WRAPLOW(-step1[27] + step1[28]);
    step2[28] = (int16_t)WRAPLOW(step1[27] + step1[28]);
    step2[29] = (int16_t)WRAPLOW(step1[26] + step1[29]);
    step2[30] = (int16_t)WRAPLOW(step1[25] + step1[30]);
    step2[31] = (int16_t)WRAPLOW(step1[24] + step1[31]);

    // stage 7
    step1[0]  = (int16_t)WRAPLOW(step2[0] + step2[15]);
    step1[1]  = (int16_t)WRAPLOW(step2[1] + step2[14]);
    step1[2]  = (int16_t)WRAPLOW(step2[2] + step2[13]);
    step1[3]  = (int16_t)WRAPLOW(step2[3] + step2[12]);
    step1[4]  = (int16_t)WRAPLOW(step2[4] + step2[11]);
    step1[5]  = (int16_t)WRAPLOW(step2[5] + step2[10]);
    step1[6]  = (int16_t)WRAPLOW(step2[6] + step2[9]);
    step1[7]  = (int16_t)WRAPLOW(step2[7] + step2[8]);
    step1[8]  = (int16_t)WRAPLOW(step2[7] - step2[8]);
    step1[9]  = (int16_t)WRAPLOW(step2[6] - step2[9]);
    step1[10] = (int16_t)WRAPLOW(step2[5] - step2[10]);
    step1[11] = (int16_t)WRAPLOW(step2[4] - step2[11]);
    step1[12] = (int16_t)WRAPLOW(step2[3] - step2[12]);
    step1[13] = (int16_t)WRAPLOW(step2[2] - step2[13]);
    step1[14] = (int16_t)WRAPLOW(step2[1] - step2[14]);
    step1[15] = (int16_t)WRAPLOW(step2[0] - step2[15]);

    step1[16] = step2[16];
    step1[17] = step2[17];
    step1[18] = step2[18];
    step1[19] = step2[19];
    temp1     = (-step2[20] + step2[27]) * cospi_16_64;
    temp2     = (step2[20] + step2[27]) * cospi_16_64;
    step1[20] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[27] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1     = (-step2[21] + step2[26]) * cospi_16_64;
    temp2     = (step2[21] + step2[26]) * cospi_16_64;
    step1[21] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[26] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1     = (-step2[22] + step2[25]) * cospi_16_64;
    temp2     = (step2[22] + step2[25]) * cospi_16_64;
    step1[22] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[25] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    temp1     = (-step2[23] + step2[24]) * cospi_16_64;
    temp2     = (step2[23] + step2[24]) * cospi_16_64;
    step1[23] = (int16_t)WRAPLOW(dct_const_round_shift(temp1));
    step1[24] = (int16_t)WRAPLOW(dct_const_round_shift(temp2));
    step1[28] = step2[28];
    step1[29] = step2[29];
    step1[30] = step2[30];
    step1[31] = step2[31];

    // final stage
    output[0]  = (int16_t)WRAPLOW(step1[0] + step1[31]);
    output[1]  = (int16_t)WRAPLOW(step1[1] + step1[30]);
    output[2]  = (int16_t)WRAPLOW(step1[2] + step1[29]);
    output[3]  = (int16_t)WRAPLOW(step1[3] + step1[28]);
    output[4]  = (int16_t)WRAPLOW(step1[4] + step1[27]);
    output[5]  = (int16_t)WRAPLOW(step1[5] + step1[26]);
    output[6]  = (int16_t)WRAPLOW(step1[6] + step1[25]);
    output[7]  = (int16_t)WRAPLOW(step1[7] + step1[24]);
    output[8]  = (int16_t)WRAPLOW(step1[8] + step1[23]);
    output[9]  = (int16_t)WRAPLOW(step1[9] + step1[22]);
    output[10] = (int16_t)WRAPLOW(step1[10] + step1[21]);
    output[11] = (int16_t)WRAPLOW(step1[11] + step1[20]);
    output[12] = (int16_t)WRAPLOW(step1[12] + step1[19]);
    output[13] = (int16_t)WRAPLOW(step1[13] + step1[18]);
    output[14] = (int16_t)WRAPLOW(step1[14] + step1[17]);
    output[15] = (int16_t)WRAPLOW(step1[15] + step1[16]);
    output[16] = (int16_t)WRAPLOW(step1[15] - step1[16]);
    output[17] = (int16_t)WRAPLOW(step1[14] - step1[17]);
    output[18] = (int16_t)WRAPLOW(step1[13] - step1[18]);
    output[19] = (int16_t)WRAPLOW(step1[12] - step1[19]);
    output[20] = (int16_t)WRAPLOW(step1[11] - step1[20]);
    output[21] = (int16_t)WRAPLOW(step1[10] - step1[21]);
    output[22] = (int16_t)WRAPLOW(step1[9] - step1[22]);
    output[23] = (int16_t)WRAPLOW(step1[8] - step1[23]);
    output[24] = (int16_t)WRAPLOW(step1[7] - step1[24]);
    output[25] = (int16_t)WRAPLOW(step1[6] - step1[25]);
    output[26] = (int16_t)WRAPLOW(step1[5] - step1[26]);
    output[27] = (int16_t)WRAPLOW(step1[4] - step1[27]);
    output[28] = (int16_t)WRAPLOW(step1[3] - step1[28]);
    output[29] = (int16_t)WRAPLOW(step1[2] - step1[29]);
    output[30] = (int16_t)WRAPLOW(step1[1] - step1[30]);
    output[31] = (int16_t)WRAPLOW(step1[0] - step1[31]);
}

void eb_vp9_idct32x32_1024_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i, j;
    tran_low_t  out[32 * 32];
    tran_low_t *out_ptr = out;
    tran_low_t  temp_in[32], temp_out[32];

    // Rows
    for (i = 0; i < 32; ++i) {
        int16_t zero_coeff = 0;
        for (j = 0; j < 32; ++j) zero_coeff |= input[j];

        if (zero_coeff)
            idct32_c(input, out_ptr);
        else
            memset(out_ptr, 0, sizeof(tran_low_t) * 32);
        input += 32;
        out_ptr += 32;
    }

    // Columns
    for (i = 0; i < 32; ++i) {
        for (j = 0; j < 32; ++j) temp_in[j] = out[j * 32 + i];
        idct32_c(temp_in, temp_out);
        for (j = 0; j < 32; ++j) {
            dest[j * stride + i] = clip_pixel_add(dest[j * stride + i], ROUND_POWER_OF_TWO(temp_out[j], 6));
        }
    }
}

void eb_vp9_idct32x32_135_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i, j;
    tran_low_t  out[32 * 32] = {0};
    tran_low_t *out_ptr      = out;
    tran_low_t  temp_in[32], temp_out[32];

    // Rows
    // Only upper-left 16x16 has non-zero coeff
    for (i = 0; i < 16; ++i) {
        idct32_c(input, out_ptr);
        input += 32;
        out_ptr += 32;
    }

    // Columns
    for (i = 0; i < 32; ++i) {
        for (j = 0; j < 32; ++j) temp_in[j] = out[j * 32 + i];
        idct32_c(temp_in, temp_out);
        for (j = 0; j < 32; ++j) {
            dest[j * stride + i] = clip_pixel_add(dest[j * stride + i], ROUND_POWER_OF_TWO(temp_out[j], 6));
        }
    }
}

void eb_vp9_idct32x32_34_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i, j;
    tran_low_t  out[32 * 32] = {0};
    tran_low_t *out_ptr      = out;
    tran_low_t  temp_in[32], temp_out[32];

    // Rows
    // Only upper-left 8x8 has non-zero coeff
    for (i = 0; i < 8; ++i) {
        idct32_c(input, out_ptr);
        input += 32;
        out_ptr += 32;
    }

    // Columns
    for (i = 0; i < 32; ++i) {
        for (j = 0; j < 32; ++j) temp_in[j] = out[j * 32 + i];
        idct32_c(temp_in, temp_out);
        for (j = 0; j < 32; ++j) {
            dest[j * stride + i] = clip_pixel_add(dest[j * stride + i], ROUND_POWER_OF_TWO(temp_out[j], 6));
        }
    }
}

void eb_vp9_idct32x32_1_add_c(const tran_low_t *input, uint8_t *dest, int stride) {
    int         i, j;
    tran_high_t a1;
    tran_low_t  out = (int16_t)WRAPLOW(dct_const_round_shift((int16_t)input[0] * cospi_16_64));

    out = (int16_t)WRAPLOW(dct_const_round_shift(out * cospi_16_64));
    a1  = ROUND_POWER_OF_TWO(out, 6);

    for (j = 0; j < 32; ++j) {
        for (i = 0; i < 32; ++i) dest[i] = clip_pixel_add(dest[i], a1);
        dest += stride;
    }
}
