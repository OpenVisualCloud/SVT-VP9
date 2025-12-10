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
#include <stdint.h>

#include "vpx_dsp_common.h"

#include "vp9_entropy.h"
#include "vp9_entropymode.h"
#include "vp9_block.h"
#include "vp9_pred_common.h"
#include "vp9_blockd.h"
#include "vp9_scan.h"
#include "vp9_seg_common.h"

#include "vp9_cost.h"
#include "vp9_encoder.h"
#include "vp9_rdopt.h"

struct rdcost_block_args {
    const VP9_COMP   *cpi;
    MACROBLOCK       *x;
    ENTROPY_CONTEXT   t_above[16];
    ENTROPY_CONTEXT   t_left[16];
    int               this_rate;
    int64_t           this_dist;
    int64_t           this_sse;
    int64_t           this_rd;
    int64_t           best_rd;
    int               exit_early;
    int               use_fast_coef_costing;
    const scan_order *so;
    uint8_t           skippable;
};
/* The trailing '0' is a terminator which is used inside cost_coeffs() to
 * decide whether to include cost of a trailing EOB node or not (i.e. we
 * can skip this if the last coefficient in this transform block, e.g. the
 * 16th coefficient in a 4x4 block or the 64th coefficient in a 8x8 block,
 * were non-zero). */
static const int16_t band_counts[TX_SIZES][8] = {
    {1, 2, 3, 4, 3, 16 - 13, 0},
    {1, 2, 3, 4, 11, 64 - 21, 0},
    {1, 2, 3, 4, 11, 256 - 21, 0},
    {1, 2, 3, 4, 11, 1024 - 21, 0},
};
int cost_coeffs(MACROBLOCK *x, int plane, int block, TX_SIZE tx_size, int pt, const int16_t *scan, const int16_t *nb,
                int use_fast_coef_costing) {
    MACROBLOCKD *const             xd                              = &x->e_mbd;
    ModeInfo                      *mi                              = xd->mi[0];
    const struct macroblock_plane *p                               = &x->plane[plane];
    const PLANE_TYPE               type                            = get_plane_type(plane);
    const int16_t                 *band_count                      = &band_counts[tx_size][1];
    const int                      eob                             = p->eobs[block];
    const tran_low_t *const        qcoeff                          = BLOCK_OFFSET(p->qcoeff, block);
    unsigned int (*token_costs)[2][COEFF_CONTEXTS][ENTROPY_TOKENS] = x->token_costs[tx_size][type][is_inter_block(mi)];
    uint8_t         token_cache[32 * 32];
    int             cost;
    const uint16_t *cat6_high_cost = vp9_get_high_cost_table(8);

    // Check for consistency of tx_size with mode info
    assert(type == PLANE_TYPE_Y ? mi->tx_size == tx_size : get_uv_tx_size(mi, &xd->plane[plane]) == tx_size);

    if (eob == 0) {
        // single eob token
        cost = token_costs[0][0][pt][EOB_TOKEN];
    } else {
        if (use_fast_coef_costing) {
            int band_left = *band_count++;
            int c;

            // dc token
            int     v = qcoeff[0];
            int16_t prev_t;
            cost = vp9_get_token_cost(v, &prev_t, cat6_high_cost);
            cost += (*token_costs)[0][pt][prev_t];

            token_cache[0] = eb_vp9_pt_energy_class[prev_t];
            ++token_costs;

            // ac tokens
            for (c = 1; c < eob; c++) {
                const int rc = scan[c];
                int16_t   t;

                v = qcoeff[rc];
                cost += vp9_get_token_cost(v, &t, cat6_high_cost);
                cost += (*token_costs)[!prev_t][!prev_t][t];
                prev_t = t;
                if (!--band_left) {
                    band_left = *band_count++;
                    ++token_costs;
                }
            }

            // eob token
            if (band_left)
                cost += (*token_costs)[0][!prev_t][EOB_TOKEN];

        } else { // !use_fast_coef_costing
            int band_left = *band_count++;
            int c;

            // dc token
            int     v = qcoeff[0];
            int16_t tok;
            unsigned int (*tok_cost_ptr)[COEFF_CONTEXTS][ENTROPY_TOKENS];
            cost = vp9_get_token_cost(v, &tok, cat6_high_cost);
            cost += (*token_costs)[0][pt][tok];

            token_cache[0] = eb_vp9_pt_energy_class[tok];
            ++token_costs;

            tok_cost_ptr = &((*token_costs)[!tok]);

            // ac tokens
            for (c = 1; c < eob; c++) {
                const int rc = scan[c];

                v = qcoeff[rc];
                cost += vp9_get_token_cost(v, &tok, cat6_high_cost);
                pt = get_coef_context(nb, token_cache, c);
                cost += (*tok_cost_ptr)[pt][tok];
                token_cache[rc] = eb_vp9_pt_energy_class[tok];
                if (!--band_left) {
                    band_left = *band_count++;
                    ++token_costs;
                }
                tok_cost_ptr = &((*token_costs)[!tok]);
            }

            // eob token
            if (band_left) {
                pt = get_coef_context(nb, token_cache, c);
                cost += (*token_costs)[0][pt][EOB_TOKEN];
            }
        }
    }

    return cost;
}
int cost_mv_ref(const VP9_COMP *cpi, PREDICTION_MODE mode, int mode_context) {
    assert(is_inter_mode(mode));
    return cpi->inter_mode_cost[mode_context][INTER_OFFSET(mode)];
}
void estimate_ref_frame_costs(const VP9_COMMON *cm, const MACROBLOCKD *xd, int segment_id,
                              unsigned int *ref_costs_single, unsigned int *ref_costs_comp, vpx_prob *comp_mode_p) {
    int seg_ref_active = segfeature_active(&cm->seg, segment_id, SEG_LVL_REF_FRAME);
    if (seg_ref_active) {
        memset(ref_costs_single, 0, MAX_REF_FRAMES * sizeof(*ref_costs_single));
        memset(ref_costs_comp, 0, MAX_REF_FRAMES * sizeof(*ref_costs_comp));
        *comp_mode_p = 128;
    } else {
        vpx_prob intra_inter_p = vp9_get_intra_inter_prob(cm, xd);
        vpx_prob comp_inter_p  = 128;

        if (cm->reference_mode == REFERENCE_MODE_SELECT) {
            comp_inter_p = vp9_get_reference_mode_prob(cm, xd);
            *comp_mode_p = comp_inter_p;
        } else {
            *comp_mode_p = 128;
        }

        ref_costs_single[INTRA_FRAME] = vp9_cost_bit(intra_inter_p, 0);

        if (cm->reference_mode != COMPOUND_REFERENCE) {
            vpx_prob     ref_single_p1 = vp9_get_pred_prob_single_ref_p1(cm, xd);
            vpx_prob     ref_single_p2 = vp9_get_pred_prob_single_ref_p2(cm, xd);
            unsigned int base_cost     = vp9_cost_bit(intra_inter_p, 1);

            if (cm->reference_mode == REFERENCE_MODE_SELECT)
                base_cost += vp9_cost_bit(comp_inter_p, 0);

            ref_costs_single[LAST_FRAME] = ref_costs_single[GOLDEN_FRAME] = ref_costs_single[ALTREF_FRAME] = base_cost;
            ref_costs_single[LAST_FRAME] += vp9_cost_bit(ref_single_p1, 0);
            ref_costs_single[GOLDEN_FRAME] += vp9_cost_bit(ref_single_p1, 1);
            ref_costs_single[ALTREF_FRAME] += vp9_cost_bit(ref_single_p1, 1);
            ref_costs_single[GOLDEN_FRAME] += vp9_cost_bit(ref_single_p2, 0);
            ref_costs_single[ALTREF_FRAME] += vp9_cost_bit(ref_single_p2, 1);
        } else {
            ref_costs_single[LAST_FRAME]   = 512;
            ref_costs_single[GOLDEN_FRAME] = 512;
            ref_costs_single[ALTREF_FRAME] = 512;
        }
        if (cm->reference_mode != SINGLE_REFERENCE) {
            vpx_prob     ref_comp_p = vp9_get_pred_prob_comp_ref_p(cm, xd);
            unsigned int base_cost  = vp9_cost_bit(intra_inter_p, 1);

            if (cm->reference_mode == REFERENCE_MODE_SELECT)
                base_cost += vp9_cost_bit(comp_inter_p, 1);

            ref_costs_comp[LAST_FRAME]   = base_cost + vp9_cost_bit(ref_comp_p, 0);
            ref_costs_comp[GOLDEN_FRAME] = base_cost + vp9_cost_bit(ref_comp_p, 1);
        } else {
            ref_costs_comp[LAST_FRAME]   = 512;
            ref_costs_comp[GOLDEN_FRAME] = 512;
        }
    }
}
