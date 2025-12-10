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
#include "vp9_reconintra.h"
#include "vp9_enums.h"
#include "stddef.h"
#include "vpx_dsp_rtcd.h"
#include "vpx_once.h"

const TX_TYPE eb_vp9_intra_mode_to_tx_type_lookup[INTRA_MODES] = {
    DCT_DCT, // DC
    ADST_DCT, // V
    DCT_ADST, // H
    DCT_DCT, // D45
    ADST_ADST, // D135
    ADST_DCT, // D117
    DCT_ADST, // D153
    DCT_ADST, // D207
    ADST_DCT, // D63
    ADST_ADST, // TM
};

enum {
    NEED_LEFT       = 1 << 1,
    NEED_ABOVE      = 1 << 2,
    NEED_ABOVERIGHT = 1 << 3,
};
typedef void (*intra_pred_fn)(uint8_t *dst, ptrdiff_t stride, const uint8_t *above, const uint8_t *left);

static intra_pred_fn pred[INTRA_MODES][TX_SIZES];
static intra_pred_fn dc_pred[2][2][TX_SIZES];

static void eb_vp9_init_intra_predictors_internal(void) {
#define INIT_ALL_SIZES(p, type)                    \
    p[TX_4X4]   = eb_vp9_##type##_predictor_4x4;   \
    p[TX_8X8]   = eb_vp9_##type##_predictor_8x8;   \
    p[TX_16X16] = eb_vp9_##type##_predictor_16x16; \
    p[TX_32X32] = eb_vp9_##type##_predictor_32x32

    INIT_ALL_SIZES(pred[V_PRED], v);
    INIT_ALL_SIZES(pred[H_PRED], h);
    INIT_ALL_SIZES(pred[D207_PRED], d207);
    INIT_ALL_SIZES(pred[D45_PRED], d45);
    INIT_ALL_SIZES(pred[D63_PRED], d63);
    INIT_ALL_SIZES(pred[D117_PRED], d117);
    INIT_ALL_SIZES(pred[D135_PRED], d135);
    INIT_ALL_SIZES(pred[D153_PRED], d153);
    INIT_ALL_SIZES(pred[TM_PRED], tm);
    INIT_ALL_SIZES(dc_pred[0][0], dc_128);
    INIT_ALL_SIZES(dc_pred[0][1], dc_top);
    INIT_ALL_SIZES(dc_pred[1][0], dc_left);
    INIT_ALL_SIZES(dc_pred[1][1], dc);

#undef intra_pred_allsizes
}

static void build_intra_predictors(EncDecContext *context_ptr, uint8_t *dst, int dst_stride, PREDICTION_MODE mode,
                                   TX_SIZE tx_size, int plane) {
    // predict
    if (mode == DC_PRED) {
        dc_pred[(plane == 0 && context_ptr->block_origin_x > 0) || ((ROUND_UV(context_ptr->block_origin_x) >> 1) > 0)]
               [(plane == 0 && context_ptr->block_origin_y > 0) || ((ROUND_UV(context_ptr->block_origin_y) >> 1) > 0)]
               [tx_size](dst, dst_stride, context_ptr->const_above_row[plane], context_ptr->left_col[plane]);
    } else {
        pred[mode][tx_size](dst, dst_stride, context_ptr->const_above_row[plane], context_ptr->left_col[plane]);
    }
}

void eb_vp9_predict_intra_block(EncDecContext *context_ptr, TX_SIZE tx_size, PREDICTION_MODE mode, uint8_t *dst,
                                int dst_stride, int plane) {
    build_intra_predictors(context_ptr, dst, dst_stride, mode, tx_size, plane);
}

void eb_vp9_init_intra_predictors(void) { once(eb_vp9_init_intra_predictors_internal); }
