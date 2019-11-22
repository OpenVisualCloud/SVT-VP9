/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
/***************************************
* Includes
***************************************/

#include "EbRateDistortionCost.h"
#include "vp9_rd.h"
#include "vp9_rdopt.h"
#include "vp9_scan.h"
#include "vp9_pred_common.h"

#if VP9_RD
// Note that the context is not pointing to the right CU.
EbErrorType get_partition_cost(
    PictureControlSet       *picture_control_set_ptr,
    EncDecContext           *context_ptr,
    BLOCK_SIZE               bsize,
    PARTITION_TYPE           partition_type,
    int                      partition_context,
    uint64_t                *partition_cost)
{
    EbErrorType  return_error    = EB_ErrorNone;
    VP9_COMP     *cpi             = picture_control_set_ptr->parent_pcs_ptr->cpi;
    RD_OPT *const rd              = &cpi->rd;

    if (bsize >= BLOCK_8X8) {
        *partition_cost = RDCOST(context_ptr->RDMULT, rd->RDDIV, cpi->partition_cost[partition_context][partition_type], 0);
    }
    else {
        *partition_cost = 0;
        SVT_LOG("ERROR: 4x4 Not supported\n");
    }
    return return_error;
}

/* The trailing '0' is a terminator which is used inside cost_coeffs() to
 * decide whether to include cost of a trailing EOB node or not (i.e. we
 * can skip this if the last coefficient in this transform block, e.g. the
 * 16th coefficient in a 4x4 block or the 64th coefficient in a 8x8 block,
 * were non-zero). */
static const int16_t band_counts[TX_SIZES][8] = {
  { 1, 2, 3, 4, 3, 16 - 13, 0 },
  { 1, 2, 3, 4, 11, 64 - 21, 0 },
  { 1, 2, 3, 4, 11, 256 - 21, 0 },
  { 1, 2, 3, 4, 11, 1024 - 21, 0 },
};

int coeff_rate_estimate( //cost_coeffs
    struct EncDecContext  *context_ptr,
    MACROBLOCK            *x,
    int16_t*               trans_coeff_buffer,
    uint16_t               eob,
    int                    plane,
    int                    block,
    TX_SIZE                tx_size,
    int                    pt, // coeff_ctx
    int                    use_fast_coef_costing)
{
    TX_TYPE tx_type = DCT_DCT;

    MACROBLOCKD *const xd = context_ptr->e_mbd;

    const scan_order *scan_order;
    const int16_t    *scan;
    const int16_t    *nb;

    if (tx_size == TX_4X4) {
        tx_type = get_tx_type_4x4(get_plane_type(plane), xd, block);
        scan_order = &eb_vp9_scan_orders[TX_4X4][tx_type];
    }
    else {
        if (tx_size == TX_32X32) {
            scan_order = &eb_vp9_default_scan_orders[TX_32X32];
        }
        else {
            tx_type = get_tx_type(get_plane_type(plane), xd);
            scan_order = &eb_vp9_scan_orders[tx_size][tx_type];
        }
    }
    scan = scan_order->scan;
    nb = scan_order->neighbors;

    ModeInfo *mi = xd->mi[0];
    const PLANE_TYPE type = get_plane_type(plane);
    const int16_t *band_count = &band_counts[tx_size][1];
    const tran_low_t *const qcoeff = trans_coeff_buffer;
    unsigned int(*token_costs)[2][COEFF_CONTEXTS][ENTROPY_TOKENS] =
        x->token_costs[tx_size][type][is_inter_block(mi)];
    uint8_t token_cache[32 * 32];
    int cost;
#if CONFIG_VP9_HIGHBITDEPTH
    const uint16_t *cat6_high_cost = vp9_get_high_cost_table(xd->bd);
#else
    const uint16_t *cat6_high_cost = vp9_get_high_cost_table(8);
#endif
    if (eob == 0) {
        // single eob token
        cost = token_costs[0][0][pt][EOB_TOKEN];
    }
    else {
        if (use_fast_coef_costing) {
            int band_left = *band_count++;
            int c;

            // dc token
            int v = qcoeff[0];
            int16_t prev_t;
            cost = vp9_get_token_cost(v, &prev_t, cat6_high_cost);
            cost += (*token_costs)[0][pt][prev_t];

            token_cache[0] = eb_vp9_pt_energy_class[prev_t];
            ++token_costs;

            // ac tokens
            for (c = 1; c < eob; c++) {
                const int rc = scan[c];
                int16_t t;

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
            if (band_left) cost += (*token_costs)[0][!prev_t][EOB_TOKEN];

        }
        else {  // !use_fast_coef_costing
            int band_left = *band_count++;
            int c;

            // dc token
            int v = qcoeff[0];
            int16_t tok;
            unsigned int(*tok_cost_ptr)[COEFF_CONTEXTS][ENTROPY_TOKENS];
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

// FROM vp9_mcomp.c
static INLINE int mv_cost(const MV *mv, const int *joint_cost,
    int *const comp_cost[2]) {
    assert(mv->row >= -MV_MAX && mv->row < MV_MAX);
    assert(mv->col >= -MV_MAX && mv->col < MV_MAX);
    return joint_cost[vp9_get_mv_joint(mv)] + comp_cost[0][mv->row] +
        comp_cost[1][mv->col];
}

int eb_vp9_mv_bit_cost(const MV *mv, const MV *ref, const int *mvjcost,
    int *mvcost[2], int weight) {
    const MV diff = { mv->row - ref->row, mv->col - ref->col };
    return ROUND_POWER_OF_TWO(mv_cost(&diff, mvjcost, mvcost) * weight, 7);
}
#endif

EbErrorType inter_full_cost(
    struct EncDecContext               *context_ptr,
    struct ModeDecisionCandidateBuffer *candidate_buffer_ptr,
    int                                *y_distortion,
    int                                *cb_distortion,
    int                                *cr_distortion,
    int                                *y_coeff_bits,
    int                                *cb_coeff_bits,
    int                                *cr_coeff_bits,
    PictureControlSet                  *picture_control_set_ptr){
    EbErrorType return_error = EB_ErrorNone;
    VP9_COMP   *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
    MACROBLOCKD *const xd = context_ptr->e_mbd;
    RD_OPT *const rd = &cpi->rd;
    int rate = 0;
    int distortion = 0;

    if (candidate_buffer_ptr->candidate_ptr->mode_info->skip == EB_TRUE) {
        rate = (int)candidate_buffer_ptr->candidate_ptr->fast_luma_rate + (int)candidate_buffer_ptr->candidate_ptr->fast_chroma_rate + vp9_cost_bit(vp9_get_skip_prob(&cpi->common, xd), 1);
        distortion = y_distortion[DIST_CALC_PREDICTION] + cb_distortion[DIST_CALC_PREDICTION] + cr_distortion[DIST_CALC_PREDICTION];
    }
    else {
        rate = *y_coeff_bits + *cb_coeff_bits + *cr_coeff_bits + (int)candidate_buffer_ptr->candidate_ptr->fast_luma_rate + (int)candidate_buffer_ptr->candidate_ptr->fast_chroma_rate + vp9_cost_bit(vp9_get_skip_prob(&cpi->common, xd), 0);
        distortion = y_distortion[DIST_CALC_RESIDUAL] + cb_distortion[DIST_CALC_RESIDUAL] + cr_distortion[DIST_CALC_RESIDUAL];
    }

    // Hsan: the partition cost of do not split should be added here for only the last depth (i.e. rate += cpi->partition_cost[context_ptr->block_ptr->partition_context][PARTITION_NONE]) if current block does not have child)
    // If not last depth, then inter depth decision is taking care of adding the partition cost

    *candidate_buffer_ptr->full_cost_ptr = RDCOST(context_ptr->RDMULT, rd->RDDIV, rate, distortion);
    return return_error;
}

EbErrorType intra_full_cost(
    struct EncDecContext               *context_ptr,
    struct ModeDecisionCandidateBuffer *candidate_buffer_ptr,
    int                                *y_distortion,
    int                                *cb_distortion,
    int                                *cr_distortion,
    int                                *y_coeff_bits,
    int                                *cb_coeff_bits,
    int                                *cr_coeff_bits,
    PictureControlSet                  *picture_control_set_ptr){
    EbErrorType return_error = EB_ErrorNone;
    VP9_COMP   *cpi           = picture_control_set_ptr->parent_pcs_ptr->cpi;
    MACROBLOCKD *const xd     = context_ptr->e_mbd;
    RD_OPT *const rd          = &cpi->rd;
    int rate                  = 0;
    int distortion            = 0;

    if (candidate_buffer_ptr->candidate_ptr->mode_info->skip == EB_TRUE) {
        rate = (int)candidate_buffer_ptr->candidate_ptr->fast_luma_rate + (int)candidate_buffer_ptr->candidate_ptr->fast_chroma_rate + vp9_cost_bit(vp9_get_skip_prob(&cpi->common, xd), 1);
        distortion = y_distortion[DIST_CALC_PREDICTION] + cb_distortion[DIST_CALC_PREDICTION] + cr_distortion[DIST_CALC_PREDICTION];
    }
    else {
        rate = *y_coeff_bits + *cb_coeff_bits + *cr_coeff_bits + (int)candidate_buffer_ptr->candidate_ptr->fast_luma_rate + (int)candidate_buffer_ptr->candidate_ptr->fast_chroma_rate + vp9_cost_bit(vp9_get_skip_prob(&cpi->common, xd), 0);
        distortion = y_distortion[DIST_CALC_RESIDUAL] + cb_distortion[DIST_CALC_RESIDUAL] + cr_distortion[DIST_CALC_RESIDUAL];
    }

    // Hsan: the partition cost of do not split should be added here for only the last depth (i.e. rate += cpi->partition_cost[context_ptr->block_ptr->partition_context][PARTITION_NONE]) if current block does not have child)
    // If not last depth, then inter depth decision is taking care of adding the partition cost

    *candidate_buffer_ptr->full_cost_ptr = RDCOST(context_ptr->RDMULT, rd->RDDIV, rate, distortion);

    return return_error;
}

int64_t inter_fast_cost(
    PictureControlSet      *picture_control_set_ptr,
    int                     has_uv,
    int                     rd_mult_sad,
    MbModeInfoExt          *mbmi_ext,
    MACROBLOCKD            *e_mbd,
    unsigned int           *ref_costs_single,
    unsigned int           *ref_costs_comp,
    vpx_prob                comp_mode_p,
    ModeDecisionCandidate  *candidate_ptr,
    uint64_t                luma_distortion,
    uint64_t                chroma_distortion)
{
    UNUSED(e_mbd);
    UNUSED(has_uv);
    VP9_COMP   *cpi                     = picture_control_set_ptr->parent_pcs_ptr->cpi;
    MACROBLOCK *const x                 = &cpi->td.mb;
    RD_OPT *const rd                    = &cpi->rd;
    MV_REFERENCE_FRAME ref_frame        = candidate_ptr->mode_info->ref_frame[0];
    MV_REFERENCE_FRAME second_ref_frame = candidate_ptr->mode_info->ref_frame[1];
    const int this_mode                 = candidate_ptr->mode_info->mode;
    int refs[2]                         = { ref_frame, (second_ref_frame < 0) ? 0 : second_ref_frame };

    int this_rate;
    int64_t this_distortion;
    int64_t this_rd;

    int rate_ref = 0;
    int rate_compmode = 0;
    int rate_mv = 0;
    int rate_inter_mode = 0;

    // Estimate the reference frame signaling cost

    int comp_pred = second_ref_frame > INTRA_FRAME;
    if (comp_pred) {
        rate_ref = ref_costs_comp[ref_frame];
    }
    else {
        rate_ref = ref_costs_single[ref_frame];
    }

    rate_compmode = (cpi->common.reference_mode == REFERENCE_MODE_SELECT) ? vp9_cost_bit(comp_mode_p, comp_pred) : 0;

    if (this_mode == NEWMV) {

        if (comp_pred) {
            rate_mv = eb_vp9_mv_bit_cost(&candidate_ptr->mode_info->mv[0].as_mv,
                &mbmi_ext->ref_mvs[refs[0]][0].as_mv,
                x->nmvjointcost,
                x->mvcost,
                MV_COST_WEIGHT);

            rate_mv += eb_vp9_mv_bit_cost(&candidate_ptr->mode_info->mv[1].as_mv,
                &mbmi_ext->ref_mvs[refs[1]][0].as_mv,
                x->nmvjointcost,
                x->mvcost,
                MV_COST_WEIGHT);

        }
        else {
            rate_mv = eb_vp9_mv_bit_cost(&candidate_ptr->mode_info->mv[0].as_mv,
                &mbmi_ext->ref_mvs[refs[0]][0].as_mv,
                x->nmvjointcost,
                x->mvcost,
                MV_COST_WEIGHT);
#if INTER_INTRA_BIAS
            // Estimate the rate implications of a new mv but discount this
            // under certain circumstances where we want to help initiate a weak
            // motion field, where the distortion gain for a single block may not
            // be enough to overcome the cost of a new mv.
            //ref_mvs[ref_frame_0][1].as_mv.col
            int discount_newmv = (/*!cpi->rc.is_src_frame_alt_ref && */(this_mode == NEWMV) &&
                (candidate_ptr->mode_info->mv[0].as_int != 0) &&
                    ((mbmi_ext->ref_mvs[refs[0]][0].as_int == 0) ||
                    (mbmi_ext->ref_mvs[refs[0]][0].as_int == INVALID_MV)) &&
                        ((mbmi_ext->ref_mvs[refs[0]][1].as_int == 0) ||
                    (mbmi_ext->ref_mvs[refs[0]][1].as_int == INVALID_MV)));

            if (discount_newmv /*discount_newmv_test(cpi, this_mode, candidate_ptr->mode_info->mv[0], mode_mv, refs[0])*/) {
                rate_mv = VPXMAX((rate_mv / NEW_MV_DISCOUNT_FACTOR), 1);
            }
#endif
        }
    }

#if INTER_INTRA_BIAS
    // We don't include the cost of the second reference here, because there
// are only two options: Last/ARF or Golden/ARF; The second one is always
// known, which is ARF.
//
// Under some circumstances we discount the cost of new mv mode to encourage
// initiation of a motion field.
    int discount_newmv = (/*!cpi->rc.is_src_frame_alt_ref && */(this_mode == NEWMV) &&
        (candidate_ptr->mode_info->mv[0].as_int != 0) &&
        ((mbmi_ext->ref_mvs[refs[0]][0].as_int == 0) ||
        (mbmi_ext->ref_mvs[refs[0]][0].as_int == INVALID_MV)) &&
            ((mbmi_ext->ref_mvs[refs[0]][1].as_int == 0) ||
        (mbmi_ext->ref_mvs[refs[0]][1].as_int == INVALID_MV)));

    if (discount_newmv) {
        rate_inter_mode  =
            VPXMIN(cost_mv_ref(cpi, (PREDICTION_MODE)this_mode, mbmi_ext->mode_context[refs[0]]),
                cost_mv_ref(cpi, (PREDICTION_MODE)NEARESTMV, mbmi_ext->mode_context[refs[0]]));
    }
    else {
        rate_inter_mode = cost_mv_ref(cpi, (PREDICTION_MODE)this_mode, mbmi_ext->mode_context[refs[0]]);
    }
#else

    rate_inter_mode = cost_mv_ref(cpi, (PREDICTION_MODE)this_mode, mbmi_ext->mode_context[refs[0]]);
#endif

    this_distortion = (int64_t)(luma_distortion + chroma_distortion);

    this_rate = rate_ref + rate_compmode + rate_mv + rate_inter_mode;
    this_rd = RDCOST(rd_mult_sad, rd->RDDIV, this_rate, this_distortion);

    // Keep the Fast Luma and Chroma rate for future use
    candidate_ptr->fast_luma_rate = this_rate;
    candidate_ptr->fast_chroma_rate = 0;

    return this_rd;

}

int64_t intra_fast_cost(
     PictureControlSet     *picture_control_set_ptr,
    int                     has_uv,
    int                     rd_mult_sad,
    MbModeInfoExt          *mbmi_ext,
    MACROBLOCKD            *e_mbd,
    unsigned int           *ref_costs_single,
    unsigned int           *ref_costs_comp,
    vpx_prob                comp_mode_p,
    ModeDecisionCandidate  *candidate_ptr,
    uint64_t                luma_distortion,
    uint64_t                chroma_distortion)
{
    UNUSED(comp_mode_p);
    UNUSED(mbmi_ext);
    VP9_COMP   *cpi      = picture_control_set_ptr->parent_pcs_ptr->cpi;
    RD_OPT *const rd     = &cpi->rd;
    PREDICTION_MODE mode = candidate_ptr->mode_info->mode;
    PREDICTION_MODE uv_mode = candidate_ptr->mode_info->uv_mode;
    int this_rate;
    int64_t this_distortion;
    int64_t this_rd;
    int rate_mode;

    int rate_uv_mode = has_uv ?
        cpi->intra_uv_mode_cost[cpi->common.frame_type][mode][uv_mode] :
        0;

    int rate_ref = 0;

    if (cpi->common.frame_type == KEY_FRAME) {

        ModeInfo *const mic                 = e_mbd->mi[0];
        const ModeInfo *above_mi            = e_mbd->above_mi;
        const ModeInfo *left_mi             = e_mbd->left_mi;
        const PREDICTION_MODE A             = eb_vp9_above_block_mode(mic, above_mi, 0);
        const PREDICTION_MODE L             = eb_vp9_left_block_mode(mic, left_mi, 0);
        int * bmode_costs                   = cpi->y_mode_costs[A][L];
        rate_mode                           = bmode_costs[mode];
    }
    else {
        rate_mode                           = cpi->mbmode_cost[mode];
        // Estimate the reference frame signaling cost
        MV_REFERENCE_FRAME ref_frame        = candidate_ptr->mode_info->ref_frame[0];
        MV_REFERENCE_FRAME second_ref_frame = candidate_ptr->mode_info->ref_frame[1];
        int comp_pred                       = second_ref_frame > INTRA_FRAME;
        if (comp_pred) {
            rate_ref                        = ref_costs_comp[ref_frame];
        }
        else {
            rate_ref                        = ref_costs_single[ref_frame];
        }
    }
    this_distortion                 = (int64_t)(luma_distortion + chroma_distortion);
    this_rate                       = rate_mode + rate_ref + rate_uv_mode;
    this_rd                         = RDCOST(rd_mult_sad, rd->RDDIV, this_rate, this_distortion);

    // Keep the Fast Luma and Chroma rate for future use
    candidate_ptr->fast_luma_rate   = rate_mode + rate_ref;
    candidate_ptr->fast_chroma_rate = rate_uv_mode;

    return this_rd;
}
