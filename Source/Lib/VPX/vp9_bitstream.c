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

#include "bitwriter_buffer.h"
#include "vpx_dsp_common.h"

#include "vp9_treewriter.h"
#include "vp9_filter.h"
#include "vp9_enums.h"
#include "bitwriter.h"
#include "vp9_entropymode.h"
#include "vp9_tokenize.h"
#include "vpx_codec.h"
#include "vp9_common.h"
#include "vp9_subexp.h"
#include "vp9_onyxc_int.h"
#include "vp9_pred_common.h"
#include "vp9_block.h"
#include "vp9_encoder.h"
#include "vp9_encodemv.h"
#include "vp9_blockd.h"
#include "vp9_bitstream.h"
#include "vp9_seg_common.h"
#include "vp9_speed_features.h"
#include "vp9_cost.h"
#include "vp9_quant_common.h"

static const struct vp9_token intra_mode_encodings[INTRA_MODES] = {
    {0, 1}, {6, 3}, {28, 5}, {30, 5}, {58, 6}, {59, 6}, {126, 7}, {127, 7}, {62, 6}, {2, 2}};
static const struct vp9_token partition_encodings[PARTITION_TYPES] = {{0, 1}, {2, 2}, {6, 3}, {7, 3}};
static const struct vp9_token inter_mode_encodings[INTER_MODES]    = {{2, 2}, {6, 3}, {0, 1}, {7, 3}};

static void write_intra_mode(VpxWriter *w, PREDICTION_MODE mode, const vpx_prob *probs) {
    vp9_write_token(w, eb_vp9_intra_mode_tree, probs, &intra_mode_encodings[mode]);
}

static void write_inter_mode(VpxWriter *w, PREDICTION_MODE mode, const vpx_prob *probs) {
    assert(is_inter_mode(mode));
    vp9_write_token(w, eb_vp9_inter_mode_tree, probs, &inter_mode_encodings[INTER_OFFSET(mode)]);
}

static void prob_diff_update(const vpx_tree_index *tree, vpx_prob probs[/*n - 1*/],
                             const unsigned int counts[/*n - 1*/], int n, VpxWriter *w) {
    int          i;
    unsigned int branch_ct[32][2];

    // Assuming max number of probabilities <= 32
    assert(n <= 32);

    eb_vp9_tree_probs_from_distribution(tree, branch_ct, counts);
    for (i = 0; i < n - 1; ++i) eb_vp9_cond_prob_diff_update(w, &probs[i], branch_ct[i]);
}

static void write_selected_tx_size(const VP9_COMMON *cm, const MACROBLOCKD *const xd, VpxWriter *w) {
    TX_SIZE               tx_size     = xd->mi[0]->tx_size;
    BLOCK_SIZE            bsize       = xd->mi[0]->sb_type;
    const TX_SIZE         max_tx_size = eb_vp9_max_txsize_lookup[bsize];
    const vpx_prob *const tx_probs    = get_tx_probs(max_tx_size, get_tx_size_context(xd), &cm->fc->tx_probs);
    vpx_write(w, tx_size != TX_4X4, tx_probs[0]);
    if (tx_size != TX_4X4 && max_tx_size >= TX_16X16) {
        vpx_write(w, tx_size != TX_8X8, tx_probs[1]);
        if (tx_size != TX_8X8 && max_tx_size >= TX_32X32)
            vpx_write(w, tx_size != TX_16X16, tx_probs[2]);
    }
}

static int write_skip(const VP9_COMMON *cm, const MACROBLOCKD *const xd, int segment_id, const ModeInfo *mi,
                      VpxWriter *w) {
    if (segfeature_active(&cm->seg, segment_id, SEG_LVL_SKIP)) {
        return 1;
    } else {
        const int skip = mi->skip;
        vpx_write(w, skip, vp9_get_skip_prob(cm, xd));
        return skip;
    }
}

static void update_skip_probs(VP9_COMMON *cm, VpxWriter *w, FRAME_COUNTS *counts) {
    int k;

    for (k = 0; k < SKIP_CONTEXTS; ++k) eb_vp9_cond_prob_diff_update(w, &cm->fc->skip_probs[k], counts->skip[k]);
}

static void pack_mb_tokens(VpxWriter *w, TOKENEXTRA **tp, const TOKENEXTRA *const stop, vpx_bit_depth_t bit_depth) {
    const TOKENEXTRA          *p;
    const vp9_extra_bit *const extra_bits = eb_vp9_extra_bits;
    (void)bit_depth;

    for (p = *tp; p < stop && p->token != EOSB_TOKEN; ++p) {
        if (p->token == EOB_TOKEN) {
            vpx_write(w, 0, p->context_tree[0]);
            continue;
        }
        vpx_write(w, 1, p->context_tree[0]);
        while (p->token == ZERO_TOKEN) {
            vpx_write(w, 0, p->context_tree[1]);
            ++p;
            if (p == stop || p->token == EOSB_TOKEN) {
                *tp = (TOKENEXTRA *)(uintptr_t)p + (p->token == EOSB_TOKEN);
                return;
            }
        }

        {
            const int             t            = p->token;
            const vpx_prob *const context_tree = p->context_tree;
            assert(t != ZERO_TOKEN);
            assert(t != EOB_TOKEN);
            assert(t != EOSB_TOKEN);
            vpx_write(w, 1, context_tree[1]);
            if (t == ONE_TOKEN) {
                vpx_write(w, 0, context_tree[2]);
                vpx_write_bit(w, p->extra & 1);
            } else { // t >= TWO_TOKEN && t < EOB_TOKEN
                const struct vp9_token *const a = &eb_vp9_coef_encodings[t];
                const int                     v = a->value;
                const int                     n = a->len;
                const int                     e = p->extra;
                vpx_write(w, 1, context_tree[2]);
                vp9_write_tree(w,
                               eb_vp9_coef_con_tree,
                               eb_vp9_pareto8_full[context_tree[PIVOT_NODE] - 1],
                               v,
                               n - UNCONSTRAINED_NODES,
                               0);
                if (t >= CATEGORY1_TOKEN) {
                    const vp9_extra_bit *const b  = &extra_bits[t];
                    const unsigned char       *pb = b->prob;
                    int                        v  = e >> 1;
                    int                        n  = b->len; // number of bits in v, assumed nonzero
                    do {
                        const int bb = (v >> --n) & 1;
                        vpx_write(w, bb, *pb++);
                    } while (n);
                }
                vpx_write_bit(w, e & 1);
            }
        }
    }
    *tp = (TOKENEXTRA *)(uintptr_t)p + (p->token == EOSB_TOKEN);
}
// This function encodes the reference frame
static void write_ref_frames(const VP9_COMMON *cm, const MACROBLOCKD *const xd, VpxWriter *w) {
    const ModeInfo *const mi          = xd->mi[0];
    const int             is_compound = has_second_ref(mi);
    const int             segment_id  = 0;
    // If segment level coding of this signal is disabled...
    // or the segment allows multiple reference frame options
    if (segfeature_active(&cm->seg, segment_id, SEG_LVL_REF_FRAME)) {
        assert(!is_compound);
        assert(mi->ref_frame[0] == get_segdata(&cm->seg, segment_id, SEG_LVL_REF_FRAME));
    } else {
        // does the feature use compound prediction or not
        // (if not specified at the frame/segment level)
        if (cm->reference_mode == REFERENCE_MODE_SELECT) {
            vpx_write(w, is_compound, vp9_get_reference_mode_prob(cm, xd));
        } else {
            assert((!is_compound) == (cm->reference_mode == SINGLE_REFERENCE));
        }

        if (is_compound) {
            const int idx = cm->ref_frame_sign_bias[cm->comp_fixed_ref];
            vpx_write(w, mi->ref_frame[!idx] == cm->comp_var_ref[1], vp9_get_pred_prob_comp_ref_p(cm, xd));
        } else {
            const int bit0 = mi->ref_frame[0] != LAST_FRAME;
            vpx_write(w, bit0, vp9_get_pred_prob_single_ref_p1(cm, xd));
            if (bit0) {
                const int bit1 = mi->ref_frame[0] != GOLDEN_FRAME;
                vpx_write(w, bit1, vp9_get_pred_prob_single_ref_p2(cm, xd));
            }
        }
    }
}

static void pack_inter_mode_mvs(VP9_COMP *cpi, const MACROBLOCKD *const xd, const MbModeInfoExt *const mbmi_ext,
                                VpxWriter *w, unsigned int *const max_mv_magnitude) {
    VP9_COMMON *const                cm          = &cpi->common;
    const nmv_context               *nmvc        = &cm->fc->nmvc;
    const struct segmentation *const seg         = &cm->seg;
    const ModeInfo *const            mi          = xd->mi[0];
    const PREDICTION_MODE            mode        = mi->mode;
    const int                        segment_id  = 0;
    const BLOCK_SIZE                 bsize       = mi->sb_type;
    const int                        allow_hp    = cm->allow_high_precision_mv;
    const int                        is_inter    = is_inter_block(mi);
    const int                        is_compound = has_second_ref(mi);
    int                              skip, ref;
    skip = write_skip(cm, xd, segment_id, mi, w);

    if (!segfeature_active(seg, segment_id, SEG_LVL_REF_FRAME))
        vpx_write(w, is_inter, vp9_get_intra_inter_prob(cm, xd));

    if (bsize >= BLOCK_8X8 && cm->tx_mode == TX_MODE_SELECT && !(is_inter && skip)) {
        write_selected_tx_size(cm, xd, w);
    }

    if (!is_inter) {
        if (bsize >= BLOCK_8X8) {
            write_intra_mode(w, mode, cm->fc->y_mode_prob[eb_vp9_size_group_lookup[bsize]]);
        } else {
            int       idx, idy;
            const int num_4x4_w = eb_vp9_num_4x4_blocks_wide_lookup[bsize];
            const int num_4x4_h = eb_vp9_num_4x4_blocks_high_lookup[bsize];
            for (idy = 0; idy < 2; idy += num_4x4_h) {
                for (idx = 0; idx < 2; idx += num_4x4_w) {
                    const PREDICTION_MODE b_mode = mi->bmi[idy * 2 + idx].as_mode;
                    write_intra_mode(w, b_mode, cm->fc->y_mode_prob[0]);
                }
            }
        }
        write_intra_mode(w, mi->uv_mode, cm->fc->uv_mode_prob[mode]);
    } else {
        const int             mode_ctx    = mbmi_ext->mode_context[mi->ref_frame[0]];
        const vpx_prob *const inter_probs = cm->fc->inter_mode_probs[mode_ctx];
        write_ref_frames(cm, xd, w);

        // If segment skip is not enabled code the mode.
        if (!segfeature_active(seg, segment_id, SEG_LVL_SKIP)) {
            if (bsize >= BLOCK_8X8) {
                write_inter_mode(w, mode, inter_probs);
            }
        }
        if (bsize < BLOCK_8X8) {
            const int num_4x4_w = eb_vp9_num_4x4_blocks_wide_lookup[bsize];
            const int num_4x4_h = eb_vp9_num_4x4_blocks_high_lookup[bsize];
            int       idx, idy;
            for (idy = 0; idy < 2; idy += num_4x4_h) {
                for (idx = 0; idx < 2; idx += num_4x4_w) {
                    const int             j      = idy * 2 + idx;
                    const PREDICTION_MODE b_mode = mi->bmi[j].as_mode;
                    write_inter_mode(w, b_mode, inter_probs);
                    if (b_mode == NEWMV) {
                        for (ref = 0; ref < 1 + is_compound; ++ref)
                            eb_vp9_encode_mv(cpi,
                                             w,
                                             &mi->bmi[j].as_mv[ref].as_mv,
                                             &mbmi_ext->ref_mvs[mi->ref_frame[ref]][0].as_mv,
                                             nmvc,
                                             allow_hp,
                                             max_mv_magnitude);
                    }
                }
            }
        } else {
            if (mode == NEWMV) {
                for (ref = 0; ref < 1 + is_compound; ++ref)
                    eb_vp9_encode_mv(cpi,
                                     w,
                                     &mi->mv[ref].as_mv,
                                     &mbmi_ext->ref_mvs[mi->ref_frame[ref]][0].as_mv,
                                     nmvc,
                                     allow_hp,
                                     max_mv_magnitude);
            }
        }
    }
}

static void write_mb_modes_kf(const VP9_COMMON *cm, const MACROBLOCKD *xd, VpxWriter *w) {
    const ModeInfo *const mi       = xd->mi[0];
    const ModeInfo *const above_mi = xd->above_mi;
    const ModeInfo *const left_mi  = xd->left_mi;
    const BLOCK_SIZE      bsize    = mi->sb_type;
    write_skip(cm, xd, 0, mi, w);
    if (bsize >= BLOCK_8X8 && cm->tx_mode == TX_MODE_SELECT)
        write_selected_tx_size(cm, xd, w);

    if (bsize >= BLOCK_8X8) {
        write_intra_mode(w, mi->mode, get_y_mode_probs(mi, above_mi, left_mi, 0));
    } else {
        const int num_4x4_w = eb_vp9_num_4x4_blocks_wide_lookup[bsize];
        const int num_4x4_h = eb_vp9_num_4x4_blocks_high_lookup[bsize];
        int       idx, idy;

        for (idy = 0; idy < 2; idy += num_4x4_h) {
            for (idx = 0; idx < 2; idx += num_4x4_w) {
                const int block = idy * 2 + idx;
                write_intra_mode(w, mi->bmi[block].as_mode, get_y_mode_probs(mi, above_mi, left_mi, block));
            }
        }
    }
    write_intra_mode(w, mi->uv_mode, eb_vp9_kf_uv_mode_prob[mi->mode]);
}

void eb_vp9_write_modes_b(EntropyCodingContext *context_ptr, VP9_COMP *cpi, MACROBLOCKD *const xd,
                          const TileInfo *const tile, VpxWriter *w, TOKENEXTRA **tok, const TOKENEXTRA *const tok_end,
                          int mi_row, int mi_col, unsigned int *const max_mv_magnitude,
                          int interp_filter_selected[MAX_REF_FRAMES][SWITCHABLE]) {
    (void)mi_col;
    (void)mi_row;
    (void)tile;
    (void)interp_filter_selected;
    const VP9_COMMON *const    cm       = &cpi->common;
    const MbModeInfoExt *const mbmi_ext = context_ptr->block_ptr->mbmi_ext;
    if (frame_is_intra_only(cm)) {
        write_mb_modes_kf(cm, xd, w);
    } else {
        pack_inter_mode_mvs(cpi, xd, mbmi_ext, w, max_mv_magnitude);
    }

    assert(*tok < tok_end);
    pack_mb_tokens(w, tok, tok_end, cm->bit_depth);
}

void write_partition(const VP9_COMMON *const cm, const MACROBLOCKD *const xd, int hbs, int mi_row, int mi_col,
                     PARTITION_TYPE p, BLOCK_SIZE bsize, VpxWriter *w) {
    const int             ctx      = partition_plane_context(xd, mi_row, mi_col, bsize);
    const vpx_prob *const probs    = xd->partition_probs[ctx];
    const int             has_rows = (mi_row + hbs) < cm->mi_rows;
    const int             has_cols = (mi_col + hbs) < cm->mi_cols;

    if (has_rows && has_cols) {
        vp9_write_token(w, eb_vp9_partition_tree, probs, &partition_encodings[p]);
    } else if (!has_rows && has_cols) {
        assert(p == PARTITION_SPLIT || p == PARTITION_HORZ);
        vpx_write(w, p == PARTITION_SPLIT, probs[1]);
    } else if (has_rows && !has_cols) {
        assert(p == PARTITION_SPLIT || p == PARTITION_VERT);
        vpx_write(w, p == PARTITION_SPLIT, probs[2]);
    } else {
        assert(p == PARTITION_SPLIT);
    }
}
static void build_tree_distribution(VP9_COMP *cpi, TX_SIZE tx_size, vp9_coeff_stats *coef_branch_ct,
                                    vp9_coeff_probs_model *coef_probs) {
    vp9_coeff_count *coef_counts                                         = cpi->td.rd_counts.coef_counts[tx_size];
    unsigned int (*eob_branch_ct)[REF_TYPES][COEF_BANDS][COEFF_CONTEXTS] = cpi->common.counts.eob_branch[tx_size];
    int i, j, k, l, m;

    for (i = 0; i < PLANE_TYPES; ++i) {
        for (j = 0; j < REF_TYPES; ++j) {
            for (k = 0; k < COEF_BANDS; ++k) {
                for (l = 0; l < BAND_COEFF_CONTEXTS(k); ++l) {
                    eb_vp9_tree_probs_from_distribution(
                        eb_vp9_coef_tree, coef_branch_ct[i][j][k][l], coef_counts[i][j][k][l]);
                    coef_branch_ct[i][j][k][l][0][1] = eob_branch_ct[i][j][k][l] - coef_branch_ct[i][j][k][l][0][0];
                    for (m = 0; m < UNCONSTRAINED_NODES; ++m)
                        coef_probs[i][j][k][l][m] = get_binary_prob(coef_branch_ct[i][j][k][l][m][0],
                                                                    coef_branch_ct[i][j][k][l][m][1]);
                }
            }
        }
    }
}

static void update_coef_probs_common(VpxWriter *const bc, VP9_COMP *cpi, TX_SIZE tx_size,
                                     vp9_coeff_stats *frame_branch_ct, vp9_coeff_probs_model *new_coef_probs) {
    vp9_coeff_probs_model *old_coef_probs       = cpi->common.fc->coef_probs[tx_size];
    const vpx_prob         upd                  = DIFF_UPDATE_PROB;
    const int              entropy_nodes_update = UNCONSTRAINED_NODES;
    int                    i, j, k, l, t;
    int                    stepsize = cpi->sf.coeff_prob_appx_step;

    switch (cpi->sf.use_fast_coef_updates) {
    case TWO_LOOP: {
        /* dry run to see if there is any update at all needed */
        int savings   = 0;
        int update[2] = {0, 0};
        for (i = 0; i < PLANE_TYPES; ++i) {
            for (j = 0; j < REF_TYPES; ++j) {
                for (k = 0; k < COEF_BANDS; ++k) {
                    for (l = 0; l < BAND_COEFF_CONTEXTS(k); ++l) {
                        for (t = 0; t < entropy_nodes_update; ++t) {
                            vpx_prob       newp = new_coef_probs[i][j][k][l][t];
                            const vpx_prob oldp = old_coef_probs[i][j][k][l][t];
                            int            s;
                            int            u = 0;
                            if (t == PIVOT_NODE)
                                s = eb_vp9_prob_diff_update_savings_search_model(
                                    frame_branch_ct[i][j][k][l][0], oldp, &newp, upd, stepsize);
                            else
                                s = eb_vp9_prob_diff_update_savings_search(
                                    frame_branch_ct[i][j][k][l][t], oldp, &newp, upd);
                            if (s > 0 && newp != oldp)
                                u = 1;
                            if (u)
                                savings += s - (int)(vp9_cost_zero(upd));
                            else
                                savings -= (int)(vp9_cost_zero(upd));
                            update[u]++;
                        }
                    }
                }
            }
        }

        // SVT_LOG("Update %d %d, savings %d\n", update[0], update[1], savings);
        /* Is coef updated at all */
        if (update[1] == 0 || savings < 0) {
            vpx_write_bit(bc, 0);
            return;
        }
        vpx_write_bit(bc, 1);
        for (i = 0; i < PLANE_TYPES; ++i) {
            for (j = 0; j < REF_TYPES; ++j) {
                for (k = 0; k < COEF_BANDS; ++k) {
                    for (l = 0; l < BAND_COEFF_CONTEXTS(k); ++l) {
                        // calc probs and branch cts for this frame only
                        for (t = 0; t < entropy_nodes_update; ++t) {
                            vpx_prob       newp = new_coef_probs[i][j][k][l][t];
                            vpx_prob      *oldp = old_coef_probs[i][j][k][l] + t;
                            const vpx_prob upd  = DIFF_UPDATE_PROB;
                            int            s;
                            int            u = 0;
                            if (t == PIVOT_NODE)
                                s = eb_vp9_prob_diff_update_savings_search_model(
                                    frame_branch_ct[i][j][k][l][0], *oldp, &newp, upd, stepsize);
                            else
                                s = eb_vp9_prob_diff_update_savings_search(
                                    frame_branch_ct[i][j][k][l][t], *oldp, &newp, upd);
                            if (s > 0 && newp != *oldp)
                                u = 1;
                            vpx_write(bc, u, upd);
                            if (u) {
                                /* send/use new probability */
                                eb_vp9_write_prob_diff_update(bc, newp, *oldp);
                                *oldp = newp;
                            }
                        }
                    }
                }
            }
        }
        return;
    }

    default: {
        int updates                = 0;
        int noupdates_before_first = 0;
        assert(cpi->sf.use_fast_coef_updates == ONE_LOOP_REDUCED);
        for (i = 0; i < PLANE_TYPES; ++i) {
            for (j = 0; j < REF_TYPES; ++j) {
                for (k = 0; k < COEF_BANDS; ++k) {
                    for (l = 0; l < BAND_COEFF_CONTEXTS(k); ++l) {
                        // calc probs and branch cts for this frame only
                        for (t = 0; t < entropy_nodes_update; ++t) {
                            vpx_prob  newp = new_coef_probs[i][j][k][l][t];
                            vpx_prob *oldp = old_coef_probs[i][j][k][l] + t;
                            int       s;
                            int       u = 0;

                            if (t == PIVOT_NODE) {
                                s = eb_vp9_prob_diff_update_savings_search_model(
                                    frame_branch_ct[i][j][k][l][0], *oldp, &newp, upd, stepsize);
                            } else {
                                s = eb_vp9_prob_diff_update_savings_search(
                                    frame_branch_ct[i][j][k][l][t], *oldp, &newp, upd);
                            }

                            if (s > 0 && newp != *oldp)
                                u = 1;
                            updates += u;
                            if (u == 0 && updates == 0) {
                                noupdates_before_first++;
                                continue;
                            }
                            if (u == 1 && updates == 1) {
                                int v;
                                // first update
                                vpx_write_bit(bc, 1);
                                for (v = 0; v < noupdates_before_first; ++v) vpx_write(bc, 0, upd);
                            }
                            vpx_write(bc, u, upd);
                            if (u) {
                                /* send/use new probability */
                                eb_vp9_write_prob_diff_update(bc, newp, *oldp);
                                *oldp = newp;
                            }
                        }
                    }
                }
            }
        }
        if (updates == 0) {
            vpx_write_bit(bc, 0); // no updates
        }
        return;
    }
    }
}

static void update_coef_probs(VP9_COMP *cpi, VpxWriter *w) {
    const TX_MODE tx_mode = cpi->common.tx_mode;
    if (tx_mode >= 5) {
        return;
    }
    const TX_SIZE max_tx_size = eb_vp9_tx_mode_to_biggest_tx_size[tx_mode];
    TX_SIZE       tx_size;
    for (tx_size = TX_4X4; tx_size <= max_tx_size; ++tx_size) {
        vp9_coeff_stats       frame_branch_ct[PLANE_TYPES];
        vp9_coeff_probs_model frame_coef_probs[PLANE_TYPES];
        if (cpi->td.counts->tx.tx_totals[tx_size] <= 20 ||
            (tx_size >= TX_16X16 && cpi->sf.tx_size_search_method == USE_TX_8X8)) {
            vpx_write_bit(w, 0);
        } else {
            build_tree_distribution(cpi, tx_size, frame_branch_ct, frame_coef_probs);
            update_coef_probs_common(w, cpi, tx_size, frame_branch_ct, frame_coef_probs);
        }
    }
}

static void encode_loopfilter(struct loop_filter *lf, struct vpx_write_bit_buffer *wb) {
    int i;

    // Encode the loop filter level and type
    eb_vp9_wb_write_literal(wb, lf->filter_level, 6);
    eb_vp9_wb_write_literal(wb, lf->sharpness_level, 3);

    // Write out loop filter deltas applied at the MB level based on mode or
    // ref frame (if they are enabled).
    eb_vp9_wb_write_bit(wb, lf->mode_ref_delta_enabled);

    if (lf->mode_ref_delta_enabled) {
        eb_vp9_wb_write_bit(wb, lf->mode_ref_delta_update);
        if (lf->mode_ref_delta_update) {
            for (i = 0; i < MAX_REF_LF_DELTAS; i++) {
                const int delta   = lf->ref_deltas[i];
                const int changed = delta != lf->last_ref_deltas[i];
                eb_vp9_wb_write_bit(wb, changed);
                if (changed) {
                    lf->last_ref_deltas[i] = (signed char)delta;
                    eb_vp9_wb_write_literal(wb, abs(delta) & 0x3F, 6);
                    eb_vp9_wb_write_bit(wb, delta < 0);
                }
            }

            for (i = 0; i < MAX_MODE_LF_DELTAS; i++) {
                const int delta   = lf->mode_deltas[i];
                const int changed = delta != lf->last_mode_deltas[i];
                eb_vp9_wb_write_bit(wb, changed);
                if (changed) {
                    lf->last_mode_deltas[i] = (signed char)delta;
                    eb_vp9_wb_write_literal(wb, abs(delta) & 0x3F, 6);
                    eb_vp9_wb_write_bit(wb, delta < 0);
                }
            }
        }
    }
}

static void write_delta_q(struct vpx_write_bit_buffer *wb, int delta_q) {
    if (delta_q != 0) {
        eb_vp9_wb_write_bit(wb, 1);
        eb_vp9_wb_write_literal(wb, abs(delta_q), 4);
        eb_vp9_wb_write_bit(wb, delta_q < 0);
    } else {
        eb_vp9_wb_write_bit(wb, 0);
    }
}

static void encode_quantization(const VP9_COMMON *const cm, struct vpx_write_bit_buffer *wb) {
    eb_vp9_wb_write_literal(wb, cm->base_qindex, QINDEX_BITS);
    write_delta_q(wb, cm->y_dc_delta_q);
    write_delta_q(wb, cm->uv_dc_delta_q);
    write_delta_q(wb, cm->uv_ac_delta_q);
}

static void encode_segmentation(VP9_COMMON *cm, MACROBLOCKD *xd, struct vpx_write_bit_buffer *wb) {
    (void)xd;
    const struct segmentation *seg = &cm->seg;

    eb_vp9_wb_write_bit(wb, seg->enabled);
    if (!seg->enabled)
        return;
}

static void encode_txfm_probs(VP9_COMMON *cm, VpxWriter *w, FRAME_COUNTS *counts) {
    // Mode
    vpx_write_literal(w, VPXMIN(cm->tx_mode, ALLOW_32X32), 2);
    if (cm->tx_mode >= ALLOW_32X32)
        vpx_write_bit(w, cm->tx_mode == TX_MODE_SELECT);

    // Probabilities
    if (cm->tx_mode == TX_MODE_SELECT) {
        int          i, j;
        unsigned int ct_8x8p[TX_SIZES - 3][2];
        unsigned int ct_16x16p[TX_SIZES - 2][2];
        unsigned int ct_32x32p[TX_SIZES - 1][2];

        for (i = 0; i < TX_SIZE_CONTEXTS; i++) {
            eb_vp9_tx_counts_to_branch_counts_8x8(counts->tx.p8x8[i], ct_8x8p);
            for (j = 0; j < TX_SIZES - 3; j++)
                eb_vp9_cond_prob_diff_update(w, &cm->fc->tx_probs.p8x8[i][j], ct_8x8p[j]);
        }

        for (i = 0; i < TX_SIZE_CONTEXTS; i++) {
            eb_vp9_tx_counts_to_branch_counts_16x16(counts->tx.p16x16[i], ct_16x16p);
            for (j = 0; j < TX_SIZES - 2; j++)
                eb_vp9_cond_prob_diff_update(w, &cm->fc->tx_probs.p16x16[i][j], ct_16x16p[j]);
        }

        for (i = 0; i < TX_SIZE_CONTEXTS; i++) {
            eb_vp9_tx_counts_to_branch_counts_32x32(counts->tx.p32x32[i], ct_32x32p);
            for (j = 0; j < TX_SIZES - 1; j++)
                eb_vp9_cond_prob_diff_update(w, &cm->fc->tx_probs.p32x32[i][j], ct_32x32p[j]);
        }
    }
}

static void write_interp_filter(INTERP_FILTER filter, struct vpx_write_bit_buffer *wb) {
    const int filter_to_literal[] = {1, 0, 2, 3};

    eb_vp9_wb_write_bit(wb, filter == SWITCHABLE);
    if (filter != SWITCHABLE)
        eb_vp9_wb_write_literal(wb, filter_to_literal[filter], 2);
}
static void write_tile_info(const VP9_COMMON *const cm, struct vpx_write_bit_buffer *wb) {
    int min_log2_tile_cols, max_log2_tile_cols, ones;
    eb_vp9_get_tile_n_bits(cm->mi_cols, &min_log2_tile_cols, &max_log2_tile_cols);

    // columns
    ones = cm->log2_tile_cols - min_log2_tile_cols;
    while (ones--) eb_vp9_wb_write_bit(wb, 1);

    if (cm->log2_tile_cols < max_log2_tile_cols)
        eb_vp9_wb_write_bit(wb, 0);

    // rows
    eb_vp9_wb_write_bit(wb, cm->log2_tile_rows != 0);
    if (cm->log2_tile_rows != 0)
        eb_vp9_wb_write_bit(wb, cm->log2_tile_rows != 1);
}

static void write_render_size(const VP9_COMMON *cm, struct vpx_write_bit_buffer *wb) {
    const int scaling_active = cm->width != cm->render_width || cm->height != cm->render_height;
    eb_vp9_wb_write_bit(wb, scaling_active);
    if (scaling_active) {
        eb_vp9_wb_write_literal(wb, cm->render_width - 1, 16);
        eb_vp9_wb_write_literal(wb, cm->render_height - 1, 16);
    }
}

static void write_frame_size(const VP9_COMMON *cm, struct vpx_write_bit_buffer *wb) {
    eb_vp9_wb_write_literal(wb, cm->width - 1, 16);
    eb_vp9_wb_write_literal(wb, cm->height - 1, 16);

    write_render_size(cm, wb);
}

static void write_frame_size_with_refs(VP9_COMP *cpi, struct vpx_write_bit_buffer *wb) {
    VP9_COMMON *const cm    = &cpi->common;
    int               found = 0;

    MV_REFERENCE_FRAME ref_frame;
    for (ref_frame = LAST_FRAME; ref_frame <= ALTREF_FRAME; ++ref_frame) {
        eb_vp9_wb_write_bit(wb, found);
        if (found) {
            break;
        }
    }

    if (!found) {
        eb_vp9_wb_write_literal(wb, cm->width - 1, 16);
        eb_vp9_wb_write_literal(wb, cm->height - 1, 16);
    }

    write_render_size(cm, wb);
}

static void write_sync_code(struct vpx_write_bit_buffer *wb) {
    eb_vp9_wb_write_literal(wb, VP9_SYNC_CODE_0, 8);
    eb_vp9_wb_write_literal(wb, VP9_SYNC_CODE_1, 8);
    eb_vp9_wb_write_literal(wb, VP9_SYNC_CODE_2, 8);
}

static void write_profile(BITSTREAM_PROFILE profile, struct vpx_write_bit_buffer *wb) {
    switch (profile) {
    case PROFILE_0: eb_vp9_wb_write_literal(wb, 0, 2); break;
    case PROFILE_1: eb_vp9_wb_write_literal(wb, 2, 2); break;
    case PROFILE_2: eb_vp9_wb_write_literal(wb, 1, 2); break;
    default:
        assert(profile == PROFILE_3);
        eb_vp9_wb_write_literal(wb, 6, 3);
        break;
    }
}

static void write_bitdepth_colorspace_sampling(VP9_COMMON *const cm, struct vpx_write_bit_buffer *wb) {
    if (cm->profile >= PROFILE_2) {
        assert(cm->bit_depth > VPX_BITS_8);
        eb_vp9_wb_write_bit(wb, cm->bit_depth == VPX_BITS_10 ? 0 : 1);
    }
    eb_vp9_wb_write_literal(wb, cm->color_space, 3);
    if (cm->color_space != VPX_CS_SRGB) {
        // 0: [16, 235] (i.e. xvYCC), 1: [0, 255]
        eb_vp9_wb_write_bit(wb, cm->color_range);
        if (cm->profile == PROFILE_1 || cm->profile == PROFILE_3) {
            assert(cm->subsampling_x != 1 || cm->subsampling_y != 1);
            eb_vp9_wb_write_bit(wb, cm->subsampling_x);
            eb_vp9_wb_write_bit(wb, cm->subsampling_y);
            eb_vp9_wb_write_bit(wb, 0); // unused
        } else {
            assert(cm->subsampling_x == 1 && cm->subsampling_y == 1);
        }
    } else {
        assert(cm->profile == PROFILE_1 || cm->profile == PROFILE_3);
        eb_vp9_wb_write_bit(wb, 0); // unused
    }
}

static void write_uncompressed_header(PictureControlSet *picture_control_set_ptr, VP9_COMP *cpi,
                                      struct vpx_write_bit_buffer *wb, int show_existing_frame,
                                      int show_existing_frame_index) {
    VP9_COMMON *const cm = &cpi->common;

    eb_vp9_wb_write_literal(wb, VP9_FRAME_MARKER, 2);

    write_profile(cm->profile, wb);

    // If to use show existing frame.
    eb_vp9_wb_write_bit(wb, show_existing_frame);
    if (show_existing_frame) {
        eb_vp9_wb_write_literal(
            wb, picture_control_set_ptr->parent_pcs_ptr->show_existing_frame_index_array[show_existing_frame_index], 3);

        return;
    }

    eb_vp9_wb_write_bit(wb, cm->frame_type);
    eb_vp9_wb_write_bit(wb, cm->show_frame);
    eb_vp9_wb_write_bit(wb, cm->error_resilient_mode);

    if (cm->frame_type == KEY_FRAME) {
        write_sync_code(wb);
        write_bitdepth_colorspace_sampling(cm, wb);
        write_frame_size(cm, wb);
    } else {
        if (!cm->show_frame)
            eb_vp9_wb_write_bit(wb, cm->intra_only);

        if (!cm->error_resilient_mode)
            eb_vp9_wb_write_literal(wb, cm->reset_frame_context, 2);

        if (cm->intra_only) {
            write_sync_code(wb);

            // Note for profile 0, 420 8bpp is assumed.
            if (cm->profile > PROFILE_0) {
                write_bitdepth_colorspace_sampling(cm, wb);
            }
            eb_vp9_wb_write_literal(
                wb, picture_control_set_ptr->parent_pcs_ptr->ref_signal.refresh_frame_mask, REF_FRAMES);

            write_frame_size(cm, wb);
        } else {
            MV_REFERENCE_FRAME ref_frame;
            eb_vp9_wb_write_literal(
                wb, picture_control_set_ptr->parent_pcs_ptr->ref_signal.refresh_frame_mask, REF_FRAMES);
            for (ref_frame = LAST_FRAME; ref_frame <= ALTREF_FRAME; ++ref_frame) {
                eb_vp9_wb_write_literal(
                    wb,
                    picture_control_set_ptr->parent_pcs_ptr->ref_signal.ref_dpb_index[ref_frame - LAST_FRAME],
                    REF_FRAMES_LOG2);
                eb_vp9_wb_write_bit(wb, cm->ref_frame_sign_bias[ref_frame]);
            }

            write_frame_size_with_refs(cpi, wb);

            eb_vp9_wb_write_bit(wb, cm->allow_high_precision_mv);
            write_interp_filter(0, wb);
        }
    }

    if (!cm->error_resilient_mode) {
        eb_vp9_wb_write_bit(wb, cm->refresh_frame_context);
        eb_vp9_wb_write_bit(wb, cm->frame_parallel_decoding_mode);
    }

    eb_vp9_wb_write_literal(wb, cm->frame_context_idx, FRAME_CONTEXTS_LOG2);

    encode_loopfilter(&cm->lf, wb);
    encode_quantization(cm, wb);
    encode_segmentation(cm, (MACROBLOCKD *)NULL, wb);

    write_tile_info(cm, wb);
}

size_t write_compressed_header(VP9_COMP *cpi, uint8_t *data) {
    VP9_COMMON *const    cm = &cpi->common;
    FRAME_CONTEXT *const fc = cm->fc;

    FRAME_COUNTS *counts = cpi->td.counts;

    VpxWriter header_bc;

    eb_vp9_start_encode(&header_bc, data);
    encode_txfm_probs(cm, &header_bc, counts);

    update_coef_probs(cpi, &header_bc);
    update_skip_probs(cm, &header_bc, counts);

    if (!frame_is_intra_only(cm)) {
        int i;

        for (i = 0; i < INTER_MODE_CONTEXTS; ++i)
            prob_diff_update(
                eb_vp9_inter_mode_tree, cm->fc->inter_mode_probs[i], counts->inter_mode[i], INTER_MODES, &header_bc);
        for (i = 0; i < INTRA_INTER_CONTEXTS; i++)
            eb_vp9_cond_prob_diff_update(&header_bc, &fc->intra_inter_prob[i], counts->intra_inter[i]);

        if (cpi->allow_comp_inter_inter) {
            const int use_compound_pred = cm->reference_mode != SINGLE_REFERENCE;
            const int use_hybrid_pred   = cm->reference_mode == REFERENCE_MODE_SELECT;

            vpx_write_bit(&header_bc, use_compound_pred);
            if (use_compound_pred) {
                vpx_write_bit(&header_bc, use_hybrid_pred);
                if (use_hybrid_pred)
                    for (i = 0; i < COMP_INTER_CONTEXTS; i++)
                        eb_vp9_cond_prob_diff_update(&header_bc, &fc->comp_inter_prob[i], counts->comp_inter[i]);
            }
        }
        if (cm->reference_mode != COMPOUND_REFERENCE) {
            for (i = 0; i < REF_CONTEXTS; i++) {
                eb_vp9_cond_prob_diff_update(&header_bc, &fc->single_ref_prob[i][0], counts->single_ref[i][0]);
                eb_vp9_cond_prob_diff_update(&header_bc, &fc->single_ref_prob[i][1], counts->single_ref[i][1]);
            }
        }

        if (cm->reference_mode != SINGLE_REFERENCE)
            for (i = 0; i < REF_CONTEXTS; i++)
                eb_vp9_cond_prob_diff_update(&header_bc, &fc->comp_ref_prob[i], counts->comp_ref[i]);

        for (i = 0; i < BLOCK_SIZE_GROUPS; ++i)
            prob_diff_update(
                eb_vp9_intra_mode_tree, cm->fc->y_mode_prob[i], counts->y_mode[i], INTRA_MODES, &header_bc);

        for (i = 0; i < PARTITION_CONTEXTS; ++i)
            prob_diff_update(
                eb_vp9_partition_tree, fc->partition_prob[i], counts->partition[i], PARTITION_TYPES, &header_bc);

        eb_vp9_write_nmv_probs(cm, cm->allow_high_precision_mv, &header_bc, &counts->mv);
    }

    eb_vp9_stop_encode(&header_bc);
    assert(header_bc.pos <= 0xffff);

    return header_bc.pos;
}

void eb_vp9_pack_bitstream(PictureControlSet *picture_control_set_ptr, VP9_COMP *cpi, uint8_t *dest, size_t *size,
                           int show_existing_frame, int show_existing_frame_index) {
    uint8_t                    *data = dest;
    size_t                      first_part_size, uncompressed_hdr_size;
    struct vpx_write_bit_buffer wb = {data, 0};
    struct vpx_write_bit_buffer saved_wb;

    write_uncompressed_header(picture_control_set_ptr, cpi, &wb, show_existing_frame, show_existing_frame_index);

    // Skip the rest coding process if use show existing frame.
    if (show_existing_frame) {
        *size = eb_vp9_wb_bytes_written(&wb);
        return;
    }

    saved_wb = wb;
    eb_vp9_wb_write_literal(&wb, 0, 16); // don't know in advance first part. size

    uncompressed_hdr_size = eb_vp9_wb_bytes_written(&wb);
    data += uncompressed_hdr_size;
    first_part_size = write_compressed_header(cpi, data);
    data += first_part_size;
    // TODO(jbb): Figure out what to do if first_part_size > 16 bits.
    eb_vp9_wb_write_literal(&saved_wb, (int)first_part_size, 16);

    // Link data from EC stream to final stream.
    unsigned int ecOutputBitstreamSize = picture_control_set_ptr->entropy_coder_ptr->residual_bc.pos;

    OutputBitstreamUnit *ec_output_bitstream_ptr =
        (OutputBitstreamUnit *)picture_control_set_ptr->entropy_coder_ptr->ec_output_bitstream_ptr;

    // Copy from EC stream to frame stream
    memcpy(data, ec_output_bitstream_ptr->buffer_begin, ecOutputBitstreamSize);
    data += ecOutputBitstreamSize;

    *size = data - dest;
}
