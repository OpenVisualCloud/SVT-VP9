/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>
#include "EbDefinitions.h"
#include "mem.h"

#include "vp9_quant_common.h"
#include "vp9_encoder.h"
#include "vp9_quantize.h"

void eb_vp9_quantize_fp_c(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *round_ptr,
                          const int16_t *quant_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr,
                          const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan) {
    int i, eob = -1;
    (void)iscan;
    (void)skip_block;
    assert(!skip_block);

    memset(qcoeff_ptr, 0, n_coeffs * sizeof(*qcoeff_ptr));
    memset(dqcoeff_ptr, 0, n_coeffs * sizeof(*dqcoeff_ptr));

    // Quantization pass: All coefficients with index >= zero_flag are
    // skippable. Note: zero_flag can be zero.
    for (i = 0; i < n_coeffs; i++) {
        const int rc         = scan[i];
        const int coeff      = coeff_ptr[rc];
        const int coeff_sign = (coeff >> 31);
        const int abs_coeff  = (coeff ^ coeff_sign) - coeff_sign;

        int tmp = clamp(abs_coeff + round_ptr[rc != 0], INT16_MIN, INT16_MAX);
        tmp     = (tmp * quant_ptr[rc != 0]) >> 16;

        qcoeff_ptr[rc]  = (int16_t)((tmp ^ coeff_sign) - coeff_sign);
        dqcoeff_ptr[rc] = qcoeff_ptr[rc] * dequant_ptr[rc != 0];

        if (tmp)
            eob = i;
    }
    *eob_ptr = (int16_t)(eob + 1);
}

// TODO(jingning) Refactor this file and combine functions with similar
// operations.
void eb_vp9_quantize_fp_32x32_c(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block,
                                const int16_t *round_ptr, const int16_t *quant_ptr, tran_low_t *qcoeff_ptr,
                                tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr,
                                const int16_t *scan, const int16_t *iscan) {
    int i, eob = -1;
    (void)iscan;
    (void)skip_block;
    assert(!skip_block);

    memset(qcoeff_ptr, 0, n_coeffs * sizeof(*qcoeff_ptr));
    memset(dqcoeff_ptr, 0, n_coeffs * sizeof(*dqcoeff_ptr));

    for (i = 0; i < n_coeffs; i++) {
        const int rc         = scan[i];
        const int coeff      = coeff_ptr[rc];
        const int coeff_sign = (coeff >> 31);
        int       tmp        = 0;
        int       abs_coeff  = (coeff ^ coeff_sign) - coeff_sign;

        if (abs_coeff >= (dequant_ptr[rc != 0] >> 2)) {
            abs_coeff += ROUND_POWER_OF_TWO(round_ptr[rc != 0], 1);
            abs_coeff       = clamp(abs_coeff, INT16_MIN, INT16_MAX);
            tmp             = (abs_coeff * quant_ptr[rc != 0]) >> 15;
            qcoeff_ptr[rc]  = (int16_t)((tmp ^ coeff_sign) - coeff_sign);
            dqcoeff_ptr[rc] = qcoeff_ptr[rc] * dequant_ptr[rc != 0] / 2;
        }

        if (tmp)
            eob = i;
    }
    *eob_ptr = (int16_t)(eob + 1);
}

static void invert_quant(int16_t *quant, int16_t *shift, int d) {
    unsigned t;
    int      l, m;
    t = d;
    for (l = 0; t > 1; l++) t >>= 1;
    m      = 1 + (1 << (16 + l)) / d;
    *quant = (int16_t)(m - (1 << 16));
    *shift = 1 << (16 - l);
}

static int get_qzbin_factor(int q, vpx_bit_depth_t bit_depth) {
    const int quant = eb_vp9_dc_quant(q, 0, bit_depth);
    (void)bit_depth;
    return q == 0 ? 64 : (quant < 148 ? 84 : 80);
}

void eb_vp9_init_quantizer(VP9_COMP *cpi) {
    VP9_COMMON *const cm     = &cpi->common;
    QUANTS *const     quants = &cpi->quants;
    int               i, q, quant;
    int               sharpness = 0;
    for (q = 0; q < QINDEX_RANGE; q++) {
        int       qzbin_factor         = get_qzbin_factor(q, cm->bit_depth);
        int       qrounding_factor     = q == 0 ? 64 : 48;
        const int sharpness_adjustment = 16 * (7 - sharpness) / 7;

        if (sharpness > 0 && q > 0) {
            qzbin_factor     = 64 + sharpness_adjustment;
            qrounding_factor = 64 - sharpness_adjustment;
        }

        for (i = 0; i < 2; ++i) {
            int qrounding_factor_fp = i == 0 ? 48 : 42;
            if (q == 0)
                qrounding_factor_fp = 64;
            if (sharpness > 0)
                qrounding_factor_fp = 64 - sharpness_adjustment;
            // y
            quant = i == 0 ? eb_vp9_dc_quant(q, cm->y_dc_delta_q, cm->bit_depth) : eb_vp9_ac_quant(q, 0, cm->bit_depth);
            invert_quant(&quants->y_quant[q][i], &quants->y_quant_shift[q][i], quant);
            quants->y_quant_fp[q][i] = (int16_t)((1 << 16) / quant);
            quants->y_round_fp[q][i] = (int16_t)((qrounding_factor_fp * quant) >> 7);
            quants->y_zbin[q][i]     = (int16_t)ROUND_POWER_OF_TWO(qzbin_factor * quant, 7);
            quants->y_round[q][i]    = (int16_t)((qrounding_factor * quant) >> 7);
            cpi->y_dequant[q][i]     = (int16_t)quant;

            // uv
            quant = i == 0 ? eb_vp9_dc_quant(q, cm->uv_dc_delta_q, cm->bit_depth)
                           : eb_vp9_ac_quant(q, cm->uv_ac_delta_q, cm->bit_depth);
            invert_quant(&quants->uv_quant[q][i], &quants->uv_quant_shift[q][i], quant);
            quants->uv_quant_fp[q][i] = (int16_t)((1 << 16) / quant);
            quants->uv_round_fp[q][i] = (int16_t)((qrounding_factor_fp * quant) >> 7);
            quants->uv_zbin[q][i]     = (int16_t)ROUND_POWER_OF_TWO(qzbin_factor * quant, 7);
            quants->uv_round[q][i]    = (int16_t)((qrounding_factor * quant) >> 7);
            cpi->uv_dequant[q][i]     = (int16_t)quant;
        }

        for (i = 2; i < 8; i++) {
            quants->y_quant[q][i]       = quants->y_quant[q][1];
            quants->y_quant_fp[q][i]    = quants->y_quant_fp[q][1];
            quants->y_round_fp[q][i]    = quants->y_round_fp[q][1];
            quants->y_quant_shift[q][i] = quants->y_quant_shift[q][1];
            quants->y_zbin[q][i]        = quants->y_zbin[q][1];
            quants->y_round[q][i]       = quants->y_round[q][1];
            cpi->y_dequant[q][i]        = cpi->y_dequant[q][1];

            quants->uv_quant[q][i]       = quants->uv_quant[q][1];
            quants->uv_quant_fp[q][i]    = quants->uv_quant_fp[q][1];
            quants->uv_round_fp[q][i]    = quants->uv_round_fp[q][1];
            quants->uv_quant_shift[q][i] = quants->uv_quant_shift[q][1];
            quants->uv_zbin[q][i]        = quants->uv_zbin[q][1];
            quants->uv_round[q][i]       = quants->uv_round[q][1];
            cpi->uv_dequant[q][i]        = cpi->uv_dequant[q][1];
        }
    }
}
// Table that converts 0-63 Q-range values passed in outside to the Qindex
// range used internally.
static const int quantizer_to_qindex[] = {
    0,   4,   8,   12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,  64,  68,  72,  76,  80,  84,
    88,  92,  96,  100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172,
    176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 249, 255,
};

int eb_vp9_quantizer_to_qindex(int quantizer) { return quantizer_to_qindex[quantizer]; }

int eb_vp9_qindex_to_quantizer(int qindex) {
    int quantizer;

    for (quantizer = 0; quantizer < 64; ++quantizer)
        if (quantizer_to_qindex[quantizer] >= qindex)
            return quantizer;

    return 63;
}
