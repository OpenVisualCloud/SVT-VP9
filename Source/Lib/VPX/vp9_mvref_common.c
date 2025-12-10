
/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp9_mvref_common.h"

// This function searches the neighborhood of a given MB/SB
// to try and find candidate reference vectors.
// 0: do not use reference MV(s) (only ZERO_MV as INTER candidate)
// 1: nearest only
// 2: both nearest and near
static int find_mv_refs_idx(EncDecContext *context_ptr, const VP9_COMMON *cm, const MACROBLOCKD *xd, ModeInfo *mi,
                            MV_REFERENCE_FRAME ref_frame, int_mv *mv_ref_list, int block, int mi_row, int mi_col,
                            uint8_t *mode_context) {
    (void)mi;

    const int            *ref_sign_bias = cm->ref_frame_sign_bias;
    int                   i, refmv_count = 0;
    const POSITION *const mv_ref_search       = mv_ref_blocks[context_ptr->ep_block_stats_ptr->bsize];
    int                   different_ref_found = 0;
    int                   context_counter     = 0;
    const TileInfo *const tile                = &xd->tile;

    // Blank the reference vector list
    memset(mv_ref_list, 0, sizeof(*mv_ref_list) * MAX_MV_REF_CANDIDATES);

    // The nearest 2 blocks are treated differently
    // if the size < 8x8 we get the mv from the bmi substructure,
    // and we also need to keep a mode count.
    for (i = 0; i < 2; ++i) {
        const POSITION *const mv_ref = &mv_ref_search[i];
        if (is_inside(tile, mi_col, mi_row, cm->mi_rows, mv_ref)) {
            ModeInfo *candidate_mi =
                context_ptr->mode_info_array[(context_ptr->mi_col + mv_ref->col) +
                                             (context_ptr->mi_row + mv_ref->row) * context_ptr->mi_stride];
            // Keep counts for entropy encoding.
            context_counter += mode_2_counter[candidate_mi->mode];
            different_ref_found = 1;

            if (candidate_mi->ref_frame[0] == ref_frame)
                ADD_MV_REF_LIST(get_sub_block_mv(candidate_mi, 0, mv_ref->col, block), refmv_count, mv_ref_list, Done);
            else if (candidate_mi->ref_frame[1] == ref_frame)
                ADD_MV_REF_LIST(get_sub_block_mv(candidate_mi, 1, mv_ref->col, block), refmv_count, mv_ref_list, Done);
        }
    }

    // Check the rest of the neighbors in much the same way
    // as before except we don't need to keep track of sub blocks or
    // mode counts.
    for (; i < MVREF_NEIGHBOURS; ++i) {
        const POSITION *const mv_ref = &mv_ref_search[i];

        if (is_inside(tile, mi_col, mi_row, cm->mi_rows, mv_ref)) {
            ModeInfo *candidate_mi =
                context_ptr->mode_info_array[(context_ptr->mi_col + mv_ref->col) +
                                             (context_ptr->mi_row + mv_ref->row) * context_ptr->mi_stride];
            different_ref_found = 1;

            if (candidate_mi->ref_frame[0] == ref_frame)
                ADD_MV_REF_LIST(candidate_mi->mv[0], refmv_count, mv_ref_list, Done);
            else if (candidate_mi->ref_frame[1] == ref_frame)
                ADD_MV_REF_LIST(candidate_mi->mv[1], refmv_count, mv_ref_list, Done);
        }
    }

    if (cm->use_prev_frame_mvs == true) {
        // Hsan : SVT-VP9 does not support temporal MV(s) as reference MV(s)
        // If here then reference MV(s) will not be used (i.e. only ZERO_MV as INTER canidate)
        mode_context[ref_frame] = (uint8_t)counter_to_context[context_counter];

        // Clamp vectors
        for (i = 0; i < refmv_count; ++i) clamp_mv_ref(&mv_ref_list[i].as_mv, xd);

        if (refmv_count == 0)
            return 0;
        else if (refmv_count == 1)
            return 1;
        else
            assert(0);
    }
    // Since we couldn't find 2 mvs from the same reference frame
    // go back through the neighbors and find motion vectors from
    // different reference frames.
    if (different_ref_found) {
        for (i = 0; i < MVREF_NEIGHBOURS; ++i) {
            const POSITION *mv_ref = &mv_ref_search[i];
            if (is_inside(tile, mi_col, mi_row, cm->mi_rows, mv_ref)) {
                ModeInfo *candidate_mi =
                    context_ptr->mode_info_array[(context_ptr->mi_col + mv_ref->col) +
                                                 (context_ptr->mi_row + mv_ref->row) * context_ptr->mi_stride];
                // If the candidate is INTRA we don't want to consider its mv.
                IF_DIFF_REF_FRAME_ADD_MV(candidate_mi, ref_frame, ref_sign_bias, refmv_count, mv_ref_list, Done);
            }
        }
    }

    //if (cm->use_prev_frame_mvs == true)
    {
        // Hsan : SVT-VP9 does not support temporal MV(s) as reference MV(s)
        // If here then reference MV(s) will not be used (i.e. only ZERO_MV as INTER canidate)
        mode_context[ref_frame] = (uint8_t)counter_to_context[context_counter];

        // Clamp vectors
        for (i = 0; i < refmv_count; ++i) clamp_mv_ref(&mv_ref_list[i].as_mv, xd);

        if (refmv_count == 0)
            return 0;
        else if (refmv_count == 1)
            return 1;
        else
            assert(0);
    }
Done:

    mode_context[ref_frame] = (uint8_t)counter_to_context[context_counter];

    // Clamp vectors
    for (i = 0; i < MAX_MV_REF_CANDIDATES; ++i) clamp_mv_ref(&mv_ref_list[i].as_mv, xd);

    // If here then refernce MV(s) will be used
    return 2;
}
int eb_vp9_find_mv_refs(EncDecContext *context_ptr, const VP9_COMMON *cm, const MACROBLOCKD *xd, ModeInfo *mi,
                        MV_REFERENCE_FRAME ref_frame, int_mv *mv_ref_list, int mi_row, int mi_col,
                        uint8_t *mode_context) {
    return find_mv_refs_idx(context_ptr, cm, xd, mi, ref_frame, mv_ref_list, -1, mi_row, mi_col, mode_context);
}
