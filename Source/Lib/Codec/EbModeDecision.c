/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/***************************************
* Includes
***************************************/
#include <stdlib.h>
#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbThreads.h"
#include "EbModeDecision.h"
#include "EbEncDecProcess.h"
#include "vp9_mvref_common.h"

static const uint32_t ep_to_pa_cu_index[EP_BLOCK_MAX_COUNT] =
{
    0,
    0,0,0,0,
    1,
    1,1,1,1,
    2,
    2,2,2,2,
    3,
    3,3,3,3,3,3,3,3,
    4,
    4,4,4,4,4,4,4,4,
    5,
    5,5,5,5,5,5,5,5,
    6,
    6,6,6,6,6,6,6,6,
    7,
    7,7,7,7,
    8,
    8,8,8,8,8,8,8,8,
    9,
    9,9,9,9,9,9,9,9,
    10,
    10,10,10,10,10,10,10,10,
    11,
    11,11,11,11,11,11,11,11,
    12,
    12,12,12,12,
    13,
    13,13,13,13,13,13,13,13,
    14,
    14,14,14,14,14,14,14,14,
    15,
    15,15,15,15,15,15,15,15,
    16,
    16,16,16,16,16,16,16,16,
    17,
    17,17,17,17,
    18,
    18,18,18,18,18,18,18,18,
    19,
    19,19,19,19,19,19,19,19,
    20,
    20,20,20,20,20,20,20,20,
    21,
    21,21,21,21,21,21,21,21,
    22,
    22,22,22,22,
    23,
    23,23,23,23,
    24,
    24,24,24,24,24,24,24,24,
    25,
    25,25,25,25,25,25,25,25,
    26,
    26,26,26,26,26,26,26,26,
    27,
    27,27,27,27,27,27,27,27,
    28,
    28,28,28,28,
    29,
    29,29,29,29,29,29,29,29,
    30,
    30,30,30,30,30,30,30,30,
    31,
    31,31,31,31,31,31,31,31,
    32,
    32,32,32,32,32,32,32,32,
    33,
    33,33,33,33,
    34,
    34,34,34,34,34,34,34,34,
    35,
    35,35,35,35,35,35,35,35,
    36,
    36,36,36,36,36,36,36,36,
    37,
    37,37,37,37,37,37,37,37,
    38,
    38,38,38,38,
    39,
    39,39,39,39,39,39,39,39,
    40,
    40,40,40,40,40,40,40,40,
    41,
    41,41,41,41,41,41,41,41,
    42,
    42,42,42,42,42,42,42,42,
    43,
    43,43,43,43,
    44,
    44,44,44,44,
    45,
    45,45,45,45,45,45,45,45,
    46,
    46,46,46,46,46,46,46,46,
    47,
    47,47,47,47,47,47,47,47,
    48,
    48,48,48,48,48,48,48,48,
    49,
    49,49,49,49,
    50,
    50,50,50,50,50,50,50,50,
    51,
    51,51,51,51,51,51,51,51,
    52,
    52,52,52,52,52,52,52,52,
    53,
    53,53,53,53,53,53,53,53,
    54,
    54,54,54,54,
    55,
    55,55,55,55,55,55,55,55,
    56,
    56,56,56,56,56,56,56,56,
    57,
    57,57,57,57,57,57,57,57,
    58,
    58,58,58,58,58,58,58,58,
    59,
    59,59,59,59,
    60,
    60,60,60,60,60,60,60,60,
    61,
    61,61,61,61,61,61,61,61,
    62,
    62,62,62,62,62,62,62,62,
    63,
    63,63,63,63,63,63,63,63,
    64,
    64,64,64,64,
    65,
    65,65,65,65,
    66,
    66,66,66,66,66,66,66,66,
    67,
    67,67,67,67,67,67,67,67,
    68,
    68,68,68,68,68,68,68,68,
    69,
    69,69,69,69,69,69,69,69,
    70,
    70,70,70,70,
    71,
    71,71,71,71,71,71,71,71,
    72,
    72,72,72,72,72,72,72,72,
    73,
    73,73,73,73,73,73,73,73,
    74,
    74,74,74,74,74,74,74,74,
    75,
    75,75,75,75,
    76,
    76,76,76,76,76,76,76,76,
    77,
    77,77,77,77,77,77,77,77,
    78,
    78,78,78,78,78,78,78,78,
    79,
    79,79,79,79,79,79,79,79,
    80,
    80,80,80,80,
    81,
    81,81,81,81,81,81,81,81,
    82,
    82,82,82,82,82,82,82,82,
    83,
    83,83,83,83,83,83,83,83,
    84,
    84,84,84,84,84,84,84,84,

};

/***************************************
* Mode Decision Candidate Ctor
***************************************/
EbErrorType eb_vp9_mode_decision_candidate_buffer_ctor(
    ModeDecisionCandidateBuffer **buffer_dbl_ptr,
    uint16_t                      sb_max_size,
    EbBitDepth                    max_bitdepth,
    uint64_t                      *fast_cost_ptr,
    uint64_t                      *full_cost_ptr)
{
    EbPictureBufferDescInitData picture_buffer_desc_init_data;
    EbPictureBufferDescInitData double_width_picture_buffer_desc_init_data;
    EbErrorType return_error = EB_ErrorNone;
    // Allocate Buffer
    ModeDecisionCandidateBuffer   *buffer_ptr;
    EB_MALLOC(ModeDecisionCandidateBuffer  *, buffer_ptr, sizeof(ModeDecisionCandidateBuffer  ), EB_N_PTR);
    *buffer_dbl_ptr = buffer_ptr;

    // Init Picture Data
    picture_buffer_desc_init_data.max_width = sb_max_size;
    picture_buffer_desc_init_data.max_height = sb_max_size;
    picture_buffer_desc_init_data.bit_depth = max_bitdepth;
    picture_buffer_desc_init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_FULL_MASK;
    picture_buffer_desc_init_data.left_padding = 0;
    picture_buffer_desc_init_data.right_padding = 0;
    picture_buffer_desc_init_data.top_padding = 0;
    picture_buffer_desc_init_data.bot_padding = 0;
    picture_buffer_desc_init_data.split_mode = EB_FALSE;

    double_width_picture_buffer_desc_init_data.max_width = sb_max_size;
    double_width_picture_buffer_desc_init_data.max_height = sb_max_size;
    double_width_picture_buffer_desc_init_data.bit_depth = EB_16BIT;
    double_width_picture_buffer_desc_init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_FULL_MASK;
    double_width_picture_buffer_desc_init_data.left_padding = 0;
    double_width_picture_buffer_desc_init_data.right_padding = 0;
    double_width_picture_buffer_desc_init_data.top_padding = 0;
    double_width_picture_buffer_desc_init_data.bot_padding = 0;
    double_width_picture_buffer_desc_init_data.split_mode = EB_FALSE;

    // Candidate Ptr
    buffer_ptr->candidate_ptr = (ModeDecisionCandidate    *)EB_NULL;

    // Video Buffers
    return_error = eb_vp9_picture_buffer_desc_ctor(
        (EbPtr *)&(buffer_ptr->prediction_ptr),
        (EbPtr)&picture_buffer_desc_init_data);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = eb_vp9_picture_buffer_desc_ctor(
        (EbPtr *)&(buffer_ptr->residual_quant_coeff_ptr),
        (EbPtr)&double_width_picture_buffer_desc_init_data);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    return_error = eb_vp9_picture_buffer_desc_ctor(
        (EbPtr *)&(buffer_ptr->recon_coeff_ptr),
        (EbPtr)&double_width_picture_buffer_desc_init_data);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = eb_vp9_picture_buffer_desc_ctor(
        (EbPtr *)&(buffer_ptr->recon_ptr),
        (EbPtr)&picture_buffer_desc_init_data);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    // Costs
    buffer_ptr->fast_cost_ptr = fast_cost_ptr;
    buffer_ptr->full_cost_ptr = full_cost_ptr;

    return EB_ErrorNone;
}

/***************************************
* Pre-Mode Decision
*   Selects which fast cost modes to
*   do full reconstruction on.
***************************************/
EbErrorType pre_mode_decision(
    uint32_t                      buffer_total_count,
    ModeDecisionCandidateBuffer **buffer_ptr_array,
    uint32_t                     *full_candidate_total_count_ptr,
    uint8_t                      *best_candidate_index_array,
    EB_BOOL                       same_fast_full_candidate
    )
{

    EbErrorType return_error = EB_ErrorNone;

    uint32_t full_recon_candidate_count;
    uint32_t highest_cost_index;
    uint64_t highest_cost;
    uint32_t cand_indx = 0, i, j, index;

    *full_candidate_total_count_ptr = buffer_total_count;

    //Second,  We substract one, because with N buffers we can determine the best N-1 candidates.
    //Note/TODO: in the case number of fast candidate is less or equal to the number of buffers, N buffers would be enough
    if (same_fast_full_candidate){
        full_recon_candidate_count = MAX(1, (*full_candidate_total_count_ptr));
    }
    else{
        full_recon_candidate_count = MAX(1, (*full_candidate_total_count_ptr) - 1);
    }

    //With N buffers, we get here with the best N-1, plus the last candidate. We need to exclude the worst, and keep the best N-1.
    highest_cost = *(buffer_ptr_array[0]->fast_cost_ptr);
    highest_cost_index = 0;

    if (buffer_total_count > 1){
        if (same_fast_full_candidate){
            for (i = 0; i < buffer_total_count; i++) {
                best_candidate_index_array[cand_indx++] = (uint8_t)i;
            }
        }
        else{
            for (i = 1; i < buffer_total_count; i++) {

                if (*(buffer_ptr_array[i]->fast_cost_ptr) >= highest_cost){
                    highest_cost = *(buffer_ptr_array[i]->fast_cost_ptr);
                    highest_cost_index = i;
                }
            }
            for (i = 0; i < buffer_total_count; i++) {

                if (i != highest_cost_index){
                    best_candidate_index_array[cand_indx++] = (uint8_t)i;
                }
            }
        }

    }
    else {
        best_candidate_index_array[0] = 0;
    }

    // Sort full loop candidates; INTRA then INTER
    for(i = 0; i < full_recon_candidate_count - 1; ++i) {
        for (j = i + 1; j < full_recon_candidate_count; ++j) {
            if ((buffer_ptr_array[best_candidate_index_array[i]]->candidate_ptr->mode_info->mode <= TM_PRED) &&
                (buffer_ptr_array[best_candidate_index_array[j]]->candidate_ptr->mode_info->mode >  TM_PRED)){
                index = best_candidate_index_array[i];
                best_candidate_index_array[i] = (uint8_t)best_candidate_index_array[j];
                best_candidate_index_array[j] = (uint8_t)index;
            }
        }
    }

    // Set (*full_candidate_total_count_ptr) to full_recon_candidate_count
    (*full_candidate_total_count_ptr) = full_recon_candidate_count;

    return return_error;
}

void new_mv_candidates_injection(
    PictureControlSet  *picture_control_set_ptr,
    EncDecContext      *context_ptr,
    SbUnit             *sb_ptr,
    uint32_t            me_2nx2n_table_offset,
    MV_REFERENCE_FRAME  ref_frame_0,
    MV_REFERENCE_FRAME  ref_frame_1,
    uint32_t           *can_total_cnt)
{
    MeCuResults * me_pu_result = &picture_control_set_ptr->parent_pcs_ptr->me_results[sb_ptr->sb_index][me_2nx2n_table_offset];

    // Exist if bipred injection and no bipred from ME
    if (ref_frame_0 == LAST_FRAME && ref_frame_1 == ALTREF_FRAME && me_pu_result->total_me_candidate_index < 3) {
        return;
    }

    // Set distortion ready
    context_ptr->fast_candidate_array[*can_total_cnt].distortion_ready = EB_TRUE;

    // Derive target direction
    uint32_t target_direction = (ref_frame_0 == LAST_FRAME && ref_frame_1 == ALTREF_FRAME) ?
        BI_PRED :
        (ref_frame_0 == LAST_FRAME) ?
            UNI_PRED_LIST_0 :
            UNI_PRED_LIST_1 ;

    // Read ME distortion
    for (int mecandidate_index = 0; mecandidate_index < me_pu_result->total_me_candidate_index; ++mecandidate_index)
    {
        if (me_pu_result->distortion_direction[mecandidate_index].direction == target_direction) {
            context_ptr->fast_candidate_array[*can_total_cnt].me_distortion = me_pu_result->distortion_direction[mecandidate_index].distortion;
            continue;
        }
    }

    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->sb_type = context_ptr->ep_block_stats_ptr->bsize;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mode = NEWMV;
#if SEG_SUPPORT // Hsan: segmentation not supported
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->segment_id = context_ptr->segment_id;
#endif
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[0] = ref_frame_0;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[1] = ref_frame_1;

    if (ref_frame_0 == LAST_FRAME && ref_frame_1 == ALTREF_FRAME) {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = me_pu_result->x_mv_l0 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = me_pu_result->y_mv_l0 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.col = me_pu_result->x_mv_l1 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.row = me_pu_result->y_mv_l1 << 1;
    }
    else if (ref_frame_0 == LAST_FRAME) {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = me_pu_result->x_mv_l0 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = me_pu_result->y_mv_l0 << 1;
    }
    else {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = me_pu_result->x_mv_l1 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = me_pu_result->y_mv_l1 << 1;
    }

#if 0 // Hsan: switchable interp_filter not supported
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->interp_filter = 0;
#endif

    // Update the total number of candidates injected
    (*can_total_cnt)++;

    return;
}

void list_1_to_last_inter_candidates_injection(
    PictureControlSet  *picture_control_set_ptr,
    EncDecContext      *context_ptr,
    SbUnit             *sb_ptr,
    uint32_t            me_2nx2n_table_offset,
    MV_REFERENCE_FRAME  ref_frame_0,
    MV_REFERENCE_FRAME  ref_frame_1,
    uint32_t           *can_total_cnt)
{
    MeCuResults * me_pu_result = &picture_control_set_ptr->parent_pcs_ptr->me_results[sb_ptr->sb_index][me_2nx2n_table_offset];

    // Set distortion ready
    context_ptr->fast_candidate_array[*can_total_cnt].distortion_ready = EB_TRUE;

    // Derive target direction
    uint32_t target_direction = UNI_PRED_LIST_1;

    // Read ME distortion
    for (int mecandidate_index = 0; mecandidate_index < me_pu_result->total_me_candidate_index; ++mecandidate_index)
    {
        if (me_pu_result->distortion_direction[mecandidate_index].direction == target_direction) {
            context_ptr->fast_candidate_array[*can_total_cnt].me_distortion = me_pu_result->distortion_direction[mecandidate_index].distortion;
            continue;
        }
    }

    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->sb_type = context_ptr->ep_block_stats_ptr->bsize;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mode = NEWMV;
#if SEG_SUPPORT // Hsan: segmentation not supported
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->segment_id = context_ptr->segment_id;
#endif
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[0] = ref_frame_0;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[1] = ref_frame_1;

    if (ref_frame_0 == LAST_FRAME && ref_frame_1 == ALTREF_FRAME) {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = me_pu_result->x_mv_l0 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = me_pu_result->y_mv_l0 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.col = me_pu_result->x_mv_l1 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.row = me_pu_result->y_mv_l1 << 1;
    }
    else if (ref_frame_0 == LAST_FRAME) {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = me_pu_result->x_mv_l1 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = me_pu_result->y_mv_l1 << 1;
    }
    else {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = me_pu_result->x_mv_l1 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = me_pu_result->y_mv_l1 << 1;
    }

#if 0 // Hsan: switchable interp_filter not supported
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->interp_filter = 0;
#endif

    // Update the total number of candidates injected
    (*can_total_cnt)++;

    return;
}

void nearest_mv_candidates_injection(
    PictureControlSet  *picture_control_set_ptr,
    EncDecContext      *context_ptr,
    MV_REFERENCE_FRAME  ref_frame_0,
    MV_REFERENCE_FRAME  ref_frame_1,
    uint32_t           *can_total_cnt)
{
    (void)picture_control_set_ptr;
    context_ptr->fast_candidate_array[*can_total_cnt].distortion_ready = 0;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->sb_type = context_ptr->ep_block_stats_ptr->bsize;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mode = NEARESTMV;
#if SEG_SUPPORT // Hsan: segmentation not supported
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->segment_id = context_ptr->segment_id;
#endif
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[0] = ref_frame_0;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[1] = ref_frame_1;

    if (ref_frame_0 == LAST_FRAME && ref_frame_1 == ALTREF_FRAME) {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_0][0].as_mv.col;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_0][0].as_mv.row;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.col = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_1][0].as_mv.col;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.row = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_1][0].as_mv.row;
    }
    else if (ref_frame_0 == LAST_FRAME) {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_0][0].as_mv.col;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_0][0].as_mv.row;
    }
    else {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_0][0].as_mv.col;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_0][0].as_mv.row;
    }

#if 0 // Hsan: switchable interp_filter not supported
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->interp_filter = 0;
#endif
    // Update the total number of candidates injected
    (*can_total_cnt)++;

    return;
}

void near_mv_candidates_injection(
    PictureControlSet  *picture_control_set_ptr,
    EncDecContext      *context_ptr,
    MV_REFERENCE_FRAME  ref_frame_0,
    MV_REFERENCE_FRAME  ref_frame_1,
    uint32_t           *can_total_cnt)
{
    (void)picture_control_set_ptr;

    context_ptr->fast_candidate_array[*can_total_cnt].distortion_ready = 0;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->sb_type = context_ptr->ep_block_stats_ptr->bsize;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mode = NEARMV;
#if SEG_SUPPORT // Hsan: segmentation not supported
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->segment_id = context_ptr->segment_id;
#endif
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[0] = ref_frame_0;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[1] = ref_frame_1;

    if (ref_frame_0 == LAST_FRAME && ref_frame_1 == ALTREF_FRAME) {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_0][1].as_mv.col;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_0][1].as_mv.row;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.col = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_1][1].as_mv.col;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.row = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_1][1].as_mv.row;
    }
    else if (ref_frame_0 == LAST_FRAME) {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_0][1].as_mv.col;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_0][1].as_mv.row;
    }
    else {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_0][1].as_mv.col;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = context_ptr->block_ptr->mbmi_ext->ref_mvs[ref_frame_0][1].as_mv.row;
    }

#if 0 // Hsan: switchable interp_filter not supported
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->interp_filter = 0;
#endif
    // Update the total number of candidates injected
    (*can_total_cnt)++;

    return;
}

void zero_mv_candidates_injection(
    PictureControlSet  *picture_control_set_ptr,
    EncDecContext      *context_ptr,
    MV_REFERENCE_FRAME  ref_frame_0,
    MV_REFERENCE_FRAME  ref_frame_1,
    uint32_t           *can_total_cnt)
{
    (void)picture_control_set_ptr;
    context_ptr->fast_candidate_array[*can_total_cnt].distortion_ready = 0;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->sb_type = context_ptr->ep_block_stats_ptr->bsize;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mode = ZEROMV;
#if SEG_SUPPORT // Hsan: segmentation not supported
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->segment_id = context_ptr->segment_id;
#endif
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[0] = ref_frame_0;
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[1] = ref_frame_1;

    if (ref_frame_0 == LAST_FRAME && ref_frame_1 == ALTREF_FRAME) {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = 0;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = 0;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.col = 0;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.row = 0;
    }
    else if (ref_frame_0 == LAST_FRAME) {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = 0;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = 0;
    }
    else {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = 0;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = 0;
    }

#if 0 // Hsan: switchable interp_filter not supported
    context_ptr->fast_candidate_array[*can_total_cnt].mode_info->interp_filter = 0;
#endif
    // Update the total number of candidates injected
    (*can_total_cnt)++;
}

#define NEWMV_3x3_REFINMENT_POSITIONS 8
int8_t newmv_3x3_x_pos[NEWMV_3x3_REFINMENT_POSITIONS] = { -1, -1,  0, 1, 1,  1,  0, -1 };
int8_t newmv_3x3_y_pos[NEWMV_3x3_REFINMENT_POSITIONS] = { 0,  1,  1, 1, 0, -1, -1, -1 };

void new_mv_unipred3x3_candidates_injection(
    PictureControlSet  *picture_control_set_ptr,
    EncDecContext      *context_ptr,
    SbUnit             *sb_ptr,
    const uint32_t      me_2nx2n_table_offset,
    MV_REFERENCE_FRAME  ref_frame_0,
    MV_REFERENCE_FRAME  ref_frame_1,
    uint32_t           *can_total_cnt)
{
    MeCuResults * me_pu_result = &picture_control_set_ptr->parent_pcs_ptr->me_results[sb_ptr->sb_index][me_2nx2n_table_offset];
    for (uint32_t position = 0; position < NEWMV_3x3_REFINMENT_POSITIONS; ++position)
    {
        context_ptr->fast_candidate_array[*can_total_cnt].distortion_ready = 0;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->sb_type = context_ptr->ep_block_stats_ptr->bsize;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mode = NEWMV;
#if SEG_SUPPORT // Hsan: segmentation not supported
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->segment_id = context_ptr->segment_id;
#endif
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[0] = ref_frame_0;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[1] = ref_frame_1;

        if (ref_frame_0 == LAST_FRAME) {
            context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = (me_pu_result->x_mv_l0 + newmv_3x3_x_pos[position]) << 1;
            context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = (me_pu_result->y_mv_l0 + newmv_3x3_y_pos[position]) << 1;
        }
        else {
            context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = (me_pu_result->x_mv_l1 + newmv_3x3_x_pos[position]) << 1;
            context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = (me_pu_result->y_mv_l1 + newmv_3x3_y_pos[position]) << 1;
        }

#if 0 // Hsan: switchable interp_filter not supported''
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->interp_filter = 0;
#endif
        // Update the total number of candidates injected
        (*can_total_cnt)++;
    }
}

void new_mv_bipred3x3_candidates_injection(
    PictureControlSet  *picture_control_set_ptr,
    EncDecContext      *context_ptr,
    SbUnit             *sb_ptr,
    const uint32_t      me_2nx2n_table_offset,
    MV_REFERENCE_FRAME  ref_frame_0,
    MV_REFERENCE_FRAME  ref_frame_1,
    uint32_t           *can_total_cnt)
{
    MeCuResults * me_pu_result = &picture_control_set_ptr->parent_pcs_ptr->me_results[sb_ptr->sb_index][me_2nx2n_table_offset];

    for (uint32_t position = 0; position < NEWMV_3x3_REFINMENT_POSITIONS; ++position)
    {
        context_ptr->fast_candidate_array[*can_total_cnt].distortion_ready = 0;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->sb_type = context_ptr->ep_block_stats_ptr->bsize;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mode = NEWMV;
#if SEG_SUPPORT // Hsan: segmentation not supported
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->segment_id = context_ptr->segment_id;
#endif
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[0] = ref_frame_0;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[1] = ref_frame_1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = me_pu_result->x_mv_l0 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = me_pu_result->y_mv_l0 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.col = (me_pu_result->x_mv_l1 + newmv_3x3_x_pos[position]) << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.row = (me_pu_result->y_mv_l1 + newmv_3x3_y_pos[position]) << 1;
#if 0 // Hsan: switchable interp_filter not supported
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->interp_filter = 0;
#endif
        // Update the total number of candidates injected
        (*can_total_cnt)++;
    }
    for (uint32_t position = 0; position < NEWMV_3x3_REFINMENT_POSITIONS; ++position)
    {
        context_ptr->fast_candidate_array[*can_total_cnt].distortion_ready = 0;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->sb_type = context_ptr->ep_block_stats_ptr->bsize;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mode = NEWMV;
#if SEG_SUPPORT // Hsan: segmentation not supported
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->segment_id = context_ptr->segment_id;
#endif
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[0] = ref_frame_0;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[1] = ref_frame_1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.col = (me_pu_result->x_mv_l0 + newmv_3x3_x_pos[position]) << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[0].as_mv.row = (me_pu_result->y_mv_l0 + newmv_3x3_y_pos[position]) << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.col = me_pu_result->x_mv_l1 << 1;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mv[1].as_mv.row = me_pu_result->y_mv_l1 << 1;
#if 0 // Hsan: switchable interp_filter not supported
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->interp_filter = 0;
#endif
        // Update the total number of candidates injected
        (*can_total_cnt)++;
    }
}

void  inter_candidates_injection(
    PictureControlSet  *picture_control_set_ptr,
    EncDecContext      *context_ptr,
    SbUnit             *sb_ptr,
    uint32_t            me_2nx2n_table_offset,
    MV_REFERENCE_FRAME  ref_frame_0,
    MV_REFERENCE_FRAME  ref_frame_1,
    uint32_t           *can_total_cnt)
{
    if (context_ptr->use_ref_mvs_flag[ref_frame_0] && context_ptr->use_ref_mvs_flag[ref_frame_1]) {

        // Nearest
        if (context_ptr->nearest_injection) {
            nearest_mv_candidates_injection(
                picture_control_set_ptr,
                context_ptr,
                ref_frame_0,
                ref_frame_1,
                can_total_cnt);
        }

        // Near
        if (context_ptr->near_injection) {
            if (context_ptr->use_ref_mvs_flag[ref_frame_0] == 2 && context_ptr->use_ref_mvs_flag[ref_frame_1] == 2) {
                near_mv_candidates_injection(
                    picture_control_set_ptr,
                    context_ptr,
                    ref_frame_0,
                    ref_frame_1,
                    can_total_cnt);
            }
        }

        // Zero
        if (context_ptr->zero_injection) {
            zero_mv_candidates_injection(
                picture_control_set_ptr,
                context_ptr,
                ref_frame_0,
                ref_frame_1,
                can_total_cnt);
        }

        // New ME
        new_mv_candidates_injection(
            picture_control_set_ptr,
            context_ptr,
            sb_ptr,
            me_2nx2n_table_offset,
            ref_frame_0,
            ref_frame_1,
            can_total_cnt);

        // New Unipred 3x3
        if (context_ptr->unipred3x3_injection) {
            if (ref_frame_1 == INTRA_FRAME) {
                new_mv_unipred3x3_candidates_injection(
                    picture_control_set_ptr,
                    context_ptr,
                    sb_ptr,
                    me_2nx2n_table_offset,
                    ref_frame_0,
                    ref_frame_1,
                    can_total_cnt);
            }
        }

        // New Bipred 3x3
        if (context_ptr->bipred3x3_injection) {
            if (picture_control_set_ptr->parent_pcs_ptr->cpi->common.reference_mode == REFERENCE_MODE_SELECT && (ref_frame_0 != INTRA_FRAME) && (ref_frame_1 != INTRA_FRAME)) {
                new_mv_bipred3x3_candidates_injection(
                    picture_control_set_ptr,
                    context_ptr,
                    sb_ptr,
                    me_2nx2n_table_offset,
                    ref_frame_0,
                    ref_frame_1,
                    can_total_cnt);

            }
        }
    }
}

void intra_candidates_injection_mi(
    PictureControlSet *picture_control_set_ptr,
    EncDecContext     *context_ptr,
    uint32_t          *can_total_cnt)
{
    (void)picture_control_set_ptr;

    PREDICTION_MODE mode;

    const EB_BOOL  is_left_cu      = context_ptr->ep_block_stats_ptr->origin_x == 0;
    const EB_BOOL  is_top_cu       = context_ptr->ep_block_stats_ptr->origin_y == 0;
    const EB_BOOL  limit_intra     = context_ptr->limit_intra;
    const uint8_t    limit_left_mode = V_PRED;
    const uint8_t    limit_top_mode  = H_PRED;

    for (mode = DC_PRED; mode <= TM_PRED; mode++, (*can_total_cnt)++)
    {

        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->sb_type = context_ptr->ep_block_stats_ptr->bsize;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mode = mode;
#if SEG_SUPPORT // Hsan: segmentation not supported
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->segment_id = context_ptr->segment_id;
#endif
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->uv_mode = (context_ptr->chroma_level == CHROMA_LEVEL_0) ?
            context_ptr->best_uv_mode[mode] :
            mode ;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[0] = INTRA_FRAME;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[1] = INTRA_FRAME;;
        context_ptr->fast_candidate_array[*can_total_cnt].distortion_ready = 0;
        if (limit_intra == 1)
            if ((is_left_cu  && mode != limit_left_mode) || (is_top_cu && mode != limit_top_mode))
                (*can_total_cnt)--;

    }
}

void intra_candidates_injection_bmi(
    PictureControlSet *picture_control_set_ptr,
    EncDecContext     *context_ptr,
    uint32_t          *can_total_cnt)
{
    (void)picture_control_set_ptr;

    PREDICTION_MODE mode;

    for (mode = DC_PRED; mode <= TM_PRED; mode++, (*can_total_cnt)++)
    {
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->sb_type = context_ptr->ep_block_stats_ptr->bsize;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->mode = mode;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->bmi[context_ptr->bmi_index].as_mode = mode;
#if SEG_SUPPORT // Hsan: segmentation not supported
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->segment_id = context_ptr->segment_id;
#endif
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->uv_mode = mode;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[0] = INTRA_FRAME;
        context_ptr->fast_candidate_array[*can_total_cnt].mode_info->ref_frame[1] = INTRA_FRAME;
        context_ptr->fast_candidate_array[*can_total_cnt].distortion_ready = 0;
    }
}

/***************************************
* prepare_fast_loop_candidates
*   Creates list of initial modes to
*   perform fast cost search on.
***************************************/
EbErrorType prepare_fast_loop_candidates(
    PictureControlSet  *picture_control_set_ptr,
    EncDecContext      *context_ptr,
    SbUnit             *sb_ptr,
    uint32_t           *buffer_total_count_ptr,
    uint32_t           *candidate_total_count_ptr)
{
    uint32_t me_2nx2n_table_offset  = pa_get_block_stats(ep_to_pa_cu_index[context_ptr->ep_block_index])->cu_num_in_depth + me2_nx2_n_offset[context_ptr->ep_block_stats_ptr->depth];
    uint32_t can_total_cnt = 0;
    EB_BOOL  spatial_reference_mv_present_flag = EB_TRUE;

    //----------------------
    // Inter
    //----------------------
    if (picture_control_set_ptr->slice_type != I_SLICE && context_ptr->ep_block_stats_ptr->bsize >= BLOCK_8X8)
    {
        // Hsan: done to use the same injection method for ALTREF_FRAME, LAST_FRAME, and COMPOUND
        context_ptr->use_ref_mvs_flag[INTRA_FRAME] = 2;
        context_ptr->use_ref_mvs_flag[LAST_FRAME] = 0;
        context_ptr->use_ref_mvs_flag[ALTREF_FRAME] = 0;

        int_mv *candidates;

        // [LAST_FRAME] Gets an initial list of candidate vectors from neighbours and orders them
        candidates = context_ptr->block_ptr->mbmi_ext->ref_mvs[LAST_FRAME];
        context_ptr->use_ref_mvs_flag[LAST_FRAME] = eb_vp9_find_mv_refs(
            context_ptr,
            &picture_control_set_ptr->parent_pcs_ptr->cpi->common,
            context_ptr->e_mbd,
            (ModeInfo *)EB_NULL,
            LAST_FRAME,
            candidates,
            context_ptr->mi_row,
            context_ptr->mi_col,
            context_ptr->block_ptr->mbmi_ext->mode_context);

        // [ALTREF_FRAME] Gets an initial list of candidate vectors from neighbours and orders them for
        if (picture_control_set_ptr->parent_pcs_ptr->cpi->common.reference_mode == REFERENCE_MODE_SELECT) {
            candidates = context_ptr->block_ptr->mbmi_ext->ref_mvs[ALTREF_FRAME];
            context_ptr->use_ref_mvs_flag[ALTREF_FRAME] = eb_vp9_find_mv_refs(
                context_ptr,
                &picture_control_set_ptr->parent_pcs_ptr->cpi->common,
                context_ptr->e_mbd,
                (ModeInfo *)EB_NULL,
                ALTREF_FRAME,
                candidates,
                context_ptr->mi_row,
                context_ptr->mi_col,
                context_ptr->block_ptr->mbmi_ext->mode_context);
        }

        // Hsan: ZERO_MV only (that does not use reference MVs table) when the reference MVs table will not have spatial information.
        if (picture_control_set_ptr->parent_pcs_ptr->cpi->common.reference_mode == REFERENCE_MODE_SELECT) {

            if (context_ptr->use_ref_mvs_flag[LAST_FRAME] == 0 && context_ptr->use_ref_mvs_flag[ALTREF_FRAME] == 0) {

                spatial_reference_mv_present_flag = EB_FALSE;

                zero_mv_candidates_injection(
                    picture_control_set_ptr,
                    context_ptr,
                    LAST_FRAME,
                    ALTREF_FRAME,
                    &can_total_cnt);
            } else {
                // [LAST_FRAME] Injects available inter candidates
                inter_candidates_injection(
                    picture_control_set_ptr,
                    context_ptr,
                    sb_ptr,
                    me_2nx2n_table_offset,
                    LAST_FRAME,
                    INTRA_FRAME,
                    &can_total_cnt);

                // [ALTREF_FRAME] Injects available inter candidates
                inter_candidates_injection(
                    picture_control_set_ptr,
                    context_ptr,
                    sb_ptr,
                    me_2nx2n_table_offset,
                    ALTREF_FRAME,
                    INTRA_FRAME,
                    &can_total_cnt);

                // [COMPOUND] Injects available inter candidates
                inter_candidates_injection(
                    picture_control_set_ptr,
                    context_ptr,
                    sb_ptr,
                    me_2nx2n_table_offset,
                    LAST_FRAME,
                    ALTREF_FRAME,
                    &can_total_cnt);
            }

        }
        else {

            if (context_ptr->use_ref_mvs_flag[LAST_FRAME] == 0) {

                spatial_reference_mv_present_flag = EB_FALSE;

                zero_mv_candidates_injection(
                    picture_control_set_ptr,
                    context_ptr,
                    LAST_FRAME,
                    INTRA_FRAME,
                    &can_total_cnt);
            }
            else {
                // [LAST_FRAME] Injects available inter candidates
                inter_candidates_injection(
                    picture_control_set_ptr,
                    context_ptr,
                    sb_ptr,
                    me_2nx2n_table_offset,
                    LAST_FRAME,
                    INTRA_FRAME,
                    &can_total_cnt);

                // [List 1 -> LAST_FRAME] only if BASE
                if (picture_control_set_ptr->temporal_layer_index == 0) {
                    list_1_to_last_inter_candidates_injection(
                        picture_control_set_ptr,
                        context_ptr,
                        sb_ptr,
                        me_2nx2n_table_offset,
                        LAST_FRAME,
                        INTRA_FRAME,
                        &can_total_cnt);
                }
            }
        }
    }

    //----------------------
    // Intra
    //----------------------
    if (context_ptr->ep_block_stats_ptr->sq_size <= 32) {  // No INTRA injection for 64x64 (INTRA_64x64 not supported)
        if (context_ptr->restrict_intra_global_motion == EB_FALSE || can_total_cnt == 0) { // No INTRA injection if restrict_intra_global_motion TRUE unless no INTER candidates
            if (spatial_reference_mv_present_flag == EB_TRUE || sb_ptr->sb_index > 0) { //  No INTRA injection @ 1st SB if no spatial reference MVs
                if (context_ptr->ep_block_stats_ptr->bsize < BLOCK_8X8) {

                    intra_candidates_injection_bmi(
                        picture_control_set_ptr,
                        context_ptr,
                        &can_total_cnt);
                }
                else {

                    const EB_BOOL  is_left_cu = context_ptr->ep_block_stats_ptr->origin_x == 0;
                    const EB_BOOL  is_top_cu = context_ptr->ep_block_stats_ptr->origin_y == 0;
                    EB_BOOL limitIntraTopLeft = context_ptr->limit_intra == EB_TRUE && is_left_cu  &&  is_top_cu;
                    if (limitIntraTopLeft == 0) {

                        intra_candidates_injection_mi(
                            picture_control_set_ptr,
                            context_ptr,
                            &can_total_cnt);
                    }
                }
            }
        }
    }

    // Set buffer_total_count: determines the number of candidates to fully reconstruct
    *buffer_total_count_ptr = context_ptr->full_recon_search_count;

    // Set buffer_total_count_ptr: determines the number of candidates to fully reconstruct
    *candidate_total_count_ptr = can_total_cnt;

    // Make sure buffer_total_count is not larger than the number of fast modes
    *buffer_total_count_ptr = MIN(*candidate_total_count_ptr, *buffer_total_count_ptr);

    return EB_ErrorNone;
}

/***************************************
* Full Mode Decision
***************************************/
uint8_t full_mode_decision(
    struct EncDecContext         *context_ptr,
    ModeDecisionCandidateBuffer **buffer_ptr_array,
    uint32_t                      candidate_total_count,
    uint8_t                      *best_candidate_index_array,
    uint16_t                      ep_block_index,
    int                           depth_part_stage)
{
    uint8_t  candidate_index;
    uint64_t lowest_cost = MAX_BLOCK_COST;
    uint8_t  lowest_cost_index = 0;

    ModeDecisionCandidate      *candidate_ptr;

    (void) depth_part_stage;

    lowest_cost_index = best_candidate_index_array[0];

    // Find the candidate with the lowest cost
    for (uint32_t index = 0; index < candidate_total_count; ++index) {
        candidate_index = best_candidate_index_array[index];
        if (*(buffer_ptr_array[candidate_index]->full_cost_ptr) < lowest_cost) {
            lowest_cost_index = candidate_index;
            lowest_cost = *(buffer_ptr_array[candidate_index]->full_cost_ptr);
        }
    }

    candidate_ptr = buffer_ptr_array[lowest_cost_index]->candidate_ptr;
    context_ptr->enc_dec_local_block_array[ep_block_index]->cost = *(buffer_ptr_array[lowest_cost_index]->full_cost_ptr);

    candidate_ptr->mode_info->tx_size = context_ptr->ep_block_stats_ptr->tx_size;

    EB_MEMCPY(&(context_ptr->enc_dec_local_block_array[ep_block_index]->mode_info), candidate_ptr->mode_info, sizeof(ModeInfo));

    // Copy eobs
    for (uint8_t tu_index = 0; tu_index < ((context_ptr->ep_block_stats_ptr->sq_size == MAX_SB_SIZE) ? 4 : 1); tu_index++) {
        context_ptr->block_ptr->eob[0][tu_index] = candidate_ptr->eob[0][tu_index];
    }

    if (context_ptr->ep_block_stats_ptr->has_uv) {
        context_ptr->block_ptr->eob[1][0] = candidate_ptr->eob[1][0];
        context_ptr->block_ptr->eob[2][0] = candidate_ptr->eob[2][0];
    }

    return lowest_cost_index;
}
