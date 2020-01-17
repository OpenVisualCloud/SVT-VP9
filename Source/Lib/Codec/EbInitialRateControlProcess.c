/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"

#include "EbMotionEstimationResults.h"
#include "EbInitialRateControlProcess.h"
#include "EbInitialRateControlResults.h"
#include "EbMotionEstimationContext.h"
#include "EbUtility.h"
#include "EbReferenceObject.h"
#include "EbMotionEstimation.h"

/**************************************
* Macros
**************************************/
#define PAN_SB_PERCENTAGE   75
#define LOW_AMPLITUDE_TH    64

static EB_BOOL check_mv_for_pan_high_amp(
    uint32_t hierarchical_levels,
    uint32_t temporal_layer_index,
    int32_t  *x_current_mv,
    int32_t  *x_candidate_mv)
{
    if (*x_current_mv * *x_candidate_mv        > 0                        // both negative or both positives and both different than 0 i.e. same direction and non Stationary)
        && ABS(*x_current_mv) >= global_motion_threshold[hierarchical_levels][temporal_layer_index]    // high amplitude
        && ABS(*x_candidate_mv) >= global_motion_threshold[hierarchical_levels][temporal_layer_index]    // high amplitude
        && ABS(*x_current_mv - *x_candidate_mv) < LOW_AMPLITUDE_TH) {    // close amplitude

        return(EB_TRUE);
    }

    else {
        return(EB_FALSE);
    }

}

static EB_BOOL check_mv_for_tilt_high_amp(
    uint32_t hierarchical_levels,
    uint32_t temporal_layer_index,
    int32_t  *y_current_mv,
    int32_t  *y_candidate_mv)
{
    if (*y_current_mv * *y_candidate_mv > 0                        // both negative or both positives and both different than 0 i.e. same direction and non Stationary)
        && ABS(*y_current_mv) >= global_motion_threshold[hierarchical_levels][temporal_layer_index]    // high amplitude
        && ABS(*y_candidate_mv) >= global_motion_threshold[hierarchical_levels][temporal_layer_index]    // high amplitude
        && ABS(*y_current_mv - *y_candidate_mv) < LOW_AMPLITUDE_TH) {    // close amplitude

        return(EB_TRUE);
    }

    else {
        return(EB_FALSE);
    }

}

static EB_BOOL check_mv_for_pan(
    uint32_t hierarchical_levels,
    uint32_t temporal_layer_index,
    int32_t *x_current_mv,
    int32_t *y_current_mv,
    int32_t *x_candidate_mv,
    int32_t *y_candidate_mv)
{
    if (*y_current_mv < LOW_AMPLITUDE_TH
        && *y_candidate_mv < LOW_AMPLITUDE_TH
        && *x_current_mv * *x_candidate_mv        > 0                        // both negative or both positives and both different than 0 i.e. same direction and non Stationary)
        && ABS(*x_current_mv) >= global_motion_threshold[hierarchical_levels][temporal_layer_index]    // high amplitude
        && ABS(*x_candidate_mv) >= global_motion_threshold[hierarchical_levels][temporal_layer_index]    // high amplitude
        && ABS(*x_current_mv - *x_candidate_mv) < LOW_AMPLITUDE_TH) {    // close amplitude

        return(EB_TRUE);
    }

    else {
        return(EB_FALSE);
    }

}

static EB_BOOL check_mv_for_tilt(
    uint32_t hierarchical_levels,
    uint32_t temporal_layer_index,
    int32_t *x_current_mv,
    int32_t *y_current_mv,
    int32_t *x_candidate_mv,
    int32_t *y_candidate_mv)
{
    if (*x_current_mv < LOW_AMPLITUDE_TH
        && *x_candidate_mv < LOW_AMPLITUDE_TH
        && *y_current_mv * *y_candidate_mv        > 0                        // both negative or both positives and both different than 0 i.e. same direction and non Stationary)
        && ABS(*y_current_mv) >= global_motion_threshold[hierarchical_levels][temporal_layer_index]    // high amplitude
        && ABS(*y_candidate_mv) >= global_motion_threshold[hierarchical_levels][temporal_layer_index]    // high amplitude
        && ABS(*y_current_mv - *y_candidate_mv) < LOW_AMPLITUDE_TH) {    // close amplitude

        return(EB_TRUE);
    }

    else {
        return(EB_FALSE);
    }

}

static void eb_vp9_DetectGlobalMotion(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr)
{
    uint32_t  sb_index;
    uint32_t  picture_width_in_sb = (picture_control_set_ptr->enhanced_picture_ptr->width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE;
    uint32_t  sb_origin_x;
    uint32_t  sb_origin_y;

    uint32_t  total_checked_sbs = 0;
    uint32_t  total_pan_lcus = 0;

    int32_t   x_current_mv = 0;
    int32_t   y_current_mv = 0;
    int32_t   x_left_mv = 0;
    int32_t   y_left_mv = 0;
    int32_t   x_top_mv = 0;
    int32_t   y_top_mv = 0;
    int32_t   x_right_mv = 0;
    int32_t   y_right_mv = 0;
    int32_t   x_bottom_mv = 0;
    int32_t   y_bottom_mv = 0;

    int64_t  x_tilt_mv_sum = 0;
    int64_t  y_tilt_mv_sum = 0;
    uint32_t total_tilt_sbs = 0;

    uint32_t  total_tilt_high_amp_sbs = 0;
    uint32_t  total_pan_high_amp_sbs = 0;

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

        sb_origin_x = sb_params->origin_x;
        sb_origin_y = sb_params->origin_y;

        if (sb_params->is_complete_sb) {

            // Current MV
            eb_vp9_get_mv(picture_control_set_ptr, sb_index, &x_current_mv, &y_current_mv);

            // Left MV
            if (sb_origin_x == 0) {
                x_left_mv = 0;
                y_left_mv = 0;
            }
            else {
                eb_vp9_get_mv(picture_control_set_ptr, sb_index - 1, &x_left_mv, &y_left_mv);
            }

            // Top MV
            if (sb_origin_y == 0) {
                x_top_mv = 0;
                y_top_mv = 0;
            }
            else {
                eb_vp9_get_mv(picture_control_set_ptr, sb_index - picture_width_in_sb, &x_top_mv, &y_top_mv);
            }

            // Right MV
            if ((sb_origin_x + (MAX_SB_SIZE << 1)) > picture_control_set_ptr->enhanced_picture_ptr->width) {
                x_right_mv = 0;
                y_right_mv = 0;
            }
            else {
                eb_vp9_get_mv(picture_control_set_ptr, sb_index + 1, &x_right_mv, &y_right_mv);
            }

            // Bottom MV
            if ((sb_origin_y + (MAX_SB_SIZE << 1)) > picture_control_set_ptr->enhanced_picture_ptr->height) {
                x_bottom_mv = 0;
                y_bottom_mv = 0;
            }
            else {
                eb_vp9_get_mv(picture_control_set_ptr, sb_index + picture_width_in_sb, &x_bottom_mv, &y_bottom_mv);
            }

            total_checked_sbs++;

            if ((EB_BOOL)(check_mv_for_pan(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &x_current_mv, &y_current_mv, &x_left_mv, &y_left_mv)   ||
                          check_mv_for_pan(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &x_current_mv, &y_current_mv, &x_top_mv, &y_top_mv)     ||
                          check_mv_for_pan(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &x_current_mv, &y_current_mv, &x_right_mv, &y_right_mv) ||
                          check_mv_for_pan(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &x_current_mv, &y_current_mv, &x_bottom_mv, &y_bottom_mv))) {

                total_pan_lcus++;

            }

            if ((EB_BOOL)(check_mv_for_tilt(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &x_current_mv, &y_current_mv, &x_left_mv, &y_left_mv)   ||
                          check_mv_for_tilt(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &x_current_mv, &y_current_mv, &x_top_mv, &y_top_mv)     ||
                          check_mv_for_tilt(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &x_current_mv, &y_current_mv, &x_right_mv, &y_right_mv) ||
                          check_mv_for_tilt(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &x_current_mv, &y_current_mv, &x_bottom_mv, &y_bottom_mv))) {

                total_tilt_sbs++;

                x_tilt_mv_sum += x_current_mv;
                y_tilt_mv_sum += y_current_mv;
            }

            if ((EB_BOOL)(check_mv_for_pan_high_amp(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &x_current_mv, &x_left_mv)   ||
                          check_mv_for_pan_high_amp(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &x_current_mv, &x_top_mv)    ||
                          check_mv_for_pan_high_amp(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &x_current_mv, &x_right_mv)  ||
                          check_mv_for_pan_high_amp(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &x_current_mv, &x_bottom_mv))) {

                total_pan_high_amp_sbs++;

            }

            if ((EB_BOOL)(check_mv_for_tilt_high_amp(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &y_current_mv, &y_left_mv)  ||
                          check_mv_for_tilt_high_amp(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &y_current_mv, &y_top_mv)   ||
                          check_mv_for_tilt_high_amp(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &y_current_mv, &y_right_mv) ||
                          check_mv_for_tilt_high_amp(picture_control_set_ptr->hierarchical_levels, picture_control_set_ptr->temporal_layer_index, &y_current_mv, &y_bottom_mv))) {

                total_tilt_high_amp_sbs++;

            }

        }
    }
    picture_control_set_ptr->is_pan = EB_FALSE;
    picture_control_set_ptr->is_tilt = EB_FALSE;

    // If more than PAN_SB_PERCENTAGE % of LCUs are PAN
    if (total_checked_sbs > 0 && ((total_pan_lcus * 100 / total_checked_sbs) > PAN_SB_PERCENTAGE)) {
        picture_control_set_ptr->is_pan = EB_TRUE;
    }

    if (total_checked_sbs > 0 && ((total_tilt_sbs * 100 / total_checked_sbs) > PAN_SB_PERCENTAGE)) {
        picture_control_set_ptr->is_tilt = EB_TRUE;
    }
}

/************************************************
* Initial Rate Control Context Constructor
************************************************/
EbErrorType eb_vp9_initial_eb_vp9_rate_control_context_ctor(
    InitialRateControlContext **context_dbl_ptr,
    EbFifo                     *motion_estimation_results_input_fifo_ptr,
    EbFifo                     *initialrate_control_results_output_fifo_ptr)
{
    InitialRateControlContext *context_ptr;
    EB_MALLOC(InitialRateControlContext*, context_ptr, sizeof(InitialRateControlContext), EB_N_PTR);
    *context_dbl_ptr = context_ptr;
    context_ptr->motion_estimation_results_input_fifo_ptr = motion_estimation_results_input_fifo_ptr;
    context_ptr->initialrate_control_results_output_fifo_ptr = initialrate_control_results_output_fifo_ptr;

    return EB_ErrorNone;
}

/************************************************
* Release Pa Reference Objects
** Check if reference pictures are needed
** release them when appropriate
************************************************/
static void eb_vp9_ReleasePaReferenceObjects(
    PictureParentControlSet *picture_control_set_ptr) {
    // PA Reference Pictures
    uint32_t num_of_list_to_search;
    uint32_t list_index;
    if (picture_control_set_ptr->slice_type != I_SLICE) {

        num_of_list_to_search = (picture_control_set_ptr->slice_type == P_SLICE) ? REF_LIST_0 : REF_LIST_1;

        // List Loop
        for (list_index = REF_LIST_0; list_index <= num_of_list_to_search; ++list_index) {

                // Release PA Reference Pictures
                if (picture_control_set_ptr->ref_pa_pic_ptr_array[list_index] != EB_NULL) {

                    eb_vp9_release_object(((EbPaReferenceObject  *)picture_control_set_ptr->ref_pa_pic_ptr_array[list_index]->object_ptr)->p_pcs_ptr->p_pcs_wrapper_ptr);
                    eb_vp9_release_object(picture_control_set_ptr->ref_pa_pic_ptr_array[list_index]);
                }
        }
    }

    if (picture_control_set_ptr->pareference_picture_wrapper_ptr != EB_NULL) {

        eb_vp9_release_object(picture_control_set_ptr->p_pcs_wrapper_ptr);
        eb_vp9_release_object(picture_control_set_ptr->pareference_picture_wrapper_ptr);
    }

    return;
}

/************************************************
* Global Motion Detection Based on ME information
** Mark pictures for pan
** Mark pictures for tilt
** No lookahead information used in this function
************************************************/
static void me_based_global_motion_detection(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr)
{
    // PAN Generation
    picture_control_set_ptr->is_pan                 = EB_FALSE;
    picture_control_set_ptr->is_tilt                = EB_FALSE;

    if (picture_control_set_ptr->slice_type != I_SLICE) {
        eb_vp9_DetectGlobalMotion(
            sequence_control_set_ptr,
            picture_control_set_ptr);
    }

    return;
}

static void stationary_edge_count_sb(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    PictureParentControlSet *temporalPictureControlSetPtr,
    uint32_t                 total_sb_count)
{

    uint32_t sb_index;
    for (sb_index = 0; sb_index < total_sb_count; sb_index++) {

        SbParams *sb_params  = &sequence_control_set_ptr->sb_params_array[sb_index];
        SbStat   *sb_stat_ptr = &picture_control_set_ptr->sb_stat_array[sb_index];

        if (sb_params->potential_logo_sb && sb_params->is_complete_sb && sb_stat_ptr->check1_for_logo_stationary_edge_over_time_flag && sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag){

            SbStat *tempLcuStatPtr = &temporalPictureControlSetPtr->sb_stat_array[sb_index];
            uint32_t raster_scan_block_index;

            if (tempLcuStatPtr->check1_for_logo_stationary_edge_over_time_flag)
            {
                for (raster_scan_block_index = PA_RASTER_SCAN_CU_INDEX_16x16_0; raster_scan_block_index <= PA_RASTER_SCAN_CU_INDEX_16x16_15; raster_scan_block_index++) {
                    sb_stat_ptr->cu_stat_array[raster_scan_block_index].similar_edge_count += tempLcuStatPtr->cu_stat_array[raster_scan_block_index].edge_cu;
                }
            }
        }

        if (sb_params->potential_logo_sb && sb_params->is_complete_sb && sb_stat_ptr->pm_check1_for_logo_stationary_edge_over_time_flag && sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag){

            SbStat *tempLcuStatPtr = &temporalPictureControlSetPtr->sb_stat_array[sb_index];
            uint32_t raster_scan_block_index;

            if (tempLcuStatPtr->pm_check1_for_logo_stationary_edge_over_time_flag)
            {
                for (raster_scan_block_index = PA_RASTER_SCAN_CU_INDEX_16x16_0; raster_scan_block_index <= PA_RASTER_SCAN_CU_INDEX_16x16_15; raster_scan_block_index++) {
                    sb_stat_ptr->cu_stat_array[raster_scan_block_index].pm_similar_edge_count += tempLcuStatPtr->cu_stat_array[raster_scan_block_index].edge_cu;
                }
            }
        }
    }
}

/************************************************
* Global Motion Detection Based on Lookahead
** Mark pictures for pan
** Mark pictures for tilt
** LAD Window: min (8 or sliding window size)
************************************************/
static void update_global_motion_detection_over_time(
    EncodeContext           *encode_context_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr)
{

    InitialRateControlReorderEntry *temporary_queue_entry_ptr;
    PictureParentControlSet        *temporary_picture_control_set_ptr;

    uint32_t                        total_pan_pictures = 0;
    uint32_t                        total_checked_pictures = 0;
    uint32_t                        total_tilt_pictures = 0;
    uint32_t                        update_is_pan_frames_to_check;
    uint32_t                        input_queue_index;
    uint32_t                        frames_to_check_index;

    (void) sequence_control_set_ptr;

    // Determine number of frames to check (8 frames)
    update_is_pan_frames_to_check = MIN(8, picture_control_set_ptr->frames_in_sw);

    // Walk the first N entries in the sliding window
    input_queue_index = encode_context_ptr->initial_rate_control_reorder_queue_head_index;
    uint32_t updateFramesToCheck = update_is_pan_frames_to_check;
    for (frames_to_check_index = 0; frames_to_check_index < updateFramesToCheck; frames_to_check_index++) {

        temporary_queue_entry_ptr = encode_context_ptr->initial_rate_control_reorder_queue[input_queue_index];
        temporary_picture_control_set_ptr = ((PictureParentControlSet  *)(temporary_queue_entry_ptr->parent_pcs_wrapper_ptr)->object_ptr);

        if (temporary_picture_control_set_ptr->slice_type != I_SLICE) {

            total_pan_pictures += (temporary_picture_control_set_ptr->is_pan == EB_TRUE);

            total_tilt_pictures += (temporary_picture_control_set_ptr->is_tilt == EB_TRUE);

            // Keep track of checked pictures
            total_checked_pictures++;
        }

        // Increment the input_queue_index Iterator
        input_queue_index = (input_queue_index == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : input_queue_index + 1;

    }

    picture_control_set_ptr->is_pan                 = EB_FALSE;
    picture_control_set_ptr->is_tilt                = EB_FALSE;

    if (total_checked_pictures) {
        if (picture_control_set_ptr->slice_type != I_SLICE) {

            if ((total_pan_pictures * 100 / total_checked_pictures) > 75) {
                picture_control_set_ptr->is_pan = EB_TRUE;
            }
        }

    }
    return;
}

/************************************************
* Update BEA Information Based on Lookahead
** Average zzCost of Collocated LCU throughout lookahead frames
** Set isMostOfPictureNonMoving based on number of non moving LCUs
** LAD Window: min (2xmgpos+1 or sliding window size)
************************************************/

static void eb_vp9_UpdateBeaInfoOverTime(
    EncodeContext *encode_context_ptr,
    PictureParentControlSet *picture_control_set_ptr) {
    InitialRateControlReorderEntry *temporary_queue_entry_ptr;
    PictureParentControlSet        *temporary_picture_control_set_ptr;
    int32_t                         update_non_moving_index_array_frames_to_check;
    uint16_t                        sb_index;
    int16_t                         frames_to_check_index;
    uint32_t                        input_queue_index;

    SequenceControlSet *sequence_control_set_ptr = (SequenceControlSet*)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
    // Update motionIndexArray of the current picture by averaging the motionIndexArray of the N future pictures
    // Determine number of frames to check N
    update_non_moving_index_array_frames_to_check = MIN(MIN(((picture_control_set_ptr->pred_struct_ptr->pred_struct_period << 1) + 1), picture_control_set_ptr->frames_in_sw), sequence_control_set_ptr->look_ahead_distance);

    // LCU Loop
    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

         uint16_t nonMovingIndexOverSlidingWindow = picture_control_set_ptr->non_moving_index_array[sb_index];

        // Walk the first N entries in the sliding window starting picture + 1
        input_queue_index = (encode_context_ptr->initial_rate_control_reorder_queue_head_index == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encode_context_ptr->initial_rate_control_reorder_queue_head_index + 1;
        for (frames_to_check_index = 0; frames_to_check_index < update_non_moving_index_array_frames_to_check - 1; frames_to_check_index++) {

            temporary_queue_entry_ptr = encode_context_ptr->initial_rate_control_reorder_queue[input_queue_index];
            temporary_picture_control_set_ptr = ((PictureParentControlSet  *)(temporary_queue_entry_ptr->parent_pcs_wrapper_ptr)->object_ptr);

            if (temporary_picture_control_set_ptr->slice_type == I_SLICE || temporary_picture_control_set_ptr->end_of_sequence_flag) {
                break;
            }
            nonMovingIndexOverSlidingWindow += temporary_picture_control_set_ptr->non_moving_index_array[sb_index];

            // Increment the input_queue_index Iterator
            input_queue_index = (input_queue_index == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : input_queue_index + 1;
        }
        picture_control_set_ptr->non_moving_index_array[sb_index]   = (uint8_t) (nonMovingIndexOverSlidingWindow / (frames_to_check_index + 1));
    }

    return;
}

/****************************************
* Init ZZ Cost array to default values
** Used when no Lookahead is available
****************************************/
static void init_zz_cost_info(
    PictureParentControlSet *picture_control_set_ptr)
{
    uint16_t sb_index;
    picture_control_set_ptr->non_moving_average_score = INVALID_NON_MOVING_SCORE;
    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {
        picture_control_set_ptr->non_moving_index_array[sb_index] = INVALID_NON_MOVING_SCORE;
    }

    return;
}

/************************************************
* Update uniform motion field
** Update Uniformly moving LCUs using
** collocated LCUs infor in lookahead pictures
** LAD Window: min (2xmgpos+1 or sliding window size)
************************************************/
static void update_motion_field_uniformity_over_time(
    EncodeContext           *encode_context_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr)
{
    InitialRateControlReorderEntry *temporary_queue_entry_ptr;
    PictureParentControlSet        *temporary_picture_control_set_ptr;
    uint32_t                        input_queue_index;
    int32_t                         no_frames_to_check;
    //SVT_LOG("To update POC %d\tframesInSw = %d\n", picture_control_set_ptr->picture_number, picture_control_set_ptr->frames_in_sw);

    // Determine number of frames to check N
    no_frames_to_check = MIN(MIN(((picture_control_set_ptr->pred_struct_ptr->pred_struct_period << 1) + 1), picture_control_set_ptr->frames_in_sw), sequence_control_set_ptr->look_ahead_distance);

    // Walk the first N entries in the sliding window starting picture + 1
    input_queue_index = (encode_context_ptr->initial_rate_control_reorder_queue_head_index == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encode_context_ptr->initial_rate_control_reorder_queue_head_index;
    for (int32_t frames_to_check_index = 0; frames_to_check_index < no_frames_to_check - 1; frames_to_check_index++) {

        temporary_queue_entry_ptr = encode_context_ptr->initial_rate_control_reorder_queue[input_queue_index];
        temporary_picture_control_set_ptr = ((PictureParentControlSet  *)(temporary_queue_entry_ptr->parent_pcs_wrapper_ptr)->object_ptr);

        if (temporary_picture_control_set_ptr->end_of_sequence_flag) {
            break;
        }
        // The values are calculated for every 4th frame
        if ((temporary_picture_control_set_ptr->picture_number & 3) == 0){
            stationary_edge_count_sb(
                sequence_control_set_ptr,
                picture_control_set_ptr,
                temporary_picture_control_set_ptr,
                picture_control_set_ptr->sb_total_count);
        }
        // Increment the input_queue_index Iterator
        input_queue_index = (input_queue_index == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : input_queue_index + 1;

    }

    return;
}

/************************************************
* Update uniform motion field
** Update Uniformly moving LCUs using
** collocated LCUs infor in lookahead pictures
** LAD Window: min (2xmgpos+1 or sliding window size)
************************************************/
static void update_homogeneity_over_time(
    EncodeContext *encode_context_ptr,
    PictureParentControlSet *picture_control_set_ptr) {
    InitialRateControlReorderEntry *temporary_queue_entry_ptr;
    PictureParentControlSet        *temporary_picture_control_set_ptr;
    int32_t                         no_frames_to_check;

    uint16_t                       *variance_ptr;

    uint64_t                        mean_sqr_variance64x64_based = 0, mean_variance64x64_based = 0;
    uint16_t                        count_of_homogeneous_over_time = 0;
    uint32_t                        input_queue_index;
    uint32_t                        sb_index;

    SequenceControlSet *sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;

    picture_control_set_ptr->pic_homogenous_over_time_sb_percentage = 0;

    // LCU Loop
    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

        mean_sqr_variance64x64_based = 0;
        mean_variance64x64_based = 0;

        // Initialize
        picture_control_set_ptr->sb_variance_of_variance_over_time[sb_index] = 0xFFFFFFFFFFFFFFFF;

        picture_control_set_ptr->is_sb_homogeneous_over_time[sb_index] = EB_FALSE;

        // Update motionIndexArray of the current picture by averaging the motionIndexArray of the N future pictures
        // Determine number of frames to check N
        no_frames_to_check = MIN(MIN(((picture_control_set_ptr->pred_struct_ptr->pred_struct_period << 1) + 1), picture_control_set_ptr->frames_in_sw), sequence_control_set_ptr->look_ahead_distance);

        // Walk the first N entries in the sliding window starting picture + 1
        input_queue_index = (encode_context_ptr->initial_rate_control_reorder_queue_head_index == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encode_context_ptr->initial_rate_control_reorder_queue_head_index;
        for (int32_t frames_to_check_index = 0; frames_to_check_index < no_frames_to_check - 1; frames_to_check_index++) {

            temporary_queue_entry_ptr = encode_context_ptr->initial_rate_control_reorder_queue[input_queue_index];
            temporary_picture_control_set_ptr = ((PictureParentControlSet  *)(temporary_queue_entry_ptr->parent_pcs_wrapper_ptr)->object_ptr);
            if (temporary_picture_control_set_ptr->scene_change_flag || temporary_picture_control_set_ptr->end_of_sequence_flag) {

                break;
            }

            variance_ptr = temporary_picture_control_set_ptr->variance[sb_index];

            mean_sqr_variance64x64_based += (variance_ptr[ME_TIER_ZERO_PU_64x64])*(variance_ptr[ME_TIER_ZERO_PU_64x64]);
            mean_variance64x64_based += (variance_ptr[ME_TIER_ZERO_PU_64x64]);

            // Increment the input_queue_index Iterator
            input_queue_index = (input_queue_index == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : input_queue_index + 1;

        } // PCS loop

        mean_sqr_variance64x64_based = mean_sqr_variance64x64_based / (no_frames_to_check - 1);
        mean_variance64x64_based = mean_variance64x64_based / (no_frames_to_check - 1);

        // Compute variance
        picture_control_set_ptr->sb_variance_of_variance_over_time[sb_index] = mean_sqr_variance64x64_based - mean_variance64x64_based * mean_variance64x64_based;

        if (picture_control_set_ptr->sb_variance_of_variance_over_time[sb_index] <= (VAR_BASED_DETAIL_PRESERVATION_SELECTOR_THRSLHD)) {
            picture_control_set_ptr->is_sb_homogeneous_over_time[sb_index] = EB_TRUE;
        }

        count_of_homogeneous_over_time += picture_control_set_ptr->is_sb_homogeneous_over_time[sb_index];

    } // LCU loop

    picture_control_set_ptr->pic_homogenous_over_time_sb_percentage = (uint8_t)(count_of_homogeneous_over_time * 100 / picture_control_set_ptr->sb_total_count);

    return;
}

static void reset_homogeneity_structures(
    PictureParentControlSet *picture_control_set_ptr) {
    uint32_t sb_index;

    picture_control_set_ptr->pic_homogenous_over_time_sb_percentage = 0;

    // Reset the structure
    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {
        picture_control_set_ptr->sb_variance_of_variance_over_time[sb_index] = 0xFFFFFFFFFFFFFFFF;
        picture_control_set_ptr->is_sb_homogeneous_over_time[sb_index] = EB_FALSE;
    }

    return;
}

static InitialRateControlReorderEntry* determine_picture_offset_in_queue(
    EncodeContext           *encode_context_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    MotionEstimationResults *input_results_ptr)
{

    InitialRateControlReorderEntry  *queue_entry_ptr;
    int32_t                          queue_entry_index;

    queue_entry_index = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->initial_rate_control_reorder_queue[encode_context_ptr->initial_rate_control_reorder_queue_head_index]->picture_number);
    queue_entry_index += encode_context_ptr->initial_rate_control_reorder_queue_head_index;
    queue_entry_index = (queue_entry_index > INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? queue_entry_index - INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH : queue_entry_index;
    queue_entry_ptr = encode_context_ptr->initial_rate_control_reorder_queue[queue_entry_index];
    queue_entry_ptr->parent_pcs_wrapper_ptr = input_results_ptr->picture_control_set_wrapper_ptr;
    queue_entry_ptr->picture_number = picture_control_set_ptr->picture_number;

    return queue_entry_ptr;
}

static void get_histogram_queue_data(
    SequenceControlSet      *sequence_control_set_ptr,
    EncodeContext           *encode_context_ptr,
    PictureParentControlSet *picture_control_set_ptr)
{
    HlRateControlHistogramEntry *histogram_queue_entry_ptr;
    int32_t                      histogram_queue_entry_index;

    // Determine offset from the Head Ptr for HLRC histogram queue
    eb_vp9_block_on_mutex(sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue_mutex);
    histogram_queue_entry_index = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
    histogram_queue_entry_index += encode_context_ptr->hl_rate_control_historgram_queue_head_index;
    histogram_queue_entry_index = (histogram_queue_entry_index > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
        histogram_queue_entry_index - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
        histogram_queue_entry_index;
    histogram_queue_entry_ptr = encode_context_ptr->hl_rate_control_historgram_queue[histogram_queue_entry_index];

    //histogram_queue_entry_ptr->parent_pcs_wrapper_ptr  = input_results_ptr->picture_control_set_wrapper_ptr;
    histogram_queue_entry_ptr->picture_number = picture_control_set_ptr->picture_number;
    histogram_queue_entry_ptr->end_of_sequence_flag = picture_control_set_ptr->end_of_sequence_flag;
    histogram_queue_entry_ptr->slice_type = picture_control_set_ptr->slice_type;
    histogram_queue_entry_ptr->temporal_layer_index = picture_control_set_ptr->temporal_layer_index;
    histogram_queue_entry_ptr->full_sb_count = picture_control_set_ptr->full_sb_count;
    histogram_queue_entry_ptr->life_count = 0;
    histogram_queue_entry_ptr->passed_to_hlrc = EB_FALSE;
    histogram_queue_entry_ptr->is_coded = EB_FALSE;
    histogram_queue_entry_ptr->total_num_bitsCoded = 0;
    histogram_queue_entry_ptr->frames_in_sw = 0;
    EB_MEMCPY(
        histogram_queue_entry_ptr->me_distortion_histogram,
        picture_control_set_ptr->me_distortion_histogram,
        sizeof(uint16_t) * NUMBER_OF_SAD_INTERVALS);

    EB_MEMCPY(
        histogram_queue_entry_ptr->ois_distortion_histogram,
        picture_control_set_ptr->ois_distortion_histogram,
        sizeof(uint16_t) * NUMBER_OF_INTRA_SAD_INTERVALS);

    eb_vp9_release_mutex(sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue_mutex);
    //SVT_LOG("Test1 POC: %d\t POC: %d\t life_count: %d\n", histogram_queue_entry_ptr->picture_number, picture_control_set_ptr->picture_number,  histogram_queue_entry_ptr->life_count);

    return;

}

static void update_histogram_queue_entry(
    SequenceControlSet      *sequence_control_set_ptr,
    EncodeContext           *encode_context_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    uint32_t                 frames_in_sw)
{

    HlRateControlHistogramEntry *histogram_queue_entry_ptr;
    int32_t                      histogram_queue_entry_index;

    eb_vp9_block_on_mutex(sequence_control_set_ptr->encode_context_ptr->rate_table_update_mutex);

    histogram_queue_entry_index = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
    histogram_queue_entry_index += encode_context_ptr->hl_rate_control_historgram_queue_head_index;
    histogram_queue_entry_index = (histogram_queue_entry_index > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
        histogram_queue_entry_index - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
        histogram_queue_entry_index;
    histogram_queue_entry_ptr = encode_context_ptr->hl_rate_control_historgram_queue[histogram_queue_entry_index];
    histogram_queue_entry_ptr->passed_to_hlrc = EB_TRUE;

    if (sequence_control_set_ptr->static_config.rate_control_mode == 2) {
        histogram_queue_entry_ptr->life_count += (int16_t)(sequence_control_set_ptr->static_config.intra_period + 1) - 3; // FramelevelRC does not decrease the life count for first picture in each temporal layer

    }
    else {
        histogram_queue_entry_ptr->life_count += picture_control_set_ptr->historgram_life_count;
    }
    histogram_queue_entry_ptr->frames_in_sw = frames_in_sw;
    eb_vp9_release_mutex(sequence_control_set_ptr->encode_context_ptr->rate_table_update_mutex);

    return;

}

EB_AURA_STATUS aura_detection64x64(
    PictureControlSet *picture_control_set_ptr,
    uint8_t            picture_qp,
    uint32_t           xsb_index,
    uint32_t           ysb_index
    );

/************************************************
* Initial Rate Control Kernel
* The Initial Rate Control Process determines the initial bit budget for each
* picture depending on the data gathered in the Picture Analysis and Motion
* Analysis processes as well as the settings determined in the Picture Decision process.
* The Initial Rate Control process also employs a sliding window buffer to analyze multiple
* pictures if the delay is allowed.  Note that through this process, until the subsequent
* Picture Manager process, no reference picture data has been used.
* P.S. Temporal noise reduction is now performed in Initial Rate Control Process.
* In future we might decide to move it to Motion Analysis Process.
************************************************/
void* eb_vp9_initial_eb_vp9_rate_control_kernel(void *input_ptr)
{
    InitialRateControlContext      *context_ptr = (InitialRateControlContext*)input_ptr;
    PictureParentControlSet        *picture_control_set_ptr;
    PictureParentControlSet        *pictureControlSetPtrTemp;
    EncodeContext                  *encode_context_ptr;
    SequenceControlSet             *sequence_control_set_ptr;

    EbObjectWrapper                *input_results_wrapper_ptr;
    MotionEstimationResults        *input_results_ptr;

    EbObjectWrapper                *output_results_wrapper_ptr;
    InitialRateControlResults      *output_results_ptr;

    // Queue variables
    uint32_t                        queue_entry_index_temp;
    uint32_t                        queue_entry_index_temp2;
    InitialRateControlReorderEntry *queue_entry_ptr;

    EB_BOOL                         move_slide_wondow_flag = EB_TRUE;
    EB_BOOL                         end_of_sequence_flag = EB_TRUE;
    uint8_t                         frames_in_sw;
    uint8_t                         temporal_layer_index;
    EbObjectWrapper                *reference_picture_wrapper_ptr;

    // Segments
    uint32_t                        segment_index;

    EbObjectWrapper                *output_stream_wrapper_ptr;

    for (;;) {

        // Get Input Full Object
        eb_vp9_get_full_object(
            context_ptr->motion_estimation_results_input_fifo_ptr,
            &input_results_wrapper_ptr);

        input_results_ptr = (MotionEstimationResults*)input_results_wrapper_ptr->object_ptr;
        picture_control_set_ptr = (PictureParentControlSet  *)input_results_ptr->picture_control_set_wrapper_ptr->object_ptr;

        segment_index = input_results_ptr->segment_index;

        // Set the segment mask
        SEGMENT_COMPLETION_MASK_SET(picture_control_set_ptr->me_segments_completion_mask, segment_index);

        // If the picture is complete, proceed
        if (SEGMENT_COMPLETION_MASK_TEST(picture_control_set_ptr->me_segments_completion_mask, picture_control_set_ptr->me_segments_total_count)) {
            sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
            encode_context_ptr = (EncodeContext*)sequence_control_set_ptr->encode_context_ptr;

            // Mark picture when global motion is detected using ME results
            //reset intraCodedEstimationLcu
            me_based_global_motion_detection(
                sequence_control_set_ptr,
                picture_control_set_ptr);

            // Release Pa Ref pictures when not needed
            eb_vp9_ReleasePaReferenceObjects(
                picture_control_set_ptr);

            //****************************************************
            // Input Motion Analysis Results into Reordering Queue
            //****************************************************

            // Determine offset from the Head Ptr
            queue_entry_ptr = determine_picture_offset_in_queue(
                encode_context_ptr,
                picture_control_set_ptr,
                input_results_ptr);

            if (sequence_control_set_ptr->static_config.rate_control_mode)
            {
                if (sequence_control_set_ptr->look_ahead_distance != 0){

                    // Getting the Histogram Queue Data
                    get_histogram_queue_data(
                        sequence_control_set_ptr,
                        encode_context_ptr,
                        picture_control_set_ptr);
                }
            }

            for (temporal_layer_index = 0; temporal_layer_index < EB_MAX_TEMPORAL_LAYERS; temporal_layer_index++){
                picture_control_set_ptr->frames_in_interval[temporal_layer_index] = 0;
            }

            picture_control_set_ptr->frames_in_sw                 = 0;
            picture_control_set_ptr->historgram_life_count        = 0;
            picture_control_set_ptr->scene_change_in_gop = EB_FALSE;

            move_slide_wondow_flag = EB_TRUE;
            while (move_slide_wondow_flag){

                // Check if the sliding window condition is valid
                queue_entry_index_temp = encode_context_ptr->initial_rate_control_reorder_queue_head_index;
                if (encode_context_ptr->initial_rate_control_reorder_queue[queue_entry_index_temp]->parent_pcs_wrapper_ptr != EB_NULL){
                    end_of_sequence_flag = (((PictureParentControlSet  *)(encode_context_ptr->initial_rate_control_reorder_queue[queue_entry_index_temp]->parent_pcs_wrapper_ptr)->object_ptr))->end_of_sequence_flag;
                }
                else{
                    end_of_sequence_flag = EB_FALSE;
                }
                frames_in_sw = 0;
                while (move_slide_wondow_flag && !end_of_sequence_flag &&
                    queue_entry_index_temp <= encode_context_ptr->initial_rate_control_reorder_queue_head_index + sequence_control_set_ptr->look_ahead_distance){
                    // frames_in_sw <= sequence_control_set_ptr->look_ahead_distance){

                    frames_in_sw++;

                    queue_entry_index_temp2 = (queue_entry_index_temp > INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH : queue_entry_index_temp;

                    move_slide_wondow_flag = (EB_BOOL)(move_slide_wondow_flag && (encode_context_ptr->initial_rate_control_reorder_queue[queue_entry_index_temp2]->parent_pcs_wrapper_ptr != EB_NULL));
                    if (encode_context_ptr->initial_rate_control_reorder_queue[queue_entry_index_temp2]->parent_pcs_wrapper_ptr != EB_NULL){
                        // check if it is the last frame. If we have reached the last frame, we would output the buffered frames in the Queue.
                        end_of_sequence_flag = ((PictureParentControlSet  *)(encode_context_ptr->initial_rate_control_reorder_queue[queue_entry_index_temp2]->parent_pcs_wrapper_ptr)->object_ptr)->end_of_sequence_flag;
                    }
                    else{
                        end_of_sequence_flag = EB_FALSE;
                    }
                    queue_entry_index_temp++;
                }

                if (move_slide_wondow_flag)  {

                    //get a new entry spot
                    queue_entry_ptr = encode_context_ptr->initial_rate_control_reorder_queue[encode_context_ptr->initial_rate_control_reorder_queue_head_index];
                    picture_control_set_ptr = ((PictureParentControlSet  *)(queue_entry_ptr->parent_pcs_wrapper_ptr)->object_ptr);
                    sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
                    picture_control_set_ptr->frames_in_sw = frames_in_sw;
                    queue_entry_index_temp = encode_context_ptr->initial_rate_control_reorder_queue_head_index;
                    end_of_sequence_flag = EB_FALSE;
                    // find the frames_in_interval for the peroid I frames
                    while (!end_of_sequence_flag &&
                        queue_entry_index_temp <= encode_context_ptr->initial_rate_control_reorder_queue_head_index + sequence_control_set_ptr->look_ahead_distance){

                        queue_entry_index_temp2 = (queue_entry_index_temp > INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH : queue_entry_index_temp;
                        pictureControlSetPtrTemp = ((PictureParentControlSet  *)(encode_context_ptr->initial_rate_control_reorder_queue[queue_entry_index_temp2]->parent_pcs_wrapper_ptr)->object_ptr);
                        if (sequence_control_set_ptr->intra_period != -1){
                            if (picture_control_set_ptr->picture_number % ((sequence_control_set_ptr->intra_period + 1)) == 0){
                                picture_control_set_ptr->frames_in_interval[pictureControlSetPtrTemp->temporal_layer_index] ++;
                                if (pictureControlSetPtrTemp->scene_change_flag)
                                    picture_control_set_ptr->scene_change_in_gop = EB_TRUE;
                            }
                        }

                        pictureControlSetPtrTemp->historgram_life_count++;
                        end_of_sequence_flag = pictureControlSetPtrTemp->end_of_sequence_flag;
                        queue_entry_index_temp++;
                    }

                    if ((sequence_control_set_ptr->look_ahead_distance != 0) && (frames_in_sw < (sequence_control_set_ptr->look_ahead_distance + 1)))
                        picture_control_set_ptr->end_of_sequence_region = EB_TRUE;
                    else
                        picture_control_set_ptr->end_of_sequence_region = EB_FALSE;

                    if (sequence_control_set_ptr->static_config.rate_control_mode)
                    {
                        // Determine offset from the Head Ptr for HLRC histogram queue and set the life count
                        if (sequence_control_set_ptr->look_ahead_distance != 0){

                            // Update Histogram Queue Entry Life count
                            update_histogram_queue_entry(
                                sequence_control_set_ptr,
                                encode_context_ptr,
                                picture_control_set_ptr,
                                frames_in_sw);
                        }
                    }

                    // Mark each input picture as PAN or not
                    // If a lookahead is present then check PAN for a period of time
                    if (!picture_control_set_ptr->end_of_sequence_flag && sequence_control_set_ptr->look_ahead_distance != 0) {

                        // Check for Pan,Tilt, Zoom and other global motion detectors over the future pictures in the lookahead
                        update_global_motion_detection_over_time(
                            encode_context_ptr,
                            sequence_control_set_ptr,
                            picture_control_set_ptr);
                    }
                    else {
                        if (picture_control_set_ptr->slice_type != I_SLICE) {
                            eb_vp9_DetectGlobalMotion(
                                sequence_control_set_ptr,
                                picture_control_set_ptr);
                        }
                    }

                    // BACKGROUND ENHANCEMENT PART II
                    if (!picture_control_set_ptr->end_of_sequence_flag && sequence_control_set_ptr->look_ahead_distance != 0) {
                        // Update BEA information based on Lookahead information
                        eb_vp9_UpdateBeaInfoOverTime(
                            encode_context_ptr,
                            picture_control_set_ptr);

                    }
                    else {
                        // Reset zzCost information to default When there's no lookahead available
                        init_zz_cost_info(
                            picture_control_set_ptr);
                    }

                    // Use the temporal layer 0 isLcuMotionFieldNonUniform array for all the other layer pictures in the mini GOP
                    if (!picture_control_set_ptr->end_of_sequence_flag && sequence_control_set_ptr->look_ahead_distance != 0) {

                        // Updat uniformly moving LCUs based on Collocated LCUs in LookAhead window
                        update_motion_field_uniformity_over_time(
                            encode_context_ptr,
                            sequence_control_set_ptr,
                            picture_control_set_ptr);
                    }

                    if (!picture_control_set_ptr->end_of_sequence_flag && sequence_control_set_ptr->look_ahead_distance != 0) {
                        // Compute and store variance of LCus over time and determine homogenuity temporally
                        update_homogeneity_over_time(
                            encode_context_ptr,
                            picture_control_set_ptr);
                    }
                    else {
                        // Reset Homogeneity Structures to default if no lookahead is detected
                        reset_homogeneity_structures(
                            picture_control_set_ptr);
                    }

                    // Get Empty Reference Picture Object
                    eb_vp9_get_empty_object(
                        sequence_control_set_ptr->encode_context_ptr->reference_picture_pool_fifo_ptr,
                        &reference_picture_wrapper_ptr);
                    ((PictureParentControlSet  *)(queue_entry_ptr->parent_pcs_wrapper_ptr->object_ptr))->reference_picture_wrapper_ptr = reference_picture_wrapper_ptr;

                    // Give the new Reference a nominal live_count of 1
                    eb_vp9_object_inc_live_count(
                        ((PictureParentControlSet  *)(queue_entry_ptr->parent_pcs_wrapper_ptr->object_ptr))->reference_picture_wrapper_ptr,
                        1);
                    //OPTION 1:  get the buffer in resource coordination
                    eb_vp9_get_empty_object(
                        sequence_control_set_ptr->encode_context_ptr->stream_output_fifo_ptr,
                        &output_stream_wrapper_ptr);
                    picture_control_set_ptr->output_stream_wrapper_ptr = output_stream_wrapper_ptr;

                    // Get Empty Results Object
                    eb_vp9_get_empty_object(
                        context_ptr->initialrate_control_results_output_fifo_ptr,
                        &output_results_wrapper_ptr);

                    output_results_ptr = (InitialRateControlResults*)output_results_wrapper_ptr->object_ptr;
                    output_results_ptr->picture_control_set_wrapper_ptr = queue_entry_ptr->parent_pcs_wrapper_ptr;
                    /////////////////////////////
                    // Post the Full Results Object
                    eb_vp9_post_full_object(output_results_wrapper_ptr);

                    // Reset the Reorder Queue Entry
                    queue_entry_ptr->picture_number += INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH;
                    queue_entry_ptr->parent_pcs_wrapper_ptr = (EbObjectWrapper *)EB_NULL;

                    // Increment the Reorder Queue head Ptr
                    encode_context_ptr->initial_rate_control_reorder_queue_head_index =
                        (encode_context_ptr->initial_rate_control_reorder_queue_head_index == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encode_context_ptr->initial_rate_control_reorder_queue_head_index + 1;

                    queue_entry_ptr = encode_context_ptr->initial_rate_control_reorder_queue[encode_context_ptr->initial_rate_control_reorder_queue_head_index];
                }
            }
        }

        // Release the Input Results
        eb_vp9_release_object(input_results_wrapper_ptr);

    }
    return EB_NULL;
}
