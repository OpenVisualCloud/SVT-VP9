/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbRateControlProcess.h"
#include "EbSystemResourceManager.h"
#include "EbSequenceControlSet.h"
#include "EbPictureControlSet.h"
#include "EbUtility.h"
#include "EbSvtVp9ErrorCodes.h"

#include "EbRateControlResults.h"
#include "EbRateControlTasks.h"

#include "vp9_quantize.h"
#include "vp9_ratectrl.h"
#include "vpx_dsp_common.h"

#ifdef AGGRESSIVE_VBR
static int gf_high = 2400;
static int gf_low = 400;
static int kf_high = 4000;
static int kf_low = 400;
#else
static int gf_high = 2000;
static int gf_low = 400;
static int kf_high = 4800;
static int kf_low = 300;
#endif
#if NEW_PRED_STRUCT
const double delta_rate_oq[2][6] = {
    {0.35, 0.70, 0.85, 1.00, 1.00, 1.00}, // 4L
    { 0.30, 0.6, 0.8,  0.9, 1.0, 1.0 } }; // 5L
#else
const double delta_rate_oq[6] = { 0.35, 0.70, 0.85, 1.00, 1.00, 1.00 };
#endif
const double delta_rate_sq[6] = { 0.35, 0.50, 0.75, 1.00, 1.00, 1.00 };
const double delta_rate_vmaf[6] = { 0.50, 0.70, 0.85, 1.00, 1.00, 1.00 };

// calculate the QP based on the QP scaling
uint32_t eb_vp9_qp_scaling_calc(
    SequenceControlSet *sequence_control_set_ptr,
    EB_SLICE            slice_type,
    uint32_t            temporal_layer_index,
    uint32_t            base_qp)
{

    uint32_t    scaled_qp;
    int         base_qindex;

    int qindex      = eb_vp9_quantizer_to_qindex(base_qp);
    const double q  = eb_vp9_convert_qindex_to_q(qindex, (vpx_bit_depth_t)sequence_control_set_ptr->static_config.encoder_bit_depth);
    RATE_CONTROL rc;
    rc.worst_quality = MAXQ;
    rc.best_quality = MINQ;
    int delta_qindex;

    if (slice_type == I_SLICE) {

        delta_qindex = eb_vp9_compute_qdelta(
            &rc,
            q,
            q* 0.25,
            (vpx_bit_depth_t)sequence_control_set_ptr->static_config.encoder_bit_depth);

    }
    else {

        if (sequence_control_set_ptr->static_config.tune == TUNE_OQ) {
            delta_qindex = eb_vp9_compute_qdelta(
                &rc,
                q,
#if NEW_PRED_STRUCT
                q* delta_rate_oq[0][temporal_layer_index], // RC does not support 5L
#else
                q* delta_rate_oq[temporal_layer_index],
#endif
                (vpx_bit_depth_t)sequence_control_set_ptr->static_config.encoder_bit_depth);
        }
        else if (sequence_control_set_ptr->static_config.tune == TUNE_SQ) {
            delta_qindex = eb_vp9_compute_qdelta(
                &rc,
                q,
                q* delta_rate_sq[temporal_layer_index],
                (vpx_bit_depth_t)sequence_control_set_ptr->static_config.encoder_bit_depth);
        }
        else {
            delta_qindex = eb_vp9_compute_qdelta(
                &rc,
                q,
                q* delta_rate_vmaf[temporal_layer_index],
                (vpx_bit_depth_t)sequence_control_set_ptr->static_config.encoder_bit_depth);
        }
    }

    base_qindex = VPXMAX(qindex + delta_qindex, rc.best_quality);
    scaled_qp = (uint32_t)(base_qindex) >> 2;

    return scaled_qp;

}
/*****************************
* Internal Typedefs
*****************************/
void eb_vp9_rate_control_layer_reset(
    RateControlLayerContext *rate_control_layer_ptr,
    PictureControlSet       *picture_control_set_ptr,
    RateControlContext      *rate_control_context_ptr,
    uint32_t                 picture_area_in_pixel,
    EB_BOOL                  was_used)
{

    SequenceControlSet *sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
    uint32_t            slice_num;
    uint32_t            temporal_layer_index;
    uint64_t            total_frame_in_interval;
    uint64_t            sum_bits_per_sw = 0;

    rate_control_layer_ptr->target_bit_rate = picture_control_set_ptr->parent_pcs_ptr->target_bit_rate*(uint64_t)rate_percentage_layer_array[sequence_control_set_ptr->hierarchical_levels][rate_control_layer_ptr->temporal_index] / 100;
    // update this based on temporal layers
    rate_control_layer_ptr->frame_rate = sequence_control_set_ptr->frame_rate;

    total_frame_in_interval = sequence_control_set_ptr->static_config.intra_period + 1;

    if (sequence_control_set_ptr->look_ahead_distance != 0 && sequence_control_set_ptr->intra_period != -1){
        if (picture_control_set_ptr->picture_number % ((sequence_control_set_ptr->intra_period + 1)) == 0){
            total_frame_in_interval = 0;
            for (temporal_layer_index = 0; temporal_layer_index< EB_MAX_TEMPORAL_LAYERS; temporal_layer_index++){
                rate_control_context_ptr->frames_in_interval[temporal_layer_index] = picture_control_set_ptr->parent_pcs_ptr->frames_in_interval[temporal_layer_index];
                total_frame_in_interval += picture_control_set_ptr->parent_pcs_ptr->frames_in_interval[temporal_layer_index];
                sum_bits_per_sw += picture_control_set_ptr->parent_pcs_ptr->bits_per_sw_per_layer[temporal_layer_index];
            }
#if ADAPTIVE_PERCENTAGE
            rate_control_layer_ptr->target_bit_rate = picture_control_set_ptr->parent_pcs_ptr->target_bit_rate* picture_control_set_ptr->parent_pcs_ptr->bits_per_sw_per_layer[rate_control_layer_ptr->temporal_index] / sum_bits_per_sw;
#endif
        }
    }

    if (sequence_control_set_ptr->static_config.intra_period != -1){
        rate_control_layer_ptr->frame_rate = sequence_control_set_ptr->frame_rate * rate_control_context_ptr->frames_in_interval[rate_control_layer_ptr->temporal_index] / total_frame_in_interval;
    }
    else{
        switch (picture_control_set_ptr->parent_pcs_ptr->hierarchical_levels) {
        case 0:
            break;

        case 1:
            if (sequence_control_set_ptr->static_config.intra_period == -1){
                rate_control_layer_ptr->frame_rate = rate_control_layer_ptr->frame_rate >> 1;
            }

            break;

        case 2:
            if (rate_control_layer_ptr->temporal_index == 0){
                rate_control_layer_ptr->frame_rate = rate_control_layer_ptr->frame_rate >> 2;
            }
            else{
                rate_control_layer_ptr->frame_rate = rate_control_layer_ptr->frame_rate >> (3 - rate_control_layer_ptr->temporal_index);
            }
            break;

        case 3:
            if (rate_control_layer_ptr->temporal_index == 0){
                rate_control_layer_ptr->frame_rate = rate_control_layer_ptr->frame_rate >> 3;
            }
            else {
                rate_control_layer_ptr->frame_rate = rate_control_layer_ptr->frame_rate >> (4 - rate_control_layer_ptr->temporal_index);
            }

            break;
        case 4:
            if (rate_control_layer_ptr->temporal_index == 0){
                rate_control_layer_ptr->frame_rate = rate_control_layer_ptr->frame_rate >> 4;
            }
            else {
                rate_control_layer_ptr->frame_rate = rate_control_layer_ptr->frame_rate >> (5 - rate_control_layer_ptr->temporal_index);
            }

            break;
        case 5:
            if (rate_control_layer_ptr->temporal_index == 0){
                rate_control_layer_ptr->frame_rate = rate_control_layer_ptr->frame_rate >> 5;
            }
            else {
                rate_control_layer_ptr->frame_rate = rate_control_layer_ptr->frame_rate >> (6 - rate_control_layer_ptr->temporal_index);
            }

            break;

        default:
            break;
        }
    }

    rate_control_layer_ptr->coeff_averaging_weight1 = 5;

    rate_control_layer_ptr->coeff_averaging_weight2 = 16 - rate_control_layer_ptr->coeff_averaging_weight1;
    if (rate_control_layer_ptr->frame_rate == 0){ // no frame in that layer
        rate_control_layer_ptr->frame_rate = 1 << RC_PRECISION;
    }
    rate_control_layer_ptr->channel_bit_rate = (((rate_control_layer_ptr->target_bit_rate << (2 * RC_PRECISION)) / rate_control_layer_ptr->frame_rate) + RC_PRECISION_OFFSET) >> RC_PRECISION;
    rate_control_layer_ptr->channel_bit_rate = (uint64_t)MAX((int64_t)1, (int64_t)rate_control_layer_ptr->channel_bit_rate);
    rate_control_layer_ptr->ec_bit_constraint = rate_control_layer_ptr->channel_bit_rate;

    // This is only for the initial frame, because the feedback is from packetization now and all of these are considered
    // considering the bits for slice header
    // *Note - only one-slice-per picture is supported for UHD
    slice_num = 1;

    rate_control_layer_ptr->ec_bit_constraint -= SLICE_HEADER_BITS_NUM*slice_num;

    rate_control_layer_ptr->ec_bit_constraint = MAX(1, rate_control_layer_ptr->ec_bit_constraint);

    rate_control_layer_ptr->previous_bit_constraint = rate_control_layer_ptr->channel_bit_rate;
    rate_control_layer_ptr->bit_constraint = rate_control_layer_ptr->channel_bit_rate;
    rate_control_layer_ptr->dif_total_and_ec_bits = 0;

    rate_control_layer_ptr->frame_same_distortion_min_qp_count = 0;
    rate_control_layer_ptr->max_qp = picture_control_set_ptr->picture_qp;

    rate_control_layer_ptr->alpha = 1 << (RC_PRECISION - 1);
    {
        if (!was_used){

            rate_control_layer_ptr->same_distortion_count = 0;

            rate_control_layer_ptr->k_coeff = 3 << RC_PRECISION;
            rate_control_layer_ptr->previous_k_coeff = 3 << RC_PRECISION;

            rate_control_layer_ptr->c_coeff = (rate_control_layer_ptr->channel_bit_rate << (2 * RC_PRECISION)) / picture_area_in_pixel / CCOEFF_INIT_FACT;
            rate_control_layer_ptr->previous_c_coeff = (rate_control_layer_ptr->channel_bit_rate << (2 * RC_PRECISION)) / picture_area_in_pixel / CCOEFF_INIT_FACT;

            // These are for handling Pred structure 2, when for higher temporal layer, frames can arrive in different orders
            // They should be modifed in a way that gets these from previous layers
            rate_control_layer_ptr->previous_frame_qp = 32;
            rate_control_layer_ptr->previous_frame_bit_actual = 1200;
            rate_control_layer_ptr->previous_framequantized_coeff_bit_actual = 1000;
            rate_control_layer_ptr->previous_frame_distortion_me = 10000000;
            rate_control_layer_ptr->previous_frame_qp = picture_control_set_ptr->picture_qp;
            rate_control_layer_ptr->delta_qp_fraction = 0;
            rate_control_layer_ptr->previous_frame_average_qp = picture_control_set_ptr->picture_qp;
            rate_control_layer_ptr->previous_calculated_frame_qp = picture_control_set_ptr->picture_qp;
            rate_control_layer_ptr->calculated_frame_qp = picture_control_set_ptr->picture_qp;
            rate_control_layer_ptr->critical_states = 0;
        }
        else{
            rate_control_layer_ptr->same_distortion_count = 0;
            rate_control_layer_ptr->critical_states = 0;
        }
    }
}

void eb_vp9_rate_control_layer_reset_part2(
    RateControlContext      *context_ptr,
    RateControlLayerContext *rate_control_layer_ptr,
    PictureControlSet       *picture_control_set_ptr)
{

    // update this based on temporal layers
    rate_control_layer_ptr->max_qp = (uint32_t)CLIP3(0, 63, (int32_t)context_ptr->qp_scaling_map[rate_control_layer_ptr->temporal_index][picture_control_set_ptr->picture_qp]);

    // These are for handling Pred structure 2, when for higher temporal layer, frames can arrive in different orders
    // They should be modifed in a way that gets these from previous layers
    rate_control_layer_ptr->previous_frame_qp = rate_control_layer_ptr->max_qp;
    rate_control_layer_ptr->previous_frame_average_qp = rate_control_layer_ptr->max_qp;
    rate_control_layer_ptr->previous_calculated_frame_qp = rate_control_layer_ptr->max_qp;
    rate_control_layer_ptr->calculated_frame_qp = rate_control_layer_ptr->max_qp;

}

EbErrorType eb_vp9_high_level_eb_vp9_rate_control_context_ctor(
    HighLevelRateControlContext **entry_dbl_ptr){

    HighLevelRateControlContext *entry_ptr;
    EB_MALLOC(HighLevelRateControlContext*, entry_ptr, sizeof(HighLevelRateControlContext), EB_N_PTR);
    *entry_dbl_ptr = entry_ptr;

    return EB_ErrorNone;
}

EbErrorType eb_vp9_rate_control_layer_context_ctor(
    RateControlLayerContext **entry_dbl_ptr){

    RateControlLayerContext *entry_ptr;
    EB_MALLOC(RateControlLayerContext*, entry_ptr, sizeof(RateControlLayerContext), EB_N_PTR);

    *entry_dbl_ptr = entry_ptr;

    entry_ptr->first_frame = 1;
    entry_ptr->first_non_intra_frame = 1;
    entry_ptr->feedback_arrived = EB_FALSE;

    return EB_ErrorNone;
}

EbErrorType eb_vp9_rate_control_interval_param_context_ctor(
    RateControlIntervalParamContext **entry_dbl_ptr){

    uint32_t temporal_index;
    EbErrorType return_error = EB_ErrorNone;
    RateControlIntervalParamContext *entry_ptr;
    EB_MALLOC(RateControlIntervalParamContext*, entry_ptr, sizeof(RateControlIntervalParamContext), EB_N_PTR);

    *entry_dbl_ptr = entry_ptr;

    entry_ptr->in_use = EB_FALSE;
    entry_ptr->was_used = EB_FALSE;
    entry_ptr->last_gop = EB_FALSE;
    entry_ptr->processed_frames_number = 0;
    EB_MALLOC(RateControlLayerContext**, entry_ptr->rate_control_layer_array, sizeof(RateControlLayerContext*)*EB_MAX_TEMPORAL_LAYERS, EB_N_PTR);

    for (temporal_index = 0; temporal_index < EB_MAX_TEMPORAL_LAYERS; temporal_index++){
        return_error = eb_vp9_rate_control_layer_context_ctor(&entry_ptr->rate_control_layer_array[temporal_index]);
        entry_ptr->rate_control_layer_array[temporal_index]->temporal_index = temporal_index;
        entry_ptr->rate_control_layer_array[temporal_index]->frame_rate = 1 << RC_PRECISION;
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    entry_ptr->min_target_rate_assigned = EB_FALSE;

    entry_ptr->intra_frames_qp = 0;
    entry_ptr->intra_frames_qp_bef_scal = 0;
    entry_ptr->next_gop_intra_frame_qp = 0;
    entry_ptr->first_pic_pred_bits   = 0;
    entry_ptr->first_pic_actual_bits = 0;
    entry_ptr->first_pic_pred_qp     = 0;
    entry_ptr->first_pic_actual_qp   = 0;
    entry_ptr->first_pic_actual_qp_assigned = EB_FALSE;
    entry_ptr->scene_change_in_gop = EB_FALSE;
    entry_ptr->extra_ap_bit_ratio_i = 0;

    return EB_ErrorNone;
}

EbErrorType eb_vp9_rate_control_coded_frames_stats_context_ctor(
    CodedFramesStatsEntry **entry_dbl_ptr,
    uint64_t                picture_number){

    CodedFramesStatsEntry *entry_ptr;
    EB_MALLOC(CodedFramesStatsEntry*, entry_ptr, sizeof(CodedFramesStatsEntry), EB_N_PTR);

    *entry_dbl_ptr = entry_ptr;

    entry_ptr->picture_number = picture_number;
    entry_ptr->frame_total_bit_actual = -1;

    return EB_ErrorNone;
}

EbErrorType eb_vp9_rate_control_context_ctor(
    RateControlContext **context_dbl_ptr,
    EbFifo             *rate_control_input_tasks_fifo_ptr,
    EbFifo             *rate_control_output_results_fifo_ptr,
    int32_t             intra_period)
{
    uint32_t temporal_index;
    uint32_t interval_index;

#if OVERSHOOT_STAT_PRINT
    uint32_t picture_index;
#endif

    EbErrorType return_error = EB_ErrorNone;
    RateControlContext *context_ptr;
    EB_MALLOC(RateControlContext  *, context_ptr, sizeof(RateControlContext  ), EB_N_PTR);

    *context_dbl_ptr = context_ptr;

    context_ptr->rate_control_input_tasks_fifo_ptr = rate_control_input_tasks_fifo_ptr;
    context_ptr->rate_control_output_results_fifo_ptr = rate_control_output_results_fifo_ptr;

    // High level RC
    return_error = eb_vp9_high_level_eb_vp9_rate_control_context_ctor(
        &context_ptr->high_level_rate_control_ptr);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    for (temporal_index = 0; temporal_index < EB_MAX_TEMPORAL_LAYERS; temporal_index++){
        context_ptr->frames_in_interval[temporal_index] = 0;
    }

    for (temporal_index = 0; temporal_index < EB_MAX_TEMPORAL_LAYERS; temporal_index++) {
        for (uint32_t base_qp = 0; base_qp < MAX_REF_QP_NUM; base_qp++) {
            context_ptr->qp_scaling_map[temporal_index][base_qp] = 0;
        }
    }
    for (uint32_t base_qp = 0; base_qp < MAX_REF_QP_NUM; base_qp++) {
        context_ptr->qp_scaling_map_I_SLICE[base_qp] = 0;
    }

    EB_MALLOC(RateControlIntervalParamContext  **, context_ptr->rate_control_param_queue, sizeof(RateControlIntervalParamContext  *)*PARALLEL_GOP_MAX_NUMBER, EB_N_PTR);

    context_ptr->rate_control_param_queue_head_index = 0;
    for (interval_index = 0; interval_index < PARALLEL_GOP_MAX_NUMBER; interval_index++){
        return_error = eb_vp9_rate_control_interval_param_context_ctor(
            &context_ptr->rate_control_param_queue[interval_index]);
        context_ptr->rate_control_param_queue[interval_index]->first_poc = (interval_index*(uint32_t)(intra_period + 1));
        context_ptr->rate_control_param_queue[interval_index]->last_poc = ((interval_index + 1)*(uint32_t)(intra_period + 1)) - 1;
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

#if OVERSHOOT_STAT_PRINT
    context_ptr->coded_frames_stat_queue_head_index = 0;
    context_ptr->coded_frames_stat_queue_tail_index = 0;
    EB_MALLOC(CodedFramesStatsEntry  **, context_ptr->coded_frames_stat_queue, sizeof(CodedFramesStatsEntry  *)*CODED_FRAMES_STAT_QUEUE_MAX_DEPTH, EB_N_PTR);

    for (picture_index = 0; picture_index < CODED_FRAMES_STAT_QUEUE_MAX_DEPTH; ++picture_index) {
        return_error = eb_vp9_rate_control_coded_frames_stats_context_ctor(
            &context_ptr->coded_frames_stat_queue[picture_index],
            picture_index);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    context_ptr->max_bit_actual_per_sw = 0;
    context_ptr->max_bit_actual_per_gop = 0;
    context_ptr->min_bit_actual_per_gop = 0xfffffffffffff;
    context_ptr->avg_bit_actual_per_gop = 0;
#endif

    context_ptr->base_layer_frames_avg_qp = 0;
    context_ptr->base_layer_intra_frames_avg_qp = 0;

    context_ptr->intra_coef_rate = 4;
    context_ptr->extra_bits = 0;
    context_ptr->extra_bits_gen = 0;
    context_ptr->max_rate_adjust_delta_qp = 0;

    return EB_ErrorNone;
}
void eb_vp9_high_level_rc_input_picture_vbr(
    PictureParentControlSet     *picture_control_set_ptr,
    SequenceControlSet          *sequence_control_set_ptr,
    EncodeContext               *encode_context_ptr,
    RateControlContext          *context_ptr,
    HighLevelRateControlContext *high_level_rate_control_ptr)
{

    EB_BOOL                      end_of_sequence_flag = EB_TRUE;

    HlRateControlHistogramEntry *hl_rate_control_histogram_ptr_temp;
    // Queue variables
    uint32_t                     queue_entry_index_temp;
    uint32_t                     queue_entry_index_temp2;
    uint32_t                     queue_entry_index_head_temp;

    uint64_t                     min_la_bit_distance;
    uint32_t                     selected_ref_qp_table_index;
    uint32_t                     selected_ref_qp;
#if RC_UPDATE_TARGET_RATE
    uint32_t                     selected_org_ref_qp;
#endif
    uint32_t                     previous_selected_ref_qp = encode_context_ptr->previous_selected_ref_qp;
    uint64_t                     max_coded_poc = encode_context_ptr->max_coded_poc;
    uint32_t                     max_coded_poc_selected_ref_qp = encode_context_ptr->max_coded_poc_selected_ref_qp;

    uint32_t                     ref_qp_index;
    uint32_t                     ref_qp_index_temp;
    uint32_t                     ref_qp_table_index;

    uint32_t                     area_in_pixel;
    uint32_t                     num_of_full_sbs;
    uint32_t                     qp_search_min;
    uint32_t                     qp_search_max;
    int32_t                      qp_step = 1;
    EB_BOOL                      best_qp_found;
    uint32_t                     temporal_layer_index;
    EB_BOOL                      tables_updated;

    uint64_t                     bit_constraint_per_sw = 0;

    RateControlTables           *rate_control_tables_ptr;
    EbBitNumber                 *sad_bits_array_ptr;
    EbBitNumber                 *intra_sad_bits_array_ptr;
    uint32_t                     pred_bits_ref_qp;

    for (temporal_layer_index = 0; temporal_layer_index < EB_MAX_TEMPORAL_LAYERS; temporal_layer_index++) {
        picture_control_set_ptr->bits_per_sw_per_layer[temporal_layer_index] = 0;
    }
    picture_control_set_ptr->sb_total_bits_per_gop = 0;

    area_in_pixel = sequence_control_set_ptr->luma_width * sequence_control_set_ptr->luma_height;;

    eb_vp9_block_on_mutex(sequence_control_set_ptr->encode_context_ptr->rate_table_update_mutex);

    tables_updated = sequence_control_set_ptr->encode_context_ptr->rate_control_tables_array_updated;
    picture_control_set_ptr->percentage_updated = EB_FALSE;

    if (sequence_control_set_ptr->look_ahead_distance != 0) {

        // Increamenting the head of the hl_rate_control_historgram_queue and clean up the entores
        hl_rate_control_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]);
        while ((hl_rate_control_histogram_ptr_temp->life_count == 0) && hl_rate_control_histogram_ptr_temp->passed_to_hlrc) {

            eb_vp9_block_on_mutex(sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue_mutex);
            // Reset the Reorder Queue Entry
            hl_rate_control_histogram_ptr_temp->picture_number += INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH;
            hl_rate_control_histogram_ptr_temp->life_count = -1;
            hl_rate_control_histogram_ptr_temp->passed_to_hlrc = EB_FALSE;
            hl_rate_control_histogram_ptr_temp->is_coded = EB_FALSE;
            hl_rate_control_histogram_ptr_temp->total_num_bitsCoded = 0;

            // Increment the Reorder Queue head Ptr
            encode_context_ptr->hl_rate_control_historgram_queue_head_index =
                (encode_context_ptr->hl_rate_control_historgram_queue_head_index == HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? 0 : encode_context_ptr->hl_rate_control_historgram_queue_head_index + 1;
            eb_vp9_release_mutex(sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue_mutex);
            hl_rate_control_histogram_ptr_temp = encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index];

        }
        // For the case that number of frames in the sliding window is less than size of the look ahead or intra Refresh. i.e. end of sequence
        if ((picture_control_set_ptr->frames_in_sw < MIN(sequence_control_set_ptr->look_ahead_distance + 1, (uint32_t)sequence_control_set_ptr->intra_period + 1))) {

            selected_ref_qp = max_coded_poc_selected_ref_qp;

            // Update the QP for the sliding window based on the status of RC
            if ((context_ptr->extra_bits_gen > (int64_t)(context_ptr->virtual_buffer_size << 3))) {
                selected_ref_qp = (uint32_t)MAX((int32_t)selected_ref_qp - 2, 0);
            }
            else if ((context_ptr->extra_bits_gen > (int64_t)(context_ptr->virtual_buffer_size << 2))) {
                selected_ref_qp = (uint32_t)MAX((int32_t)selected_ref_qp - 1, 0);
            }
            if ((context_ptr->extra_bits_gen < -(int64_t)(context_ptr->virtual_buffer_size << 2))) {
                selected_ref_qp += 2;
            }
            else if ((context_ptr->extra_bits_gen < -(int64_t)(context_ptr->virtual_buffer_size << 1))) {
                selected_ref_qp += 1;
            }

            if ((picture_control_set_ptr->frames_in_sw < (uint32_t)(sequence_control_set_ptr->intra_period + 1)) &&
                (picture_control_set_ptr->picture_number % ((sequence_control_set_ptr->intra_period + 1)) == 0)) {
                selected_ref_qp++;
            }

            selected_ref_qp = (uint32_t)CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                sequence_control_set_ptr->static_config.max_qp_allowed,
                selected_ref_qp);

            queue_entry_index_head_temp = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
            queue_entry_index_head_temp += encode_context_ptr->hl_rate_control_historgram_queue_head_index;
            queue_entry_index_head_temp = (queue_entry_index_head_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                queue_entry_index_head_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                queue_entry_index_head_temp;

            queue_entry_index_temp = queue_entry_index_head_temp;
            {

                hl_rate_control_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_temp]);

                if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE)
                    ref_qp_index_temp = context_ptr->qp_scaling_map_I_SLICE[selected_ref_qp];
                else
                    ref_qp_index_temp = context_ptr->qp_scaling_map[hl_rate_control_histogram_ptr_temp->temporal_layer_index][selected_ref_qp];

                ref_qp_index_temp = (uint32_t)CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    ref_qp_index_temp);

                hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = 0;
                rate_control_tables_ptr = &encode_context_ptr->rate_control_tables_array[ref_qp_index_temp];
                sad_bits_array_ptr = rate_control_tables_ptr->sad_bits_array[hl_rate_control_histogram_ptr_temp->temporal_layer_index];
                intra_sad_bits_array_ptr = rate_control_tables_ptr->intra_sad_bits_array[hl_rate_control_histogram_ptr_temp->temporal_layer_index];
                pred_bits_ref_qp = 0;
                num_of_full_sbs = 0;

                if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE) {
                    // Loop over block in the frame and calculated the predicted bits at reg QP
                    {
                        unsigned i;
                        uint32_t accum = 0;
                        for (i = 0; i < NUMBER_OF_INTRA_SAD_INTERVALS; ++i)
                        {
                            accum += (uint32_t)(hl_rate_control_histogram_ptr_temp->ois_distortion_histogram[i] * intra_sad_bits_array_ptr[i]);
                        }

                        pred_bits_ref_qp = accum;
                        num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;
                    }
                    hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                }

                else {
                    {
                        unsigned i;
                        uint32_t accum = 0;
                        for (i = 0; i < NUMBER_OF_SAD_INTERVALS; ++i)
                        {
                            accum += (uint32_t)(hl_rate_control_histogram_ptr_temp->me_distortion_histogram[i] * sad_bits_array_ptr[i]);
                        }

                        pred_bits_ref_qp = accum;
                        num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;

                    }
                    hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                }

                // Scale for in complete
                //  pred_bits_ref_qp is normalized based on the area because of the LCUs at the picture boundries
                hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] * (uint64_t)area_in_pixel / (num_of_full_sbs << 12);

                // Store the pred_bits_ref_qp for the first frame in the window to PCS
                picture_control_set_ptr->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];

            }
        }
        else {
            // Loop over the QPs and find the best QP
            min_la_bit_distance = MAX_UNSIGNED_VALUE;
            qp_search_min = (uint8_t)CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                sequence_control_set_ptr->static_config.max_qp_allowed,
                (uint32_t)MAX((int32_t)sequence_control_set_ptr->qp - 40, 0));

            qp_search_max = (uint8_t)CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                sequence_control_set_ptr->static_config.max_qp_allowed,
                sequence_control_set_ptr->qp + 40);

            for (ref_qp_table_index = qp_search_min; ref_qp_table_index < qp_search_max; ref_qp_table_index++) {
                high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_table_index] = 0;
            }

            bit_constraint_per_sw = high_level_rate_control_ptr->bit_constraint_per_sw * picture_control_set_ptr->frames_in_sw / (sequence_control_set_ptr->look_ahead_distance + 1);

            // Update the target rate for the sliding window based on the status of RC
            if ((context_ptr->extra_bits_gen > (int64_t)(context_ptr->virtual_buffer_size * 10))) {
                bit_constraint_per_sw = bit_constraint_per_sw * 130 / 100;
            }
            else if ((context_ptr->extra_bits_gen > (int64_t)(context_ptr->virtual_buffer_size << 3))) {
                bit_constraint_per_sw = bit_constraint_per_sw * 120 / 100;
            }
            else if ((context_ptr->extra_bits_gen > (int64_t)(context_ptr->virtual_buffer_size << 2))) {
                bit_constraint_per_sw = bit_constraint_per_sw * 110 / 100;
            }
            if ((context_ptr->extra_bits_gen < -(int64_t)(context_ptr->virtual_buffer_size << 3))) {
                bit_constraint_per_sw = bit_constraint_per_sw * 80 / 100;
            }
            else if ((context_ptr->extra_bits_gen < -(int64_t)(context_ptr->virtual_buffer_size << 2))) {
                bit_constraint_per_sw = bit_constraint_per_sw * 90 / 100;
            }

            // Loop over proper QPs and find the Predicted bits for that QP. Find the QP with the closest total predicted rate to target bits for the sliding window.
            previous_selected_ref_qp = CLIP3(
                qp_search_min,
                qp_search_max,
                previous_selected_ref_qp);
            ref_qp_table_index = previous_selected_ref_qp;
            selected_ref_qp_table_index = ref_qp_table_index;
            selected_ref_qp = selected_ref_qp_table_index;
            best_qp_found = EB_FALSE;
            while (ref_qp_table_index >= qp_search_min && ref_qp_table_index <= qp_search_max && !best_qp_found) {

                ref_qp_index = CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    ref_qp_table_index);
                high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] = 0;

                // Finding the predicted bits for each frame in the sliding window at the reference Qp(s)
                queue_entry_index_head_temp = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
                queue_entry_index_head_temp += encode_context_ptr->hl_rate_control_historgram_queue_head_index;
                queue_entry_index_head_temp = (queue_entry_index_head_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                    queue_entry_index_head_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                    queue_entry_index_head_temp;

                queue_entry_index_temp = queue_entry_index_head_temp;
                // This is set to false, so the last frame would go inside the loop
                end_of_sequence_flag = EB_FALSE;

                while (!end_of_sequence_flag &&
                    queue_entry_index_temp <= queue_entry_index_head_temp + sequence_control_set_ptr->look_ahead_distance) {

                    queue_entry_index_temp2 = (queue_entry_index_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH : queue_entry_index_temp;
                    hl_rate_control_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_temp2]);

                    if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE)
                        ref_qp_index_temp = context_ptr->qp_scaling_map_I_SLICE[ref_qp_index];
                    else
                        ref_qp_index_temp = context_ptr->qp_scaling_map[hl_rate_control_histogram_ptr_temp->temporal_layer_index][ref_qp_index];

                    ref_qp_index_temp = (uint32_t)CLIP3(
                        sequence_control_set_ptr->static_config.min_qp_allowed,
                        sequence_control_set_ptr->static_config.max_qp_allowed,
                        ref_qp_index_temp);

                    hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = 0;

                    if (ref_qp_table_index == previous_selected_ref_qp) {
                        hl_rate_control_histogram_ptr_temp->life_count--;
                    }
                    if (hl_rate_control_histogram_ptr_temp->is_coded) {
                        // If the frame is already coded, use the actual number of bits
                        hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->total_num_bitsCoded;
                    }
                    else {
                        rate_control_tables_ptr = &encode_context_ptr->rate_control_tables_array[ref_qp_index_temp];
                        sad_bits_array_ptr = rate_control_tables_ptr->sad_bits_array[hl_rate_control_histogram_ptr_temp->temporal_layer_index];
                        intra_sad_bits_array_ptr = rate_control_tables_ptr->intra_sad_bits_array[0];
                        pred_bits_ref_qp = 0;
                        num_of_full_sbs = 0;

                        if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE) {
                            // Loop over block in the frame and calculated the predicted bits at reg QP
                            unsigned i;
                            uint32_t accum = 0;
                            for (i = 0; i < NUMBER_OF_INTRA_SAD_INTERVALS; ++i)
                            {
                                accum += (uint32_t)((uint32_t)hl_rate_control_histogram_ptr_temp->ois_distortion_histogram[i] * (uint32_t)intra_sad_bits_array_ptr[i]);
                            }

                            pred_bits_ref_qp = accum;
                            num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;
                            hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                        }
                        else {
                            unsigned i;
                            uint32_t accum = 0;
                            uint32_t accum_intra = 0;
                            for (i = 0; i < NUMBER_OF_SAD_INTERVALS; ++i)
                            {
                                accum += (uint32_t)((uint32_t)hl_rate_control_histogram_ptr_temp->me_distortion_histogram[i] * (uint32_t)sad_bits_array_ptr[i]);
                                accum_intra += (uint32_t)((uint32_t)hl_rate_control_histogram_ptr_temp->ois_distortion_histogram[i] * (uint32_t)intra_sad_bits_array_ptr[i]);

                            }
                            if (accum > accum_intra * 3)
                                pred_bits_ref_qp = accum_intra;
                            else
                                pred_bits_ref_qp = accum;
                            num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;
                            hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                        }

                        // Scale for in complete LCSs
                        //  pred_bits_ref_qp is normalized based on the area because of the LCUs at the picture boundries
                        hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] * (uint64_t)area_in_pixel / (num_of_full_sbs << 12);

                    }
                    high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] += hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];
                    //if (picture_control_set_ptr->slice_type == I_SLICE) {
                    //    //if (picture_control_set_ptr->picture_number > 280 && picture_control_set_ptr->picture_number < 350) {
                    //    SVT_LOG("%d\t%d\t%d\t%d\t%d\n", picture_control_set_ptr->picture_number, ref_qp_index_temp, ref_qp_index, hl_rate_control_histogram_ptr_temp->picture_number, hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp]);
                    //}
                    // Store the pred_bits_ref_qp for the first frame in the window to PCS
                    if (queue_entry_index_head_temp == queue_entry_index_temp2)
                        picture_control_set_ptr->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];

                    end_of_sequence_flag = hl_rate_control_histogram_ptr_temp->end_of_sequence_flag;
                    queue_entry_index_temp++;
                }

                if (min_la_bit_distance >= (uint64_t)ABS((int64_t)high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] - (int64_t)bit_constraint_per_sw)) {
                    min_la_bit_distance = (uint64_t)ABS((int64_t)high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] - (int64_t)bit_constraint_per_sw);
                    selected_ref_qp_table_index = ref_qp_table_index;
                    selected_ref_qp = ref_qp_index;
                }
                else {
                    best_qp_found = EB_TRUE;
                }

                if (ref_qp_table_index == previous_selected_ref_qp) {
                    if (high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] > bit_constraint_per_sw) {
                        qp_step = +1;
                    }
                    else {
                        qp_step = -1;
                    }
                }
                ref_qp_table_index = (uint32_t)(ref_qp_table_index + qp_step);

            }
        }

#if RC_UPDATE_TARGET_RATE
        selected_org_ref_qp = selected_ref_qp;
        if (sequence_control_set_ptr->intra_period != -1 && picture_control_set_ptr->picture_number % ((sequence_control_set_ptr->intra_period + 1)) == 0 &&
            (int32_t)picture_control_set_ptr->frames_in_sw > sequence_control_set_ptr->intra_period) {
            if (picture_control_set_ptr->picture_number > 0) {
                picture_control_set_ptr->intra_selected_org_qp = (uint8_t)selected_ref_qp;
            }
            ref_qp_index = selected_ref_qp;
            high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] = 0;

            if (high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] == 0) {

                // Finding the predicted bits for each frame in the sliding window at the reference Qp(s)
                //queue_entry_index_temp = encode_context_ptr->hl_rate_control_historgram_queue_head_index;
                queue_entry_index_head_temp = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
                queue_entry_index_head_temp += encode_context_ptr->hl_rate_control_historgram_queue_head_index;
                queue_entry_index_head_temp = (queue_entry_index_head_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                    queue_entry_index_head_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                    queue_entry_index_head_temp;

                queue_entry_index_temp = queue_entry_index_head_temp;

                // This is set to false, so the last frame would go inside the loop
                end_of_sequence_flag = EB_FALSE;

                while (!end_of_sequence_flag &&
                    //queue_entry_index_temp <= encode_context_ptr->hl_rate_control_historgram_queue_head_index+sequence_control_set_ptr->look_ahead_distance){
                    queue_entry_index_temp <= queue_entry_index_head_temp + sequence_control_set_ptr->look_ahead_distance) {

                    queue_entry_index_temp2 = (queue_entry_index_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH : queue_entry_index_temp;
                    hl_rate_control_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_temp2]);

                    if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE)
                        ref_qp_index_temp = context_ptr->qp_scaling_map_I_SLICE[ref_qp_index];
                    else
                        ref_qp_index_temp = context_ptr->qp_scaling_map[hl_rate_control_histogram_ptr_temp->temporal_layer_index][ref_qp_index];

                    ref_qp_index_temp = (uint32_t)CLIP3(
                        sequence_control_set_ptr->static_config.min_qp_allowed,
                        sequence_control_set_ptr->static_config.max_qp_allowed,
                        ref_qp_index_temp);

                    hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = 0;

                    if (hl_rate_control_histogram_ptr_temp->is_coded) {
                        hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->total_num_bitsCoded;
                    }
                    else {
                        rate_control_tables_ptr = &encode_context_ptr->rate_control_tables_array[ref_qp_index_temp];
                        sad_bits_array_ptr = rate_control_tables_ptr->sad_bits_array[hl_rate_control_histogram_ptr_temp->temporal_layer_index];
                        intra_sad_bits_array_ptr = rate_control_tables_ptr->intra_sad_bits_array[hl_rate_control_histogram_ptr_temp->temporal_layer_index];
                        pred_bits_ref_qp = 0;

                        num_of_full_sbs = 0;

                        if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE) {
                            // Loop over block in the frame and calculated the predicted bits at reg QP

                            {
                                unsigned i;
                                uint32_t accum = 0;
                                for (i = 0; i < NUMBER_OF_INTRA_SAD_INTERVALS; ++i)
                                {
                                    accum += (uint32_t)(hl_rate_control_histogram_ptr_temp->ois_distortion_histogram[i] * intra_sad_bits_array_ptr[i]);
                                }

                                pred_bits_ref_qp = accum;
                                num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;
                            }
                            hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                        }

                        else {
                            unsigned i;
                            uint32_t accum = 0;
                            uint32_t accum_intra = 0;
                            for (i = 0; i < NUMBER_OF_SAD_INTERVALS; ++i)
                            {
                                accum += (uint32_t)(hl_rate_control_histogram_ptr_temp->me_distortion_histogram[i] * sad_bits_array_ptr[i]);
                                accum_intra += (uint32_t)(hl_rate_control_histogram_ptr_temp->ois_distortion_histogram[i] * intra_sad_bits_array_ptr[i]);

                            }
                            if (accum > accum_intra * 3)
                                pred_bits_ref_qp = accum_intra;
                            else
                                pred_bits_ref_qp = accum;
                            num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;
                            hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                        }

                        // Scale for in complete
                        //  pred_bits_ref_qp is normalized based on the area because of the LCUs at the picture boundries
                        hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] * (uint64_t)area_in_pixel / (num_of_full_sbs << 12);

                    }
                    high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] += hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];
                    // Store the pred_bits_ref_qp for the first frame in the window to PCS
                    //  if(encode_context_ptr->hl_rate_control_historgram_queue_head_index == queue_entry_index_temp2)
                    if (queue_entry_index_head_temp == queue_entry_index_temp2)
                        picture_control_set_ptr->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];

                    end_of_sequence_flag = hl_rate_control_histogram_ptr_temp->end_of_sequence_flag;
                    queue_entry_index_temp++;
                }
            }
        }
#endif
        picture_control_set_ptr->tables_updated = tables_updated;
        EB_BOOL expensive_i_slice = EB_FALSE;
        // Looping over the window to find the percentage of bit allocation in each layer
        if ((sequence_control_set_ptr->intra_period != -1) &&
            ((int32_t)picture_control_set_ptr->frames_in_sw > sequence_control_set_ptr->intra_period) &&
            ((int32_t)picture_control_set_ptr->frames_in_sw > sequence_control_set_ptr->intra_period)) {
            uint64_t i_slice_bits = 0;

            if (picture_control_set_ptr->picture_number % ((sequence_control_set_ptr->intra_period + 1)) == 0) {

                queue_entry_index_head_temp = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
                queue_entry_index_head_temp += encode_context_ptr->hl_rate_control_historgram_queue_head_index;
                queue_entry_index_head_temp = (queue_entry_index_head_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                    queue_entry_index_head_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                    queue_entry_index_head_temp;

                queue_entry_index_temp = queue_entry_index_head_temp;

                // This is set to false, so the last frame would go inside the loop
                end_of_sequence_flag = EB_FALSE;

                while (!end_of_sequence_flag &&
                    queue_entry_index_temp <= queue_entry_index_head_temp + sequence_control_set_ptr->intra_period) {

                    queue_entry_index_temp2 = (queue_entry_index_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH : queue_entry_index_temp;
                    hl_rate_control_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_temp2]);

                    if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE)
                        ref_qp_index_temp = context_ptr->qp_scaling_map_I_SLICE[selected_ref_qp];
                    else
                        ref_qp_index_temp = context_ptr->qp_scaling_map[hl_rate_control_histogram_ptr_temp->temporal_layer_index][selected_ref_qp];

                    ref_qp_index_temp = (uint32_t)CLIP3(
                        sequence_control_set_ptr->static_config.min_qp_allowed,
                        sequence_control_set_ptr->static_config.max_qp_allowed,
                        ref_qp_index_temp);

                    if (queue_entry_index_temp == queue_entry_index_head_temp) {
                        i_slice_bits = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];
                    }
                    picture_control_set_ptr->sb_total_bits_per_gop += hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];
                    picture_control_set_ptr->bits_per_sw_per_layer[hl_rate_control_histogram_ptr_temp->temporal_layer_index] += hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];
                    picture_control_set_ptr->percentage_updated = EB_TRUE;

                    end_of_sequence_flag = hl_rate_control_histogram_ptr_temp->end_of_sequence_flag;
                    queue_entry_index_temp++;
                }
                if (i_slice_bits * 100 > 85 * picture_control_set_ptr->sb_total_bits_per_gop) {
                    expensive_i_slice = EB_TRUE;
                }
                if (picture_control_set_ptr->sb_total_bits_per_gop == 0) {
                    for (temporal_layer_index = 0; temporal_layer_index < EB_MAX_TEMPORAL_LAYERS; temporal_layer_index++) {
                        picture_control_set_ptr->bits_per_sw_per_layer[temporal_layer_index] = rate_percentage_layer_array[sequence_control_set_ptr->hierarchical_levels][temporal_layer_index];
                    }
                }
            }
        }
        else {
            for (temporal_layer_index = 0; temporal_layer_index < EB_MAX_TEMPORAL_LAYERS; temporal_layer_index++) {
                picture_control_set_ptr->bits_per_sw_per_layer[temporal_layer_index] = rate_percentage_layer_array[sequence_control_set_ptr->hierarchical_levels][temporal_layer_index];
            }
        }
        if (expensive_i_slice) {
            if (tables_updated) {
                selected_ref_qp = (uint32_t)MAX((int32_t)selected_ref_qp - 1, 0);
            }
            else {
                selected_ref_qp = (uint32_t)MAX((int32_t)selected_ref_qp - 3, 0);
            }
            selected_ref_qp = (uint32_t)CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                sequence_control_set_ptr->static_config.max_qp_allowed,
                selected_ref_qp);
        }
        // Set the QP
        previous_selected_ref_qp = selected_ref_qp;
        if (picture_control_set_ptr->picture_number > max_coded_poc && picture_control_set_ptr->temporal_layer_index < 2 && !picture_control_set_ptr->end_of_sequence_region) {

            max_coded_poc = picture_control_set_ptr->picture_number;
            max_coded_poc_selected_ref_qp = previous_selected_ref_qp;
            encode_context_ptr->previous_selected_ref_qp = previous_selected_ref_qp;
            encode_context_ptr->max_coded_poc = max_coded_poc;
            encode_context_ptr->max_coded_poc_selected_ref_qp = max_coded_poc_selected_ref_qp;

        }

        if (picture_control_set_ptr->slice_type == I_SLICE)
            picture_control_set_ptr->best_pred_qp = (uint8_t)context_ptr->qp_scaling_map_I_SLICE[selected_ref_qp];
        else
            picture_control_set_ptr->best_pred_qp = (uint8_t)context_ptr->qp_scaling_map[picture_control_set_ptr->temporal_layer_index][selected_ref_qp];

        picture_control_set_ptr->best_pred_qp = (uint8_t)CLIP3(
            sequence_control_set_ptr->static_config.min_qp_allowed,
            sequence_control_set_ptr->static_config.max_qp_allowed,
            picture_control_set_ptr->best_pred_qp);

#if RC_UPDATE_TARGET_RATE
        if (picture_control_set_ptr->picture_number == 0) {
            high_level_rate_control_ptr->prev_intra_selected_ref_qp = selected_ref_qp;
            high_level_rate_control_ptr->prev_intra_org_selected_ref_qp = selected_ref_qp;
        }
        if (sequence_control_set_ptr->intra_period != -1) {
            if (picture_control_set_ptr->picture_number % ((sequence_control_set_ptr->intra_period + 1)) == 0) {
                high_level_rate_control_ptr->prev_intra_selected_ref_qp = selected_ref_qp;
                high_level_rate_control_ptr->prev_intra_org_selected_ref_qp = selected_org_ref_qp;
            }
        }
#endif
        picture_control_set_ptr->target_bits_best_pred_qp = picture_control_set_ptr->pred_bits_ref_qp[picture_control_set_ptr->best_pred_qp];
#if 0//VP9_RC_PRINTS
        ////if (picture_control_set_ptr->slice_type == 2)
        {
            SVT_LOG("\nTID: %d\t", picture_control_set_ptr->temporal_layer_index);
            SVT_LOG("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n",
                picture_control_set_ptr->picture_number,
                picture_control_set_ptr->best_pred_qp,
                selected_ref_qp,
                (int)picture_control_set_ptr->target_bits_best_pred_qp,
                (int)high_level_rate_control_ptr->pred_bits_ref_qpPerSw[selected_ref_qp - 1],
                (int)high_level_rate_control_ptr->pred_bits_ref_qpPerSw[selected_ref_qp],
                (int)high_level_rate_control_ptr->pred_bits_ref_qpPerSw[selected_ref_qp + 1],
                (int)high_level_rate_control_ptr->bit_constraint_per_sw,
                (int)bit_constraint_per_sw/*,
                (int)high_level_rate_control_ptr->virtual_buffer_level*/);
        }
#endif
    }
    eb_vp9_release_mutex(sequence_control_set_ptr->encode_context_ptr->rate_table_update_mutex);
}
void eb_vp9_frame_level_rc_input_picture_vbr(
    PictureControlSet               *picture_control_set_ptr,
    SequenceControlSet              *sequence_control_set_ptr,
    RateControlContext              *context_ptr,
    RateControlLayerContext         *rate_control_layer_ptr,
    RateControlIntervalParamContext *rate_control_param_ptr)
{

    RateControlLayerContext *rate_control_layer_temp_ptr;

    // Tiles
    uint32_t                 picture_area_in_pixel;
    uint32_t                 area_in_pixel;

    // SB Loop variables
    SbParams                *sb_params_ptr;
    uint32_t                 sb_index;
    uint64_t                 temp_qp;
    uint32_t                 area_in_sbs;

    picture_area_in_pixel = sequence_control_set_ptr->luma_height*sequence_control_set_ptr->luma_width;

    if (rate_control_layer_ptr->first_frame == 1) {
        rate_control_layer_ptr->first_frame = 0;
        picture_control_set_ptr->parent_pcs_ptr->first_frame_in_temporal_layer = 1;
    }
    else {
        picture_control_set_ptr->parent_pcs_ptr->first_frame_in_temporal_layer = 0;
    }
    if (picture_control_set_ptr->slice_type != I_SLICE) {
        if (rate_control_layer_ptr->first_non_intra_frame == 1) {
            rate_control_layer_ptr->first_non_intra_frame = 0;
            picture_control_set_ptr->parent_pcs_ptr->first_non_intra_frame_in_temporal_layer = 1;
        }
        else {
            picture_control_set_ptr->parent_pcs_ptr->first_non_intra_frame_in_temporal_layer = 0;
        }
    }
    else
        picture_control_set_ptr->parent_pcs_ptr->first_non_intra_frame_in_temporal_layer = 0;

    picture_control_set_ptr->parent_pcs_ptr->target_bits_rc = 0;

    // ***Rate Control***
    area_in_sbs = 0;
    area_in_pixel = 0;

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

        sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];

        if (sb_params_ptr->is_complete_sb) {
            // add the area of one LCU (64x64=4096) to the area of the tile
            area_in_pixel += 4096;
            area_in_sbs++;
        }
        else {
            // add the area of the LCU to the area of the tile
            area_in_pixel += sb_params_ptr->width * sb_params_ptr->height;
        }
    }
    rate_control_layer_ptr->area_in_pixel = area_in_pixel;

    if (picture_control_set_ptr->parent_pcs_ptr->first_frame_in_temporal_layer || (picture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc)) {
        if (sequence_control_set_ptr->enable_qp_scaling_flag && (picture_control_set_ptr->picture_number != rate_control_param_ptr->first_poc)) {

            picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                (int32_t)sequence_control_set_ptr->static_config.min_qp_allowed,
                (int32_t)sequence_control_set_ptr->static_config.max_qp_allowed,
                (int32_t)(rate_control_param_ptr->intra_frames_qp + context_ptr->qp_scaling_map[picture_control_set_ptr->temporal_layer_index][rate_control_param_ptr->intra_frames_qp_bef_scal] - context_ptr->qp_scaling_map_I_SLICE[rate_control_param_ptr->intra_frames_qp_bef_scal]));
        }

        if (picture_control_set_ptr->picture_number == 0) {
            rate_control_param_ptr->intra_frames_qp = sequence_control_set_ptr->qp;
            rate_control_param_ptr->intra_frames_qp_bef_scal = (uint8_t)sequence_control_set_ptr->qp;
        }

        if (picture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc) {
            uint32_t temporal_layer_idex;
            rate_control_param_ptr->previous_virtual_buffer_level = context_ptr->virtual_buffer_level_initial_value;
            rate_control_param_ptr->virtual_buffer_level = context_ptr->virtual_buffer_level_initial_value;
            rate_control_param_ptr->extra_ap_bit_ratio_i = 0;
            if (picture_control_set_ptr->parent_pcs_ptr->end_of_sequence_region) {
                rate_control_param_ptr->last_poc = MAX(rate_control_param_ptr->first_poc + picture_control_set_ptr->parent_pcs_ptr->frames_in_sw - 1, rate_control_param_ptr->first_poc);
                rate_control_param_ptr->last_gop = EB_TRUE;
            }

            if ((context_ptr->extra_bits > (int64_t)(context_ptr->virtual_buffer_size >> 8)) ||
                (context_ptr->extra_bits < -(int64_t)(context_ptr->virtual_buffer_size >> 8))) {

                int64_t extra_bits_per_gop = 0;

                if (picture_control_set_ptr->parent_pcs_ptr->end_of_sequence_region) {
                    if ((context_ptr->extra_bits > (int64_t)(context_ptr->virtual_buffer_size << 4)) ||
                        (context_ptr->extra_bits < -(int64_t)(context_ptr->virtual_buffer_size << 4))) {
                        extra_bits_per_gop = context_ptr->extra_bits;
                        extra_bits_per_gop = CLIP3(
                            -(int64_t)(context_ptr->vb_fill_threshold2 << 3),
                            (int64_t)(context_ptr->vb_fill_threshold2 << 3),
                            extra_bits_per_gop);
                    }
                    else
                        if ((context_ptr->extra_bits > (int64_t)(context_ptr->virtual_buffer_size << 3)) ||
                            (context_ptr->extra_bits < -(int64_t)(context_ptr->virtual_buffer_size << 3))) {
                            extra_bits_per_gop = context_ptr->extra_bits;
                            extra_bits_per_gop = CLIP3(
                                -(int64_t)(context_ptr->vb_fill_threshold2 << 2),
                                (int64_t)(context_ptr->vb_fill_threshold2 << 2),
                                extra_bits_per_gop);
                        }
                        else if ((context_ptr->extra_bits > (int64_t)(context_ptr->virtual_buffer_size << 2)) ||
                            (context_ptr->extra_bits < -(int64_t)(context_ptr->virtual_buffer_size << 2))) {

                            extra_bits_per_gop = CLIP3(
                                -(int64_t)context_ptr->vb_fill_threshold2 << 1,
                                (int64_t)context_ptr->vb_fill_threshold2 << 1,
                                extra_bits_per_gop);
                        }
                        else {
                            extra_bits_per_gop = CLIP3(
                                -(int64_t)context_ptr->vb_fill_threshold1,
                                (int64_t)context_ptr->vb_fill_threshold1,
                                extra_bits_per_gop);
                        }
                }
                else {
                    if ((context_ptr->extra_bits > (int64_t)(context_ptr->virtual_buffer_size << 3)) ||
                        (context_ptr->extra_bits < -(int64_t)(context_ptr->virtual_buffer_size << 3))) {
                        extra_bits_per_gop = context_ptr->extra_bits;
                        extra_bits_per_gop = CLIP3(
                            -(int64_t)(context_ptr->vb_fill_threshold2 << 2),
                            (int64_t)(context_ptr->vb_fill_threshold2 << 2),
                            extra_bits_per_gop);
                    }
                    else if ((context_ptr->extra_bits > (int64_t)(context_ptr->virtual_buffer_size << 2)) ||
                        (context_ptr->extra_bits < -(int64_t)(context_ptr->virtual_buffer_size << 2))) {

                        extra_bits_per_gop = CLIP3(
                            -(int64_t)context_ptr->vb_fill_threshold2 << 1,
                            (int64_t)context_ptr->vb_fill_threshold2 << 1,
                            extra_bits_per_gop);
                    }
                }

                rate_control_param_ptr->virtual_buffer_level -= extra_bits_per_gop;
                rate_control_param_ptr->previous_virtual_buffer_level -= extra_bits_per_gop;
                context_ptr->extra_bits -= extra_bits_per_gop;
            }

            for (temporal_layer_idex = 0; temporal_layer_idex < EB_MAX_TEMPORAL_LAYERS; temporal_layer_idex++) {

                rate_control_layer_temp_ptr = rate_control_param_ptr->rate_control_layer_array[temporal_layer_idex];
                eb_vp9_rate_control_layer_reset(
                    rate_control_layer_temp_ptr,
                    picture_control_set_ptr,
                    context_ptr,
                    picture_area_in_pixel,
                    rate_control_param_ptr->was_used);
            }
        }

        picture_control_set_ptr->parent_pcs_ptr->sad_me = 0;
        // Finding the QP of the Intra frame by using variance tables
        if (picture_control_set_ptr->slice_type == I_SLICE) {
            uint32_t         selected_ref_qp;

            if (sequence_control_set_ptr->look_ahead_distance == 0) {
                uint32_t         selected_ref_qp_table_index;
                uint32_t         intra_sad_interval_index;
                uint32_t         ref_qp_index;
                uint32_t         ref_qp_table_index;
                uint32_t         qp_search_min;
                uint32_t         qp_search_max;
                uint32_t         num_of_full_sbs;
                uint64_t         min_la_bit_distance;

                min_la_bit_distance = MAX_UNSIGNED_VALUE;
                selected_ref_qp_table_index = 0;
                selected_ref_qp = selected_ref_qp_table_index;
                qp_search_min = (uint32_t)CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    (uint32_t)MAX((int32_t)sequence_control_set_ptr->qp - 40, 0));

                qp_search_max = CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    sequence_control_set_ptr->qp + 40);

                if (!sequence_control_set_ptr->encode_context_ptr->rate_control_tables_array_updated) {
                    context_ptr->intra_coef_rate = CLIP3(
                        1,
                        2,
                        (uint32_t)(rate_control_layer_ptr->frame_rate >> 16) / 4);
                }
                else {
                    if (context_ptr->base_layer_frames_avg_qp > context_ptr->base_layer_intra_frames_avg_qp + 3)
                        context_ptr->intra_coef_rate--;
                    else if (context_ptr->base_layer_frames_avg_qp <= context_ptr->base_layer_intra_frames_avg_qp + 2)
                        context_ptr->intra_coef_rate += 2;
                    else if (context_ptr->base_layer_frames_avg_qp <= context_ptr->base_layer_intra_frames_avg_qp)
                        context_ptr->intra_coef_rate++;

                    context_ptr->intra_coef_rate = CLIP3(
                        1,
                        15,//(uint32_t) (context_ptr->frames_in_interval[0]+1) / 2,
                        context_ptr->intra_coef_rate);
                }

                // Loop over proper QPs and find the Predicted bits for that QP. Find the QP with the closest total predicted rate to target bits for the sliding window.
                for (ref_qp_table_index = qp_search_min; ref_qp_table_index < qp_search_max; ref_qp_table_index++) {
                    ref_qp_index = ref_qp_table_index;

                    picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] = 0;

                    num_of_full_sbs = 0;
                    // Loop over block in the frame and calculated the predicted bits at reg QP
                    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

                        sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];

                        if (sb_params_ptr->is_complete_sb) {
                            num_of_full_sbs++;
                            intra_sad_interval_index = picture_control_set_ptr->parent_pcs_ptr->intra_sad_interval_index[sb_index];
                            picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] += sequence_control_set_ptr->encode_context_ptr->rate_control_tables_array[ref_qp_index].intra_sad_bits_array[picture_control_set_ptr->temporal_layer_index][intra_sad_interval_index];

                        }
                    }

                    // Scale for in complete LCUs
                    //  pred_bits_ref_qp is normalized based on the area because of the LCUs at the tile boundries
                    picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] = picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] * rate_control_layer_ptr->area_in_pixel / (num_of_full_sbs << 12);

                    if (min_la_bit_distance > (uint64_t)ABS((int64_t)rate_control_layer_ptr->ec_bit_constraint*context_ptr->intra_coef_rate - (int64_t)picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index])) {
                        min_la_bit_distance = (uint64_t)ABS((int64_t)rate_control_layer_ptr->ec_bit_constraint*context_ptr->intra_coef_rate - (int64_t)picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index]);

                        selected_ref_qp_table_index = ref_qp_table_index;
                        selected_ref_qp = ref_qp_index;
                    }

                }

                if (!sequence_control_set_ptr->encode_context_ptr->rate_control_tables_array_updated) {
                    picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)selected_ref_qp - (int32_t)1, 0);
                    rate_control_layer_ptr->calculated_frame_qp = (uint8_t)MAX((int32_t)selected_ref_qp - (int32_t)1, 0);
                    picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->picture_qp;
                }
                else {
                    picture_control_set_ptr->picture_qp = (uint8_t)selected_ref_qp;
                    rate_control_layer_ptr->calculated_frame_qp = (uint8_t)selected_ref_qp;
                    picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->picture_qp;
                    picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                        (uint32_t)MAX((int32_t)context_ptr->base_layer_frames_avg_qp - (int32_t)3, 0),
                        context_ptr->base_layer_frames_avg_qp,
                        picture_control_set_ptr->picture_qp);
                    picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                        (uint32_t)MAX((int32_t)context_ptr->base_layer_intra_frames_avg_qp - (int32_t)5, 0),
                        context_ptr->base_layer_intra_frames_avg_qp + 2,
                        picture_control_set_ptr->picture_qp);
                }
            }
            else {
                selected_ref_qp = picture_control_set_ptr->parent_pcs_ptr->best_pred_qp;
                picture_control_set_ptr->picture_qp = (uint8_t)selected_ref_qp;
                picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->picture_qp;
            }

            // Update the QP based on the VB
            if (picture_control_set_ptr->parent_pcs_ptr->end_of_sequence_region) {
                if (rate_control_param_ptr->virtual_buffer_level >= context_ptr->vb_fill_threshold2 << 1) {
                    picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 2;
                }
                else if (rate_control_param_ptr->virtual_buffer_level >= context_ptr->vb_fill_threshold2) {
                    picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE;
                }
                else if (rate_control_param_ptr->virtual_buffer_level >= context_ptr->vb_fill_threshold1 &&
                    rate_control_param_ptr->virtual_buffer_level < context_ptr->vb_fill_threshold2) {
                    picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD1QPINCREASE;
                }
                if (rate_control_param_ptr->virtual_buffer_level <= -(context_ptr->vb_fill_threshold2 << 2))
                    picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE - (int32_t)2, 0);
                else
                    if (rate_control_param_ptr->virtual_buffer_level <= -(context_ptr->vb_fill_threshold2 << 1))
                        picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE - (int32_t)1, 0);
                    else if (rate_control_param_ptr->virtual_buffer_level <= 0)
                        picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE, 0);
            }
            else {

                if (rate_control_param_ptr->virtual_buffer_level >= context_ptr->vb_fill_threshold2) {
                    picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE;
                }
                if (rate_control_param_ptr->virtual_buffer_level <= -(context_ptr->vb_fill_threshold2 << 2))
                    picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp - (uint8_t)THRESHOLD2QPINCREASE - (int32_t)2;
                else if (rate_control_param_ptr->virtual_buffer_level <= -(context_ptr->vb_fill_threshold2 << 1))
                    picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp - (uint8_t)THRESHOLD2QPINCREASE - (int32_t)1;
                else
                    if (rate_control_param_ptr->virtual_buffer_level <= 0)
                        picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE, 0);
            }
            picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                sequence_control_set_ptr->static_config.max_qp_allowed,
                picture_control_set_ptr->picture_qp);
        }
        else {

            // LCU Loop
            for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

                sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];

                if (sb_params_ptr->is_complete_sb) {
                    picture_control_set_ptr->parent_pcs_ptr->sad_me += picture_control_set_ptr->parent_pcs_ptr->rcme_distortion[sb_index];
                }
            }

            //  tileSadMe is normalized based on the area because of the LCUs at the tile boundries
            picture_control_set_ptr->parent_pcs_ptr->sad_me = MAX((picture_control_set_ptr->parent_pcs_ptr->sad_me*rate_control_layer_ptr->area_in_pixel / (area_in_sbs << 12)), 1);

            // totalSquareMad has RC_PRECISION precision
            picture_control_set_ptr->parent_pcs_ptr->sad_me <<= RC_PRECISION;

        }

        temp_qp = picture_control_set_ptr->picture_qp;

        if (picture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc) {
            uint32_t temporal_layer_idex;
            for (temporal_layer_idex = 0; temporal_layer_idex < EB_MAX_TEMPORAL_LAYERS; temporal_layer_idex++) {
                rate_control_layer_temp_ptr = rate_control_param_ptr->rate_control_layer_array[temporal_layer_idex];
                eb_vp9_rate_control_layer_reset_part2(
                    context_ptr,
                    rate_control_layer_temp_ptr,
                    picture_control_set_ptr);
            }
        }

        if (picture_control_set_ptr->picture_number == 0) {
            context_ptr->base_layer_frames_avg_qp = picture_control_set_ptr->picture_qp + 1;
            context_ptr->base_layer_intra_frames_avg_qp = picture_control_set_ptr->picture_qp;
        }
    }
    else {

        picture_control_set_ptr->parent_pcs_ptr->sad_me = 0;

        // if the pixture is an I slice, for now we set the QP as the QP of the previous frame
        if (picture_control_set_ptr->slice_type == I_SLICE) {
            uint32_t         selected_ref_qp;

            if (sequence_control_set_ptr->look_ahead_distance == 0)
            {
                uint32_t         selected_ref_qp_table_index;
                uint32_t         intra_sad_interval_index;
                uint32_t         ref_qp_index;
                uint32_t         ref_qp_table_index;
                uint32_t         qp_search_min;
                uint32_t         qp_search_max;
                uint32_t         num_of_full_sbs;
                uint64_t         min_la_bit_distance;

                min_la_bit_distance = MAX_UNSIGNED_VALUE;
                selected_ref_qp_table_index = 0;
                selected_ref_qp = selected_ref_qp_table_index;
                qp_search_min = (uint8_t)CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    (uint32_t)MAX((int32_t)sequence_control_set_ptr->qp - 40, 0));

                qp_search_max = (uint8_t)CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    sequence_control_set_ptr->qp + 40);

                context_ptr->intra_coef_rate = CLIP3(
                    1,
                    (uint32_t)(rate_control_layer_ptr->frame_rate >> 16) / 4,
                    context_ptr->intra_coef_rate);
                // Loop over proper QPs and find the Predicted bits for that QP. Find the QP with the closest total predicted rate to target bits for the sliding window.
                for (ref_qp_table_index = qp_search_min; ref_qp_table_index < qp_search_max; ref_qp_table_index++) {
                    ref_qp_index = ref_qp_table_index;
                    picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] = 0;
                    num_of_full_sbs = 0;
                    // Loop over block in the frame and calculated the predicted bits at reg QP
                    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

                        sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];

                        if (sb_params_ptr->is_complete_sb) {
                            num_of_full_sbs++;
                            intra_sad_interval_index = picture_control_set_ptr->parent_pcs_ptr->intra_sad_interval_index[sb_index];
                            picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] += sequence_control_set_ptr->encode_context_ptr->rate_control_tables_array[ref_qp_index].intra_sad_bits_array[picture_control_set_ptr->temporal_layer_index][intra_sad_interval_index];
                        }
                    }

                    // Scale for in complete LCUs
                    // pred_bits_ref_qp is normalized based on the area because of the LCUs at the tile boundries
                    picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] = picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] * rate_control_layer_ptr->area_in_pixel / (num_of_full_sbs << 12);
                    if (min_la_bit_distance > (uint64_t)ABS((int64_t)rate_control_layer_ptr->ec_bit_constraint*context_ptr->intra_coef_rate - (int64_t)picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index])) {
                        min_la_bit_distance = (uint64_t)ABS((int64_t)rate_control_layer_ptr->ec_bit_constraint*context_ptr->intra_coef_rate - (int64_t)picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index]);

                        selected_ref_qp_table_index = ref_qp_table_index;
                        selected_ref_qp = ref_qp_index;
                    }
                }
                if (!sequence_control_set_ptr->encode_context_ptr->rate_control_tables_array_updated) {
                    picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)selected_ref_qp - (int32_t)1, 0);
                    rate_control_layer_ptr->calculated_frame_qp = (uint8_t)MAX((int32_t)selected_ref_qp - (int32_t)1, 0);
                    picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->picture_qp;
                }
                else {
                    picture_control_set_ptr->picture_qp = (uint8_t)selected_ref_qp;
                    rate_control_layer_ptr->calculated_frame_qp = (uint8_t)selected_ref_qp;
                    picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->picture_qp;
                    picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                        (uint32_t)MAX((int32_t)context_ptr->base_layer_frames_avg_qp - (int32_t)3, 0),
                        context_ptr->base_layer_frames_avg_qp + 1,
                        picture_control_set_ptr->picture_qp);
                    picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                        (uint32_t)MAX((int32_t)context_ptr->base_layer_intra_frames_avg_qp - (int32_t)5, 0),
                        context_ptr->base_layer_intra_frames_avg_qp + 2,
                        picture_control_set_ptr->picture_qp);
                }
            }
            else {
                selected_ref_qp = picture_control_set_ptr->parent_pcs_ptr->best_pred_qp;
                picture_control_set_ptr->picture_qp = (uint8_t)selected_ref_qp;
                picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->picture_qp;
            }

            picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                sequence_control_set_ptr->static_config.max_qp_allowed,
                picture_control_set_ptr->picture_qp);

            temp_qp = picture_control_set_ptr->picture_qp;

        }

        else { // Not an I slice
            // combining the target rate from initial RC and frame level RC
            if (sequence_control_set_ptr->look_ahead_distance != 0) {
                picture_control_set_ptr->parent_pcs_ptr->target_bits_rc = rate_control_layer_ptr->bit_constraint;
                rate_control_layer_ptr->ec_bit_constraint = (rate_control_layer_ptr->alpha * picture_control_set_ptr->parent_pcs_ptr->target_bits_best_pred_qp +
                    ((1 << RC_PRECISION) - rate_control_layer_ptr->alpha) * picture_control_set_ptr->parent_pcs_ptr->target_bits_rc + RC_PRECISION_OFFSET) >> RC_PRECISION;

                rate_control_layer_ptr->ec_bit_constraint = (uint64_t)MAX((int64_t)rate_control_layer_ptr->ec_bit_constraint - (int64_t)rate_control_layer_ptr->dif_total_and_ec_bits, 1);

                picture_control_set_ptr->parent_pcs_ptr->target_bits_rc = rate_control_layer_ptr->ec_bit_constraint;
            }

            // LCU Loop
            for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

                sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];

                if (sb_params_ptr->is_complete_sb) {
                    picture_control_set_ptr->parent_pcs_ptr->sad_me += picture_control_set_ptr->parent_pcs_ptr->rcme_distortion[sb_index];
                }
            }

            //  tileSadMe is normalized based on the area because of the LCUs at the tile boundries
            picture_control_set_ptr->parent_pcs_ptr->sad_me = MAX((picture_control_set_ptr->parent_pcs_ptr->sad_me*rate_control_layer_ptr->area_in_pixel / (area_in_sbs << 12)), 1);
            picture_control_set_ptr->parent_pcs_ptr->sad_me <<= RC_PRECISION;

            rate_control_layer_ptr->total_mad = MAX((picture_control_set_ptr->parent_pcs_ptr->sad_me / rate_control_layer_ptr->area_in_pixel), 1);

            if (!rate_control_layer_ptr->feedback_arrived) {
                rate_control_layer_ptr->previous_frame_distortion_me = picture_control_set_ptr->parent_pcs_ptr->sad_me;
            }

            {
                uint64_t qp_calc_temp1, qp_calc_temp2, qp_calc_temp3;

                qp_calc_temp1 = picture_control_set_ptr->parent_pcs_ptr->sad_me *rate_control_layer_ptr->total_mad;
                qp_calc_temp2 =
                    MAX((int64_t)(rate_control_layer_ptr->ec_bit_constraint << (2 * RC_PRECISION)) - (int64_t)rate_control_layer_ptr->c_coeff*(int64_t)rate_control_layer_ptr->area_in_pixel,
                    (int64_t)(rate_control_layer_ptr->ec_bit_constraint << (2 * RC_PRECISION - 2)));

                // This is a more complex but with higher precision implementation
                if (qp_calc_temp1 > qp_calc_temp2)
                    qp_calc_temp3 = (uint64_t)((qp_calc_temp1 / qp_calc_temp2)*rate_control_layer_ptr->k_coeff);
                else
                    qp_calc_temp3 = (uint64_t)(qp_calc_temp1*rate_control_layer_ptr->k_coeff / qp_calc_temp2);
                temp_qp = (uint64_t)(eb_vp9_log2f_high_precision(MAX(((qp_calc_temp3 + RC_PRECISION_OFFSET) >> RC_PRECISION)*((qp_calc_temp3 + RC_PRECISION_OFFSET) >> RC_PRECISION)*((qp_calc_temp3 + RC_PRECISION_OFFSET) >> RC_PRECISION), 1), RC_PRECISION));

                rate_control_layer_ptr->calculated_frame_qp = (uint8_t)(CLIP3(1, 63, (uint32_t)(temp_qp + RC_PRECISION_OFFSET) >> RC_PRECISION));
                picture_control_set_ptr->parent_pcs_ptr->calculated_qp = (uint8_t)(CLIP3(1, 63, (uint32_t)(temp_qp + RC_PRECISION_OFFSET) >> RC_PRECISION));
            }

            temp_qp += rate_control_layer_ptr->delta_qp_fraction;
            picture_control_set_ptr->picture_qp = (uint8_t)((temp_qp + RC_PRECISION_OFFSET) >> RC_PRECISION);
            // Use the QP of HLRC instead of calculated one in FLRC
            if (picture_control_set_ptr->parent_pcs_ptr->hierarchical_levels > 1) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->parent_pcs_ptr->best_pred_qp;
                picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->parent_pcs_ptr->best_pred_qp;
            }
        }
        if (picture_control_set_ptr->parent_pcs_ptr->first_non_intra_frame_in_temporal_layer && picture_control_set_ptr->temporal_layer_index == 0 && picture_control_set_ptr->slice_type != I_SLICE) {
            picture_control_set_ptr->picture_qp = (uint8_t)(rate_control_param_ptr->intra_frames_qp + context_ptr->qp_scaling_map[picture_control_set_ptr->temporal_layer_index][rate_control_param_ptr->intra_frames_qp_bef_scal] - context_ptr->qp_scaling_map_I_SLICE[rate_control_param_ptr->intra_frames_qp_bef_scal]);

        }

        if (!rate_control_layer_ptr->feedback_arrived && picture_control_set_ptr->slice_type != I_SLICE) {

            picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                (int32_t)sequence_control_set_ptr->static_config.min_qp_allowed,
                (int32_t)sequence_control_set_ptr->static_config.max_qp_allowed,
                (int32_t)(rate_control_param_ptr->intra_frames_qp + context_ptr->qp_scaling_map[picture_control_set_ptr->temporal_layer_index][rate_control_param_ptr->intra_frames_qp_bef_scal] - context_ptr->qp_scaling_map_I_SLICE[rate_control_param_ptr->intra_frames_qp_bef_scal]));
        }

        if (picture_control_set_ptr->parent_pcs_ptr->end_of_sequence_region) {
            if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2 << 2) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 4;
            }
            else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2 << 1) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 3;
            }
            else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 2;
            }
            else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold1 &&
                rate_control_param_ptr->virtual_buffer_level < context_ptr->vb_fill_threshold2) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD1QPINCREASE + 2;
            }
        }
        else {

            if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2 << 2) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 2;

            }
            else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2 << 1) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 1;

            }
            else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 1;
            }
            else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold1 &&
                rate_control_param_ptr->virtual_buffer_level < context_ptr->vb_fill_threshold2) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD1QPINCREASE;
            }

        }
        if (picture_control_set_ptr->parent_pcs_ptr->end_of_sequence_region) {
            if (rate_control_param_ptr->virtual_buffer_level < -(context_ptr->vb_fill_threshold2 << 2))
                picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE - 2, 0);
            else if (rate_control_param_ptr->virtual_buffer_level < -(context_ptr->vb_fill_threshold2 << 1))
                picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE - 1, 0);
            else if (rate_control_param_ptr->virtual_buffer_level < 0)
                picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE, 0);
        }
        else {

            if (rate_control_param_ptr->virtual_buffer_level < -(context_ptr->vb_fill_threshold2 << 2))
                picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE - 1, 0);
            else if (rate_control_param_ptr->virtual_buffer_level < -context_ptr->vb_fill_threshold2)
                picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE, 0);
        }

        // limiting the QP based on the predicted QP
        if (sequence_control_set_ptr->look_ahead_distance != 0) {
            if (picture_control_set_ptr->parent_pcs_ptr->end_of_sequence_region) {
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)MAX((int32_t)picture_control_set_ptr->parent_pcs_ptr->best_pred_qp - 8, 0),
                    (uint32_t)picture_control_set_ptr->parent_pcs_ptr->best_pred_qp + 8,
                    (uint32_t)picture_control_set_ptr->picture_qp);
            }
            else {
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)MAX((int32_t)picture_control_set_ptr->parent_pcs_ptr->best_pred_qp - 8, 0),
                    (uint32_t)picture_control_set_ptr->parent_pcs_ptr->best_pred_qp + 8,
                    (uint32_t)picture_control_set_ptr->picture_qp);

            }
        }
        if (picture_control_set_ptr->picture_number != rate_control_param_ptr->first_poc &&
            picture_control_set_ptr->picture_qp == picture_control_set_ptr->parent_pcs_ptr->best_pred_qp && rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold1) {
            if (rate_control_param_ptr->extra_ap_bit_ratio_i > 200) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + 3;
            }
            else if (rate_control_param_ptr->extra_ap_bit_ratio_i > 100) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + 2;
            }
            else if (rate_control_param_ptr->extra_ap_bit_ratio_i > 50) {
                picture_control_set_ptr->picture_qp++;
            }
        }
        //Limiting the QP based on the QP of the Reference frame

        uint32_t ref_qp;
        if ((int32_t)picture_control_set_ptr->temporal_layer_index == 0 && picture_control_set_ptr->slice_type != I_SLICE) {
            if (picture_control_set_ptr->ref_slice_type_array[0] == I_SLICE) {
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)picture_control_set_ptr->ref_pic_qp_array[0],
                    (uint32_t)picture_control_set_ptr->picture_qp,
                    picture_control_set_ptr->picture_qp);
            }
            else {
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)MAX((int32_t)picture_control_set_ptr->ref_pic_qp_array[0] - 1, 0),
                    (uint32_t)picture_control_set_ptr->picture_qp,
                    picture_control_set_ptr->picture_qp);
            }
        }
        else {
            ref_qp = 0;
            if (picture_control_set_ptr->ref_slice_type_array[0] != I_SLICE) {
                ref_qp = MAX(ref_qp, picture_control_set_ptr->ref_pic_qp_array[0]);
            }
            if ((picture_control_set_ptr->slice_type == B_SLICE) && (picture_control_set_ptr->ref_slice_type_array[1] != I_SLICE)) {
                ref_qp = MAX(ref_qp, picture_control_set_ptr->ref_pic_qp_array[1]);
            }
            if (ref_qp > 0) {
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)ref_qp - 1,
                    picture_control_set_ptr->picture_qp,
                    picture_control_set_ptr->picture_qp);
            }
        }
        // limiting the QP between min Qp allowed and max Qp allowed
        picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
            sequence_control_set_ptr->static_config.min_qp_allowed,
            sequence_control_set_ptr->static_config.max_qp_allowed,
            picture_control_set_ptr->picture_qp);

        rate_control_layer_ptr->delta_qp_fraction = CLIP3(-RC_PRECISION_OFFSET, RC_PRECISION_OFFSET, -((int64_t)temp_qp - (int64_t)(picture_control_set_ptr->picture_qp << RC_PRECISION)));

        if (picture_control_set_ptr->parent_pcs_ptr->sad_me == rate_control_layer_ptr->previous_frame_distortion_me &&
            (rate_control_layer_ptr->previous_frame_distortion_me != 0))
            rate_control_layer_ptr->same_distortion_count++;
        else
            rate_control_layer_ptr->same_distortion_count = 0;
    }

    rate_control_layer_ptr->previous_c_coeff = rate_control_layer_ptr->c_coeff;
    rate_control_layer_ptr->previous_k_coeff = rate_control_layer_ptr->k_coeff;
    rate_control_layer_ptr->previous_calculated_frame_qp = rate_control_layer_ptr->calculated_frame_qp;
}

void eb_vp9_frame_level_rc_feedback_picture_vbr(
    PictureParentControlSet *parentpicture_control_set_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    RateControlContext      *context_ptr)
{
    RateControlLayerContext         *rate_control_layer_temp_ptr;
    RateControlIntervalParamContext *rate_control_param_ptr;
    RateControlLayerContext         *rate_control_layer_ptr;
    // SB Loop variables
    uint32_t                         slice_num;
    uint64_t                         previous_frame_bit_actual;

    if (sequence_control_set_ptr->intra_period == -1)
        rate_control_param_ptr = context_ptr->rate_control_param_queue[0];
    else {
        uint32_t interval_index_temp = 0;
        while ((!(parentpicture_control_set_ptr->picture_number >= context_ptr->rate_control_param_queue[interval_index_temp]->first_poc &&
            parentpicture_control_set_ptr->picture_number <= context_ptr->rate_control_param_queue[interval_index_temp]->last_poc)) &&
            (interval_index_temp < PARALLEL_GOP_MAX_NUMBER)) {
            interval_index_temp++;
        }
        CHECK_REPORT_ERROR(
            interval_index_temp != PARALLEL_GOP_MAX_NUMBER,
            sequence_control_set_ptr->encode_context_ptr->app_callback_ptr,
            EB_ENC_RC_ERROR0);
        rate_control_param_ptr = context_ptr->rate_control_param_queue[interval_index_temp];
    }

    rate_control_layer_ptr = rate_control_param_ptr->rate_control_layer_array[parentpicture_control_set_ptr->temporal_layer_index];

    rate_control_layer_ptr->max_qp = 0;

    rate_control_layer_ptr->feedback_arrived = EB_TRUE;
    rate_control_layer_ptr->max_qp = MAX(rate_control_layer_ptr->max_qp, parentpicture_control_set_ptr->picture_qp);

    rate_control_layer_ptr->previous_frame_qp = parentpicture_control_set_ptr->picture_qp;
    rate_control_layer_ptr->previous_frame_bit_actual = parentpicture_control_set_ptr->total_num_bits;
    if (parentpicture_control_set_ptr->quantized_coeff_num_bits == 0)
        parentpicture_control_set_ptr->quantized_coeff_num_bits = 1;
    rate_control_layer_ptr->previous_framequantized_coeff_bit_actual = parentpicture_control_set_ptr->quantized_coeff_num_bits;

    // Setting Critical states for adjusting the averaging weights on C and K
    if ((parentpicture_control_set_ptr->sad_me > (3 * rate_control_layer_ptr->previous_frame_distortion_me) >> 1) &&
        (rate_control_layer_ptr->previous_frame_distortion_me != 0)) {
        rate_control_layer_ptr->critical_states = 3;
    }
    else if (rate_control_layer_ptr->critical_states) {
        rate_control_layer_ptr->critical_states--;
    }
    else {
        rate_control_layer_ptr->critical_states = 0;
    }

    if (parentpicture_control_set_ptr->slice_type != I_SLICE) {
        // Updating c_coeff
        rate_control_layer_ptr->c_coeff = (((int64_t)rate_control_layer_ptr->previous_frame_bit_actual - (int64_t)rate_control_layer_ptr->previous_framequantized_coeff_bit_actual) << (2 * RC_PRECISION))
            / rate_control_layer_ptr->area_in_pixel;
        rate_control_layer_ptr->c_coeff = MAX(rate_control_layer_ptr->c_coeff, 1);

        // Updating k_coeff
        if ((parentpicture_control_set_ptr->sad_me + RC_PRECISION_OFFSET) >> RC_PRECISION > 5) {
            {
                uint64_t test1, test2, test3;
                test1 = rate_control_layer_ptr->previous_framequantized_coeff_bit_actual*(two_to_power_qp_over_three[parentpicture_control_set_ptr->picture_qp]);
                test2 = MAX(parentpicture_control_set_ptr->sad_me / rate_control_layer_ptr->area_in_pixel, 1);
                test3 = test1 * 65536 / test2 * 65536 / parentpicture_control_set_ptr->sad_me;

                rate_control_layer_ptr->k_coeff = test3;
            }
        }

        if (rate_control_layer_ptr->critical_states) {
            rate_control_layer_ptr->k_coeff = (8 * rate_control_layer_ptr->k_coeff + 8 * rate_control_layer_ptr->previous_k_coeff + 8) >> 4;
            rate_control_layer_ptr->c_coeff = (8 * rate_control_layer_ptr->c_coeff + 8 * rate_control_layer_ptr->previous_c_coeff + 8) >> 4;
        }
        else {
            rate_control_layer_ptr->k_coeff = (rate_control_layer_ptr->coeff_averaging_weight1*rate_control_layer_ptr->k_coeff + rate_control_layer_ptr->coeff_averaging_weight2*rate_control_layer_ptr->previous_k_coeff + 8) >> 4;
            rate_control_layer_ptr->c_coeff = (rate_control_layer_ptr->coeff_averaging_weight1*rate_control_layer_ptr->c_coeff + rate_control_layer_ptr->coeff_averaging_weight2*rate_control_layer_ptr->previous_c_coeff + 8) >> 4;
        }
        rate_control_layer_ptr->k_coeff = MIN(rate_control_layer_ptr->k_coeff, rate_control_layer_ptr->previous_k_coeff * 4);
        rate_control_layer_ptr->c_coeff = MIN(rate_control_layer_ptr->c_coeff, rate_control_layer_ptr->previous_c_coeff * 4);
        if (parentpicture_control_set_ptr->slice_type != I_SLICE) {
            rate_control_layer_ptr->previous_frame_distortion_me = parentpicture_control_set_ptr->sad_me;
        }
        else {
            rate_control_layer_ptr->previous_frame_distortion_me = 0;
        }
    }

    if (sequence_control_set_ptr->look_ahead_distance != 0) {
        if (parentpicture_control_set_ptr->slice_type == I_SLICE) {
            if (parentpicture_control_set_ptr->total_num_bits < parentpicture_control_set_ptr->target_bits_best_pred_qp << 1)
                context_ptr->base_layer_intra_frames_avg_qp = (3 * context_ptr->base_layer_intra_frames_avg_qp + parentpicture_control_set_ptr->picture_qp + 2) >> 2;
            else if (parentpicture_control_set_ptr->total_num_bits > parentpicture_control_set_ptr->target_bits_best_pred_qp << 2)
                context_ptr->base_layer_intra_frames_avg_qp = (3 * context_ptr->base_layer_intra_frames_avg_qp + parentpicture_control_set_ptr->picture_qp + 4 + 2) >> 2;
            else if (parentpicture_control_set_ptr->total_num_bits > parentpicture_control_set_ptr->target_bits_best_pred_qp << 1)
                context_ptr->base_layer_intra_frames_avg_qp = (3 * context_ptr->base_layer_intra_frames_avg_qp + parentpicture_control_set_ptr->picture_qp + 2 + 2) >> 2;
        }
    }

    {
        uint64_t previous_frame_ec_bits = 0;
        EB_BOOL picture_min_qp_allowed = EB_TRUE;
        rate_control_layer_ptr->previous_frame_average_qp = 0;
        rate_control_layer_ptr->previous_frame_average_qp += rate_control_layer_ptr->previous_frame_qp;
        previous_frame_ec_bits += rate_control_layer_ptr->previous_frame_bit_actual;
        if (rate_control_layer_ptr->same_distortion_count == 0 ||
            parentpicture_control_set_ptr->picture_qp != sequence_control_set_ptr->static_config.min_qp_allowed) {
            picture_min_qp_allowed = EB_FALSE;
        }
        if (picture_min_qp_allowed)
            rate_control_layer_ptr->frame_same_distortion_min_qp_count++;
        else
            rate_control_layer_ptr->frame_same_distortion_min_qp_count = 0;

        rate_control_layer_ptr->previous_ec_bits = previous_frame_ec_bits;
        previous_frame_bit_actual = parentpicture_control_set_ptr->total_num_bits;
        if (parentpicture_control_set_ptr->first_frame_in_temporal_layer) {
            rate_control_layer_ptr->dif_total_and_ec_bits = (previous_frame_bit_actual - previous_frame_ec_bits);
        }
        else {
            rate_control_layer_ptr->dif_total_and_ec_bits = ((previous_frame_bit_actual - previous_frame_ec_bits) + rate_control_layer_ptr->dif_total_and_ec_bits) >> 1;
        }

        // update bitrate of different layers in the interval based on the rate of the I frame
        if (parentpicture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc &&
            (parentpicture_control_set_ptr->slice_type == I_SLICE) &&
            sequence_control_set_ptr->static_config.intra_period != -1) {
            uint32_t temporal_layer_idex;
            uint64_t target_bit_rate;
            uint64_t channel_bit_rate;
            uint64_t sum_bits_per_sw = 0;
#if ADAPTIVE_PERCENTAGE
            if (sequence_control_set_ptr->look_ahead_distance != 0) {
                if (parentpicture_control_set_ptr->tables_updated && parentpicture_control_set_ptr->percentage_updated) {
                    parentpicture_control_set_ptr->bits_per_sw_per_layer[0] =
                        (uint64_t)MAX((int64_t)parentpicture_control_set_ptr->bits_per_sw_per_layer[0] + (int64_t)parentpicture_control_set_ptr->total_num_bits - (int64_t)parentpicture_control_set_ptr->target_bits_best_pred_qp, 1);
                }
            }
#endif

            if (sequence_control_set_ptr->look_ahead_distance != 0 && sequence_control_set_ptr->intra_period != -1) {
                for (temporal_layer_idex = 0; temporal_layer_idex < EB_MAX_TEMPORAL_LAYERS; temporal_layer_idex++) {
                    sum_bits_per_sw += parentpicture_control_set_ptr->bits_per_sw_per_layer[temporal_layer_idex];
                }
            }

            for (temporal_layer_idex = 0; temporal_layer_idex < EB_MAX_TEMPORAL_LAYERS; temporal_layer_idex++) {
                rate_control_layer_temp_ptr = rate_control_param_ptr->rate_control_layer_array[temporal_layer_idex];

                target_bit_rate = (uint64_t)((int64_t)parentpicture_control_set_ptr->target_bit_rate -
                    MIN((int64_t)parentpicture_control_set_ptr->target_bit_rate * 3 / 4, (int64_t)(parentpicture_control_set_ptr->total_num_bits*context_ptr->frame_rate / (sequence_control_set_ptr->static_config.intra_period + 1)) >> RC_PRECISION))
                    *rate_percentage_layer_array[sequence_control_set_ptr->hierarchical_levels][temporal_layer_idex] / 100;

#if ADAPTIVE_PERCENTAGE
                if (sequence_control_set_ptr->look_ahead_distance != 0 && sequence_control_set_ptr->intra_period != -1) {
                    target_bit_rate = (uint64_t)((int64_t)parentpicture_control_set_ptr->target_bit_rate -
                        MIN((int64_t)parentpicture_control_set_ptr->target_bit_rate * 3 / 4, (int64_t)(parentpicture_control_set_ptr->total_num_bits*context_ptr->frame_rate / (sequence_control_set_ptr->static_config.intra_period + 1)) >> RC_PRECISION))
                        *parentpicture_control_set_ptr->bits_per_sw_per_layer[temporal_layer_idex] / sum_bits_per_sw;
                }
#endif
                // update this based on temporal layers
                if (temporal_layer_idex == 0)
                    channel_bit_rate = (((target_bit_rate << (2 * RC_PRECISION)) / MAX(1, rate_control_layer_temp_ptr->frame_rate - (1 * context_ptr->frame_rate / (sequence_control_set_ptr->static_config.intra_period + 1)))) + RC_PRECISION_OFFSET) >> RC_PRECISION;
                else
                    channel_bit_rate = (((target_bit_rate << (2 * RC_PRECISION)) / rate_control_layer_temp_ptr->frame_rate) + RC_PRECISION_OFFSET) >> RC_PRECISION;
                channel_bit_rate = (uint64_t)MAX((int64_t)1, (int64_t)channel_bit_rate);
                rate_control_layer_temp_ptr->ec_bit_constraint = channel_bit_rate;

                slice_num = 1;
                rate_control_layer_temp_ptr->ec_bit_constraint -= SLICE_HEADER_BITS_NUM * slice_num;

                rate_control_layer_temp_ptr->previous_bit_constraint = channel_bit_rate;
                rate_control_layer_temp_ptr->bit_constraint = channel_bit_rate;
                rate_control_layer_temp_ptr->channel_bit_rate = channel_bit_rate;
            }
            if ((int64_t)parentpicture_control_set_ptr->target_bit_rate * 3 / 4 < (int64_t)(parentpicture_control_set_ptr->total_num_bits*context_ptr->frame_rate / (sequence_control_set_ptr->static_config.intra_period + 1)) >> RC_PRECISION) {
                rate_control_param_ptr->previous_virtual_buffer_level += (int64_t)((parentpicture_control_set_ptr->total_num_bits*context_ptr->frame_rate / (sequence_control_set_ptr->static_config.intra_period + 1)) >> RC_PRECISION) - (int64_t)parentpicture_control_set_ptr->target_bit_rate * 3 / 4;
                context_ptr->extra_bits_gen -= (int64_t)((parentpicture_control_set_ptr->total_num_bits*context_ptr->frame_rate / (sequence_control_set_ptr->static_config.intra_period + 1)) >> RC_PRECISION) - (int64_t)parentpicture_control_set_ptr->target_bit_rate * 3 / 4;
            }
        }

        if (previous_frame_bit_actual) {

            uint64_t bit_changes_rate;
            // Updating virtual buffer level and it can be negative
            if ((parentpicture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc) &&
                (parentpicture_control_set_ptr->slice_type == I_SLICE) &&
                (rate_control_param_ptr->last_gop == EB_FALSE) &&
                sequence_control_set_ptr->static_config.intra_period != -1) {
                rate_control_param_ptr->virtual_buffer_level =
                    (int64_t)rate_control_param_ptr->previous_virtual_buffer_level;
            }
            else {
                rate_control_param_ptr->virtual_buffer_level =
                    (int64_t)rate_control_param_ptr->previous_virtual_buffer_level +
                    (int64_t)previous_frame_bit_actual - (int64_t)rate_control_layer_ptr->channel_bit_rate;
#if !VP9_RC
                context_ptr->extra_bits_gen -= (int64_t)previous_frame_bit_actual - (int64_t)rate_control_layer_ptr->channel_bit_rate;
#endif
            }
#if VP9_RC
            context_ptr->extra_bits_gen -= (int64_t)previous_frame_bit_actual - (int64_t)context_ptr->high_level_rate_control_ptr->channel_bit_rate_per_frame;
#endif
            if (parentpicture_control_set_ptr->hierarchical_levels > 1 && rate_control_layer_ptr->frame_same_distortion_min_qp_count > 10) {
                rate_control_layer_ptr->previous_bit_constraint = (int64_t)rate_control_layer_ptr->channel_bit_rate;
                rate_control_param_ptr->virtual_buffer_level = ((int64_t)context_ptr->virtual_buffer_size >> 1);
            }
            // Updating bit difference
            rate_control_layer_ptr->bit_diff = (int64_t)rate_control_param_ptr->virtual_buffer_level
                //- ((int64_t)context_ptr->virtual_buffer_size>>1);
                - ((int64_t)rate_control_layer_ptr->channel_bit_rate >> 1);

            // Limit the bit difference
            rate_control_layer_ptr->bit_diff = CLIP3(-(int64_t)(rate_control_layer_ptr->channel_bit_rate), (int64_t)(rate_control_layer_ptr->channel_bit_rate >> 1), rate_control_layer_ptr->bit_diff);
            bit_changes_rate = rate_control_layer_ptr->frame_rate;

            // Updating bit Constraint
            rate_control_layer_ptr->bit_constraint = MAX((int64_t)rate_control_layer_ptr->previous_bit_constraint - ((rate_control_layer_ptr->bit_diff << RC_PRECISION) / ((int64_t)bit_changes_rate)), 1);

            // Limiting the bit_constraint
            if (parentpicture_control_set_ptr->temporal_layer_index == 0) {
                rate_control_layer_ptr->bit_constraint = CLIP3(rate_control_layer_ptr->channel_bit_rate >> 2,
                    rate_control_layer_ptr->channel_bit_rate * 200 / 100,
                    rate_control_layer_ptr->bit_constraint);
            }
            else {
                rate_control_layer_ptr->bit_constraint = CLIP3(rate_control_layer_ptr->channel_bit_rate >> 1,
                    rate_control_layer_ptr->channel_bit_rate * 200 / 100,
                    rate_control_layer_ptr->bit_constraint);
            }
            rate_control_layer_ptr->ec_bit_constraint = (uint64_t)MAX((int64_t)rate_control_layer_ptr->bit_constraint - (int64_t)rate_control_layer_ptr->dif_total_and_ec_bits, 1);
            rate_control_param_ptr->previous_virtual_buffer_level = rate_control_param_ptr->virtual_buffer_level;
            rate_control_layer_ptr->previous_bit_constraint = rate_control_layer_ptr->bit_constraint;
        }

        rate_control_param_ptr->processed_frames_number++;
        rate_control_param_ptr->in_use = EB_TRUE;
        // check if all the frames in the interval have arrived
        if (rate_control_param_ptr->processed_frames_number == (rate_control_param_ptr->last_poc - rate_control_param_ptr->first_poc + 1) &&
            sequence_control_set_ptr->intra_period != -1) {

            uint32_t temporal_index;
            int64_t extra_bits;
            rate_control_param_ptr->first_poc += PARALLEL_GOP_MAX_NUMBER * (uint32_t)(sequence_control_set_ptr->intra_period + 1);
            rate_control_param_ptr->last_poc += PARALLEL_GOP_MAX_NUMBER * (uint32_t)(sequence_control_set_ptr->intra_period + 1);
            rate_control_param_ptr->processed_frames_number = 0;
            rate_control_param_ptr->extra_ap_bit_ratio_i = 0;
            rate_control_param_ptr->in_use = EB_FALSE;
            rate_control_param_ptr->was_used = EB_TRUE;
            rate_control_param_ptr->last_gop = EB_FALSE;
            rate_control_param_ptr->first_pic_actual_qp_assigned = EB_FALSE;
            for (temporal_index = 0; temporal_index < EB_MAX_TEMPORAL_LAYERS; temporal_index++) {
                rate_control_param_ptr->rate_control_layer_array[temporal_index]->first_frame = 1;
                rate_control_param_ptr->rate_control_layer_array[temporal_index]->first_non_intra_frame = 1;
                rate_control_param_ptr->rate_control_layer_array[temporal_index]->feedback_arrived = EB_FALSE;
            }
            extra_bits = ((int64_t)context_ptr->virtual_buffer_size >> 1) - (int64_t)rate_control_param_ptr->virtual_buffer_level;

            rate_control_param_ptr->virtual_buffer_level = context_ptr->virtual_buffer_size >> 1;
            context_ptr->extra_bits += extra_bits;

        }
        // Allocate the extra_bits among other GOPs
        if ((parentpicture_control_set_ptr->temporal_layer_index <= 2) &&
            ((context_ptr->extra_bits > (int64_t)(context_ptr->virtual_buffer_size >> 8)) ||
            (context_ptr->extra_bits < -(int64_t)(context_ptr->virtual_buffer_size >> 8)))) {
            uint32_t interval_index_temp, interval_in_use_count;
            int64_t extra_bits_per_gop;
            int64_t extra_bits = context_ptr->extra_bits;
            int32_t clip_coef1, clip_coef2;
            if (parentpicture_control_set_ptr->end_of_sequence_region) {
                clip_coef1 = -1;
                clip_coef2 = -1;
            }
            else {
                if (context_ptr->extra_bits > (int64_t)(context_ptr->virtual_buffer_size << 3) ||
                    context_ptr->extra_bits < -(int64_t)(context_ptr->virtual_buffer_size << 3)) {
                    clip_coef1 = 0;
                    clip_coef2 = 0;
                }
                else {
                    clip_coef1 = 2;
                    clip_coef2 = 4;
                }
            }

            interval_in_use_count = 0;

            if (extra_bits > 0) {
                // Extra bits to be distributed
                // Distribute it among those that are consuming more
                for (interval_index_temp = 0; interval_index_temp < PARALLEL_GOP_MAX_NUMBER; interval_index_temp++) {
                    if (context_ptr->rate_control_param_queue[interval_index_temp]->in_use &&
                        context_ptr->rate_control_param_queue[interval_index_temp]->virtual_buffer_level > ((int64_t)context_ptr->virtual_buffer_size >> 1)) {
                        interval_in_use_count++;
                    }
                }
                // Distribute the rate among them
                if (interval_in_use_count) {
                    extra_bits_per_gop = extra_bits / interval_in_use_count;
                    if (clip_coef1 > 0)
                        extra_bits_per_gop = CLIP3(
                            -(int64_t)context_ptr->virtual_buffer_size >> clip_coef1,
                            (int64_t)context_ptr->virtual_buffer_size >> clip_coef1,
                            extra_bits_per_gop);
                    else
                        extra_bits_per_gop = CLIP3(
                            -(int64_t)context_ptr->virtual_buffer_size << (-clip_coef1),
                            (int64_t)context_ptr->virtual_buffer_size << (-clip_coef1),
                            extra_bits_per_gop);

                    for (interval_index_temp = 0; interval_index_temp < PARALLEL_GOP_MAX_NUMBER; interval_index_temp++) {
                        if (context_ptr->rate_control_param_queue[interval_index_temp]->in_use &&
                            context_ptr->rate_control_param_queue[interval_index_temp]->virtual_buffer_level > ((int64_t)context_ptr->virtual_buffer_size >> 1)) {
                            context_ptr->rate_control_param_queue[interval_index_temp]->virtual_buffer_level -= extra_bits_per_gop;
                            context_ptr->rate_control_param_queue[interval_index_temp]->previous_virtual_buffer_level -= extra_bits_per_gop;
                            context_ptr->extra_bits -= extra_bits_per_gop;
                        }
                    }
                }
                // if no interval with more consuming was found, allocate it to ones with consuming less
                else {
                    interval_in_use_count = 0;
                    // Distribute it among those that are consuming less
                    for (interval_index_temp = 0; interval_index_temp < PARALLEL_GOP_MAX_NUMBER; interval_index_temp++) {

                        if (context_ptr->rate_control_param_queue[interval_index_temp]->in_use &&
                            context_ptr->rate_control_param_queue[interval_index_temp]->virtual_buffer_level <= ((int64_t)context_ptr->virtual_buffer_size >> 1)) {
                            interval_in_use_count++;
                        }
                    }
                    if (interval_in_use_count) {
                        extra_bits_per_gop = extra_bits / interval_in_use_count;
                        if (clip_coef2 > 0)
                            extra_bits_per_gop = CLIP3(
                                -(int64_t)context_ptr->virtual_buffer_size >> clip_coef2,
                                (int64_t)context_ptr->virtual_buffer_size >> clip_coef2,
                                extra_bits_per_gop);
                        else
                            extra_bits_per_gop = CLIP3(
                                -(int64_t)context_ptr->virtual_buffer_size << (-clip_coef2),
                                (int64_t)context_ptr->virtual_buffer_size << (-clip_coef2),
                                extra_bits_per_gop);
                        // Distribute the rate among them
                        for (interval_index_temp = 0; interval_index_temp < PARALLEL_GOP_MAX_NUMBER; interval_index_temp++) {

                            if (context_ptr->rate_control_param_queue[interval_index_temp]->in_use &&
                                context_ptr->rate_control_param_queue[interval_index_temp]->virtual_buffer_level <= ((int64_t)context_ptr->virtual_buffer_size >> 1)) {
                                context_ptr->rate_control_param_queue[interval_index_temp]->virtual_buffer_level -= extra_bits_per_gop;
                                context_ptr->rate_control_param_queue[interval_index_temp]->previous_virtual_buffer_level -= extra_bits_per_gop;
                                context_ptr->extra_bits -= extra_bits_per_gop;
                            }
                        }
                    }
                }
            }
            else {
                // Distribute it among those that are consuming less
                for (interval_index_temp = 0; interval_index_temp < PARALLEL_GOP_MAX_NUMBER; interval_index_temp++) {

                    if (context_ptr->rate_control_param_queue[interval_index_temp]->in_use &&
                        context_ptr->rate_control_param_queue[interval_index_temp]->virtual_buffer_level < ((int64_t)context_ptr->virtual_buffer_size >> 1)) {
                        interval_in_use_count++;
                    }
                }
                if (interval_in_use_count) {
                    extra_bits_per_gop = extra_bits / interval_in_use_count;
                    if (clip_coef1 > 0)
                        extra_bits_per_gop = CLIP3(
                            -(int64_t)context_ptr->virtual_buffer_size >> clip_coef1,
                            (int64_t)context_ptr->virtual_buffer_size >> clip_coef1,
                            extra_bits_per_gop);
                    else
                        extra_bits_per_gop = CLIP3(
                            -(int64_t)context_ptr->virtual_buffer_size << (-clip_coef1),
                            (int64_t)context_ptr->virtual_buffer_size << (-clip_coef1),
                            extra_bits_per_gop);
                    // Distribute the rate among them
                    for (interval_index_temp = 0; interval_index_temp < PARALLEL_GOP_MAX_NUMBER; interval_index_temp++) {
                        if (context_ptr->rate_control_param_queue[interval_index_temp]->in_use &&
                            context_ptr->rate_control_param_queue[interval_index_temp]->virtual_buffer_level < ((int64_t)context_ptr->virtual_buffer_size >> 1)) {
                            context_ptr->rate_control_param_queue[interval_index_temp]->virtual_buffer_level -= extra_bits_per_gop;
                            context_ptr->rate_control_param_queue[interval_index_temp]->previous_virtual_buffer_level -= extra_bits_per_gop;
                            context_ptr->extra_bits -= extra_bits_per_gop;
                        }
                    }
                }
                // if no interval with less consuming was found, allocate it to ones with consuming more
                else {
                    interval_in_use_count = 0;
                    for (interval_index_temp = 0; interval_index_temp < PARALLEL_GOP_MAX_NUMBER; interval_index_temp++) {

                        if (context_ptr->rate_control_param_queue[interval_index_temp]->in_use &&
                            context_ptr->rate_control_param_queue[interval_index_temp]->virtual_buffer_level < (int64_t)(context_ptr->virtual_buffer_size)) {
                            interval_in_use_count++;
                        }
                    }
                    if (interval_in_use_count) {
                        extra_bits_per_gop = extra_bits / interval_in_use_count;
                        if (clip_coef2 > 0)
                            extra_bits_per_gop = CLIP3(
                                -(int64_t)context_ptr->virtual_buffer_size >> clip_coef2,
                                (int64_t)context_ptr->virtual_buffer_size >> clip_coef2,
                                extra_bits_per_gop);
                        else
                            extra_bits_per_gop = CLIP3(
                                -(int64_t)context_ptr->virtual_buffer_size << (-clip_coef2),
                                (int64_t)context_ptr->virtual_buffer_size << (-clip_coef2),
                                extra_bits_per_gop);
                        // Distribute the rate among them
                        for (interval_index_temp = 0; interval_index_temp < PARALLEL_GOP_MAX_NUMBER; interval_index_temp++) {

                            if (context_ptr->rate_control_param_queue[interval_index_temp]->in_use &&
                                context_ptr->rate_control_param_queue[interval_index_temp]->virtual_buffer_level < (int64_t)(context_ptr->virtual_buffer_size)) {
                                context_ptr->rate_control_param_queue[interval_index_temp]->virtual_buffer_level -= extra_bits_per_gop;
                                context_ptr->rate_control_param_queue[interval_index_temp]->previous_virtual_buffer_level -= extra_bits_per_gop;
                                context_ptr->extra_bits -= extra_bits_per_gop;
                            }
                        }
                    }
                }
            }
        }
    }

#if 0//VP9_RC_PRINTS
    ////if (parentpicture_control_set_ptr->temporal_layer_index == 0)
    {
        SVT_LOG("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.0f\t%.0f\t%.0f\t%.0f\t%d\t%d\n",
            (int)parentpicture_control_set_ptr->slice_type,
            (int)parentpicture_control_set_ptr->picture_number,
            (int)parentpicture_control_set_ptr->temporal_layer_index,
            (int)parentpicture_control_set_ptr->picture_qp, (int)parentpicture_control_set_ptr->calculated_qp, (int)parentpicture_control_set_ptr->best_pred_qp,
            (int)previous_frame_bit_actual,
            (int)parentpicture_control_set_ptr->target_bits_best_pred_qp,
            (int)parentpicture_control_set_ptr->target_bits_rc,
            (int)rate_control_layer_ptr->channel_bit_rate,
            (int)rate_control_layer_ptr->bit_constraint,
            (double)rate_control_layer_ptr->c_coeff,
            (double)rate_control_layer_ptr->k_coeff,
            (double)parentpicture_control_set_ptr->sad_me,
#if 1 //RC_IMPROVEMENT
            (double)context_ptr->extra_bits_gen,
#else
            (double)rate_control_layer_ptr->previous_frame_distortion_me,
#endif
            (int)rate_control_param_ptr->virtual_buffer_level,
            (int)context_ptr->extra_bits);
    }
#endif

}
void high_level_rc_input_picture_cbr(
    PictureParentControlSet     *picture_control_set_ptr,
    SequenceControlSet          *sequence_control_set_ptr,
    EncodeContext               *encode_context_ptr,
    RateControlContext          *context_ptr,
    HighLevelRateControlContext *high_level_rate_control_ptr)
{

    EB_BOOL                      end_of_sequence_flag = EB_TRUE;

    HlRateControlHistogramEntry *hl_rate_control_histogram_ptr_temp;
    // Queue variables
    uint32_t                     queue_entry_index_temp;
    uint32_t                     queue_entry_index_temp2;
    uint32_t                     queue_entry_index_head_temp;

    uint64_t                     min_la_bit_distance;
    uint32_t                     selected_ref_qp_table_index;
    uint32_t                     selected_ref_qp;
#if RC_UPDATE_TARGET_RATE
    uint32_t                     selected_org_ref_qp;
#endif
    uint32_t                     previous_selected_ref_qp = encode_context_ptr->previous_selected_ref_qp;
    uint64_t                     max_coded_poc = encode_context_ptr->max_coded_poc;
    uint32_t                     max_coded_poc_selected_ref_qp = encode_context_ptr->max_coded_poc_selected_ref_qp;

    uint32_t                     ref_qp_index;
    uint32_t                     ref_qp_index_temp;
    uint32_t                     ref_qp_table_index;

    uint32_t                     area_in_pixel;
    uint32_t                     num_of_full_sbs;
    uint32_t                     qp_search_min;
    uint32_t                     qp_search_max;
    int32_t                      qp_step = 1;
    EB_BOOL                      best_qp_found;
    uint32_t                     temporal_layer_index;
    EB_BOOL                      tables_updated;

    uint64_t                     bit_constraint_per_sw= 0;

    RateControlTables           *rate_control_tables_ptr;
    EbBitNumber                 *sad_bits_array_ptr;
    EbBitNumber                 *intra_sad_bits_array_ptr;
    uint32_t                     pred_bits_ref_qp;
    int delta_qp = 0;

    for (temporal_layer_index = 0; temporal_layer_index< EB_MAX_TEMPORAL_LAYERS; temporal_layer_index++){
        picture_control_set_ptr->bits_per_sw_per_layer[temporal_layer_index] = 0;
    }
    picture_control_set_ptr->sb_total_bits_per_gop = 0;

    area_in_pixel = sequence_control_set_ptr->luma_width * sequence_control_set_ptr->luma_height;;

    eb_vp9_block_on_mutex(sequence_control_set_ptr->encode_context_ptr->rate_table_update_mutex);

    tables_updated = sequence_control_set_ptr->encode_context_ptr->rate_control_tables_array_updated;
    picture_control_set_ptr->percentage_updated = EB_FALSE;

    if (sequence_control_set_ptr->look_ahead_distance != 0){

        // Increamenting the head of the hl_rate_control_historgram_queue and clean up the entores
        hl_rate_control_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]);

        while ((hl_rate_control_histogram_ptr_temp->life_count == 0) && hl_rate_control_histogram_ptr_temp->passed_to_hlrc){
            eb_vp9_block_on_mutex(sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue_mutex);
            // Reset the Reorder Queue Entry
            hl_rate_control_histogram_ptr_temp->picture_number += INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH;
            hl_rate_control_histogram_ptr_temp->life_count = -1;
            hl_rate_control_histogram_ptr_temp->passed_to_hlrc = EB_FALSE;
            hl_rate_control_histogram_ptr_temp->is_coded = EB_FALSE;
            hl_rate_control_histogram_ptr_temp->total_num_bitsCoded = 0;

            // Increment the Reorder Queue head Ptr
            encode_context_ptr->hl_rate_control_historgram_queue_head_index =
                (encode_context_ptr->hl_rate_control_historgram_queue_head_index == HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? 0 : encode_context_ptr->hl_rate_control_historgram_queue_head_index + 1;
            eb_vp9_release_mutex(sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue_mutex);
            hl_rate_control_histogram_ptr_temp = encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index];

        }
        // For the case that number of frames in the sliding window is less than size of the look ahead or intra Refresh. i.e. end of sequence
        if ((picture_control_set_ptr->frames_in_sw <  MIN(sequence_control_set_ptr->look_ahead_distance + 1, (uint32_t)sequence_control_set_ptr->intra_period + 1))) {

            selected_ref_qp = max_coded_poc_selected_ref_qp;

            // Update the QP for the sliding window based on the status of RC
            if ((context_ptr->extra_bits_gen >(int64_t)(context_ptr->virtual_buffer_size << 3))){
                selected_ref_qp = (uint32_t)MAX((int32_t)selected_ref_qp -2, 0);
            }
            else if ((context_ptr->extra_bits_gen >(int64_t)(context_ptr->virtual_buffer_size << 2))){
                selected_ref_qp = (uint32_t)MAX((int32_t)selected_ref_qp - 1, 0);
            }
            if ((context_ptr->extra_bits_gen < -(int64_t)(context_ptr->virtual_buffer_size << 2))){
                selected_ref_qp += 2;
            }
            else if ((context_ptr->extra_bits_gen < -(int64_t)(context_ptr->virtual_buffer_size << 1))){
                selected_ref_qp += 1;
            }

            if ((picture_control_set_ptr->frames_in_sw < (uint32_t)(sequence_control_set_ptr->intra_period + 1)) &&
                (picture_control_set_ptr->picture_number % ((sequence_control_set_ptr->intra_period + 1)) == 0)){
                selected_ref_qp++;
            }

            selected_ref_qp = (uint32_t)CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                sequence_control_set_ptr->static_config.max_qp_allowed,
                selected_ref_qp);

            queue_entry_index_head_temp = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
            queue_entry_index_head_temp += encode_context_ptr->hl_rate_control_historgram_queue_head_index;
            queue_entry_index_head_temp = (queue_entry_index_head_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                queue_entry_index_head_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                queue_entry_index_head_temp;

            queue_entry_index_temp = queue_entry_index_head_temp;
            {

                hl_rate_control_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_temp]);

                if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE)
                    ref_qp_index_temp = context_ptr->qp_scaling_map_I_SLICE[selected_ref_qp];
                else
                    ref_qp_index_temp = context_ptr->qp_scaling_map[hl_rate_control_histogram_ptr_temp->temporal_layer_index][selected_ref_qp];

                ref_qp_index_temp = (uint32_t)CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    ref_qp_index_temp);

                hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = 0;
                rate_control_tables_ptr = &encode_context_ptr->rate_control_tables_array[ref_qp_index_temp];
                sad_bits_array_ptr = rate_control_tables_ptr->sad_bits_array[hl_rate_control_histogram_ptr_temp->temporal_layer_index];
                intra_sad_bits_array_ptr = rate_control_tables_ptr->intra_sad_bits_array[hl_rate_control_histogram_ptr_temp->temporal_layer_index];
                pred_bits_ref_qp = 0;
                num_of_full_sbs = 0;

                if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE){
                    // Loop over block in the frame and calculated the predicted bits at reg QP
                    {
                        unsigned i;
                        uint32_t accum = 0;
                        for (i = 0; i < NUMBER_OF_INTRA_SAD_INTERVALS; ++i)
                        {
                            accum += (uint32_t)(hl_rate_control_histogram_ptr_temp->ois_distortion_histogram[i] * intra_sad_bits_array_ptr[i]);
                        }

                        pred_bits_ref_qp = accum;
                        num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;
                    }
                    hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                }

                else{
                    {
                        unsigned i;
                        uint32_t accum = 0;
                        for (i = 0; i < NUMBER_OF_SAD_INTERVALS; ++i)
                        {
                            accum += (uint32_t)(hl_rate_control_histogram_ptr_temp->me_distortion_histogram[i] * sad_bits_array_ptr[i]);
                        }

                        pred_bits_ref_qp = accum;
                        num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;

                    }
                    hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                }

                // Scale for in complete
                //  pred_bits_ref_qp is normalized based on the area because of the LCUs at the picture boundries
                hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] * (uint64_t)area_in_pixel / (num_of_full_sbs << 12);

                // Store the pred_bits_ref_qp for the first frame in the window to PCS
                picture_control_set_ptr->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];

            }
        }
        else{
            // Loop over the QPs and find the best QP
            min_la_bit_distance = MAX_UNSIGNED_VALUE;
            qp_search_min = (uint8_t)CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                MAX_REF_QP_NUM,//sequence_control_set_ptr->static_config.max_qp_allowed,
                (uint32_t)MAX((int32_t)sequence_control_set_ptr->qp - 40, 0));

            qp_search_max = (uint8_t)CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                MAX_REF_QP_NUM,//sequence_control_set_ptr->static_config.max_qp_allowed,
                sequence_control_set_ptr->qp + 40);

            for (ref_qp_table_index = qp_search_min; ref_qp_table_index < qp_search_max; ref_qp_table_index++){
                high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_table_index] = 0;
            }

            bit_constraint_per_sw = high_level_rate_control_ptr->bit_constraint_per_sw * picture_control_set_ptr->frames_in_sw / (sequence_control_set_ptr->look_ahead_distance + 1);

            // Update the target rate for the sliding window based on the status of RC
            if ((context_ptr->extra_bits_gen >(int64_t)(context_ptr->virtual_buffer_size * 10))){
                bit_constraint_per_sw = bit_constraint_per_sw * 130 / 100;
            }
            else if((context_ptr->extra_bits_gen >(int64_t)(context_ptr->virtual_buffer_size << 3))){
                bit_constraint_per_sw = bit_constraint_per_sw * 120 / 100;
            }
            else if ((context_ptr->extra_bits_gen >(int64_t)(context_ptr->virtual_buffer_size << 2))){
                bit_constraint_per_sw = bit_constraint_per_sw * 110 / 100;
            }
            if ((context_ptr->extra_bits_gen < -(int64_t)(context_ptr->virtual_buffer_size << 3))){
                bit_constraint_per_sw = bit_constraint_per_sw * 80 / 100;
            }
            else if ((context_ptr->extra_bits_gen < -(int64_t)(context_ptr->virtual_buffer_size << 2))){
                bit_constraint_per_sw = bit_constraint_per_sw * 90 / 100;
            }

            // Loop over proper QPs and find the Predicted bits for that QP. Find the QP with the closest total predicted rate to target bits for the sliding window.
            previous_selected_ref_qp = CLIP3(
                qp_search_min + 1,
                qp_search_max - 1,
                previous_selected_ref_qp);
            ref_qp_table_index  = previous_selected_ref_qp;
            ref_qp_index = ref_qp_table_index;
            selected_ref_qp_table_index = ref_qp_table_index;
            selected_ref_qp = selected_ref_qp_table_index;
            if (sequence_control_set_ptr->intra_period != -1 && picture_control_set_ptr->picture_number % ((sequence_control_set_ptr->intra_period + 1)) == 0 &&
                (int32_t)picture_control_set_ptr->frames_in_sw > sequence_control_set_ptr->intra_period) {
                best_qp_found = EB_FALSE;
                while (ref_qp_table_index >= qp_search_min && ref_qp_table_index <= qp_search_max && !best_qp_found) {

                    ref_qp_index = CLIP3(
                        sequence_control_set_ptr->static_config.min_qp_allowed,
                        MAX_REF_QP_NUM,//sequence_control_set_ptr->static_config.max_qp_allowed,
                        ref_qp_table_index);
                    high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] = 0;

                    // Finding the predicted bits for each frame in the sliding window at the reference Qp(s)
                    queue_entry_index_head_temp = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
                    queue_entry_index_head_temp += encode_context_ptr->hl_rate_control_historgram_queue_head_index;
                    queue_entry_index_head_temp = (queue_entry_index_head_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                        queue_entry_index_head_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                        queue_entry_index_head_temp;

                    queue_entry_index_temp = queue_entry_index_head_temp;
                    // This is set to false, so the last frame would go inside the loop
                    end_of_sequence_flag = EB_FALSE;

                    while (!end_of_sequence_flag &&
                        queue_entry_index_temp <= queue_entry_index_head_temp + sequence_control_set_ptr->look_ahead_distance) {

                        queue_entry_index_temp2 = (queue_entry_index_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH : queue_entry_index_temp;
                        hl_rate_control_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_temp2]);

                        if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE)
                            ref_qp_index_temp = context_ptr->qp_scaling_map_I_SLICE[ref_qp_index];
                        else
                            ref_qp_index_temp = context_ptr->qp_scaling_map[hl_rate_control_histogram_ptr_temp->temporal_layer_index][ref_qp_index];

                        ref_qp_index_temp = (uint32_t)CLIP3(
                            sequence_control_set_ptr->static_config.min_qp_allowed,
                            sequence_control_set_ptr->static_config.max_qp_allowed,
                            ref_qp_index_temp);

                        hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = 0;

                        if (ref_qp_table_index == previous_selected_ref_qp) {
                            hl_rate_control_histogram_ptr_temp->life_count--;
                        }
                        if (hl_rate_control_histogram_ptr_temp->is_coded) {
                            // If the frame is already coded, use the actual number of bits
                            hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->total_num_bitsCoded;
                        }
                        else {
                            rate_control_tables_ptr = &encode_context_ptr->rate_control_tables_array[ref_qp_index_temp];
                            sad_bits_array_ptr = rate_control_tables_ptr->sad_bits_array[hl_rate_control_histogram_ptr_temp->temporal_layer_index];
                            intra_sad_bits_array_ptr = rate_control_tables_ptr->intra_sad_bits_array[0];
                            pred_bits_ref_qp = 0;
                            num_of_full_sbs = 0;

                            if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE) {
                                // Loop over block in the frame and calculated the predicted bits at reg QP
                                unsigned i;
                                uint32_t accum = 0;
                                for (i = 0; i < NUMBER_OF_INTRA_SAD_INTERVALS; ++i)
                                {
                                    accum += (uint32_t)((uint32_t)hl_rate_control_histogram_ptr_temp->ois_distortion_histogram[i] * (uint32_t)intra_sad_bits_array_ptr[i]);
                                }

                                pred_bits_ref_qp = accum;
                                num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;
                                hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                            }
                            else {
                                unsigned i;
                                uint32_t accum = 0;
                                uint32_t accum_intra = 0;
                                for (i = 0; i < NUMBER_OF_SAD_INTERVALS; ++i)
                                {
                                    accum += (uint32_t)((uint32_t)hl_rate_control_histogram_ptr_temp->me_distortion_histogram[i] * (uint32_t)sad_bits_array_ptr[i]);
                                    accum_intra += (uint32_t)((uint32_t)hl_rate_control_histogram_ptr_temp->ois_distortion_histogram[i] * (uint32_t)intra_sad_bits_array_ptr[i]);

                                }
                                if (accum > accum_intra * 3)
                                    pred_bits_ref_qp = accum_intra;
                                else
                                    pred_bits_ref_qp = accum;
                                num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;
                                hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                            }

                            // Scale for in complete LCSs
                            //  pred_bits_ref_qp is normalized based on the area because of the LCUs at the picture boundries
                            hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] * (uint64_t)area_in_pixel / (num_of_full_sbs << 12);

                        }
                        high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] += hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];
                        //if (picture_control_set_ptr->slice_type == I_SLICE) {
                        //    //if (picture_control_set_ptr->picture_number > 280 && picture_control_set_ptr->picture_number < 350) {
                        //    SVT_LOG("%d\t%d\t%d\t%d\t%d\n", picture_control_set_ptr->picture_number, ref_qp_index_temp, ref_qp_index, hl_rate_control_histogram_ptr_temp->picture_number, hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp]);
                        //}
                        // Store the pred_bits_ref_qp for the first frame in the window to PCS
                        if (queue_entry_index_head_temp == queue_entry_index_temp2)
                            picture_control_set_ptr->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];

                        end_of_sequence_flag = hl_rate_control_histogram_ptr_temp->end_of_sequence_flag;
                        queue_entry_index_temp++;
                    }

                    if (min_la_bit_distance >= (uint64_t)ABS((int64_t)high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] - (int64_t)bit_constraint_per_sw)) {
                        min_la_bit_distance = (uint64_t)ABS((int64_t)high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] - (int64_t)bit_constraint_per_sw);
                        selected_ref_qp_table_index = ref_qp_table_index;
                        selected_ref_qp = ref_qp_index;
                    }
                    else {
                        best_qp_found = EB_TRUE;
                    }

                    if (ref_qp_table_index == previous_selected_ref_qp) {
                        if (high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] > bit_constraint_per_sw) {
                            qp_step = +1;
                        }
                        else {
                            qp_step = -1;
                        }
                    }
                    ref_qp_table_index = (uint32_t)(ref_qp_table_index + qp_step);

                }

                if (ref_qp_index == sequence_control_set_ptr->static_config.max_qp_allowed && high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] > bit_constraint_per_sw) {
                    delta_qp = (int)((high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] - bit_constraint_per_sw) * 100 / (high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index - 1] - high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index]));
                    delta_qp = (delta_qp + 50) / 100;
                }
            }
        }
#if RC_UPDATE_TARGET_RATE
        selected_org_ref_qp = selected_ref_qp;
        if (sequence_control_set_ptr->intra_period != -1 && picture_control_set_ptr->picture_number % ((sequence_control_set_ptr->intra_period + 1)) == 0 &&
            (int32_t)picture_control_set_ptr->frames_in_sw >  sequence_control_set_ptr->intra_period){
            if (picture_control_set_ptr->picture_number > 0){
                picture_control_set_ptr->intra_selected_org_qp = (uint8_t)selected_ref_qp;
            }
            ref_qp_index = selected_ref_qp;
            high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] = 0;

            if (high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] == 0){

                // Finding the predicted bits for each frame in the sliding window at the reference Qp(s)
                //queue_entry_index_temp = encode_context_ptr->hl_rate_control_historgram_queue_head_index;
                queue_entry_index_head_temp = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
                queue_entry_index_head_temp += encode_context_ptr->hl_rate_control_historgram_queue_head_index;
                queue_entry_index_head_temp = (queue_entry_index_head_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                    queue_entry_index_head_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                    queue_entry_index_head_temp;

                queue_entry_index_temp = queue_entry_index_head_temp;

                // This is set to false, so the last frame would go inside the loop
                end_of_sequence_flag = EB_FALSE;

                while (!end_of_sequence_flag &&
                    //queue_entry_index_temp <= encode_context_ptr->hl_rate_control_historgram_queue_head_index+sequence_control_set_ptr->look_ahead_distance){
                    queue_entry_index_temp <= queue_entry_index_head_temp + sequence_control_set_ptr->look_ahead_distance){

                    queue_entry_index_temp2 = (queue_entry_index_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH : queue_entry_index_temp;
                    hl_rate_control_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_temp2]);

                    if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE)
                        ref_qp_index_temp = context_ptr->qp_scaling_map_I_SLICE[ref_qp_index];
                    else
                        ref_qp_index_temp = context_ptr->qp_scaling_map[hl_rate_control_histogram_ptr_temp->temporal_layer_index][ref_qp_index];

                    ref_qp_index_temp = (uint32_t)CLIP3(
                        sequence_control_set_ptr->static_config.min_qp_allowed,
                        sequence_control_set_ptr->static_config.max_qp_allowed,
                        ref_qp_index_temp);

                    hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = 0;

                    if (hl_rate_control_histogram_ptr_temp->is_coded){
                        hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->total_num_bitsCoded;
                    }
                    else{
                        rate_control_tables_ptr = &encode_context_ptr->rate_control_tables_array[ref_qp_index_temp];
                        sad_bits_array_ptr = rate_control_tables_ptr->sad_bits_array[hl_rate_control_histogram_ptr_temp->temporal_layer_index];
                        intra_sad_bits_array_ptr = rate_control_tables_ptr->intra_sad_bits_array[hl_rate_control_histogram_ptr_temp->temporal_layer_index];
                        pred_bits_ref_qp = 0;

                        num_of_full_sbs = 0;

                        if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE){
                            // Loop over block in the frame and calculated the predicted bits at reg QP

                            {
                                unsigned i;
                                uint32_t accum = 0;
                                for (i = 0; i < NUMBER_OF_INTRA_SAD_INTERVALS; ++i)
                                {
                                    accum += (uint32_t)(hl_rate_control_histogram_ptr_temp->ois_distortion_histogram[i] * intra_sad_bits_array_ptr[i]);
                                }

                                pred_bits_ref_qp = accum;
                                num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;
                            }
                            hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                        }

                        else{
                            unsigned i;
                            uint32_t accum = 0;
                            uint32_t accum_intra = 0;
                            for (i = 0; i < NUMBER_OF_SAD_INTERVALS; ++i)
                            {
                                accum += (uint32_t)(hl_rate_control_histogram_ptr_temp->me_distortion_histogram[i] * sad_bits_array_ptr[i]);
                                accum_intra += (uint32_t)(hl_rate_control_histogram_ptr_temp->ois_distortion_histogram[i] * intra_sad_bits_array_ptr[i]);

                            }
                            if (accum > accum_intra * 3)
                                pred_bits_ref_qp = accum_intra;
                            else
                                pred_bits_ref_qp = accum;
                            num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;
                            hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                        }

                        // Scale for in complete
                        //  pred_bits_ref_qp is normalized based on the area because of the LCUs at the picture boundries
                        hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] * (uint64_t)area_in_pixel / (num_of_full_sbs << 12);

                    }
                    high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] += hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];
                    // Store the pred_bits_ref_qp for the first frame in the window to PCS
                    //  if(encode_context_ptr->hl_rate_control_historgram_queue_head_index == queue_entry_index_temp2)
                    if (queue_entry_index_head_temp == queue_entry_index_temp2)
                        picture_control_set_ptr->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];

                    end_of_sequence_flag = hl_rate_control_histogram_ptr_temp->end_of_sequence_flag;
                    queue_entry_index_temp++;
                }
            }
        }
#endif
        picture_control_set_ptr->tables_updated = tables_updated;

        // Looping over the window to find the percentage of bit allocation in each layer
        if ((sequence_control_set_ptr->intra_period != -1) &&
            ((int32_t)picture_control_set_ptr->frames_in_sw >  sequence_control_set_ptr->intra_period) &&
            ((int32_t)picture_control_set_ptr->frames_in_sw >  sequence_control_set_ptr->intra_period)){

            if (picture_control_set_ptr->picture_number % ((sequence_control_set_ptr->intra_period + 1)) == 0){

                queue_entry_index_head_temp = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
                queue_entry_index_head_temp += encode_context_ptr->hl_rate_control_historgram_queue_head_index;
                queue_entry_index_head_temp = (queue_entry_index_head_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                    queue_entry_index_head_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                    queue_entry_index_head_temp;

                queue_entry_index_temp = queue_entry_index_head_temp;

                // This is set to false, so the last frame would go inside the loop
                end_of_sequence_flag = EB_FALSE;

                while (!end_of_sequence_flag &&
                    queue_entry_index_temp <= queue_entry_index_head_temp + sequence_control_set_ptr->intra_period){

                    queue_entry_index_temp2 = (queue_entry_index_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH : queue_entry_index_temp;
                    hl_rate_control_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_temp2]);

                    if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE)
                        ref_qp_index_temp = context_ptr->qp_scaling_map_I_SLICE[selected_ref_qp];
                    else
                        ref_qp_index_temp = context_ptr->qp_scaling_map[hl_rate_control_histogram_ptr_temp->temporal_layer_index][selected_ref_qp];

                    ref_qp_index_temp = (uint32_t)CLIP3(
                        sequence_control_set_ptr->static_config.min_qp_allowed,
                        sequence_control_set_ptr->static_config.max_qp_allowed,
                        ref_qp_index_temp);

                    picture_control_set_ptr->sb_total_bits_per_gop += hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];
                    picture_control_set_ptr->bits_per_sw_per_layer[hl_rate_control_histogram_ptr_temp->temporal_layer_index] += hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];
                    picture_control_set_ptr->percentage_updated = EB_TRUE;

                    end_of_sequence_flag = hl_rate_control_histogram_ptr_temp->end_of_sequence_flag;
                    queue_entry_index_temp++;
                }
                if (picture_control_set_ptr->sb_total_bits_per_gop == 0){
                    for (temporal_layer_index = 0; temporal_layer_index < EB_MAX_TEMPORAL_LAYERS; temporal_layer_index++){
                        picture_control_set_ptr->bits_per_sw_per_layer[temporal_layer_index] = rate_percentage_layer_array[sequence_control_set_ptr->hierarchical_levels][temporal_layer_index];
                    }
                }
            }
        }
        else{
            for (temporal_layer_index = 0; temporal_layer_index < EB_MAX_TEMPORAL_LAYERS; temporal_layer_index++){
                picture_control_set_ptr->bits_per_sw_per_layer[temporal_layer_index] = rate_percentage_layer_array[sequence_control_set_ptr->hierarchical_levels][temporal_layer_index];
            }
        }

        // Set the QP
        previous_selected_ref_qp = selected_ref_qp;
        if (picture_control_set_ptr->picture_number > max_coded_poc && picture_control_set_ptr->temporal_layer_index < 2 && !picture_control_set_ptr->end_of_sequence_region){

            max_coded_poc = picture_control_set_ptr->picture_number;
            max_coded_poc_selected_ref_qp = previous_selected_ref_qp;
            encode_context_ptr->previous_selected_ref_qp = previous_selected_ref_qp;
            encode_context_ptr->max_coded_poc = max_coded_poc;
            encode_context_ptr->max_coded_poc_selected_ref_qp = max_coded_poc_selected_ref_qp;
        }

        if (picture_control_set_ptr->slice_type == I_SLICE)
            picture_control_set_ptr->best_pred_qp = (uint8_t)context_ptr->qp_scaling_map_I_SLICE[selected_ref_qp];
        else
            picture_control_set_ptr->best_pred_qp = (uint8_t)context_ptr->qp_scaling_map[picture_control_set_ptr->temporal_layer_index][selected_ref_qp];

        picture_control_set_ptr->target_bits_best_pred_qp = picture_control_set_ptr->pred_bits_ref_qp[picture_control_set_ptr->best_pred_qp];
        picture_control_set_ptr->best_pred_qp = (uint8_t)CLIP3(
            sequence_control_set_ptr->static_config.min_qp_allowed,
            sequence_control_set_ptr->static_config.max_qp_allowed,
            (uint8_t)((int)picture_control_set_ptr->best_pred_qp + delta_qp));

#if RC_UPDATE_TARGET_RATE
        if (picture_control_set_ptr->picture_number == 0){
            high_level_rate_control_ptr->prev_intra_selected_ref_qp = selected_ref_qp;
            high_level_rate_control_ptr->prev_intra_org_selected_ref_qp = selected_ref_qp;
        }
        if (sequence_control_set_ptr->intra_period != -1){
            if (picture_control_set_ptr->picture_number % ((sequence_control_set_ptr->intra_period + 1)) == 0){
                high_level_rate_control_ptr->prev_intra_selected_ref_qp = selected_ref_qp;
                high_level_rate_control_ptr->prev_intra_org_selected_ref_qp = selected_org_ref_qp;
            }
        }
#endif
#if 0 //VP9_RC_PRINTS
        ////if (picture_control_set_ptr->slice_type == 2)
         {
        SVT_LOG("\nTID: %d\t", picture_control_set_ptr->temporal_layer_index);
        SVT_LOG("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n",
            picture_control_set_ptr->picture_number,
            picture_control_set_ptr->best_pred_qp,
            selected_ref_qp,
            (int)picture_control_set_ptr->target_bits_best_pred_qp,
            (int)high_level_rate_control_ptr->pred_bits_ref_qpPerSw[selected_ref_qp - 1],
            (int)high_level_rate_control_ptr->pred_bits_ref_qpPerSw[selected_ref_qp],
            (int)high_level_rate_control_ptr->pred_bits_ref_qpPerSw[selected_ref_qp + 1],
            (int)high_level_rate_control_ptr->bit_constraint_per_sw,
            (int)bit_constraint_per_sw/*,
            (int)high_level_rate_control_ptr->virtual_buffer_level*/);
        }
#endif
    }
    eb_vp9_release_mutex(sequence_control_set_ptr->encode_context_ptr->rate_table_update_mutex);
}
void eb_vp9_frame_level_rc_input_picture_cbr(
    PictureControlSet               *picture_control_set_ptr,
    SequenceControlSet              *sequence_control_set_ptr,
    RateControlContext              *context_ptr,
    RateControlLayerContext         *rate_control_layer_ptr,
    RateControlIntervalParamContext *rate_control_param_ptr)
{

    RateControlLayerContext *rate_control_layer_temp_ptr;

    // Tiles
    uint32_t                 picture_area_in_pixel;
    uint32_t                 area_in_pixel;

    // LCU Loop variables
    SbParams                *sb_params_ptr;
    uint32_t                 sb_index;
    uint64_t                 temp_qp;
    uint32_t                 area_in_sbs;

    picture_area_in_pixel = sequence_control_set_ptr->luma_height*sequence_control_set_ptr->luma_width;

    if (rate_control_layer_ptr->first_frame == 1){
        rate_control_layer_ptr->first_frame = 0;
        picture_control_set_ptr->parent_pcs_ptr->first_frame_in_temporal_layer = 1;
    }
    else{
        picture_control_set_ptr->parent_pcs_ptr->first_frame_in_temporal_layer = 0;
    }
    if (picture_control_set_ptr->slice_type != I_SLICE) {
        if (rate_control_layer_ptr->first_non_intra_frame == 1){
            rate_control_layer_ptr->first_non_intra_frame = 0;
            picture_control_set_ptr->parent_pcs_ptr->first_non_intra_frame_in_temporal_layer = 1;
        }
        else{
            picture_control_set_ptr->parent_pcs_ptr->first_non_intra_frame_in_temporal_layer = 0;
        }
    }else
        picture_control_set_ptr->parent_pcs_ptr->first_non_intra_frame_in_temporal_layer = 0;

    picture_control_set_ptr->parent_pcs_ptr->target_bits_rc = 0;

    // ***Rate Control***
    area_in_sbs = 0;
    area_in_pixel = 0;

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

        sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];

        if (sb_params_ptr->is_complete_sb) {
            // add the area of one LCU (64x64=4096) to the area of the tile
            area_in_pixel += 4096;
            area_in_sbs++;
        }
        else{
            // add the area of the LCU to the area of the tile
            area_in_pixel += sb_params_ptr->width * sb_params_ptr->height;
        }
    }
    rate_control_layer_ptr->area_in_pixel = area_in_pixel;

    if (picture_control_set_ptr->parent_pcs_ptr->first_frame_in_temporal_layer || (picture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc)){
        if (sequence_control_set_ptr->enable_qp_scaling_flag && (picture_control_set_ptr->picture_number != rate_control_param_ptr->first_poc)) {

            picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                (int32_t)sequence_control_set_ptr->static_config.min_qp_allowed,
                (int32_t)sequence_control_set_ptr->static_config.max_qp_allowed,
                (int32_t)(rate_control_param_ptr->intra_frames_qp + context_ptr->qp_scaling_map[picture_control_set_ptr->temporal_layer_index][rate_control_param_ptr->intra_frames_qp_bef_scal] - context_ptr->qp_scaling_map_I_SLICE[rate_control_param_ptr->intra_frames_qp_bef_scal]));
        }

        if (picture_control_set_ptr->picture_number == 0){
            rate_control_param_ptr->intra_frames_qp = sequence_control_set_ptr->qp;
            rate_control_param_ptr->intra_frames_qp_bef_scal = (uint8_t)sequence_control_set_ptr->qp;
        }

        if (picture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc){
            uint32_t temporal_layer_idex;
            rate_control_param_ptr->previous_virtual_buffer_level = context_ptr->virtual_buffer_level_initial_value;
            rate_control_param_ptr->virtual_buffer_level = context_ptr->virtual_buffer_level_initial_value;
            rate_control_param_ptr->extra_ap_bit_ratio_i = 0;
            if (picture_control_set_ptr->parent_pcs_ptr->end_of_sequence_region){
                rate_control_param_ptr->last_poc = MAX(rate_control_param_ptr->first_poc + picture_control_set_ptr->parent_pcs_ptr->frames_in_sw - 1, rate_control_param_ptr->first_poc);
                rate_control_param_ptr->last_gop = EB_TRUE;
            }

            for (temporal_layer_idex = 0; temporal_layer_idex < EB_MAX_TEMPORAL_LAYERS; temporal_layer_idex++){

                rate_control_layer_temp_ptr = rate_control_param_ptr->rate_control_layer_array[temporal_layer_idex];
                eb_vp9_rate_control_layer_reset(
                    rate_control_layer_temp_ptr,
                    picture_control_set_ptr,
                    context_ptr,
                    picture_area_in_pixel,
                    rate_control_param_ptr->was_used);
            }
        }

        picture_control_set_ptr->parent_pcs_ptr->sad_me = 0;
        // Finding the QP of the Intra frame by using variance tables
        if (picture_control_set_ptr->slice_type == I_SLICE) {
            uint32_t         selected_ref_qp;

            if (sequence_control_set_ptr->look_ahead_distance == 0){
                uint32_t         selected_ref_qp_table_index;
                uint32_t         intra_sad_interval_index;
                uint32_t         ref_qp_index;
                uint32_t         ref_qp_table_index;
                uint32_t         qp_search_min;
                uint32_t         qp_search_max;
                uint32_t         num_of_full_sbs;
                uint64_t         min_la_bit_distance;

                min_la_bit_distance = MAX_UNSIGNED_VALUE;
                selected_ref_qp_table_index = 0;
                selected_ref_qp = selected_ref_qp_table_index;
                qp_search_min = (uint32_t)CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    (uint32_t)MAX((int32_t)sequence_control_set_ptr->qp - 40, 0));

                qp_search_max = CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    sequence_control_set_ptr->qp + 40);

                if (!sequence_control_set_ptr->encode_context_ptr->rate_control_tables_array_updated) {
                    context_ptr->intra_coef_rate = CLIP3(
                        1,
                        2,
                        (uint32_t)(rate_control_layer_ptr->frame_rate >> 16) / 4);
                }
                else{
                    if (context_ptr->base_layer_frames_avg_qp > context_ptr->base_layer_intra_frames_avg_qp + 3)
                        context_ptr->intra_coef_rate--;
                    else if (context_ptr->base_layer_frames_avg_qp <= context_ptr->base_layer_intra_frames_avg_qp + 2)
                        context_ptr->intra_coef_rate += 2;
                    else if (context_ptr->base_layer_frames_avg_qp <= context_ptr->base_layer_intra_frames_avg_qp)
                        context_ptr->intra_coef_rate++;

                    context_ptr->intra_coef_rate = CLIP3(
                        1,
                        15,//(uint32_t) (context_ptr->frames_in_interval[0]+1) / 2,
                        context_ptr->intra_coef_rate);
                }

                // Loop over proper QPs and find the Predicted bits for that QP. Find the QP with the closest total predicted rate to target bits for the sliding window.
                for (ref_qp_table_index = qp_search_min; ref_qp_table_index < qp_search_max; ref_qp_table_index++){
                    ref_qp_index = ref_qp_table_index;

                    picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] = 0;

                    num_of_full_sbs = 0;
                    // Loop over block in the frame and calculated the predicted bits at reg QP
                    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

                        sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];

                        if (sb_params_ptr->is_complete_sb) {
                            num_of_full_sbs++;
                            intra_sad_interval_index = picture_control_set_ptr->parent_pcs_ptr->intra_sad_interval_index[sb_index];
                            picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] += sequence_control_set_ptr->encode_context_ptr->rate_control_tables_array[ref_qp_index].intra_sad_bits_array[picture_control_set_ptr->temporal_layer_index][intra_sad_interval_index];

                        }
                    }

                    // Scale for in complete LCUs
                    //  pred_bits_ref_qp is normalized based on the area because of the LCUs at the tile boundries
                    picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] = picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] * rate_control_layer_ptr->area_in_pixel / (num_of_full_sbs << 12);

                    if (min_la_bit_distance > (uint64_t)ABS((int64_t)rate_control_layer_ptr->ec_bit_constraint*context_ptr->intra_coef_rate - (int64_t)picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index])){
                        min_la_bit_distance = (uint64_t)ABS((int64_t)rate_control_layer_ptr->ec_bit_constraint*context_ptr->intra_coef_rate - (int64_t)picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index]);

                        selected_ref_qp_table_index = ref_qp_table_index;
                        selected_ref_qp = ref_qp_index;
                    }
                }

                if (!sequence_control_set_ptr->encode_context_ptr->rate_control_tables_array_updated) {
                    picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)selected_ref_qp - (int32_t)1, 0);
                    rate_control_layer_ptr->calculated_frame_qp              = (uint8_t)MAX((int32_t)selected_ref_qp - (int32_t)1, 0);
                    picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->picture_qp;
                }
                else{
                    picture_control_set_ptr->picture_qp  = (uint8_t)selected_ref_qp;
                    rate_control_layer_ptr->calculated_frame_qp = (uint8_t)selected_ref_qp;
                    picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->picture_qp;
                    picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                        (uint32_t)MAX((int32_t)context_ptr->base_layer_frames_avg_qp - (int32_t)3, 0),
                        context_ptr->base_layer_frames_avg_qp,
                        picture_control_set_ptr->picture_qp);
                    picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                        (uint32_t)MAX((int32_t)context_ptr->base_layer_intra_frames_avg_qp - (int32_t)5, 0),
                        context_ptr->base_layer_intra_frames_avg_qp + 2,
                        picture_control_set_ptr->picture_qp);
                }
            }
            else{
                selected_ref_qp = picture_control_set_ptr->parent_pcs_ptr->best_pred_qp;
                picture_control_set_ptr->picture_qp = (uint8_t)selected_ref_qp;
                picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->picture_qp;
            }

            picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                sequence_control_set_ptr->static_config.max_qp_allowed,
                picture_control_set_ptr->picture_qp);
        }
        else{

            // LCU Loop
            for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

                sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];

                if (sb_params_ptr->is_complete_sb) {
                    picture_control_set_ptr->parent_pcs_ptr->sad_me += picture_control_set_ptr->parent_pcs_ptr->rcme_distortion[sb_index];
                }
            }

            //  tileSadMe is normalized based on the area because of the LCUs at the tile boundries
            picture_control_set_ptr->parent_pcs_ptr->sad_me = MAX((picture_control_set_ptr->parent_pcs_ptr->sad_me*rate_control_layer_ptr->area_in_pixel / (area_in_sbs << 12)), 1);

            // totalSquareMad has RC_PRECISION precision
            picture_control_set_ptr->parent_pcs_ptr->sad_me <<= RC_PRECISION;

        }

        temp_qp = picture_control_set_ptr->picture_qp;

        if (picture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc){
            uint32_t temporal_layer_idex;
            for (temporal_layer_idex = 0; temporal_layer_idex < EB_MAX_TEMPORAL_LAYERS; temporal_layer_idex++){
                rate_control_layer_temp_ptr = rate_control_param_ptr->rate_control_layer_array[temporal_layer_idex];
                eb_vp9_rate_control_layer_reset_part2(
                    context_ptr,
                    rate_control_layer_temp_ptr,
                    picture_control_set_ptr);
            }
        }

        if (picture_control_set_ptr->picture_number == 0){
            context_ptr->base_layer_frames_avg_qp = picture_control_set_ptr->picture_qp + 1;
            context_ptr->base_layer_intra_frames_avg_qp = picture_control_set_ptr->picture_qp;
        }
    }
    else{

        picture_control_set_ptr->parent_pcs_ptr->sad_me = 0;

#if 1

        HighLevelRateControlContext *high_level_rate_control_ptr = context_ptr->high_level_rate_control_ptr;
        EncodeContext               *encode_context_ptr = sequence_control_set_ptr->encode_context_ptr;
        HlRateControlHistogramEntry *hl_rate_control_histogram_ptr_temp;
        // Queue variables
        uint32_t                     queue_entry_index_temp;
        uint32_t                     queue_entry_index_temp2;
        uint32_t                     queue_entry_index_head_temp;

        uint64_t                     min_la_bit_distance;
        uint32_t                     selected_ref_qp_table_index;
        uint32_t                     selected_ref_qp;
        uint32_t                     previous_selected_ref_qp = encode_context_ptr->previous_selected_ref_qp;

        uint32_t                     ref_qp_index;
        uint32_t                     ref_qp_index_temp;
        uint32_t                     ref_qp_table_index;

        uint32_t                     num_of_full_sbs;
        uint32_t                     qp_search_min;
        uint32_t                     qp_search_max;
        int32_t                      qp_step = 1;
        EB_BOOL                      best_qp_found;

        uint64_t                     bit_constraint_per_sw = 0;

        RateControlTables           *rate_control_tables_ptr;
        EbBitNumber                 *sad_bits_array_ptr;
        EbBitNumber                 *intra_sad_bits_array_ptr;
        uint32_t                     pred_bits_ref_qp;
        EB_BOOL                      end_of_sequence_flag = EB_TRUE;

        // Loop over the QPs and find the best QP
        min_la_bit_distance = MAX_UNSIGNED_VALUE;
        qp_search_min = (uint8_t)CLIP3(
            sequence_control_set_ptr->static_config.min_qp_allowed,
            MAX_REF_QP_NUM,//sequence_control_set_ptr->static_config.max_qp_allowed,
            (uint32_t)MAX((int32_t)sequence_control_set_ptr->qp - 40, 0));

        qp_search_max = (uint8_t)CLIP3(
            sequence_control_set_ptr->static_config.min_qp_allowed,
            MAX_REF_QP_NUM,
            sequence_control_set_ptr->qp + 40);

        for (ref_qp_table_index = qp_search_min; ref_qp_table_index < qp_search_max; ref_qp_table_index++) {
            high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_table_index] = 0;
        }

        // Finding the predicted bits for each frame in the sliding window at the reference Qp(s)
        ///queue_entry_index_head_temp = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
        queue_entry_index_head_temp = (int32_t)(rate_control_param_ptr->first_poc - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
        queue_entry_index_head_temp += encode_context_ptr->hl_rate_control_historgram_queue_head_index;
        queue_entry_index_head_temp = (queue_entry_index_head_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
            queue_entry_index_head_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
            queue_entry_index_head_temp;

        if (picture_control_set_ptr->parent_pcs_ptr->picture_number + picture_control_set_ptr->parent_pcs_ptr->frames_in_sw > rate_control_param_ptr->first_poc + sequence_control_set_ptr->static_config.intra_period + 1)
            bit_constraint_per_sw = high_level_rate_control_ptr->bit_constraint_per_sw;
        else
            bit_constraint_per_sw = high_level_rate_control_ptr->bit_constraint_per_sw * encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_head_temp]->frames_in_sw / (sequence_control_set_ptr->look_ahead_distance + 1);

        // Loop over proper QPs and find the Predicted bits for that QP. Find the QP with the closest total predicted rate to target bits for the sliding window.
        previous_selected_ref_qp = CLIP3(
            qp_search_min + 1,
            qp_search_max - 1,
            previous_selected_ref_qp);
        ref_qp_table_index = previous_selected_ref_qp;
        ref_qp_index = ref_qp_table_index;
        selected_ref_qp_table_index = ref_qp_table_index;
        selected_ref_qp = selected_ref_qp_table_index;
        best_qp_found = EB_FALSE;
        while (ref_qp_table_index >= qp_search_min && ref_qp_table_index <= qp_search_max && !best_qp_found) {

            ref_qp_index = CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                MAX_REF_QP_NUM,
                ref_qp_table_index);
            high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] = 0;

            queue_entry_index_temp = queue_entry_index_head_temp;
            // This is set to false, so the last frame would go inside the loop
            end_of_sequence_flag = EB_FALSE;

            while (!end_of_sequence_flag &&
                //queue_entry_index_temp <= queue_entry_index_head_temp + sequence_control_set_ptr->look_ahead_distance) {
                queue_entry_index_temp <= queue_entry_index_head_temp + encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_head_temp]->frames_in_sw-1) {

                queue_entry_index_temp2 = (queue_entry_index_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH : queue_entry_index_temp;
                hl_rate_control_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_temp2]);

                if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE)
                    ref_qp_index_temp = context_ptr->qp_scaling_map_I_SLICE[ref_qp_index];
                else
                    ref_qp_index_temp = context_ptr->qp_scaling_map[hl_rate_control_histogram_ptr_temp->temporal_layer_index][ref_qp_index];

                ref_qp_index_temp = (uint32_t)CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    ref_qp_index_temp);

                hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = 0;

                if (ref_qp_table_index == previous_selected_ref_qp) {
                    hl_rate_control_histogram_ptr_temp->life_count--;
                }
                if (hl_rate_control_histogram_ptr_temp->is_coded) {
                    // If the frame is already coded, use the actual number of bits
                    hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->total_num_bitsCoded;
                }
                else {
                    rate_control_tables_ptr = &encode_context_ptr->rate_control_tables_array[ref_qp_index_temp];
                    sad_bits_array_ptr = rate_control_tables_ptr->sad_bits_array[hl_rate_control_histogram_ptr_temp->temporal_layer_index];
                    intra_sad_bits_array_ptr = rate_control_tables_ptr->intra_sad_bits_array[0];
                    pred_bits_ref_qp = 0;
                    num_of_full_sbs = 0;

                    if (hl_rate_control_histogram_ptr_temp->slice_type == I_SLICE) {
                        // Loop over block in the frame and calculated the predicted bits at reg QP
                        unsigned i;
                        uint32_t accum = 0;
                        for (i = 0; i < NUMBER_OF_INTRA_SAD_INTERVALS; ++i)
                        {
                            accum += (uint32_t)((uint32_t)hl_rate_control_histogram_ptr_temp->ois_distortion_histogram[i] * (uint32_t)intra_sad_bits_array_ptr[i]);
                        }

                        pred_bits_ref_qp = accum;
                        num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;
                        hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                    }
                    else {
                        unsigned i;
                        uint32_t accum = 0;
                        uint32_t accum_intra = 0;
                        for (i = 0; i < NUMBER_OF_SAD_INTERVALS; ++i)
                        {
                            accum += (uint32_t)((uint32_t)hl_rate_control_histogram_ptr_temp->me_distortion_histogram[i] * (uint32_t)sad_bits_array_ptr[i]);
                            accum_intra += (uint32_t)((uint32_t)hl_rate_control_histogram_ptr_temp->ois_distortion_histogram[i] * (uint32_t)intra_sad_bits_array_ptr[i]);

                        }
                        if (accum > accum_intra * 3)
                            pred_bits_ref_qp = accum_intra;
                        else
                            pred_bits_ref_qp = accum;
                        num_of_full_sbs = hl_rate_control_histogram_ptr_temp->full_sb_count;
                        hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] += pred_bits_ref_qp;
                    }

                    // Scale for in complete LCSs
                    //  pred_bits_ref_qp is normalized based on the area because of the LCUs at the picture boundries
                    hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp] * (uint64_t)rate_control_layer_ptr->area_in_pixel / (num_of_full_sbs << 12);

                }
                high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] += hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];
                // Store the pred_bits_ref_qp for the first frame in the window to PCS
                if (queue_entry_index_head_temp == queue_entry_index_temp2)
                    picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index_temp] = hl_rate_control_histogram_ptr_temp->pred_bits_ref_qp[ref_qp_index_temp];

                end_of_sequence_flag = hl_rate_control_histogram_ptr_temp->end_of_sequence_flag;
                queue_entry_index_temp++;
            }

            if (min_la_bit_distance >= (uint64_t)ABS((int64_t)high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] - (int64_t)bit_constraint_per_sw)) {
                min_la_bit_distance = (uint64_t)ABS((int64_t)high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] - (int64_t)bit_constraint_per_sw);
                selected_ref_qp_table_index = ref_qp_table_index;
                selected_ref_qp = ref_qp_index;
            }
            else {
                best_qp_found = EB_TRUE;
            }

            if (ref_qp_table_index == previous_selected_ref_qp) {
                if (high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] > bit_constraint_per_sw) {
                    qp_step = +1;
                }
                else {
                    qp_step = -1;
                }
            }
            ref_qp_table_index = (uint32_t)(ref_qp_table_index + qp_step);

        }

        int delta_qp = 0;
        if (ref_qp_index == sequence_control_set_ptr->static_config.max_qp_allowed && high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] > bit_constraint_per_sw) {
            delta_qp = (int)((high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index] - bit_constraint_per_sw) * 100 / (high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index - 1] - high_level_rate_control_ptr->pred_bits_ref_qpPerSw[ref_qp_index]));
            delta_qp = (delta_qp + 50) / 100;
        }

        if (picture_control_set_ptr->slice_type == I_SLICE)
            picture_control_set_ptr->parent_pcs_ptr->best_pred_qp = (uint8_t)context_ptr->qp_scaling_map_I_SLICE[selected_ref_qp];
        else
            picture_control_set_ptr->parent_pcs_ptr->best_pred_qp = (uint8_t)context_ptr->qp_scaling_map[picture_control_set_ptr->temporal_layer_index][selected_ref_qp];

        picture_control_set_ptr->parent_pcs_ptr->best_pred_qp = (uint8_t)CLIP3(
            sequence_control_set_ptr->static_config.min_qp_allowed,
            sequence_control_set_ptr->static_config.max_qp_allowed,
            (uint8_t)((int)picture_control_set_ptr->parent_pcs_ptr->best_pred_qp + delta_qp));

#if VP9_RC_PRINTS
        // if (picture_control_set_ptr->slice_type == 2)
        {
            SVT_LOG("\nTID2: %d\t", picture_control_set_ptr->temporal_layer_index);
            SVT_LOG("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n",
                picture_control_set_ptr->picture_number,
                picture_control_set_ptr->parent_pcs_ptr->best_pred_qp,
                selected_ref_qp,
                (int)picture_control_set_ptr->parent_pcs_ptr->targetBitsbest_pred_qp,
                (int)high_level_rate_control_ptr->pred_bits_ref_qp_per_sw[selected_ref_qp - 1],
                (int)high_level_rate_control_ptr->pred_bits_ref_qp_per_sw[selected_ref_qp],
                (int)high_level_rate_control_ptr->pred_bits_ref_qp_per_sw[selected_ref_qp + 1],
                (int)high_level_rate_control_ptr->bit_constraint_per_sw,
                (int)bit_constraint_per_sw/*,
                (int)high_level_rate_control_ptr->virtual_buffer_level*/);
        }
#endif
#endif

        // if the pixture is an I slice, for now we set the QP as the QP of the previous frame
        if (picture_control_set_ptr->slice_type == I_SLICE) {
            uint32_t         selected_ref_qp;

            if (sequence_control_set_ptr->look_ahead_distance == 0)
            {
                uint32_t         selected_ref_qp_table_index;
                uint32_t         intra_sad_interval_index;
                uint32_t         ref_qp_index;
                uint32_t         ref_qp_table_index;
                uint32_t         qp_search_min;
                uint32_t         qp_search_max;
                uint32_t         num_of_full_sbs;
                uint64_t         min_la_bit_distance;

                min_la_bit_distance = MAX_UNSIGNED_VALUE;
                selected_ref_qp_table_index = 0;
                selected_ref_qp = selected_ref_qp_table_index;
                qp_search_min = (uint8_t)CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    (uint32_t)MAX((int32_t)sequence_control_set_ptr->qp - 40, 0));

                qp_search_max = (uint8_t)CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    sequence_control_set_ptr->qp + 40);

                context_ptr->intra_coef_rate = CLIP3(
                    1,
                    (uint32_t)(rate_control_layer_ptr->frame_rate >> 16) / 4,
                    context_ptr->intra_coef_rate);
                // Loop over proper QPs and find the Predicted bits for that QP. Find the QP with the closest total predicted rate to target bits for the sliding window.
                for (ref_qp_table_index = qp_search_min; ref_qp_table_index < qp_search_max; ref_qp_table_index++){
                    ref_qp_index = ref_qp_table_index;
                    picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] = 0;
                    num_of_full_sbs = 0;
                    // Loop over block in the frame and calculated the predicted bits at reg QP
                    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

                        sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];

                        if (sb_params_ptr->is_complete_sb) {
                            num_of_full_sbs++;
                            intra_sad_interval_index = picture_control_set_ptr->parent_pcs_ptr->intra_sad_interval_index[sb_index];
                            picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] += sequence_control_set_ptr->encode_context_ptr->rate_control_tables_array[ref_qp_index].intra_sad_bits_array[picture_control_set_ptr->temporal_layer_index][intra_sad_interval_index];
                        }
                    }

                    // Scale for in complete LCUs
                    // pred_bits_ref_qp is normalized based on the area because of the LCUs at the tile boundries
                    picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] = picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index] * rate_control_layer_ptr->area_in_pixel / (num_of_full_sbs << 12);
                    if (min_la_bit_distance > (uint64_t)ABS((int64_t)rate_control_layer_ptr->ec_bit_constraint*context_ptr->intra_coef_rate - (int64_t)picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index])){
                        min_la_bit_distance = (uint64_t)ABS((int64_t)rate_control_layer_ptr->ec_bit_constraint*context_ptr->intra_coef_rate - (int64_t)picture_control_set_ptr->parent_pcs_ptr->pred_bits_ref_qp[ref_qp_index]);

                        selected_ref_qp_table_index = ref_qp_table_index;
                        selected_ref_qp = ref_qp_index;
                    }
                }
                if (!sequence_control_set_ptr->encode_context_ptr->rate_control_tables_array_updated) {
                    picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)selected_ref_qp - (int32_t)1, 0);
                    rate_control_layer_ptr->calculated_frame_qp              = (uint8_t)MAX((int32_t)selected_ref_qp - (int32_t)1, 0);
                    picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->picture_qp;
                }
                else{
                    picture_control_set_ptr->picture_qp = (uint8_t)selected_ref_qp;
                    rate_control_layer_ptr->calculated_frame_qp = (uint8_t)selected_ref_qp;
                    picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->picture_qp;
                    picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                        (uint32_t)MAX((int32_t)context_ptr->base_layer_frames_avg_qp - (int32_t)3, 0),
                        context_ptr->base_layer_frames_avg_qp + 1,
                        picture_control_set_ptr->picture_qp);
                    picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                        (uint32_t)MAX((int32_t)context_ptr->base_layer_intra_frames_avg_qp - (int32_t)5, 0),
                        context_ptr->base_layer_intra_frames_avg_qp + 2,
                        picture_control_set_ptr->picture_qp);
                }
            }
            else{
                selected_ref_qp = picture_control_set_ptr->parent_pcs_ptr->best_pred_qp;
                picture_control_set_ptr->picture_qp = (uint8_t)selected_ref_qp;
                picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->picture_qp;
            }

            picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                sequence_control_set_ptr->static_config.max_qp_allowed,
                picture_control_set_ptr->picture_qp);

            temp_qp = picture_control_set_ptr->picture_qp;

        }

        else{ // Not an I slice
            // combining the target rate from initial RC and frame level RC
            if (sequence_control_set_ptr->look_ahead_distance != 0){
                picture_control_set_ptr->parent_pcs_ptr->target_bits_rc = rate_control_layer_ptr->bit_constraint;
                rate_control_layer_ptr->ec_bit_constraint = (rate_control_layer_ptr->alpha * picture_control_set_ptr->parent_pcs_ptr->target_bits_best_pred_qp +
                    ((1 << RC_PRECISION) - rate_control_layer_ptr->alpha) * picture_control_set_ptr->parent_pcs_ptr->target_bits_rc + RC_PRECISION_OFFSET) >> RC_PRECISION;

                rate_control_layer_ptr->ec_bit_constraint = (uint64_t)MAX((int64_t)rate_control_layer_ptr->ec_bit_constraint - (int64_t)rate_control_layer_ptr->dif_total_and_ec_bits, 1);

                picture_control_set_ptr->parent_pcs_ptr->target_bits_rc = rate_control_layer_ptr->ec_bit_constraint;
            }

            // LCU Loop
            for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

                sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];

                if (sb_params_ptr->is_complete_sb) {
                    picture_control_set_ptr->parent_pcs_ptr->sad_me += picture_control_set_ptr->parent_pcs_ptr->rcme_distortion[sb_index];
                }
            }

            //  tileSadMe is normalized based on the area because of the LCUs at the tile boundries
            picture_control_set_ptr->parent_pcs_ptr->sad_me = MAX((picture_control_set_ptr->parent_pcs_ptr->sad_me*rate_control_layer_ptr->area_in_pixel / (area_in_sbs << 12)), 1);
            picture_control_set_ptr->parent_pcs_ptr->sad_me <<= RC_PRECISION;

            rate_control_layer_ptr->total_mad = MAX((picture_control_set_ptr->parent_pcs_ptr->sad_me / rate_control_layer_ptr->area_in_pixel), 1);

            if (!rate_control_layer_ptr->feedback_arrived){
                rate_control_layer_ptr->previous_frame_distortion_me = picture_control_set_ptr->parent_pcs_ptr->sad_me;
            }

            {
                uint64_t qp_calc_temp1, qp_calc_temp2, qp_calc_temp3;

                qp_calc_temp1 = picture_control_set_ptr->parent_pcs_ptr->sad_me *rate_control_layer_ptr->total_mad;
                qp_calc_temp2 =
                    MAX((int64_t)(rate_control_layer_ptr->ec_bit_constraint << (2 * RC_PRECISION)) - (int64_t)rate_control_layer_ptr->c_coeff*(int64_t)rate_control_layer_ptr->area_in_pixel,
                    (int64_t)(rate_control_layer_ptr->ec_bit_constraint << (2 * RC_PRECISION - 2)));

                // This is a more complex but with higher precision implementation
                if (qp_calc_temp1 > qp_calc_temp2)
                    qp_calc_temp3 = (uint64_t)((qp_calc_temp1 / qp_calc_temp2)*rate_control_layer_ptr->k_coeff);
                else
                    qp_calc_temp3 = (uint64_t)(qp_calc_temp1*rate_control_layer_ptr->k_coeff / qp_calc_temp2);
                temp_qp = (uint64_t)(eb_vp9_log2f_high_precision(MAX(((qp_calc_temp3 + RC_PRECISION_OFFSET) >> RC_PRECISION)*((qp_calc_temp3 + RC_PRECISION_OFFSET) >> RC_PRECISION)*((qp_calc_temp3 + RC_PRECISION_OFFSET) >> RC_PRECISION), 1), RC_PRECISION));

                rate_control_layer_ptr->calculated_frame_qp = (uint8_t)(CLIP3(1, 63, (uint32_t)(temp_qp + RC_PRECISION_OFFSET) >> RC_PRECISION));
                picture_control_set_ptr->parent_pcs_ptr->calculated_qp = (uint8_t)(CLIP3(1, 63, (uint32_t)(temp_qp + RC_PRECISION_OFFSET) >> RC_PRECISION));
            }

            temp_qp += rate_control_layer_ptr->delta_qp_fraction;
            picture_control_set_ptr->picture_qp = (uint8_t)((temp_qp + RC_PRECISION_OFFSET) >> RC_PRECISION);
            // Use the QP of HLRC instead of calculated one in FLRC
            if (picture_control_set_ptr->parent_pcs_ptr->hierarchical_levels > 1){
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->parent_pcs_ptr->best_pred_qp;
                picture_control_set_ptr->parent_pcs_ptr->calculated_qp = picture_control_set_ptr->parent_pcs_ptr->best_pred_qp;
            }
        }
        if (picture_control_set_ptr->parent_pcs_ptr->first_non_intra_frame_in_temporal_layer && picture_control_set_ptr->temporal_layer_index == 0 && picture_control_set_ptr->slice_type != I_SLICE){
            picture_control_set_ptr->picture_qp = (uint8_t)(rate_control_param_ptr->intra_frames_qp + context_ptr->qp_scaling_map[picture_control_set_ptr->temporal_layer_index][rate_control_param_ptr->intra_frames_qp_bef_scal] - context_ptr->qp_scaling_map_I_SLICE[rate_control_param_ptr->intra_frames_qp_bef_scal]);

        }

        if (!rate_control_layer_ptr->feedback_arrived && picture_control_set_ptr->slice_type != I_SLICE){

            picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                (int32_t)sequence_control_set_ptr->static_config.min_qp_allowed,
                (int32_t)sequence_control_set_ptr->static_config.max_qp_allowed,
                (int32_t)(rate_control_param_ptr->intra_frames_qp + context_ptr->qp_scaling_map[picture_control_set_ptr->temporal_layer_index][rate_control_param_ptr->intra_frames_qp_bef_scal] - context_ptr->qp_scaling_map_I_SLICE[rate_control_param_ptr->intra_frames_qp_bef_scal]));
        }

        if (picture_control_set_ptr->parent_pcs_ptr->end_of_sequence_region){
            if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2 << 2){
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 4;
            }
            else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2 << 1){
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 3;
            }
            else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2){
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 2;
            }
            else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold1 &&
                rate_control_param_ptr->virtual_buffer_level < context_ptr->vb_fill_threshold2){
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD1QPINCREASE + 2;
            }
        }
        else{

            //if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2 << 2){
            if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2 + (int64_t)(context_ptr->virtual_buffer_size * 2 / 3)) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 2 ;

            }
            //else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2 << 1){
            else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2 + (int64_t)(context_ptr->virtual_buffer_size / 3)) {
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 1;

            }
            else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold2){
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD2QPINCREASE + 1;
            }
            else if (rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold1 &&
                rate_control_param_ptr->virtual_buffer_level < context_ptr->vb_fill_threshold2){
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + (uint8_t)THRESHOLD1QPINCREASE;
            }

        }
        if (picture_control_set_ptr->parent_pcs_ptr->end_of_sequence_region){
            if (rate_control_param_ptr->virtual_buffer_level < -(context_ptr->vb_fill_threshold2 << 2))
                picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE - 2, 0);
            else if (rate_control_param_ptr->virtual_buffer_level < -(context_ptr->vb_fill_threshold2 << 1))
                picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE - 1, 0);
            else if (rate_control_param_ptr->virtual_buffer_level < 0)
                picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE, 0);
        }
        else{

            if (rate_control_param_ptr->virtual_buffer_level < -(context_ptr->vb_fill_threshold2 << 2))
                picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE - 1, 0);
            else if (rate_control_param_ptr->virtual_buffer_level < -context_ptr->vb_fill_threshold2)
                picture_control_set_ptr->picture_qp = (uint8_t)MAX((int32_t)picture_control_set_ptr->picture_qp - (int32_t)THRESHOLD2QPINCREASE, 0);
        }

#if !RC_NO_EXTRA
        // limiting the QP based on the predicted QP
        if (sequence_control_set_ptr->look_ahead_distance != 0){
            if (picture_control_set_ptr->parent_pcs_ptr->end_of_sequence_region){
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)MAX((int32_t)picture_control_set_ptr->parent_pcs_ptr->best_pred_qp - 8, 0),
                    (uint32_t)picture_control_set_ptr->parent_pcs_ptr->best_pred_qp + 8,
                    (uint32_t)picture_control_set_ptr->picture_qp);
            }
            else{
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)MAX((int32_t)picture_control_set_ptr->parent_pcs_ptr->best_pred_qp - 8, 0),
                    (uint32_t)picture_control_set_ptr->parent_pcs_ptr->best_pred_qp + 8,
                    (uint32_t)picture_control_set_ptr->picture_qp);

            }
        }
        if (picture_control_set_ptr->picture_number != rate_control_param_ptr->first_poc &&
            picture_control_set_ptr->picture_qp == picture_control_set_ptr->parent_pcs_ptr->best_pred_qp && rate_control_param_ptr->virtual_buffer_level > context_ptr->vb_fill_threshold1){
            if (rate_control_param_ptr->extra_ap_bit_ratio_i > 200){
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + 3;
            }
            else if (rate_control_param_ptr->extra_ap_bit_ratio_i > 100){
                picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + 2;
            }
            else if (rate_control_param_ptr->extra_ap_bit_ratio_i > 50){
                picture_control_set_ptr->picture_qp++;
            }
        }
        //Limiting the QP based on the QP of the Reference frame

        uint32_t ref_qp;
        if ((int32_t)picture_control_set_ptr->temporal_layer_index == 0 && picture_control_set_ptr->slice_type != I_SLICE) {
            if (picture_control_set_ptr->ref_slice_type_array[0] == I_SLICE) {
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)picture_control_set_ptr->ref_pic_qp_array[0],
                    (uint32_t)picture_control_set_ptr->picture_qp,
                    picture_control_set_ptr->picture_qp);
            }
            else {
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)MAX((int32_t)picture_control_set_ptr->ref_pic_qp_array[0] - 1, 0),
                    (uint32_t)picture_control_set_ptr->picture_qp,
                    picture_control_set_ptr->picture_qp);
            }
        }
        else{
            ref_qp = 0;
            if (picture_control_set_ptr->ref_slice_type_array[0] != I_SLICE) {
                ref_qp = MAX(ref_qp, picture_control_set_ptr->ref_pic_qp_array[0]);
            }
            if ((picture_control_set_ptr->slice_type == B_SLICE) && (picture_control_set_ptr->ref_slice_type_array[1] != I_SLICE)) {
                ref_qp = MAX(ref_qp, picture_control_set_ptr->ref_pic_qp_array[1]);
            }
            if (ref_qp > 0) {
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)ref_qp - 1,
                    picture_control_set_ptr->picture_qp,
                    picture_control_set_ptr->picture_qp);
            }
        }
#else
        uint32_t ref_qp;
        if ((int32_t)picture_control_set_ptr->temporal_layer_index == 0 && picture_control_set_ptr->slice_type != I_SLICE) {
            if (picture_control_set_ptr->ref_slice_type_array[0] == I_SLICE) {
                /*    picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                        (uint32_t)picture_control_set_ptr->ref_pic_qp_array[0],
                        (uint32_t)picture_control_set_ptr->picture_qp,
                        picture_control_set_ptr->picture_qp);*/
            }
            else {
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)MAX((int32_t)picture_control_set_ptr->ref_pic_qp_array[0] - 1, 0),
                    (uint32_t)picture_control_set_ptr->ref_pic_qp_array[0] + 3,
                    picture_control_set_ptr->picture_qp);
            }
        }
        else {
            ref_qp = 0;
            if (picture_control_set_ptr->ref_slice_type_array[0] != I_SLICE) {
                ref_qp = MAX(ref_qp, picture_control_set_ptr->ref_pic_qp_array[0]);
            }
            if ((picture_control_set_ptr->slice_type == B_SLICE) && (picture_control_set_ptr->ref_slice_type_array[1] != I_SLICE)) {
                ref_qp = MAX(ref_qp, picture_control_set_ptr->ref_pic_qp_array[1]);
            }
            if (ref_qp > 0) {
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)ref_qp - 1,
                    picture_control_set_ptr->picture_qp,
                    picture_control_set_ptr->picture_qp);
            }
        }

#endif
        // limiting the QP between min Qp allowed and max Qp allowed
        picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
            sequence_control_set_ptr->static_config.min_qp_allowed,
            sequence_control_set_ptr->static_config.max_qp_allowed,
            picture_control_set_ptr->picture_qp);

        rate_control_layer_ptr->delta_qp_fraction = CLIP3(-RC_PRECISION_OFFSET, RC_PRECISION_OFFSET, -((int64_t)temp_qp - (int64_t)(picture_control_set_ptr->picture_qp << RC_PRECISION)));

        if (picture_control_set_ptr->parent_pcs_ptr->sad_me == rate_control_layer_ptr->previous_frame_distortion_me &&
            (rate_control_layer_ptr->previous_frame_distortion_me != 0))
            rate_control_layer_ptr->same_distortion_count++;
        else
            rate_control_layer_ptr->same_distortion_count = 0;
    }

    rate_control_layer_ptr->previous_c_coeff = rate_control_layer_ptr->c_coeff;
    rate_control_layer_ptr->previous_k_coeff = rate_control_layer_ptr->k_coeff;
    rate_control_layer_ptr->previous_calculated_frame_qp = rate_control_layer_ptr->calculated_frame_qp;
}

void eb_vp9_frame_level_rc_feedback_picture_cbr(
    PictureParentControlSet *parentpicture_control_set_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    RateControlContext      *context_ptr)
{

    RateControlLayerContext             *rate_control_layer_temp_ptr;
    RateControlIntervalParamContext     *rate_control_param_ptr;
    RateControlLayerContext             *rate_control_layer_ptr;
    // LCU Loop variables
    uint32_t                       slice_num;
    uint64_t                       previous_frame_bit_actual;

    if (sequence_control_set_ptr->intra_period == -1)
        rate_control_param_ptr = context_ptr->rate_control_param_queue[0];
    else{
        uint32_t interval_index_temp = 0;
        while ((!(parentpicture_control_set_ptr->picture_number >= context_ptr->rate_control_param_queue[interval_index_temp]->first_poc &&
            parentpicture_control_set_ptr->picture_number <= context_ptr->rate_control_param_queue[interval_index_temp]->last_poc)) &&
            (interval_index_temp < PARALLEL_GOP_MAX_NUMBER)){
            interval_index_temp++;
        }
        CHECK_REPORT_ERROR(
            interval_index_temp != PARALLEL_GOP_MAX_NUMBER,
            sequence_control_set_ptr->encode_context_ptr->app_callback_ptr,
            EB_ENC_RC_ERROR0);
        rate_control_param_ptr = context_ptr->rate_control_param_queue[interval_index_temp];
    }

    rate_control_layer_ptr = rate_control_param_ptr->rate_control_layer_array[parentpicture_control_set_ptr->temporal_layer_index];

    rate_control_layer_ptr->max_qp = 0;

    rate_control_layer_ptr->feedback_arrived = EB_TRUE;
    rate_control_layer_ptr->max_qp = MAX(rate_control_layer_ptr->max_qp, parentpicture_control_set_ptr->picture_qp);

    rate_control_layer_ptr->previous_frame_qp = parentpicture_control_set_ptr->picture_qp;
    rate_control_layer_ptr->previous_frame_bit_actual = parentpicture_control_set_ptr->total_num_bits;
    if (parentpicture_control_set_ptr->quantized_coeff_num_bits == 0)
        parentpicture_control_set_ptr->quantized_coeff_num_bits = 1;
    rate_control_layer_ptr->previous_framequantized_coeff_bit_actual = parentpicture_control_set_ptr->quantized_coeff_num_bits;

    // Setting Critical states for adjusting the averaging weights on C and K
    if ((parentpicture_control_set_ptr->sad_me  > (3 * rate_control_layer_ptr->previous_frame_distortion_me) >> 1) &&
        (rate_control_layer_ptr->previous_frame_distortion_me != 0)){
        rate_control_layer_ptr->critical_states = 3;
    }
    else if (rate_control_layer_ptr->critical_states){
        rate_control_layer_ptr->critical_states--;
    }
    else{
        rate_control_layer_ptr->critical_states = 0;
    }

    if (parentpicture_control_set_ptr->slice_type != I_SLICE){
        // Updating c_coeff
        rate_control_layer_ptr->c_coeff = (((int64_t)rate_control_layer_ptr->previous_frame_bit_actual - (int64_t)rate_control_layer_ptr->previous_framequantized_coeff_bit_actual) << (2 * RC_PRECISION))
            / rate_control_layer_ptr->area_in_pixel;
        rate_control_layer_ptr->c_coeff = MAX(rate_control_layer_ptr->c_coeff, 1);

        // Updating k_coeff
        if ((parentpicture_control_set_ptr->sad_me + RC_PRECISION_OFFSET) >> RC_PRECISION > 5){
            {
                uint64_t test1, test2, test3;
                test1 = rate_control_layer_ptr->previous_framequantized_coeff_bit_actual*(two_to_power_qp_over_three[parentpicture_control_set_ptr->picture_qp]);
                test2 = MAX(parentpicture_control_set_ptr->sad_me / rate_control_layer_ptr->area_in_pixel, 1);
                test3 = test1 * 65536 / test2 * 65536 / parentpicture_control_set_ptr->sad_me;

                rate_control_layer_ptr->k_coeff = test3;
            }
        }

        if (rate_control_layer_ptr->critical_states){
            rate_control_layer_ptr->k_coeff = (8 * rate_control_layer_ptr->k_coeff + 8 * rate_control_layer_ptr->previous_k_coeff + 8) >> 4;
            rate_control_layer_ptr->c_coeff = (8 * rate_control_layer_ptr->c_coeff + 8 * rate_control_layer_ptr->previous_c_coeff + 8) >> 4;
        }
        else{
            rate_control_layer_ptr->k_coeff = (rate_control_layer_ptr->coeff_averaging_weight1*rate_control_layer_ptr->k_coeff + rate_control_layer_ptr->coeff_averaging_weight2*rate_control_layer_ptr->previous_k_coeff + 8) >> 4;
            rate_control_layer_ptr->c_coeff = (rate_control_layer_ptr->coeff_averaging_weight1*rate_control_layer_ptr->c_coeff + rate_control_layer_ptr->coeff_averaging_weight2*rate_control_layer_ptr->previous_c_coeff + 8) >> 4;
        }
        rate_control_layer_ptr->k_coeff = MIN(rate_control_layer_ptr->k_coeff, rate_control_layer_ptr->previous_k_coeff * 4);
        rate_control_layer_ptr->c_coeff = MIN(rate_control_layer_ptr->c_coeff, rate_control_layer_ptr->previous_c_coeff * 4);
        if (parentpicture_control_set_ptr->slice_type != I_SLICE) {
            rate_control_layer_ptr->previous_frame_distortion_me = parentpicture_control_set_ptr->sad_me;
        }
        else{
            rate_control_layer_ptr->previous_frame_distortion_me = 0;
        }
    }

    if (sequence_control_set_ptr->look_ahead_distance != 0){
        if (parentpicture_control_set_ptr->slice_type == I_SLICE){
            if (parentpicture_control_set_ptr->total_num_bits < parentpicture_control_set_ptr->target_bits_best_pred_qp << 1)
                context_ptr->base_layer_intra_frames_avg_qp = (3 * context_ptr->base_layer_intra_frames_avg_qp + parentpicture_control_set_ptr->picture_qp + 2) >> 2;
            else if (parentpicture_control_set_ptr->total_num_bits > parentpicture_control_set_ptr->target_bits_best_pred_qp << 2)
                context_ptr->base_layer_intra_frames_avg_qp = (3 * context_ptr->base_layer_intra_frames_avg_qp + parentpicture_control_set_ptr->picture_qp + 4 + 2) >> 2;
            else if (parentpicture_control_set_ptr->total_num_bits > parentpicture_control_set_ptr->target_bits_best_pred_qp << 1)
                context_ptr->base_layer_intra_frames_avg_qp = (3 * context_ptr->base_layer_intra_frames_avg_qp + parentpicture_control_set_ptr->picture_qp + 2 + 2) >> 2;
        }
    }

    {
        uint64_t previous_frame_ec_bits = 0;
        EB_BOOL picture_min_qp_allowed = EB_TRUE;
        rate_control_layer_ptr->previous_frame_average_qp = 0;
        rate_control_layer_ptr->previous_frame_average_qp += rate_control_layer_ptr->previous_frame_qp;
        previous_frame_ec_bits += rate_control_layer_ptr->previous_frame_bit_actual;
        if (rate_control_layer_ptr->same_distortion_count == 0 ||
            parentpicture_control_set_ptr->picture_qp != sequence_control_set_ptr->static_config.min_qp_allowed){
            picture_min_qp_allowed = EB_FALSE;
        }
        if (picture_min_qp_allowed)
            rate_control_layer_ptr->frame_same_distortion_min_qp_count++;
        else
            rate_control_layer_ptr->frame_same_distortion_min_qp_count = 0;

        rate_control_layer_ptr->previous_ec_bits = previous_frame_ec_bits;
        previous_frame_bit_actual = parentpicture_control_set_ptr->total_num_bits;
        if (parentpicture_control_set_ptr->first_frame_in_temporal_layer){
            rate_control_layer_ptr->dif_total_and_ec_bits = (previous_frame_bit_actual - previous_frame_ec_bits);
        }
        else{
            rate_control_layer_ptr->dif_total_and_ec_bits = ((previous_frame_bit_actual - previous_frame_ec_bits) + rate_control_layer_ptr->dif_total_and_ec_bits) >> 1;
        }

        // update bitrate of different layers in the interval based on the rate of the I frame
        if (parentpicture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc &&
            (parentpicture_control_set_ptr->slice_type == I_SLICE) &&
            sequence_control_set_ptr->static_config.intra_period != -1){
            uint32_t temporal_layer_idex;
            uint64_t target_bit_rate;
            uint64_t channel_bit_rate;
            uint64_t sum_bits_per_sw = 0;
#if ADAPTIVE_PERCENTAGE
            if (sequence_control_set_ptr->look_ahead_distance != 0){
                if (parentpicture_control_set_ptr->tables_updated && parentpicture_control_set_ptr->percentage_updated){
                    parentpicture_control_set_ptr->bits_per_sw_per_layer[0] =
                        (uint64_t)MAX((int64_t)parentpicture_control_set_ptr->bits_per_sw_per_layer[0] + (int64_t)parentpicture_control_set_ptr->total_num_bits - (int64_t)parentpicture_control_set_ptr->target_bits_best_pred_qp, 1);
                }
            }
#endif

            if (sequence_control_set_ptr->look_ahead_distance != 0 && sequence_control_set_ptr->intra_period != -1){
                for (temporal_layer_idex = 0; temporal_layer_idex < EB_MAX_TEMPORAL_LAYERS; temporal_layer_idex++){
                    sum_bits_per_sw += parentpicture_control_set_ptr->bits_per_sw_per_layer[temporal_layer_idex];
                }
            }

            for (temporal_layer_idex = 0; temporal_layer_idex < EB_MAX_TEMPORAL_LAYERS; temporal_layer_idex++){
                rate_control_layer_temp_ptr = rate_control_param_ptr->rate_control_layer_array[temporal_layer_idex];

                target_bit_rate = (uint64_t)((int64_t)parentpicture_control_set_ptr->target_bit_rate -
                    MIN((int64_t)parentpicture_control_set_ptr->target_bit_rate * 3 / 4, (int64_t)(parentpicture_control_set_ptr->total_num_bits*context_ptr->frame_rate / (sequence_control_set_ptr->static_config.intra_period + 1)) >> RC_PRECISION))
                    *rate_percentage_layer_array[sequence_control_set_ptr->hierarchical_levels][temporal_layer_idex] / 100;

#if ADAPTIVE_PERCENTAGE
                if (sequence_control_set_ptr->look_ahead_distance != 0 && sequence_control_set_ptr->intra_period != -1){
                    target_bit_rate = (uint64_t)((int64_t)parentpicture_control_set_ptr->target_bit_rate -
                        MIN((int64_t)parentpicture_control_set_ptr->target_bit_rate * 3 / 4, (int64_t)(parentpicture_control_set_ptr->total_num_bits*context_ptr->frame_rate / (sequence_control_set_ptr->static_config.intra_period + 1)) >> RC_PRECISION))
                        *parentpicture_control_set_ptr->bits_per_sw_per_layer[temporal_layer_idex] / sum_bits_per_sw;
                }
#endif
                // update this based on temporal layers
                if (temporal_layer_idex == 0)
                    channel_bit_rate = (((target_bit_rate << (2 * RC_PRECISION)) / MAX(1, rate_control_layer_temp_ptr->frame_rate - (1 * context_ptr->frame_rate / (sequence_control_set_ptr->static_config.intra_period + 1)))) + RC_PRECISION_OFFSET) >> RC_PRECISION;
                else
                    channel_bit_rate = (((target_bit_rate << (2 * RC_PRECISION)) / rate_control_layer_temp_ptr->frame_rate) + RC_PRECISION_OFFSET) >> RC_PRECISION;
                channel_bit_rate = (uint64_t)MAX((int64_t)1, (int64_t)channel_bit_rate);
                rate_control_layer_temp_ptr->ec_bit_constraint = channel_bit_rate;

                slice_num = 1;
                rate_control_layer_temp_ptr->ec_bit_constraint -= SLICE_HEADER_BITS_NUM*slice_num;

                rate_control_layer_temp_ptr->previous_bit_constraint = channel_bit_rate;
                rate_control_layer_temp_ptr->bit_constraint = channel_bit_rate;
                rate_control_layer_temp_ptr->channel_bit_rate = channel_bit_rate;
            }
            if ((int64_t)parentpicture_control_set_ptr->target_bit_rate * 3 / 4 < (int64_t)(parentpicture_control_set_ptr->total_num_bits*context_ptr->frame_rate / (sequence_control_set_ptr->static_config.intra_period + 1)) >> RC_PRECISION){
                rate_control_param_ptr->previous_virtual_buffer_level += (int64_t)((parentpicture_control_set_ptr->total_num_bits*context_ptr->frame_rate / (sequence_control_set_ptr->static_config.intra_period + 1)) >> RC_PRECISION) - (int64_t)parentpicture_control_set_ptr->target_bit_rate * 3 / 4;
                context_ptr->extra_bits_gen -= (int64_t)((parentpicture_control_set_ptr->total_num_bits*context_ptr->frame_rate / (sequence_control_set_ptr->static_config.intra_period + 1)) >> RC_PRECISION) - (int64_t)parentpicture_control_set_ptr->target_bit_rate * 3 / 4;
#if RC_NO_EXTRA
                context_ptr->extra_bits_gen = 0;
#endif
            }
        }

        if (previous_frame_bit_actual ){

            uint64_t bit_changes_rate;
            // Updating virtual buffer level and it can be negative
            if ((parentpicture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc) &&
                (parentpicture_control_set_ptr->slice_type == I_SLICE) &&
                (rate_control_param_ptr->last_gop == EB_FALSE) &&
                sequence_control_set_ptr->static_config.intra_period != -1){
                rate_control_param_ptr->virtual_buffer_level =
                    (int64_t)rate_control_param_ptr->previous_virtual_buffer_level;
            }
            else{
                rate_control_param_ptr->virtual_buffer_level =
                    (int64_t)rate_control_param_ptr->previous_virtual_buffer_level +
                    (int64_t)previous_frame_bit_actual - (int64_t)rate_control_layer_ptr->channel_bit_rate;
#if !VP9_RC
                context_ptr->extra_bits_gen -= (int64_t)previous_frame_bit_actual - (int64_t)rate_control_layer_ptr->channel_bit_rate;
#endif
            }
#if VP9_RC
            context_ptr->extra_bits_gen -= (int64_t)previous_frame_bit_actual - (int64_t)context_ptr->high_level_rate_control_ptr->channel_bit_rate_per_frame;
#endif
#if RC_NO_EXTRA
            context_ptr->extra_bits_gen = 0;
            rate_control_param_ptr->virtual_buffer_level =
                (int64_t)rate_control_param_ptr->previous_virtual_buffer_level +
                (int64_t)previous_frame_bit_actual - (int64_t)context_ptr->high_level_rate_control_ptr->channel_bit_rate_per_frame;
#endif
            if (parentpicture_control_set_ptr->hierarchical_levels > 1 && rate_control_layer_ptr->frame_same_distortion_min_qp_count > 10){
                rate_control_layer_ptr->previous_bit_constraint = (int64_t)rate_control_layer_ptr->channel_bit_rate;
                rate_control_param_ptr->virtual_buffer_level = ((int64_t)context_ptr->virtual_buffer_size >> 1);
            }
            // Updating bit difference
            rate_control_layer_ptr->bit_diff = (int64_t)rate_control_param_ptr->virtual_buffer_level
                //- ((int64_t)context_ptr->virtual_buffer_size>>1);
                - ((int64_t)rate_control_layer_ptr->channel_bit_rate >> 1);

            // Limit the bit difference
            rate_control_layer_ptr->bit_diff = CLIP3(-(int64_t)(rate_control_layer_ptr->channel_bit_rate), (int64_t)(rate_control_layer_ptr->channel_bit_rate >> 1), rate_control_layer_ptr->bit_diff);
            bit_changes_rate = rate_control_layer_ptr->frame_rate;

            // Updating bit Constraint
            rate_control_layer_ptr->bit_constraint = MAX((int64_t)rate_control_layer_ptr->previous_bit_constraint - ((rate_control_layer_ptr->bit_diff << RC_PRECISION) / ((int64_t)bit_changes_rate)), 1);

            // Limiting the bit_constraint
            if (parentpicture_control_set_ptr->temporal_layer_index == 0){
                rate_control_layer_ptr->bit_constraint = CLIP3(rate_control_layer_ptr->channel_bit_rate >> 2,
                    rate_control_layer_ptr->channel_bit_rate * 200 / 100,
                    rate_control_layer_ptr->bit_constraint);
            }
            else{
                rate_control_layer_ptr->bit_constraint = CLIP3(rate_control_layer_ptr->channel_bit_rate >> 1,
                    rate_control_layer_ptr->channel_bit_rate * 200 / 100,
                    rate_control_layer_ptr->bit_constraint);
            }
            rate_control_layer_ptr->ec_bit_constraint = (uint64_t)MAX((int64_t)rate_control_layer_ptr->bit_constraint - (int64_t)rate_control_layer_ptr->dif_total_and_ec_bits, 1);
            rate_control_param_ptr->previous_virtual_buffer_level = rate_control_param_ptr->virtual_buffer_level;
            rate_control_layer_ptr->previous_bit_constraint = rate_control_layer_ptr->bit_constraint;
        }

        rate_control_param_ptr->processed_frames_number++;
        rate_control_param_ptr->in_use = EB_TRUE;
        // check if all the frames in the interval have arrived
        if (rate_control_param_ptr->processed_frames_number == (rate_control_param_ptr->last_poc - rate_control_param_ptr->first_poc + 1) &&
            sequence_control_set_ptr->intra_period != -1){

            uint32_t temporal_index;
            int64_t extra_bits;
            rate_control_param_ptr->first_poc += PARALLEL_GOP_MAX_NUMBER*(uint32_t)(sequence_control_set_ptr->intra_period + 1);
            rate_control_param_ptr->last_poc += PARALLEL_GOP_MAX_NUMBER*(uint32_t)(sequence_control_set_ptr->intra_period + 1);
            rate_control_param_ptr->processed_frames_number = 0;
            rate_control_param_ptr->extra_ap_bit_ratio_i = 0;
            rate_control_param_ptr->in_use = EB_FALSE;
            rate_control_param_ptr->was_used = EB_TRUE;
            rate_control_param_ptr->last_gop = EB_FALSE;
            rate_control_param_ptr->first_pic_actual_qp_assigned = EB_FALSE;
            for (temporal_index = 0; temporal_index < EB_MAX_TEMPORAL_LAYERS; temporal_index++){
                rate_control_param_ptr->rate_control_layer_array[temporal_index]->first_frame = 1;
                rate_control_param_ptr->rate_control_layer_array[temporal_index]->first_non_intra_frame = 1;
                rate_control_param_ptr->rate_control_layer_array[temporal_index]->feedback_arrived = EB_FALSE;
            }
            extra_bits = ((int64_t)context_ptr->virtual_buffer_size >> 1) - (int64_t)rate_control_param_ptr->virtual_buffer_level;

            rate_control_param_ptr->virtual_buffer_level = context_ptr->virtual_buffer_size >> 1;
            context_ptr->extra_bits += extra_bits;

        }
#if RC_NO_EXTRA
        context_ptr->extra_bits = 0;
#endif

    }

#if VP9_RC_PRINTS
    ////if (parentpicture_control_set_ptr->temporal_layer_index == 0)
            {
            SVT_LOG("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.0f\t%.0f\t%.0f\t%.0f\t%d\t%d\n",
                (int)parentpicture_control_set_ptr->slice_type,
                (int)parentpicture_control_set_ptr->picture_number,
                (int)parentpicture_control_set_ptr->temporal_layer_index,
                (int)parentpicture_control_set_ptr->picture_qp, (int)parentpicture_control_set_ptr->calculated_qp, (int)parentpicture_control_set_ptr->best_pred_qp,
                (int)previous_frame_bit_actual,
                (int)parentpicture_control_set_ptr->target_bits_best_pred_qp,
                (int)parentpicture_control_set_ptr->target_bits_rc,
                (int)rate_control_layer_ptr->channel_bit_rate,
                (int)rate_control_layer_ptr->bit_constraint,
                (double)rate_control_layer_ptr->c_coeff,
                (double)rate_control_layer_ptr->k_coeff,
                (double)parentpicture_control_set_ptr->sad_me,
#if 1 //RC_IMPROVEMENT
                (double)context_ptr->extra_bits_gen,
#else
                (double)rate_control_layer_ptr->previous_frame_distortion_me,
#endif
                (int)rate_control_param_ptr->virtual_buffer_level,
                (int)context_ptr->extra_bits);
            }
#endif

}

void eb_vp9_high_level_rc_feed_back_picture(
    PictureParentControlSet *picture_control_set_ptr,
    SequenceControlSet      *sequence_control_set_ptr)
{

    // Queue variables
    HlRateControlHistogramEntry *hl_rate_control_histogram_ptr_temp;
    uint32_t                     queue_entry_index_head_temp;

    //SVT_LOG("\nOut:%d Slidings: ",picture_control_set_ptr->picture_number);
    if (sequence_control_set_ptr->look_ahead_distance != 0){

        // Update the coded rate in the histogram queue
        if (picture_control_set_ptr->picture_number >= sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue[sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number){
            queue_entry_index_head_temp = (int32_t)(picture_control_set_ptr->picture_number - sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue[sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
            queue_entry_index_head_temp += sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue_head_index;
            queue_entry_index_head_temp = (queue_entry_index_head_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                queue_entry_index_head_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                queue_entry_index_head_temp;

            hl_rate_control_histogram_ptr_temp = (sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_head_temp]);
            if (hl_rate_control_histogram_ptr_temp->picture_number == picture_control_set_ptr->picture_number &&
                hl_rate_control_histogram_ptr_temp->passed_to_hlrc){
                eb_vp9_block_on_mutex(sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue_mutex);
                hl_rate_control_histogram_ptr_temp->total_num_bitsCoded = picture_control_set_ptr->total_num_bits;
                hl_rate_control_histogram_ptr_temp->is_coded = EB_TRUE;
                eb_vp9_release_mutex(sequence_control_set_ptr->encode_context_ptr->hl_rate_control_historgram_queue_mutex);
            }
        }

    }
}
// rate control QP refinement
void eb_vp9_rate_control_refinement(
    PictureControlSet               *picture_control_set_ptr,
    SequenceControlSet              *sequence_control_set_ptr,
    RateControlIntervalParamContext *rate_control_param_ptr,
    RateControlIntervalParamContext *prev_gop_rate_control_param_ptr,
    RateControlIntervalParamContext *next_gop_rate_control_param_ptr) {

    if (picture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc && picture_control_set_ptr->picture_number != 0 && !prev_gop_rate_control_param_ptr->scene_change_in_gop) {
        int16_t deltaApQp = (int16_t)prev_gop_rate_control_param_ptr->first_pic_actual_qp - (int16_t)prev_gop_rate_control_param_ptr->first_pic_pred_qp;
        int64_t extraApBitRatio = (prev_gop_rate_control_param_ptr->first_pic_pred_bits != 0) ?
            (((int64_t)prev_gop_rate_control_param_ptr->first_pic_actual_bits - (int64_t)prev_gop_rate_control_param_ptr->first_pic_pred_bits) * 100) / ((int64_t)prev_gop_rate_control_param_ptr->first_pic_pred_bits) :
            0;
        extraApBitRatio += (int64_t)deltaApQp * 15;
        if (extraApBitRatio > 200) {
            picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + 3;
        }
        else if (extraApBitRatio > 100) {
            picture_control_set_ptr->picture_qp = picture_control_set_ptr->picture_qp + 2;
        }
        else if (extraApBitRatio > 50) {
            picture_control_set_ptr->picture_qp++;
        }
    }

    if (picture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc && picture_control_set_ptr->picture_number != 0) {
        uint8_t qpIncAllowed = 3;
        uint8_t qpDecAllowed = 4;
        if (picture_control_set_ptr->parent_pcs_ptr->intra_selected_org_qp + 10 <= prev_gop_rate_control_param_ptr->first_pic_actual_qp)
        {
            qpDecAllowed = (uint8_t)(prev_gop_rate_control_param_ptr->first_pic_actual_qp - picture_control_set_ptr->parent_pcs_ptr->intra_selected_org_qp) >> 1;
        }

        if (picture_control_set_ptr->parent_pcs_ptr->intra_selected_org_qp >= prev_gop_rate_control_param_ptr->first_pic_actual_qp + 10)
        {
            qpIncAllowed = (uint8_t)(picture_control_set_ptr->parent_pcs_ptr->intra_selected_org_qp - prev_gop_rate_control_param_ptr->first_pic_actual_qp) * 2 / 3;
            if (prev_gop_rate_control_param_ptr->first_pic_actual_qp <= 15)
                qpIncAllowed += 5;
            else if (prev_gop_rate_control_param_ptr->first_pic_actual_qp <= 20)
                qpIncAllowed += 4;
            else if (prev_gop_rate_control_param_ptr->first_pic_actual_qp <= 25)
                qpIncAllowed += 3;
        }
        else if (prev_gop_rate_control_param_ptr->scene_change_in_gop) {
            qpIncAllowed = 5;
        }
        if (picture_control_set_ptr->parent_pcs_ptr->end_of_sequence_region) {
            qpIncAllowed += 2;
            qpDecAllowed += 4;
        }
        picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
            (uint32_t)MAX((int32_t)prev_gop_rate_control_param_ptr->first_pic_actual_qp - (int32_t)qpDecAllowed, 0),
            (uint32_t)prev_gop_rate_control_param_ptr->first_pic_actual_qp + qpIncAllowed,
            picture_control_set_ptr->picture_qp);
    }

    // Scene change
    if (picture_control_set_ptr->slice_type == I_SLICE && picture_control_set_ptr->picture_number != rate_control_param_ptr->first_poc) {
        if (next_gop_rate_control_param_ptr->first_pic_actual_qp_assigned) {

            picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                (uint32_t)MAX((int32_t)next_gop_rate_control_param_ptr->first_pic_actual_qp - (int32_t)1, 0),
                (uint32_t)next_gop_rate_control_param_ptr->first_pic_actual_qp + 8,
                picture_control_set_ptr->picture_qp);
        }
        else {
            if (rate_control_param_ptr->first_pic_actual_qp < 20) {
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)MAX((int32_t)rate_control_param_ptr->first_pic_actual_qp - (int32_t)4, 0),
                    (uint32_t)rate_control_param_ptr->first_pic_actual_qp + 10,
                    picture_control_set_ptr->picture_qp);
            }
            else {
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    (uint32_t)MAX((int32_t)rate_control_param_ptr->first_pic_actual_qp - (int32_t)4, 0),
                    (uint32_t)rate_control_param_ptr->first_pic_actual_qp + 8,
                    picture_control_set_ptr->picture_qp);

            }

        }
    }

    if (sequence_control_set_ptr->intra_period != -1 && picture_control_set_ptr->parent_pcs_ptr->hierarchical_levels < 2 && (int32_t)picture_control_set_ptr->temporal_layer_index == 0 && picture_control_set_ptr->slice_type != I_SLICE) {
        if (next_gop_rate_control_param_ptr->first_pic_actual_qp_assigned || next_gop_rate_control_param_ptr->was_used) {

            picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                (uint32_t)MAX((int32_t)next_gop_rate_control_param_ptr->first_pic_actual_qp - (int32_t)4, 0),
                (uint32_t)picture_control_set_ptr->picture_qp,
                picture_control_set_ptr->picture_qp);
        }
        else {
            picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                (uint32_t)MAX((int32_t)rate_control_param_ptr->first_pic_actual_qp - (int32_t)4, 0),
                (uint32_t)picture_control_set_ptr->picture_qp,
                picture_control_set_ptr->picture_qp);
        }
    }
}
// initialize the rate control parameter at the beginning
void eb_vp9_init_rc(
    RateControlContext *context_ptr,
    PictureControlSet  *picture_control_set_ptr,
    SequenceControlSet *sequence_control_set_ptr) {

    context_ptr->high_level_rate_control_ptr->target_bit_rate = sequence_control_set_ptr->static_config.target_bit_rate;
    context_ptr->high_level_rate_control_ptr->frame_rate = sequence_control_set_ptr->frame_rate;
    context_ptr->high_level_rate_control_ptr->channel_bit_rate_per_frame = (uint64_t)MAX((int64_t)1, (int64_t)((context_ptr->high_level_rate_control_ptr->target_bit_rate << RC_PRECISION) / context_ptr->high_level_rate_control_ptr->frame_rate));

    context_ptr->high_level_rate_control_ptr->channel_bit_rate_per_sw = context_ptr->high_level_rate_control_ptr->channel_bit_rate_per_frame * (sequence_control_set_ptr->look_ahead_distance + 1);
    context_ptr->high_level_rate_control_ptr->bit_constraint_per_sw = context_ptr->high_level_rate_control_ptr->channel_bit_rate_per_sw;

#if RC_UPDATE_TARGET_RATE
    context_ptr->high_level_rate_control_ptr->previous_updated_bit_constraint_per_sw = context_ptr->high_level_rate_control_ptr->channel_bit_rate_per_sw;
#endif

    int32_t total_frame_in_interval = sequence_control_set_ptr->intra_period;
    uint32_t gopPeriod = (1 << picture_control_set_ptr->parent_pcs_ptr->hierarchical_levels);
    context_ptr->frame_rate = sequence_control_set_ptr->frame_rate;
    while (total_frame_in_interval >= 0) {
        if (total_frame_in_interval % (gopPeriod) == 0)
            context_ptr->frames_in_interval[0] ++;
        else if (total_frame_in_interval % (gopPeriod >> 1) == 0)
            context_ptr->frames_in_interval[1] ++;
        else if (total_frame_in_interval % (gopPeriod >> 2) == 0)
            context_ptr->frames_in_interval[2] ++;
        else if (total_frame_in_interval % (gopPeriod >> 3) == 0)
            context_ptr->frames_in_interval[3] ++;
        else if (total_frame_in_interval % (gopPeriod >> 4) == 0)
            context_ptr->frames_in_interval[4] ++;
        else if (total_frame_in_interval % (gopPeriod >> 5) == 0)
            context_ptr->frames_in_interval[5] ++;
        total_frame_in_interval--;
    }
    if (sequence_control_set_ptr->static_config.rate_control_mode == 1) { // VBR
        context_ptr->virtual_buffer_size = (((uint64_t)sequence_control_set_ptr->static_config.target_bit_rate * 3) << RC_PRECISION) / (context_ptr->frame_rate);
        context_ptr->rate_average_periodin_frames = (uint64_t)sequence_control_set_ptr->static_config.intra_period + 1;
        context_ptr->virtual_buffer_level_initial_value = context_ptr->virtual_buffer_size >> 1;
        context_ptr->virtual_buffer_level = context_ptr->virtual_buffer_size >> 1;
        context_ptr->previous_virtual_buffer_level = context_ptr->virtual_buffer_size >> 1;
        context_ptr->vb_fill_threshold1 = (context_ptr->virtual_buffer_size * 6) >> 3;
        context_ptr->vb_fill_threshold2 = (context_ptr->virtual_buffer_size << 3) >> 3;
        context_ptr->base_layer_frames_avg_qp = sequence_control_set_ptr->qp;
        context_ptr->base_layer_intra_frames_avg_qp = sequence_control_set_ptr->qp;
    }
    else { // CBR
        context_ptr->virtual_buffer_size = ((uint64_t)sequence_control_set_ptr->static_config.target_bit_rate);// vbv_buf_size);
        context_ptr->rate_average_periodin_frames = (uint64_t)sequence_control_set_ptr->static_config.intra_period + 1;
        context_ptr->virtual_buffer_level_initial_value = context_ptr->virtual_buffer_size >> 1;
        context_ptr->virtual_buffer_level = context_ptr->virtual_buffer_size >> 1;
        context_ptr->previous_virtual_buffer_level = context_ptr->virtual_buffer_size >> 1;
        context_ptr->vb_fill_threshold1 = context_ptr->virtual_buffer_level_initial_value + (context_ptr->virtual_buffer_size / 4);
        context_ptr->vb_fill_threshold2 = context_ptr->virtual_buffer_level_initial_value + (context_ptr->virtual_buffer_size / 3);
        context_ptr->base_layer_frames_avg_qp = sequence_control_set_ptr->qp;
        context_ptr->base_layer_intra_frames_avg_qp = sequence_control_set_ptr->qp;

    }

    for (uint32_t base_qp = 0; base_qp < MAX_REF_QP_NUM; base_qp++) {
        if (base_qp < 64) {
            context_ptr->qp_scaling_map_I_SLICE[base_qp] = eb_vp9_qp_scaling_calc(
                sequence_control_set_ptr,
                I_SLICE,
                0,
                base_qp);
        }
        else {
            context_ptr->qp_scaling_map_I_SLICE[base_qp] = (uint32_t)CLIP3(0, 63, (int)base_qp - (63 - (int)context_ptr->qp_scaling_map_I_SLICE[63]));

        }

        for (uint32_t temporal_layer_index = 0; temporal_layer_index < 4; temporal_layer_index++) {
            if (base_qp < 64) {
                context_ptr->qp_scaling_map[temporal_layer_index][base_qp] = eb_vp9_qp_scaling_calc(
                    sequence_control_set_ptr,
                    0,
                    temporal_layer_index,
                    base_qp);
            }
            else {
                context_ptr->qp_scaling_map[temporal_layer_index][base_qp] = (uint32_t)CLIP3(0, 63, (int)base_qp - (63 - (int)context_ptr->qp_scaling_map[temporal_layer_index][63]));
            }
        }
    }
}

uint64_t predictBits(
    SequenceControlSet *sequence_control_set_ptr,
    EncodeContext *encode_context_ptr,
    HlRateControlHistogramEntry *hlRateControl_histogram_ptr_temp,
    uint32_t qp) {

    uint64_t                          total_bits = 0;

    if (hlRateControl_histogram_ptr_temp->is_coded) {
        // If the frame is already coded, use the actual number of bits
        total_bits = hlRateControl_histogram_ptr_temp->total_num_bitsCoded;
    }
    else {
        RateControlTables *rate_control_tables_ptr = &encode_context_ptr->rate_control_tables_array[qp];
        EbBitNumber *sad_bits_array_ptr = rate_control_tables_ptr->sad_bits_array[hlRateControl_histogram_ptr_temp->temporal_layer_index];
        EbBitNumber *intra_sad_bits_array_ptr = rate_control_tables_ptr->intra_sad_bits_array[0];
        uint32_t pred_bits_ref_qp = 0;
        uint32_t num_of_full_lcus = 0;
        uint32_t area_in_pixel = sequence_control_set_ptr->luma_width * sequence_control_set_ptr->luma_height;

        if (hlRateControl_histogram_ptr_temp->slice_type == EB_I_PICTURE) {
            // Loop over block in the frame and calculated the predicted bits at reg QP
            uint32_t i;
            int32_t accum = 0;
            for (i = 0; i < NUMBER_OF_INTRA_SAD_INTERVALS; ++i)
            {
            accum += (uint32_t)(hlRateControl_histogram_ptr_temp->ois_distortion_histogram[i] * intra_sad_bits_array_ptr[i]);
            }

            pred_bits_ref_qp = accum;
            num_of_full_lcus = hlRateControl_histogram_ptr_temp->full_sb_count;
            total_bits += pred_bits_ref_qp;
        }
        else {
            uint32_t i;
            uint32_t accum = 0;
            uint32_t accum_intra = 0;
            for (i = 0; i < NUMBER_OF_SAD_INTERVALS; ++i)
            {
            accum += (uint32_t)(hlRateControl_histogram_ptr_temp->me_distortion_histogram[i] * sad_bits_array_ptr[i]);
            accum_intra += (uint32_t)(hlRateControl_histogram_ptr_temp->ois_distortion_histogram[i] * intra_sad_bits_array_ptr[i]);
            }
            if (accum > accum_intra * 3)
                pred_bits_ref_qp = accum_intra;
            else
                pred_bits_ref_qp = accum;
            num_of_full_lcus = hlRateControl_histogram_ptr_temp->full_sb_count;
            total_bits += pred_bits_ref_qp;
        }

        // Scale for in complete LCSs
        // pred_bits_ref_qp is normalized based on the area because of the LCUs at the picture boundries
    total_bits = total_bits * (uint64_t)area_in_pixel / (num_of_full_lcus << 12);
    }
    hlRateControl_histogram_ptr_temp->pred_bits_ref_qp[qp] = total_bits;
    return total_bits;
}

uint8_t Vbv_Buf_Calc(PictureControlSet *picture_control_set_ptr,
    SequenceControlSet *sequence_control_set_ptr,
    EncodeContext *encode_context_ptr) {

    int32_t                           loop_terminate = 0;
    uint32_t                          q = picture_control_set_ptr->picture_qp;
    uint32_t                          q0 = picture_control_set_ptr->picture_qp;
    // Queue variables
    uint32_t                          queue_entry_index_temp;
    uint32_t                          queue_entry_index_temp2;
    uint32_t                          queue_entry_index_head_temp;
    HlRateControlHistogramEntry       *hlRateControl_histogram_ptr_temp;
    EB_BOOL                              bitrate_flag;

    /* Lookahead VBV: If lookahead is done, raise the quantizer as necessary
    * such that no frames in the lookahead overflow and such that the buffer
    * is in a reasonable state by the end of the lookahead. */

    queue_entry_index_head_temp = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->hl_rate_control_historgram_queue[encode_context_ptr->hl_rate_control_historgram_queue_head_index]->picture_number);
    queue_entry_index_head_temp += encode_context_ptr->hl_rate_control_historgram_queue_head_index;
    queue_entry_index_head_temp = (queue_entry_index_head_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
        queue_entry_index_head_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
        queue_entry_index_head_temp;

    queue_entry_index_temp = queue_entry_index_head_temp;
    bitrate_flag = encode_context_ptr->vbv_max_rate <= encode_context_ptr->available_target_bitrate;
    int32_t current_ind = (queue_entry_index_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH : queue_entry_index_temp;

    /* Avoid an infinite loop. */
    for (int32_t iterations = 0; iterations < 1000 && loop_terminate != 3; iterations++)
    {
        hlRateControl_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[current_ind]);
        double cur_bits = (double)predictBits(sequence_control_set_ptr, encode_context_ptr, hlRateControl_histogram_ptr_temp, q);
        double buffer_fill_cur = encode_context_ptr->buffer_fill - cur_bits;
        double target_fill;
        double fps = 1.0 / (sequence_control_set_ptr->frame_rate >> RC_PRECISION);
        double total_duration = fps;
        queue_entry_index_temp = current_ind;

        /* Loop over the planned future frames. */
        for (int32_t j = 0; buffer_fill_cur >= 0; j++)
        {
            queue_entry_index_temp2 = (queue_entry_index_temp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH : queue_entry_index_temp;
            hlRateControl_histogram_ptr_temp = (encode_context_ptr->hl_rate_control_historgram_queue[queue_entry_index_temp2]);

            if ((queue_entry_index_temp >= (current_ind + sequence_control_set_ptr->look_ahead_distance)) ||
                (queue_entry_index_temp >= sequence_control_set_ptr->static_config.frames_to_be_encoded)
                || (total_duration >= 1.0))
            break;
            total_duration += fps;
            double wanted_frame_size = encode_context_ptr->vbv_max_rate * fps;
            if (buffer_fill_cur + wanted_frame_size <= encode_context_ptr->vbv_buf_size)
                buffer_fill_cur += wanted_frame_size;
            cur_bits = (double)predictBits(sequence_control_set_ptr, encode_context_ptr, hlRateControl_histogram_ptr_temp, q);
            buffer_fill_cur -= cur_bits;
            queue_entry_index_temp++;
        }
        /* Try to get the buffer at least 50% filled, but don't set an impossible goal. */

        target_fill = MIN(encode_context_ptr->buffer_fill + total_duration * encode_context_ptr->vbv_max_rate * 0.5, encode_context_ptr->vbv_buf_size * (1 - 0.5));
        if (buffer_fill_cur < target_fill)
        {
            q++;
            q = CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                sequence_control_set_ptr->static_config.max_qp_allowed,
                q);
            loop_terminate |= 1;
            continue;
        }
        /* Try to get the buffer not more than 80% filled, but don't set an impossible goal. */
        target_fill = CLIP3(encode_context_ptr->vbv_buf_size * (1 - 0.05), encode_context_ptr->vbv_buf_size, encode_context_ptr->buffer_fill - total_duration * encode_context_ptr->vbv_max_rate * 0.5);
        if ((bitrate_flag) && (buffer_fill_cur > target_fill))
        {
            q--;
            q = CLIP3(
                sequence_control_set_ptr->static_config.min_qp_allowed,
                sequence_control_set_ptr->static_config.max_qp_allowed,
                q);
            loop_terminate |= 2;
            continue;
        }
        break;
    }
    q = MAX(q0 / 2, q);
    return (uint8_t)q;
}

void* eb_vp9_rate_control_kernel(void *input_ptr)
{
    // Context
    RateControlContext                *context_ptr = (RateControlContext  *)input_ptr;
    EncodeContext                     *encode_context_ptr;

    RateControlIntervalParamContext   *rate_control_param_ptr;

    RateControlIntervalParamContext   *prev_gop_rate_control_param_ptr;
    RateControlIntervalParamContext   *next_gop_rate_control_param_ptr;

    PictureControlSet                 *picture_control_set_ptr;
    PictureParentControlSet           *parentpicture_control_set_ptr;

    // Config
    SequenceControlSet                *sequence_control_set_ptr;

    // Input
    EbObjectWrapper                   *rate_control_tasks_wrapper_ptr;
    RateControlTasks                  *rate_control_tasks_ptr;

    // Output
    EbObjectWrapper                   *rate_control_results_wrapper_ptr;
    RateControlResults                *rate_control_results_ptr;

    RateControlLayerContext           *rate_control_layer_ptr;

    uint64_t                           total_number_of_fb_frames = 0;

    RateControlTaskTypes               task_type;

    for (;;) {

        // Get RateControl Task
        eb_vp9_get_full_object(
            context_ptr->rate_control_input_tasks_fifo_ptr,
            &rate_control_tasks_wrapper_ptr);

        rate_control_tasks_ptr = (RateControlTasks*)rate_control_tasks_wrapper_ptr->object_ptr;
        task_type = rate_control_tasks_ptr->task_type;

        // Modify these for different temporal layers later
        switch (task_type){

        case RC_PICTURE_MANAGER_RESULT:

            picture_control_set_ptr = (PictureControlSet  *)rate_control_tasks_ptr->picture_control_set_wrapper_ptr->object_ptr;
            sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
            encode_context_ptr = (EncodeContext*)sequence_control_set_ptr->encode_context_ptr;
            if (sequence_control_set_ptr->static_config.rate_control_mode){

                if (picture_control_set_ptr->picture_number == 0) {
                    //init rate control parameters
                    eb_vp9_init_rc(
                        context_ptr,
                        picture_control_set_ptr,
                        sequence_control_set_ptr);
                    encode_context_ptr->buffer_fill = (uint64_t)(sequence_control_set_ptr->static_config.vbv_buf_size * 0.9);
                    encode_context_ptr->vbv_max_rate = sequence_control_set_ptr->static_config.vbv_max_rate;
                    encode_context_ptr->vbv_buf_size = sequence_control_set_ptr->static_config.vbv_buf_size;
                }

                picture_control_set_ptr->parent_pcs_ptr->intra_selected_org_qp = 0;
                // High level RC
                if(sequence_control_set_ptr->static_config.rate_control_mode == 1)
                    eb_vp9_high_level_rc_input_picture_vbr(
                        picture_control_set_ptr->parent_pcs_ptr,
                        sequence_control_set_ptr,
                        sequence_control_set_ptr->encode_context_ptr,
                        context_ptr,
                        context_ptr->high_level_rate_control_ptr);
                else
                    high_level_rc_input_picture_cbr(
                        picture_control_set_ptr->parent_pcs_ptr,
                        sequence_control_set_ptr,
                        sequence_control_set_ptr->encode_context_ptr,
                        context_ptr,
                        context_ptr->high_level_rate_control_ptr);
            }

            // Frame level RC. Find the ParamPtr for the current GOP
            if (sequence_control_set_ptr->intra_period == -1 || sequence_control_set_ptr->static_config.rate_control_mode == 0){
                rate_control_param_ptr = context_ptr->rate_control_param_queue[0];
                prev_gop_rate_control_param_ptr = context_ptr->rate_control_param_queue[0];
                next_gop_rate_control_param_ptr = context_ptr->rate_control_param_queue[0];
            }
            else{
                uint32_t interval_index_temp = 0;
                EB_BOOL intervalFound = EB_FALSE;
                while ((interval_index_temp < PARALLEL_GOP_MAX_NUMBER) && !intervalFound){

                    if (picture_control_set_ptr->picture_number >= context_ptr->rate_control_param_queue[interval_index_temp]->first_poc &&
                        picture_control_set_ptr->picture_number <= context_ptr->rate_control_param_queue[interval_index_temp]->last_poc) {
                        intervalFound = EB_TRUE;
                    }
                    else{
                        interval_index_temp++;
                    }
                }
                CHECK_REPORT_ERROR(
                    interval_index_temp != PARALLEL_GOP_MAX_NUMBER,
                    sequence_control_set_ptr->encode_context_ptr->app_callback_ptr,
                    EB_ENC_RC_ERROR0);

                rate_control_param_ptr = context_ptr->rate_control_param_queue[interval_index_temp];

                prev_gop_rate_control_param_ptr = (interval_index_temp == 0) ?
                    context_ptr->rate_control_param_queue[PARALLEL_GOP_MAX_NUMBER - 1] :
                    context_ptr->rate_control_param_queue[interval_index_temp - 1];
                next_gop_rate_control_param_ptr = (interval_index_temp == PARALLEL_GOP_MAX_NUMBER-1) ?
                    context_ptr->rate_control_param_queue[0] :
                    context_ptr->rate_control_param_queue[interval_index_temp + 1];
            }

            rate_control_layer_ptr = rate_control_param_ptr->rate_control_layer_array[picture_control_set_ptr->temporal_layer_index];

            if (sequence_control_set_ptr->static_config.rate_control_mode == 0){
                // if RC mode is 0,  fixed QP is used
                // QP scaling based on POC number for Flat IPPP structure

                if (sequence_control_set_ptr->enable_qp_scaling_flag && picture_control_set_ptr->parent_pcs_ptr->qp_on_the_fly == EB_FALSE) {

                    int qindex = eb_vp9_quantizer_to_qindex(sequence_control_set_ptr->qp);
                    RATE_CONTROL rc;
                    rc.worst_quality = MAXQ;
                    rc.best_quality  = MINQ;

                    if (picture_control_set_ptr->parent_pcs_ptr->qp_scaling_mode == QP_SCALING_MODE_1) {

                        VP9_COMP   *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
                        VP9_COMMON *const cm = &cpi->common;
                        RATE_FACTOR_LEVEL rate_factor_level;
                        int active_best_quality;
                        int active_worst_quality = qindex;

                        if (picture_control_set_ptr->slice_type == I_SLICE) {

                            rate_factor_level = KF_STD;

                            cpi->rc.worst_quality = MAXQ;
                            cpi->rc.best_quality = MINQ;

                            // Hsan: cross multiplication to derive kf_boost from non_moving_average_score; kf_boost range is [kf_low,kf_high], and non_moving_average_score range [NON_MOVING_SCORE_0,NON_MOVING_SCORE_3]
                            cpi->rc.kf_boost = (((NON_MOVING_SCORE_3 - picture_control_set_ptr->parent_pcs_ptr->non_moving_average_score)  * (kf_high - kf_low)) / NON_MOVING_SCORE_3) + kf_low;

                            double q_adj_factor = 1.0;
                            double q_val;

                            // Baseline value derived from cpi->active_worst_quality and kf boost.
                            active_best_quality = get_kf_active_quality(&cpi->rc, active_worst_quality, cm->bit_depth);

                            // Threshold used to define a KF group as static (e.g. a slide show).
                            // Essentially this means that no frame in the group has more than 1% of MBs
                            // that are not marked as coded with 0,0 motion in the first pass.
                            if (picture_control_set_ptr->parent_pcs_ptr->kf_zeromotion_pct >= STATIC_KF_GROUP_THRESH) {
                                active_best_quality /= 4;
                            }

                            // Dont allow the active min to be lossless (q0) unlesss the max q
                            // already indicates lossless.
                            active_best_quality =
                                VPXMIN(active_worst_quality, VPXMAX(1, active_best_quality));

                            // Allow somewhat lower kf minq with small image formats.
                            if ((cm->width * cm->height) <= (352 * 288)) {
                                q_adj_factor -= 0.25;
                            }

                            // Make a further adjustment based on the kf zero motion measure.
                            q_adj_factor += 0.05 - (0.001 * (double)picture_control_set_ptr->parent_pcs_ptr->kf_zeromotion_pct);

                            // Convert the adjustment factor to a qindex delta
                            // on active_best_quality.
                            q_val = eb_vp9_convert_qindex_to_q(active_best_quality, cm->bit_depth);
                            active_best_quality += eb_vp9_compute_qdelta(&rc, q_val, q_val * q_adj_factor, cm->bit_depth);
                        }
                        else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag) {
                            rate_factor_level = GF_ARF_STD;
                            // Hsan: cross multiplication to derive kf_boost from non_moving_average_score; kf_boost range is [gf_low,gf_high], and non_moving_average_score range [NON_MOVING_SCORE_0,NON_MOVING_SCORE_3]
                            cpi->rc.gfu_boost = (((NON_MOVING_SCORE_3 - picture_control_set_ptr->parent_pcs_ptr->non_moving_average_score)  * (gf_high - gf_low)) / NON_MOVING_SCORE_3) + gf_low;
                            active_best_quality = get_gf_active_quality(cpi, active_worst_quality, cm->bit_depth);
#if 0
                            // Modify best quality for second level arfs. For mode VPX_Q this
                            // becomes the baseline frame q.
                            if (gf_group->rf_level[gf_group_index] == GF_ARF_LOW)
                                active_best_quality = (active_best_quality + cq_level + 1) / 2;
#endif
                        }
                        else {
                            rate_factor_level = INTER_NORMAL;
                            active_best_quality = qindex;
                        }

                        // Static forced key frames Q restrictions dealt with elsewhere.
#if 0
                        if (!frame_is_intra_only(cm) || !rc->this_key_frame_forced ||
                            cpi->twopass.last_kfgroup_zeromotion_pct < STATIC_MOTION_THRESH)
#endif
                        {
                            int qdelta = eb_vp9_frame_type_qdelta(cpi, rate_factor_level,
                                active_worst_quality);
                            active_worst_quality =
                                VPXMAX(active_worst_quality + qdelta, active_best_quality);
                        }

                        active_best_quality =
                            clamp(active_best_quality, rc.best_quality, rc.worst_quality);
                        active_worst_quality =
                            clamp(active_worst_quality, active_best_quality, rc.worst_quality);

                        int q;
                        q = active_best_quality;
                        clamp(q, active_best_quality, active_worst_quality);
                        picture_control_set_ptr->parent_pcs_ptr->cpi->common.base_qindex = picture_control_set_ptr->base_qindex = q;

                    }
                    else {
                        const double q = eb_vp9_convert_qindex_to_q(qindex, (vpx_bit_depth_t)sequence_control_set_ptr->static_config.encoder_bit_depth);
                        int delta_qindex;

                        if (picture_control_set_ptr->slice_type == I_SLICE) {

                            delta_qindex = eb_vp9_compute_qdelta(
                                &rc,
                                q,
                                q* 0.25,
                                (vpx_bit_depth_t)sequence_control_set_ptr->static_config.encoder_bit_depth);

                            picture_control_set_ptr->parent_pcs_ptr->cpi->common.base_qindex = picture_control_set_ptr->base_qindex = VPXMAX(qindex + delta_qindex, rc.best_quality);

                        }
                        else {
                            if (sequence_control_set_ptr->static_config.tune == TUNE_OQ) {
                                delta_qindex = eb_vp9_compute_qdelta(
                                    &rc,
                                    q,
#if NEW_PRED_STRUCT
                                    q* delta_rate_oq[picture_control_set_ptr->parent_pcs_ptr->hierarchical_levels == 4][picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index],
#else
                                    q* delta_rate_oq[picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index],
#endif
                                    (vpx_bit_depth_t)sequence_control_set_ptr->static_config.encoder_bit_depth);
                            }
                            else if (sequence_control_set_ptr->static_config.tune == TUNE_SQ) {
                                delta_qindex = eb_vp9_compute_qdelta(
                                    &rc,
                                    q,
                                    q* delta_rate_sq[picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index],
                                    (vpx_bit_depth_t)sequence_control_set_ptr->static_config.encoder_bit_depth);
                            }
                            else {
                                delta_qindex = eb_vp9_compute_qdelta(
                                    &rc,
                                    q,
                                    q* delta_rate_vmaf[picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index],
                                    (vpx_bit_depth_t)sequence_control_set_ptr->static_config.encoder_bit_depth);
                            }

                            picture_control_set_ptr->parent_pcs_ptr->cpi->common.base_qindex = picture_control_set_ptr->base_qindex = VPXMAX(qindex + delta_qindex, rc.best_quality);

                        }
                    }
                }
                else if (picture_control_set_ptr->parent_pcs_ptr->qp_on_the_fly == EB_TRUE){
                    picture_control_set_ptr->picture_qp = (uint8_t)CLIP3((int32_t)sequence_control_set_ptr->static_config.min_qp_allowed, (int32_t)sequence_control_set_ptr->static_config.max_qp_allowed,picture_control_set_ptr->parent_pcs_ptr->picture_qp);
                    picture_control_set_ptr->parent_pcs_ptr->cpi->common.base_qindex = picture_control_set_ptr->base_qindex = eb_vp9_quantizer_to_qindex(picture_control_set_ptr->picture_qp);
                }

            }
            else{

                // ***Rate Control***
                if (sequence_control_set_ptr->static_config.rate_control_mode == 1) {
                    eb_vp9_frame_level_rc_input_picture_vbr(
                        picture_control_set_ptr,
                        sequence_control_set_ptr,
                        context_ptr,
                        rate_control_layer_ptr,
                        rate_control_param_ptr);

                    // rate control QP refinement
                    eb_vp9_rate_control_refinement(
                        picture_control_set_ptr,
                        sequence_control_set_ptr,
                        rate_control_param_ptr,
                        prev_gop_rate_control_param_ptr,
                        next_gop_rate_control_param_ptr);
                }
                else {

                    eb_vp9_frame_level_rc_input_picture_cbr(
                        picture_control_set_ptr,
                        sequence_control_set_ptr,
                        context_ptr,
                        rate_control_layer_ptr,
                        rate_control_param_ptr);

                }
                picture_control_set_ptr->picture_qp = (uint8_t)CLIP3(
                    sequence_control_set_ptr->static_config.min_qp_allowed,
                    sequence_control_set_ptr->static_config.max_qp_allowed,
                    picture_control_set_ptr->picture_qp);

                picture_control_set_ptr->parent_pcs_ptr->cpi->common.base_qindex = picture_control_set_ptr->base_qindex = eb_vp9_quantizer_to_qindex(picture_control_set_ptr->picture_qp);
            }
            if (encode_context_ptr->vbv_buf_size && encode_context_ptr->vbv_max_rate)
            {
                eb_vp9_block_on_mutex(encode_context_ptr->sc_buffer_mutex);
                picture_control_set_ptr->picture_qp = (uint8_t)Vbv_Buf_Calc(picture_control_set_ptr, sequence_control_set_ptr, encode_context_ptr);
                eb_vp9_release_mutex(encode_context_ptr->sc_buffer_mutex);
            }
            picture_control_set_ptr->parent_pcs_ptr->picture_qp = picture_control_set_ptr->picture_qp;

            if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0 && sequence_control_set_ptr->look_ahead_distance != 0){
                context_ptr->base_layer_frames_avg_qp = (3 * context_ptr->base_layer_frames_avg_qp + picture_control_set_ptr->picture_qp + 2) >> 2;
            }
            if (picture_control_set_ptr->slice_type == I_SLICE){
                if (picture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc){
                    rate_control_param_ptr->first_pic_pred_qp = (uint16_t) picture_control_set_ptr->parent_pcs_ptr->best_pred_qp;
                    rate_control_param_ptr->first_pic_actual_qp = (uint16_t) picture_control_set_ptr->picture_qp;
                    rate_control_param_ptr->scene_change_in_gop = picture_control_set_ptr->parent_pcs_ptr->scene_change_in_gop;
                    rate_control_param_ptr->first_pic_actual_qp_assigned = EB_TRUE;
                }
                {
                    if (picture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc){
                        if (sequence_control_set_ptr->look_ahead_distance != 0){
                            context_ptr->base_layer_intra_frames_avg_qp = (3 * context_ptr->base_layer_intra_frames_avg_qp + picture_control_set_ptr->picture_qp + 2) >> 2;
                        }
                    }

                    if (picture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc){
                        rate_control_param_ptr->intra_frames_qp = picture_control_set_ptr->picture_qp;
                        rate_control_param_ptr->next_gop_intra_frame_qp = picture_control_set_ptr->picture_qp;
                        rate_control_param_ptr->intra_frames_qp_bef_scal = (uint8_t)sequence_control_set_ptr->static_config.max_qp_allowed;
                        for (uint32_t qindex = sequence_control_set_ptr->static_config.min_qp_allowed; qindex <= sequence_control_set_ptr->static_config.max_qp_allowed; qindex++) {
                            if (rate_control_param_ptr->intra_frames_qp <= context_ptr->qp_scaling_map_I_SLICE[qindex]) {
                                rate_control_param_ptr->intra_frames_qp_bef_scal = (uint8_t)qindex;
                                break;
                            }
                        }

                    }
                }
            }

            // Get Empty Rate Control Results Buffer
            eb_vp9_get_empty_object(
                context_ptr->rate_control_output_results_fifo_ptr,
                &rate_control_results_wrapper_ptr);
            rate_control_results_ptr = (RateControlResults*)rate_control_results_wrapper_ptr->object_ptr;
            rate_control_results_ptr->picture_control_set_wrapper_ptr = rate_control_tasks_ptr->picture_control_set_wrapper_ptr;

            // Post Full Rate Control Results
            eb_vp9_post_full_object(rate_control_results_wrapper_ptr);

            // Release Rate Control Tasks
            eb_vp9_release_object(rate_control_tasks_wrapper_ptr);

            break;

        case RC_PACKETIZATION_FEEDBACK_RESULT:
            //loop_count++;
            //SVT_LOG("Rate Control Thread FeedBack %d\n", (int) loop_count);

            parentpicture_control_set_ptr = (PictureParentControlSet  *)rate_control_tasks_ptr->picture_control_set_wrapper_ptr->object_ptr;
            sequence_control_set_ptr = (SequenceControlSet *)parentpicture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;

            // Update feedback arrived in referencepictureQueue
                     //  SVT_LOG("RC FEEDBACK ARRIVED %d\n", (int)parentpicture_control_set_ptr->picture_number);
            if (sequence_control_set_ptr->static_config.rate_control_mode) {
                ReferenceQueueEntry           *reference_entry_ptr;
                uint32_t                          reference_queue_index;
                EncodeContext             *encode_context_ptr = sequence_control_set_ptr->encode_context_ptr;
                reference_queue_index = encode_context_ptr->reference_picture_queue_head_index;
                // Find the Reference in the Reference Queue
                do {

                    reference_entry_ptr = encode_context_ptr->reference_picture_queue[reference_queue_index];

                    if (reference_entry_ptr->picture_number == parentpicture_control_set_ptr->picture_number) {

                        // Set the feedback arrived
                        reference_entry_ptr->feedback_arrived = EB_TRUE;
                    }

                    // Increment the reference_queue_index Iterator
                    reference_queue_index = (reference_queue_index == REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : reference_queue_index + 1;

                } while ((reference_queue_index != encode_context_ptr->reference_picture_queue_tail_index) && (reference_entry_ptr->picture_number != parentpicture_control_set_ptr->picture_number));
            }
            // Frame level RC
            if (sequence_control_set_ptr->intra_period == -1 || sequence_control_set_ptr->static_config.rate_control_mode == 0){
                rate_control_param_ptr = context_ptr->rate_control_param_queue[0];
                prev_gop_rate_control_param_ptr = context_ptr->rate_control_param_queue[0];
                if (parentpicture_control_set_ptr->slice_type == I_SLICE){

                    if (parentpicture_control_set_ptr->total_num_bits > MAX_BITS_PER_FRAME){
                        context_ptr->max_rate_adjust_delta_qp++;
                    }
                    else if (context_ptr->max_rate_adjust_delta_qp > 0 && parentpicture_control_set_ptr->total_num_bits < MAX_BITS_PER_FRAME * 85 / 100){
                        context_ptr->max_rate_adjust_delta_qp--;
                    }
                    context_ptr->max_rate_adjust_delta_qp = CLIP3(0, 63, context_ptr->max_rate_adjust_delta_qp);
                    context_ptr->max_rate_adjust_delta_qp = 0;
                }
            }
            else{
                uint32_t interval_index_temp = 0;
                EB_BOOL intervalFound = EB_FALSE;
                while ((interval_index_temp < PARALLEL_GOP_MAX_NUMBER) && !intervalFound){

                    if (parentpicture_control_set_ptr->picture_number >= context_ptr->rate_control_param_queue[interval_index_temp]->first_poc &&
                        parentpicture_control_set_ptr->picture_number <= context_ptr->rate_control_param_queue[interval_index_temp]->last_poc) {
                        intervalFound = EB_TRUE;
                    }
                    else{
                        interval_index_temp++;
                    }
                }
                CHECK_REPORT_ERROR(
                    interval_index_temp != PARALLEL_GOP_MAX_NUMBER,
                    sequence_control_set_ptr->encode_context_ptr->app_callback_ptr,
                    EB_ENC_RC_ERROR0);

                rate_control_param_ptr = context_ptr->rate_control_param_queue[interval_index_temp];

                prev_gop_rate_control_param_ptr = (interval_index_temp == 0) ?
                    context_ptr->rate_control_param_queue[PARALLEL_GOP_MAX_NUMBER - 1] :
                    context_ptr->rate_control_param_queue[interval_index_temp - 1];

            }
            if (sequence_control_set_ptr->static_config.rate_control_mode != 0){

                context_ptr->previous_virtual_buffer_level = context_ptr->virtual_buffer_level;

                context_ptr->virtual_buffer_level =
                    (int64_t)context_ptr->previous_virtual_buffer_level +
                    (int64_t)parentpicture_control_set_ptr->total_num_bits - (int64_t)context_ptr->high_level_rate_control_ptr->channel_bit_rate_per_frame;

                eb_vp9_high_level_rc_feed_back_picture(
                    parentpicture_control_set_ptr,
                    sequence_control_set_ptr);
                if (sequence_control_set_ptr->static_config.rate_control_mode == 1)
                    eb_vp9_frame_level_rc_feedback_picture_vbr(
                        parentpicture_control_set_ptr,
                        sequence_control_set_ptr,
                        context_ptr);
                else
                    eb_vp9_frame_level_rc_feedback_picture_cbr(
                        parentpicture_control_set_ptr,
                        sequence_control_set_ptr,
                        context_ptr);
                if (parentpicture_control_set_ptr->picture_number == rate_control_param_ptr->first_poc){
                    rate_control_param_ptr->first_pic_pred_bits   = parentpicture_control_set_ptr->target_bits_best_pred_qp;
                    rate_control_param_ptr->first_pic_actual_bits = parentpicture_control_set_ptr->total_num_bits;
                    {
                        int16_t deltaApQp = (int16_t)rate_control_param_ptr->first_pic_actual_qp - (int16_t)rate_control_param_ptr->first_pic_pred_qp;
                        rate_control_param_ptr->extra_ap_bit_ratio_i = (rate_control_param_ptr->first_pic_pred_bits != 0) ?
                            (((int64_t)rate_control_param_ptr->first_pic_actual_bits - (int64_t)rate_control_param_ptr->first_pic_pred_bits) * 100) / ((int64_t)rate_control_param_ptr->first_pic_pred_bits) :
                            0;
                        rate_control_param_ptr->extra_ap_bit_ratio_i += (int64_t)deltaApQp * 15;
                    }

                }

            }

            // Queue variables
#if OVERSHOOT_STAT_PRINT
            if (sequence_control_set_ptr->intra_period != -1){

                int32_t                       queue_entry_index;
                uint32_t                       queue_entry_index_temp;
                uint32_t                       queue_entry_index_temp2;
                CodedFramesStatsEntry       *queue_entry_ptr;
                EB_BOOL                      move_slide_wondow_flag = EB_TRUE;
                EB_BOOL                      end_of_sequence_flag = EB_TRUE;
                uint32_t                       frames_in_sw;

                // Determine offset from the Head Ptr
                queue_entry_index = (int32_t)(parentpicture_control_set_ptr->picture_number - context_ptr->coded_frames_stat_queue[context_ptr->coded_frames_stat_queue_head_index]->picture_number);
                queue_entry_index += context_ptr->coded_frames_stat_queue_head_index;
                queue_entry_index = (queue_entry_index > CODED_FRAMES_STAT_QUEUE_MAX_DEPTH - 1) ? queue_entry_index - CODED_FRAMES_STAT_QUEUE_MAX_DEPTH : queue_entry_index;
                queue_entry_ptr   = context_ptr->coded_frames_stat_queue[queue_entry_index];

                queue_entry_ptr->frame_total_bit_actual  = (uint64_t)parentpicture_control_set_ptr->total_num_bits;
                queue_entry_ptr->picture_number        = parentpicture_control_set_ptr->picture_number;
                queue_entry_ptr->end_of_sequence_flag    = parentpicture_control_set_ptr->end_of_sequence_flag;
                context_ptr->rate_average_periodin_frames = (uint64_t)sequence_control_set_ptr->static_config.intra_period + 1;

                //SVT_LOG("\n0_POC: %d\n",
                //    queue_entry_ptr->picture_number);
                move_slide_wondow_flag = EB_TRUE;
                while (move_slide_wondow_flag){
                  //  SVT_LOG("\n1_POC: %d\n",
                  //      queue_entry_ptr->picture_number);
                    // Check if the sliding window condition is valid
                    queue_entry_index_temp = context_ptr->coded_frames_stat_queue_head_index;
                    if (context_ptr->coded_frames_stat_queue[queue_entry_index_temp]->frame_total_bit_actual != -1){
                        end_of_sequence_flag = context_ptr->coded_frames_stat_queue[queue_entry_index_temp]->end_of_sequence_flag;
                    }
                    else{
                        end_of_sequence_flag = EB_FALSE;
                    }
                    while (move_slide_wondow_flag && !end_of_sequence_flag &&
                        queue_entry_index_temp < context_ptr->coded_frames_stat_queue_head_index + context_ptr->rate_average_periodin_frames){
                       // SVT_LOG("\n2_POC: %d\n",
                       //     queue_entry_ptr->picture_number);

                        queue_entry_index_temp2 = (queue_entry_index_temp > CODED_FRAMES_STAT_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - CODED_FRAMES_STAT_QUEUE_MAX_DEPTH : queue_entry_index_temp;

                        move_slide_wondow_flag = (EB_BOOL)(move_slide_wondow_flag && (context_ptr->coded_frames_stat_queue[queue_entry_index_temp2]->frame_total_bit_actual != -1));

                        if (context_ptr->coded_frames_stat_queue[queue_entry_index_temp2]->frame_total_bit_actual != -1){
                            // check if it is the last frame. If we have reached the last frame, we would output the buffered frames in the Queue.
                            end_of_sequence_flag = context_ptr->coded_frames_stat_queue[queue_entry_index_temp]->end_of_sequence_flag;
                        }
                        else{
                            end_of_sequence_flag = EB_FALSE;
                        }
                        queue_entry_index_temp =
                            (queue_entry_index_temp == CODED_FRAMES_STAT_QUEUE_MAX_DEPTH - 1) ? 0 : queue_entry_index_temp + 1;

                    }

                    if (move_slide_wondow_flag)  {
                        //get a new entry spot
                        queue_entry_ptr = (context_ptr->coded_frames_stat_queue[context_ptr->coded_frames_stat_queue_head_index]);
                        queue_entry_index_temp = context_ptr->coded_frames_stat_queue_head_index;
                        // This is set to false, so the last frame would go inside the loop
                        end_of_sequence_flag = EB_FALSE;
                        frames_in_sw = 0;
                        context_ptr->total_bit_actual_per_sw = 0;

                        while (!end_of_sequence_flag &&
                            queue_entry_index_temp < context_ptr->coded_frames_stat_queue_head_index + context_ptr->rate_average_periodin_frames){
                            frames_in_sw++;

                            queue_entry_index_temp2 = (queue_entry_index_temp > CODED_FRAMES_STAT_QUEUE_MAX_DEPTH - 1) ? queue_entry_index_temp - CODED_FRAMES_STAT_QUEUE_MAX_DEPTH : queue_entry_index_temp;

                            context_ptr->total_bit_actual_per_sw += context_ptr->coded_frames_stat_queue[queue_entry_index_temp2]->frame_total_bit_actual;
                            end_of_sequence_flag = context_ptr->coded_frames_stat_queue[queue_entry_index_temp2]->end_of_sequence_flag;

                            queue_entry_index_temp =
                                (queue_entry_index_temp == CODED_FRAMES_STAT_QUEUE_MAX_DEPTH - 1) ? 0 : queue_entry_index_temp + 1;

                        }
                        //

                        //if(frames_in_sw == context_ptr->rate_average_periodin_frames)
                        //    SVT_LOG("POC:%d\t %.3f\n", queue_entry_ptr->picture_number, (double)context_ptr->total_bit_actual_per_sw*(sequence_control_set_ptr->frame_rate>> RC_PRECISION)/(double)frames_in_sw/1000);
                        if (frames_in_sw == (uint32_t)sequence_control_set_ptr->intra_period + 1){
                            context_ptr->max_bit_actual_per_sw = MAX(context_ptr->max_bit_actual_per_sw, context_ptr->total_bit_actual_per_sw*(sequence_control_set_ptr->frame_rate >> RC_PRECISION) / frames_in_sw / 1000);
                            if (queue_entry_ptr->picture_number % ((sequence_control_set_ptr->intra_period + 1)) == 0){
                                context_ptr->max_bit_actual_per_gop = MAX(context_ptr->max_bit_actual_per_gop, context_ptr->total_bit_actual_per_sw*(sequence_control_set_ptr->frame_rate >> RC_PRECISION) / frames_in_sw / 1000);
                                context_ptr->min_bit_actual_per_gop = MIN(context_ptr->min_bit_actual_per_gop, context_ptr->total_bit_actual_per_sw*(sequence_control_set_ptr->frame_rate >> RC_PRECISION) / frames_in_sw / 1000);
                                if (1) {
                                //if (context_ptr->total_bit_actual_per_sw > sequence_control_set_ptr->static_config.max_buffersize){
                                    SVT_LOG("POC:%d\t%.0f\t%.2f%% \n",
                                        (int)queue_entry_ptr->picture_number,
                                        (double)((int64_t)context_ptr->total_bit_actual_per_sw*(sequence_control_set_ptr->frame_rate >> RC_PRECISION) / frames_in_sw / 1000),
                                        (double)(100 * (double)context_ptr->total_bit_actual_per_sw*(sequence_control_set_ptr->frame_rate >> RC_PRECISION) / frames_in_sw / (double)sequence_control_set_ptr->static_config.target_bit_rate) - 100);
                                }
                            }
                        }
                        if (frames_in_sw == context_ptr->rate_average_periodin_frames - 1){
                            //SVT_LOG("\n%d MAX\n", (int32_t)context_ptr->max_bit_actual_per_sw);
                            SVT_LOG("\n%d GopMax\t", (int32_t)context_ptr->max_bit_actual_per_gop);
                            SVT_LOG("%d GopMin\n", (int32_t)context_ptr->min_bit_actual_per_gop);
                        }
                        // Reset the Queue Entry
                        queue_entry_ptr->picture_number += CODED_FRAMES_STAT_QUEUE_MAX_DEPTH;
                        queue_entry_ptr->frame_total_bit_actual = -1;

                        // Increment the Reorder Queue head Ptr
                        context_ptr->coded_frames_stat_queue_head_index =
                            (context_ptr->coded_frames_stat_queue_head_index == CODED_FRAMES_STAT_QUEUE_MAX_DEPTH - 1) ? 0 : context_ptr->coded_frames_stat_queue_head_index + 1;

                        queue_entry_ptr = (context_ptr->coded_frames_stat_queue[context_ptr->coded_frames_stat_queue_head_index]);

                    }
                }
            }
#endif
            total_number_of_fb_frames++;

            // Release the SequenceControlSet
            eb_vp9_release_object(parentpicture_control_set_ptr->sequence_control_set_wrapper_ptr);
            // Release the ParentPictureControlSet

            eb_vp9_release_object(parentpicture_control_set_ptr->input_picture_wrapper_ptr);
            eb_vp9_release_object(rate_control_tasks_ptr->picture_control_set_wrapper_ptr);

            // Release Rate Control Tasks
            eb_vp9_release_object(rate_control_tasks_wrapper_ptr);
            break;

        case RC_ENTROPY_CODING_ROW_FEEDBACK_RESULT:

            // Extract bits-per-lcu-row

            // Release Rate Control Tasks
            eb_vp9_release_object(rate_control_tasks_wrapper_ptr);

            break;

        default:
            picture_control_set_ptr = (PictureControlSet*)rate_control_tasks_ptr->picture_control_set_wrapper_ptr->object_ptr;
            sequence_control_set_ptr = (SequenceControlSet*)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;

            break;
        }
    }
    return EB_NULL;
}
