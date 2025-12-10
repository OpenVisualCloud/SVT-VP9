/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdint.h>
#include "vp9_rtcd.h"
#include "vpx_dsp_rtcd.h"
#include "vp9_idct.h"
#include "inv_txfm.h"
#include "mem.h"

void eb_vp9_iht4x4_16_add_c(const tran_low_t *input, uint8_t *dest, int stride, int tx_type) {
    const transform_2d IHT_4[] = {
        {eb_vp9_idct4_c, eb_vp9_idct4_c}, // DCT_DCT  = 0
        {eb_vp9_iadst4_c, eb_vp9_idct4_c}, // ADST_DCT = 1
        {eb_vp9_idct4_c, eb_vp9_iadst4_c}, // DCT_ADST = 2
        {eb_vp9_iadst4_c, eb_vp9_iadst4_c} // ADST_ADST = 3
    };

    int         i, j;
    tran_low_t  out[4 * 4];
    tran_low_t *out_ptr = out;
    tran_low_t  temp_in[4], temp_out[4];

    // inverse transform row vectors
    for (i = 0; i < 4; ++i) {
        IHT_4[tx_type].rows(input, out_ptr);
        input += 4;
        out_ptr += 4;
    }

    // inverse transform column vectors
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) temp_in[j] = out[j * 4 + i];
        IHT_4[tx_type].cols(temp_in, temp_out);
        for (j = 0; j < 4; ++j) {
            dest[j * stride + i] = clip_pixel_add(dest[j * stride + i], ROUND_POWER_OF_TWO(temp_out[j], 4));
        }
    }
}

static const transform_2d IHT_8[] = {
    {eb_vp9_idct8_c, eb_vp9_idct8_c}, // DCT_DCT  = 0
    {eb_vp9_iadst8_c, eb_vp9_idct8_c}, // ADST_DCT = 1
    {eb_vp9_idct8_c, eb_vp9_iadst8_c}, // DCT_ADST = 2
    {eb_vp9_iadst8_c, eb_vp9_iadst8_c} // ADST_ADST = 3
};

void eb_vp9_iht8x8_64_add_c(const tran_low_t *input, uint8_t *dest, int stride, int tx_type) {
    int                i, j;
    tran_low_t         out[8 * 8];
    tran_low_t        *out_ptr = out;
    tran_low_t         temp_in[8], temp_out[8];
    const transform_2d ht = IHT_8[tx_type];

    // inverse transform row vectors
    for (i = 0; i < 8; ++i) {
        ht.rows(input, out_ptr);
        input += 8;
        out_ptr += 8;
    }

    // inverse transform column vectors
    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) temp_in[j] = out[j * 8 + i];
        ht.cols(temp_in, temp_out);
        for (j = 0; j < 8; ++j) {
            dest[j * stride + i] = clip_pixel_add(dest[j * stride + i], ROUND_POWER_OF_TWO(temp_out[j], 5));
        }
    }
}

static const transform_2d IHT_16[] = {
    {eb_vp9_idct16_c, eb_vp9_idct16_c}, // DCT_DCT  = 0
    {eb_vp9_iadst16_c, eb_vp9_idct16_c}, // ADST_DCT = 1
    {eb_vp9_idct16_c, eb_vp9_iadst16_c}, // DCT_ADST = 2
    {eb_vp9_iadst16_c, eb_vp9_iadst16_c} // ADST_ADST = 3
};

void eb_vp9_iht16x16_256_add_c(const tran_low_t *input, uint8_t *dest, int stride, int tx_type) {
    int                i, j;
    tran_low_t         out[16 * 16];
    tran_low_t        *out_ptr = out;
    tran_low_t         temp_in[16], temp_out[16];
    const transform_2d ht = IHT_16[tx_type];

    // Rows
    for (i = 0; i < 16; ++i) {
        ht.rows(input, out_ptr);
        input += 16;
        out_ptr += 16;
    }

    // Columns
    for (i = 0; i < 16; ++i) {
        for (j = 0; j < 16; ++j) temp_in[j] = out[j * 16 + i];
        ht.cols(temp_in, temp_out);
        for (j = 0; j < 16; ++j) {
            dest[j * stride + i] = clip_pixel_add(dest[j * stride + i], ROUND_POWER_OF_TWO(temp_out[j], 6));
        }
    }
}

// idct
void eb_vp9_idct4x4_add(const tran_low_t *input, uint8_t *dest, int stride, int eob) {
    if (eob > 1)
        eb_vpx_idct4x4_16_add(input, dest, stride);
    else
        eb_vpx_idct4x4_1_add(input, dest, stride);
}

static void idct8x8_add(const tran_low_t *input, uint8_t *dest, int stride, int eob) {
    // If dc is 1, then input[0] is the reconstructed value, do not need
    // dequantization. Also, when dc is 1, dc is counted in eobs, namely eobs >=1.

    // The calculation can be simplified if there are not many non-zero dct
    // coefficients. Use eobs to decide what to do.
    if (eob == 1)
        // DC only DCT coefficient
        eb_vpx_idct8x8_1_add(input, dest, stride);
    else if (eob <= 12)
        eb_vp9_idct8x8_12_add(input, dest, stride);
    else
        eb_vp9_idct8x8_64_add(input, dest, stride);
}

static void idct16x16_add(const tran_low_t *input, uint8_t *dest, int stride, int eob) {
    /* The calculation can be simplified if there are not many non-zero dct
   * coefficients. Use eobs to separate different cases. */
    if (eob == 1) /* DC only DCT coefficient. */
        eb_vpx_idct16x16_1_add(input, dest, stride);
    else if (eob <= 10)
        eb_vpx_idct16x16_10_add(input, dest, stride);
    else if (eob <= 38)
        eb_vpx_idct16x16_38_add(input, dest, stride);
    else
        eb_vpx_idct16x16_256_add(input, dest, stride);
}

void eb_vp9_idct32x32_add(const tran_low_t *input, uint8_t *dest, int stride, int eob) {
    if (eob == 1)
        eb_vpx_idct32x32_1_add(input, dest, stride);
    else if (eob <= 34)
        // non-zero coeff only in upper-left 8x8
        eb_vp9_idct32x32_34_add(input, dest, stride);
    else if (eob <= 135)
        // non-zero coeff only in upper-left 16x16
        eb_vp9_idct32x32_135_add(input, dest, stride);
    else
        eb_vp9_idct32x32_1024_add(input, dest, stride);
}

void eb_vp9_iht8x8_add(TX_TYPE tx_type, const tran_low_t *input, uint8_t *dest, int stride, int eob) {
    if (tx_type == DCT_DCT) {
        idct8x8_add(input, dest, stride, eob);
    } else {
        eb_vp9_iht8x8_64_add(input, dest, stride, tx_type);
    }
}

void eb_vp9_iht16x16_add(TX_TYPE tx_type, const tran_low_t *input, uint8_t *dest, int stride, int eob) {
    if (tx_type == DCT_DCT) {
        idct16x16_add(input, dest, stride, eob);
    } else {
        eb_vp9_iht16x16_256_add(input, dest, stride, tx_type);
    }
}
