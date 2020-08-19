/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <math.h>

#include "EbEncDecTasks.h"
#include "EbEncDecResults.h"
#include "EbPictureDemuxResults.h"
#include "EbEncDecProcess.h"
#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbRateDistortionCost.h"
#include "EbPictureOperators.h"
#include "EbSvtVp9ErrorCodes.h"
#include "EbComputeSAD.h"
#include "EbPictureControlSet.h"
#include "EbCodingUnit.h"
#include "EbModeDecision.h"
#include "EbMcp.h"

#include "vp9_rd.h"
#include "vp9_picklpf.h"
#include "vpx_dsp_rtcd.h"
#include "vp9_scan.h"
#include "vp9_idct.h"
#include "vp9_rtcd.h"
#include "vp9_scale.h"
#include "vp9_rdopt.h"
#include "vp9_pred_common.h"

// compute the cost of curr depth, and the depth above
void eb_vp9_compute_depth_costs(
    PictureControlSet *picture_control_set_ptr,
    SbUnit            *sb_ptr,
    EncDecContext     *context_ptr,
    uint32_t           current_depth_idx_mds,
    uint32_t           parent_depth_idx_mds,
    BLOCK_SIZE         parent_depth_bsize,
    uint32_t           step,
    uint64_t          *parent_depth_cost,
    uint64_t          *current_depth_cost)
{
    uint64_t       parent_depth_part_cost = 0;
    uint64_t       current_depth_part_cost = 0;

    if (picture_control_set_ptr->slice_type == I_SLICE && parent_depth_bsize == BLOCK_64X64) {

        *parent_depth_cost = MAX_BLOCK_COST;

        *current_depth_cost =
            context_ptr->enc_dec_local_block_array[current_depth_idx_mds]->cost +
            context_ptr->enc_dec_local_block_array[current_depth_idx_mds - 1 * step]->cost +
            context_ptr->enc_dec_local_block_array[current_depth_idx_mds - 2 * step]->cost +
            context_ptr->enc_dec_local_block_array[current_depth_idx_mds - 3 * step]->cost;
    }
    else {
#if VP9_RD
        get_partition_cost(
            picture_control_set_ptr,
            context_ptr,
            parent_depth_bsize,
            PARTITION_NONE,
            sb_ptr->coded_block_array_ptr[parent_depth_idx_mds]->partition_context,
            &parent_depth_part_cost);
#endif
        *parent_depth_cost = (context_ptr->enc_dec_local_block_array[parent_depth_idx_mds]->cost == MAX_BLOCK_COST) ?
            MAX_BLOCK_COST :
            context_ptr->enc_dec_local_block_array[parent_depth_idx_mds]->cost + parent_depth_part_cost;

#if VP9_RD

        get_partition_cost(
            picture_control_set_ptr,
            context_ptr,
            parent_depth_bsize,
            PARTITION_SPLIT,
            sb_ptr->coded_block_array_ptr[parent_depth_idx_mds]->partition_context,
            &current_depth_part_cost);
#endif

        *current_depth_cost =
            context_ptr->enc_dec_local_block_array[current_depth_idx_mds]->cost +
            context_ptr->enc_dec_local_block_array[current_depth_idx_mds - 1 * step]->cost +
            context_ptr->enc_dec_local_block_array[current_depth_idx_mds - 2 * step]->cost +
            context_ptr->enc_dec_local_block_array[current_depth_idx_mds - 3 * step]->cost +
            current_depth_part_cost;
    }
}

uint32_t inter_depth_decision(
    PictureControlSet *picture_control_set_ptr,
    SbUnit            *sb_ptr,
    EncDecContext     *context_ptr,
    uint32_t           blk_mds)
{
    uint32_t            last_block_index;
    uint64_t            parent_depth_cost = 0, current_depth_cost = 0;

    EB_BOOL             last_depth_flag;
    const EpBlockStats *ep_block_stats_ptr;
    BLOCK_SIZE          parent_depth_bsize;

    last_depth_flag = sb_ptr->coded_block_array_ptr[blk_mds]->split_flag == EB_FALSE ? EB_TRUE : EB_FALSE;

    last_block_index = blk_mds;
    ep_block_stats_ptr = ep_get_block_stats(blk_mds);

    uint32_t parent_depth_idx_mds = blk_mds;
    uint32_t current_depth_idx_mds = blk_mds;

    if (last_depth_flag) {
        while (ep_block_stats_ptr->is_last_quadrant) { // is_last_quadrant to move to geom

            //get parent idx
            parent_depth_idx_mds = current_depth_idx_mds - parent_depth_offset[ep_block_stats_ptr->depth]; // parent_depth_offset to move to geom

            parent_depth_bsize = ep_get_block_stats(parent_depth_idx_mds)->bsize;

            eb_vp9_compute_depth_costs(
                picture_control_set_ptr,
                sb_ptr,
                context_ptr,
                current_depth_idx_mds,
                parent_depth_idx_mds,
                parent_depth_bsize,
                sq_depth_offset[ep_block_stats_ptr->depth],
                &parent_depth_cost,
                &current_depth_cost);

            if (parent_depth_cost <= current_depth_cost) {
                sb_ptr->coded_block_array_ptr[parent_depth_idx_mds]->split_flag = EB_FALSE;
                context_ptr->enc_dec_local_block_array[parent_depth_idx_mds]->cost = parent_depth_cost;
                last_block_index = parent_depth_idx_mds;
            }
            else {
                context_ptr->enc_dec_local_block_array[parent_depth_idx_mds]->cost = current_depth_cost;
            }

            //setup next parent inter depth
            ep_block_stats_ptr = ep_get_block_stats(parent_depth_idx_mds);
            current_depth_idx_mds = parent_depth_idx_mds;
        }
    }

    return last_block_index;
}

const EbPredictionFunc prediction_fun_table[2] =
{
    inter_prediction,
    intra_prediction
};

EbFastCostFunc fast_cost_func_table[2] =
{
    inter_fast_cost,
    intra_fast_cost
};
const EbFullCostFunc full_cost_func_table[2] =
{
    inter_full_cost,
    intra_full_cost
};

void set_nfl(
    PictureControlSet *picture_control_set_ptr,
    EncDecContext     *context_ptr,
    SbUnit            *sb_ptr)
{
    // NFL Level MD         Settings
    // 0                    11
    // 1                    4
    // 2                    3
    // 3                    2
    // 4                    1
    if (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_SB_SWITCH_DEPTH_MODE && picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_ptr->sb_index] == SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE) {
        context_ptr->full_recon_search_count = 1;
    }
    else {
        if (context_ptr->nfl_level == 0) {
            context_ptr->full_recon_search_count = 11;
        }
        else if (context_ptr->nfl_level == 1) {
            context_ptr->full_recon_search_count = 4;
        }
        else if (context_ptr->nfl_level == 2) {
            context_ptr->full_recon_search_count = 3;
        }
        else if (context_ptr->nfl_level == 3) {
            context_ptr->full_recon_search_count = 2;
        }
        else {
            context_ptr->full_recon_search_count = 1;
        }
    }
}

/*******************************************
* Coding Loop - Fast Loop Initialization
*******************************************/
void coding_loop_init_fast_loop(
    EncDecContext *context_ptr)
{
    // Initialize the candidate buffer costs
    uint32_t buffer_depth_index_start = context_ptr->buffer_depth_index_start[context_ptr->ep_block_stats_ptr->depth];
    uint32_t buffer_depth_index_width = context_ptr->buffer_depth_index_width[context_ptr->ep_block_stats_ptr->depth];
    uint32_t index = 0;

    for (index = 0; index < buffer_depth_index_width; ++index) {
        context_ptr->fast_cost_array[buffer_depth_index_start + index] = MAX_BLOCK_COST;
        context_ptr->full_cost_array[buffer_depth_index_start + index] = MAX_BLOCK_COST;
    }

    return;
}

void perform_fast_loop(
    PictureControlSet            *picture_control_set_ptr,
    EncDecContext                *context_ptr,
    SbUnit                       *sb_ptr,
    ModeDecisionCandidateBuffer **candidate_buffer_ptr_array_base,
    ModeDecisionCandidate        *fast_candidate_array,
    uint32_t                      fast_candidate_total_count,
    EbPictureBufferDesc          *input_picture_ptr,
    uint32_t                      max_buffers,
    uint32_t                     *second_fast_candidate_total_count)
{

    int32_t                      fast_loop_candidate_index;
    uint64_t                     luma_fast_distortion;
    uint64_t                     chroma_fast_distortion;
    ModeDecisionCandidateBuffer *candidate_buffer;
    ModeDecisionCandidate       *candidate_ptr;

    uint32_t                     highest_cost_index;
    uint64_t                     highest_cost;

    // Initialize first fast cost loop variables
    uint64_t first_best_candidate_cost = MAX_BLOCK_COST;
    int32_t first_best_candidate_index = INVALID_FAST_CANDIDATE_INDEX;
    (void)sb_ptr;
    if (context_ptr->single_fast_loop_flag == EB_FALSE) {

        // First Fast-Cost Search Candidate Loop
        fast_loop_candidate_index = fast_candidate_total_count - 1;
        do
        {
            luma_fast_distortion = 0;

            // Set the Candidate Buffer
            candidate_buffer = candidate_buffer_ptr_array_base[0];
            ModeDecisionCandidate *candidate_ptr = candidate_buffer->candidate_ptr = &fast_candidate_array[fast_loop_candidate_index];

            // Set the WebM mi
            context_ptr->e_mbd->mi[0] = candidate_ptr->mode_info;

            // Perform src to src if distortion ready or INTRA and open loop intra enabled
            if (candidate_ptr->distortion_ready)
            {
                luma_fast_distortion = candidate_ptr->me_distortion;

                // Fast Cost Calc
                *candidate_buffer->fast_cost_ptr = fast_cost_func_table[candidate_ptr->mode_info->mode <= TM_PRED](
                    picture_control_set_ptr,
                    context_ptr->ep_block_stats_ptr->has_uv,
                    context_ptr->rd_mult_sad,
                    context_ptr->block_ptr->mbmi_ext,
                    context_ptr->e_mbd,
                    context_ptr->ref_costs_single,
                    context_ptr->ref_costs_comp,
                    context_ptr->comp_mode_p,
                    candidate_buffer->candidate_ptr,
                    luma_fast_distortion,
                    0);

                // Keep track of the candidate index of the best  (src - src) candidate
                if (*(candidate_buffer->fast_cost_ptr) <= first_best_candidate_cost) {
                    first_best_candidate_index = fast_loop_candidate_index;
                    first_best_candidate_cost = *(candidate_buffer->fast_cost_ptr);
                }

                // Initialize Fast Cost - to do not interact with the second Fast-Cost Search
                *(candidate_buffer->fast_cost_ptr) = MAX_BLOCK_COST;
            }

        } while (--fast_loop_candidate_index >= 0);
    }

    // Second Fast-Cost Search Candidate Loop
    *second_fast_candidate_total_count = 0;
    highest_cost_index = context_ptr->buffer_depth_index_start[context_ptr->ep_block_stats_ptr->depth];
    fast_loop_candidate_index = fast_candidate_total_count - 1;

    do
    {
        candidate_buffer = candidate_buffer_ptr_array_base[highest_cost_index];
        candidate_ptr = candidate_buffer->candidate_ptr = &fast_candidate_array[fast_loop_candidate_index];

        // Set the WebM mi
        context_ptr->e_mbd->mi[0] = candidate_ptr->mode_info;

        EbPictureBufferDesc *prediction_ptr = candidate_buffer->prediction_ptr;

        if (!candidate_ptr->distortion_ready || fast_loop_candidate_index == first_best_candidate_index || context_ptr->single_fast_loop_flag) {

            luma_fast_distortion = 0;
            chroma_fast_distortion = 0;

            // Prediction
            for (int plane = 0; plane < (int)(((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) ? MAX_MB_PLANE : 1); ++plane) {

                EbByte pred_buffer = (plane == 0) ?
                    (prediction_ptr->buffer_y + context_ptr->block_origin_index) :
                    (plane == 1) ?
                    prediction_ptr->buffer_cb + context_ptr->block_chroma_origin_index :
                    prediction_ptr->buffer_cr + context_ptr->block_chroma_origin_index;

                uint16_t pred_stride = (plane == 0) ?
                    prediction_ptr->stride_y :
                    (plane == 1) ?
                    prediction_ptr->stride_cb :
                    prediction_ptr->stride_cr;

                prediction_fun_table[candidate_ptr->mode_info->mode <= TM_PRED](
                    context_ptr,
                    pred_buffer,
                    pred_stride,
                    plane);
            }

            // Luma
            luma_fast_distortion = (n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][context_ptr->ep_block_stats_ptr->sq_size >> 3](
                input_picture_ptr->buffer_y + context_ptr->input_origin_index,
                input_picture_ptr->stride_y,
                prediction_ptr->buffer_y + context_ptr->block_origin_index,
                MAX_SB_SIZE,
                context_ptr->ep_block_stats_ptr->sq_size,
                context_ptr->ep_block_stats_ptr->sq_size));

            // Chroma
            if ((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv)
            {
                chroma_fast_distortion += n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][context_ptr->ep_block_stats_ptr->sq_size_uv >> 3](
                    input_picture_ptr->buffer_cb + context_ptr->input_chroma_origin_index,
                    input_picture_ptr->stride_cb,
                    candidate_buffer->prediction_ptr->buffer_cb + context_ptr->block_chroma_origin_index,
                    MAX_SB_SIZE >> 1,
                    context_ptr->ep_block_stats_ptr->sq_size_uv,
                    context_ptr->ep_block_stats_ptr->sq_size_uv);

                chroma_fast_distortion += n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][context_ptr->ep_block_stats_ptr->sq_size_uv >> 3](
                    input_picture_ptr->buffer_cr + context_ptr->input_chroma_origin_index,
                    input_picture_ptr->stride_cr,
                    candidate_buffer->prediction_ptr->buffer_cr + context_ptr->block_chroma_origin_index,
                    MAX_SB_SIZE >> 1,
                    context_ptr->ep_block_stats_ptr->sq_size_uv,
                    context_ptr->ep_block_stats_ptr->sq_size_uv);
            }

            *candidate_buffer->fast_cost_ptr = fast_cost_func_table[candidate_ptr->mode_info->mode <= TM_PRED](
                picture_control_set_ptr,
                context_ptr->ep_block_stats_ptr->has_uv,
                context_ptr->rd_mult_sad,
                context_ptr->block_ptr->mbmi_ext,
                context_ptr->e_mbd,
                context_ptr->ref_costs_single,
                context_ptr->ref_costs_comp,
                context_ptr->comp_mode_p,
                candidate_buffer->candidate_ptr,
                luma_fast_distortion,
                chroma_fast_distortion);

            (*second_fast_candidate_total_count)++;
        }

        // Find the buffer with the highest cost
        if (fast_loop_candidate_index)
        {
            // max_cost is volatile to prevent the compiler from loading 0xFFFFFFFFFFFFFF
            //   as a const at the early-out. Loading a large constant on intel x64 processors
            //   clogs the i-cache/intstruction decode. This still reloads the variable from
            //   the stack each pass, so a better solution would be to register the variable,
            //   but this might require asm.

            volatile uint64_t max_cost = ~0ull;
            uint64_t *fast_cost_array = context_ptr->fast_cost_array;
            uint32_t buffer_index_start = context_ptr->buffer_depth_index_start[context_ptr->ep_block_stats_ptr->depth];
            uint32_t buffer_index_end = buffer_index_start + max_buffers;

            uint32_t buffer_index;

            highest_cost_index = buffer_index_start;
            buffer_index = buffer_index_start + 1;

            do {

                highest_cost = fast_cost_array[highest_cost_index];

                if (highest_cost == max_cost) {
                    break;
                }

                if (fast_cost_array[buffer_index] > highest_cost)
                {
                    highest_cost_index = buffer_index;
                }

            } while (++buffer_index < buffer_index_end);

        }
    } while (--fast_loop_candidate_index >= 0);
}

void perform_coding_loop(
    EncDecContext *context_ptr,
    int16_t*       residual_quant_coeff_buffer,
    const int      residual_quant_coeff_stride,
    EbByte         input_buffer,
    uint16_t       input_stride,
    EbByte         pred_buffer,
    uint16_t       pred_stride,
    int16_t*       trans_coeff_buffer,
    int16_t*       recon_coeff_buffer,
    EbByte         recon_buffer,
    uint16_t       recon_stride,
    const int16_t *zbin_ptr,
    const int16_t *round_ptr,
    const int16_t *quant_ptr,
    const int16_t *quant_shift_ptr,
    int16_t       *dequant_ptr,
    uint16_t      *eob,
    TX_SIZE        tx_size,
    int            plane,
    EB_BOOL        is_encode_pass,
    EB_BOOL        do_recon)
{
    TX_TYPE tx_type = DCT_DCT;

    MACROBLOCKD *const xd = context_ptr->e_mbd;
    int block = context_ptr->bmi_index;

    const scan_order *scan_order;
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

    switch (tx_size) {
    case TX_32X32:
        if (!(is_encode_pass && context_ptr->skip_eob_zero_mode_ep &&  *eob == 0)) {
            eb_vp9_residual_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][32 >> 3](
                input_buffer,
                input_stride,
                pred_buffer,
                pred_stride,
                residual_quant_coeff_buffer,
                residual_quant_coeff_stride,
                32,
                32);

            if (!is_encode_pass && context_ptr->pf_md_level) {
                memset(trans_coeff_buffer, 0, 1024 * sizeof(int16_t));
                vpx_partial_fdct32x32(
                    residual_quant_coeff_buffer,
                    trans_coeff_buffer,
                    residual_quant_coeff_stride);
            }
            else {
                eb_vp9_fdct32x32(
                    residual_quant_coeff_buffer,
                    trans_coeff_buffer,
                    residual_quant_coeff_stride);
            }

            eb_vp9_quantize_b_32x32(
                trans_coeff_buffer,
                1024,
                0,
                zbin_ptr,
                round_ptr,
                quant_ptr,
                quant_shift_ptr,
                residual_quant_coeff_buffer,
                recon_coeff_buffer,
                dequant_ptr,
                eob,
                scan_order->scan,
                scan_order->iscan);

#if 0 // Hsan is it similar to RDOQ: to evaluate
            if (args->enable_coeff_opt) {
                *a = *l = vp9_optimize_b(x, plane, block, tx_size, entropy_ctx) > 0;
            }
#endif
        }
        if (context_ptr->spatial_sse_full_loop || (is_encode_pass && do_recon)) {
            // Hsan: both pred and rec samples are needed @ MD and EP to perform the eob zero mode decision
            pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][4](
                pred_buffer,
                pred_stride,
                recon_buffer,
                recon_stride,
                32,
                32);

            if (*eob) {
                eb_vp9_idct32x32_add(
                    recon_coeff_buffer,
                    recon_buffer,
                    recon_stride,
                    *eob);
            }
        }
        break;

    case TX_16X16:
        if (!(is_encode_pass && context_ptr->skip_eob_zero_mode_ep &&  *eob == 0)) {
            eb_vp9_residual_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][16 >> 3](
                input_buffer,
                input_stride,
                pred_buffer,
                pred_stride,
                residual_quant_coeff_buffer,
                residual_quant_coeff_stride,
                16,
                16);

            vp9_fht16x16(
                residual_quant_coeff_buffer,
                trans_coeff_buffer,
                residual_quant_coeff_stride,
                tx_type);

            eb_vp9_quantize_b(
                trans_coeff_buffer,
                256,
                0,
                zbin_ptr,
                round_ptr,
                quant_ptr,
                quant_shift_ptr,
                residual_quant_coeff_buffer,
                recon_coeff_buffer,
                dequant_ptr,
                eob,
                scan_order->scan,
                scan_order->iscan);

#if 0 // Hsan is it similar to RDOQ: to evaluate
            if (args->enable_coeff_opt) {
                *a = *l = vp9_optimize_b(x, plane, block, tx_size, entropy_ctx) > 0;
            }
#endif
        }
        if (context_ptr->spatial_sse_full_loop || (is_encode_pass && do_recon)) {

            // Hsan: both pred and rec samples are needed @ MD and EP to perform the eob zero mode decision
            pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][2](
                pred_buffer,
                pred_stride,
                recon_buffer,
                recon_stride,
                16,
                16);

            if (*eob) {
                eb_vp9_iht16x16_add(
                    tx_type,
                    recon_coeff_buffer,
                    recon_buffer,
                    recon_stride,
                    *eob);
            }
        }

        break;

    case TX_8X8:
        if (!(is_encode_pass && context_ptr->skip_eob_zero_mode_ep &&  *eob == 0)) {
            eb_vp9_residual_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][8 >> 3](
                input_buffer,
                input_stride,
                pred_buffer,
                pred_stride,
                residual_quant_coeff_buffer,
                residual_quant_coeff_stride,
                8,
                8);

            vp9_fht8x8(
                residual_quant_coeff_buffer,
                trans_coeff_buffer,
                residual_quant_coeff_stride,
                tx_type);

            eb_vp9_quantize_b(
                trans_coeff_buffer,
                64,
                0,
                zbin_ptr,
                round_ptr,
                quant_ptr,
                quant_shift_ptr,
                residual_quant_coeff_buffer,
                recon_coeff_buffer,
                dequant_ptr,
                eob,
                scan_order->scan,
                scan_order->iscan);

#if 0 // Hsan is it similar to RDOQ: to evaluate
            if (args->enable_coeff_opt) {
                *a = *l = vp9_optimize_b(x, plane, block, tx_size, entropy_ctx) > 0;
            }
#endif
        }
        if (context_ptr->spatial_sse_full_loop || (is_encode_pass && do_recon)) {

            // Hsan: both pred and rec samples are needed @ MD and EP to perform the eob zero mode decision
            pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][1](
                pred_buffer,
                pred_stride,
                recon_buffer,
                recon_stride,
                8,
                8);

            if (*eob) {
                eb_vp9_iht8x8_add(
                    tx_type,
                    recon_coeff_buffer,
                    recon_buffer,
                    recon_stride,
                    *eob);
            }
        }
        break;

    default:
        assert(tx_size == TX_4X4);
        if (!(is_encode_pass && context_ptr->skip_eob_zero_mode_ep &&  *eob == 0)) {
            eb_vp9_residual_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][4 >> 3](
                input_buffer,
                input_stride,
                pred_buffer,
                pred_stride,
                residual_quant_coeff_buffer,
                residual_quant_coeff_stride,
                4,
                4);

            if (tx_type != DCT_DCT)
                vp9_fht4x4(
                    residual_quant_coeff_buffer,
                    trans_coeff_buffer,
                    residual_quant_coeff_stride,
                    tx_type);
            else
                vpx_fdct4x4(
                    residual_quant_coeff_buffer,
                    trans_coeff_buffer,
                    residual_quant_coeff_stride);

            eb_vp9_quantize_b(
                trans_coeff_buffer,
                16,
                0,
                zbin_ptr,
                round_ptr,
                quant_ptr,
                quant_shift_ptr,
                residual_quant_coeff_buffer,
                recon_coeff_buffer,
                dequant_ptr,
                eob,
                scan_order->scan,
                scan_order->iscan);

#if 0 // Hsan is it similar to RDOQ: to evaluate
            if (args->enable_coeff_opt) {
                *a = *l = vp9_optimize_b(x, plane, block, tx_size, entropy_ctx) > 0;
            }
#endif
        }
        if (context_ptr->spatial_sse_full_loop || (is_encode_pass && do_recon)) {
            // Hsan: both pred and rec samples are needed @ MD and EP to perform the eob zero mode decision
            pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][0](
                pred_buffer,
                pred_stride,
                recon_buffer,
                recon_stride,
                4,
                4);

            if (*eob) {

                if (tx_type == DCT_DCT)
                    // this is like vp9_short_idct4x4 but has a special case around eob<=1
                    // which is significant (not just an optimization) for the lossless
                    // case.
                    eb_vp9_idct4x4_add(
                        recon_coeff_buffer,
                        recon_buffer,
                        recon_stride,
                        *eob);
                else
                    vp9_iht4x4_16_add(
                        recon_coeff_buffer,
                        recon_buffer,
                        recon_stride,
                        tx_type);
            }
        }
        break;
    }
}

void perform_inv_trans_add(
    EncDecContext *context_ptr,
    EbByte         pred_buffer,
    uint16_t       pred_stride,
    int16_t*       recon_coeff_buffer,
    EbByte         recon_buffer,
    uint16_t       recon_stride,
    uint16_t      *eob,
    TX_SIZE        tx_size,
    int            plane)
{
    TX_TYPE tx_type = DCT_DCT;

    MACROBLOCKD *const xd = context_ptr->e_mbd;
    int block = context_ptr->bmi_index;

    if (tx_size == TX_4X4) {
        tx_type = get_tx_type_4x4(get_plane_type(plane), xd, block);
    }
    else {
        if (tx_size != TX_32X32) {
            tx_type = get_tx_type(get_plane_type(plane), xd);
        }
    }

    switch (tx_size) {
    case TX_32X32:

        pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][4](
            pred_buffer,
            pred_stride,
            recon_buffer,
            recon_stride,
            32,
            32);

        if (*eob) {
            eb_vp9_idct32x32_add(
                recon_coeff_buffer,
                recon_buffer,
                recon_stride,
                *eob);
        }

        break;

    case TX_16X16:

        pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][2](
            pred_buffer,
            pred_stride,
            recon_buffer,
            recon_stride,
            16,
            16);

        if (*eob) {
            eb_vp9_iht16x16_add(
                tx_type,
                recon_coeff_buffer,
                recon_buffer,
                recon_stride,
                *eob);
        }

        break;

    case TX_8X8:
        pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][1](
            pred_buffer,
            pred_stride,
            recon_buffer,
            recon_stride,
            8,
            8);

        if (*eob) {
            eb_vp9_iht8x8_add(
                tx_type,
                recon_coeff_buffer,
                recon_buffer,
                recon_stride,
                *eob);
        }

        break;

    default:
        assert(tx_size == TX_4X4);
        pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][0](
            pred_buffer,
            pred_stride,
            recon_buffer,
            recon_stride,
            4,
            4);

        if (*eob) {

            if (tx_type == DCT_DCT)
                // this is like vp9_short_idct4x4 but has a special case around eob<=1
                // which is significant (not just an optimization) for the lossless
                // case.
                eb_vp9_idct4x4_add(
                    recon_coeff_buffer,
                    recon_buffer,
                    recon_stride,
                    *eob);
            else
                vp9_iht4x4_16_add(
                    recon_coeff_buffer,
                    recon_buffer,
                    recon_stride,
                    tx_type);
        }

        break;
    }
}

void perform_inverse_transform_recon(
    EncDecContext               *context_ptr,
    ModeDecisionCandidateBuffer *candidate_buffer) {

    for (uint8_t tu_index = 0; tu_index < ((context_ptr->ep_block_stats_ptr->sq_size == MAX_SB_SIZE) ? 4 : 1); tu_index++) {

        uint32_t pred_recon_tu_origin_index = ((tu_index & 0x1) * tu_size[context_ptr->ep_block_stats_ptr->tx_size]) + ((tu_index > 1) *  tu_size[context_ptr->ep_block_stats_ptr->tx_size] * MAX_SB_SIZE);
        uint32_t reconCoeffTuOriginIndex = tu_index * tu_size[context_ptr->ep_block_stats_ptr->tx_size] * tu_size[context_ptr->ep_block_stats_ptr->tx_size];

        perform_inv_trans_add(
            context_ptr,
            &(candidate_buffer->prediction_ptr->buffer_y[context_ptr->block_origin_index + pred_recon_tu_origin_index]),
            candidate_buffer->prediction_ptr->stride_y,
            &(((int16_t*)candidate_buffer->recon_coeff_ptr->buffer_y)[reconCoeffTuOriginIndex]),
            &(candidate_buffer->recon_ptr->buffer_y[context_ptr->block_origin_index + pred_recon_tu_origin_index]),
            candidate_buffer->recon_ptr->stride_y,
            &candidate_buffer->candidate_ptr->eob[0][tu_index],
            context_ptr->ep_block_stats_ptr->tx_size,
            0);

    }

    if ((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) {
        perform_inv_trans_add(
            context_ptr,
            &(candidate_buffer->prediction_ptr->buffer_cb[context_ptr->block_chroma_origin_index]),
            candidate_buffer->prediction_ptr->stride_cb,
            &(((int16_t*)candidate_buffer->recon_coeff_ptr->buffer_cb)[0]),
            &(candidate_buffer->recon_ptr->buffer_cb[context_ptr->block_chroma_origin_index]),
            candidate_buffer->recon_ptr->stride_cb,
            &candidate_buffer->candidate_ptr->eob[1][0],
            context_ptr->ep_block_stats_ptr->tx_size_uv,
            1);

        perform_inv_trans_add(
            context_ptr,
            &(candidate_buffer->prediction_ptr->buffer_cr[context_ptr->block_chroma_origin_index]),
            candidate_buffer->prediction_ptr->stride_cr,
            &(((int16_t*)candidate_buffer->recon_coeff_ptr->buffer_cr)[0]),
            &(candidate_buffer->recon_ptr->buffer_cr[context_ptr->block_chroma_origin_index]),
            candidate_buffer->recon_ptr->stride_cr,
            &candidate_buffer->candidate_ptr->eob[2][0],
            context_ptr->ep_block_stats_ptr->tx_size_uv,
            2);
    }
}
static void perform_dist_rate_calc(
    EncDecContext     *context_ptr,
    PictureControlSet *picture_control_set_ptr,
    int16_t*           residual_quant_coeff_buffer,
    EbByte             input_buffer,
    uint16_t           input_stride,
    EbByte             pred_buffer,
    uint16_t           pred_stride,
    int16_t*           trans_coeff_buffer,
    int16_t*           recon_coeff_buffer,
    EbByte             recon_buffer,
    uint16_t           recon_stride,
    uint16_t          *eob,
    TX_SIZE            tx_size,
    int                plane,
    int                tu_index,
    PREDICTION_MODE    mode,
    int                tufull_distortion[DIST_CALC_TOTAL],
    int               *tu_coeff_bits)
{

    int coeff_ctx = 0;
    int tu_size = 1 << (2 + tx_size);

    if (context_ptr->spatial_sse_full_loop) {
        tufull_distortion[DIST_CALC_RESIDUAL] = (int)spatialfull_distortion_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][tx_size](
            input_buffer,
            input_stride,
            recon_buffer,
            recon_stride,
            tu_size,
            tu_size);

        tufull_distortion[DIST_CALC_PREDICTION] = (int)spatialfull_distortion_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][tx_size](
            input_buffer,
            input_stride,
            pred_buffer,
            pred_stride,
            tu_size,
            tu_size);

        tufull_distortion[DIST_CALC_RESIDUAL] = (tufull_distortion[DIST_CALC_RESIDUAL] << 4);
        tufull_distortion[DIST_CALC_PREDICTION] = (tufull_distortion[DIST_CALC_PREDICTION] << 4);
    }
    else {
        const int shift = tx_size == TX_32X32 ? 0 : 2;
        uint64_t                    tufull_distortionTemp[DIST_CALC_TOTAL];
        full_distortion_intrinsic_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][*eob != 0][0][tu_size >> 3](
            trans_coeff_buffer,
            tu_size,
            recon_coeff_buffer,
            tu_size,
            tufull_distortionTemp,
            tu_size,
            tu_size);

        tufull_distortion[DIST_CALC_RESIDUAL] = (int)(tufull_distortionTemp[DIST_CALC_RESIDUAL] >> shift);
        tufull_distortion[DIST_CALC_PREDICTION] = (int)(tufull_distortionTemp[DIST_CALC_PREDICTION] >> shift);
    }

    int blk_col = (tu_index % 2)*(1 << tx_size);
    int blk_row = (tu_index >> 1)*(1 << tx_size);
    *tu_coeff_bits = 0;

    coeff_ctx = combine_entropy_contexts(context_ptr->t_left[plane][blk_row], context_ptr->t_above[plane][blk_col]);

    *tu_coeff_bits = coeff_rate_estimate( //cost_coeffs
        context_ptr,
        &picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb,
        residual_quant_coeff_buffer,
        *eob,
        plane,
        tu_index,
        tx_size,
        coeff_ctx,
        0);

        if (context_ptr->eob_zero_mode) {
            if (plane == 0 && mode > TM_PRED) {
                RD_OPT *const rd = &picture_control_set_ptr->parent_pcs_ptr->cpi->rd;
                uint64_t rd1 = (uint64_t)RDCOST(context_ptr->RDMULT, rd->RDDIV, *tu_coeff_bits, (uint64_t)(tufull_distortion[DIST_CALC_RESIDUAL]));
                uint64_t rd2 = (uint64_t)RDCOST(context_ptr->RDMULT, rd->RDDIV, 0, (uint64_t)(tufull_distortion[DIST_CALC_PREDICTION]));
                if (rd1 >= rd2) {
                    tufull_distortion[DIST_CALC_RESIDUAL] = (tufull_distortion[DIST_CALC_PREDICTION]);
                    *tu_coeff_bits = 0;
                    if (*eob) {
                        pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][tu_size == 32 ? 4 : tx_size](
                            pred_buffer,
                            pred_stride,
                            recon_buffer,
                            recon_stride,
                            tu_size,
                            tu_size);
                        *eob = 0;
                    }
                }
            }
        }
}
    void perform_full_loop(
        PictureControlSet   *picture_control_set_ptr,
        EncDecContext       *context_ptr,
        SbUnit              *sb_ptr,
        EbPictureBufferDesc *input_picture_ptr,
        uint32_t             full_candidate_total_count)
    {
        (void)sb_ptr;
        uint32_t best_inter_luma_eob;
        uint64_t bestfull_cost;

        uint32_t full_loop_candidate_index;
        uint8_t  candidate_index;

        int tufull_distortion[3][DIST_CALC_TOTAL];

#if VP9_RD
        VP9_COMP   *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
        RD_OPT *const rd = &cpi->rd;
#endif
#if INTER_INTRA_BIAS
        const int intra_cost_penalty =
            vp9_get_intra_cost_penalty(context_ptr->ep_block_stats_ptr->bsize, cpi->common.base_qindex, cpi->common.y_dc_delta_q, picture_control_set_ptr->parent_pcs_ptr->sb_flat_noise_array[sb_ptr->sb_index]);
#endif
        best_inter_luma_eob = 1;
        bestfull_cost = 0xFFFFFFFFull;

        ModeDecisionCandidateBuffer **candidate_buffer_ptr_array_base = context_ptr->candidate_buffer_ptr_array;
        ModeDecisionCandidateBuffer **candidate_buffer_ptr_array = &(candidate_buffer_ptr_array_base[context_ptr->buffer_depth_index_start[context_ptr->ep_block_stats_ptr->depth]]);
        ModeDecisionCandidateBuffer  *candidate_buffer;
        ModeDecisionCandidate        *candidate_ptr;

        for (full_loop_candidate_index = 0; full_loop_candidate_index < full_candidate_total_count; ++full_loop_candidate_index) {

            int y_coeff_bits = 0;
            int cb_coeff_bits = 0;
            int cr_coeff_bits = 0;
            int yfull_distortion[DIST_CALC_TOTAL] = { 0,0 };
            int cbfull_distortion[DIST_CALC_TOTAL] = { 0,0 };
            int crfull_distortion[DIST_CALC_TOTAL] = { 0,0 };

            // Get candidate index
            candidate_index = context_ptr->best_candidate_index_array[full_loop_candidate_index];

            // Get full loop candidate
            candidate_buffer = candidate_buffer_ptr_array[candidate_index];

            // Get fast loop candidate
            candidate_ptr = candidate_buffer->candidate_ptr;

            // Initialize eob to 1
            for (uint8_t tu_index = 0; tu_index < ((context_ptr->ep_block_stats_ptr->sq_size == MAX_SB_SIZE) ? 4 : 1); tu_index++) {
                candidate_buffer->candidate_ptr->eob[0][tu_index] = 1;
            }
            candidate_buffer->candidate_ptr->eob[1][0] = 1;
            candidate_buffer->candidate_ptr->eob[2][0] = 1;

            // Set the WebM mi
            context_ptr->e_mbd->mi[0] = candidate_ptr->mode_info;

            if (picture_control_set_ptr->slice_type != I_SLICE) {
                if (candidate_ptr->mode_info->mode <= TM_PRED && best_inter_luma_eob == 0) {
                    continue;
                }
            }
            QUANTS *quants = &picture_control_set_ptr->parent_pcs_ptr->cpi->quants;
#if SEG_SUPPORT
            VP9_COMMON *const cm = &cpi->common;
            struct segmentation *const seg = &cm->seg;
            const int qindex = eb_vp9_get_qindex(seg, candidate_ptr->mode_info->segment_id, picture_control_set_ptr->base_qindex);

#else
            const int qindex = picture_control_set_ptr->base_qindex;
#endif
            for (uint8_t tu_index = 0; tu_index < ((context_ptr->ep_block_stats_ptr->sq_size == MAX_SB_SIZE) ? 4 : 1); tu_index++) {

                uint32_t input_tu_origin_index = ((tu_index & 0x1) * tu_size[context_ptr->ep_block_stats_ptr->tx_size]) + ((tu_index > 1) * tu_size[context_ptr->ep_block_stats_ptr->tx_size] * input_picture_ptr->stride_y);
                uint32_t residual_quant_tu_origin_index = tu_index * (tu_size[context_ptr->ep_block_stats_ptr->tx_size]) * (tu_size[context_ptr->ep_block_stats_ptr->tx_size]);
                uint32_t pred_recon_tu_origin_index = ((tu_index & 0x1) * tu_size[context_ptr->ep_block_stats_ptr->tx_size]) + ((tu_index > 1) * tu_size[context_ptr->ep_block_stats_ptr->tx_size] * MAX_SB_SIZE);
                int tu_coeff_bits = 0;

                // Luma Coding Loop
                perform_coding_loop(
                    context_ptr,
                    &(((int16_t*)candidate_buffer->residual_quant_coeff_ptr->buffer_y)[residual_quant_tu_origin_index]),
                    (uint16_t)tu_size[context_ptr->ep_block_stats_ptr->tx_size],
                    &(input_picture_ptr->buffer_y[context_ptr->input_origin_index + input_tu_origin_index]),
                    input_picture_ptr->stride_y,
                    &(candidate_buffer->prediction_ptr->buffer_y[context_ptr->block_origin_index + pred_recon_tu_origin_index]),
                    candidate_buffer->prediction_ptr->stride_y,
                    &(((int16_t*)context_ptr->trans_quant_buffers_ptr->tu_trans_coeff2_nx2_n_ptr->buffer_y)[0]),   // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                    &(((int16_t*)candidate_buffer->recon_coeff_ptr->buffer_y)[residual_quant_tu_origin_index]),                           // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                    &(candidate_buffer->recon_ptr->buffer_y[context_ptr->block_origin_index + pred_recon_tu_origin_index]),
                    candidate_buffer->recon_ptr->stride_y,
                    quants->y_zbin[qindex],
                    quants->y_round[qindex],
                    quants->y_quant[qindex],
                    quants->y_quant_shift[qindex],
                    &picture_control_set_ptr->parent_pcs_ptr->cpi->y_dequant[qindex][0],
                    &candidate_buffer->candidate_ptr->eob[0][tu_index],
                    context_ptr->ep_block_stats_ptr->tx_size,
                    0,
                    0,
                    0);

#if VP9_RD
                // Luma Distortion and Rate Calculation
                perform_dist_rate_calc(
                    context_ptr,
                    picture_control_set_ptr,
                    &(((int16_t*)candidate_buffer->residual_quant_coeff_ptr->buffer_y)[residual_quant_tu_origin_index]),
                    &(input_picture_ptr->buffer_y[context_ptr->input_origin_index + input_tu_origin_index]),
                    input_picture_ptr->stride_y,
                    &(candidate_buffer->prediction_ptr->buffer_y[context_ptr->block_origin_index + pred_recon_tu_origin_index]),
                    candidate_buffer->prediction_ptr->stride_y,
                    &(((int16_t*)context_ptr->trans_quant_buffers_ptr->tu_trans_coeff2_nx2_n_ptr->buffer_y)[0]),
                    &(((int16_t*)candidate_buffer->recon_coeff_ptr->buffer_y)[residual_quant_tu_origin_index]),
                    &(candidate_buffer->recon_ptr->buffer_y[context_ptr->block_origin_index + pred_recon_tu_origin_index]),
                    candidate_buffer->recon_ptr->stride_y,
                    &candidate_buffer->candidate_ptr->eob[0][tu_index],
                    context_ptr->ep_block_stats_ptr->tx_size,
                    0,
                    tu_index,
                    candidate_buffer->candidate_ptr->mode_info->mode,
                    tufull_distortion[0],
                    &tu_coeff_bits);

                yfull_distortion[DIST_CALC_RESIDUAL] += tufull_distortion[0][DIST_CALC_RESIDUAL];
                yfull_distortion[DIST_CALC_PREDICTION] += tufull_distortion[0][DIST_CALC_PREDICTION];
                y_coeff_bits += tu_coeff_bits;
#endif
            }

            if ((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) {
                // Cb Coding Loop
                perform_coding_loop(
                    context_ptr,
                    &(((int16_t*)candidate_buffer->residual_quant_coeff_ptr->buffer_cb)[0]),
                    (uint16_t)tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv],
                    &(input_picture_ptr->buffer_cb[context_ptr->input_chroma_origin_index]),
                    input_picture_ptr->stride_cb,
                    &(candidate_buffer->prediction_ptr->buffer_cb[context_ptr->block_chroma_origin_index]),
                    candidate_buffer->prediction_ptr->stride_cb,
                    &(((int16_t*)context_ptr->trans_quant_buffers_ptr->tu_trans_coeff2_nx2_n_ptr->buffer_cb)[0]),  // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                    &(((int16_t*)candidate_buffer->recon_coeff_ptr->buffer_cb)[0]),                          // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                    &(candidate_buffer->recon_ptr->buffer_cb[context_ptr->block_chroma_origin_index]),
                    candidate_buffer->recon_ptr->stride_cb,
                    quants->uv_zbin[qindex],
                    quants->uv_round[qindex],
                    quants->uv_quant[qindex],
                    quants->uv_quant_shift[qindex],
                    &picture_control_set_ptr->parent_pcs_ptr->cpi->uv_dequant[qindex][0],
                    &candidate_buffer->candidate_ptr->eob[1][0],
                    context_ptr->ep_block_stats_ptr->tx_size_uv,
                    1,
                    0,
                    0);

#if VP9_RD
                // Cb Distortion and Rate Calculation
                perform_dist_rate_calc(
                    context_ptr,
                    picture_control_set_ptr,
                    &(((int16_t*)candidate_buffer->residual_quant_coeff_ptr->buffer_cb)[0]),
                    &(input_picture_ptr->buffer_cb[context_ptr->input_chroma_origin_index]),
                    input_picture_ptr->stride_cb,
                    &(candidate_buffer->prediction_ptr->buffer_cb[context_ptr->block_chroma_origin_index]),
                    candidate_buffer->prediction_ptr->stride_cb,
                    &(((int16_t*)context_ptr->trans_quant_buffers_ptr->tu_trans_coeff2_nx2_n_ptr->buffer_cb)[0]),
                    &(((int16_t*)candidate_buffer->recon_coeff_ptr->buffer_cb)[0]),
                    &(candidate_buffer->recon_ptr->buffer_cb[context_ptr->block_chroma_origin_index]),
                    candidate_buffer->recon_ptr->stride_cb,
                    &candidate_buffer->candidate_ptr->eob[1][0],
                    context_ptr->ep_block_stats_ptr->tx_size_uv,
                    1,
                    0,
                    candidate_buffer->candidate_ptr->mode_info->mode,
                    &cbfull_distortion[0],
                    &cb_coeff_bits);
#endif

                // Cr Coding Loop
                perform_coding_loop(
                    context_ptr,
                    &(((int16_t*)candidate_buffer->residual_quant_coeff_ptr->buffer_cr)[0]),
                    (uint16_t)tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv],
                    &(input_picture_ptr->buffer_cr[context_ptr->input_chroma_origin_index]),
                    input_picture_ptr->stride_cr,
                    &(candidate_buffer->prediction_ptr->buffer_cr[context_ptr->block_chroma_origin_index]),
                    candidate_buffer->prediction_ptr->stride_cr,
                    &(((int16_t*)context_ptr->trans_quant_buffers_ptr->tu_trans_coeff2_nx2_n_ptr->buffer_cr)[0]),  // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                    &(((int16_t*)candidate_buffer->recon_coeff_ptr->buffer_cr)[0]),                          // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                    &(candidate_buffer->recon_ptr->buffer_cr[context_ptr->block_chroma_origin_index]),
                    candidate_buffer->recon_ptr->stride_cr,
                    quants->uv_zbin[qindex],
                    quants->uv_round[qindex],
                    quants->uv_quant[qindex],
                    quants->uv_quant_shift[qindex],
                    &picture_control_set_ptr->parent_pcs_ptr->cpi->uv_dequant[qindex][0],
                    &candidate_buffer->candidate_ptr->eob[2][0],
                    context_ptr->ep_block_stats_ptr->tx_size_uv,
                    2,
                    0,
                    0);

#if VP9_RD
                // Cr Distortion and Rate Calculation
                perform_dist_rate_calc(
                    context_ptr,
                    picture_control_set_ptr,
                    &(((int16_t*)candidate_buffer->residual_quant_coeff_ptr->buffer_cr)[0]),
                    &(input_picture_ptr->buffer_cr[context_ptr->input_chroma_origin_index]),
                    input_picture_ptr->stride_cr,
                    &(candidate_buffer->prediction_ptr->buffer_cr[context_ptr->block_chroma_origin_index]),
                    candidate_buffer->prediction_ptr->stride_cr,
                    &(((int16_t*)context_ptr->trans_quant_buffers_ptr->tu_trans_coeff2_nx2_n_ptr->buffer_cr)[0]),
                    &(((int16_t*)candidate_buffer->recon_coeff_ptr->buffer_cr)[0]),
                    &(candidate_buffer->recon_ptr->buffer_cr[context_ptr->block_chroma_origin_index]),
                    candidate_buffer->recon_ptr->stride_cr,
                    &candidate_buffer->candidate_ptr->eob[2][0],
                    context_ptr->ep_block_stats_ptr->tx_size_uv,
                    2,
                    0,
                    candidate_buffer->candidate_ptr->mode_info->mode,
                    &crfull_distortion[0],
                    &cr_coeff_bits);

                // SKIP decision making
                    if (context_ptr->eob_zero_mode) {
                        if (candidate_buffer->candidate_ptr->mode_info->mode > TM_PRED) {
                            MACROBLOCKD *const xd = context_ptr->e_mbd;
                            int rate1 = y_coeff_bits + cb_coeff_bits + cr_coeff_bits + vp9_cost_bit(vp9_get_skip_prob(&cpi->common, xd), 0);
                            int rate2 = vp9_cost_bit(vp9_get_skip_prob(&cpi->common, xd), 1);
                            int distortion1 = yfull_distortion[DIST_CALC_RESIDUAL] + cbfull_distortion[DIST_CALC_RESIDUAL] + crfull_distortion[DIST_CALC_RESIDUAL];
                            int distortion2 = yfull_distortion[DIST_CALC_PREDICTION] + cbfull_distortion[DIST_CALC_PREDICTION] + crfull_distortion[DIST_CALC_PREDICTION];
                            uint64_t rd1 = (uint64_t)RDCOST(context_ptr->RDMULT, rd->RDDIV, rate1, (uint64_t)distortion1);
                            uint64_t rd2 = (uint64_t)RDCOST(context_ptr->RDMULT, rd->RDDIV, rate2, (uint64_t)distortion2);
                            if (rd1 >= rd2) {
                                for (uint8_t tu_index = 0; tu_index < ((context_ptr->ep_block_stats_ptr->sq_size == MAX_SB_SIZE) ? 4 : 1); tu_index++) {
                                    if (candidate_buffer->candidate_ptr->eob[0][tu_index]) {
                                        uint32_t pred_recon_tu_origin_index = ((tu_index % 2) * tu_size[context_ptr->ep_block_stats_ptr->tx_size]) + ((tu_index > 1) * tu_size[context_ptr->ep_block_stats_ptr->tx_size] * MAX_SB_SIZE);
                                        candidate_buffer->candidate_ptr->eob[0][tu_index] = 0;
                                        int arr_index = tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv] == 32 ? 4 : context_ptr->ep_block_stats_ptr->tx_size - 1;
                                        if (arr_index >= 0 && arr_index < 9)
                                            pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][arr_index](
                                                &(candidate_buffer->prediction_ptr->buffer_y[context_ptr->block_origin_index + pred_recon_tu_origin_index]),
                                                candidate_buffer->prediction_ptr->stride_y,
                                                &(candidate_buffer->recon_ptr->buffer_y[context_ptr->block_origin_index + pred_recon_tu_origin_index]),
                                                candidate_buffer->recon_ptr->stride_y,
                                                tu_size[context_ptr->ep_block_stats_ptr->tx_size],
                                                tu_size[context_ptr->ep_block_stats_ptr->tx_size]);
                                    }
                                }

                                if (candidate_buffer->candidate_ptr->eob[1][0]) {
                                    candidate_buffer->candidate_ptr->eob[1][0] = 0;
                                    int arr_index = tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv] == 32 ? 4 : context_ptr->ep_block_stats_ptr->tx_size - 1;
                                    if (arr_index >= 0 && arr_index < 9)
                                        pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][arr_index](
                                            &(candidate_buffer->prediction_ptr->buffer_cb[context_ptr->block_chroma_origin_index]),
                                            candidate_buffer->prediction_ptr->stride_cb,
                                            &(candidate_buffer->recon_ptr->buffer_cb[context_ptr->block_chroma_origin_index]),
                                            candidate_buffer->recon_ptr->stride_cb,
                                            tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv],
                                            tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv]);
                                }

                                if (candidate_buffer->candidate_ptr->eob[2][0]) {
                                    candidate_buffer->candidate_ptr->eob[2][0] = 0;
                                    int arr_index = tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv] == 32 ? 4 : context_ptr->ep_block_stats_ptr->tx_size - 1;
                                    if (arr_index >= 0 && arr_index < 9)
                                        pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][arr_index](
                                            &(candidate_buffer->prediction_ptr->buffer_cr[context_ptr->block_chroma_origin_index]),
                                            candidate_buffer->prediction_ptr->stride_cr,
                                            &(candidate_buffer->recon_ptr->buffer_cr[context_ptr->block_chroma_origin_index]),
                                            candidate_buffer->recon_ptr->stride_cr,
                                            tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv],
                                            tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv]);
                                }
                            }
                        }
                    }
#endif
            }

                // Derive skip flag
                candidate_ptr->mode_info->skip = EB_TRUE;
                if (candidate_buffer->candidate_ptr->eob[1][0] || candidate_buffer->candidate_ptr->eob[2][0]) {
                    candidate_ptr->mode_info->skip = EB_FALSE;
                }
                else {
                    for (uint8_t tu_index = 0; tu_index < ((context_ptr->ep_block_stats_ptr->sq_size == MAX_SB_SIZE) ? 4 : 1); tu_index++) {
                        if (candidate_buffer->candidate_ptr->eob[0][tu_index]) {
                            candidate_ptr->mode_info->skip = EB_FALSE;
                            break;
                        }
                    }
                }

#if VP9_RD
                full_cost_func_table[candidate_ptr->mode_info->mode <= TM_PRED](
                    context_ptr,
                    candidate_buffer,
                    yfull_distortion,
                    cbfull_distortion,
                    crfull_distortion,
                    &y_coeff_bits,
                    &cb_coeff_bits,
                    &cr_coeff_bits,
                    picture_control_set_ptr);
#if INTER_INTRA_BIAS
                if (picture_control_set_ptr->slice_type != I_SLICE && candidate_ptr->mode_info->mode <= TM_PRED && candidate_ptr->mode_info->mode != DC_PRED && candidate_ptr->mode_info->mode != TM_PRED)
                    *candidate_buffer->full_cost_ptr += intra_cost_penalty;
#endif
#endif
                if (context_ptr->full_loop_escape) {
                    if (picture_control_set_ptr->slice_type != I_SLICE) {
                        if (context_ptr->ep_block_stats_ptr->sq_size != MAX_SB_SIZE) {
                            if (candidate_ptr->mode_info->mode > TM_PRED) {
                                if (*candidate_buffer->full_cost_ptr < bestfull_cost) {
                                    best_inter_luma_eob = candidate_buffer->candidate_ptr->eob[0][0];
                                    bestfull_cost = *candidate_buffer->full_cost_ptr;
                                }
                            }
                        }
                    }
                }
        }
    }

        // use neighbor sample arrays to contruct intra reference samples
        void generate_intra_reference_samples(
            SequenceControlSet *sequence_control_set_ptr,
            EncDecContext      *context_ptr,
            int                 plane)
        {

            MACROBLOCKD *const xd = context_ptr->e_mbd;
            struct macroblockd_plane *const pd = &xd->plane[plane];
            const TX_SIZE tx_size = plane ? context_ptr->ep_block_stats_ptr->tx_size_uv : context_ptr->ep_block_stats_ptr->tx_size;
            const BLOCK_SIZE plane_bsize = get_plane_block_size(VPXMAX(context_ptr->ep_block_stats_ptr->bsize, BLOCK_8X8), pd);
            const int bwl = eb_vp9_b_width_log2_lookup[plane_bsize];
            const int aoff = context_ptr->bmi_index & 0x1;
            const int loff = context_ptr->bmi_index >> 1;

            const int bw = (1 << bwl);
            const int txw = (1 << tx_size);

            const int have_top = ((plane == 0 && context_ptr->block_origin_y > 0) || ((ROUND_UV(context_ptr->block_origin_y) >> 1) > 0));
            const int have_left = ((plane == 0 && context_ptr->block_origin_x > 0) || ((ROUND_UV(context_ptr->block_origin_x) >> 1) > 0));

            const int have_right = (aoff + txw) < bw;

            const int x = aoff * 4;
            const int y = loff * 4;

            int i;

            uint8_t *above_row = context_ptr->above_data[plane] + 16;
            context_ptr->const_above_row[plane] = above_row;
            const int bs = 4 << tx_size;
            int frame_width, frame_height;
            int x0, y0;

            // 127 127 127 .. 127 127 127 127 127 127
            // 129  A   B  ..  Y   Z
            // 129  C   D  ..  W   X
            // 129  E   F  ..  U   V
            // 129  G   H  ..  S   T   T   T   T   T
            // ..

            // Get current frame width and height.
            if (plane == 0) {
                frame_width = sequence_control_set_ptr->luma_width;
                frame_height = sequence_control_set_ptr->luma_height;
            }
            else {
                frame_width = sequence_control_set_ptr->chroma_width;
                frame_height = sequence_control_set_ptr->chroma_height;
            }

            // Get block position in current frame.
            x0 = (-xd->mb_to_left_edge >> (3 + pd->subsampling_x)) + x;
            y0 = (-xd->mb_to_top_edge >> (3 + pd->subsampling_y)) + y;

            if (plane == 0) {
                if (context_ptr->block_origin_y != 0)
                    EB_MEMCPY(context_ptr->intra_above_ref + 1, context_ptr->luma_recon_neighbor_array->top_array + context_ptr->block_origin_x, context_ptr->ep_block_stats_ptr->sq_size << 1);
                if (context_ptr->block_origin_x != 0)
                    EB_MEMCPY(context_ptr->intra_left_ref + 1, context_ptr->luma_recon_neighbor_array->left_array + context_ptr->block_origin_y, context_ptr->ep_block_stats_ptr->sq_size << 1);
                if (context_ptr->block_origin_y != 0 && context_ptr->block_origin_x != 0)
                    context_ptr->intra_above_ref[0] = context_ptr->intra_left_ref[0] = context_ptr->luma_recon_neighbor_array->top_left_array[MAX_PICTURE_HEIGHT_SIZE + context_ptr->block_origin_x - context_ptr->block_origin_y];
            }
            else if (plane == 1) {
                if ((ROUND_UV(context_ptr->block_origin_y) >> 1) > 0)
                    EB_MEMCPY(context_ptr->intra_above_ref + 1, context_ptr->cb_recon_neighbor_array->top_array + (ROUND_UV(context_ptr->block_origin_x) >> 1), context_ptr->ep_block_stats_ptr->sq_size_uv << 1);
                if ((ROUND_UV(context_ptr->block_origin_x) >> 1) > 0)
                    EB_MEMCPY(context_ptr->intra_left_ref + 1, context_ptr->cb_recon_neighbor_array->left_array + (ROUND_UV(context_ptr->block_origin_y) >> 1), context_ptr->ep_block_stats_ptr->sq_size_uv << 1);
                if (((ROUND_UV(context_ptr->block_origin_y) >> 1) > 0) && ((ROUND_UV(context_ptr->block_origin_x) >> 1) > 0))
                    context_ptr->intra_above_ref[0] = context_ptr->intra_left_ref[0] = context_ptr->cb_recon_neighbor_array->top_left_array[(MAX_PICTURE_HEIGHT_SIZE >> 1) + (ROUND_UV(context_ptr->block_origin_x) >> 1) - (ROUND_UV(context_ptr->block_origin_y) >> 1)];
            }
            else {
                if ((ROUND_UV(context_ptr->block_origin_y) >> 1) > 0)
                    EB_MEMCPY(context_ptr->intra_above_ref + 1, context_ptr->cr_recon_neighbor_array->top_array + (ROUND_UV(context_ptr->block_origin_x) >> 1), context_ptr->ep_block_stats_ptr->sq_size_uv << 1);
                if ((ROUND_UV(context_ptr->block_origin_x) >> 1) > 0)
                    EB_MEMCPY(context_ptr->intra_left_ref + 1, context_ptr->cr_recon_neighbor_array->left_array + (ROUND_UV(context_ptr->block_origin_y) >> 1), context_ptr->ep_block_stats_ptr->sq_size_uv << 1);
                if (((ROUND_UV(context_ptr->block_origin_y) >> 1) > 0) && ((ROUND_UV(context_ptr->block_origin_x) >> 1) > 0))
                    context_ptr->intra_above_ref[0] = context_ptr->intra_left_ref[0] = context_ptr->cr_recon_neighbor_array->top_left_array[(MAX_PICTURE_HEIGHT_SIZE >> 1) + (ROUND_UV(context_ptr->block_origin_x) >> 1) - (ROUND_UV(context_ptr->block_origin_y) >> 1)];
            }

            // NEED_LEFT
            if (have_left) {
                if (xd->mb_to_bottom_edge < 0) {
                    /* slower path if the block needs border extension */
                    if (y0 + bs <= frame_height) {
                        for (i = 0; i < bs; ++i) context_ptr->left_col[plane][i] = context_ptr->intra_left_ref[1 + i];
                    }
                    else {
                        const int extend_bottom = frame_height - y0;
                        for (i = 0; i < extend_bottom; ++i)
                            context_ptr->left_col[plane][i] = context_ptr->intra_left_ref[1 + i];
                        for (; i < bs; ++i)
                            context_ptr->left_col[plane][i] = context_ptr->intra_left_ref[1 + extend_bottom - 1];
                    }
                }
                else {
                    /* faster path if the block does not need extension */
                    for (i = 0; i < bs; ++i) context_ptr->left_col[plane][i] = context_ptr->intra_left_ref[1 + i];
                }
            }
            else {
                memset(context_ptr->left_col[plane], 129, bs);
            }

            // NEED_ABOVE
            if (have_top) {

                const uint8_t *above_ref = context_ptr->intra_above_ref + 1;

                if (xd->mb_to_right_edge < 0) {
                    /* slower path if the block needs border extension */
                    if (x0 + bs <= frame_width) {
                        EB_MEMCPY(above_row, (void*)above_ref, bs);
                    }
                    else if (x0 <= frame_width) {
                        const int r = frame_width - x0;
                        EB_MEMCPY(above_row, (void*)above_ref, r);
                        memset(above_row + r, above_row[r - 1], x0 + bs - frame_width);
                    }
                }
                else {
                    /* faster path if the block does not need extension */
                    if (bs == 4 && have_right && have_left) {
                        context_ptr->const_above_row[plane] = above_ref;
                    }
                    else {
                        EB_MEMCPY(above_row, (void*)above_ref, bs);
                    }
                }
                above_row[-1] = have_left ? above_ref[-1] : 129;
            }
            else {
                memset(above_row, 127, bs);
                above_row[-1] = 127;
            }

            // NEED_ABOVERIGHT
            if (have_top) {
                const uint8_t *above_ref = context_ptr->intra_above_ref + 1;

                if (xd->mb_to_right_edge < 0) {
                    /* slower path if the block needs border extension */
                    if (x0 + 2 * bs <= frame_width) {
                        if (have_right && bs == 4) {
                            EB_MEMCPY(above_row, (void*)above_ref, 2 * bs);
                        }
                        else {
                            EB_MEMCPY(above_row, (void*)above_ref, bs);
                            memset(above_row + bs, above_row[bs - 1], bs);
                        }
                    }
                    else if (x0 + bs <= frame_width) {
                        const int r = frame_width - x0;
                        if (have_right && bs == 4) {
                            EB_MEMCPY(above_row, (void*)above_ref, r);
                            memset(above_row + r, above_row[r - 1], x0 + 2 * bs - frame_width);
                        }
                        else {
                            EB_MEMCPY(above_row, (void*)above_ref, bs);
                            memset(above_row + bs, above_row[bs - 1], bs);
                        }
                    }
                    else if (x0 <= frame_width) {
                        const int r = frame_width - x0;
                        EB_MEMCPY(above_row, (void*)above_ref, r);
                        memset(above_row + r, above_row[r - 1], x0 + 2 * bs - frame_width);
                    }
                }
                else {
                    /* faster path if the block does not need extension */
                    if (bs == 4 && have_right && have_left) {
                        context_ptr->const_above_row[plane] = above_ref;
                    }
                    else {
                        EB_MEMCPY(above_row, (void*)above_ref, bs);
                        if (bs == 4 && have_right)
                            EB_MEMCPY(above_row + bs, (void*)(above_ref + bs), bs);
                        else
                            memset(above_row + bs, above_row[bs - 1], bs);
                    }
                }
                above_row[-1] = have_left ? above_ref[-1] : 129;
            }
            else {
                memset(above_row, 127, bs * 2);
                above_row[-1] = 127;
            }
        }

        // use input or recon buffer to construct neighbor sample arrays
        void update_recon_neighbor_arrays(
            EbPictureBufferDesc         *input_picture_ptr,
            EncDecContext               *context_ptr,
            SbUnit                      *sb_ptr,
            ModeDecisionCandidateBuffer *candidate_buffer,
            EB_BOOL                      recon_location)
        {  // 0: candidate buffer ot 1: scratch buffer
            (void)sb_ptr;
            if (context_ptr->intra_md_open_loop_flag) {

                // Input Samples - Luma
                eb_vp9_neighbor_array_unit_sample_write(
                    context_ptr->luma_recon_neighbor_array,
                    input_picture_ptr->buffer_y,
                    input_picture_ptr->stride_y,
                    input_picture_ptr->origin_x + context_ptr->block_origin_x,
                    input_picture_ptr->origin_y + context_ptr->block_origin_y,
                    context_ptr->block_origin_x,
                    context_ptr->block_origin_y,
                    context_ptr->ep_block_stats_ptr->bwidth,
                    context_ptr->ep_block_stats_ptr->bheight,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                if ((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) {

                    // Input Samples - Cb
                    eb_vp9_neighbor_array_unit_sample_write(
                        context_ptr->cb_recon_neighbor_array,
                        input_picture_ptr->buffer_cb,
                        input_picture_ptr->stride_cb,
                        (input_picture_ptr->origin_x + ROUND_UV(context_ptr->block_origin_x)) >> 1,
                        (input_picture_ptr->origin_y + ROUND_UV(context_ptr->block_origin_y)) >> 1,
                        ROUND_UV(context_ptr->block_origin_x) >> 1,
                        ROUND_UV(context_ptr->block_origin_y) >> 1,
                        context_ptr->ep_block_stats_ptr->bwidth_uv,
                        context_ptr->ep_block_stats_ptr->bheight_uv,
                        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                    // Input Samples - Cr
                    eb_vp9_neighbor_array_unit_sample_write(
                        context_ptr->cr_recon_neighbor_array,
                        input_picture_ptr->buffer_cr,
                        input_picture_ptr->stride_cr,
                        (input_picture_ptr->origin_x + ROUND_UV(context_ptr->block_origin_x)) >> 1,
                        (input_picture_ptr->origin_y + ROUND_UV(context_ptr->block_origin_y)) >> 1,
                        ROUND_UV(context_ptr->block_origin_x) >> 1,
                        ROUND_UV(context_ptr->block_origin_y) >> 1,
                        context_ptr->ep_block_stats_ptr->bwidth_uv,
                        context_ptr->ep_block_stats_ptr->bheight_uv,
                        NEIGHBOR_ARRAY_UNIT_FULL_MASK);
                }

            }
            else {

                // Set recon buffer
                EbPictureBufferDesc   *recon_buffer = (recon_location) ?
                    context_ptr->bdp_pillar_scratch_recon_buffer :
                    candidate_buffer->recon_ptr;

                // Recon Samples - Luma
                eb_vp9_neighbor_array_unit_sample_write(
                    context_ptr->luma_recon_neighbor_array,
                    recon_buffer->buffer_y,
                    recon_buffer->stride_y,
                    context_ptr->ep_block_stats_ptr->origin_x,
                    context_ptr->ep_block_stats_ptr->origin_y,
                    context_ptr->block_origin_x,
                    context_ptr->block_origin_y,
                    context_ptr->ep_block_stats_ptr->bwidth,
                    context_ptr->ep_block_stats_ptr->bheight,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                if ((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) {
                    // Recon Samples - Cb
                    eb_vp9_neighbor_array_unit_sample_write(
                        context_ptr->cb_recon_neighbor_array,
                        recon_buffer->buffer_cb,
                        recon_buffer->stride_cb,
                        ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x) >> 1,
                        ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) >> 1,
                        ROUND_UV(context_ptr->block_origin_x) >> 1,
                        ROUND_UV(context_ptr->block_origin_y) >> 1,
                        context_ptr->ep_block_stats_ptr->bwidth_uv,
                        context_ptr->ep_block_stats_ptr->bheight_uv,
                        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                    // Recon Samples - Cr
                    eb_vp9_neighbor_array_unit_sample_write(
                        context_ptr->cr_recon_neighbor_array,
                        recon_buffer->buffer_cr,
                        recon_buffer->stride_cr,
                        ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x) >> 1,
                        ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) >> 1,
                        ROUND_UV(context_ptr->block_origin_x) >> 1,
                        ROUND_UV(context_ptr->block_origin_y) >> 1,
                        context_ptr->ep_block_stats_ptr->bwidth_uv,
                        context_ptr->ep_block_stats_ptr->bheight_uv,
                        NEIGHBOR_ARRAY_UNIT_FULL_MASK);
                }

            }
        }

        // use input or recon buffer to construct neighbor sample arrays
        void update_bdp_recon_neighbor_arrays(
            PictureControlSet           *picture_control_set_ptr,
            EbPictureBufferDesc         *input_picture_ptr,
            EncDecContext               *context_ptr,
            SbUnit                      *sb_ptr,
            ModeDecisionCandidateBuffer *candidate_buffer,
            int                          depth_part_stage)
        {
            (void)sb_ptr;
            if (context_ptr->intra_md_open_loop_flag) {

                // Input Samples - Luma
                eb_vp9_neighbor_array_unit_sample_write(
                    picture_control_set_ptr->md_luma_recon_neighbor_array[depth_part_stage],
                    input_picture_ptr->buffer_y,
                    input_picture_ptr->stride_y,
                    input_picture_ptr->origin_x + context_ptr->block_origin_x,
                    input_picture_ptr->origin_y + context_ptr->block_origin_y,
                    context_ptr->block_origin_x,
                    context_ptr->block_origin_y,
                    context_ptr->ep_block_stats_ptr->bwidth,
                    context_ptr->ep_block_stats_ptr->bheight,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                if ((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) {

                    // Input Samples - Cb
                    eb_vp9_neighbor_array_unit_sample_write(
                        picture_control_set_ptr->md_cb_recon_neighbor_array[depth_part_stage],
                        input_picture_ptr->buffer_cb,
                        input_picture_ptr->stride_cb,
                        (input_picture_ptr->origin_x + ROUND_UV(context_ptr->block_origin_x)) >> 1,
                        (input_picture_ptr->origin_y + ROUND_UV(context_ptr->block_origin_y)) >> 1,
                        ROUND_UV(context_ptr->block_origin_x) >> 1,
                        ROUND_UV(context_ptr->block_origin_y) >> 1,
                        context_ptr->ep_block_stats_ptr->bwidth_uv,
                        context_ptr->ep_block_stats_ptr->bheight_uv,
                        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                    // Input Samples - Cr
                    eb_vp9_neighbor_array_unit_sample_write(
                        picture_control_set_ptr->md_cr_recon_neighbor_array[depth_part_stage],
                        input_picture_ptr->buffer_cr,
                        input_picture_ptr->stride_cr,
                        (input_picture_ptr->origin_x + ROUND_UV(context_ptr->block_origin_x)) >> 1,
                        (input_picture_ptr->origin_y + ROUND_UV(context_ptr->block_origin_y)) >> 1,
                        ROUND_UV(context_ptr->block_origin_x) >> 1,
                        ROUND_UV(context_ptr->block_origin_y) >> 1,
                        context_ptr->ep_block_stats_ptr->bwidth_uv,
                        context_ptr->ep_block_stats_ptr->bheight_uv,
                        NEIGHBOR_ARRAY_UNIT_FULL_MASK);
                }

            }
            else {

                // Recon Samples - Luma
                eb_vp9_neighbor_array_unit_sample_write(
                    picture_control_set_ptr->md_luma_recon_neighbor_array[depth_part_stage],
                    candidate_buffer->recon_ptr->buffer_y,
                    candidate_buffer->recon_ptr->stride_y,
                    context_ptr->ep_block_stats_ptr->origin_x,
                    context_ptr->ep_block_stats_ptr->origin_y,
                    context_ptr->block_origin_x,
                    context_ptr->block_origin_y,
                    context_ptr->ep_block_stats_ptr->bwidth,
                    context_ptr->ep_block_stats_ptr->bheight,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                if ((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) {
                    // Recon Samples - Cb
                    eb_vp9_neighbor_array_unit_sample_write(
                        picture_control_set_ptr->md_cb_recon_neighbor_array[depth_part_stage],
                        candidate_buffer->recon_ptr->buffer_cb,
                        candidate_buffer->recon_ptr->stride_cb,
                        ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x) >> 1,
                        ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) >> 1,
                        ROUND_UV(context_ptr->block_origin_x) >> 1,
                        ROUND_UV(context_ptr->block_origin_y) >> 1,
                        context_ptr->ep_block_stats_ptr->bwidth_uv,
                        context_ptr->ep_block_stats_ptr->bheight_uv,
                        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                    // Recon Samples - Cr
                    eb_vp9_neighbor_array_unit_sample_write(
                        picture_control_set_ptr->md_cr_recon_neighbor_array[depth_part_stage],
                        candidate_buffer->recon_ptr->buffer_cr,
                        candidate_buffer->recon_ptr->stride_cr,
                        ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x) >> 1,
                        ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) >> 1,
                        ROUND_UV(context_ptr->block_origin_x) >> 1,
                        ROUND_UV(context_ptr->block_origin_y) >> 1,
                        context_ptr->ep_block_stats_ptr->bwidth_uv,
                        context_ptr->ep_block_stats_ptr->bheight_uv,
                        NEIGHBOR_ARRAY_UNIT_FULL_MASK);
                }

            }
        }

        // Set context for the rate of sbs
        void set_sb_rate_context(
            PictureControlSet *picture_control_set_ptr,
            EncDecContext     *context_ptr,
            MACROBLOCKD       *xd)
        {

            VP9_COMP    *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
            VP9_COMMON  *const cm = &cpi->common;

            if (context_ptr->sb_ptr->origin_x == 0) {
                set_partition_probs(cm, xd);
            }

            for (int i = 0; i < MAX_MB_PLANE; ++i) {
                xd->above_context[i] =
                    context_ptr->above_context +
                    i * sizeof(*context_ptr->above_context) * 2 * mi_cols_aligned_to_sb(cm->mi_cols);
            }
            xd->above_seg_context = context_ptr->above_seg_context;

            for (int i = 0; i < MAX_MB_PLANE; ++i) {
                EB_MEMCPY(&xd->left_context[i], &context_ptr->left_context[(context_ptr->sb_ptr->origin_y >> 2) + (i * sizeof(*context_ptr->left_context) * 2 * mi_cols_aligned_to_sb(cm->mi_rows))], 16);
            }
            EB_MEMCPY(&xd->left_seg_context, &context_ptr->left_seg_context[context_ptr->sb_ptr->origin_y >> MI_SIZE_LOG2], sizeof(xd->left_seg_context));
        }

        // update context for the rate of sb
        void update_sb_rate_context(
            PictureControlSet *picture_control_set_ptr,
            EncDecContext     *context_ptr,
            MACROBLOCKD       *xd) {

            VP9_COMP    *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
            VP9_COMMON  *const cm = &cpi->common;

            for (int i = 0; i < MAX_MB_PLANE; ++i) {
                EB_MEMCPY(&context_ptr->left_context[(context_ptr->sb_ptr->origin_y >> 2) + (i * sizeof(*context_ptr->left_context) * 2 * mi_cols_aligned_to_sb(cm->mi_rows))], &xd->left_context[i], 16);
            }
            EB_MEMCPY(&context_ptr->left_seg_context[context_ptr->sb_ptr->origin_y >> MI_SIZE_LOG2], &xd->left_seg_context, sizeof(xd->left_seg_context));

        }

        // update the context rate for BDP pilar and refinment stages
        void update_bdp_sb_rate_context(
            PictureControlSet *picture_control_set_ptr,
            EncDecContext     *context_ptr,
            MACROBLOCKD       *xd,
            int                depth_part_stage)
        {

            VP9_COMP    *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
            VP9_COMMON  *const cm = &cpi->common;

            for (int i = 0; i < MAX_MB_PLANE; ++i) {
                EB_MEMCPY(&picture_control_set_ptr->md_left_context[depth_part_stage][(context_ptr->sb_ptr->origin_y >> 2) + (i * sizeof(*picture_control_set_ptr->md_left_context[depth_part_stage]) * 2 * mi_cols_aligned_to_sb(cm->mi_rows))], &xd->left_context[i], 16);
            }
            EB_MEMCPY(&picture_control_set_ptr->md_left_seg_context[depth_part_stage][context_ptr->sb_ptr->origin_y >> MI_SIZE_LOG2], &xd->left_seg_context, sizeof(xd->left_seg_context));

            for (int i = 0; i < MAX_MB_PLANE; ++i) {
                struct macroblockd_plane *const pd = &xd->plane[i];
                EB_MEMCPY(&picture_control_set_ptr->md_above_context[depth_part_stage][((context_ptr->sb_ptr->origin_x >> 2) >> pd->subsampling_x) + (i * sizeof(*picture_control_set_ptr->md_above_context[depth_part_stage]) * 2 * mi_cols_aligned_to_sb(cm->mi_cols))], &xd->above_context[i][(context_ptr->sb_ptr->origin_x >> 2) >> pd->subsampling_x], 16);
            }

            EB_MEMCPY(&picture_control_set_ptr->md_above_seg_context[depth_part_stage][context_ptr->sb_ptr->origin_x >> MI_SIZE_LOG2], &xd->above_seg_context[context_ptr->sb_ptr->origin_x >> MI_SIZE_LOG2], 8 * sizeof(*xd->above_seg_context));
        }

        // Set context for the rate of cus
        void set_block_rate_context(
            PictureControlSet *picture_control_set_ptr,
            EncDecContext     *context_ptr,
            MACROBLOCKD       *xd)
        {

            // Set skip context
            set_skip_context(xd, context_ptr->mi_row, context_ptr->mi_col);

            estimate_ref_frame_costs(
                &picture_control_set_ptr->parent_pcs_ptr->cpi->common,
                context_ptr->e_mbd,
#if SEG_SUPPORT
                context_ptr->segment_id,
#else
                0,
#endif
                context_ptr->ref_costs_single,
                context_ptr->ref_costs_comp,
                &context_ptr->comp_mode_p);

            eb_vp9_get_entropy_contexts(
                context_ptr->ep_block_stats_ptr->bsize,
                context_ptr->ep_block_stats_ptr->tx_size,
                &context_ptr->e_mbd->plane[0],
                context_ptr->t_above[0],
                context_ptr->t_left[0]);

            eb_vp9_get_entropy_contexts(
                context_ptr->ep_block_stats_ptr->bsize,
                context_ptr->ep_block_stats_ptr->tx_size_uv,
                &context_ptr->e_mbd->plane[1],
                context_ptr->t_above[1],
                context_ptr->t_left[1]);

            eb_vp9_get_entropy_contexts(
                context_ptr->ep_block_stats_ptr->bsize,
                context_ptr->ep_block_stats_ptr->tx_size_uv,
                &context_ptr->e_mbd->plane[2],
                context_ptr->t_above[2],
                context_ptr->t_left[2]);

        }
        // Update context for the rate of cus
        void update_block_rate_context(
            EncDecContext *context_ptr,
            MACROBLOCKD   *xd) {

            struct macroblockd_plane *pd = &xd->plane[0];
            // Set skip context. point to the right location
            set_skip_context(xd, context_ptr->mi_row, context_ptr->mi_col);

            if (context_ptr->ep_block_stats_ptr->bsize < BLOCK_8X8) {
                eb_vp9_set_contexts(xd,
                    pd,
                    context_ptr->ep_block_stats_ptr->bsize,
                    context_ptr->ep_block_stats_ptr->tx_size,
                    context_ptr->block_ptr->eob[0][0] > 0,
                    context_ptr->bmi_index & 0x1,
                    context_ptr->bmi_index >> 1);
            }
            else {
                for (uint8_t tu_index = 0; tu_index < ((context_ptr->ep_block_stats_ptr->sq_size == MAX_SB_SIZE) ? 4 : 1); tu_index++) {
                    int blk_col = (tu_index & 0x1)*(1 << context_ptr->ep_block_stats_ptr->tx_size);
                    int blk_row = (tu_index >> 1)*(1 << context_ptr->ep_block_stats_ptr->tx_size);

                    eb_vp9_set_contexts(xd,
                        pd,
                        context_ptr->ep_block_stats_ptr->bsize,
                        context_ptr->ep_block_stats_ptr->tx_size,
                        context_ptr->block_ptr->eob[0][tu_index] > 0,
                        blk_col,
                        blk_row);
                }
            }
            if (context_ptr->ep_block_stats_ptr->has_uv) {
                struct macroblockd_plane *pd = &xd->plane[1];
                // Set skip context. point to the right location
                set_skip_context(xd, context_ptr->mi_row, context_ptr->mi_col);
                eb_vp9_set_contexts(xd,
                    pd,
                    context_ptr->ep_block_stats_ptr->bsize,
                    context_ptr->ep_block_stats_ptr->tx_size_uv,
                    context_ptr->block_ptr->eob[1][0] > 0,
                    0,
                    0);

                pd = &xd->plane[2];
                eb_vp9_set_contexts(xd,
                    pd,
                    context_ptr->ep_block_stats_ptr->bsize,
                    context_ptr->ep_block_stats_ptr->tx_size_uv,
                    context_ptr->block_ptr->eob[2][0] > 0,
                    0,
                    0);
            }

            // Update partition context
            PARTITION_TYPE partition = context_ptr->block_ptr->split_flag ? PARTITION_SPLIT : PARTITION_NONE;
            BLOCK_SIZE subsize = get_subsize(context_ptr->ep_block_stats_ptr->bsize, partition);
            if (context_ptr->ep_block_stats_ptr->bsize >= BLOCK_8X8 &&
                (context_ptr->ep_block_stats_ptr->bsize == BLOCK_8X8 || partition != PARTITION_SPLIT))
                update_partition_context(xd, context_ptr->mi_row, context_ptr->mi_col, subsize, context_ptr->ep_block_stats_ptr->bsize);

        }

        void update_scratch_recon_buffer(
            EncDecContext       *context_ptr,
            EbPictureBufferDesc *recon_src_ptr,
            EbPictureBufferDesc *recon_dst_ptr,
            SbUnit              *sb_ptr)
        {
            (void)sb_ptr;
            if ((context_ptr->ep_block_stats_ptr->sq_size >> 3) < 9) {
                pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][context_ptr->ep_block_stats_ptr->sq_size >> 3](
                    &(recon_src_ptr->buffer_y[context_ptr->ep_block_stats_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_y * recon_src_ptr->stride_y]),
                    recon_src_ptr->stride_y,
                    &(recon_dst_ptr->buffer_y[context_ptr->ep_block_stats_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_y * recon_dst_ptr->stride_y]),
                    recon_dst_ptr->stride_y,
                    context_ptr->ep_block_stats_ptr->sq_size,
                    context_ptr->ep_block_stats_ptr->sq_size);

                if ((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) {
                    uint16_t  chromaorigin_x = context_ptr->ep_block_stats_ptr->origin_x >> 1;
                    uint16_t  chromaorigin_y = context_ptr->ep_block_stats_ptr->origin_y >> 1;

                    pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][context_ptr->ep_block_stats_ptr->sq_size_uv >> 3](
                        &(recon_src_ptr->buffer_cb[chromaorigin_x + chromaorigin_y * recon_src_ptr->stride_cb]),
                        recon_src_ptr->stride_cb,
                        &(recon_dst_ptr->buffer_cb[chromaorigin_x + chromaorigin_y * recon_dst_ptr->stride_cb]),
                        recon_dst_ptr->stride_cb,
                        context_ptr->ep_block_stats_ptr->sq_size_uv,
                        context_ptr->ep_block_stats_ptr->sq_size_uv);

                    pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][context_ptr->ep_block_stats_ptr->sq_size_uv >> 3](
                        &(recon_src_ptr->buffer_cr[chromaorigin_x + chromaorigin_y * recon_src_ptr->stride_cr]),
                        recon_src_ptr->stride_cr,
                        &(recon_dst_ptr->buffer_cr[chromaorigin_x + chromaorigin_y * recon_dst_ptr->stride_cr]),
                        recon_dst_ptr->stride_cr,
                        context_ptr->ep_block_stats_ptr->sq_size_uv,
                        context_ptr->ep_block_stats_ptr->sq_size_uv);
                }
            }
        }

        EB_BOOL check_skip_sub_blocks(
            SequenceControlSet *sequence_control_set_ptr,
            PictureControlSet  *picture_control_set_ptr,
            EncDecContext      *context_ptr,
            SbUnit             *sb_ptr)
        {

            SbParams *sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_ptr->sb_index];

            // Only if complete SB, if both neighbors are available, and if current block will be further splitted
            if (sb_params_ptr->is_complete_sb && context_ptr->e_mbd->left_mi != NULL && context_ptr->e_mbd->above_mi != NULL && context_ptr->block_ptr->split_flag == EB_TRUE) {
                // Only for open loop depth partitioning cases (SB_LIGHT_AVC_DEPTH_MODE, SB_PRED_OPEN_LOOP_DEPTH_MOD, SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE are not considered as already 1 partitioning scheme
                if ((picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_OPEN_LOOP_DEPTH_MODE ||
                    (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_SB_SWITCH_DEPTH_MODE &&
                    (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_ptr->sb_index] == SB_OPEN_LOOP_DEPTH_MODE ||
                        picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_ptr->sb_index] == SB_LIGHT_OPEN_LOOP_DEPTH_MODE ||
                        picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_ptr->sb_index] == SB_AVC_DEPTH_MODE)))) {
                    if (picture_control_set_ptr->sb_ptr_array[sb_ptr->sb_index]->aura_status == AURA_STATUS_0 && (picture_control_set_ptr->parent_pcs_ptr->sb_stat_array[sb_ptr->sb_index]).stationary_edge_over_time_flag == 0)
                    {
                        if ((context_ptr->e_mbd->left_mi->sb_type == context_ptr->ep_block_stats_ptr->bsize) && (context_ptr->e_mbd->above_mi->sb_type == context_ptr->ep_block_stats_ptr->bsize))
                        {
                            context_ptr->block_ptr->split_flag = EB_FALSE;
                            return(EB_TRUE);
                        }
                    }
                }
            }
            return(EB_FALSE);
        }

        void next_mdc_block_index(
            MdcSbData *mdc_sb_data_ptr,
            EB_BOOL    skip_sub_blocks,
            uint16_t  *mdc_block_index)
        {

            PartBlockData *part_block_data_ptr = &mdc_sb_data_ptr->block_data_array[*mdc_block_index];

            if (skip_sub_blocks == EB_TRUE) {

                uint16_t origin_x = ep_get_block_stats(part_block_data_ptr->block_index)->origin_x;
                uint16_t origin_y = ep_get_block_stats(part_block_data_ptr->block_index)->origin_y;
                const int size = ep_get_block_stats(part_block_data_ptr->block_index)->sq_size;

                while (*mdc_block_index < mdc_sb_data_ptr->block_count &&
                    (ep_get_block_stats((&mdc_sb_data_ptr->block_data_array[*mdc_block_index])->block_index)->origin_x < origin_x + size) &&
                    (ep_get_block_stats((&mdc_sb_data_ptr->block_data_array[*mdc_block_index])->block_index)->origin_y < origin_y + size)) {

                    (*mdc_block_index)++;
                }
            }
            else {
                (*mdc_block_index)++;
            }
        }

        void search_uv_mode(
            PictureControlSet   *picture_control_set_ptr,
            EbPictureBufferDesc *input_picture_ptr,
            EncDecContext       *context_ptr)
        {

            QUANTS *quants = &picture_control_set_ptr->parent_pcs_ptr->cpi->quants;
#if SEG_SUPPORT
            VP9_COMMON *const cm = &cpi->common;
            struct segmentation *const seg = &cm->seg;
            const int qindex = eb_vp9_get_qindex(seg, candidate_ptr->mode_info->segment_id, picture_control_set_ptr->base_qindex);

#else
            const int qindex = picture_control_set_ptr->base_qindex;
#endif

            context_ptr->uv_mode_search_eob[1][0] = 1;
            context_ptr->uv_mode_search_eob[2][0] = 1;

            PREDICTION_MODE mode;
            PREDICTION_MODE uv_mode;
            PREDICTION_MODE best_uv_mode = DC_PRED;

            int rate;
            int coeff_rate[TM_PRED + 1];
            int distortion[TM_PRED + 1];

            for (uv_mode = DC_PRED; uv_mode <= TM_PRED; uv_mode++) {

                int cb_coeff_bits = 0;
                int cr_coeff_bits = 0;
                int cbfull_distortion[DIST_CALC_TOTAL] = { 0,0 };
                int crfull_distortion[DIST_CALC_TOTAL] = { 0,0 };
                context_ptr->uv_mode_search_eob[1][0] = 1;
                context_ptr->uv_mode_search_eob[2][0] = 1;

                // Set the WebM mi
                context_ptr->uv_mode_search_mode_info->uv_mode = uv_mode;
                context_ptr->uv_mode_search_mode_info->sb_type = context_ptr->ep_block_stats_ptr->bsize;
#if SEG_SUPPORT // Hsan: segmentation not supported
                context_ptr->uv_mode_search_mode_info->segment_id = context_ptr->segment_id;
#endif
                context_ptr->uv_mode_search_mode_info->ref_frame[0] = INTRA_FRAME;
                context_ptr->uv_mode_search_mode_info->ref_frame[1] = INTRA_FRAME;

                context_ptr->e_mbd->mi[0] = context_ptr->uv_mode_search_mode_info;

                // Cb Coding Loop
                prediction_fun_table[1](
                    context_ptr,
                    context_ptr->uv_mode_search_prediction_buffer->buffer_cb + context_ptr->block_chroma_origin_index,
                    context_ptr->uv_mode_search_prediction_buffer->stride_cb,
                    1);

                perform_coding_loop(
                    context_ptr,
                    &(((int16_t*)context_ptr->uv_mode_search_residual_quant_coeff_buffer->buffer_cb)[0]),
                    (uint16_t)tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv],
                    &(input_picture_ptr->buffer_cb[context_ptr->input_chroma_origin_index]),
                    input_picture_ptr->stride_cb,
                    &(context_ptr->uv_mode_search_prediction_buffer->buffer_cb[context_ptr->block_chroma_origin_index]),
                    context_ptr->uv_mode_search_prediction_buffer->stride_cb,
                    &(((int16_t*)context_ptr->trans_quant_buffers_ptr->tu_trans_coeff2_nx2_n_ptr->buffer_cb)[0]),             // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                    &(((int16_t*)context_ptr->uv_mode_search_recon_coeff_buffer->buffer_cb)[0]),                          // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                    &(context_ptr->uv_mode_search_recon_buffer->buffer_cb[context_ptr->block_chroma_origin_index]),
                    context_ptr->uv_mode_search_recon_buffer->stride_cb,
                    quants->uv_zbin[qindex],
                    quants->uv_round[qindex],
                    quants->uv_quant[qindex],
                    quants->uv_quant_shift[qindex],
                    &picture_control_set_ptr->parent_pcs_ptr->cpi->uv_dequant[qindex][0],
                    &context_ptr->uv_mode_search_eob[1][0],
                    context_ptr->ep_block_stats_ptr->tx_size_uv,
                    1,
                    0,
                    0);

                // Cb Distortion and Rate Calculation
                perform_dist_rate_calc(
                    context_ptr,
                    picture_control_set_ptr,
                    &(((int16_t*)context_ptr->uv_mode_search_residual_quant_coeff_buffer->buffer_cb)[0]),
                    &(input_picture_ptr->buffer_cb[context_ptr->input_chroma_origin_index]),
                    input_picture_ptr->stride_cb,
                    &(context_ptr->uv_mode_search_prediction_buffer->buffer_cb[context_ptr->block_chroma_origin_index]),
                    context_ptr->uv_mode_search_prediction_buffer->stride_cb,
                    &(((int16_t*)context_ptr->trans_quant_buffers_ptr->tu_trans_coeff2_nx2_n_ptr->buffer_cb)[0]),
                    &(((int16_t*)context_ptr->uv_mode_search_recon_coeff_buffer->buffer_cb)[0]),
                    &(context_ptr->uv_mode_search_recon_buffer->buffer_cb[context_ptr->block_chroma_origin_index]),
                    context_ptr->uv_mode_search_recon_buffer->stride_cb,
                    &context_ptr->uv_mode_search_eob[1][0],
                    context_ptr->ep_block_stats_ptr->tx_size_uv,
                    1,
                    0,
                    uv_mode,
                    &cbfull_distortion[0],
                    &cb_coeff_bits);

                // Cr Coding Loop
                prediction_fun_table[1](
                    context_ptr,
                    context_ptr->uv_mode_search_prediction_buffer->buffer_cr + context_ptr->block_chroma_origin_index,
                    context_ptr->uv_mode_search_prediction_buffer->stride_cr,
                    2);

                perform_coding_loop(
                    context_ptr,
                    &(((int16_t*)context_ptr->uv_mode_search_residual_quant_coeff_buffer->buffer_cr)[0]),
                    (uint16_t)tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv],
                    &(input_picture_ptr->buffer_cr[context_ptr->input_chroma_origin_index]),
                    input_picture_ptr->stride_cr,
                    &(context_ptr->uv_mode_search_prediction_buffer->buffer_cr[context_ptr->block_chroma_origin_index]),
                    context_ptr->uv_mode_search_prediction_buffer->stride_cr,
                    &(((int16_t*)context_ptr->trans_quant_buffers_ptr->tu_trans_coeff2_nx2_n_ptr->buffer_cr)[0]),  // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                    &(((int16_t*)context_ptr->uv_mode_search_recon_coeff_buffer->buffer_cr)[0]),                          // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                    &(context_ptr->uv_mode_search_recon_buffer->buffer_cr[context_ptr->block_chroma_origin_index]),
                    context_ptr->uv_mode_search_recon_buffer->stride_cr,
                    quants->uv_zbin[qindex],
                    quants->uv_round[qindex],
                    quants->uv_quant[qindex],
                    quants->uv_quant_shift[qindex],
                    &picture_control_set_ptr->parent_pcs_ptr->cpi->uv_dequant[qindex][0],
                    &context_ptr->uv_mode_search_eob[2][0],
                    context_ptr->ep_block_stats_ptr->tx_size_uv,
                    2,
                    0,
                    0);

                // Cr Distortion and Rate Calculation
                perform_dist_rate_calc(
                    context_ptr,
                    picture_control_set_ptr,
                    &(((int16_t*)context_ptr->uv_mode_search_residual_quant_coeff_buffer->buffer_cr)[0]),
                    &(input_picture_ptr->buffer_cr[context_ptr->input_chroma_origin_index]),
                    input_picture_ptr->stride_cr,
                    &(context_ptr->uv_mode_search_prediction_buffer->buffer_cr[context_ptr->block_chroma_origin_index]),
                    context_ptr->uv_mode_search_prediction_buffer->stride_cr,
                    &(((int16_t*)context_ptr->trans_quant_buffers_ptr->tu_trans_coeff2_nx2_n_ptr->buffer_cr)[0]),
                    &(((int16_t*)context_ptr->uv_mode_search_recon_coeff_buffer->buffer_cr)[0]),
                    &(context_ptr->uv_mode_search_recon_buffer->buffer_cr[context_ptr->block_chroma_origin_index]),
                    context_ptr->uv_mode_search_recon_buffer->stride_cr,
                    &context_ptr->uv_mode_search_eob[2][0],
                    context_ptr->ep_block_stats_ptr->tx_size_uv,
                    2,
                    0,
                    best_uv_mode,
                    &crfull_distortion[0],
                    &cr_coeff_bits);

#if 1
                coeff_rate[uv_mode] = cb_coeff_bits + cr_coeff_bits;
                distortion[uv_mode] = cbfull_distortion[DIST_CALC_RESIDUAL] + crfull_distortion[DIST_CALC_RESIDUAL];
#else

                if (candidate_buffer_ptr->candidate_ptr->mode_info->skip == EB_TRUE) {
                    rate = (int)candidate_buffer_ptr->candidate_ptr->fast_luma_rate + (int)candidate_buffer_ptr->candidate_ptr->fast_chroma_rate + vp9_cost_bit(vp9_get_skip_prob(&cpi->common, xd), 1);
                    distortion = y_distortion[DIST_CALC_PREDICTION] + cb_distortion[DIST_CALC_PREDICTION] + cr_distortion[DIST_CALC_PREDICTION];
                }
                else {
                    rate = *y_coeff_bits + *cb_coeff_bits + *cr_coeff_bits + (int)candidate_buffer_ptr->candidate_ptr->fast_luma_rate + (int)candidate_buffer_ptr->candidate_ptr->fast_chroma_rate + vp9_cost_bit(vp9_get_skip_prob(&cpi->common, xd), 0);
                    distortion = y_distortion[DIST_CALC_RESIDUAL] + cb_distortion[DIST_CALC_RESIDUAL] + cr_distortion[DIST_CALC_RESIDUAL];
                }

                // The cost of Split flag for 8x8 to be added here. It is assumed that 8x8 is not splited. If 4x4 is selected later, the cost of 8x8 will be replaced by 4x4 with split flag =1
                if (context_ptr->ep_block_stats_ptr->bsize == BLOCK_8X8) {
                    rate += cpi->partition_cost[context_ptr->block_ptr->partition_context][PARTITION_NONE];
                }

                *candidate_buffer_ptr->full_cost_ptr = RDCOST(context_ptr->RDMULT, rd->RDDIV, rate, distortion);
#endif

            }

            //
            uint64_t uv_cost;
            uint64_t best_uv_mode_cost;
            VP9_COMP   *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
            RD_OPT *const rd = &cpi->rd;

            for (mode = DC_PRED; mode <= TM_PRED; mode++) {
                best_uv_mode_cost = MAX_BLOCK_COST;
                for (uv_mode = DC_PRED; uv_mode <= TM_PRED; uv_mode++) {
                    int rate_uv_mode = cpi->intra_uv_mode_cost[cpi->common.frame_type][mode][uv_mode];
                    rate = coeff_rate[uv_mode] + rate_uv_mode;
                    uv_cost = RDCOST(context_ptr->RDMULT, rd->RDDIV, rate, distortion[uv_mode]);

                    if (uv_cost < best_uv_mode_cost) {
                        context_ptr->best_uv_mode[mode] = uv_mode;
                        best_uv_mode_cost = uv_cost;
                    }
                }
            }
        }

        void eb_vp9_mode_decision_sb(
            SequenceControlSet *sequence_control_set_ptr,
            PictureControlSet   *picture_control_set_ptr,
            EncDecContext       *context_ptr,
            SbUnit              *sb_ptr)
        {

            EbPictureBufferDesc          *input_picture_ptr = picture_control_set_ptr->parent_pcs_ptr->enhanced_picture_ptr;
            uint32_t                      buffer_total_count;
            ModeDecisionCandidateBuffer  *candidate_buffer;
            ModeDecisionCandidateBuffer  *best_candidate_buffers[EB_MAX_SB_DEPTH];
            uint8_t                       candidate_index;
            uint32_t                      fast_candidate_total_count;
            uint32_t                      full_candidate_total_count;
            uint32_t                      second_fast_candidate_total_count;
            uint32_t                      last_block_index;
            MdcSbData                    *mdc_sb_data_ptr = &picture_control_set_ptr->mdc_sb_data_array[sb_ptr->sb_index];
            ModeDecisionCandidate        *fast_candidate_array = context_ptr->fast_candidate_array;
            ModeDecisionCandidateBuffer **candidate_buffer_ptr_array_base = context_ptr->candidate_buffer_ptr_array;
            ModeDecisionCandidateBuffer **candidate_buffer_ptr_array;
            uint32_t                      max_buffers;
            PartBlockData                *part_block_data_ptr;

            // Keep track of the LCU Ptr
            context_ptr->sb_ptr = sb_ptr;

            // Mode Decision Neighbor Arrays
            context_ptr->luma_recon_neighbor_array = picture_control_set_ptr->md_luma_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->cb_recon_neighbor_array = picture_control_set_ptr->md_cb_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->cr_recon_neighbor_array = picture_control_set_ptr->md_cr_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->above_seg_context = picture_control_set_ptr->md_above_seg_context[context_ptr->depth_part_stage];
            context_ptr->left_seg_context = picture_control_set_ptr->md_left_seg_context[context_ptr->depth_part_stage];
            context_ptr->above_context = picture_control_set_ptr->md_above_context[context_ptr->depth_part_stage];
            context_ptr->left_context = picture_control_set_ptr->md_left_context[context_ptr->depth_part_stage];

            // Mode Info Array
            context_ptr->mode_info_array = picture_control_set_ptr->mode_info_array;
#if VP9_RD
            VP9_COMP    *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
            VP9_COMMON  *const cm = &cpi->common;
            MACROBLOCKD *const xd = context_ptr->e_mbd;

            set_sb_rate_context(
                picture_control_set_ptr,
                context_ptr,
                xd);
#endif

            uint16_t mdc_block_index = 0;
#if 0 // Hsan - to do
            uint16_t pa_block_index = 0;
#endif
            uint16_t ep_block_index = 0;
            EB_BOOL skip_sub_blocks;

            while (ep_block_index < EP_BLOCK_MAX_COUNT) {
                if (ep_get_block_stats(ep_block_index)->shape == PART_N) {

                    skip_sub_blocks = EB_FALSE;

                    context_ptr->block_ptr = sb_ptr->coded_block_array_ptr[ep_block_index];
                    context_ptr->ep_block_index = ep_block_index;
                    context_ptr->ep_block_stats_ptr = ep_get_block_stats(ep_block_index);
                    context_ptr->block_origin_x = (uint16_t)sb_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_x;
                    context_ptr->block_origin_y = (uint16_t)sb_ptr->origin_y + context_ptr->ep_block_stats_ptr->origin_y;
                    context_ptr->mi_col = context_ptr->block_origin_x >> MI_SIZE_LOG2;
                    context_ptr->mi_row = context_ptr->block_origin_y >> MI_SIZE_LOG2;
                    context_ptr->mi_stride = input_picture_ptr->width >> MI_SIZE_LOG2;
                    context_ptr->bmi_index = ((context_ptr->block_origin_x >> 2) & 0x1) + (((context_ptr->block_origin_y >> 2) & 0x1) << 1);
                    context_ptr->enc_dec_local_block_array[ep_block_index]->cost = MAX_BLOCK_COST;
                    context_ptr->block_ptr->partition_context = partition_plane_context(xd, context_ptr->mi_row, context_ptr->mi_col, context_ptr->ep_block_stats_ptr->bsize);

                    part_block_data_ptr = &mdc_sb_data_ptr->block_data_array[mdc_block_index];

                    if (mdc_block_index < mdc_sb_data_ptr->block_count && ep_block_index == part_block_data_ptr->block_index) {

                        context_ptr->block_ptr->split_flag = part_block_data_ptr->split_flag;

                        context_ptr->input_origin_index = (context_ptr->block_origin_y + input_picture_ptr->origin_y) * input_picture_ptr->stride_y + (context_ptr->block_origin_x + input_picture_ptr->origin_x);
                        context_ptr->input_chroma_origin_index = ((ROUND_UV(context_ptr->block_origin_y) >> 1) + (input_picture_ptr->origin_y >> 1)) * input_picture_ptr->stride_cb + ((ROUND_UV(context_ptr->block_origin_x) >> 1) + (input_picture_ptr->origin_x >> 1));
                        context_ptr->block_origin_index = context_ptr->ep_block_stats_ptr->origin_y * MAX_SB_SIZE + context_ptr->ep_block_stats_ptr->origin_x;
                        context_ptr->block_chroma_origin_index = (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) >> 1) * (MAX_SB_SIZE >> 1) + (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x) >> 1);

                        context_ptr->e_mbd->mb_to_top_edge = -((context_ptr->mi_row * MI_SIZE) * 8);
                        context_ptr->e_mbd->mb_to_bottom_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_rows - eb_vp9_num_8x8_blocks_high_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_row) * MI_SIZE) * 8;
                        context_ptr->e_mbd->mb_to_left_edge = -((context_ptr->mi_col * MI_SIZE) * 8);
                        context_ptr->e_mbd->mb_to_right_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_cols - eb_vp9_num_8x8_blocks_wide_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_col) * MI_SIZE) * 8;

                        // Set above_mi and left_mi
                        context_ptr->e_mbd->above_mi = (context_ptr->mi_row > 0) ? context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - cm->mi_stride] : NULL;
                        context_ptr->e_mbd->left_mi = (context_ptr->mi_col > 0) ? context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - 1] : NULL;

                        candidate_buffer_ptr_array = &(candidate_buffer_ptr_array_base[context_ptr->buffer_depth_index_start[context_ptr->ep_block_stats_ptr->depth]]);

#if SEG_SUPPORT
                        VP9_COMMON *const cm = &cpi->common;
                        struct segmentation *const seg = &cm->seg;
                        const int qindex = eb_vp9_get_qindex(seg, context_ptr->segment_id, picture_control_set_ptr->base_qindex);
#endif

                        // Initialize Fast Loop
                        coding_loop_init_fast_loop(
                            context_ptr);

                        // Skip sub blocks if the current block has the same depth as the left block and above block
                        skip_sub_blocks = check_skip_sub_blocks(
                            sequence_control_set_ptr,
                            picture_control_set_ptr,
                            context_ptr,
                            sb_ptr);

#if VP9_RD
                        // Set context for the rate of cus
                        set_block_rate_context(
                            picture_control_set_ptr,
                            context_ptr,
                            xd);

#endif
                        // Set the number full loop candidates
                        set_nfl(
                            picture_control_set_ptr,
                            context_ptr,
                            sb_ptr);

                        // Generate intra reference samples if intra present
                        if (context_ptr->ep_block_stats_ptr->sq_size < MAX_CU_SIZE) {
                            for (int plane = 0; plane < (int)(((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) ? MAX_MB_PLANE : 1); ++plane) {
                                generate_intra_reference_samples(
                                    sequence_control_set_ptr,
                                    context_ptr,
                                    plane);
                            }
                        }

                        // Search the best intra chroma mode
                        if (context_ptr->ep_block_stats_ptr->sq_size < MAX_CU_SIZE) {
                            if (context_ptr->ep_block_stats_ptr->bsize >= BLOCK_8X8) {
                                if (context_ptr->chroma_level == CHROMA_LEVEL_0) {
                                    search_uv_mode(
                                        picture_control_set_ptr,
                                        input_picture_ptr,
                                        context_ptr);
                                }
                            }
                        }

                        prepare_fast_loop_candidates(
                            picture_control_set_ptr,
                            context_ptr,
                            sb_ptr,
                            &buffer_total_count,
                            &fast_candidate_total_count);

                        // If we want to recon N candidate, we would need N+1 buffers
                        max_buffers = MIN((buffer_total_count + 1), context_ptr->buffer_depth_index_width[context_ptr->ep_block_stats_ptr->depth]);

                        perform_fast_loop(
                            picture_control_set_ptr,
                            context_ptr,
                            sb_ptr,
                            candidate_buffer_ptr_array_base,
                            fast_candidate_array,
                            fast_candidate_total_count,
                            input_picture_ptr,
                            max_buffers,
                            &second_fast_candidate_total_count);

                        // Make sure buffer_total_count is not larger than the number of fast modes
                        buffer_total_count = MIN(second_fast_candidate_total_count, buffer_total_count);

                        // pre_mode_decision
                        pre_mode_decision(
                            (second_fast_candidate_total_count == buffer_total_count) ? buffer_total_count : max_buffers,
                            candidate_buffer_ptr_array,
                            &full_candidate_total_count,
                            context_ptr->best_candidate_index_array,
                            (EB_BOOL)(second_fast_candidate_total_count == buffer_total_count));

                        perform_full_loop(
                            picture_control_set_ptr,
                            context_ptr,
                            sb_ptr,
                            input_picture_ptr,
                            MIN(full_candidate_total_count, buffer_total_count));

                        // Full Mode Decision (choose the best mode)
                        candidate_index = full_mode_decision(
                            context_ptr,
                            candidate_buffer_ptr_array,
                            full_candidate_total_count,
                            context_ptr->best_candidate_index_array,
                            ep_block_index,
                            MDC_STAGE);

                        candidate_buffer = candidate_buffer_ptr_array[candidate_index];

                        best_candidate_buffers[context_ptr->ep_block_stats_ptr->depth] = candidate_buffer;

                        if (context_ptr->intra_md_open_loop_flag == EB_FALSE) {
                            if (context_ptr->spatial_sse_full_loop == EB_FALSE) {
                                context_ptr->e_mbd->mi[0] = &(context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info);
                                perform_inverse_transform_recon(
                                    context_ptr,
                                    candidate_buffer);
                            }
                        }

                        // Inter Depth Decision
                        last_block_index = inter_depth_decision(
                            picture_control_set_ptr,
                            sb_ptr,
                            context_ptr,
                            context_ptr->ep_block_stats_ptr->sqi_mds);

                        if (sb_ptr->coded_block_array_ptr[last_block_index]->split_flag == EB_FALSE)
                        {
                            // Get the settings of the best partition
                            context_ptr->block_ptr = sb_ptr->coded_block_array_ptr[last_block_index];
                            context_ptr->ep_block_index = last_block_index;
                            context_ptr->ep_block_stats_ptr = ep_get_block_stats(last_block_index);
                            context_ptr->block_origin_x = (uint16_t)sb_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_x;
                            context_ptr->block_origin_y = (uint16_t)sb_ptr->origin_y + context_ptr->ep_block_stats_ptr->origin_y;
                            context_ptr->mi_col = context_ptr->block_origin_x >> MI_SIZE_LOG2;
                            context_ptr->mi_row = context_ptr->block_origin_y >> MI_SIZE_LOG2;

                            // Get the settings of the best candidate
                            candidate_buffer = best_candidate_buffers[context_ptr->ep_block_stats_ptr->depth];

                            // Update Neighbor Arrays
                            {
                                // Update mode info array
                                if (context_ptr->ep_block_stats_ptr->bsize < BLOCK_8X8) {
                                    int mi_index = context_ptr->mi_col + context_ptr->mi_row * context_ptr->mi_stride;
                                    // Hsan: if last bmi (i.e. 4th bmi) then update the mode info
                                    context_ptr->mode_info_array[mi_index]->bmi[context_ptr->bmi_index].as_mode = context_ptr->enc_dec_local_block_array[last_block_index]->mode_info.bmi[context_ptr->bmi_index].as_mode;

                                    if (context_ptr->ep_block_stats_ptr->is_last_quadrant) {
                                        context_ptr->mode_info_array[mi_index]->sb_type = context_ptr->enc_dec_local_block_array[last_block_index]->mode_info.sb_type;
                                        context_ptr->mode_info_array[mi_index]->mode = context_ptr->enc_dec_local_block_array[last_block_index]->mode_info.mode;
                                        context_ptr->mode_info_array[mi_index]->tx_size = context_ptr->enc_dec_local_block_array[last_block_index]->mode_info.tx_size;
#if SEG_SUPPORT // Hsan: segmentation not supported
                                        context_ptr->mode_info_array[mi_index]->segment_id = context_ptr->enc_dec_local_block_array[last_block_index]->mode_info.segment_id;
#endif
                                        context_ptr->mode_info_array[mi_index]->uv_mode = context_ptr->enc_dec_local_block_array[last_block_index]->mode_info.uv_mode;
                                        context_ptr->mode_info_array[mi_index]->ref_frame[0] = context_ptr->enc_dec_local_block_array[last_block_index]->mode_info.ref_frame[0];
                                        context_ptr->mode_info_array[mi_index]->ref_frame[1] = context_ptr->enc_dec_local_block_array[last_block_index]->mode_info.ref_frame[1];

                                        // Derive skip flag
                                        context_ptr->mode_info_array[mi_index]->skip =
                                            context_ptr->enc_dec_local_block_array[last_block_index - 3]->mode_info.skip == EB_TRUE &&
                                            context_ptr->enc_dec_local_block_array[last_block_index - 2]->mode_info.skip == EB_TRUE &&
                                            context_ptr->enc_dec_local_block_array[last_block_index - 1]->mode_info.skip == EB_TRUE &&
                                            context_ptr->enc_dec_local_block_array[last_block_index]->mode_info.skip == EB_TRUE;

                                    }

                                }
                                else {
                                    int mi_row_index;
                                    int mi_col_index;
                                    for (mi_row_index = context_ptr->mi_row; mi_row_index < context_ptr->mi_row + ((int)context_ptr->ep_block_stats_ptr->bheight >> MI_SIZE_LOG2); mi_row_index++) {
                                        for (mi_col_index = context_ptr->mi_col; mi_col_index < context_ptr->mi_col + ((int)context_ptr->ep_block_stats_ptr->bwidth >> MI_SIZE_LOG2); mi_col_index++) {
                                            EB_MEMCPY(context_ptr->mode_info_array[mi_col_index + mi_row_index * context_ptr->mi_stride], &(context_ptr->enc_dec_local_block_array[last_block_index]->mode_info), sizeof(ModeInfo));
                                        }
                                    }
                                }

                                // Update neighbor sample arrays
                                update_recon_neighbor_arrays(
                                    input_picture_ptr,
                                    context_ptr,
                                    sb_ptr,
                                    candidate_buffer,
                                    0);

                                // Hsan: check the lossless optimization that consists of copying the recon samples to an SB sized scratch buffer here, then copy from the scratch buffer to the bdp pillar neighbor sample arrays outside
                                // Update bdp neighbor sample arrays
                                if (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_SB_SWITCH_DEPTH_MODE && picture_control_set_ptr->bdp_present_flag) {

                                    // Update bdp pillar neighbor sample arrays
                                    update_bdp_recon_neighbor_arrays(
                                        picture_control_set_ptr,
                                        input_picture_ptr,
                                        context_ptr,
                                        sb_ptr,
                                        candidate_buffer,
                                        1);

                                    // Update bdp refinement neighbor sample arrays
                                    update_bdp_recon_neighbor_arrays(
                                        picture_control_set_ptr,
                                        input_picture_ptr,
                                        context_ptr,
                                        sb_ptr,
                                        candidate_buffer,
                                        2);
                                }

#if VP9_RD
                                // Update context for the rate of cus
                                update_block_rate_context(
                                    context_ptr,
                                    xd);
#endif

#if !VP9_PERFORM_EP
                                // copy coeff
                                {

                                    int j;

                                    //Y
                                    {
                                        int16_t* src_ptr = &(((int16_t*)candidate_buffer->residual_quant_coeff_ptr->buffer_y)[0]); // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                                        int16_t* dst_ptr = &(((int16_t*)context_ptr->sb_ptr->quantized_coeff->buffer_y)[context_ptr->ep_block_stats_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_y * context_ptr->sb_ptr->quantized_coeff->stride_y]);

                                        for (j = 0; j < context_ptr->ep_block_stats_ptr->sq_size; j++) {
                                            EB_MEMCPY(dst_ptr, src_ptr, context_ptr->ep_block_stats_ptr->sq_size * sizeof(int16_t));
                                            src_ptr = src_ptr + context_ptr->ep_block_stats_ptr->sq_size;
                                            dst_ptr = dst_ptr + context_ptr->sb_ptr->quantized_coeff->stride_y;
                                        }
                                    }

                                    if (context_ptr->ep_block_stats_ptr->has_uv) {
                                        //Cb
                                        {
                                            int16_t* src_ptr = &(((int16_t*)candidate_buffer->residual_quant_coeff_ptr->buffer_cb)[0]); // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                                            int16_t* dst_ptr = &(((int16_t*)context_ptr->sb_ptr->quantized_coeff->buffer_cb)[(ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x) >> 1) + (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) >> 1) * context_ptr->sb_ptr->quantized_coeff->stride_cb]);

                                            for (j = 0; j < context_ptr->ep_block_stats_ptr->sq_size_uv; j++) {
                                                EB_MEMCPY(dst_ptr, src_ptr, context_ptr->ep_block_stats_ptr->sq_size_uv * sizeof(int16_t));
                                                src_ptr = src_ptr + context_ptr->ep_block_stats_ptr->sq_size_uv;
                                                dst_ptr = dst_ptr + context_ptr->sb_ptr->quantized_coeff->stride_cb;
                                            }
                                        }

                                        //Cr
                                        {
                                            int16_t* src_ptr = &(((int16_t*)candidate_buffer->residual_quant_coeff_ptr->buffer_cr)[0]); // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                                            int16_t* dst_ptr = &(((int16_t*)context_ptr->sb_ptr->quantized_coeff->buffer_cr)[(ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x) >> 1) + (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) >> 1) * context_ptr->sb_ptr->quantized_coeff->stride_cr]);

                                            for (j = 0; j < context_ptr->ep_block_stats_ptr->sq_size_uv; j++) {
                                                EB_MEMCPY(dst_ptr, src_ptr, context_ptr->ep_block_stats_ptr->sq_size_uv * sizeof(int16_t));
                                                src_ptr = src_ptr + context_ptr->ep_block_stats_ptr->sq_size_uv;
                                                dst_ptr = dst_ptr + context_ptr->sb_ptr->quantized_coeff->stride_cr;
                                            }
                                        }
                                    }
                                }

                                // copy recon
                                {
                                    EbPictureBufferDesc   *recon_buffer = context_ptr->recon_buffer;

                                    int j;
                                    uint32_t recLumaOffset = context_ptr->ep_block_stats_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_y * candidate_buffer->recon_ptr->stride_y;
                                    uint8_t* src_ptr = candidate_buffer->recon_ptr->buffer_y + recLumaOffset;
                                    uint8_t* dst_ptr = recon_buffer->buffer_y + recon_buffer->origin_x + context_ptr->block_origin_x + (recon_buffer->origin_y + context_ptr->block_origin_y)*recon_buffer->stride_y;

                                    for (j = 0; j < context_ptr->ep_block_stats_ptr->sq_size; j++) {
                                        EB_MEMCPY(dst_ptr, src_ptr, context_ptr->ep_block_stats_ptr->sq_size);
                                        src_ptr = src_ptr + candidate_buffer->recon_ptr->stride_y;
                                        dst_ptr = dst_ptr + recon_buffer->stride_y;
                                    }

                                    if (context_ptr->ep_block_stats_ptr->has_uv) {
                                        uint32_t recCbOffset = (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) * candidate_buffer->recon_ptr->stride_cb + ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x)) >> 1;
                                        uint32_t recCrOffset = (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) * candidate_buffer->recon_ptr->stride_cr + ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x)) >> 1;

                                        src_ptr = candidate_buffer->recon_ptr->buffer_cb + recCbOffset;
                                        dst_ptr = recon_buffer->buffer_cb + (recon_buffer->origin_x >> 1) + (ROUND_UV(context_ptr->block_origin_x) >> 1) + ((recon_buffer->origin_y >> 1) + (ROUND_UV(context_ptr->block_origin_y) >> 1))*recon_buffer->stride_cb;

                                        for (j = 0; j < context_ptr->ep_block_stats_ptr->sq_size_uv; j++) {
                                            EB_MEMCPY(dst_ptr, src_ptr, context_ptr->ep_block_stats_ptr->sq_size_uv);
                                            src_ptr = src_ptr + candidate_buffer->recon_ptr->stride_cb;
                                            dst_ptr = dst_ptr + recon_buffer->stride_cb;
                                        }

                                        src_ptr = candidate_buffer->recon_ptr->buffer_cr + recCrOffset;
                                        dst_ptr = recon_buffer->buffer_cr + (recon_buffer->origin_x >> 1) + (ROUND_UV(context_ptr->block_origin_x) >> 1) + ((recon_buffer->origin_y >> 1) + (ROUND_UV(context_ptr->block_origin_y) >> 1))*recon_buffer->stride_cr;

                                        for (j = 0; j < context_ptr->ep_block_stats_ptr->sq_size_uv; j++) {
                                            EB_MEMCPY(dst_ptr, src_ptr, context_ptr->ep_block_stats_ptr->sq_size_uv);
                                            src_ptr = src_ptr + candidate_buffer->recon_ptr->stride_cr;
                                            dst_ptr = dst_ptr + recon_buffer->stride_cr;
                                        }
                                    }
                                }
#endif
                            }
                        }

                        // Increment mdc_block_index
                        next_mdc_block_index(
                            mdc_sb_data_ptr,
                            skip_sub_blocks,
                            &mdc_block_index);

                    }
                    else {
                        context_ptr->block_ptr->split_flag = EB_TRUE;
                    }
                }
#if 0 // Hsan - to do
                pa_block_index++;
#else
                ep_block_index++;
#endif
            }

            update_sb_rate_context(
                picture_control_set_ptr,
                context_ptr,
                xd);

            // Hsan: add comments
            if (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_SB_SWITCH_DEPTH_MODE && picture_control_set_ptr->bdp_present_flag) {

                update_bdp_sb_rate_context(
                    picture_control_set_ptr,
                    context_ptr,
                    xd,
                    1);

                update_bdp_sb_rate_context(
                    picture_control_set_ptr,
                    context_ptr,
                    xd,
                    2);
            }
        }

        void construct_pillar_block_data_array(
            SequenceControlSet *sequence_control_set_ptr,
            EncDecContext      *context_ptr,
            SbUnit             *sb_ptr)
        {
            uint16_t block_index = 0;
            EB_BOOL split_flag;
            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_ptr->sb_index];

            context_ptr->bdp_block_data.block_count = 0;

            while (block_index < EP_BLOCK_MAX_COUNT) {

                split_flag = EB_TRUE;

                const EpBlockStats *ep_block_stats_ptr = ep_get_block_stats(block_index);
                uint8_t depth = ep_block_stats_ptr->depth;
                if (sb_params->ep_scan_block_validity[block_index] && ep_block_stats_ptr->shape == PART_N)
                {
                    if (depth == 0) {
                        split_flag = EB_TRUE;
                    }

                    else if (depth == 1) {
                        context_ptr->bdp_block_data.block_data_array[context_ptr->bdp_block_data.block_count].split_flag = split_flag = EB_TRUE;
                        context_ptr->bdp_block_data.block_data_array[context_ptr->bdp_block_data.block_count].block_index = block_index;
                        context_ptr->bdp_block_data.block_count++;
                    }
                    else if (depth == 2) {
                        context_ptr->bdp_block_data.block_data_array[context_ptr->bdp_block_data.block_count].split_flag = split_flag = EB_FALSE;
                        context_ptr->bdp_block_data.block_data_array[context_ptr->bdp_block_data.block_count].block_index = block_index;
                        context_ptr->bdp_block_data.block_count++;
                    }
                    else if (depth == 3) {
                        context_ptr->bdp_block_data.block_data_array[context_ptr->bdp_block_data.block_count].split_flag = split_flag = EB_FALSE;
                        context_ptr->bdp_block_data.block_data_array[context_ptr->bdp_block_data.block_count].block_index = block_index;
                        context_ptr->bdp_block_data.block_count++;
                    }
                }

                block_index += (split_flag == EB_FALSE) ? sq_depth_offset[depth] : nsq_depth_offset[depth];
            }
        }

        void bdp_pillar_sb(
            SequenceControlSet *sequence_control_set_ptr,
            PictureControlSet  *picture_control_set_ptr,
            EncDecContext      *context_ptr,
            SbUnit             *sb_ptr)
        {

            EbPictureBufferDesc          *input_picture_ptr = picture_control_set_ptr->parent_pcs_ptr->enhanced_picture_ptr;
            uint32_t                      buffer_total_count;
            ModeDecisionCandidateBuffer  *candidate_buffer;
            ModeDecisionCandidateBuffer  *best_candidate_buffers[EB_MAX_SB_DEPTH];
            uint8_t                       candidate_index;
            uint32_t                      fast_candidate_total_count;
            uint32_t                      full_candidate_total_count;
            uint32_t                      second_fast_candidate_total_count;
            uint32_t                      last_block_index;
            ModeDecisionCandidate        *fast_candidate_array = context_ptr->fast_candidate_array;
            ModeDecisionCandidateBuffer **candidate_buffer_ptr_array_base = context_ptr->candidate_buffer_ptr_array;
            ModeDecisionCandidateBuffer **candidate_buffer_ptr_array;
            uint32_t                      max_buffers;
            PartBlockData                *part_block_data_ptr;

            // Construct pillar block data array
            construct_pillar_block_data_array(
                sequence_control_set_ptr,
                context_ptr,
                sb_ptr);

            // Keep track of the LCU Ptr
            context_ptr->sb_ptr = sb_ptr;

            // Mode Decision Neighbor Arrays
            context_ptr->luma_recon_neighbor_array = picture_control_set_ptr->md_luma_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->cb_recon_neighbor_array = picture_control_set_ptr->md_cb_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->cr_recon_neighbor_array = picture_control_set_ptr->md_cr_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->above_seg_context = picture_control_set_ptr->md_above_seg_context[context_ptr->depth_part_stage];
            context_ptr->left_seg_context = picture_control_set_ptr->md_left_seg_context[context_ptr->depth_part_stage];
            context_ptr->above_context = picture_control_set_ptr->md_above_context[context_ptr->depth_part_stage];
            context_ptr->left_context = picture_control_set_ptr->md_left_context[context_ptr->depth_part_stage];

            // Mode Info Array
            context_ptr->mode_info_array = picture_control_set_ptr->mode_info_array;

            VP9_COMP    *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
            VP9_COMMON  *const cm = &cpi->common;
            MACROBLOCKD *const xd = context_ptr->e_mbd;

            set_sb_rate_context(
                picture_control_set_ptr,
                context_ptr,
                xd);

            uint16_t bdp_block_index = 0;
            uint16_t ep_block_index = 0;

            while (ep_block_index < EP_BLOCK_MAX_COUNT) {

                if (ep_get_block_stats(ep_block_index)->shape == PART_N) {
                    context_ptr->block_ptr = sb_ptr->coded_block_array_ptr[ep_block_index];
                    context_ptr->ep_block_index = ep_block_index;
                    context_ptr->ep_block_stats_ptr = ep_get_block_stats(ep_block_index);
                    context_ptr->block_origin_x = (uint16_t)sb_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_x;
                    context_ptr->block_origin_y = (uint16_t)sb_ptr->origin_y + context_ptr->ep_block_stats_ptr->origin_y;
                    context_ptr->mi_col = context_ptr->block_origin_x >> MI_SIZE_LOG2;
                    context_ptr->mi_row = context_ptr->block_origin_y >> MI_SIZE_LOG2;
                    context_ptr->mi_stride = input_picture_ptr->width >> MI_SIZE_LOG2;
                    context_ptr->bmi_index = ((context_ptr->block_origin_x >> 2) & 0x1) + (((context_ptr->block_origin_y >> 2) & 0x1) << 1);
                    context_ptr->enc_dec_local_block_array[ep_block_index]->cost = MAX_BLOCK_COST;
                    context_ptr->block_ptr->partition_context = partition_plane_context(xd, context_ptr->mi_row, context_ptr->mi_col, context_ptr->ep_block_stats_ptr->bsize);

                    part_block_data_ptr = &context_ptr->bdp_block_data.block_data_array[bdp_block_index];

                    if (bdp_block_index < context_ptr->bdp_block_data.block_count && ep_block_index == part_block_data_ptr->block_index) {

                        context_ptr->block_ptr->split_flag = part_block_data_ptr->split_flag;

                        context_ptr->input_origin_index = (context_ptr->block_origin_y + input_picture_ptr->origin_y) * input_picture_ptr->stride_y + (context_ptr->block_origin_x + input_picture_ptr->origin_x);
                        context_ptr->input_chroma_origin_index = ((ROUND_UV(context_ptr->block_origin_y) >> 1) + (input_picture_ptr->origin_y >> 1)) * input_picture_ptr->stride_cb + ((ROUND_UV(context_ptr->block_origin_x) >> 1) + (input_picture_ptr->origin_x >> 1));
                        context_ptr->block_origin_index = context_ptr->ep_block_stats_ptr->origin_y * MAX_SB_SIZE + context_ptr->ep_block_stats_ptr->origin_x;
                        context_ptr->block_chroma_origin_index = (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) >> 1) * (MAX_SB_SIZE >> 1) + (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x) >> 1);

                        context_ptr->e_mbd->mb_to_top_edge = -((context_ptr->mi_row * MI_SIZE) * 8);
                        context_ptr->e_mbd->mb_to_bottom_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_rows - eb_vp9_num_8x8_blocks_high_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_row) * MI_SIZE) * 8;
                        context_ptr->e_mbd->mb_to_left_edge = -((context_ptr->mi_col * MI_SIZE) * 8);
                        context_ptr->e_mbd->mb_to_right_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_cols - eb_vp9_num_8x8_blocks_wide_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_col) * MI_SIZE) * 8;

                        // Set above_mi and left_mi
                        context_ptr->e_mbd->above_mi = (context_ptr->mi_row > 0) ? context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - cm->mi_stride] : NULL;
                        context_ptr->e_mbd->left_mi = (context_ptr->mi_col > 0) ? context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - 1] : NULL;

                        candidate_buffer_ptr_array = &(candidate_buffer_ptr_array_base[context_ptr->buffer_depth_index_start[context_ptr->ep_block_stats_ptr->depth]]);

                        // Initialize Fast Loop
                        coding_loop_init_fast_loop(
                            context_ptr);

#if VP9_RD
                        // Set context for the rate of cus
                        set_block_rate_context(
                            picture_control_set_ptr,
                            context_ptr,
                            xd);

#endif
                        // Set the number full loop candidates
                        set_nfl(
                            picture_control_set_ptr,
                            context_ptr,
                            sb_ptr);

                        // Generate intra reference samples if intra present
                        if (context_ptr->ep_block_stats_ptr->sq_size < MAX_CU_SIZE) {
                            for (int plane = 0; plane < (int)(((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) ? MAX_MB_PLANE : 1); ++plane) {
                                generate_intra_reference_samples(
                                    sequence_control_set_ptr,
                                    context_ptr,
                                    plane);
                            }
                        }

                        // Search the best intra chroma mode
                        if (context_ptr->ep_block_stats_ptr->sq_size < MAX_CU_SIZE) {
                            if (context_ptr->ep_block_stats_ptr->bsize >= BLOCK_8X8) {
                                if (context_ptr->chroma_level == CHROMA_LEVEL_0) {
                                    search_uv_mode(
                                        picture_control_set_ptr,
                                        input_picture_ptr,
                                        context_ptr);
                                }
                            }
                        }

                        prepare_fast_loop_candidates(
                            picture_control_set_ptr,
                            context_ptr,
                            sb_ptr,
                            &buffer_total_count,
                            &fast_candidate_total_count);

                        // If we want to recon N candidate, we would need N+1 buffers
                        max_buffers = MIN((buffer_total_count + 1), context_ptr->buffer_depth_index_width[context_ptr->ep_block_stats_ptr->depth]);

                        perform_fast_loop(
                            picture_control_set_ptr,
                            context_ptr,
                            sb_ptr,
                            candidate_buffer_ptr_array_base,
                            fast_candidate_array,
                            fast_candidate_total_count,
                            input_picture_ptr,
                            max_buffers,
                            &second_fast_candidate_total_count);

                        // Make sure buffer_total_count is not larger than the number of fast modes
                        buffer_total_count = MIN(second_fast_candidate_total_count, buffer_total_count);

                        // pre_mode_decision
                        pre_mode_decision(
                            (second_fast_candidate_total_count == buffer_total_count) ? buffer_total_count : max_buffers,
                            candidate_buffer_ptr_array,
                            &full_candidate_total_count,
                            context_ptr->best_candidate_index_array,
                            (EB_BOOL)(second_fast_candidate_total_count == buffer_total_count));

                        perform_full_loop(
                            picture_control_set_ptr,
                            context_ptr,
                            sb_ptr,
                            input_picture_ptr,
                            MIN(full_candidate_total_count, buffer_total_count));

                        // Full Mode Decision (choose the best mode)
                        candidate_index = full_mode_decision(
                            context_ptr,
                            candidate_buffer_ptr_array,
                            full_candidate_total_count,
                            context_ptr->best_candidate_index_array,
                            ep_block_index,
                            BDP_PILLAR_STAGE);

                        candidate_buffer = candidate_buffer_ptr_array[candidate_index];

                        best_candidate_buffers[context_ptr->ep_block_stats_ptr->depth] = candidate_buffer;

                        if (context_ptr->intra_md_open_loop_flag == EB_FALSE) {
                            if (context_ptr->spatial_sse_full_loop == EB_FALSE) {
                                context_ptr->e_mbd->mi[0] = &(context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info);
                                perform_inverse_transform_recon(
                                    context_ptr,
                                    candidate_buffer);
                            }
                        }

                        // Inter Depth Decision
                        last_block_index = inter_depth_decision(
                            picture_control_set_ptr,
                            sb_ptr,
                            context_ptr,
                            context_ptr->ep_block_stats_ptr->sqi_mds);

                        if (sb_ptr->coded_block_array_ptr[last_block_index]->split_flag == EB_FALSE)
                        {
                            // Get the settings of the best partition
                            context_ptr->block_ptr = sb_ptr->coded_block_array_ptr[last_block_index];
                            context_ptr->ep_block_index = last_block_index;
                            context_ptr->ep_block_stats_ptr = ep_get_block_stats(last_block_index);
                            context_ptr->block_origin_x = (uint16_t)sb_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_x;
                            context_ptr->block_origin_y = (uint16_t)sb_ptr->origin_y + context_ptr->ep_block_stats_ptr->origin_y;
                            context_ptr->mi_col = context_ptr->block_origin_x >> MI_SIZE_LOG2;
                            context_ptr->mi_row = context_ptr->block_origin_y >> MI_SIZE_LOG2;

                            // Get the settings of the best candidate
                            candidate_buffer = best_candidate_buffers[context_ptr->ep_block_stats_ptr->depth];

                            // Update Neighbor Arrays
                            {
                                int mi_row_index;
                                int mi_col_index;

                                for (mi_row_index = context_ptr->mi_row; mi_row_index < context_ptr->mi_row + ((int)context_ptr->ep_block_stats_ptr->bheight >> MI_SIZE_LOG2); mi_row_index++) {
                                    for (mi_col_index = context_ptr->mi_col; mi_col_index < context_ptr->mi_col + ((int)context_ptr->ep_block_stats_ptr->bwidth >> MI_SIZE_LOG2); mi_col_index++) {
                                        EB_MEMCPY(context_ptr->mode_info_array[mi_col_index + mi_row_index * context_ptr->mi_stride], &(context_ptr->enc_dec_local_block_array[last_block_index]->mode_info), sizeof(ModeInfo));
                                    }
                                }

                                // Update neighbor sample arrays
                                update_recon_neighbor_arrays(
                                    input_picture_ptr,
                                    context_ptr,
                                    sb_ptr,
                                    candidate_buffer,
                                    0);

#if VP9_RD
                                // Update context for the rate of cus
                                update_block_rate_context(
                                    context_ptr,
                                    xd);
#endif
                            }

                            // Keep track of pillar recon buffer as might be used @ bdp_8x8_refinement_sb
                            if (context_ptr->intra_md_open_loop_flag == EB_FALSE)
                            {
                                update_scratch_recon_buffer(
                                    context_ptr,
                                    candidate_buffer->recon_ptr,
                                    context_ptr->bdp_pillar_scratch_recon_buffer,
                                    sb_ptr);
                            }
                        }
                        bdp_block_index++;
                    }
                    else {
                        context_ptr->block_ptr->split_flag = EB_TRUE;
                    }
                }
#if 0 // Hsan - to do
                pa_block_index++;
#else
                ep_block_index++;
#endif
            }

            update_sb_rate_context(
                picture_control_set_ptr,
                context_ptr,
                xd);
        }

        void bdp_64x64_vs_32x32_sb(
            SequenceControlSet *sequence_control_set_ptr,
            PictureControlSet  *picture_control_set_ptr,
            EncDecContext      *context_ptr,
            SbUnit             *sb_ptr)
        {

            EbPictureBufferDesc          *input_picture_ptr = picture_control_set_ptr->parent_pcs_ptr->enhanced_picture_ptr;
            uint32_t                      buffer_total_count;
            ModeDecisionCandidateBuffer  *candidate_buffer;
            ModeDecisionCandidateBuffer  *best_candidate_buffers[EB_MAX_SB_DEPTH];
            uint8_t                       candidate_index;
            uint32_t                      fast_candidate_total_count;
            uint32_t                      full_candidate_total_count;
            uint32_t                      second_fast_candidate_total_count;
            ModeDecisionCandidate        *fast_candidate_array = context_ptr->fast_candidate_array;
            ModeDecisionCandidateBuffer **candidate_buffer_ptr_array_base = context_ptr->candidate_buffer_ptr_array;
            ModeDecisionCandidateBuffer **candidate_buffer_ptr_array;
            uint32_t                      max_buffers;

            // Construct pillar block data array
            construct_pillar_block_data_array(
                sequence_control_set_ptr,
                context_ptr,
                sb_ptr);

            // Keep track of the LCU Ptr
            context_ptr->sb_ptr = sb_ptr;

            // Mode Decision Neighbor Arrays
            context_ptr->luma_recon_neighbor_array = picture_control_set_ptr->md_luma_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->cb_recon_neighbor_array = picture_control_set_ptr->md_cb_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->cr_recon_neighbor_array = picture_control_set_ptr->md_cr_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->above_seg_context = picture_control_set_ptr->md_above_seg_context[context_ptr->depth_part_stage];
            context_ptr->left_seg_context = picture_control_set_ptr->md_left_seg_context[context_ptr->depth_part_stage];
            context_ptr->above_context = picture_control_set_ptr->md_above_context[context_ptr->depth_part_stage];
            context_ptr->left_context = picture_control_set_ptr->md_left_context[context_ptr->depth_part_stage];

            // Mode Info Array
            context_ptr->mode_info_array = picture_control_set_ptr->mode_info_array;
#if VP9_RD
            VP9_COMP    *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
            VP9_COMMON  *const cm = &cpi->common;
            MACROBLOCKD *const xd = context_ptr->e_mbd;

            set_sb_rate_context(
                picture_control_set_ptr,
                context_ptr,
                xd);
#endif

            // Hsan: context variables derivation could be simplified as always 64x64
            uint16_t ep_block_index = 0;
            {
                context_ptr->block_ptr = sb_ptr->coded_block_array_ptr[ep_block_index];
                context_ptr->ep_block_index = ep_block_index;
                context_ptr->ep_block_stats_ptr = ep_get_block_stats(ep_block_index);
                context_ptr->block_origin_x = (uint16_t)sb_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_x;
                context_ptr->block_origin_y = (uint16_t)sb_ptr->origin_y + context_ptr->ep_block_stats_ptr->origin_y;
                context_ptr->mi_col = context_ptr->block_origin_x >> MI_SIZE_LOG2;
                context_ptr->mi_row = context_ptr->block_origin_y >> MI_SIZE_LOG2;
                context_ptr->mi_stride = input_picture_ptr->width >> MI_SIZE_LOG2;
                context_ptr->bmi_index = ((context_ptr->block_origin_x >> 2) & 0x1) + (((context_ptr->block_origin_y >> 2) & 0x1) << 1);
                context_ptr->enc_dec_local_block_array[ep_block_index]->cost = MAX_BLOCK_COST;
                context_ptr->block_ptr->partition_context = partition_plane_context(xd, context_ptr->mi_row, context_ptr->mi_col, context_ptr->ep_block_stats_ptr->bsize);

                context_ptr->input_origin_index = (context_ptr->block_origin_y + input_picture_ptr->origin_y) * input_picture_ptr->stride_y + (context_ptr->block_origin_x + input_picture_ptr->origin_x);
                context_ptr->input_chroma_origin_index = ((ROUND_UV(context_ptr->block_origin_y) >> 1) + (input_picture_ptr->origin_y >> 1)) * input_picture_ptr->stride_cb + ((ROUND_UV(context_ptr->block_origin_x) >> 1) + (input_picture_ptr->origin_x >> 1));
                context_ptr->block_origin_index = context_ptr->ep_block_stats_ptr->origin_y * MAX_SB_SIZE + context_ptr->ep_block_stats_ptr->origin_x;
                context_ptr->block_chroma_origin_index = (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) >> 1) * (MAX_SB_SIZE >> 1) + (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x) >> 1);

                context_ptr->e_mbd->mb_to_top_edge = -((context_ptr->mi_row * MI_SIZE) * 8);
                context_ptr->e_mbd->mb_to_bottom_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_rows - eb_vp9_num_8x8_blocks_high_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_row) * MI_SIZE) * 8;
                context_ptr->e_mbd->mb_to_left_edge = -((context_ptr->mi_col * MI_SIZE) * 8);
                context_ptr->e_mbd->mb_to_right_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_cols - eb_vp9_num_8x8_blocks_wide_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_col) * MI_SIZE) * 8;

                // Set above_mi and left_mi
                context_ptr->e_mbd->above_mi = (context_ptr->mi_row > 0) ? context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - cm->mi_stride] : NULL;
                context_ptr->e_mbd->left_mi = (context_ptr->mi_col > 0) ? context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - 1] : NULL;

                candidate_buffer_ptr_array = &(candidate_buffer_ptr_array_base[context_ptr->buffer_depth_index_start[context_ptr->ep_block_stats_ptr->depth]]);

                // Initialize Fast Loop
                coding_loop_init_fast_loop(
                    context_ptr);

#if VP9_RD
                // Set context for the rate of cus
                set_block_rate_context(
                    picture_control_set_ptr,
                    context_ptr,
                    xd);

#endif
                // Set the number full loop candidates
                set_nfl(
                    picture_control_set_ptr,
                    context_ptr,
                    sb_ptr);

                // Generate intra reference samples if intra present
                if (context_ptr->ep_block_stats_ptr->sq_size < MAX_CU_SIZE) {
                    for (int plane = 0; plane < (int)(((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) ? MAX_MB_PLANE : 1); ++plane) {
                        generate_intra_reference_samples(
                            sequence_control_set_ptr,
                            context_ptr,
                            plane);
                    }
                }

                // Search the best intra chroma mode
                if (context_ptr->ep_block_stats_ptr->sq_size < MAX_CU_SIZE) {
                    if (context_ptr->ep_block_stats_ptr->bsize >= BLOCK_8X8) {
                        if (context_ptr->chroma_level == CHROMA_LEVEL_0) {
                            search_uv_mode(
                                picture_control_set_ptr,
                                input_picture_ptr,
                                context_ptr);
                        }
                    }
                }

                prepare_fast_loop_candidates(
                    picture_control_set_ptr,
                    context_ptr,
                    sb_ptr,
                    &buffer_total_count,
                    &fast_candidate_total_count);

                // If we want to recon N candidate, we would need N+1 buffers
                max_buffers = MIN((buffer_total_count + 1), context_ptr->buffer_depth_index_width[context_ptr->ep_block_stats_ptr->depth]);

                perform_fast_loop(
                    picture_control_set_ptr,
                    context_ptr,
                    sb_ptr,
                    candidate_buffer_ptr_array_base,
                    fast_candidate_array,
                    fast_candidate_total_count,
                    input_picture_ptr,
                    max_buffers,
                    &second_fast_candidate_total_count);

                // Make sure buffer_total_count is not larger than the number of fast modes
                buffer_total_count = MIN(second_fast_candidate_total_count, buffer_total_count);

                // pre_mode_decision
                pre_mode_decision(
                    (second_fast_candidate_total_count == buffer_total_count) ? buffer_total_count : max_buffers,
                    candidate_buffer_ptr_array,
                    &full_candidate_total_count,
                    context_ptr->best_candidate_index_array,
                    (EB_BOOL)(second_fast_candidate_total_count == buffer_total_count));

                perform_full_loop(
                    picture_control_set_ptr,
                    context_ptr,
                    sb_ptr,
                    input_picture_ptr,
                    MIN(full_candidate_total_count, buffer_total_count));

                // Full Mode Decision (choose the best mode)
                candidate_index = full_mode_decision(
                    context_ptr,
                    candidate_buffer_ptr_array,
                    full_candidate_total_count,
                    context_ptr->best_candidate_index_array,
                    0,
                    BDP_64X64_32X32_REF_STAGE);

                candidate_buffer = candidate_buffer_ptr_array[candidate_index];

                best_candidate_buffers[context_ptr->ep_block_stats_ptr->depth] = candidate_buffer;

                if (context_ptr->intra_md_open_loop_flag == EB_FALSE) {
                    if (context_ptr->spatial_sse_full_loop == EB_FALSE) {
                        context_ptr->e_mbd->mi[0] = &(context_ptr->enc_dec_local_block_array[0]->mode_info);
                        perform_inverse_transform_recon(
                            context_ptr,
                            candidate_buffer);
                    }
                }

                // Inter Depth Decision
                {
                    uint64_t depth_n_part_cost = 0;
                    uint64_t depth_n_plus_one_part_cost = 0;
                    uint64_t depth_n_cost = 0;
                    uint64_t depth_n_plus_one_cost = 0;

                    // Compute depth N cost (64x64)
#if VP9_RD
                    get_partition_cost(
                        picture_control_set_ptr,
                        context_ptr,
                        BLOCK_64X64,
                        PARTITION_NONE,
                        sb_ptr->coded_block_array_ptr[0]->partition_context,
                        &depth_n_part_cost);
#endif

                    depth_n_cost = (context_ptr->enc_dec_local_block_array[0]->cost == MAX_BLOCK_COST) ?
                        MAX_BLOCK_COST :
                        context_ptr->enc_dec_local_block_array[0]->cost + depth_n_part_cost;

                    // Compute depth N+1 cost (32x32)
#if VP9_RD
                    get_partition_cost(
                        picture_control_set_ptr,
                        context_ptr,
                        BLOCK_64X64,
                        PARTITION_SPLIT,
                        sb_ptr->coded_block_array_ptr[0]->partition_context,
                        &depth_n_plus_one_part_cost);
#endif

                    depth_n_plus_one_cost =
                        context_ptr->enc_dec_local_block_array[5]->cost +
                        context_ptr->enc_dec_local_block_array[174]->cost +
                        context_ptr->enc_dec_local_block_array[343]->cost +
                        context_ptr->enc_dec_local_block_array[512]->cost +
                        depth_n_plus_one_part_cost;

                    // Inter depth comparison: depth 0 vs depth 1
                    if (depth_n_cost <= depth_n_plus_one_cost) {

                        // If the cost is low enough to warrant not spliting further:
                        // 1. set the split flag of the candidate pu for merging to false
                        // 2. update the last pu index
                        sb_ptr->coded_block_array_ptr[0]->split_flag = EB_FALSE;
                    }
                }

                if (sb_ptr->coded_block_array_ptr[0]->split_flag == EB_FALSE)
                {
                    // Get the settings of the best partition
                    context_ptr->block_ptr = sb_ptr->coded_block_array_ptr[0];
                    context_ptr->ep_block_index = 0;
                    context_ptr->ep_block_stats_ptr = ep_get_block_stats(0);
                    context_ptr->block_origin_x = (uint16_t)sb_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_x;
                    context_ptr->block_origin_y = (uint16_t)sb_ptr->origin_y + context_ptr->ep_block_stats_ptr->origin_y;
                    context_ptr->mi_col = context_ptr->block_origin_x >> MI_SIZE_LOG2;
                    context_ptr->mi_row = context_ptr->block_origin_y >> MI_SIZE_LOG2;

                    // Get the settings of the best candidate
                    candidate_buffer = best_candidate_buffers[context_ptr->ep_block_stats_ptr->depth];

                    // Update Neighbor Arrays
                    {
                        int mi_row_index;
                        int mi_col_index;

                        for (mi_row_index = context_ptr->mi_row; mi_row_index < context_ptr->mi_row + ((int)context_ptr->ep_block_stats_ptr->bheight >> MI_SIZE_LOG2); mi_row_index++) {
                            for (mi_col_index = context_ptr->mi_col; mi_col_index < context_ptr->mi_col + ((int)context_ptr->ep_block_stats_ptr->bwidth >> MI_SIZE_LOG2); mi_col_index++) {
                                EB_MEMCPY(context_ptr->mode_info_array[mi_col_index + mi_row_index * context_ptr->mi_stride], &(context_ptr->enc_dec_local_block_array[0]->mode_info), sizeof(ModeInfo));
                            }
                        }

                        // Update neighbor sample arrays
                        update_recon_neighbor_arrays(
                            input_picture_ptr,
                            context_ptr,
                            sb_ptr,
                            candidate_buffer,
                            0);

#if VP9_RD
                        // Update context for the rate of cus
                        update_block_rate_context(
                            context_ptr,
                            xd);

#endif
                    }

                    if (context_ptr->intra_md_open_loop_flag == EB_FALSE)
                    {
                        update_scratch_recon_buffer(
                            context_ptr,
                            candidate_buffer->recon_ptr,
                            context_ptr->bdp_pillar_scratch_recon_buffer,
                            sb_ptr);
                    }
                }
            }

            update_sb_rate_context(
                picture_control_set_ptr,
                context_ptr,
                xd);
        }

        void bdp_8x8_refinement_sb(
            SequenceControlSet *sequence_control_set_ptr,
            PictureControlSet   *picture_control_set_ptr,
            EncDecContext       *context_ptr,
            SbUnit              *sb_ptr)
        {

            EbPictureBufferDesc          *input_picture_ptr = picture_control_set_ptr->parent_pcs_ptr->enhanced_picture_ptr;
            uint32_t                      buffer_total_count;
            ModeDecisionCandidateBuffer  *candidate_buffer;
            ModeDecisionCandidateBuffer  *best_candidate_buffers[EB_MAX_SB_DEPTH];
            uint8_t                       candidate_index;
            uint32_t                      fast_candidate_total_count;
            uint32_t                      full_candidate_total_count;
            uint32_t                      second_fast_candidate_total_count;
            uint16_t                      last_block_index;
            ModeDecisionCandidate        *fast_candidate_array = context_ptr->fast_candidate_array;
            ModeDecisionCandidateBuffer **candidate_buffer_ptr_array_base = context_ptr->candidate_buffer_ptr_array;
            ModeDecisionCandidateBuffer **candidate_buffer_ptr_array;
            uint32_t                      max_buffers;

            // Keep track of the LCU Ptr
            context_ptr->sb_ptr = sb_ptr;

            // Mode Decision Neighbor Arrays
            context_ptr->luma_recon_neighbor_array = picture_control_set_ptr->md_luma_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->cb_recon_neighbor_array = picture_control_set_ptr->md_cb_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->cr_recon_neighbor_array = picture_control_set_ptr->md_cr_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->above_seg_context = picture_control_set_ptr->md_above_seg_context[context_ptr->depth_part_stage];
            context_ptr->left_seg_context = picture_control_set_ptr->md_left_seg_context[context_ptr->depth_part_stage];
            context_ptr->above_context = picture_control_set_ptr->md_above_context[context_ptr->depth_part_stage];
            context_ptr->left_context = picture_control_set_ptr->md_left_context[context_ptr->depth_part_stage];

            // Mode Info Array
            context_ptr->mode_info_array = picture_control_set_ptr->mode_info_array;
#if VP9_RD
            VP9_COMP    *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
            VP9_COMMON  *const cm = &cpi->common;
            MACROBLOCKD *const xd = context_ptr->e_mbd;

            set_sb_rate_context(
                picture_control_set_ptr,
                context_ptr,
                xd);
#endif

            uint16_t ep_block_index = 0;

            while (ep_block_index < EP_BLOCK_MAX_COUNT) {

                if (sb_ptr->coded_block_array_ptr[ep_block_index]->split_flag == EB_FALSE) {

                    context_ptr->ep_block_index = ep_block_index;
                    context_ptr->ep_block_stats_ptr = ep_get_block_stats(ep_block_index);
                    EB_BOOL cu16x16RefinementFlag;
                    if (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_LIGHT_BDP_DEPTH_MODE || (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_SB_SWITCH_DEPTH_MODE && (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_ptr->sb_index] == SB_LIGHT_BDP_DEPTH_MODE))) {
                        cu16x16RefinementFlag = context_ptr->ep_block_stats_ptr->sq_size == 16 && context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info.mode <= TM_PRED;
                    }
                    else {
                        cu16x16RefinementFlag = context_ptr->ep_block_stats_ptr->sq_size == 16;
                    }

                    if (cu16x16RefinementFlag) {

                        sb_ptr->coded_block_array_ptr[ep_block_index]->split_flag = EB_TRUE;

                        uint16_t ep_8x8_block_index_array[4];
                        uint16_t ep_8x8_block_index;

                        ep_8x8_block_index_array[0] = ep_block_index + ep_inter_depth_offset;
                        ep_8x8_block_index_array[1] = ep_block_index + ep_inter_depth_offset + ep_intra_depth_offset[3];
                        ep_8x8_block_index_array[2] = ep_block_index + ep_inter_depth_offset + ep_intra_depth_offset[3] * 2;
                        ep_8x8_block_index_array[3] = ep_block_index + ep_inter_depth_offset + ep_intra_depth_offset[3] * 3;

                        for (int block_index = 0; block_index < 4; block_index++) {

                            ep_8x8_block_index = ep_8x8_block_index_array[block_index];
                            sb_ptr->coded_block_array_ptr[ep_8x8_block_index]->split_flag = EB_FALSE;
                            context_ptr->block_ptr = sb_ptr->coded_block_array_ptr[ep_8x8_block_index];
                            context_ptr->ep_block_index = ep_8x8_block_index;
                            context_ptr->ep_block_stats_ptr = ep_get_block_stats(ep_8x8_block_index);
                            context_ptr->block_origin_x = (uint16_t)sb_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_x;
                            context_ptr->block_origin_y = (uint16_t)sb_ptr->origin_y + context_ptr->ep_block_stats_ptr->origin_y;
                            context_ptr->mi_col = context_ptr->block_origin_x >> MI_SIZE_LOG2;
                            context_ptr->mi_row = context_ptr->block_origin_y >> MI_SIZE_LOG2;
                            context_ptr->mi_stride = input_picture_ptr->width >> MI_SIZE_LOG2;
                            context_ptr->bmi_index = ((context_ptr->block_origin_x >> 2) & 0x1) + (((context_ptr->block_origin_y >> 2) & 0x1) << 1);

                            context_ptr->block_ptr->partition_context = partition_plane_context(xd, context_ptr->mi_row, context_ptr->mi_col, context_ptr->ep_block_stats_ptr->bsize);

                            context_ptr->input_origin_index = (context_ptr->block_origin_y + input_picture_ptr->origin_y) * input_picture_ptr->stride_y + (context_ptr->block_origin_x + input_picture_ptr->origin_x);
                            context_ptr->input_chroma_origin_index = ((ROUND_UV(context_ptr->block_origin_y) >> 1) + (input_picture_ptr->origin_y >> 1)) * input_picture_ptr->stride_cb + ((ROUND_UV(context_ptr->block_origin_x) >> 1) + (input_picture_ptr->origin_x >> 1));
                            context_ptr->block_origin_index = context_ptr->ep_block_stats_ptr->origin_y * MAX_SB_SIZE + context_ptr->ep_block_stats_ptr->origin_x;
                            context_ptr->block_chroma_origin_index = (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) >> 1) * (MAX_SB_SIZE >> 1) + (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x) >> 1);

                            context_ptr->e_mbd->mb_to_top_edge = -((context_ptr->mi_row * MI_SIZE) * 8);
                            context_ptr->e_mbd->mb_to_bottom_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_rows - eb_vp9_num_8x8_blocks_high_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_row) * MI_SIZE) * 8;
                            context_ptr->e_mbd->mb_to_left_edge = -((context_ptr->mi_col * MI_SIZE) * 8);
                            context_ptr->e_mbd->mb_to_right_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_cols - eb_vp9_num_8x8_blocks_wide_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_col) * MI_SIZE) * 8;

                            // Set above_mi and left_mi
                            context_ptr->e_mbd->above_mi = (context_ptr->mi_row > 0) ? context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - cm->mi_stride] : NULL;
                            context_ptr->e_mbd->left_mi = (context_ptr->mi_col > 0) ? context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - 1] : NULL;

                            candidate_buffer_ptr_array = &(candidate_buffer_ptr_array_base[context_ptr->buffer_depth_index_start[context_ptr->ep_block_stats_ptr->depth]]);

                            // Initialize Fast Loop
                            coding_loop_init_fast_loop(
                                context_ptr);

#if VP9_RD
                            // Set context for the rate of cus
                            set_block_rate_context(
                                picture_control_set_ptr,
                                context_ptr,
                                xd);

#endif
                            // Set the number full loop candidates
                            set_nfl(
                                picture_control_set_ptr,
                                context_ptr,
                                sb_ptr);

                            // Generate intra reference samples if intra present
                            if (context_ptr->ep_block_stats_ptr->sq_size < MAX_CU_SIZE) {
                                for (int plane = 0; plane < (int)(((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) ? MAX_MB_PLANE : 1); ++plane) {
                                    generate_intra_reference_samples(
                                        sequence_control_set_ptr,
                                        context_ptr,
                                        plane);
                                }
                            }

                            // Search the best intra chroma mode
                            if (context_ptr->ep_block_stats_ptr->sq_size < MAX_CU_SIZE) {
                                if (context_ptr->ep_block_stats_ptr->bsize >= BLOCK_8X8) {
                                    if (context_ptr->chroma_level == CHROMA_LEVEL_0) {
                                        search_uv_mode(
                                            picture_control_set_ptr,
                                            input_picture_ptr,
                                            context_ptr);
                                    }
                                }
                            }

                            prepare_fast_loop_candidates(
                                picture_control_set_ptr,
                                context_ptr,
                                sb_ptr,
                                &buffer_total_count,
                                &fast_candidate_total_count);

                            // If we want to recon N candidate, we would need N+1 buffers
                            max_buffers = MIN((buffer_total_count + 1), context_ptr->buffer_depth_index_width[context_ptr->ep_block_stats_ptr->depth]);

                            perform_fast_loop(
                                picture_control_set_ptr,
                                context_ptr,
                                sb_ptr,
                                candidate_buffer_ptr_array_base,
                                fast_candidate_array,
                                fast_candidate_total_count,
                                input_picture_ptr,
                                max_buffers,
                                &second_fast_candidate_total_count);

                            // Make sure buffer_total_count is not larger than the number of fast modes
                            buffer_total_count = MIN(second_fast_candidate_total_count, buffer_total_count);

                            // pre_mode_decision
                            pre_mode_decision(
                                (second_fast_candidate_total_count == buffer_total_count) ? buffer_total_count : max_buffers,
                                candidate_buffer_ptr_array,
                                &full_candidate_total_count,
                                context_ptr->best_candidate_index_array,
                                (EB_BOOL)(second_fast_candidate_total_count == buffer_total_count));

                            perform_full_loop(
                                picture_control_set_ptr,
                                context_ptr,
                                sb_ptr,
                                input_picture_ptr,
                                MIN(full_candidate_total_count, buffer_total_count));

                            // Full Mode Decision (choose the best mode)
                            candidate_index = full_mode_decision(
                                context_ptr,
                                candidate_buffer_ptr_array,
                                full_candidate_total_count,
                                context_ptr->best_candidate_index_array,
                                ep_8x8_block_index,
                                BDP_8X8_REF_STAGE);

                            candidate_buffer = candidate_buffer_ptr_array[candidate_index];

                            best_candidate_buffers[context_ptr->ep_block_stats_ptr->depth] = candidate_buffer;

                            if (context_ptr->intra_md_open_loop_flag == EB_FALSE) {
                                if (context_ptr->spatial_sse_full_loop == EB_FALSE) {
                                    context_ptr->e_mbd->mi[0] = &(context_ptr->enc_dec_local_block_array[ep_8x8_block_index]->mode_info);
                                    perform_inverse_transform_recon(
                                        context_ptr,
                                        candidate_buffer);
                                }
                            }

                            // Inter Depth Decision
                            last_block_index = ep_8x8_block_index;
                            if (ep_8x8_block_index == ep_8x8_block_index_array[3]) {

                                uint64_t depth_n_part_cost = 0;
                                uint64_t depth_n_plus_one_part_cost = 0;
                                uint64_t depth_n_cost = 0;
                                uint64_t depth_n_plus_one_cost = 0;

                                // Compute depth N cost (16x16)
#if VP9_RD
                                get_partition_cost(
                                    picture_control_set_ptr,
                                    context_ptr,
                                    BLOCK_16X16,
                                    PARTITION_NONE,
                                    sb_ptr->coded_block_array_ptr[ep_block_index]->partition_context,
                                    &depth_n_part_cost);
#endif

                                depth_n_cost = (context_ptr->enc_dec_local_block_array[ep_block_index]->cost == MAX_BLOCK_COST) ?
                                    MAX_BLOCK_COST :
                                    context_ptr->enc_dec_local_block_array[ep_block_index]->cost + depth_n_part_cost;

                                // Compute depth N+1 cost

#if VP9_RD
                                get_partition_cost(
                                    picture_control_set_ptr,
                                    context_ptr,
                                    BLOCK_16X16,
                                    PARTITION_SPLIT,
                                    sb_ptr->coded_block_array_ptr[ep_block_index]->partition_context,
                                    &depth_n_plus_one_part_cost);
#endif

                                depth_n_plus_one_cost =
                                    context_ptr->enc_dec_local_block_array[ep_8x8_block_index_array[0]]->cost +
                                    context_ptr->enc_dec_local_block_array[ep_8x8_block_index_array[1]]->cost +
                                    context_ptr->enc_dec_local_block_array[ep_8x8_block_index_array[2]]->cost +
                                    context_ptr->enc_dec_local_block_array[ep_8x8_block_index_array[3]]->cost +
                                    depth_n_plus_one_part_cost;

                                // Inter depth comparison: depth 2 vs depth 3
                                if (depth_n_cost <= depth_n_plus_one_cost) {

                                    // If the cost is low enough to warrant not spliting further:
                                    // 1. set the split flag of the candidate pu for merging to false
                                    // 2. update the last pu index
                                    sb_ptr->coded_block_array_ptr[ep_block_index]->split_flag = EB_FALSE;
                                    context_ptr->enc_dec_local_block_array[ep_block_index]->cost = depth_n_cost;
                                    last_block_index = ep_block_index;
                                }
                                else {
                                    // If the cost is not low enough:
                                    // update the cost of the candidate pu for merging
                                    // this update is required for the next inter depth decision
                                    context_ptr->enc_dec_local_block_array[ep_block_index]->cost = depth_n_plus_one_cost;
                                }
                            }

                            {
                                // Get the settings of the best partition
                                context_ptr->block_ptr = sb_ptr->coded_block_array_ptr[last_block_index];
                                context_ptr->ep_block_index = last_block_index;
                                context_ptr->ep_block_stats_ptr = ep_get_block_stats(last_block_index);
                                context_ptr->block_origin_x = (uint16_t)sb_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_x;
                                context_ptr->block_origin_y = (uint16_t)sb_ptr->origin_y + context_ptr->ep_block_stats_ptr->origin_y;
                                context_ptr->mi_col = context_ptr->block_origin_x >> MI_SIZE_LOG2;
                                context_ptr->mi_row = context_ptr->block_origin_y >> MI_SIZE_LOG2;

                                // Get the settings of the best candidate
                                candidate_buffer = best_candidate_buffers[context_ptr->ep_block_stats_ptr->depth];

                                // Update Neighbor Arrays
                                {
                                    // Update mode info array
                                    int mi_row_index;
                                    int mi_col_index;

                                    for (mi_row_index = context_ptr->mi_row; mi_row_index < context_ptr->mi_row + ((int)context_ptr->ep_block_stats_ptr->bheight >> MI_SIZE_LOG2); mi_row_index++) {
                                        for (mi_col_index = context_ptr->mi_col; mi_col_index < context_ptr->mi_col + ((int)context_ptr->ep_block_stats_ptr->bwidth >> MI_SIZE_LOG2); mi_col_index++) {
                                            EB_MEMCPY(context_ptr->mode_info_array[mi_col_index + mi_row_index * context_ptr->mi_stride], &(context_ptr->enc_dec_local_block_array[last_block_index]->mode_info), sizeof(ModeInfo));
                                        }
                                    }

                                    // Update neighbor sample arrays
                                    update_recon_neighbor_arrays(
                                        input_picture_ptr,
                                        context_ptr,
                                        sb_ptr,
                                        candidate_buffer,
                                        last_block_index == ep_block_index);
#if VP9_RD
                                    // Update context for the rate of cus
                                    update_block_rate_context(
                                        context_ptr,
                                        xd);
#endif
                                }
                            }
                        }
                    }
                    ep_block_index += ep_intra_depth_offset[ep_raster_scan_block_depth[ep_scan_to_raster_scan[ep_block_index]]];
                }
                else {
                    ep_block_index += (uint16_t)((ep_raster_scan_block_depth[ep_scan_to_raster_scan[ep_block_index]] < 4) ? ep_inter_depth_offset : ep_intra_depth_offset[ep_raster_scan_block_depth[ep_scan_to_raster_scan[ep_block_index]]]);
                }
            }

            update_sb_rate_context(
                picture_control_set_ptr,
                context_ptr,
                xd);
        }

        void bdp_nearest_near_sb(
            SequenceControlSet *sequence_control_set_ptr,
            PictureControlSet  *picture_control_set_ptr,
            EncDecContext      *context_ptr,
            SbUnit             *sb_ptr)
        {

            EbPictureBufferDesc          *input_picture_ptr = picture_control_set_ptr->parent_pcs_ptr->enhanced_picture_ptr;
            uint32_t                      buffer_total_count;
            ModeDecisionCandidateBuffer  *candidate_buffer;
            ModeDecisionCandidateBuffer  *best_candidate_buffers[EB_MAX_SB_DEPTH];
            uint8_t                       candidate_index;
            uint32_t                      fast_candidate_total_count;
            uint32_t                      full_candidate_total_count;
            uint32_t                      second_fast_candidate_total_count;
            ModeDecisionCandidate        *fast_candidate_array = context_ptr->fast_candidate_array;
            ModeDecisionCandidateBuffer **candidate_buffer_ptr_array_base = context_ptr->candidate_buffer_ptr_array;
            ModeDecisionCandidateBuffer **candidate_buffer_ptr_array;
            uint32_t                      max_buffers;

            // Keep track of the LCU Ptr
            context_ptr->sb_ptr = sb_ptr;

            // Mode Decision Neighbor Arrays
            context_ptr->luma_recon_neighbor_array = picture_control_set_ptr->md_luma_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->cb_recon_neighbor_array = picture_control_set_ptr->md_cb_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->cr_recon_neighbor_array = picture_control_set_ptr->md_cr_recon_neighbor_array[context_ptr->depth_part_stage];
            context_ptr->above_seg_context = picture_control_set_ptr->md_above_seg_context[context_ptr->depth_part_stage];
            context_ptr->left_seg_context = picture_control_set_ptr->md_left_seg_context[context_ptr->depth_part_stage];
            context_ptr->above_context = picture_control_set_ptr->md_above_context[context_ptr->depth_part_stage];
            context_ptr->left_context = picture_control_set_ptr->md_left_context[context_ptr->depth_part_stage];

            // Mode Info Array
            context_ptr->mode_info_array = picture_control_set_ptr->mode_info_array;
#if VP9_RD
            VP9_COMP    *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
            VP9_COMMON  *const cm = &cpi->common;
            MACROBLOCKD *const xd = context_ptr->e_mbd;

            set_sb_rate_context(
                picture_control_set_ptr,
                context_ptr,
                xd);
#endif

            uint16_t ep_block_index = 0;

            while (ep_block_index < EP_BLOCK_MAX_COUNT) {

                if (sb_ptr->coded_block_array_ptr[ep_block_index]->split_flag == EB_FALSE) {

                    context_ptr->block_ptr = sb_ptr->coded_block_array_ptr[ep_block_index];
                    context_ptr->ep_block_index = ep_block_index;
                    context_ptr->ep_block_stats_ptr = ep_get_block_stats(ep_block_index);
                    context_ptr->block_origin_x = (uint16_t)sb_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_x;
                    context_ptr->block_origin_y = (uint16_t)sb_ptr->origin_y + context_ptr->ep_block_stats_ptr->origin_y;
                    context_ptr->mi_col = context_ptr->block_origin_x >> MI_SIZE_LOG2;
                    context_ptr->mi_row = context_ptr->block_origin_y >> MI_SIZE_LOG2;
                    context_ptr->mi_stride = input_picture_ptr->width >> MI_SIZE_LOG2;
                    context_ptr->bmi_index = ((context_ptr->block_origin_x >> 2) & 0x1) + (((context_ptr->block_origin_y >> 2) & 0x1) << 1);
                    context_ptr->block_ptr->partition_context = partition_plane_context(xd, context_ptr->mi_row, context_ptr->mi_col, context_ptr->ep_block_stats_ptr->bsize);

                    context_ptr->input_origin_index = (context_ptr->block_origin_y + input_picture_ptr->origin_y) * input_picture_ptr->stride_y + (context_ptr->block_origin_x + input_picture_ptr->origin_x);
                    context_ptr->input_chroma_origin_index = ((ROUND_UV(context_ptr->block_origin_y) >> 1) + (input_picture_ptr->origin_y >> 1)) * input_picture_ptr->stride_cb + ((ROUND_UV(context_ptr->block_origin_x) >> 1) + (input_picture_ptr->origin_x >> 1));
                    context_ptr->block_origin_index = context_ptr->ep_block_stats_ptr->origin_y * MAX_SB_SIZE + context_ptr->ep_block_stats_ptr->origin_x;
                    context_ptr->block_chroma_origin_index = (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) >> 1) * (MAX_SB_SIZE >> 1) + (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x) >> 1);

                    context_ptr->e_mbd->mb_to_top_edge = -((context_ptr->mi_row * MI_SIZE) * 8);
                    context_ptr->e_mbd->mb_to_bottom_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_rows - eb_vp9_num_8x8_blocks_high_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_row) * MI_SIZE) * 8;
                    context_ptr->e_mbd->mb_to_left_edge = -((context_ptr->mi_col * MI_SIZE) * 8);
                    context_ptr->e_mbd->mb_to_right_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_cols - eb_vp9_num_8x8_blocks_wide_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_col) * MI_SIZE) * 8;

                    // Set above_mi and left_mi
                    context_ptr->e_mbd->above_mi = (context_ptr->mi_row > 0) ? context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - cm->mi_stride] : NULL;
                    context_ptr->e_mbd->left_mi = (context_ptr->mi_col > 0) ? context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - 1] : NULL;

                    candidate_buffer_ptr_array = &(candidate_buffer_ptr_array_base[context_ptr->buffer_depth_index_start[context_ptr->ep_block_stats_ptr->depth]]);

                    // Initialize Fast Loop
                    coding_loop_init_fast_loop(
                        context_ptr);

#if VP9_RD
                    // Set context for the rate of cus
                    set_block_rate_context(
                        picture_control_set_ptr,
                        context_ptr,
                        xd);

#endif
                    // Set the number full loop candidates
                    set_nfl(
                        picture_control_set_ptr,
                        context_ptr,
                        sb_ptr);

                    // Generate intra reference samples if intra present
                    if (context_ptr->ep_block_stats_ptr->sq_size < MAX_CU_SIZE) {
                        for (int plane = 0; plane < (int)(((context_ptr->chroma_level == CHROMA_LEVEL_0 || context_ptr->chroma_level == CHROMA_LEVEL_1) && context_ptr->ep_block_stats_ptr->has_uv) ? MAX_MB_PLANE : 1); ++plane) {
                            generate_intra_reference_samples(
                                sequence_control_set_ptr,
                                context_ptr,
                                plane);
                        }
                    }

                    // Search the best intra chroma mode
                    if (context_ptr->ep_block_stats_ptr->sq_size < MAX_CU_SIZE) {
                        if (context_ptr->ep_block_stats_ptr->bsize >= BLOCK_8X8) {
                            if (context_ptr->chroma_level == CHROMA_LEVEL_0) {
                                search_uv_mode(
                                    picture_control_set_ptr,
                                    input_picture_ptr,
                                    context_ptr);
                            }
                        }
                    }

                    prepare_fast_loop_candidates(
                        picture_control_set_ptr,
                        context_ptr,
                        sb_ptr,
                        &buffer_total_count,
                        &fast_candidate_total_count);

                    // If we want to recon N candidate, we would need N+1 buffers
                    max_buffers = MIN((buffer_total_count + 1), context_ptr->buffer_depth_index_width[context_ptr->ep_block_stats_ptr->depth]);

                    perform_fast_loop(
                        picture_control_set_ptr,
                        context_ptr,
                        sb_ptr,
                        candidate_buffer_ptr_array_base,
                        fast_candidate_array,
                        fast_candidate_total_count,
                        input_picture_ptr,
                        max_buffers,
                        &second_fast_candidate_total_count);

                    // Make sure buffer_total_count is not larger than the number of fast modes
                    buffer_total_count = MIN(second_fast_candidate_total_count, buffer_total_count);

                    // pre_mode_decision
                    pre_mode_decision(
                        (second_fast_candidate_total_count == buffer_total_count) ? buffer_total_count : max_buffers,
                        candidate_buffer_ptr_array,
                        &full_candidate_total_count,
                        context_ptr->best_candidate_index_array,
                        (EB_BOOL)(second_fast_candidate_total_count == buffer_total_count));

                    perform_full_loop(
                        picture_control_set_ptr,
                        context_ptr,
                        sb_ptr,
                        input_picture_ptr,
                        MIN(full_candidate_total_count, buffer_total_count));

                    // Full Mode Decision (choose the best mode)
                    candidate_index = full_mode_decision(
                        context_ptr,
                        candidate_buffer_ptr_array,
                        full_candidate_total_count,
                        context_ptr->best_candidate_index_array,
                        ep_block_index,
                        BDP_NEAREST_NEAR_STAGE);

                    candidate_buffer = candidate_buffer_ptr_array[candidate_index];

                    best_candidate_buffers[context_ptr->ep_block_stats_ptr->depth] = candidate_buffer;

                    if (context_ptr->intra_md_open_loop_flag == EB_FALSE) {
                        if (context_ptr->spatial_sse_full_loop == EB_FALSE) {
                            context_ptr->e_mbd->mi[0] = &(context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info);
                            perform_inverse_transform_recon(
                                context_ptr,
                                candidate_buffer);
                        }
                    }

                    {

                        // Get the settings of the best candidate
                        candidate_buffer = best_candidate_buffers[context_ptr->ep_block_stats_ptr->depth];

                        // Update Neighbor Arrays
                        {
                            int mi_row_index;
                            int mi_col_index;
                            for (mi_row_index = context_ptr->mi_row; mi_row_index < context_ptr->mi_row + ((int)context_ptr->ep_block_stats_ptr->bheight >> MI_SIZE_LOG2); mi_row_index++) {
                                for (mi_col_index = context_ptr->mi_col; mi_col_index < context_ptr->mi_col + ((int)context_ptr->ep_block_stats_ptr->bwidth >> MI_SIZE_LOG2); mi_col_index++) {
                                    EB_MEMCPY(context_ptr->mode_info_array[mi_col_index + mi_row_index * context_ptr->mi_stride], &(context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info), sizeof(ModeInfo));
                                }
                            }

                            // Update neighbor sample arrays
                            update_recon_neighbor_arrays(
                                input_picture_ptr,
                                context_ptr,
                                sb_ptr,
                                candidate_buffer,
                                0);

                            // Update bdp neighbor sample arrays

                            // Update bdp pillar neighbor sample arrays
                            update_bdp_recon_neighbor_arrays(
                                picture_control_set_ptr,
                                input_picture_ptr,
                                context_ptr,
                                sb_ptr,
                                candidate_buffer,
                                1);

                            // Update bdp refinement neighbor sample arrays
                            update_bdp_recon_neighbor_arrays(
                                picture_control_set_ptr,
                                input_picture_ptr,
                                context_ptr,
                                sb_ptr,
                                candidate_buffer,
                                2);

#if VP9_RD
                            // Update context for the rate of cus
                            update_block_rate_context(
                                context_ptr,
                                xd);

#endif
                        }
                    }
                    ep_block_index += ep_intra_depth_offset[ep_raster_scan_block_depth[ep_scan_to_raster_scan[ep_block_index]]];

                }
                else {
                    ep_block_index += (uint16_t)((ep_raster_scan_block_depth[ep_scan_to_raster_scan[ep_block_index]] < 4) ? ep_inter_depth_offset : ep_intra_depth_offset[ep_raster_scan_block_depth[ep_scan_to_raster_scan[ep_block_index]]]);
                }
            }

            update_sb_rate_context(
                picture_control_set_ptr,
                context_ptr,
                xd);

            // Hsan: add comments
            if (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_SB_SWITCH_DEPTH_MODE && picture_control_set_ptr->bdp_present_flag) {

                update_bdp_sb_rate_context(
                    picture_control_set_ptr,
                    context_ptr,
                    xd,
                    1);

                update_bdp_sb_rate_context(
                    picture_control_set_ptr,
                    context_ptr,
                    xd,
                    2);
            }

        }

        /************************************
        this function checks whether any intra
        block is present in the current SB
        *************************************/
        EB_BOOL is_intra_present(
            EncDecContext *context_ptr,
            SbUnit        *sb_ptr)
        {
            uint16_t block_index = 0;
            while (block_index < EP_BLOCK_MAX_COUNT)
            {
                CodingUnit * const block_ptr = sb_ptr->coded_block_array_ptr[block_index];
                if (block_ptr->split_flag == EB_FALSE) {
                    if (context_ptr->enc_dec_local_block_array[block_index]->mode_info.mode <= TM_PRED)
                        return EB_TRUE;
                    block_index += ep_intra_depth_offset[ep_raster_scan_block_depth[ep_scan_to_raster_scan[block_index]]];
                }
                else {
                    block_index += (uint16_t)((ep_raster_scan_block_depth[ep_scan_to_raster_scan[block_index]] < 4) ? ep_inter_depth_offset : ep_intra_depth_offset[ep_raster_scan_block_depth[ep_scan_to_raster_scan[block_index]]]);
                }
            }
            return EB_FALSE;

        }

#if VP9_PERFORM_EP
        /*******************************************
        * Encode Pass
        *
        * Summary: Performs a Vp9 conformant
        *   reconstruction based on the LCU
        *   mode decision.
        *
        * Inputs:
        *   source_pic
        *   Coding Results
        *   LCU Location
        *   Sequence Control Set
        *   Picture Control Set
        *
        * Outputs:
        *   Reconstructed Samples
        *   Coefficient Samples
        *
        *******************************************/
        EB_EXTERN EbErrorType encode_pass_sb(
            SequenceControlSet *sequence_control_set_ptr,
            PictureControlSet  *picture_control_set_ptr,
            EncDecContext      *context_ptr,
            SbUnit             *sb_ptr)
        {
            EbErrorType  return_error = EB_ErrorNone;

            EbPictureBufferDesc  *input_picture_ptr = picture_control_set_ptr->parent_pcs_ptr->enhanced_picture_ptr;

            VP9_COMP    *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
            VP9_COMMON  *const cm = &cpi->common;
            MACROBLOCKD *const xd = context_ptr->e_mbd;
            RD_OPT *const rd = &cpi->rd;

            // Encode Pass Neighbor Arrays
            context_ptr->luma_recon_neighbor_array = context_ptr->is16bit ? picture_control_set_ptr->ep_luma_recon_neighbor_array_16bit : picture_control_set_ptr->ep_luma_recon_neighbor_array;
            context_ptr->cb_recon_neighbor_array = context_ptr->is16bit ? picture_control_set_ptr->ep_cb_recon_neighbor_array_16bit : picture_control_set_ptr->ep_cb_recon_neighbor_array;
            context_ptr->cr_recon_neighbor_array = context_ptr->is16bit ? picture_control_set_ptr->ep_cr_recon_neighbor_array_16bit : picture_control_set_ptr->ep_cr_recon_neighbor_array;

            // Mode Info Array
            context_ptr->mode_info_array = picture_control_set_ptr->mode_info_array;

            const EB_BOOL is_intra_sb = context_ptr->limit_intra ? is_intra_present(context_ptr, sb_ptr) : EB_TRUE;

            EB_BOOL do_recon = (EB_BOOL)((context_ptr->limit_intra == 0 || is_intra_sb == 1) || picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag || sequence_control_set_ptr->static_config.recon_file);

            // Reset above context @ the 1st SB
            if (sb_ptr->sb_index == 0) {

                // Note: this memset assumes above_context[0], [1] and [2]
                // are allocated as part of the same buffer.
                memset(picture_control_set_ptr->ep_above_context, 0, sizeof(*picture_control_set_ptr->ep_above_context) * MAX_MB_PLANE * 2 * mi_cols_aligned_to_sb(cm->mi_cols));

                memset(picture_control_set_ptr->ep_left_context, 0, sizeof(*picture_control_set_ptr->ep_left_context)*MAX_MB_PLANE * 2 * mi_cols_aligned_to_sb(cm->mi_rows));
            }

#if VP9_PERFORM_EP
            // Reset Quantized Coeff Buffer CU Offsets
            sb_ptr->quantized_coeff_buffer_block_offset[0] = sb_ptr->quantized_coeff_buffer_block_offset[1] = sb_ptr->quantized_coeff_buffer_block_offset[2] = 0;
#endif

            uint16_t ep_block_index = 0;
            while (ep_block_index < EP_BLOCK_MAX_COUNT) {

                context_ptr->block_ptr = sb_ptr->coded_block_array_ptr[ep_block_index];

                if (context_ptr->block_ptr->split_flag == EB_FALSE) {

                    context_ptr->ep_block_index = ep_block_index;
                    context_ptr->ep_block_stats_ptr = ep_get_block_stats(ep_block_index);
                    context_ptr->block_origin_x = (uint16_t)sb_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_x;
                    context_ptr->block_origin_y = (uint16_t)sb_ptr->origin_y + context_ptr->ep_block_stats_ptr->origin_y;

                    context_ptr->input_origin_index = (context_ptr->block_origin_y + input_picture_ptr->origin_y) * input_picture_ptr->stride_y + (context_ptr->block_origin_x + input_picture_ptr->origin_x);
                    context_ptr->input_chroma_origin_index = ((ROUND_UV(context_ptr->block_origin_y) >> 1) + (input_picture_ptr->origin_y >> 1)) * input_picture_ptr->stride_cb + ((ROUND_UV(context_ptr->block_origin_x) >> 1) + (input_picture_ptr->origin_x >> 1));
                    context_ptr->block_origin_index = context_ptr->ep_block_stats_ptr->origin_y * MAX_SB_SIZE + context_ptr->ep_block_stats_ptr->origin_x;
                    context_ptr->block_chroma_origin_index = (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_y) >> 1) * (MAX_SB_SIZE >> 1) + (ROUND_UV(context_ptr->ep_block_stats_ptr->origin_x) >> 1);

                    context_ptr->mi_col = context_ptr->block_origin_x >> MI_SIZE_LOG2;
                    context_ptr->mi_row = context_ptr->block_origin_y >> MI_SIZE_LOG2;
                    context_ptr->mi_stride = input_picture_ptr->width >> MI_SIZE_LOG2;
                    context_ptr->bmi_index = ((context_ptr->block_origin_x >> 2) & 0x1) + (((context_ptr->block_origin_y >> 2) & 0x1) << 1);
                    context_ptr->e_mbd->mb_to_top_edge = -((context_ptr->mi_row * MI_SIZE) * 8);
                    context_ptr->e_mbd->mb_to_bottom_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_rows - eb_vp9_num_8x8_blocks_high_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_row) * MI_SIZE) * 8;
                    context_ptr->e_mbd->mb_to_left_edge = -((context_ptr->mi_col * MI_SIZE) * 8);
                    context_ptr->e_mbd->mb_to_right_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_cols - eb_vp9_num_8x8_blocks_wide_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_col) * MI_SIZE) * 8;

                    // Set above_mi and left_mi
                    context_ptr->e_mbd->above_mi = (context_ptr->mi_row > 0) ? context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - cm->mi_stride] : NULL;
                    context_ptr->e_mbd->left_mi = (context_ptr->mi_col > 0) ? context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - 1] : NULL;

                    // Set the WebM mi
                    context_ptr->e_mbd->mi[0] = context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride];

                    // Set recon buffer indices
                    uint32_t cuOriginReconIndex = context_ptr->recon_buffer->origin_x + context_ptr->block_origin_x + context_ptr->recon_buffer->stride_y * (context_ptr->recon_buffer->origin_y + context_ptr->block_origin_y);
                    uint32_t cuChromaOriginReconIndex = ((context_ptr->recon_buffer->origin_x + ROUND_UV(context_ptr->block_origin_x)) >> 1) + (context_ptr->recon_buffer->stride_y >> 1) * ((context_ptr->recon_buffer->origin_y + ROUND_UV(context_ptr->block_origin_y)) >> 1);

                    for (int i = 0; i < MAX_MB_PLANE; ++i) {
                        xd->above_context[i] =
                            picture_control_set_ptr->ep_above_context +
                            i * sizeof(*picture_control_set_ptr->ep_above_context) * 2 * mi_cols_aligned_to_sb(cm->mi_cols);
                    }

                    for (int i = 0; i < MAX_MB_PLANE; ++i) {
                        EB_MEMCPY(&xd->left_context[i], &picture_control_set_ptr->ep_left_context[(sb_ptr->origin_y >> 2) + (i * sizeof(*picture_control_set_ptr->ep_left_context) * 2 * mi_cols_aligned_to_sb(cm->mi_rows))], 16);
                    }

                    // Set skip context
                    set_skip_context(xd, context_ptr->mi_row, context_ptr->mi_col);

                    eb_vp9_get_entropy_contexts(
                        context_ptr->ep_block_stats_ptr->bsize,
                        context_ptr->ep_block_stats_ptr->tx_size,
                        &context_ptr->e_mbd->plane[0],
                        context_ptr->t_above[0],
                        context_ptr->t_left[0]);

                    eb_vp9_get_entropy_contexts(
                        context_ptr->ep_block_stats_ptr->bsize,
                        context_ptr->ep_block_stats_ptr->tx_size_uv,
                        &context_ptr->e_mbd->plane[1],
                        context_ptr->t_above[1],
                        context_ptr->t_left[1]);

                    eb_vp9_get_entropy_contexts(
                        context_ptr->ep_block_stats_ptr->bsize,
                        context_ptr->ep_block_stats_ptr->tx_size_uv,
                        &context_ptr->e_mbd->plane[2],
                        context_ptr->t_above[2],
                        context_ptr->t_left[2]);

                    int y_coeff_bits = 0;
                    int cb_coeff_bits = 0;
                    int cr_coeff_bits = 0;
                    int yfull_distortion[DIST_CALC_TOTAL] = { 0,0 };
                    int cbfull_distortion[DIST_CALC_TOTAL] = { 0,0 };
                    int crfull_distortion[DIST_CALC_TOTAL] = { 0,0 };
                    int tufull_distortion[3][DIST_CALC_TOTAL] = { {0,0},{ 0,0 },{ 0,0 } };

                    // Prediction
                    for (int plane = 0; plane < (int)(context_ptr->ep_block_stats_ptr->has_uv ? MAX_MB_PLANE : 1); ++plane) {

                        EbByte pred_buffer = (plane == 0) ?
                            (context_ptr->prediction_buffer->buffer_y + context_ptr->block_origin_index) :
                            (plane == 1) ?
                            context_ptr->prediction_buffer->buffer_cb + context_ptr->block_chroma_origin_index :
                            context_ptr->prediction_buffer->buffer_cr + context_ptr->block_chroma_origin_index;

                        uint16_t pred_stride = (plane == 0) ?
                            context_ptr->prediction_buffer->stride_y :
                            (plane == 1) ?
                            context_ptr->prediction_buffer->stride_cb :
                            context_ptr->prediction_buffer->stride_cr;

                        // Generate intra reference samples if intra present
                        generate_intra_reference_samples(
                            sequence_control_set_ptr,
                            context_ptr,
                            plane);

                        prediction_fun_table[context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info.mode <= TM_PRED](
                            context_ptr,
                            pred_buffer,
                            pred_stride,
                            plane);
                    }

                    QUANTS *quants = &picture_control_set_ptr->parent_pcs_ptr->cpi->quants;
#if SEG_SUPPORT
                    VP9_COMMON *const cm = &cpi->common;
                    struct segmentation *const seg = &cm->seg;
                    const int qindex = eb_vp9_get_qindex(seg, context_ptr->segment_id, picture_control_set_ptr->base_qindex);

#else
                    const int qindex = picture_control_set_ptr->base_qindex;
#endif
                    for (uint8_t tu_index = 0; tu_index < ((context_ptr->ep_block_stats_ptr->sq_size == MAX_SB_SIZE) ? 4 : 1); tu_index++) {

                        uint32_t input_tu_origin_index = ((tu_index % 2) * tu_size[context_ptr->ep_block_stats_ptr->tx_size]) + ((tu_index > 1) * tu_size[context_ptr->ep_block_stats_ptr->tx_size] * input_picture_ptr->stride_y);
                        uint32_t residual_quant_tu_origin_index = tu_index * (tu_size[context_ptr->ep_block_stats_ptr->tx_size]) * (tu_size[context_ptr->ep_block_stats_ptr->tx_size]);
                        uint32_t pred_recon_tu_origin_index = ((tu_index % 2) * tu_size[context_ptr->ep_block_stats_ptr->tx_size]) + ((tu_index > 1) * tu_size[context_ptr->ep_block_stats_ptr->tx_size] * context_ptr->recon_buffer->stride_y);
                        uint32_t pred_tu_origin_index = ((tu_index % 2) * tu_size[context_ptr->ep_block_stats_ptr->tx_size]) + ((tu_index > 1) * tu_size[context_ptr->ep_block_stats_ptr->tx_size] * context_ptr->prediction_buffer->stride_y);
                        int tu_coeff_bits = 0;

                        // Luma Coding Loop
                        perform_coding_loop(
                            context_ptr,
                            &(((int16_t*)sb_ptr->quantized_coeff_buffer[0])[sb_ptr->quantized_coeff_buffer_block_offset[0] + residual_quant_tu_origin_index]),
                            context_ptr->ep_block_stats_ptr->sq_size,
                            &(input_picture_ptr->buffer_y[context_ptr->input_origin_index + input_tu_origin_index]),
                            input_picture_ptr->stride_y,
                            &((context_ptr->prediction_buffer->buffer_y)[context_ptr->block_origin_index + pred_tu_origin_index]),
                            context_ptr->prediction_buffer->stride_y,
                            &(((int16_t*)context_ptr->residual_buffer->buffer_y)[0]),                              // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                            &(((int16_t*)context_ptr->transform_buffer->buffer_y)[0]),                             // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                            &(context_ptr->recon_buffer->buffer_y[cuOriginReconIndex + pred_recon_tu_origin_index]),
                            context_ptr->recon_buffer->stride_y,
                            quants->y_zbin[qindex],
                            quants->y_round[qindex],
                            quants->y_quant[qindex],
                            quants->y_quant_shift[qindex],
                            &picture_control_set_ptr->parent_pcs_ptr->cpi->y_dequant[qindex][0],
                            &context_ptr->block_ptr->eob[0][tu_index],
                            context_ptr->ep_block_stats_ptr->tx_size,
                            0,
                            1,
                            do_recon);

                        // Luma Distortion and Rate Calculation
                        if (!(context_ptr->skip_eob_zero_mode_ep && context_ptr->block_ptr->eob[0][tu_index] == 0)) {
                            perform_dist_rate_calc(
                                context_ptr,
                                picture_control_set_ptr,
                                &(((int16_t*)sb_ptr->quantized_coeff_buffer[0])[sb_ptr->quantized_coeff_buffer_block_offset[0] + residual_quant_tu_origin_index]),
                                &(input_picture_ptr->buffer_y[context_ptr->input_origin_index + input_tu_origin_index]),
                                input_picture_ptr->stride_y,
                                &((context_ptr->prediction_buffer->buffer_y)[context_ptr->block_origin_index + pred_tu_origin_index]),
                                context_ptr->prediction_buffer->stride_y,
                                &(((int16_t*)context_ptr->residual_buffer->buffer_y)[0]),
                                &(((int16_t*)context_ptr->transform_buffer->buffer_y)[0]),
                                &(context_ptr->recon_buffer->buffer_y[cuOriginReconIndex + pred_recon_tu_origin_index]),
                                context_ptr->recon_buffer->stride_y,
                                &context_ptr->block_ptr->eob[0][tu_index],
                                context_ptr->ep_block_stats_ptr->tx_size,
                                0,
                                tu_index,
                                context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info.mode,
                                tufull_distortion[0],
                                &tu_coeff_bits);
                        }

                        yfull_distortion[DIST_CALC_RESIDUAL] += tufull_distortion[0][DIST_CALC_RESIDUAL];
                        yfull_distortion[DIST_CALC_PREDICTION] += tufull_distortion[0][DIST_CALC_PREDICTION];
                        y_coeff_bits += tu_coeff_bits;

                    }

                    // Cb Coding Loop
                    if (context_ptr->ep_block_stats_ptr->has_uv) {
                        perform_coding_loop(
                            context_ptr,
                            &(((int16_t*)sb_ptr->quantized_coeff_buffer[1])[sb_ptr->quantized_coeff_buffer_block_offset[1]]),
                            context_ptr->ep_block_stats_ptr->sq_size_uv,
                            &(input_picture_ptr->buffer_cb[context_ptr->input_chroma_origin_index]),
                            input_picture_ptr->stride_cb,
                            &((context_ptr->prediction_buffer->buffer_cb)[context_ptr->block_chroma_origin_index]),
                            context_ptr->prediction_buffer->stride_cb,
                            &(((int16_t*)context_ptr->residual_buffer->buffer_cb)[0]),                              // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                            &(((int16_t*)context_ptr->transform_buffer->buffer_cb)[0]),                             // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                            &(context_ptr->recon_buffer->buffer_cb[cuChromaOriginReconIndex]),
                            context_ptr->recon_buffer->stride_cb,
                            quants->uv_zbin[qindex],
                            quants->uv_round[qindex],
                            quants->uv_quant[qindex],
                            quants->uv_quant_shift[qindex],
                            &picture_control_set_ptr->parent_pcs_ptr->cpi->uv_dequant[qindex][0],
                            &context_ptr->block_ptr->eob[1][0],
                            context_ptr->ep_block_stats_ptr->tx_size_uv,
                            1,
                            1,
                            do_recon);

                        // Cb Distortion and Rate Calculation
                        if (!(context_ptr->skip_eob_zero_mode_ep && context_ptr->block_ptr->eob[1][0] == 0)) {
                            perform_dist_rate_calc(
                                context_ptr,
                                picture_control_set_ptr,
                                &(((int16_t*)sb_ptr->quantized_coeff_buffer[1])[sb_ptr->quantized_coeff_buffer_block_offset[1]]),
                                &(input_picture_ptr->buffer_cb[context_ptr->input_chroma_origin_index]),
                                input_picture_ptr->stride_cb,
                                &((context_ptr->prediction_buffer->buffer_cb)[context_ptr->block_chroma_origin_index]),
                                context_ptr->prediction_buffer->stride_cb,
                                &(((int16_t*)context_ptr->residual_buffer->buffer_cb)[0]),
                                &(((int16_t*)context_ptr->transform_buffer->buffer_cb)[0]),
                                &(context_ptr->recon_buffer->buffer_cb[cuChromaOriginReconIndex]),
                                context_ptr->recon_buffer->stride_cb,
                                &context_ptr->block_ptr->eob[1][0],
                                context_ptr->ep_block_stats_ptr->tx_size_uv,
                                1,
                                0,
                                context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info.mode,
                                &cbfull_distortion[0],
                                &cb_coeff_bits);
                        }

                        // Cr Coding Loop
                        perform_coding_loop(
                            context_ptr,
                            &(((int16_t*)sb_ptr->quantized_coeff_buffer[2])[sb_ptr->quantized_coeff_buffer_block_offset[2]]),
                            context_ptr->ep_block_stats_ptr->sq_size_uv,
                            &(input_picture_ptr->buffer_cr[context_ptr->input_chroma_origin_index]),
                            input_picture_ptr->stride_cr,
                            &((context_ptr->prediction_buffer->buffer_cr)[context_ptr->block_chroma_origin_index]),
                            context_ptr->prediction_buffer->stride_cr,
                            &(((int16_t*)context_ptr->residual_buffer->buffer_cr)[0]),                              // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                            &(((int16_t*)context_ptr->transform_buffer->buffer_cr)[0]),                             // Hsan - does not match block location (i.e. stride not used) - kept as is to do not change WebM kernels
                            &(context_ptr->recon_buffer->buffer_cr[cuChromaOriginReconIndex]),
                            context_ptr->recon_buffer->stride_cr,
                            quants->uv_zbin[qindex],
                            quants->uv_round[qindex],
                            quants->uv_quant[qindex],
                            quants->uv_quant_shift[qindex],
                            &picture_control_set_ptr->parent_pcs_ptr->cpi->uv_dequant[qindex][0],
                            &context_ptr->block_ptr->eob[2][0],
                            context_ptr->ep_block_stats_ptr->tx_size_uv,
                            2,
                            1,
                            do_recon);

                        // Cr Distortion and Rate Calculation
                        if (!(context_ptr->skip_eob_zero_mode_ep && context_ptr->block_ptr->eob[2][0] == 0)) {
                            perform_dist_rate_calc(
                                context_ptr,
                                picture_control_set_ptr,
                                &(((int16_t*)sb_ptr->quantized_coeff_buffer[2])[sb_ptr->quantized_coeff_buffer_block_offset[2]]),
                                &(input_picture_ptr->buffer_cr[context_ptr->input_chroma_origin_index]),
                                input_picture_ptr->stride_cr,
                                &((context_ptr->prediction_buffer->buffer_cr)[context_ptr->block_chroma_origin_index]),
                                context_ptr->prediction_buffer->stride_cr,
                                &(((int16_t*)context_ptr->residual_buffer->buffer_cr)[0]),
                                &(((int16_t*)context_ptr->transform_buffer->buffer_cr)[0]),
                                &(context_ptr->recon_buffer->buffer_cr[cuChromaOriginReconIndex]),
                                context_ptr->recon_buffer->stride_cr,
                                &context_ptr->block_ptr->eob[2][0],
                                context_ptr->ep_block_stats_ptr->tx_size_uv,
                                2,
                                0,
                                context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info.mode,
                                &crfull_distortion[0],
                                &cr_coeff_bits);
                        }
                    }

                    // SKIP decision making
                    if (context_ptr->eob_zero_mode) {
                        if (context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info.mode > TM_PRED) {
                            MACROBLOCKD *const xd = context_ptr->e_mbd;
                            int rate1 = y_coeff_bits + cb_coeff_bits + cr_coeff_bits + vp9_cost_bit(vp9_get_skip_prob(&cpi->common, xd), 0);
                            int rate2 = vp9_cost_bit(vp9_get_skip_prob(&cpi->common, xd), 1);
                            int distortion1 = yfull_distortion[DIST_CALC_RESIDUAL] + cbfull_distortion[DIST_CALC_RESIDUAL] + crfull_distortion[DIST_CALC_RESIDUAL];
                            int distortion2 = yfull_distortion[DIST_CALC_PREDICTION] + cbfull_distortion[DIST_CALC_PREDICTION] + crfull_distortion[DIST_CALC_PREDICTION];

                            uint64_t rd1 = (uint64_t)RDCOST(context_ptr->RDMULT, rd->RDDIV, rate1, (uint64_t)distortion1);
                            uint64_t rd2 = (uint64_t)RDCOST(context_ptr->RDMULT, rd->RDDIV, rate2, (uint64_t)distortion2);
                            if (rd1 >= rd2) {
                                for (uint8_t tu_index = 0; tu_index < ((context_ptr->ep_block_stats_ptr->sq_size == MAX_SB_SIZE) ? 4 : 1); tu_index++) {
                                    if (context_ptr->block_ptr->eob[0][tu_index]) {
                                        uint32_t pred_recon_tu_origin_index = ((tu_index % 2) * tu_size[context_ptr->ep_block_stats_ptr->tx_size]) + ((tu_index > 1) * tu_size[context_ptr->ep_block_stats_ptr->tx_size] * context_ptr->recon_buffer->stride_y);
                                        uint32_t pred_tu_origin_index = ((tu_index % 2) * tu_size[context_ptr->ep_block_stats_ptr->tx_size]) + ((tu_index > 1) * tu_size[context_ptr->ep_block_stats_ptr->tx_size] * context_ptr->prediction_buffer->stride_y);
                                        context_ptr->block_ptr->eob[0][tu_index] = 0;
                                        if (do_recon)
                                            pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][tu_size[context_ptr->ep_block_stats_ptr->tx_size] == 32 ? 4 : context_ptr->ep_block_stats_ptr->tx_size](
                                                &(context_ptr->prediction_buffer->buffer_y[context_ptr->block_origin_index + pred_tu_origin_index]),
                                                context_ptr->prediction_buffer->stride_y,
                                                &(context_ptr->recon_buffer->buffer_y[cuOriginReconIndex + pred_recon_tu_origin_index]),
                                                context_ptr->recon_buffer->stride_y,
                                                tu_size[context_ptr->ep_block_stats_ptr->tx_size],
                                                tu_size[context_ptr->ep_block_stats_ptr->tx_size]);
                                    }
                                }
                                if (context_ptr->block_ptr->eob[1][0]) {
                                    context_ptr->block_ptr->eob[1][0] = 0;
                                    if (do_recon)
                                        pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv] == 32 ? 4 : context_ptr->ep_block_stats_ptr->tx_size - 1](
                                            &((context_ptr->prediction_buffer->buffer_cb)[context_ptr->block_chroma_origin_index]),
                                            context_ptr->prediction_buffer->stride_cb,
                                            &(context_ptr->recon_buffer->buffer_cb[cuChromaOriginReconIndex]),
                                            context_ptr->recon_buffer->stride_cb,
                                            tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv],
                                            tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv]);
                                }
                                if (context_ptr->block_ptr->eob[2][0]) {
                                    context_ptr->block_ptr->eob[2][0] = 0;
                                    if (do_recon)
                                        pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv] == 32 ? 4 : context_ptr->ep_block_stats_ptr->tx_size - 1](
                                            &((context_ptr->prediction_buffer->buffer_cr)[context_ptr->block_chroma_origin_index]),
                                            context_ptr->prediction_buffer->stride_cr,
                                            &(context_ptr->recon_buffer->buffer_cr[cuChromaOriginReconIndex]),
                                            context_ptr->recon_buffer->stride_cr,
                                            tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv],
                                            tu_size[context_ptr->ep_block_stats_ptr->tx_size_uv]);
                                }
                            }
                        }
                    }

                    // Derive skip flag
                    context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info.skip = EB_TRUE;
                    if (context_ptr->ep_block_stats_ptr->has_uv && (context_ptr->block_ptr->eob[1][0] || context_ptr->block_ptr->eob[2][0])) {
                        context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info.skip = EB_FALSE;
                    }
                    else {
                        for (uint8_t tu_index = 0; tu_index < ((context_ptr->ep_block_stats_ptr->sq_size == MAX_SB_SIZE) ? 4 : 1); tu_index++) {
                            if (context_ptr->block_ptr->eob[0][tu_index]) {
                                context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info.skip = EB_FALSE;
                                break;
                            }
                        }
                    }

                    if (context_ptr->ep_block_stats_ptr->bsize < BLOCK_8X8 && context_ptr->ep_block_stats_ptr->is_last_quadrant) {

                        // Derive skip flag
                        context_ptr->mode_info_array[context_ptr->mi_col + context_ptr->mi_row * context_ptr->mi_stride]->skip =
                            context_ptr->enc_dec_local_block_array[context_ptr->ep_block_index - 3]->mode_info.skip == EB_TRUE &&
                            context_ptr->enc_dec_local_block_array[context_ptr->ep_block_index - 2]->mode_info.skip == EB_TRUE &&
                            context_ptr->enc_dec_local_block_array[context_ptr->ep_block_index - 1]->mode_info.skip == EB_TRUE &&
                            context_ptr->enc_dec_local_block_array[context_ptr->ep_block_index]->mode_info.skip == EB_TRUE;
                    }

                    // Increment Quantized Coeff Buffer CU Offsets
                    sb_ptr->quantized_coeff_buffer_block_offset[0] += (context_ptr->ep_block_stats_ptr->sq_size  * context_ptr->ep_block_stats_ptr->sq_size);
                    if (context_ptr->ep_block_stats_ptr->has_uv) {
                        sb_ptr->quantized_coeff_buffer_block_offset[1] += (context_ptr->ep_block_stats_ptr->sq_size_uv  * context_ptr->ep_block_stats_ptr->sq_size_uv);
                        sb_ptr->quantized_coeff_buffer_block_offset[2] += (context_ptr->ep_block_stats_ptr->sq_size_uv  * context_ptr->ep_block_stats_ptr->sq_size_uv);
                    }

                    // Update Neighbor Arrays
                    {
                        int mi_row_index;
                        int mi_col_index;

                        for (mi_row_index = context_ptr->mi_row; mi_row_index < context_ptr->mi_row + ((int)context_ptr->ep_block_stats_ptr->bheight >> MI_SIZE_LOG2); mi_row_index++) {
                            for (mi_col_index = context_ptr->mi_col; mi_col_index < context_ptr->mi_col + ((int)context_ptr->ep_block_stats_ptr->bwidth >> MI_SIZE_LOG2); mi_col_index++) {
                                EB_MEMCPY(context_ptr->mode_info_array[mi_col_index + mi_row_index * context_ptr->mi_stride], &(context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info), sizeof(ModeInfo));
                            }
                        }
                        if (do_recon) {
                            // Recon Samples - Luma
                            eb_vp9_neighbor_array_unit_sample_write(
                                context_ptr->luma_recon_neighbor_array,
                                context_ptr->recon_buffer->buffer_y,
                                context_ptr->recon_buffer->stride_y,
                                context_ptr->recon_buffer->origin_x + context_ptr->block_origin_x,
                                context_ptr->recon_buffer->origin_y + context_ptr->block_origin_y,
                                context_ptr->block_origin_x,
                                context_ptr->block_origin_y,
                                context_ptr->ep_block_stats_ptr->bwidth,
                                context_ptr->ep_block_stats_ptr->bheight,
                                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                            if (context_ptr->ep_block_stats_ptr->has_uv) {
                                // Recon Samples - Cb
                                eb_vp9_neighbor_array_unit_sample_write(
                                    context_ptr->cb_recon_neighbor_array,
                                    context_ptr->recon_buffer->buffer_cb,
                                    context_ptr->recon_buffer->stride_cb,
                                    (context_ptr->recon_buffer->origin_x + ROUND_UV(context_ptr->block_origin_x)) >> 1,
                                    (context_ptr->recon_buffer->origin_y + ROUND_UV(context_ptr->block_origin_y)) >> 1,
                                    ROUND_UV(context_ptr->block_origin_x) >> 1,
                                    ROUND_UV(context_ptr->block_origin_y) >> 1,
                                    context_ptr->ep_block_stats_ptr->bwidth_uv,
                                    context_ptr->ep_block_stats_ptr->bheight_uv,
                                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                                // Recon Samples - Cr
                                eb_vp9_neighbor_array_unit_sample_write(
                                    context_ptr->cr_recon_neighbor_array,
                                    context_ptr->recon_buffer->buffer_cr,
                                    context_ptr->recon_buffer->stride_cr,
                                    (context_ptr->recon_buffer->origin_x + ROUND_UV(context_ptr->block_origin_x)) >> 1,
                                    (context_ptr->recon_buffer->origin_y + ROUND_UV(context_ptr->block_origin_y)) >> 1,
                                    ROUND_UV(context_ptr->block_origin_x) >> 1,
                                    ROUND_UV(context_ptr->block_origin_y) >> 1,
                                    context_ptr->ep_block_stats_ptr->bwidth_uv,
                                    context_ptr->ep_block_stats_ptr->bheight_uv,
                                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);
                            }
                        }

                        struct macroblockd_plane *pd = &xd->plane[0];
                        // Set skip context. point to the right location
                        set_skip_context(xd, context_ptr->mi_row, context_ptr->mi_col);

                        if (context_ptr->ep_block_stats_ptr->bsize < BLOCK_8X8) {
                            eb_vp9_set_contexts(xd,
                                pd,
                                context_ptr->ep_block_stats_ptr->bsize,
                                context_ptr->ep_block_stats_ptr->tx_size,
                                context_ptr->block_ptr->eob[0][0] > 0,
                                context_ptr->bmi_index & 0x1,
                                context_ptr->bmi_index >> 1);
                        }
                        else {
                            for (uint8_t tu_index = 0; tu_index < ((context_ptr->ep_block_stats_ptr->sq_size == MAX_SB_SIZE) ? 4 : 1); tu_index++) {
                                int blk_col = (tu_index & 0x1)*(1 << context_ptr->ep_block_stats_ptr->tx_size);
                                int blk_row = (tu_index >> 1)*(1 << context_ptr->ep_block_stats_ptr->tx_size);

                                eb_vp9_set_contexts(xd,
                                    pd,
                                    context_ptr->ep_block_stats_ptr->bsize,
                                    context_ptr->ep_block_stats_ptr->tx_size,
                                    context_ptr->block_ptr->eob[0][tu_index] > 0,
                                    blk_col,
                                    blk_row);
                            }
                        }

                        if (context_ptr->ep_block_stats_ptr->has_uv) {
                            struct macroblockd_plane *pd = &xd->plane[1];
                            // Set skip context. point to the right location
                            set_skip_context(xd, context_ptr->mi_row, context_ptr->mi_col);
                            eb_vp9_set_contexts(xd,
                                pd,
                                context_ptr->ep_block_stats_ptr->bsize,
                                context_ptr->ep_block_stats_ptr->tx_size_uv,
                                context_ptr->block_ptr->eob[1][0] > 0,
                                0,
                                0);

                            pd = &xd->plane[2];
                            eb_vp9_set_contexts(xd,
                                pd,
                                context_ptr->ep_block_stats_ptr->bsize,
                                context_ptr->ep_block_stats_ptr->tx_size_uv,
                                context_ptr->block_ptr->eob[2][0] > 0,
                                0,
                                0);
                        }
                        // Amir-Hsan: per sb or per block
                        for (int i = 0; i < MAX_MB_PLANE; ++i) {
                            EB_MEMCPY(&picture_control_set_ptr->ep_left_context[(sb_ptr->origin_y >> 2) + (i * sizeof(*picture_control_set_ptr->ep_left_context) * 2 * mi_cols_aligned_to_sb(cm->mi_rows))], &xd->left_context[i], 16);
                        }

                    }

                    ep_block_index += ep_intra_depth_offset[ep_raster_scan_block_depth[ep_scan_to_raster_scan[ep_block_index]]];

                }
                else {
                    ep_block_index += (uint16_t)((ep_raster_scan_block_depth[ep_scan_to_raster_scan[ep_block_index]] < 4) ? ep_inter_depth_offset : ep_intra_depth_offset[ep_raster_scan_block_depth[ep_scan_to_raster_scan[ep_block_index]]]);
                }
            }

            return return_error;
        }
#endif

        /******************************************************
         * Enc Dec Context Constructor
         ******************************************************/
        EbErrorType eb_vp9_enc_dec_context_ctor(
            EncDecContext **context_dbl_ptr,
            EbFifo         *mode_decision_configuration_input_fifo_ptr,
            EbFifo         *packetization_output_fifo_ptr,
            EbFifo         *feedback_fifo_ptr,
            EbFifo         *picture_demux_fifo_ptr)
        {
            EbErrorType return_error = EB_ErrorNone;
            EncDecContext   *context_ptr;
            EB_MALLOC(EncDecContext  *, context_ptr, sizeof(EncDecContext), EB_N_PTR);
            *context_dbl_ptr = context_ptr;

            // Input/Output System Resource Manager FIFOs
            context_ptr->mode_decision_input_fifo_ptr = mode_decision_configuration_input_fifo_ptr;
            context_ptr->enc_dec_output_fifo_ptr = packetization_output_fifo_ptr;
            context_ptr->enc_dec_feedback_fifo_ptr = feedback_fifo_ptr;
            context_ptr->picture_demux_output_fifo_ptr = picture_demux_fifo_ptr;

            // Residual scratch buffer
            {
                EbPictureBufferDescInitData init_data;

                init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_FULL_MASK;
                init_data.max_width = MAX_SB_SIZE;
                init_data.max_height = MAX_SB_SIZE;
                init_data.bit_depth = EB_16BIT;
                init_data.left_padding = 0;
                init_data.right_padding = 0;
                init_data.top_padding = 0;
                init_data.bot_padding = 0;
                init_data.split_mode = EB_FALSE;

                return_error = eb_vp9_picture_buffer_desc_ctor(
                    (EbPtr *)&context_ptr->residual_buffer,
                    (EbPtr)&init_data);
                if (return_error == EB_ErrorInsufficientResources) {
                    return EB_ErrorInsufficientResources;
                }

                return_error = eb_vp9_picture_buffer_desc_ctor(
                    (EbPtr *)&context_ptr->transform_buffer,
                    (EbPtr)&init_data);
                if (return_error == EB_ErrorInsufficientResources) {
                    return EB_ErrorInsufficientResources;
                }
            }

            // Pred scratch buffer
            {
                EbPictureBufferDescInitData init_data;

                init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_FULL_MASK;
                init_data.max_width = MAX_SB_SIZE;
                init_data.max_height = MAX_SB_SIZE;
                init_data.bit_depth = EB_8BIT;
                init_data.left_padding = 0;
                init_data.right_padding = 0;
                init_data.top_padding = 0;
                init_data.bot_padding = 0;
                init_data.split_mode = EB_FALSE;

                return_error = eb_vp9_picture_buffer_desc_ctor(
                    (EbPtr *)&context_ptr->prediction_buffer,
                    (EbPtr)&init_data);

                if (return_error == EB_ErrorInsufficientResources) {
                    return EB_ErrorInsufficientResources;
                }
            }

            // BDP recon scratch buffer
            {
                EbPictureBufferDescInitData init_data;

                init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_FULL_MASK;
                init_data.max_width = MAX_SB_SIZE;
                init_data.max_height = MAX_SB_SIZE;
                init_data.bit_depth = EB_8BIT;
                init_data.left_padding = 0;
                init_data.right_padding = 0;
                init_data.top_padding = 0;
                init_data.bot_padding = 0;
                init_data.split_mode = EB_FALSE;

                return_error = eb_vp9_picture_buffer_desc_ctor(
                    (EbPtr *)&context_ptr->bdp_pillar_scratch_recon_buffer,
                    (EbPtr)&init_data);

                if (return_error == EB_ErrorInsufficientResources) {
                    return EB_ErrorInsufficientResources;
                }
            }

            // uv_mode search scratch buffers
            {
                EbPictureBufferDescInitData init_data;

                init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_CHROMA_MASK;
                init_data.max_width = MAX_SB_SIZE;
                init_data.max_height = MAX_SB_SIZE;
                init_data.bit_depth = EB_8BIT;
                init_data.left_padding = 0;
                init_data.right_padding = 0;
                init_data.top_padding = 0;
                init_data.bot_padding = 0;
                init_data.split_mode = EB_FALSE;

                return_error = eb_vp9_picture_buffer_desc_ctor(
                    (EbPtr *)&context_ptr->uv_mode_search_prediction_buffer,
                    (EbPtr)&init_data);

                if (return_error == EB_ErrorInsufficientResources) {
                    return EB_ErrorInsufficientResources;
                }
            }

            {
                EbPictureBufferDescInitData init_data;

                init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_CHROMA_MASK;
                init_data.max_width = MAX_SB_SIZE;
                init_data.max_height = MAX_SB_SIZE;
                init_data.bit_depth = EB_16BIT;
                init_data.left_padding = 0;
                init_data.right_padding = 0;
                init_data.top_padding = 0;
                init_data.bot_padding = 0;
                init_data.split_mode = EB_FALSE;

                return_error = eb_vp9_picture_buffer_desc_ctor(
                    (EbPtr *)&context_ptr->uv_mode_search_residual_quant_coeff_buffer,
                    (EbPtr)&init_data);

                if (return_error == EB_ErrorInsufficientResources) {
                    return EB_ErrorInsufficientResources;
                }
            }

            {
                EbPictureBufferDescInitData init_data;

                init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_CHROMA_MASK;
                init_data.max_width = MAX_SB_SIZE;
                init_data.max_height = MAX_SB_SIZE;
                init_data.bit_depth = EB_16BIT;
                init_data.left_padding = 0;
                init_data.right_padding = 0;
                init_data.top_padding = 0;
                init_data.bot_padding = 0;
                init_data.split_mode = EB_FALSE;

                return_error = eb_vp9_picture_buffer_desc_ctor(
                    (EbPtr *)&context_ptr->uv_mode_search_recon_coeff_buffer,
                    (EbPtr)&init_data);

                if (return_error == EB_ErrorInsufficientResources) {
                    return EB_ErrorInsufficientResources;
                }
            }

            {
                EbPictureBufferDescInitData init_data;

                init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_CHROMA_MASK;
                init_data.max_width = MAX_SB_SIZE;
                init_data.max_height = MAX_SB_SIZE;
                init_data.bit_depth = EB_16BIT;
                init_data.left_padding = 0;
                init_data.right_padding = 0;
                init_data.top_padding = 0;
                init_data.bot_padding = 0;
                init_data.split_mode = EB_FALSE;

                return_error = eb_vp9_picture_buffer_desc_ctor(
                    (EbPtr *)&context_ptr->uv_mode_search_recon_buffer,
                    (EbPtr)&init_data);

                if (return_error == EB_ErrorInsufficientResources) {
                    return EB_ErrorInsufficientResources;
                }
            }
            EB_MALLOC(ModeInfo*, context_ptr->uv_mode_search_mode_info, sizeof(ModeInfo), EB_N_PTR);

            // Fast Candidate Array
            EB_MALLOC(ModeDecisionCandidate    *, context_ptr->fast_candidate_array, sizeof(ModeDecisionCandidate) * MODE_DECISION_CANDIDATE_MAX_COUNT, EB_N_PTR);

            EB_MALLOC(ModeDecisionCandidate    **, context_ptr->fast_candidate_ptr_array, sizeof(ModeDecisionCandidate *) * MODE_DECISION_CANDIDATE_MAX_COUNT, EB_N_PTR);

            for (uint32_t candidate_index = 0; candidate_index < MODE_DECISION_CANDIDATE_MAX_COUNT; ++candidate_index) {
                context_ptr->fast_candidate_ptr_array[candidate_index] = &context_ptr->fast_candidate_array[candidate_index];
                EB_MALLOC(ModeInfo*, context_ptr->fast_candidate_ptr_array[candidate_index]->mode_info, sizeof(ModeInfo), EB_N_PTR);
            }

            // Transform and Quantization Buffers
            EB_MALLOC(EbTransQuantBuffers*, context_ptr->trans_quant_buffers_ptr, sizeof(EbTransQuantBuffers), EB_N_PTR);

            return_error = eb_vp9_trans_quant_buffers_ctor(
                context_ptr->trans_quant_buffers_ptr);

            if (return_error == EB_ErrorInsufficientResources) {
                return EB_ErrorInsufficientResources;
            }
            // Cost Arrays
            EB_MALLOC(uint64_t*, context_ptr->fast_cost_array, sizeof(uint64_t) * MODE_DECISION_CANDIDATE_BUFFER_MAX_COUNT, EB_N_PTR);

            EB_MALLOC(uint64_t*, context_ptr->full_cost_array, sizeof(uint64_t) * MODE_DECISION_CANDIDATE_BUFFER_MAX_COUNT, EB_N_PTR);

            // Candidate Buffers
            EB_MALLOC(ModeDecisionCandidateBuffer  **, context_ptr->candidate_buffer_ptr_array, sizeof(ModeDecisionCandidateBuffer  *) * MODE_DECISION_CANDIDATE_BUFFER_MAX_COUNT, EB_N_PTR);

            for (uint32_t buffer_index = 0; buffer_index < MODE_DECISION_CANDIDATE_BUFFER_MAX_COUNT; ++buffer_index) {
                return_error = eb_vp9_mode_decision_candidate_buffer_ctor(
                    &(context_ptr->candidate_buffer_ptr_array[buffer_index]),
                    MAX_SB_SIZE,
                    EB_8BIT,
                    &(context_ptr->fast_cost_array[buffer_index]),
                    &(context_ptr->full_cost_array[buffer_index]));

                if (return_error == EB_ErrorInsufficientResources) {
                    return EB_ErrorInsufficientResources;
                }
            }

            EB_MALLOC(uint8_t *, context_ptr->intra_above_ref, sizeof(uint8_t) * ((MAX_SB_SIZE << 1) + 1), EB_N_PTR);
            EB_MALLOC(uint8_t *, context_ptr->intra_left_ref, sizeof(uint8_t) * ((MAX_SB_SIZE << 1) + 1), EB_N_PTR);

            EB_MALLOC(struct scale_factors*, context_ptr->sf, sizeof(struct scale_factors), EB_N_PTR);

            EB_MALLOC(MACROBLOCKD  *, context_ptr->e_mbd, sizeof(MACROBLOCKD), EB_N_PTR);
            EB_MALLOC(ModeInfo   **, context_ptr->e_mbd->mi, sizeof(ModeInfo *), EB_N_PTR);

#if VP9_RD
            EB_MALLOC(ENTROPY_CONTEXT*, context_ptr->e_mbd->plane[0].above_context, sizeof(ENTROPY_CONTEXT) * 16, EB_N_PTR);
            EB_MALLOC(ENTROPY_CONTEXT*, context_ptr->e_mbd->plane[0].left_context, sizeof(ENTROPY_CONTEXT) * 16, EB_N_PTR);

            EB_MALLOC(ENTROPY_CONTEXT*, context_ptr->e_mbd->plane[1].above_context, sizeof(ENTROPY_CONTEXT) * 16, EB_N_PTR);
            EB_MALLOC(ENTROPY_CONTEXT*, context_ptr->e_mbd->plane[1].left_context, sizeof(ENTROPY_CONTEXT) * 16, EB_N_PTR);

            EB_MALLOC(ENTROPY_CONTEXT*, context_ptr->e_mbd->plane[2].above_context, sizeof(ENTROPY_CONTEXT) * 16, EB_N_PTR);
            EB_MALLOC(ENTROPY_CONTEXT*, context_ptr->e_mbd->plane[2].left_context, sizeof(ENTROPY_CONTEXT) * 16, EB_N_PTR);
#endif

            EB_MALLOC(EncDecBlockUnit**, context_ptr->enc_dec_local_block_array, sizeof(EncDecBlockUnit*) * EP_BLOCK_MAX_COUNT, EB_N_PTR);

            for (uint16_t block_index = 0; block_index < EP_BLOCK_MAX_COUNT; ++block_index) {
                EB_MALLOC(EncDecBlockUnit*, context_ptr->enc_dec_local_block_array[block_index], sizeof(EncDecBlockUnit), EB_N_PTR);
            }

            context_ptr->enc_dec_context_ptr = context_ptr;

            return EB_ErrorNone;
        }

        /**************************************************
         * Reset Mode Decision Neighbor Arrays
         *************************************************/
        static void reset_encode_pass_neighbor_arrays(PictureControlSet *picture_control_set_ptr)
        {
            eb_vp9_neighbor_array_unit_reset(picture_control_set_ptr->ep_luma_recon_neighbor_array);
            eb_vp9_neighbor_array_unit_reset(picture_control_set_ptr->ep_cb_recon_neighbor_array);
            eb_vp9_neighbor_array_unit_reset(picture_control_set_ptr->ep_cr_recon_neighbor_array);
            return;
        }

        /**************************************************
         * Reset Coding Loop
         **************************************************/
        static void reset_enc_dec(
            EncDecContext      *context_ptr,
            PictureControlSet  *picture_control_set_ptr,
            SequenceControlSet *sequence_control_set_ptr,
            uint32_t            segment_index)
        {
            context_ptr->is16bit = (EB_BOOL)(sequence_control_set_ptr->static_config.encoder_bit_depth > EB_8BIT);

            if (segment_index == 0) {
                reset_encode_pass_neighbor_arrays(picture_control_set_ptr);
            }

            return;
        }

        /******************************************************
         * Update MD Segments
         *
         * This function is responsible for synchronizing the
         *   processing of MD Segment-rows.
         *   In short, the function starts processing
         *   of MD segment-rows as soon as their inputs are available
         *   and the previous segment-row has completed.  At
         *   any given time, only one segment row per picture
         *   is being processed.
         *
         * The function has two functions:
         *
         * (1) Update the Segment Completion Mask which tracks
         *   which MD Segment inputs are available.
         *
         * (2) Increment the segment-row counter (current_row_idx)
         *   as the segment-rows are completed.
         *
         * Since there is the potentential for thread collusion,
         *   a MUTEX a used to protect the sensitive data and
         *   the execution flow is separated into two paths
         *
         * (A) Initial update.
         *  -Update the Completion Mask [see (1) above]
         *  -If the picture is not currently being processed,
         *     check to see if the next segment-row is available
         *     and start processing.
         * (B) Continued processing
         *  -Upon the completion of a segment-row, check
         *     to see if the next segment-row's inputs have
         *     become available and begin processing if so.
         *
         * On last important point is that the thread-safe
         *   code section is kept minimally short. The MUTEX
         *   should NOT be locked for the entire processing
         *   of the segment-row (B) as this would block other
         *   threads from performing an update (A).
         ******************************************************/
        static EB_BOOL assign_enc_dec_segments(
            EncDecSegments *segment_ptr,
            uint16_t       *segment_in_out_index,
            EncDecTasks    *task_ptr,
            EbFifo         *srm_fifo_ptr)
        {
            EB_BOOL continue_processing_flag = EB_FALSE;
            EbObjectWrapper *wrapper_ptr;
            EncDecTasks *feedback_task_ptr;

            uint32_t row_segment_index = 0;
            uint32_t segment_index = 0;
            uint32_t right_segment_index;
            uint32_t bottom_left_segment_index;

            int16_t feedback_row_index = -1;

            uint32_t self_assigned = EB_FALSE;

            //static FILE *trace = 0;
            //
            //if(trace == 0) {
            //    trace = fopen("seg-trace.txt","w");
            //}

            switch (task_ptr->input_type) {

            case ENCDEC_TASKS_MDC_INPUT:

                // The entire picture is provided by the MDC process, so
                //   no logic is necessary to clear input dependencies.

                // Start on Segment 0 immediately
                *segment_in_out_index = segment_ptr->row_array[0].current_seg_index;
                task_ptr->input_type = ENCDEC_TASKS_CONTINUE;
                ++segment_ptr->row_array[0].current_seg_index;
                continue_processing_flag = EB_TRUE;

                //fprintf(trace, "Start  Pic: %u Seg: %u\n",
                //    (unsigned) ((PictureControlSet  *) task_ptr->picture_control_set_wrapper_ptr->object_ptr)->picture_number,
                //    *segment_in_out_index);

                break;

            case ENCDEC_TASKS_ENCDEC_INPUT:

                // Setup row_segment_index to release the in_progress token
                //row_segment_index = task_ptr->encDecSegmentRowArray[0];

                // Start on the assigned row immediately
                *segment_in_out_index = segment_ptr->row_array[task_ptr->enc_dec_segment_row].current_seg_index;
                task_ptr->input_type = ENCDEC_TASKS_CONTINUE;
                ++segment_ptr->row_array[task_ptr->enc_dec_segment_row].current_seg_index;
                continue_processing_flag = EB_TRUE;

                //fprintf(trace, "Start  Pic: %u Seg: %u\n",
                //    (unsigned) ((PictureControlSet  *) task_ptr->picture_control_set_wrapper_ptr->object_ptr)->picture_number,
                //    *segment_in_out_index);

                break;

            case ENCDEC_TASKS_CONTINUE:

                // Update the Dependency List for Right and Bottom Neighbors
                segment_index = *segment_in_out_index;
                row_segment_index = segment_index / segment_ptr->segment_band_count;

                right_segment_index = segment_index + 1;
                bottom_left_segment_index = segment_index + segment_ptr->segment_band_count;

                // Right Neighbor
                if (segment_index < segment_ptr->row_array[row_segment_index].ending_seg_index)
                {
                    eb_vp9_block_on_mutex(segment_ptr->row_array[row_segment_index].assignment_mutex);

                    --segment_ptr->dep_map.dependency_map[right_segment_index];

                    if (segment_ptr->dep_map.dependency_map[right_segment_index] == 0) {
                        *segment_in_out_index = segment_ptr->row_array[row_segment_index].current_seg_index;
                        ++segment_ptr->row_array[row_segment_index].current_seg_index;
                        self_assigned = EB_TRUE;
                        continue_processing_flag = EB_TRUE;

                        //fprintf(trace, "Start  Pic: %u Seg: %u\n",
                        //    (unsigned) ((PictureControlSet  *) task_ptr->picture_control_set_wrapper_ptr->object_ptr)->picture_number,
                        //    *segment_in_out_index);
                    }

                    eb_vp9_release_mutex(segment_ptr->row_array[row_segment_index].assignment_mutex);
                }

                // Bottom-left Neighbor
                if (row_segment_index < segment_ptr->segment_row_count - 1 && bottom_left_segment_index >= segment_ptr->row_array[row_segment_index + 1].starting_seg_index)
                {
                    eb_vp9_block_on_mutex(segment_ptr->row_array[row_segment_index + 1].assignment_mutex);

                    --segment_ptr->dep_map.dependency_map[bottom_left_segment_index];

                    if (segment_ptr->dep_map.dependency_map[bottom_left_segment_index] == 0) {
                        if (self_assigned == EB_TRUE) {
                            feedback_row_index = (int16_t)row_segment_index + 1;
                        }
                        else {
                            *segment_in_out_index = segment_ptr->row_array[row_segment_index + 1].current_seg_index;
                            ++segment_ptr->row_array[row_segment_index + 1].current_seg_index;
                            self_assigned = EB_TRUE;
                            continue_processing_flag = EB_TRUE;

                            //fprintf(trace, "Start  Pic: %u Seg: %u\n",
                            //    (unsigned) ((PictureControlSet  *) task_ptr->picture_control_set_wrapper_ptr->object_ptr)->picture_number,
                            //    *segment_in_out_index);
                        }
                    }
                    eb_vp9_release_mutex(segment_ptr->row_array[row_segment_index + 1].assignment_mutex);
                }

                if (feedback_row_index > 0) {
                    eb_vp9_get_empty_object(
                        srm_fifo_ptr,
                        &wrapper_ptr);
                    feedback_task_ptr = (EncDecTasks*)wrapper_ptr->object_ptr;
                    feedback_task_ptr->input_type = ENCDEC_TASKS_ENCDEC_INPUT;
                    feedback_task_ptr->enc_dec_segment_row = feedback_row_index;
                    feedback_task_ptr->picture_control_set_wrapper_ptr = task_ptr->picture_control_set_wrapper_ptr;
                    eb_vp9_post_full_object(wrapper_ptr);
                }

                break;

            default:
                break;
            }

            return continue_processing_flag;
        }
        static void recon_output(
            PictureControlSet    *picture_control_set_ptr,
            SequenceControlSet   *sequence_control_set_ptr) {

            EbObjectWrapper             *outputReconWrapperPtr;
            EbBufferHeaderType           *outputReconPtr;
            EncodeContext               *encode_context_ptr = sequence_control_set_ptr->encode_context_ptr;
            EbBool is16bit = (sequence_control_set_ptr->static_config.encoder_bit_depth > EB_8BIT);
            // The totalNumberOfReconFrames counter has to be write/read protected as
            //   it is used to determine the end of the stream.  If it is not protected
            //   the encoder might not properly terminate.
            eb_vp9_block_on_mutex(encode_context_ptr->total_number_of_recon_frame_mutex);

            // Get Recon Buffer
            eb_vp9_get_empty_object(
                sequence_control_set_ptr->encode_context_ptr->recon_output_fifo_ptr,
                &outputReconWrapperPtr);
            outputReconPtr = (EbBufferHeaderType*)outputReconWrapperPtr->object_ptr;
            outputReconPtr->flags = 0;

            // START READ/WRITE PROTECTED SECTION
            if (encode_context_ptr->total_number_of_recon_frames == encode_context_ptr->terminating_picture_number)
                outputReconPtr->flags = EB_BUFFERFLAG_EOS;

            encode_context_ptr->total_number_of_recon_frames++;

            //EbReleaseMutex(encode_context_ptr->terminating_conditions_mutex);

            // STOP READ/WRITE PROTECTED SECTION
            outputReconPtr->n_filled_len = 0;

            // Copy the Reconstructed Picture to the Output Recon Buffer
            {
                uint32_t sampleTotalCount;
                uint8_t *reconReadPtr;
                uint8_t *reconWritePtr;

                EbPictureBufferDesc *reconPtr;
                {
                    if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                        reconPtr = is16bit ?
                        ((EbReferenceObject*)picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr->object_ptr)->reference_picture16bit :
                        ((EbReferenceObject*)picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr->object_ptr)->reference_picture;
                    else {
                        if (is16bit)
                            reconPtr = picture_control_set_ptr->recon_picture_16bit_ptr;
                        else
                            reconPtr = picture_control_set_ptr->recon_picture_ptr;
                    }
                }

                // End running the film grain
                // Y Recon Samples
                sampleTotalCount = ((reconPtr->max_width - sequence_control_set_ptr->max_input_pad_right) * (reconPtr->max_height - sequence_control_set_ptr->max_input_pad_bottom)) << is16bit;
                reconReadPtr = reconPtr->buffer_y + (reconPtr->origin_y << is16bit) * reconPtr->stride_y + (reconPtr->origin_x << is16bit);
                reconWritePtr = &(outputReconPtr->p_buffer[outputReconPtr->n_filled_len]);

                CHECK_REPORT_ERROR(
                    (outputReconPtr->n_filled_len + sampleTotalCount <= outputReconPtr->n_alloc_len),
                    encode_context_ptr->app_callback_ptr,
                    EB_ENC_ROB_OF_ERROR);

                // Initialize Y recon buffer
                eb_vp9_picture_copy_kernel(
                    reconReadPtr,
                    reconPtr->stride_y,
                    reconWritePtr,
                    reconPtr->max_width - sequence_control_set_ptr->max_input_pad_right,
                    reconPtr->width - sequence_control_set_ptr->pad_right,
                    reconPtr->height - sequence_control_set_ptr->pad_bottom,
                    1 << is16bit);

                outputReconPtr->n_filled_len += sampleTotalCount;

                // U Recon Samples
                sampleTotalCount = ((reconPtr->max_width - sequence_control_set_ptr->max_input_pad_right) * (reconPtr->max_height - sequence_control_set_ptr->max_input_pad_bottom) >> 2) << is16bit;
                reconReadPtr = reconPtr->buffer_cb + ((reconPtr->origin_y << is16bit) >> 1) * reconPtr->stride_cb + ((reconPtr->origin_x << is16bit) >> 1);
                reconWritePtr = &(outputReconPtr->p_buffer[outputReconPtr->n_filled_len]);

                CHECK_REPORT_ERROR(
                    (outputReconPtr->n_filled_len + sampleTotalCount <= outputReconPtr->n_alloc_len),
                    encode_context_ptr->app_callback_ptr,
                    EB_ENC_ROB_OF_ERROR);

                // Initialize U recon buffer
                eb_vp9_picture_copy_kernel(
                    reconReadPtr,
                    reconPtr->stride_cb,
                    reconWritePtr,
                    (reconPtr->max_width - sequence_control_set_ptr->max_input_pad_right) >> 1,
                    (reconPtr->width - sequence_control_set_ptr->pad_right) >> 1,
                    (reconPtr->height - sequence_control_set_ptr->pad_bottom) >> 1,
                    1 << is16bit);
                outputReconPtr->n_filled_len += sampleTotalCount;

                // V Recon Samples
                sampleTotalCount = ((reconPtr->max_width - sequence_control_set_ptr->max_input_pad_right) * (reconPtr->max_height - sequence_control_set_ptr->max_input_pad_bottom) >> 2) << is16bit;
                reconReadPtr = reconPtr->buffer_cr + ((reconPtr->origin_y << is16bit) >> 1) * reconPtr->stride_cr + ((reconPtr->origin_x << is16bit) >> 1);
                reconWritePtr = &(outputReconPtr->p_buffer[outputReconPtr->n_filled_len]);

                CHECK_REPORT_ERROR(
                    (outputReconPtr->n_filled_len + sampleTotalCount <= outputReconPtr->n_alloc_len),
                    encode_context_ptr->app_callback_ptr,
                    EB_ENC_ROB_OF_ERROR);

                // Initialize V recon buffer

                eb_vp9_picture_copy_kernel(
                    reconReadPtr,
                    reconPtr->stride_cr,
                    reconWritePtr,
                    (reconPtr->max_width - sequence_control_set_ptr->max_input_pad_right) >> 1,
                    (reconPtr->width - sequence_control_set_ptr->pad_right) >> 1,
                    (reconPtr->height - sequence_control_set_ptr->pad_bottom) >> 1,
                    1 << is16bit);
                outputReconPtr->n_filled_len += sampleTotalCount;
                outputReconPtr->pts = picture_control_set_ptr->picture_number;
            }

            // Post the Recon object
            eb_vp9_post_full_object(outputReconWrapperPtr);
            eb_vp9_release_mutex(encode_context_ptr->total_number_of_recon_frame_mutex);
        }

        static void pad_ref_and_set_flags(
            PictureControlSet  *picture_control_set_ptr,
            SequenceControlSet *sequence_control_set_ptr
        )
        {

            EbReferenceObject     *reference_object = (EbReferenceObject  *)picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr->object_ptr;
            EbPictureBufferDesc   *ref_pic_ptr = (EbPictureBufferDesc  *)reference_object->reference_picture;
            EbPictureBufferDesc   *ref_pic16_bit_ptr = (EbPictureBufferDesc  *)reference_object->reference_picture16bit;
            EB_BOOL                is16bit = (sequence_control_set_ptr->static_config.encoder_bit_depth > EB_8BIT);

            if (!is16bit) {
                // Y samples
                eb_vp9_generate_padding(
                    ref_pic_ptr->buffer_y,
                    ref_pic_ptr->stride_y,
                    ref_pic_ptr->width,
                    ref_pic_ptr->height,
                    ref_pic_ptr->origin_x,
                    ref_pic_ptr->origin_y);

                // Cb samples
                eb_vp9_generate_padding(
                    ref_pic_ptr->buffer_cb,
                    ref_pic_ptr->stride_cb,
                    ref_pic_ptr->width >> 1,
                    ref_pic_ptr->height >> 1,
                    ref_pic_ptr->origin_x >> 1,
                    ref_pic_ptr->origin_y >> 1);

                // Cr samples
                eb_vp9_generate_padding(
                    ref_pic_ptr->buffer_cr,
                    ref_pic_ptr->stride_cr,
                    ref_pic_ptr->width >> 1,
                    ref_pic_ptr->height >> 1,
                    ref_pic_ptr->origin_x >> 1,
                    ref_pic_ptr->origin_y >> 1);
            }

            //We need this for MCP
            if (is16bit) {
                // Y samples
                eb_vp9_generate_padding_16bit(
                    ref_pic16_bit_ptr->buffer_y,
                    ref_pic16_bit_ptr->stride_y << 1,
                    ref_pic16_bit_ptr->width << 1,
                    ref_pic16_bit_ptr->height,
                    ref_pic16_bit_ptr->origin_x << 1,
                    ref_pic16_bit_ptr->origin_y);

                // Cb samples
                eb_vp9_generate_padding_16bit(
                    ref_pic16_bit_ptr->buffer_cb,
                    ref_pic16_bit_ptr->stride_cb << 1,
                    ref_pic16_bit_ptr->width,
                    ref_pic16_bit_ptr->height >> 1,
                    ref_pic16_bit_ptr->origin_x,
                    ref_pic16_bit_ptr->origin_y >> 1);

                // Cr samples
                eb_vp9_generate_padding_16bit(
                    ref_pic16_bit_ptr->buffer_cr,
                    ref_pic16_bit_ptr->stride_cr << 1,
                    ref_pic16_bit_ptr->width,
                    ref_pic16_bit_ptr->height >> 1,
                    ref_pic16_bit_ptr->origin_x,
                    ref_pic16_bit_ptr->origin_y >> 1);
            }

            // set up the ref POC
            reference_object->ref_poc = picture_control_set_ptr->parent_pcs_ptr->picture_number;

            // set up the QP
            reference_object->qp = (uint8_t)picture_control_set_ptr->parent_pcs_ptr->picture_qp;

            // set up the Slice Type
            reference_object->slice_type = picture_control_set_ptr->parent_pcs_ptr->slice_type;

        }

        void copy_statistics_to_ref_object(
            PictureControlSet *picture_control_set_ptr) {

            uint32_t sb_index;
            for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {
                ((EbReferenceObject*)picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr->object_ptr)->non_moving_index_array[sb_index] = picture_control_set_ptr->parent_pcs_ptr->non_moving_index_array[sb_index];
            }

            ((EbReferenceObject*)picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr->object_ptr)->tmp_layer_idx = (uint8_t)picture_control_set_ptr->temporal_layer_index;
            ((EbReferenceObject*)picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr->object_ptr)->is_scene_change = picture_control_set_ptr->parent_pcs_ptr->scene_change_flag;
        }

        /******************************************************
        * Derive EncDec Settings for SQ
        Input   : encoder mode and tune
        Output  : EncDec Kernel signal(s)
        ******************************************************/
        EbErrorType eb_vp9_signal_derivation_enc_dec_kernel_sq(
            SequenceControlSet *sequence_control_set_ptr,
            PictureControlSet  *picture_control_set_ptr,
            EncDecContext      *context_ptr) {

            EbErrorType return_error = EB_ErrorNone;
            UNUSED(sequence_control_set_ptr);
            // Derive Open Loop INTRA @ MD
            context_ptr->intra_md_open_loop_flag = EB_FALSE;

            // Derive Spatial SSE Flag
            if (picture_control_set_ptr->parent_pcs_ptr->enc_mode <= ENC_MODE_2) {
                if (picture_control_set_ptr->slice_type == I_SLICE && context_ptr->intra_md_open_loop_flag == EB_FALSE) {
                    context_ptr->spatial_sse_full_loop = EB_TRUE;
                }
                else {
                    context_ptr->spatial_sse_full_loop = EB_FALSE;
                }
            }
            else {
                context_ptr->spatial_sse_full_loop = EB_FALSE;
            }

            // Set Chroma Level
            // Level            Settings
            // CHROMA_LEVEL_0   CHROMA ON @ MD + separate CHROMA search ON
            // CHROMA_LEVEL_1   CHROMA ON @ MD + separate CHROMA search OFF
            // CHROMA_LEVEL_2   CHROMA OFF @ MD
            if (picture_control_set_ptr->enc_mode == ENC_MODE_0) {
                context_ptr->chroma_level = CHROMA_LEVEL_0;
            }
            else if (picture_control_set_ptr->enc_mode <= ENC_MODE_3) {
                if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag) {
                    context_ptr->chroma_level = CHROMA_LEVEL_1;
                }
                else {
                    context_ptr->chroma_level = CHROMA_LEVEL_2;
                }
            }
            else if (picture_control_set_ptr->enc_mode <= ENC_MODE_5) {
                if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0) {
                    context_ptr->chroma_level = CHROMA_LEVEL_1;
                }
                else {
                    context_ptr->chroma_level = CHROMA_LEVEL_2;
                }
            }
            else {
                context_ptr->chroma_level = CHROMA_LEVEL_2;
            }

            // Set allow_enc_dec_mismatch
            if (picture_control_set_ptr->parent_pcs_ptr->enc_mode <= ENC_MODE_6) {
                context_ptr->allow_enc_dec_mismatch = EB_FALSE;
            }
            else {
                context_ptr->allow_enc_dec_mismatch = (picture_control_set_ptr->temporal_layer_index > 0) ?
                    EB_TRUE :
                    EB_FALSE;
            }

            // Set eob based Full-Loop Escape Flag
            context_ptr->full_loop_escape = EB_TRUE;

            // Set Fast-Loop Method
            context_ptr->single_fast_loop_flag = EB_TRUE;

            // Set Nearest Injection Flag
            context_ptr->nearest_injection = EB_TRUE;

            // Set Near Injection Flag
            context_ptr->near_injection = EB_TRUE;

            // Set Zero Injection Flag
            context_ptr->zero_injection = EB_TRUE;

            // Set Unipred 3x3 Injection Flag
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_2) {
                context_ptr->unipred3x3_injection = EB_TRUE;
            }
            else {
                context_ptr->unipred3x3_injection = EB_FALSE;
            }

            // Set Bipred 3x3 Injection Flag
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_1)
                context_ptr->bipred3x3_injection = EB_TRUE;
            else
                context_ptr->bipred3x3_injection = EB_FALSE;

            // Set Limit INTRA Flag
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
                context_ptr->limit_intra = EB_FALSE;
            }
            else {
                if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_FALSE) {
                    context_ptr->limit_intra = EB_TRUE;
                }
                else {
                    context_ptr->limit_intra = EB_FALSE;
                }
            }

            // Set PF @ MD Level
            // Level    Settings
            // 0        OFF
            // 1        N2
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_8) {
                context_ptr->pf_md_level = 0;
            }
            else if (picture_control_set_ptr->enc_mode <= ENC_MODE_9) {
                if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag) {
                    context_ptr->pf_md_level = 0;
                }
                else {
                    context_ptr->pf_md_level = 1;
                }
            }
            else {
                if (picture_control_set_ptr->temporal_layer_index == 0) {
                    context_ptr->pf_md_level = 0;
                }
                else {
                    context_ptr->pf_md_level = 1;
                }
            }

            // NFL Level MD         Settings
            // 0                    11
            // 1                    4
            // 2                    3
            // 3                    2
            // 4                    1
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_2) {
                context_ptr->nfl_level = 0;
            }
            else if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
                context_ptr->nfl_level = 1;
            }
            else {
                context_ptr->nfl_level = 2;
            }

            return return_error;
        }

        /******************************************************
        * Derive EncDec Settings for OQ
        Input   : encoder mode and tune
        Output  : EncDec Kernel signal(s)
        ******************************************************/
        EbErrorType eb_vp9_signal_derivation_enc_dec_kernel_oq(
            SequenceControlSet *sequence_control_set_ptr,
            PictureControlSet  *picture_control_set_ptr,
            EncDecContext      *context_ptr) {

            EbErrorType return_error = EB_ErrorNone;

            // Derive Open Loop INTRA @ MD
            context_ptr->intra_md_open_loop_flag = EB_FALSE;

            // Derive Spatial SSE Flag
            if (picture_control_set_ptr->parent_pcs_ptr->enc_mode <= ENC_MODE_2) {
                if (picture_control_set_ptr->slice_type == I_SLICE && context_ptr->intra_md_open_loop_flag == EB_FALSE) {
                    context_ptr->spatial_sse_full_loop = EB_TRUE;
                }
                else {
                    context_ptr->spatial_sse_full_loop = EB_FALSE;
                }
            }
            else {
                context_ptr->spatial_sse_full_loop = EB_FALSE;
            }

            // Set Chroma Level
            // Level            Settings
            // CHROMA_LEVEL_0   CHROMA ON @ MD + separate CHROMA search ON
            // CHROMA_LEVEL_1   CHROMA ON @ MD + separate CHROMA search OFF
            // CHROMA_LEVEL_2   CHROMA OFF @ MD
            if (picture_control_set_ptr->enc_mode == ENC_MODE_0) {
                context_ptr->chroma_level = CHROMA_LEVEL_0;
            }
            else if (picture_control_set_ptr->enc_mode <= ENC_MODE_3) {
                if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag) {
                    context_ptr->chroma_level = CHROMA_LEVEL_1;
                }
                else {
                    context_ptr->chroma_level = CHROMA_LEVEL_2;
                }
            }
            else if (picture_control_set_ptr->enc_mode <= ENC_MODE_5) {
                if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0) {
                    context_ptr->chroma_level = CHROMA_LEVEL_1;
                }
                else {
                    context_ptr->chroma_level = CHROMA_LEVEL_2;
                }
            }
            else {
                context_ptr->chroma_level = CHROMA_LEVEL_2;
            }

            // Set allow_enc_dec_mismatch
            if (picture_control_set_ptr->parent_pcs_ptr->enc_mode <= ENC_MODE_6) {
                context_ptr->allow_enc_dec_mismatch = EB_FALSE;
            }
            else {
                context_ptr->allow_enc_dec_mismatch = (picture_control_set_ptr->temporal_layer_index > 0) ?
                    EB_TRUE :
                    EB_FALSE;
            }

            // Set eob based Full-Loop Escape Flag
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_1) {
                context_ptr->full_loop_escape = EB_FALSE;
            }
            else {
                if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER) {
                    context_ptr->full_loop_escape = EB_FALSE;
                }
                else {
                    context_ptr->full_loop_escape = EB_TRUE;
                }
            }

            // Set Fast-Loop Method
            context_ptr->single_fast_loop_flag = EB_TRUE;

            // Set Nearest Injection Flag
            context_ptr->nearest_injection = EB_TRUE;

            // Set Near Injection Flag
            context_ptr->near_injection = EB_TRUE;

            // Set Zero Injection Flag
            context_ptr->zero_injection = EB_TRUE;

            // Set Unipred 3x3 Injection Flag
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_1) {
                context_ptr->unipred3x3_injection = EB_TRUE;
            }
            else {
                context_ptr->unipred3x3_injection = EB_FALSE;
            }

            // Set Bipred 3x3 Injection Flag
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_1) {
                context_ptr->bipred3x3_injection = EB_TRUE;
            }
            else {
                context_ptr->bipred3x3_injection = EB_FALSE;
            }

            // Set Limit INTRA Flag
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
                context_ptr->limit_intra = EB_FALSE;
            }
            else {
                if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_FALSE) {
                    context_ptr->limit_intra = EB_TRUE;
                }
                else {
                    context_ptr->limit_intra = EB_FALSE;
                }
            }

            // Set PF @ MD Level
            // Level    Settings
            // 0        OFF
            // 1        N2
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_5) {
                context_ptr->pf_md_level = 0;
            }
            else if (picture_control_set_ptr->enc_mode <= ENC_MODE_7) {
                if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag) {
                    context_ptr->pf_md_level = 0;
                }
                else {
                    context_ptr->pf_md_level = 1;
                }
            }
            else {
                if (picture_control_set_ptr->temporal_layer_index == 0) {
                    context_ptr->pf_md_level = 0;
                }
                else {
                    context_ptr->pf_md_level = 1;
                }
            }

            // NFL Level MD         Settings
            // 0                    11
            // 1                    4
            // 2                    3
            // 3                    2
            // 4                    1
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_2) {
                context_ptr->nfl_level = 0;
            }
            else if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
                context_ptr->nfl_level = 1;
            }
            else {
                context_ptr->nfl_level = 2;
            }

            return return_error;
        }

        /******************************************************
        * Derive EncDec Settings for VMAF
        Input   : encoder mode and tune
        Output  : EncDec Kernel signal(s)
        ******************************************************/
        EbErrorType eb_vp9_signal_derivation_enc_dec_kernel_vmaf(
            SequenceControlSet *sequence_control_set_ptr,
            PictureControlSet  *picture_control_set_ptr,
            EncDecContext      *context_ptr) {

            EbErrorType return_error = EB_ErrorNone;

            // Derive Open Loop INTRA @ MD
            context_ptr->intra_md_open_loop_flag = EB_FALSE;

            // Derive Spatial SSE Flag
            if (picture_control_set_ptr->parent_pcs_ptr->enc_mode <= ENC_MODE_2) {
                if (picture_control_set_ptr->slice_type == I_SLICE && context_ptr->intra_md_open_loop_flag == EB_FALSE) {
                    context_ptr->spatial_sse_full_loop = EB_TRUE;
                }
                else {
                    context_ptr->spatial_sse_full_loop = EB_FALSE;
                }
            }
            else {
                context_ptr->spatial_sse_full_loop = EB_FALSE;
            }

            // Set Allow EncDec Mismatch Flag
            context_ptr->allow_enc_dec_mismatch = EB_FALSE;

            // Set Chroma Level
            // Level            Settings
            // CHROMA_LEVEL_0   CHROMA ON @ MD + separate CHROMA search ON
            // CHROMA_LEVEL_1   CHROMA ON @ MD + separate CHROMA search OFF
            // CHROMA_LEVEL_2   CHROMA OFF @ MD
            if (picture_control_set_ptr->enc_mode == ENC_MODE_0) {
                context_ptr->chroma_level = CHROMA_LEVEL_0;
            }
            else if (picture_control_set_ptr->enc_mode <= ENC_MODE_3) {
                if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag) {
                    context_ptr->chroma_level = CHROMA_LEVEL_1;
                }
                else {
                    context_ptr->chroma_level = CHROMA_LEVEL_2;
                }
            }
            else if (picture_control_set_ptr->enc_mode <= ENC_MODE_5) {
                if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0) {
                    context_ptr->chroma_level = CHROMA_LEVEL_1;
                }
                else {
                    context_ptr->chroma_level = CHROMA_LEVEL_2;
                }
            }
            else {
                context_ptr->chroma_level = CHROMA_LEVEL_2;
            }

            // Set eob based Full-Loop Escape Flag
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_1) {
                context_ptr->full_loop_escape = EB_FALSE;
            }
            else {
                if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER) {
                    context_ptr->full_loop_escape = EB_FALSE;
                }
                else {
                    context_ptr->full_loop_escape = EB_TRUE;
                }
            }

            // Set Fast-Loop Method
            context_ptr->single_fast_loop_flag = EB_TRUE;

            // Set Nearest Injection Flag
            context_ptr->nearest_injection = EB_TRUE;

            // Set Near Injection Flag
            context_ptr->near_injection = EB_TRUE;

            // Set Zero Injection Flag
            context_ptr->zero_injection = EB_TRUE;

            // Set Unipred 3x3 Injection Flag
            context_ptr->unipred3x3_injection = (picture_control_set_ptr->enc_mode <= ENC_MODE_1) ? EB_TRUE : EB_FALSE;

            // Set Bipred 3x3 Injection Flag
            context_ptr->bipred3x3_injection = (picture_control_set_ptr->enc_mode <= ENC_MODE_1) ? EB_TRUE : EB_FALSE;

            // Set Limit INTRA Flag
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
                context_ptr->limit_intra = EB_FALSE;
            }
            else {
                if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_FALSE) {
                    context_ptr->limit_intra = EB_TRUE;
                }
                else {
                    context_ptr->limit_intra = EB_FALSE;
                }
            }

            // Set PF @ MD Level
            // Level    Settings
            // 0        OFF
            // 1        N2
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_5) {
                context_ptr->pf_md_level = 0;
            }
            else if (picture_control_set_ptr->enc_mode <= ENC_MODE_7) {
                if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag) {
                    context_ptr->pf_md_level = 0;
                }
                else {
                    context_ptr->pf_md_level = 1;
                }
            }
            else {
                if (picture_control_set_ptr->temporal_layer_index == 0) {
                    context_ptr->pf_md_level = 0;
                }
                else {
                    context_ptr->pf_md_level = 1;
                }
            }

            // NFL Level MD         Settings
            // 0                    11
            // 1                    4
            // 2                    3
            // 3                    2
            // 4                    1
            if (picture_control_set_ptr->enc_mode <= ENC_MODE_2) {
                context_ptr->nfl_level = 0;
            }
            else if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
                context_ptr->nfl_level = 1;
            }
            else {
                context_ptr->nfl_level = 2;
            }

            return return_error;
        }

        /******************************************************
         * EncDec Kernel
         ******************************************************/
        void* eb_vp9_enc_dec_kernel(void *input_ptr)
        {

            EncDecContext          *context_ptr = (EncDecContext  *)input_ptr;
            PictureControlSet      *picture_control_set_ptr;
            SequenceControlSet     *sequence_control_set_ptr;
            EncodeContext          *encode_context_ptr;
            // Input
            EbObjectWrapper        *enc_dec_tasks_wrapper_ptr;
            EncDecTasks            *enc_dec_tasks_ptr;

            // Output
            EbObjectWrapper        *enc_dec_results_wrapper_ptr;
            EncDecResults          *enc_dec_results_ptr;
            EbObjectWrapper        *picture_demux_results_wrapper_ptr;
            PictureDemuxResults    *picture_demux_results_ptr;

            // SB Loop variables
            SbUnit                 *sb_ptr;
            uint16_t                sb_index;
            uint32_t                xsb_index;
            uint32_t                ysb_index;
            SbParams               *sb_params_ptr;

            EB_BOOL                 last_sb_flag;
            EB_BOOL                 end_of_row_flag;
            uint32_t                sb_row_index_start;
            uint32_t                sb_row_index_count;
            uint32_t                picture_width_in_sb;

            // Segments
            uint16_t                segment_index = 0;
            uint32_t                x_sb_start_index;
            uint32_t                y_sb_start_index;
            uint32_t                sb_start_index;
            uint32_t                sb_segment_count;
            uint32_t                sb_segment_index;
            uint32_t                segment_row_index;
            uint32_t                segment_band_index;
            uint32_t                segment_band_size;
            EncDecSegments         *segments_ptr;

            for (;;) {

                // Get Mode Decision Results
                eb_vp9_get_full_object(
                    context_ptr->mode_decision_input_fifo_ptr,
                    &enc_dec_tasks_wrapper_ptr);

                enc_dec_tasks_ptr = (EncDecTasks*)enc_dec_tasks_wrapper_ptr->object_ptr;
                picture_control_set_ptr = (PictureControlSet  *)enc_dec_tasks_ptr->picture_control_set_wrapper_ptr->object_ptr;
                sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
                encode_context_ptr = sequence_control_set_ptr->encode_context_ptr;
                segments_ptr = picture_control_set_ptr->enc_dec_segment_ctrl;
                last_sb_flag = EB_FALSE;

                context_ptr->is16bit = !(sequence_control_set_ptr->input_bit_depth == EB_8BIT);

                // SB Constants
                picture_width_in_sb = (sequence_control_set_ptr->luma_width + MAX_SB_SIZE_MINUS_1) >> LOG2F_MAX_SB_SIZE;
                end_of_row_flag = EB_FALSE;
                sb_row_index_start = sb_row_index_count = 0;

                // EncDec Kernel Signal(s) derivation
                if (sequence_control_set_ptr->static_config.tune == TUNE_SQ) {
                    eb_vp9_signal_derivation_enc_dec_kernel_sq(
                        sequence_control_set_ptr,
                        picture_control_set_ptr,
                        context_ptr);
                }
                else if (sequence_control_set_ptr->static_config.tune == TUNE_VMAF) {
                    eb_vp9_signal_derivation_enc_dec_kernel_vmaf(
                        sequence_control_set_ptr,
                        picture_control_set_ptr,
                        context_ptr);
                }
                else {
                    eb_vp9_signal_derivation_enc_dec_kernel_oq(
                        sequence_control_set_ptr,
                        picture_control_set_ptr,
                        context_ptr);
                }

                // Set valid ref_frame
                if (picture_control_set_ptr->slice_type != I_SLICE) {
                    EbReferenceObject      *reference_object;

                    if (picture_control_set_ptr->parent_pcs_ptr->cpi->common.reference_mode == SINGLE_REFERENCE || picture_control_set_ptr->parent_pcs_ptr->cpi->common.reference_mode == REFERENCE_MODE_SELECT) {
                        reference_object = (EbReferenceObject  *)picture_control_set_ptr->ref_pic_ptr_array[REF_LIST_0]->object_ptr;
#if USE_SRC_REF
                        context_ptr->ref_pic_list[REF_LIST_0] = (picture_control_set_ptr->parent_pcs_ptr->use_src_ref) ?
                            (EbPictureBufferDesc  *)reference_object->ref_den_src_picture :
                            (EbPictureBufferDesc  *)reference_object->reference_picture;
#else
                        context_ptr->ref_pic_list[REF_LIST_0] = (EbPictureBufferDesc  *)reference_object->reference_picture;
#endif
                    }
                    else {
                        context_ptr->ref_pic_list[REF_LIST_0] = (EbPictureBufferDesc  *)EB_NULL;
                    }

                    if (picture_control_set_ptr->parent_pcs_ptr->cpi->common.reference_mode == REFERENCE_MODE_SELECT) {
                        reference_object = (EbReferenceObject  *)picture_control_set_ptr->ref_pic_ptr_array[REF_LIST_1]->object_ptr;
#if USE_SRC_REF
                        context_ptr->ref_pic_list[REF_LIST_1] = (picture_control_set_ptr->parent_pcs_ptr->use_src_ref) ?
                            (EbPictureBufferDesc  *)reference_object->ref_den_src_picture :
                            (EbPictureBufferDesc  *)reference_object->reference_picture;
#else
                        context_ptr->ref_pic_list[REF_LIST_1] = (EbPictureBufferDesc  *)reference_object->reference_picture;
#endif
                    }
                    else {
                        context_ptr->ref_pic_list[REF_LIST_1] = (EbPictureBufferDesc  *)EB_NULL;
                    }
                }

                // Set recon
                if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE) {
                    context_ptr->recon_buffer = context_ptr->is16bit ?
                        ((EbReferenceObject  *)picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr->object_ptr)->reference_picture16bit :
                        ((EbReferenceObject  *)picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr->object_ptr)->reference_picture;
                }
                else {
                    context_ptr->recon_buffer = context_ptr->is16bit ?
                        picture_control_set_ptr->recon_picture_16bit_ptr :
                        picture_control_set_ptr->recon_picture_ptr;
                }

                // Set eob zero mode  flags
                context_ptr->skip_eob_zero_mode_ep = EB_TRUE;

                // Set eob zero mode flags
                context_ptr->eob_zero_mode = (sequence_control_set_ptr->static_config.tune != TUNE_SQ || picture_control_set_ptr->temporal_layer_index > 0);

                // Hsan: could be simplified
                eb_vp9_setup_scale_factors_for_frame(
                    context_ptr->sf,
                    picture_control_set_ptr->parent_pcs_ptr->cpi->common.width,      // Hsan: should be ref width
                    picture_control_set_ptr->parent_pcs_ptr->cpi->common.height,     // Hsan: should be ref height
                    picture_control_set_ptr->parent_pcs_ptr->cpi->common.width,
                    picture_control_set_ptr->parent_pcs_ptr->cpi->common.height);

                // Hsan: could move to resource coordination
                eb_vp9_tile_init(&context_ptr->e_mbd->tile, &(picture_control_set_ptr->parent_pcs_ptr->cpi->common), 0, 0);

                context_ptr->e_mbd->plane[0].subsampling_x = context_ptr->e_mbd->plane[0].subsampling_y = 0;
                context_ptr->e_mbd->plane[1].subsampling_x = context_ptr->e_mbd->plane[1].subsampling_y = 1;
                context_ptr->e_mbd->plane[2].subsampling_x = context_ptr->e_mbd->plane[2].subsampling_y = 1;

                context_ptr->e_mbd->lossless = 0;

                // Segment-loop
                while (assign_enc_dec_segments(segments_ptr, &segment_index, enc_dec_tasks_ptr, context_ptr->enc_dec_feedback_fifo_ptr) == EB_TRUE)
                {
                    x_sb_start_index = segments_ptr->x_start_array[segment_index];
                    y_sb_start_index = segments_ptr->y_start_array[segment_index];
                    sb_start_index = y_sb_start_index * picture_width_in_sb + x_sb_start_index;
                    sb_segment_count = segments_ptr->valid_sb_count_array[segment_index];

                    segment_row_index = segment_index / segments_ptr->segment_band_count;
                    segment_band_index = segment_index - segment_row_index * segments_ptr->segment_band_count;
                    segment_band_size = (segments_ptr->sb_band_count * (segment_band_index + 1) + segments_ptr->segment_band_count - 1) / segments_ptr->segment_band_count;

                    // Reset Coding Loop State
                    eb_vp9_reset_mode_decision(
                        context_ptr,
                        picture_control_set_ptr,
                        segment_index);

                    // Reset EncDec Coding State
                    reset_enc_dec(
                        context_ptr,
                        picture_control_set_ptr,
                        sequence_control_set_ptr,
                        segment_index);

                    if (picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr != NULL) {
                        ((EbReferenceObject  *)picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr->object_ptr)->pic_avg_variance = picture_control_set_ptr->parent_pcs_ptr->pic_avg_variance;
                        ((EbReferenceObject  *)picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr->object_ptr)->average_intensity = picture_control_set_ptr->parent_pcs_ptr->average_intensity[0];
                    }

                    for (ysb_index = y_sb_start_index, sb_segment_index = sb_start_index; sb_segment_index < sb_start_index + sb_segment_count; ++ysb_index) {
                        for (xsb_index = x_sb_start_index; xsb_index < picture_width_in_sb && (xsb_index + ysb_index < segment_band_size) && sb_segment_index < sb_start_index + sb_segment_count; ++xsb_index, ++sb_segment_index) {

                            sb_index = (uint16_t)(ysb_index * picture_width_in_sb + xsb_index);
                            sb_ptr = picture_control_set_ptr->sb_ptr_array[sb_index];
                            last_sb_flag = (sb_index == picture_control_set_ptr->sb_total_count - 1) ? EB_TRUE : EB_FALSE;
                            end_of_row_flag = (xsb_index == picture_width_in_sb - 1) ? EB_TRUE : EB_FALSE;
                            sb_row_index_start = (xsb_index == picture_width_in_sb - 1 && sb_row_index_count == 0) ? ysb_index : sb_row_index_start;
                            sb_row_index_count = (xsb_index == picture_width_in_sb - 1) ? sb_row_index_count + 1 : sb_row_index_count;
                            context_ptr->sb_index = sb_index;
                            sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];

                            // Derive restrict_intra_global_motion Flag
                            context_ptr->restrict_intra_global_motion = (sequence_control_set_ptr->static_config.tune == TUNE_SQ && (picture_control_set_ptr->parent_pcs_ptr->is_pan || picture_control_set_ptr->parent_pcs_ptr->is_tilt) && picture_control_set_ptr->parent_pcs_ptr->non_moving_index_array[sb_index] < INTRA_GLOBAL_MOTION_NON_MOVING_INDEX_TH && picture_control_set_ptr->parent_pcs_ptr->y_mean[sb_index][PA_RASTER_SCAN_CU_INDEX_64x64] < INTRA_GLOBAL_MOTION_DARK_SB_TH);

                            // Derive Interpoldation Method @ Mode Decision
                            context_ptr->use_subpel_flag = (picture_control_set_ptr->parent_pcs_ptr->use_subpel_flag == EB_FALSE) ?
                                EB_FALSE :
                                EB_TRUE;

#if SEG_SUPPORT
                            VP9_COMMON *const cm = &picture_control_set_ptr->parent_pcs_ptr->cpi->common;
                            struct segmentation *const seg = &cm->seg;
                            const int qindex = eb_vp9_get_qindex(seg, context_ptr->segment_id, picture_control_set_ptr->base_qindex);
#else
                            const int qindex = picture_control_set_ptr->base_qindex;
#endif
                            context_ptr->RDMULT = eb_vp9_compute_rd_mult(picture_control_set_ptr->parent_pcs_ptr->cpi, qindex);
                            context_ptr->rd_mult_sad = (int)MAX(round(sqrtf((float)context_ptr->RDMULT / 128) * 128), 1);

#if SEG_SUPPORT
                            context_ptr->segment_id = sb_ptr->segment_id;
#endif
                            if (
                                picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_FULL85_DEPTH_MODE ||
                                picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_FULL84_DEPTH_MODE ||
                                picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_OPEN_LOOP_DEPTH_MODE ||
                                (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_SB_SWITCH_DEPTH_MODE && (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_FULL85_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_FULL84_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_OPEN_LOOP_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_LIGHT_OPEN_LOOP_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_AVC_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_LIGHT_AVC_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_PRED_OPEN_LOOP_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE))) {

                                // MDC depth partitioning
                                context_ptr->depth_part_stage = 0;
                                eb_vp9_mode_decision_sb(
                                    sequence_control_set_ptr,
                                    picture_control_set_ptr,
                                    context_ptr,
                                    sb_ptr);

                            }
                            else {

                                // Pillar depth partitioning : 32x32 vs 16x16
                                context_ptr->depth_part_stage = 1;
                                bdp_pillar_sb(
                                    sequence_control_set_ptr,
                                    picture_control_set_ptr,
                                    context_ptr,
                                    sb_ptr);

                                // If all 4 quadrants are CU32x32, the compare the 4 CU32x32 to CU64x64
#if SHUT_64x64_BASE_RESTRICTION
                                EB_BOOL is_4_32x32 = (sb_params_ptr->is_complete_sb && (picture_control_set_ptr->temporal_layer_index > 0 || sequence_control_set_ptr->static_config.tune == TUNE_OQ) &&
#else
                                EB_BOOL is_4_32x32 = (sb_params_ptr->is_complete_sb && picture_control_set_ptr->temporal_layer_index > 0 &&
#endif
                                    sb_ptr->coded_block_array_ptr[5]->split_flag == EB_FALSE &&
                                    sb_ptr->coded_block_array_ptr[174]->split_flag == EB_FALSE &&
                                    sb_ptr->coded_block_array_ptr[343]->split_flag == EB_FALSE &&
                                    sb_ptr->coded_block_array_ptr[512]->split_flag == EB_FALSE);

                                // 64x64 refinement depth partitioning
                                context_ptr->depth_part_stage = 2;
                                if (picture_control_set_ptr->slice_type != I_SLICE && is_4_32x32) {
                                    bdp_64x64_vs_32x32_sb(
                                        sequence_control_set_ptr,
                                        picture_control_set_ptr,
                                        context_ptr,
                                        sb_ptr);
                                }

                                // 8x8 refinement depth partitioning
                                context_ptr->depth_part_stage = 2;
                                bdp_8x8_refinement_sb(
                                    sequence_control_set_ptr,
                                    picture_control_set_ptr,
                                    context_ptr,
                                    sb_ptr);

                                // Nearest/Near depth partitioning
                                context_ptr->depth_part_stage = 0;
                                bdp_nearest_near_sb(
                                    sequence_control_set_ptr,
                                    picture_control_set_ptr,
                                    context_ptr,
                                    sb_ptr);
                            }
#if VP9_PERFORM_EP
#if USE_SRC_REF
                            // Set valid ref_frame
                            if (picture_control_set_ptr->slice_type != I_SLICE) {
                                EbReferenceObject      *reference_object;

                                if (picture_control_set_ptr->parent_pcs_ptr->cpi->common.reference_mode == SINGLE_REFERENCE || picture_control_set_ptr->parent_pcs_ptr->cpi->common.reference_mode == REFERENCE_MODE_SELECT) {
                                    reference_object = (EbReferenceObject  *)picture_control_set_ptr->ref_pic_ptr_array[REF_LIST_0]->object_ptr;
                                    context_ptr->ref_pic_list[REF_LIST_0] = (EbPictureBufferDesc  *)reference_object->reference_picture;

                                }
                                else {
                                    context_ptr->ref_pic_list[REF_LIST_0] = (EbPictureBufferDesc  *)EB_NULL;
                                }

                                if (picture_control_set_ptr->parent_pcs_ptr->cpi->common.reference_mode == REFERENCE_MODE_SELECT) {
                                    reference_object = (EbReferenceObject  *)picture_control_set_ptr->ref_pic_ptr_array[REF_LIST_1]->object_ptr;
                                    context_ptr->ref_pic_list[REF_LIST_1] = (picture_control_set_ptr->parent_pcs_ptr->use_src_ref) ?
                                        (EbPictureBufferDesc  *)reference_object->ref_den_src_picture :
                                        (EbPictureBufferDesc  *)reference_object->reference_picture;
                                }
                                else {
                                    context_ptr->ref_pic_list[REF_LIST_1] = (EbPictureBufferDesc  *)EB_NULL;
                                }
                            }
#endif
                            // Derive Interpoldation Method @ Encode Pass
                            context_ptr->use_subpel_flag = EB_TRUE;

                            // Encode Pass
                            encode_pass_sb(
                                sequence_control_set_ptr,
                                picture_control_set_ptr,
                                context_ptr,
                                sb_ptr);
#endif

                        }
                        x_sb_start_index = (x_sb_start_index > 0) ? x_sb_start_index - 1 : 0;
                    }
                }

                if (last_sb_flag) {

                    struct loop_filter *lf = &picture_control_set_ptr->parent_pcs_ptr->cpi->common.lf;
                    // Initialize Loop Filter Parameters
                    lf->filter_level = 0;            // Hsan - Loop filter level (from 0 to 63) (default 16)
                    lf->sharpness_level = 0;         // Hsan - Loop filter sharpness (from 0 to 15) (default 4)
                    lf->last_sharpness_level = 0;
                    lf->mode_ref_delta_enabled = 0;

                    if (sequence_control_set_ptr->static_config.loop_filter) {

                        eb_vp9_pick_filter_level(
#if 0
                            picture_control_set_ptr->parent_pcs_ptr->cpi->Source,
#endif
                            picture_control_set_ptr->parent_pcs_ptr->cpi,
                            LPF_PICK_FROM_Q);

                        EB_BOOL lf_application_enable_flag;

                        if ((context_ptr->allow_enc_dec_mismatch == EB_TRUE || picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_FALSE) && sequence_control_set_ptr->static_config.recon_file == EB_FALSE) {
                            lf_application_enable_flag = EB_FALSE;
                        }
                        else {
                            lf_application_enable_flag = EB_TRUE;
                        }

                        if (lf_application_enable_flag) {
                            eb_vp9_loop_filter_init(&picture_control_set_ptr->parent_pcs_ptr->cpi->common);
                            eb_vp9_reset_lfm(&picture_control_set_ptr->parent_pcs_ptr->cpi->common);

                            // Set mi_grid_visible
                            picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_grid_visible = picture_control_set_ptr->mode_info_array;

                            context_ptr->e_mbd->plane[0].dst.buf = &context_ptr->recon_buffer->buffer_y[context_ptr->recon_buffer->origin_x + context_ptr->recon_buffer->stride_y * context_ptr->recon_buffer->origin_y];
                            context_ptr->e_mbd->plane[0].dst.stride = context_ptr->recon_buffer->stride_y;

                            context_ptr->e_mbd->plane[1].dst.buf = &context_ptr->recon_buffer->buffer_cb[(context_ptr->recon_buffer->origin_x >> 1) + context_ptr->recon_buffer->stride_cb * (context_ptr->recon_buffer->origin_y >> 1)];
                            context_ptr->e_mbd->plane[1].dst.stride = context_ptr->recon_buffer->stride_cb;

                            context_ptr->e_mbd->plane[2].dst.buf = &context_ptr->recon_buffer->buffer_cr[(context_ptr->recon_buffer->origin_x >> 1) + context_ptr->recon_buffer->stride_cr * (context_ptr->recon_buffer->origin_y >> 1)];
                            context_ptr->e_mbd->plane[2].dst.stride = context_ptr->recon_buffer->stride_cr;

                            eb_vp9_build_mask_frame(
                                &picture_control_set_ptr->parent_pcs_ptr->cpi->common,
                                lf->filter_level,
                                0);

                            eb_vp9_loop_filter_frame(
#if 0
                                cm->frame_to_show,
#endif
                                &picture_control_set_ptr->parent_pcs_ptr->cpi->common,
                                context_ptr->e_mbd,
                                lf->filter_level,
                                0,
                                0);
                        }
                    }

                    if (picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr != NULL) {
                        copy_statistics_to_ref_object(
                            picture_control_set_ptr);
                    }

                    // Pad the reference picture and set up TMVP flag and ref POC
                    if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE) {
                        pad_ref_and_set_flags(
                            picture_control_set_ptr,
                            sequence_control_set_ptr);
                    }

                    if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE && picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr)
                    {
                        EbPictureBufferDesc *input_picture_ptr = (EbPictureBufferDesc  *)picture_control_set_ptr->parent_pcs_ptr->enhanced_picture_ptr;
                        const uint32_t       src_luma_off_set = input_picture_ptr->origin_x + input_picture_ptr->origin_y    *input_picture_ptr->stride_y;
                        const uint32_t       src_cb_offset = (input_picture_ptr->origin_x >> 1) + (input_picture_ptr->origin_y >> 1)*input_picture_ptr->stride_cb;
                        const uint32_t       src_cr_offset = (input_picture_ptr->origin_x >> 1) + (input_picture_ptr->origin_y >> 1)*input_picture_ptr->stride_cr;

                        EbReferenceObject   *reference_object = (EbReferenceObject  *)picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr->object_ptr;
                        EbPictureBufferDesc *ref_den_pic = reference_object->ref_den_src_picture;
                        const uint32_t       ref_luma_off_set = ref_den_pic->origin_x + ref_den_pic->origin_y    *ref_den_pic->stride_y;
                        const uint32_t       ref_cb_offset = (ref_den_pic->origin_x >> 1) + (ref_den_pic->origin_y >> 1)*ref_den_pic->stride_cb;
                        const uint32_t       ref_cr_offset = (ref_den_pic->origin_x >> 1) + (ref_den_pic->origin_y >> 1)*ref_den_pic->stride_cr;

                        uint16_t  vertical_idx;

                        for (vertical_idx = 0; vertical_idx < ref_den_pic->height; ++vertical_idx)
                        {
                            EB_MEMCPY(ref_den_pic->buffer_y + ref_luma_off_set + vertical_idx * ref_den_pic->stride_y,
                                input_picture_ptr->buffer_y + src_luma_off_set + vertical_idx * input_picture_ptr->stride_y,
                                input_picture_ptr->width);
                        }

                        for (vertical_idx = 0; vertical_idx < input_picture_ptr->height / 2; ++vertical_idx)
                        {
                            EB_MEMCPY(ref_den_pic->buffer_cb + ref_cb_offset + vertical_idx * ref_den_pic->stride_cb,
                                input_picture_ptr->buffer_cb + src_cb_offset + vertical_idx * input_picture_ptr->stride_cb,
                                input_picture_ptr->width / 2);

                            EB_MEMCPY(ref_den_pic->buffer_cr + ref_cr_offset + vertical_idx * ref_den_pic->stride_cr,
                                input_picture_ptr->buffer_cr + src_cr_offset + vertical_idx * input_picture_ptr->stride_cr,
                                input_picture_ptr->width / 2);
                        }

                        eb_vp9_generate_padding(
                            ref_den_pic->buffer_y,
                            ref_den_pic->stride_y,
                            ref_den_pic->width,
                            ref_den_pic->height,
                            ref_den_pic->origin_x,
                            ref_den_pic->origin_y);

                        eb_vp9_generate_padding(
                            ref_den_pic->buffer_cb,
                            ref_den_pic->stride_cb,
                            ref_den_pic->width >> 1,
                            ref_den_pic->height >> 1,
                            ref_den_pic->origin_x >> 1,
                            ref_den_pic->origin_y >> 1);

                        eb_vp9_generate_padding(
                            ref_den_pic->buffer_cr,
                            ref_den_pic->stride_cr,
                            ref_den_pic->width >> 1,
                            ref_den_pic->height >> 1,
                            ref_den_pic->origin_x >> 1,
                            ref_den_pic->origin_y >> 1);
                    }

                    if (encode_context_ptr->recon_port_active) {
                        recon_output(
                            picture_control_set_ptr,
                            sequence_control_set_ptr);
                    }

                    if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag) {

                        // Get Empty EntropyCoding Results
                        eb_vp9_get_empty_object(
                            context_ptr->picture_demux_output_fifo_ptr,
                            &picture_demux_results_wrapper_ptr);

                        picture_demux_results_ptr = (PictureDemuxResults*)picture_demux_results_wrapper_ptr->object_ptr;
                        picture_demux_results_ptr->reference_picture_wrapper_ptr = picture_control_set_ptr->parent_pcs_ptr->reference_picture_wrapper_ptr;
                        picture_demux_results_ptr->sequence_control_set_wrapper_ptr = picture_control_set_ptr->sequence_control_set_wrapper_ptr;
                        picture_demux_results_ptr->picture_number = picture_control_set_ptr->picture_number;
                        picture_demux_results_ptr->picture_type = EB_PIC_REFERENCE;

                        // Post Reference Picture
                        eb_vp9_post_full_object(picture_demux_results_wrapper_ptr);
                    }

                    // When de interlacing is performed in the lib, each two consecutive pictures (fields: top & bottom) are going to use the same input buffer
                    // only when both fields are encoded we can free the input buffer
                    // using the current prediction structure, bottom fields are usually encoded after top fields
                    // so that when picture scan type is interlaced we free the input buffer after encoding the bottom field
                    // we are trying to avoid making a such change in the APP (ideally an input buffer live count should be set in the APP (under EbBufferHeaderType data structure))

                }

                // Send the Entropy Coder incremental updates as each LCU row becomes available
                if (end_of_row_flag == EB_TRUE) {

                    // Get Empty EncDec Results
                    eb_vp9_get_empty_object(
                        context_ptr->enc_dec_output_fifo_ptr,
                        &enc_dec_results_wrapper_ptr);
                    enc_dec_results_ptr = (EncDecResults*)enc_dec_results_wrapper_ptr->object_ptr;
                    enc_dec_results_ptr->picture_control_set_wrapper_ptr = enc_dec_tasks_ptr->picture_control_set_wrapper_ptr;
                    enc_dec_results_ptr->completed_sb_row_index_start = sb_row_index_start;
                    enc_dec_results_ptr->completed_sb_row_count = sb_row_index_count;

                    // Post EncDec Results
                    eb_vp9_post_full_object(enc_dec_results_wrapper_ptr);
                }

                // Release Mode Decision Results
                eb_vp9_release_object(enc_dec_tasks_wrapper_ptr);

            }
            return EB_NULL;
        }
