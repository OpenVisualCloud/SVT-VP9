/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <string.h>
#include "stdint.h"
#include "EbEncDecProcess.h"
#include "vp9_encoder.h"

/**************************************************
 * Reset Mode Decision Neighbor Arrays
 *************************************************/
void eb_vp9_reset_mode_decision_neighbor_arrays(PictureControlSet *picture_control_set_ptr)
{
    VP9_COMP    *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
    VP9_COMMON  *const cm = &cpi->common;
    int neighbor_array_count;

    for (neighbor_array_count = 0; neighbor_array_count < NEIGHBOR_ARRAY_TOTAL_COUNT; neighbor_array_count++) {

        eb_vp9_neighbor_array_unit_reset(picture_control_set_ptr->md_luma_recon_neighbor_array[neighbor_array_count]);
        eb_vp9_neighbor_array_unit_reset(picture_control_set_ptr->md_cb_recon_neighbor_array[neighbor_array_count]);
        eb_vp9_neighbor_array_unit_reset(picture_control_set_ptr->md_cr_recon_neighbor_array[neighbor_array_count]);

        // Note: this memset assumes above_context[0], [1] and [2]
        // are allocated as part of the same buffer.
        memset(picture_control_set_ptr->md_above_context[neighbor_array_count], 0, sizeof(*picture_control_set_ptr->md_above_context[neighbor_array_count]) * MAX_MB_PLANE * 2 * mi_cols_aligned_to_sb(cm->mi_cols));
        memset(picture_control_set_ptr->md_above_seg_context[neighbor_array_count], 0, sizeof(*picture_control_set_ptr->md_above_seg_context[neighbor_array_count]) * mi_cols_aligned_to_sb(cm->mi_cols));

        memset(picture_control_set_ptr->md_left_context[neighbor_array_count], 0, sizeof(*picture_control_set_ptr->md_left_context[neighbor_array_count])*MAX_MB_PLANE * 2 * mi_cols_aligned_to_sb(cm->mi_rows));
        memset(picture_control_set_ptr->md_left_seg_context[neighbor_array_count], 0, sizeof(*picture_control_set_ptr->md_left_seg_context[neighbor_array_count]) *  mi_cols_aligned_to_sb(cm->mi_rows));

    }

    return;
}

void eb_vp9_reset_mode_decision(
    EncDecContext     *context_ptr,
    PictureControlSet *picture_control_set_ptr,
    uint32_t           segment_index)
{
    // Configure the number of candidate buffers to search at each depth

    // 64x64 CU
    context_ptr->buffer_depth_index_start[0] = 0;
    context_ptr->buffer_depth_index_width[0] = MAX_NFL + 1; // MAX_NFL NFL + 1 for temporary data

    // 32x32 CU
    context_ptr->buffer_depth_index_start[1] = context_ptr->buffer_depth_index_start[0] + context_ptr->buffer_depth_index_width[0];
    context_ptr->buffer_depth_index_width[1] = MAX_NFL + 1; // MAX_NFL NFL + 1 for temporary data

    // 16x16 CU
    context_ptr->buffer_depth_index_start[2] = context_ptr->buffer_depth_index_start[1] + context_ptr->buffer_depth_index_width[1];
    context_ptr->buffer_depth_index_width[2] = MAX_NFL + 1; // MAX_NFL NFL + 1 for temporary data

    // 8x8 CU
    context_ptr->buffer_depth_index_start[3] = context_ptr->buffer_depth_index_start[2] + context_ptr->buffer_depth_index_width[2];
    context_ptr->buffer_depth_index_width[3] = MAX_NFL + 1; // MAX_NFL NFL + 1 for temporary data

    // 4x4 CU
    context_ptr->buffer_depth_index_start[4] = context_ptr->buffer_depth_index_start[3] + context_ptr->buffer_depth_index_width[3];
    context_ptr->buffer_depth_index_width[4] = MAX_NFL + 1; // MAX_NFL NFL + 1 for temporary data

    // Reset Neighbor Arrays at start of new Segment / Picture
    if (segment_index == 0) {

        eb_vp9_reset_mode_decision_neighbor_arrays(picture_control_set_ptr);
    }

    return;
}
