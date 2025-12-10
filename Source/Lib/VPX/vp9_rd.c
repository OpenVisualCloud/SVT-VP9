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
#include <math.h>
#include <stdint.h>

#include "vpx_dsp_common.h"
//#include "vpx_mem.h"
#include "bitops.h"
#include "mem.h"
//#include "system_state.h"

#include "vp9_entropy.h"
#include "vp9_entropymode.h"
#include "EbUtility.h"
#include "vp9_pred_common.h"
#include "vp9_quant_common.h"

#include "vp9_cost.h"
//#include "vp9_encodemb.h"
#include "vp9_encodemv.h"
#include "vp9_encoder.h"
//#include "vp9_mcomp.h"
#include "vp9_ratectrl.h"
#include "vp9_rd.h"
#include "vp9_tokenize.h"

#define RD_THRESH_POW 1.25

// Factor to weigh the rate for switchable interp filters.
#define SWITCHABLE_INTERP_RATE_FACTOR 1

static const vpx_tree_index switchable_interp_tree[TREE_SIZE(SWITCHABLE_FILTERS)] = {
    -EIGHTTAP, 2, -EIGHTTAP_SMOOTH, -EIGHTTAP_SHARP};

static void fill_mode_costs(VP9_COMP *cpi) {
    const FRAME_CONTEXT *const fc = cpi->common.fc;
    int                        i, j;

    for (i = 0; i < INTRA_MODES; ++i) {
        for (j = 0; j < INTRA_MODES; ++j) {
            eb_vp9_cost_tokens(cpi->y_mode_costs[i][j], eb_vp9_kf_y_mode_prob[i][j], eb_vp9_intra_mode_tree);
        }
    }

    eb_vp9_cost_tokens(cpi->mbmode_cost, fc->y_mode_prob[1], eb_vp9_intra_mode_tree);
    for (i = 0; i < INTRA_MODES; ++i) {
        eb_vp9_cost_tokens(cpi->intra_uv_mode_cost[KEY_FRAME][i], eb_vp9_kf_uv_mode_prob[i], eb_vp9_intra_mode_tree);
        eb_vp9_cost_tokens(cpi->intra_uv_mode_cost[INTER_FRAME][i], fc->uv_mode_prob[i], eb_vp9_intra_mode_tree);
    }

    for (i = 0; i < SWITCHABLE_FILTER_CONTEXTS; ++i) {
        eb_vp9_cost_tokens(cpi->switchable_interp_costs[i], fc->switchable_interp_prob[i], switchable_interp_tree);
    }

    for (i = TX_8X8; i < TX_SIZES; ++i) {
        for (j = 0; j < TX_SIZE_CONTEXTS; ++j) {
            const vpx_prob *tx_probs = get_tx_probs((TX_SIZE)i, j, &fc->tx_probs);
            int             k;
            for (k = 0; k <= i; ++k) {
                int cost = 0;
                int m;
                for (m = 0; m <= k - (k == i); ++m) {
                    if (m == k)
                        cost += vp9_cost_zero(tx_probs[m]);
                    else
                        cost += vp9_cost_one(tx_probs[m]);
                }
                cpi->tx_size_cost[i - 1][j][k] = cost;
            }
        }
    }
}

static void fill_token_costs(vp9_coeff_cost *c, vp9_coeff_probs_model (*p)[PLANE_TYPES]) {
    int     i, j, k, l;
    TX_SIZE t;
    for (t = TX_4X4; t <= TX_32X32; ++t)
        for (i = 0; i < PLANE_TYPES; ++i)
            for (j = 0; j < REF_TYPES; ++j)
                for (k = 0; k < COEF_BANDS; ++k)
                    for (l = 0; l < BAND_COEFF_CONTEXTS(k); ++l) {
                        vpx_prob probs[ENTROPY_NODES];
                        eb_vp9_model_to_full_probs(p[t][i][j][k][l], probs);
                        eb_vp9_cost_tokens((int *)c[t][i][j][k][0][l], probs, eb_vp9_coef_tree);
                        eb_vp9_cost_tokens_skip((int *)c[t][i][j][k][1][l], probs, eb_vp9_coef_tree);
                        assert(c[t][i][j][k][0][l][EOB_TOKEN] == c[t][i][j][k][1][l][EOB_TOKEN]);
                    }
}

// Note that the element below for frame type "USE_BUF_FRAME", which indicates
// that the show frame flag is set, should not be used as no real frame
// is encoded so we should not reach here. However, a dummy value
// is inserted here to make sure the data structure has the right number
// of values assigned.
static int64_t compute_rd_mult_based_on_qindex(const VP9_COMP *cpi, int qindex) {
    const int64_t q      = eb_vp9_dc_quant(qindex, 0, cpi->common.bit_depth);
    int64_t       rdmult = 88 * q * q / 24;
    return rdmult;
}

int eb_vp9_compute_rd_mult(const VP9_COMP *cpi, int qindex) {
    int64_t rdmult = compute_rd_mult_based_on_qindex(cpi, qindex);
    if (rdmult < 1)
        rdmult = 1;
    return (int)rdmult;
}

void eb_vp9_initialize_rd_consts(VP9_COMP *cpi) {
    VP9_COMMON *const  cm = &cpi->common;
    MACROBLOCK *const  x  = &cpi->td.mb;
    MACROBLOCKD *const xd = &cpi->td.mb.e_mbd;
    RD_OPT *const      rd = &cpi->rd;
    int                i;
    rd->RDDIV       = RDDIV_BITS; // In bits (to multiply D by 128).
    rd->RDMULT      = eb_vp9_compute_rd_mult(cpi, cm->base_qindex + cm->y_dc_delta_q);
    rd->rd_mult_sad = (int)MAX(round(sqrtf((float)rd->RDMULT / 128) * 128), 1);

    set_error_per_bit(x, rd->RDMULT);

    x->select_tx_size = (/*cpi->sf.tx_size_search_method == USE_LARGESTALL &&*/
                         cm->frame_type != KEY_FRAME)
        ? 0
        : 1;
    set_partition_probs(cm, xd);
    {
        //if (!cpi->sf.use_nonrd_pick_mode || cm->frame_type == KEY_FRAME)
        fill_token_costs(x->token_costs, cm->fc->coef_probs);

        //if (cpi->sf.partition_search_type != VAR_BASED_PARTITION ||
        //    cm->frame_type == KEY_FRAME)
        {
            for (i = 0; i < PARTITION_CONTEXTS; ++i)
                eb_vp9_cost_tokens(cpi->partition_cost[i], get_partition_probs(xd, i), eb_vp9_partition_tree);
        }

        //if (!cpi->sf.use_nonrd_pick_mode || (cm->current_video_frame & 0x07) == 1 ||
        //    cm->frame_type == KEY_FRAME)
        {
            fill_mode_costs(cpi);

            if (!frame_is_intra_only(cm)) {
                eb_vp9_build_nmv_cost_table(x->nmvjointcost,
                                            cm->allow_high_precision_mv ? x->nmvcost_hp : x->nmvcost,
                                            &cm->fc->nmvc,
                                            cm->allow_high_precision_mv);

                for (i = 0; i < INTER_MODE_CONTEXTS; ++i)
                    eb_vp9_cost_tokens(
                        (int *)cpi->inter_mode_cost[i], cm->fc->inter_mode_probs[i], eb_vp9_inter_mode_tree);
            }
        }
    }
}

void eb_vp9_get_entropy_contexts(BLOCK_SIZE bsize, TX_SIZE tx_size, const struct macroblockd_plane *pd,
                                 ENTROPY_CONTEXT t_above[16], ENTROPY_CONTEXT t_left[16]) {
    const BLOCK_SIZE             plane_bsize = get_plane_block_size(bsize, pd);
    const int                    num_4x4_w   = eb_vp9_num_4x4_blocks_wide_lookup[plane_bsize];
    const int                    num_4x4_h   = eb_vp9_num_4x4_blocks_high_lookup[plane_bsize];
    const ENTROPY_CONTEXT *const above       = pd->above_context;
    const ENTROPY_CONTEXT *const left        = pd->left_context;

    int i;
    switch (tx_size) {
    case TX_4X4:
        memcpy(t_above, above, sizeof(ENTROPY_CONTEXT) * num_4x4_w);
        memcpy(t_left, left, sizeof(ENTROPY_CONTEXT) * num_4x4_h);
        break;
    case TX_8X8:
        for (i = 0; i < num_4x4_w; i += 2) {
            assert(i < 16);
            t_above[i] = !!*(const uint16_t *)&above[i];
        }
        for (i = 0; i < num_4x4_h; i += 2) {
            assert(i < 16);
            t_left[i] = !!*(const uint16_t *)&left[i];
        }
        break;
    case TX_16X16:
        for (i = 0; i < num_4x4_w; i += 4) {
            assert(i < 16);
            t_above[i] = !!*(const uint32_t *)&above[i];
        }
        for (i = 0; i < num_4x4_h; i += 4) {
            assert(i < 16);
            t_left[i] = !!*(const uint32_t *)&left[i];
        }
        break;
    default:
        assert(tx_size == TX_32X32);
        for (i = 0; i < num_4x4_w; i += 8) {
            assert(i < 16);
            t_above[i] = !!*(const uint64_t *)&above[i];
        }
        for (i = 0; i < num_4x4_h; i += 8) {
            assert(i < 16);
            t_left[i] = !!*(const uint64_t *)&left[i];
        }
        break;
    }
}
