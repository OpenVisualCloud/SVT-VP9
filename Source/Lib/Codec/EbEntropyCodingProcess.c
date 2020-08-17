/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbEntropyCodingProcess.h"
#include "EbEncDecResults.h"
#include "EbEntropyCodingResults.h"
#include "EbRateControlTasks.h"

#include "vp9_blockd.h"
#include "vp9_encoder.h"
#include "stdint.h"//#include "bitwriter_buffer.h"
#include "vp9_bitstream.h"
#include "vp9_enums.h"
#include "bitwriter.h"
#if SEG_SUPPORT
#include "vp9_segmentation.h"

#endif
/******************************************************
 * Enc Dec Context Constructor
 ******************************************************/
EbErrorType eb_vp9_entropy_coding_context_ctor(
    EntropyCodingContext **context_dbl_ptr,
    EbFifo                *enc_dec_input_fifo_ptr,
    EbFifo                *packetization_output_fifo_ptr,
    EbFifo                *rate_control_output_fifo_ptr,
    EB_BOOL                is16bit)
{
    EntropyCodingContext *context_ptr;
    EB_MALLOC(EntropyCodingContext*, context_ptr, sizeof(EntropyCodingContext), EB_N_PTR);
    *context_dbl_ptr = context_ptr;

    context_ptr->is16bit = is16bit;

    // Input/Output System Resource Manager FIFOs
    context_ptr->enc_dec_input_fifo_ptr          = enc_dec_input_fifo_ptr;
    context_ptr->entropy_coding_output_fifo_ptr  = packetization_output_fifo_ptr;
    context_ptr->rate_control_output_fifo_ptr    = rate_control_output_fifo_ptr;

    EB_MALLOC(MACROBLOCKD*, context_ptr->e_mbd       , sizeof(MACROBLOCKD), EB_N_PTR);
    EB_MALLOC(ModeInfo **, context_ptr->e_mbd->mi   , sizeof(ModeInfo *), EB_N_PTR);

    EB_MALLOC(ENTROPY_CONTEXT*, context_ptr->e_mbd->plane[0].above_context, sizeof(ENTROPY_CONTEXT) * 16, EB_N_PTR);
    EB_MALLOC(ENTROPY_CONTEXT*, context_ptr->e_mbd->plane[0].left_context , sizeof(ENTROPY_CONTEXT) * 16, EB_N_PTR);

    EB_MALLOC(ENTROPY_CONTEXT*, context_ptr->e_mbd->plane[1].above_context, sizeof(ENTROPY_CONTEXT) * 16, EB_N_PTR);
    EB_MALLOC(ENTROPY_CONTEXT*, context_ptr->e_mbd->plane[1].left_context , sizeof(ENTROPY_CONTEXT) * 16, EB_N_PTR);

    EB_MALLOC(ENTROPY_CONTEXT*, context_ptr->e_mbd->plane[2].above_context, sizeof(ENTROPY_CONTEXT) * 16, EB_N_PTR);
    EB_MALLOC(ENTROPY_CONTEXT*, context_ptr->e_mbd->plane[2].left_context , sizeof(ENTROPY_CONTEXT) * 16, EB_N_PTR);
    // Hsan - how many token do we really need ?
    EB_MALLOC(TOKENEXTRA*, context_ptr->tok, sizeof(TOKENEXTRA) * MAX_CU_SIZE * MAX_CU_SIZE * MAX_MB_PLANE, EB_N_PTR);

    return EB_ErrorNone;
}

/**********************************************
* Entropy Coding SB
**********************************************/
EbErrorType EntropyCodingSb(
    PictureControlSet    *picture_control_set_ptr,
    EntropyCodingContext *context_ptr,
    SbUnit               *sb_ptr,
    EntropyCoder         *entropy_coder_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    SequenceControlSet      *sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;

    VpxWriter                *residual_bc = &(entropy_coder_ptr->residual_bc);
    VP9_COMP                 *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
    VP9_COMMON               *const cm = &cpi->common;
    MACROBLOCKD              *const xd = context_ptr->e_mbd;
    OutputBitstreamUnit      *output_bitstream_ptr = (OutputBitstreamUnit  *)(picture_control_set_ptr->entropy_coder_ptr->ec_output_bitstream_ptr);

    uint32_t                  block_index = 0;
    uint32_t                  rasterScanIndex;
    uint16_t                  valid_block_index;

    // Set mi_grid_visible
    cm->mi_grid_visible = picture_control_set_ptr->mode_info_array;

    // Get SB Params
    SbParams * lcuParam = &sequence_control_set_ptr->sb_params_array[sb_ptr->sb_index];

    // Reset residual_bc each SB
    uint8_t *data = output_bitstream_ptr->buffer;
    size_t   total_size = 0;

    // Reset above context @ the 1st SB
    if (sb_ptr->sb_index == 0) {

        eb_vp9_start_encode(residual_bc, data + total_size);

        // Note: this memset assumes above_context[0], [1] and [2]
        // are allocated as part of the same buffer.
        memset(cm->above_context, 0, sizeof(*cm->above_context) * MAX_MB_PLANE * 2 * mi_cols_aligned_to_sb(cm->mi_cols));
        memset(cm->above_seg_context, 0, sizeof(*cm->above_seg_context) * mi_cols_aligned_to_sb(cm->mi_cols));
    }

    // Reset left context @ each row of SB
    if (sb_ptr->origin_x == 0) {
        // Initialize the left context for the new SB row
        memset(&xd->left_context, 0, sizeof(xd->left_context));
        memset(xd->left_seg_context, 0, sizeof(xd->left_seg_context));
        set_partition_probs(cm, xd);
    }
#if VP9_PERFORM_EP
    sb_ptr->quantized_coeff_buffer_block_offset[0] = sb_ptr->quantized_coeff_buffer_block_offset[1] = sb_ptr->quantized_coeff_buffer_block_offset[2] = 0;
#endif
    do {

        context_ptr->block_ptr = sb_ptr->coded_block_array_ptr[block_index];
        context_ptr->ep_block_stats_ptr = ep_get_block_stats(block_index);
        rasterScanIndex = ep_scan_to_raster_scan[block_index];
        context_ptr->block_width = context_ptr->ep_block_stats_ptr->bwidth;
        context_ptr->block_height = context_ptr->ep_block_stats_ptr->bheight;
        context_ptr->block_origin_x = (uint16_t)sb_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_x;
        context_ptr->block_origin_y = (uint16_t)sb_ptr->origin_y + context_ptr->ep_block_stats_ptr->origin_y;
        context_ptr->mi_col = context_ptr->block_origin_x >> MI_SIZE_LOG2;
        context_ptr->mi_row = context_ptr->block_origin_y >> MI_SIZE_LOG2;

        // Derive partition using block validity and split_flag
        // ec_scan_block_valid_block holds the block index for the first valid block/subblock
        // covered by the block (at any depth).  A block is valid if it is within the frame boundary.
        valid_block_index = lcuParam->ec_scan_block_valid_block[block_index];

        PARTITION_TYPE partition;
        if (valid_block_index == (uint16_t)~0) {
            partition = PARTITION_INVALID;
        }
        else {
            if (valid_block_index == block_index) {
                if(context_ptr->block_ptr->split_flag == EB_TRUE)
                    partition = PARTITION_SPLIT;
                else
                    partition = PARTITION_NONE;
            }
            else {
                partition = PARTITION_SPLIT;
            }
        }

        if (partition != PARTITION_INVALID) {
            const int  bsl = eb_vp9_b_width_log2_lookup[context_ptr->ep_block_stats_ptr->bsize];
            const int  bs = (1 << bsl) / 4;
            BLOCK_SIZE subsize = get_subsize(context_ptr->ep_block_stats_ptr->bsize, partition);

            // From SVT to WebM (symbols)
            xd->mi[0] = cm->mi_grid_visible[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride];

            vp9_init_macroblockd(cm, xd, NULL);

            // Write partition
            write_partition(cm, xd, bs, context_ptr->mi_row, context_ptr->mi_col, partition, context_ptr->ep_block_stats_ptr->bsize, residual_bc);

            // Write mode info for final partitioning
            if (partition == PARTITION_NONE || (partition == PARTITION_SPLIT && ep_get_block_stats(block_index)->bsize == BLOCK_8X8)) {

                // Set above_mi and left_mi
                context_ptr->e_mbd->above_mi = (context_ptr->mi_row > 0) ? cm->mi_grid_visible[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - cm->mi_stride] : NULL;
                context_ptr->e_mbd->left_mi = (context_ptr->mi_col > 0) ? cm->mi_grid_visible[context_ptr->mi_col + context_ptr->mi_row * cm->mi_stride - 1] : NULL;

                // Set WebM relevant fields
                xd->mb_to_top_edge = -((context_ptr->mi_row * MI_SIZE) * 8);
                xd->mb_to_bottom_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_rows - eb_vp9_num_8x8_blocks_high_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_row) * MI_SIZE) * 8;
                xd->mb_to_left_edge = -((context_ptr->mi_col * MI_SIZE) * 8);
                xd->mb_to_right_edge = ((picture_control_set_ptr->parent_pcs_ptr->cpi->common.mi_cols - eb_vp9_num_8x8_blocks_wide_lookup[context_ptr->ep_block_stats_ptr->bsize] - context_ptr->mi_col) * MI_SIZE) * 8;

                xd->plane[0].subsampling_x = xd->plane[0].subsampling_y = 0;
                xd->plane[1].subsampling_x = xd->plane[1].subsampling_y = 1;
                xd->plane[2].subsampling_x = xd->plane[2].subsampling_y = 1;

                xd->lossless = 0;

                // Get eobs
                if (xd->mi[0]->sb_type < BLOCK_8X8) {
                    cpi->td.mb.plane[0].eobs[0] = sb_ptr->coded_block_array_ptr[block_index + ep_inter_depth_offset    ]->eob[0][0];
                    cpi->td.mb.plane[0].eobs[1] = sb_ptr->coded_block_array_ptr[block_index + ep_inter_depth_offset + 1]->eob[0][0];
                    cpi->td.mb.plane[0].eobs[2] = sb_ptr->coded_block_array_ptr[block_index + ep_inter_depth_offset + 2]->eob[0][0];
                    cpi->td.mb.plane[0].eobs[3] = sb_ptr->coded_block_array_ptr[block_index + ep_inter_depth_offset + 3]->eob[0][0];

                    cpi->td.mb.plane[1].eobs[0] = sb_ptr->coded_block_array_ptr[block_index + ep_inter_depth_offset + 3]->eob[1][0];
                    cpi->td.mb.plane[2].eobs[0] = sb_ptr->coded_block_array_ptr[block_index + ep_inter_depth_offset + 3]->eob[2][0];
                }
                else {
                    for (uint8_t tu_index = 0; tu_index < ((context_ptr->ep_block_stats_ptr->sq_size == MAX_SB_SIZE) ? 4 : 1); tu_index++) {
                        cpi->td.mb.plane[0].eobs[tu_index * 64] = context_ptr->block_ptr->eob[0][tu_index];
                    }
                    cpi->td.mb.plane[1].eobs[0] = context_ptr->block_ptr->eob[1][0];
                    cpi->td.mb.plane[2].eobs[0] = context_ptr->block_ptr->eob[2][0];
                }

                // From SVT to WebM (coeff)
#if VP9_PERFORM_EP
                cpi->td.mb.plane[0].qcoeff = &(((int16_t*)sb_ptr->quantized_coeff_buffer[0])[sb_ptr->quantized_coeff_buffer_block_offset[0]]);
                cpi->td.mb.plane[1].qcoeff = &(((int16_t*)sb_ptr->quantized_coeff_buffer[1])[sb_ptr->quantized_coeff_buffer_block_offset[1]]);
                cpi->td.mb.plane[2].qcoeff = &(((int16_t*)sb_ptr->quantized_coeff_buffer[2])[sb_ptr->quantized_coeff_buffer_block_offset[2]]);

                sb_ptr->quantized_coeff_buffer_block_offset[0] += (context_ptr->ep_block_stats_ptr->sq_size * context_ptr->ep_block_stats_ptr->sq_size );
                sb_ptr->quantized_coeff_buffer_block_offset[1] += (context_ptr->ep_block_stats_ptr->sq_size * context_ptr->ep_block_stats_ptr->sq_size ) >> 2;
                sb_ptr->quantized_coeff_buffer_block_offset[2] += (context_ptr->ep_block_stats_ptr->sq_size * context_ptr->ep_block_stats_ptr->sq_size ) >> 2;
#else
                if (xd->mi[0]->skip == EB_FALSE)
                {

                    if (xd->mi[0]->sb_type < BLOCK_8X8) {

                        // Y: 1st mbi
                        {
                            int16_t* src_ptr = &(((int16_t*)sb_ptr->quantized_coeff->buffer_y)[context_ptr->ep_block_stats_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_y * sb_ptr->quantized_coeff->stride_y]);
                            tran_low_t* dst_ptr = &(((tran_low_t*)cpi->td.mb.plane[0].qcoeff)[0]);

                            for (int j = 0; j < 4; j++) {
                                EB_MEMCPY(dst_ptr, src_ptr, 4 * sizeof(int16_t));
                                src_ptr = src_ptr + sb_ptr->quantized_coeff->stride_y;
                                dst_ptr = dst_ptr + 4;
                            }
                        }

                        // Y: 2nd mbi
                        {
                            int16_t* src_ptr = &(((int16_t*)sb_ptr->quantized_coeff->buffer_y)[context_ptr->ep_block_stats_ptr->origin_x + 4 + context_ptr->ep_block_stats_ptr->origin_y * sb_ptr->quantized_coeff->stride_y]);
                            tran_low_t* dst_ptr = &(((tran_low_t*)cpi->td.mb.plane[0].qcoeff)[16]);

                            for (int j = 0; j < 4; j++) {
                                EB_MEMCPY(dst_ptr, src_ptr, 4 * sizeof(int16_t));
                                src_ptr = src_ptr + sb_ptr->quantized_coeff->stride_y;
                                dst_ptr = dst_ptr + 4;
                            }
                        }

                        // Y: 3rd mbi
                        {
                            int16_t* src_ptr = &(((int16_t*)sb_ptr->quantized_coeff->buffer_y)[context_ptr->ep_block_stats_ptr->origin_x + (context_ptr->ep_block_stats_ptr->origin_y + 4) * sb_ptr->quantized_coeff->stride_y]);
                            tran_low_t* dst_ptr = &(((tran_low_t*)cpi->td.mb.plane[0].qcoeff)[32]);

                            for (int j = 0; j < 4; j++) {
                                EB_MEMCPY(dst_ptr, src_ptr, 4 * sizeof(int16_t));
                                src_ptr = src_ptr + sb_ptr->quantized_coeff->stride_y;
                                dst_ptr = dst_ptr + 4;
                            }
                        }

                        // Y: 4th mbi
                        {
                            int16_t* src_ptr = &(((int16_t*)sb_ptr->quantized_coeff->buffer_y)[context_ptr->ep_block_stats_ptr->origin_x + 4 + (context_ptr->ep_block_stats_ptr->origin_y + 4) * sb_ptr->quantized_coeff->stride_y]);
                            tran_low_t* dst_ptr = &(((tran_low_t*)cpi->td.mb.plane[0].qcoeff)[48]);

                            for (int j = 0; j < 4; j++) {
                                EB_MEMCPY(dst_ptr, src_ptr, 4 * sizeof(int16_t));
                                src_ptr = src_ptr + sb_ptr->quantized_coeff->stride_y;
                                dst_ptr = dst_ptr + 4;
                            }
                        }

#if 1
                        //Cb
                        {
                            int16_t* src_ptr = &(((int16_t*)sb_ptr->quantized_coeff->buffer_cb)[(context_ptr->ep_block_stats_ptr->origin_x >> 1) + (context_ptr->ep_block_stats_ptr->origin_y >> 1) * sb_ptr->quantized_coeff->stride_cb]);
                            tran_low_t* dst_ptr = &(((tran_low_t*)cpi->td.mb.plane[1].qcoeff)[0]);

                            for (int j = 0; j < context_ptr->ep_block_stats_ptr->sq_size_uv; j++) {
                                EB_MEMCPY(dst_ptr, src_ptr, context_ptr->ep_block_stats_ptr->sq_size_uv * sizeof(int16_t));
                                src_ptr = src_ptr + sb_ptr->quantized_coeff->stride_cb;
                                dst_ptr = dst_ptr + context_ptr->ep_block_stats_ptr->sq_size_uv;
                            }
                        }

                        //Cr
                        {
                            int16_t* src_ptr = &(((int16_t*)sb_ptr->quantized_coeff->buffer_cr)[(context_ptr->ep_block_stats_ptr->origin_x >> 1) + (context_ptr->ep_block_stats_ptr->origin_y >> 1) * sb_ptr->quantized_coeff->stride_cr]);
                            tran_low_t* dst_ptr = &(((tran_low_t*)cpi->td.mb.plane[2].qcoeff)[0]);

                            for (int j = 0; j < context_ptr->ep_block_stats_ptr->sq_size_uv; j++) {
                                EB_MEMCPY(dst_ptr, src_ptr, context_ptr->ep_block_stats_ptr->sq_size_uv * sizeof(int16_t));
                                src_ptr = src_ptr + sb_ptr->quantized_coeff->stride_cr;
                                dst_ptr = dst_ptr + context_ptr->ep_block_stats_ptr->sq_size_uv;
                            }
                        }
#endif

                    }
                    else {
                        //Y
                        {
                            int16_t* src_ptr = &(((int16_t*)sb_ptr->quantized_coeff->buffer_y)[context_ptr->ep_block_stats_ptr->origin_x + context_ptr->ep_block_stats_ptr->origin_y * sb_ptr->quantized_coeff->stride_y]);
                            tran_low_t* dst_ptr = &(((tran_low_t*)cpi->td.mb.plane[0].qcoeff)[0]);

                            for (int j = 0; j < context_ptr->ep_block_stats_ptr->sq_size; j++) {
                                EB_MEMCPY(dst_ptr, src_ptr, context_ptr->ep_block_stats_ptr->sq_size * sizeof(int16_t));
                                src_ptr = src_ptr + sb_ptr->quantized_coeff->stride_y;
                                dst_ptr = dst_ptr + context_ptr->ep_block_stats_ptr->sq_size;
                            }
                        }

                        //Cb
                        {
                            int16_t* src_ptr = &(((int16_t*)sb_ptr->quantized_coeff->buffer_cb)[(context_ptr->ep_block_stats_ptr->origin_x >> 1) + (context_ptr->ep_block_stats_ptr->origin_y >> 1) * sb_ptr->quantized_coeff->stride_cb]);
                            tran_low_t* dst_ptr = &(((tran_low_t*)cpi->td.mb.plane[1].qcoeff)[0]);

                            for (int j = 0; j < context_ptr->ep_block_stats_ptr->sq_size_uv; j++) {
                                EB_MEMCPY(dst_ptr, src_ptr, context_ptr->ep_block_stats_ptr->sq_size_uv * sizeof(int16_t));
                                src_ptr = src_ptr + sb_ptr->quantized_coeff->stride_cb;
                                dst_ptr = dst_ptr + context_ptr->ep_block_stats_ptr->sq_size_uv;
                            }
                        }

                        //Cr
                        {
                            int16_t* src_ptr = &(((int16_t*)sb_ptr->quantized_coeff->buffer_cr)[(context_ptr->ep_block_stats_ptr->origin_x >> 1) + (context_ptr->ep_block_stats_ptr->origin_y >> 1) * sb_ptr->quantized_coeff->stride_cr]);
                            tran_low_t* dst_ptr = &(((tran_low_t*)cpi->td.mb.plane[2].qcoeff)[0]);

                            for (int j = 0; j < context_ptr->ep_block_stats_ptr->sq_size_uv; j++) {
                                EB_MEMCPY(dst_ptr, src_ptr, context_ptr->ep_block_stats_ptr->sq_size_uv * sizeof(int16_t));
                                src_ptr = src_ptr + sb_ptr->quantized_coeff->stride_cr;
                                dst_ptr = dst_ptr + context_ptr->ep_block_stats_ptr->sq_size_uv;
                            }
                        }
                    }
                }
#endif

                // Set skip context
                set_skip_context(xd, context_ptr->mi_row, context_ptr->mi_col);

                // Track the head of the tok buffer
                context_ptr->tok_start = context_ptr->tok;

                // Tokonize the SB
                eb_vp9_tokenize_sb(cpi, xd, &cpi->td, &context_ptr->tok, 0, 0,  VPXMAX(context_ptr->ep_block_stats_ptr->bsize, BLOCK_8X8));

                // Add EOSB_TOKEN
                context_ptr->tok->token  = EOSB_TOKEN;
                context_ptr->tok_end     = context_ptr->tok + 1;

                // Reset the tok buffer
                context_ptr->tok         = context_ptr->tok_start;

                // Write mode info
                eb_vp9_write_modes_b(context_ptr, cpi, xd, (TileInfo *)EB_NULL, residual_bc, &context_ptr->tok, context_ptr->tok_end, context_ptr->mi_row, context_ptr->mi_col, &cpi->max_mv_magnitude, cpi->interp_filter_selected);

                // Reset the tok buffer
                context_ptr->tok = context_ptr->tok_start;

                // Update partition context
                if (context_ptr->ep_block_stats_ptr->bsize >= BLOCK_8X8 && (context_ptr->ep_block_stats_ptr->bsize == BLOCK_8X8 || partition != PARTITION_SPLIT))
                    update_partition_context(xd, context_ptr->mi_row, context_ptr->mi_col, subsize, context_ptr->ep_block_stats_ptr->bsize);

                // Next CU in the current depth
                block_index += ep_intra_depth_offset[ep_raster_scan_block_depth[rasterScanIndex]];
            }
            else {
                // Next CU in the next depth
                block_index = block_index + ep_inter_depth_offset;
            }
        }
        else {

            if (context_ptr->block_origin_x >= sequence_control_set_ptr->luma_width || context_ptr->block_origin_y >= sequence_control_set_ptr->luma_height) {
                // Next CU in the current depth
                block_index += ep_intra_depth_offset[ep_raster_scan_block_depth[rasterScanIndex]];
            }
            else {
                // Next CU in the next depth
                block_index = block_index + ep_inter_depth_offset;
            }
        }

    } while (block_index < EP_BLOCK_MAX_COUNT);

    // Stop writing
    if (sb_ptr->sb_index == (unsigned) picture_control_set_ptr->sb_total_count - 1) {
        eb_vp9_stop_encode(residual_bc);
    }

    return return_error;
}

/******************************************************
 * Update Entropy Coding Rows
 *
 * This function is responsible for synchronizing the
 *   processing of Entropy Coding LCU-rows and starts
 *   processing of LCU-rows as soon as their inputs are
 *   available and the previous LCU-row has completed.
 *   At any given time, only one segment row per picture
 *   is being processed.
 *
 * The function has two parts:
 *
 * (1) Update the available row index which tracks
 *   which LCU Row-inputs are available.
 *
 * (2) Increment the lcu-row counter as the segment-rows
 *   are completed.
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
static EB_BOOL update_entropy_coding_rows(
    PictureControlSet *picture_control_set_ptr,
    uint32_t          *row_index,
    uint32_t           row_count,
    EB_BOOL           *initial_process_call)
{
    EB_BOOL process_next_row = EB_FALSE;

    // Note, any writes & reads to status variables (e.g. in_progress) in MD-CTRL must be thread-safe
    eb_vp9_block_on_mutex(picture_control_set_ptr->entropy_coding_mutex);

    // Update availability mask
    if (*initial_process_call == EB_TRUE) {
        unsigned i;

        for(i=*row_index; i < *row_index + row_count; ++i) {
            picture_control_set_ptr->entropy_coding_row_array[i] = EB_TRUE;
        }

        while(picture_control_set_ptr->entropy_coding_row_array[picture_control_set_ptr->entropy_coding_current_available_row] == EB_TRUE &&
              picture_control_set_ptr->entropy_coding_current_available_row < picture_control_set_ptr->entropy_coding_row_count)
        {
            ++picture_control_set_ptr->entropy_coding_current_available_row;
        }
    }

    // Release in_progress token
    if(*initial_process_call == EB_FALSE && picture_control_set_ptr->entropy_coding_in_progress == EB_TRUE) {
        picture_control_set_ptr->entropy_coding_in_progress = EB_FALSE;
    }

    // Test if the picture is not already complete AND not currently being worked on by another ENCDEC process
    if(picture_control_set_ptr->entropy_coding_current_row < picture_control_set_ptr->entropy_coding_row_count &&
       picture_control_set_ptr->entropy_coding_row_array[picture_control_set_ptr->entropy_coding_current_row] == EB_TRUE &&
       picture_control_set_ptr->entropy_coding_in_progress == EB_FALSE)
    {
        // Test if the next LCU-row is ready to go
        if(picture_control_set_ptr->entropy_coding_current_row <= picture_control_set_ptr->entropy_coding_current_available_row)
        {
            picture_control_set_ptr->entropy_coding_in_progress = EB_TRUE;
            *row_index = picture_control_set_ptr->entropy_coding_current_row++;
            process_next_row = EB_TRUE;
        }
    }

    *initial_process_call = EB_FALSE;

    eb_vp9_release_mutex(picture_control_set_ptr->entropy_coding_mutex);

    return process_next_row;
}

/******************************************************
 * Entropy Coding Kernel
 ******************************************************/
void* eb_vp9_entropy_coding_kernel(void *input_ptr)
{
    // Context & SCS & PCS
    EntropyCodingContext *context_ptr = (EntropyCodingContext*) input_ptr;
    PictureControlSet    *picture_control_set_ptr;
    SequenceControlSet   *sequence_control_set_ptr;

    // Input
    EbObjectWrapper      *enc_dec_results_wrapper_ptr;
    EncDecResults        *enc_dec_results_ptr;

    // Output
    EbObjectWrapper      *entropy_coding_results_wrapper_ptr;
    EntropyCodingResults *entropy_coding_results_ptr;

    // LCU Loop variables
    SbUnit               *sb_ptr;
    uint16_t              sb_index;
    uint32_t              xsb_index;
    uint32_t              ysb_index;
    uint32_t              picture_width_in_sb;
    // Variables
    EB_BOOL               initial_process_call;

    for(;;) {

        // Get Mode Decision Results
        eb_vp9_get_full_object(
            context_ptr->enc_dec_input_fifo_ptr,
            &enc_dec_results_wrapper_ptr);
        enc_dec_results_ptr       = (EncDecResults*) enc_dec_results_wrapper_ptr->object_ptr;
        picture_control_set_ptr   = (PictureControlSet  *) enc_dec_results_ptr->picture_control_set_wrapper_ptr->object_ptr;
        sequence_control_set_ptr  = (SequenceControlSet *) picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;

        // LCU Constants
        picture_width_in_sb = (sequence_control_set_ptr->luma_width + MAX_SB_SIZE_MINUS_1) >> LOG2F_MAX_SB_SIZE;
        {
            initial_process_call = EB_TRUE;
            ysb_index = enc_dec_results_ptr->completed_sb_row_index_start;

            // LCU-loops
            while(update_entropy_coding_rows(picture_control_set_ptr, &ysb_index, enc_dec_results_ptr->completed_sb_row_count, &initial_process_call) == EB_TRUE)
            {
                uint32_t rowsb_total_bits = 0;

                if(ysb_index == 0) {
                    picture_control_set_ptr->entropy_coding_pic_done = EB_FALSE;
                }

                for(xsb_index = 0; xsb_index < picture_width_in_sb; ++xsb_index)
                {

                    sb_index = (uint16_t)(xsb_index + ysb_index * picture_width_in_sb);
                    sb_ptr = picture_control_set_ptr->sb_ptr_array[sb_index];

                    sb_ptr->sb_total_bits = 0;
#if VP9_RC
                    uint32_t prev_pos = sb_index ? picture_control_set_ptr->entropy_coder_ptr->residual_bc.pos : 0;
#endif
#if SEG_SUPPORT
                    if (sb_index == 0) {
                        MACROBLOCKD *xd = NULL;
                        VP9_COMMON *const cm = &picture_control_set_ptr->parent_pcs_ptr->cpi->common;

                        struct segmentation *seg = &cm->seg;
                        if (seg->update_map) {
                            vpx_prob no_pred_tree[SEG_TREE_PROBS];
                            memset(seg->tree_probs, 255, sizeof(seg->tree_probs));
                            memset(seg->pred_probs, 255, sizeof(seg->pred_probs));
                            // Work out probability tree for coding segments without prediction
                            calc_segtree_probs(picture_control_set_ptr->segment_counts, no_pred_tree);
                            seg->temporal_update = 0;
                            EB_MEMCPY(seg->tree_probs, no_pred_tree, sizeof(no_pred_tree));

                        }
                    }
#endif
                    // Entropy Coding
                    EntropyCodingSb(
                        picture_control_set_ptr,
                        context_ptr,
                        sb_ptr,
                        picture_control_set_ptr->entropy_coder_ptr);
#if VP9_RC
                    sb_ptr->sb_total_bits = (picture_control_set_ptr->entropy_coder_ptr->residual_bc.pos - prev_pos)<<3;
                    picture_control_set_ptr->parent_pcs_ptr->quantized_coeff_num_bits += sb_ptr->sb_total_bits;
#endif

                    rowsb_total_bits += sb_ptr->sb_total_bits;
                }

                // At the end of each LCU-row, send the updated bit-count to Entropy Coding
                {
                    EbObjectWrapper  *rate_control_task_wrapper_ptr;
                    RateControlTasks *rate_control_task_ptr;

                    // Get Empty EncDec Results
                    eb_vp9_get_empty_object(
                        context_ptr->rate_control_output_fifo_ptr,
                        &rate_control_task_wrapper_ptr);
                    rate_control_task_ptr = (RateControlTasks*) rate_control_task_wrapper_ptr->object_ptr;
                    rate_control_task_ptr->task_type = RC_ENTROPY_CODING_ROW_FEEDBACK_RESULT;
                    rate_control_task_ptr->picture_number = picture_control_set_ptr->picture_number;
                    rate_control_task_ptr->row_number = ysb_index;
                    rate_control_task_ptr->bit_count = rowsb_total_bits;

                    rate_control_task_ptr->picture_control_set_wrapper_ptr = 0;
                    rate_control_task_ptr->segment_index = ~0u;

                    // Post EncDec Results
                    eb_vp9_post_full_object(rate_control_task_wrapper_ptr);
                }

                eb_vp9_block_on_mutex(picture_control_set_ptr->entropy_coding_mutex);
                if (picture_control_set_ptr->entropy_coding_pic_done == EB_FALSE) {

                    // If the picture is complete, terminate the slice
                    if (picture_control_set_ptr->entropy_coding_current_row == picture_control_set_ptr->entropy_coding_row_count)
                    {
                        uint32_t ref_idx;

                        picture_control_set_ptr->entropy_coding_pic_done = EB_TRUE;

                        // Release the List 0 Reference Pictures
                        for (ref_idx = 0; ref_idx < picture_control_set_ptr->parent_pcs_ptr->ref_list0_count; ++ref_idx) {
                            if (picture_control_set_ptr->ref_pic_ptr_array[0] != EB_NULL) {

                                eb_vp9_release_object(picture_control_set_ptr->ref_pic_ptr_array[0]);
                            }
                        }

                        // Release the List 1 Reference Pictures
                        for (ref_idx = 0; ref_idx < picture_control_set_ptr->parent_pcs_ptr->ref_list1_count; ++ref_idx) {
                            if (picture_control_set_ptr->ref_pic_ptr_array[1] != EB_NULL) {

                                eb_vp9_release_object(picture_control_set_ptr->ref_pic_ptr_array[1]);
                            }
                        }

                        // Get Empty Entropy Coding Results
                        eb_vp9_get_empty_object(
                            context_ptr->entropy_coding_output_fifo_ptr,
                            &entropy_coding_results_wrapper_ptr);
                        entropy_coding_results_ptr = (EntropyCodingResults*)entropy_coding_results_wrapper_ptr->object_ptr;
                        entropy_coding_results_ptr->picture_control_set_wrapper_ptr = enc_dec_results_ptr->picture_control_set_wrapper_ptr;

                        // Post EntropyCoding Results
                        eb_vp9_post_full_object(entropy_coding_results_wrapper_ptr);

                    } // End if(PictureCompleteFlag)
                }
                eb_vp9_release_mutex(picture_control_set_ptr->entropy_coding_mutex);

            }
        }
        // Release Mode Decision Results
        eb_vp9_release_object(enc_dec_results_wrapper_ptr);

    }

    return EB_NULL;
}
