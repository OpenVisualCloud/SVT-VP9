/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbPictureBufferDesc.h"

#include "EbResourceCoordinationProcess.h"

#include "EbResourceCoordinationResults.h"
#include "EbReferenceObject.h"
#include "EbTime.h"

#include "vp9_alloccommon.h"
#include "vp9_common.h"
#include "vp9_reconintra.h"
#include "vp9_quantize.h"
#include "vp9_entropy.h"
#include "vp9_encodemv.h"
#include "vp9_ratectrl.h"

/************************************************
 * Resource Coordination Context Constructor
 ************************************************/
EbErrorType eb_vp9_resource_coordination_context_ctor(
    ResourceCoordinationContext  **context_dbl_ptr,
    EbFifo                        *input_buffer_fifo_ptr,
    EbFifo                        *resource_coordination_results_output_fifo_ptr,
    EbFifo                       **picture_control_set_fifo_ptr_array,
    EbSequenceControlSetInstance **sequence_control_set_instance_array,
    EbFifo                        *sequence_control_set_empty_fifo_ptr,
    EbCallback                   **app_callback_ptr_array,
    uint32_t                      *compute_segments_total_count_array,
    uint32_t                       encode_instances_total_count)
{
    uint32_t instance_index;

    ResourceCoordinationContext   *context_ptr;
    EB_MALLOC(ResourceCoordinationContext  *, context_ptr, sizeof(ResourceCoordinationContext  ), EB_N_PTR);

    *context_dbl_ptr = context_ptr;

    context_ptr->input_buffer_fifo_ptr                            = input_buffer_fifo_ptr;
    context_ptr->resource_coordination_results_output_fifo_ptr    = resource_coordination_results_output_fifo_ptr;
    context_ptr->picture_control_set_fifo_ptr_array               = picture_control_set_fifo_ptr_array;
    context_ptr->sequence_control_set_instance_array              = sequence_control_set_instance_array;
    context_ptr->sequence_control_set_empty_fifo_ptr              = sequence_control_set_empty_fifo_ptr;
    context_ptr->app_callback_ptr_array                           = app_callback_ptr_array;
    context_ptr->compute_segments_total_count_array               = compute_segments_total_count_array;
    context_ptr->encode_instances_total_count                     = encode_instances_total_count;

    // Allocate sequence_control_set_active_array
    EB_MALLOC(EbObjectWrapper**, context_ptr->sequence_control_set_active_array, sizeof(EbObjectWrapper*) * context_ptr->encode_instances_total_count, EB_N_PTR);

    for(instance_index=0; instance_index < context_ptr->encode_instances_total_count; ++instance_index) {
        context_ptr->sequence_control_set_active_array[instance_index] = 0;
    }

    // Picture Stats
    EB_MALLOC(uint64_t*, context_ptr->picture_number_array, sizeof(uint64_t) * context_ptr->encode_instances_total_count, EB_N_PTR);

    for(instance_index=0; instance_index < context_ptr->encode_instances_total_count; ++instance_index) {
        context_ptr->picture_number_array[instance_index] = 0;
    }

    context_ptr->average_enc_mod                    = 0;
    context_ptr->prev_enc_mod                       = 0;
    context_ptr->prev_enc_mode_delta                = 0;
    context_ptr->cur_speed                          = 0; // speed x 1000
    context_ptr->previous_mode_change_buffer        = 0;
    context_ptr->first_in_pic_arrived_time_seconds  = 0;
    context_ptr->first_in_pic_arrived_timeu_seconds = 0;
    context_ptr->previous_frame_in_check1           = 0;
    context_ptr->previous_frame_in_check2           = 0;
    context_ptr->previous_frame_in_check3           = 0;
    context_ptr->previous_mode_change_frame_in      = 0;
    context_ptr->prevs_time_seconds                 = 0;
    context_ptr->prevs_timeu_seconds                = 0;
    context_ptr->prev_frame_out                     = 0;
    context_ptr->start_flag                         = EB_FALSE;
    context_ptr->previous_buffer_check1             = 0;
    context_ptr->prev_change_cond                   = 0;

    return EB_ErrorNone;
}

//******************************************************************************//
// Modify the Enc mode based on the buffer Status
// Inputs: TargetSpeed, Status of the sc_buffer
// Output: EncMod
//******************************************************************************//
void eb_vp9_SpeedBufferControl(
    ResourceCoordinationContext *context_ptr,
    PictureParentControlSet     *picture_control_set_ptr,
    SequenceControlSet          *sequence_control_set_ptr)
{

    uint64_t curs_time_seconds  = 0;
    uint64_t curs_timeu_seconds = 0;
    double overall_duration = 0.0;
    double inst_duration = 0.0;
    int8_t   encoder_mode_delta = 0;
    int64_t  input_frames_count = 0;
    int8_t   change_cond        = 0;
    int64_t  target_fps         = (sequence_control_set_ptr->static_config.injector_frame_rate >> 16);

    int64_t buffer_trshold1 = SC_FRAMES_INTERVAL_T1;
    int64_t buffer_trshold2 = SC_FRAMES_INTERVAL_T2;
    int64_t buffer_trshold3 = SC_FRAMES_INTERVAL_T3;
    int64_t buffer_trshold4 = MAX(SC_FRAMES_INTERVAL_T1, target_fps);
    eb_vp9_block_on_mutex(sequence_control_set_ptr->encode_context_ptr->sc_buffer_mutex);

    if (sequence_control_set_ptr->encode_context_ptr->sc_frame_in == 0) {
        svt_vp9_get_time(&context_ptr->first_in_pic_arrived_time_seconds,
                        &context_ptr->first_in_pic_arrived_timeu_seconds);
    }
    else if (sequence_control_set_ptr->encode_context_ptr->sc_frame_in == SC_FRAMES_TO_IGNORE) {
        context_ptr->start_flag = EB_TRUE;
    }

    // Compute duration since the start of the encode and since the previous checkpoint
    svt_vp9_get_time(&curs_time_seconds, &curs_timeu_seconds);

    overall_duration = svt_vp9_compute_overall_elapsed_time_ms(
        context_ptr->first_in_pic_arrived_time_seconds,
        context_ptr->first_in_pic_arrived_timeu_seconds, curs_time_seconds,
        curs_timeu_seconds);

    inst_duration = svt_vp9_compute_overall_elapsed_time_ms(
        context_ptr->prevs_time_seconds, context_ptr->prevs_timeu_seconds,
        curs_time_seconds, curs_timeu_seconds);

    input_frames_count = (int64_t)overall_duration *(sequence_control_set_ptr->static_config.injector_frame_rate >> 16) / 1000;
    sequence_control_set_ptr->encode_context_ptr->sc_buffer = input_frames_count - sequence_control_set_ptr->encode_context_ptr->sc_frame_in;

    encoder_mode_delta = 0;

    // Check every bufferTsshold1 for the changes (previous_frame_in_check1 variable)
    if ((sequence_control_set_ptr->encode_context_ptr->sc_frame_in > context_ptr->previous_frame_in_check1 + buffer_trshold1 && sequence_control_set_ptr->encode_context_ptr->sc_frame_in >= SC_FRAMES_TO_IGNORE)) {
        // Go to a slower mode based on the fullness and changes of the buffer
        if (sequence_control_set_ptr->encode_context_ptr->sc_buffer < buffer_trshold4 && (context_ptr->prev_enc_mode_delta >-1 || (context_ptr->prev_enc_mode_delta < 0 && sequence_control_set_ptr->encode_context_ptr->sc_frame_in > context_ptr->previous_mode_change_frame_in + buffer_trshold4 * 2))) {
            if (context_ptr->previous_buffer_check1 > sequence_control_set_ptr->encode_context_ptr->sc_buffer + buffer_trshold1) {
                encoder_mode_delta += -1;
                change_cond = 2;
            }
            else if (context_ptr->previous_mode_change_buffer > buffer_trshold1 + sequence_control_set_ptr->encode_context_ptr->sc_buffer && sequence_control_set_ptr->encode_context_ptr->sc_buffer < buffer_trshold1) {
                encoder_mode_delta += -1;
                change_cond = 4;
            }
        }

        // Go to a faster mode based on the fullness and changes of the buffer
        if (sequence_control_set_ptr->encode_context_ptr->sc_buffer >buffer_trshold1 + context_ptr->previous_buffer_check1) {
            encoder_mode_delta += +1;
            change_cond = 1;
        }
        else if (sequence_control_set_ptr->encode_context_ptr->sc_buffer > buffer_trshold1 + context_ptr->previous_mode_change_buffer) {
            encoder_mode_delta += +1;
            change_cond = 3;
        }

        // Update the encode mode based on the fullness of the buffer
        // If previous change_cond was the same, double the threshold2
        if (sequence_control_set_ptr->encode_context_ptr->sc_buffer > buffer_trshold3 &&
            (context_ptr->prev_change_cond != 7 || sequence_control_set_ptr->encode_context_ptr->sc_frame_in > context_ptr->previous_mode_change_frame_in + buffer_trshold2 * 2) &&
            sequence_control_set_ptr->encode_context_ptr->sc_buffer > context_ptr->previous_mode_change_buffer) {
            encoder_mode_delta += 1;
            change_cond = 7;
        }
        encoder_mode_delta = CLIP3(-1, 1, encoder_mode_delta);
        sequence_control_set_ptr->encode_context_ptr->enc_mode = (EB_ENC_MODE)CLIP3(ENC_MODE_0, sequence_control_set_ptr->max_enc_mode, (int8_t)sequence_control_set_ptr->encode_context_ptr->enc_mode + encoder_mode_delta);

        // Update previous stats
        context_ptr->previous_frame_in_check1 = sequence_control_set_ptr->encode_context_ptr->sc_frame_in;
        context_ptr->previous_buffer_check1 = sequence_control_set_ptr->encode_context_ptr->sc_buffer;

        if (encoder_mode_delta) {
            context_ptr->previous_mode_change_buffer = sequence_control_set_ptr->encode_context_ptr->sc_buffer;
            context_ptr->previous_mode_change_frame_in = sequence_control_set_ptr->encode_context_ptr->sc_frame_in;
            context_ptr->prev_enc_mode_delta = encoder_mode_delta;
        }
    }

    // Check every buffer_trshold2 for the changes (previous_frame_in_check2 variable)
    if ((sequence_control_set_ptr->encode_context_ptr->sc_frame_in > context_ptr->previous_frame_in_check2 + buffer_trshold2 && sequence_control_set_ptr->encode_context_ptr->sc_frame_in >= SC_FRAMES_TO_IGNORE)) {
        encoder_mode_delta = 0;

        // if no change in the encoder mode and buffer is low enough and level is not increasing, switch to a slower encoder mode
        // If previous change_cond was the same, double the threshold2
        if (encoder_mode_delta == 0 && sequence_control_set_ptr->encode_context_ptr->sc_frame_in > context_ptr->previous_mode_change_frame_in + buffer_trshold2 &&
            (context_ptr->prev_change_cond != 8 || sequence_control_set_ptr->encode_context_ptr->sc_frame_in > context_ptr->previous_mode_change_frame_in + buffer_trshold2 * 2) &&
            ((sequence_control_set_ptr->encode_context_ptr->sc_buffer - context_ptr->previous_mode_change_buffer < (buffer_trshold4 / 3)) || context_ptr->previous_mode_change_buffer == 0) &&
            sequence_control_set_ptr->encode_context_ptr->sc_buffer < buffer_trshold3) {
            encoder_mode_delta = -1;
            change_cond = 8;
        }

        encoder_mode_delta = CLIP3(-1, 1, encoder_mode_delta);
        sequence_control_set_ptr->encode_context_ptr->enc_mode = (EB_ENC_MODE)CLIP3(ENC_MODE_0, sequence_control_set_ptr->max_enc_mode, (int8_t)sequence_control_set_ptr->encode_context_ptr->enc_mode + encoder_mode_delta);
        // Update previous stats
        context_ptr->previous_frame_in_check2 = sequence_control_set_ptr->encode_context_ptr->sc_frame_in;

        if (encoder_mode_delta) {
            context_ptr->previous_mode_change_buffer = sequence_control_set_ptr->encode_context_ptr->sc_buffer;
            context_ptr->previous_mode_change_frame_in = sequence_control_set_ptr->encode_context_ptr->sc_frame_in;
            context_ptr->prev_enc_mode_delta = encoder_mode_delta;
        }

    }
    // Check every SC_FRAMES_INTERVAL_SPEED frames for the speed calculation (previous_frame_in_check3 variable)
    if (context_ptr->start_flag || (sequence_control_set_ptr->encode_context_ptr->sc_frame_in > context_ptr->previous_frame_in_check3 + SC_FRAMES_INTERVAL_SPEED && sequence_control_set_ptr->encode_context_ptr->sc_frame_in >= SC_FRAMES_TO_IGNORE)) {
        if (context_ptr->start_flag) {
            context_ptr->cur_speed = (uint64_t)(sequence_control_set_ptr->encode_context_ptr->sc_frame_out - 0) * 1000 / (uint64_t)(overall_duration);
        }
        else {
            if (inst_duration != 0)
                context_ptr->cur_speed = (uint64_t)(sequence_control_set_ptr->encode_context_ptr->sc_frame_out - context_ptr->prev_frame_out) * 1000 / (uint64_t)(inst_duration);
        }
        context_ptr->start_flag = EB_FALSE;

        // Update previous stats
        context_ptr->previous_frame_in_check3 = sequence_control_set_ptr->encode_context_ptr->sc_frame_in;
        context_ptr->prevs_time_seconds = curs_time_seconds;
        context_ptr->prevs_timeu_seconds = curs_timeu_seconds;
        context_ptr->prev_frame_out = sequence_control_set_ptr->encode_context_ptr->sc_frame_out;

    }
    else if (sequence_control_set_ptr->encode_context_ptr->sc_frame_in < SC_FRAMES_TO_IGNORE && (overall_duration != 0)) {
        context_ptr->cur_speed = (uint64_t)(sequence_control_set_ptr->encode_context_ptr->sc_frame_out - 0) * 1000 / (uint64_t)(overall_duration);
    }

    if (change_cond) {
        context_ptr->prev_change_cond = change_cond;
    }
    sequence_control_set_ptr->encode_context_ptr->sc_frame_in++;
    if (sequence_control_set_ptr->encode_context_ptr->sc_frame_in >= SC_FRAMES_TO_IGNORE) {
        context_ptr->average_enc_mod += sequence_control_set_ptr->encode_context_ptr->enc_mode;
    }
    else {
        context_ptr->average_enc_mod = 0;
    }

    // Set the encoder level
    picture_control_set_ptr->enc_mode = sequence_control_set_ptr->encode_context_ptr->enc_mode;

    eb_vp9_release_mutex(sequence_control_set_ptr->encode_context_ptr->sc_buffer_mutex);

    context_ptr->prev_enc_mod = sequence_control_set_ptr->encode_context_ptr->enc_mode;
}

/******************************************************
* Derive Pre-Analysis settings for SQ
Input   : encoder mode and tune
Output  : Pre-Analysis signal(s)
******************************************************/
EbErrorType eb_vp9_signal_derivation_pre_analysis_sq(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr) {

    EbErrorType return_error = EB_ErrorNone;

    uint8_t input_resolution = sequence_control_set_ptr->input_resolution;

    // Derive Noise Detection Method
    if (input_resolution == INPUT_SIZE_4K_RANGE) {
        if (picture_control_set_ptr->enc_mode <= ENC_MODE_8) {
            picture_control_set_ptr->noise_detection_method = NOISE_DETECT_FULL_PRECISION;
        }
        else {
            picture_control_set_ptr->noise_detection_method = NOISE_DETECT_HALF_PRECISION;
        }

    }
    else if (input_resolution == INPUT_SIZE_1080p_RANGE) {
        if (picture_control_set_ptr->enc_mode <= ENC_MODE_8) {
            picture_control_set_ptr->noise_detection_method = NOISE_DETECT_FULL_PRECISION;
        }
        else {
            picture_control_set_ptr->noise_detection_method = NOISE_DETECT_QUARTER_PRECISION;
        }

    }
    else {
        picture_control_set_ptr->noise_detection_method = NOISE_DETECT_FULL_PRECISION;
    }
#if TURN_OFF_PRE_PROCESSING
    picture_control_set_ptr->enable_denoise_src_flag = EB_FALSE;
#else
    picture_control_set_ptr->enable_denoise_src_flag = picture_control_set_ptr->enc_mode <= ENC_MODE_11 ? sequence_control_set_ptr->enable_denoise_flag : EB_FALSE;
#endif

    // Derive Noise Detection Threshold
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_8) {
        picture_control_set_ptr->noise_detection_th = 0;
    }
    else {
        picture_control_set_ptr->noise_detection_th = 1;
    }

    // Derive HME Flag
    uint8_t hme_me_level = picture_control_set_ptr->enc_mode;

    uint32_t input_ratio = sequence_control_set_ptr->luma_width / sequence_control_set_ptr->luma_height;
    uint8_t resolution_index = input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER ? 0 : // 480P
        (input_resolution <= INPUT_SIZE_1080i_RANGE && input_ratio < 2) ? 1 : // 720P
        (input_resolution <= INPUT_SIZE_1080i_RANGE && input_ratio > 3) ? 2 : // 1080I
        (input_resolution <= INPUT_SIZE_1080p_RANGE) ? 3 : // 1080I
        4;    // 4K
    if (sequence_control_set_ptr->static_config.use_default_me_hme) {

        picture_control_set_ptr->enable_hme_flag = EB_TRUE;

    }
    else {
        picture_control_set_ptr->enable_hme_flag = sequence_control_set_ptr->static_config.enable_hme_flag;

    }
    picture_control_set_ptr->enable_hme_level_0_flag = enable_hme_level0_flag_sq[resolution_index][hme_me_level];
    picture_control_set_ptr->enable_hme_level_1_flag = enable_hme_level1_flag_sq[resolution_index][hme_me_level];
    picture_control_set_ptr->enable_hme_level_2_flag = enable_hme_level2_flag_sq[resolution_index][hme_me_level];
    return return_error;
}

/******************************************************
* Derive Pre-Analysis settings for OQ
Input   : encoder mode and tune
Output  : Pre-Analysis signal(s)
******************************************************/
EbErrorType eb_vp9_signal_derivation_pre_analysis_oq(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr) {

    EbErrorType return_error = EB_ErrorNone;

    uint8_t input_resolution = sequence_control_set_ptr->input_resolution;

    // Derive Noise Detection Method
    if (input_resolution == INPUT_SIZE_4K_RANGE) {
        if (picture_control_set_ptr->enc_mode <= ENC_MODE_3) {
            picture_control_set_ptr->noise_detection_method = NOISE_DETECT_FULL_PRECISION;
        }
        else {
            picture_control_set_ptr->noise_detection_method = NOISE_DETECT_HALF_PRECISION;
        }
    }
    else if (input_resolution == INPUT_SIZE_1080p_RANGE) {
        if (picture_control_set_ptr->enc_mode <= ENC_MODE_3) {
            picture_control_set_ptr->noise_detection_method = NOISE_DETECT_FULL_PRECISION;
        }
        else {
            picture_control_set_ptr->noise_detection_method = NOISE_DETECT_QUARTER_PRECISION;
        }
    }
    else {
        picture_control_set_ptr->noise_detection_method = NOISE_DETECT_FULL_PRECISION;
    }

    // Derive Noise Detection Threshold
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_3) {
        picture_control_set_ptr->noise_detection_th = 0;
    }
    else if (picture_control_set_ptr->enc_mode <= ENC_MODE_8) {
        if (input_resolution <= INPUT_SIZE_1080p_RANGE) {
            picture_control_set_ptr->noise_detection_th = 1;
        }
        else {
            picture_control_set_ptr->noise_detection_th = 0;
        }
    }
    else {
        picture_control_set_ptr->noise_detection_th = 1;
    }

    // Derive HME Flag
    uint8_t  hme_me_level = picture_control_set_ptr->enc_mode;

    uint32_t input_ratio = sequence_control_set_ptr->luma_width / sequence_control_set_ptr->luma_height;
    uint8_t resolution_index = input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER ? 0 : // 480P
        (input_resolution <= INPUT_SIZE_1080i_RANGE && input_ratio < 2) ? 1 : // 720P
        (input_resolution <= INPUT_SIZE_1080i_RANGE && input_ratio > 3) ? 2 : // 1080I
        (input_resolution <= INPUT_SIZE_1080p_RANGE) ? 3 : // 1080I
        4;    // 4K
    if (sequence_control_set_ptr->static_config.use_default_me_hme) {

        picture_control_set_ptr->enable_hme_flag = EB_TRUE;

    }
    else {
        picture_control_set_ptr->enable_hme_flag = sequence_control_set_ptr->static_config.enable_hme_flag;

    }
    picture_control_set_ptr->enable_hme_level_0_flag = enable_hme_level0_flag_oq[resolution_index][hme_me_level];
    picture_control_set_ptr->enable_hme_level_1_flag = enable_hme_level1_flag_oq[resolution_index][hme_me_level];
    picture_control_set_ptr->enable_hme_level_2_flag = enable_hme_level2_flag_oq[resolution_index][hme_me_level];

    picture_control_set_ptr->enable_denoise_src_flag = EB_FALSE;

    return return_error;
}

/******************************************************
* Derive Pre-Analysis settings for VMAF
Input   : encoder mode and tune
Output  : Pre-Analysis signal(s)
******************************************************/
EbErrorType eb_vp9_signal_derivation_pre_analysis_vmaf(
    SequenceControlSet       *sequence_control_set_ptr,
    PictureParentControlSet  *picture_control_set_ptr) {

    EbErrorType return_error = EB_ErrorNone;

    uint8_t input_resolution = sequence_control_set_ptr->input_resolution;

    // Derive Noise Detection Method
    picture_control_set_ptr->noise_detection_method = NOISE_DETECT_QUARTER_PRECISION;

    // Derive Noise Detection Threshold
    picture_control_set_ptr->noise_detection_th = 1;

    // Derive HME Flag
    uint8_t  hme_me_level = picture_control_set_ptr->enc_mode;
    uint32_t input_ratio = sequence_control_set_ptr->luma_width / sequence_control_set_ptr->luma_height;
    uint8_t resolution_index = input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER ? 0 : // 480P
        (input_resolution <= INPUT_SIZE_1080i_RANGE && input_ratio < 2) ? 1 : // 720P
        (input_resolution <= INPUT_SIZE_1080i_RANGE && input_ratio > 3) ? 2 : // 1080I
        (input_resolution <= INPUT_SIZE_1080p_RANGE) ? 3 : // 1080I
        4;    // 4K
    resolution_index = 3;
    if (sequence_control_set_ptr->static_config.use_default_me_hme) {

        picture_control_set_ptr->enable_hme_flag = EB_TRUE;

    }
    else {
        picture_control_set_ptr->enable_hme_flag = sequence_control_set_ptr->static_config.enable_hme_flag;

    }
    picture_control_set_ptr->enable_hme_level_0_flag = enable_hme_level0_flag_vmaf[resolution_index][hme_me_level];
    picture_control_set_ptr->enable_hme_level_1_flag = enable_hme_level1_flag_vmaf[resolution_index][hme_me_level];
    picture_control_set_ptr->enable_hme_level_2_flag = enable_hme_level2_flag_vmaf[resolution_index][hme_me_level];

    picture_control_set_ptr->enable_denoise_src_flag = EB_FALSE;

    return return_error;
}

/***************************************
 * ResourceCoordination Kernel
 ***************************************/
void* eb_vp9_resource_coordination_kernel(void *input_ptr)
{
    ResourceCoordinationContext   *context_ptr = (ResourceCoordinationContext*) input_ptr;

    EbObjectWrapper               *picture_control_set_wrapper_ptr;

    PictureParentControlSet       *picture_control_set_ptr;

    EbObjectWrapper               *previoussequence_control_set_wrapper_ptr;
    SequenceControlSet            *sequence_control_set_ptr;

    EbObjectWrapper               *ebInputWrapperPtr;
    EbBufferHeaderType            *eb_input_ptr;
    EbObjectWrapper               *output_wrapper_ptr;
    ResourceCoordinationResults   *output_results_ptr;

    EbObjectWrapper               *input_picture_wrapper_ptr;
    EbObjectWrapper               *reference_picture_wrapper_ptr;
    EbObjectWrapper               *prev_picture_control_set_wrapper_ptr = NULL;
    uint32_t                       instance_index;

    EB_BOOL                        end_of_sequence_flag = EB_FALSE;
    uint32_t                       input_size = 0;

    for(;;) {

        // Tie instance_index to zero for now...
        instance_index = 0;

        // Get the Next Input Buffer [BLOCKING]
        eb_vp9_get_full_object(
            context_ptr->input_buffer_fifo_ptr,
            &ebInputWrapperPtr);
        eb_input_ptr = (EbBufferHeaderType*) ebInputWrapperPtr->object_ptr;

        sequence_control_set_ptr       = context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr;

        // If config changes occured since the last picture began encoding, then
        //   prepare a new sequence_control_set_ptr containing the new changes and update the state
        //   of the previous Active SequenceControlSet
        eb_vp9_block_on_mutex(context_ptr->sequence_control_set_instance_array[instance_index]->config_mutex);
        if(context_ptr->sequence_control_set_instance_array[instance_index]->encode_context_ptr->initial_picture) {

            // Update picture width, picture height, cropping right offset, cropping bottom offset, and conformance windows
            if(context_ptr->sequence_control_set_instance_array[instance_index]->encode_context_ptr->initial_picture)

            {
                context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->luma_width = context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->max_input_luma_width;
                context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->luma_height = context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->max_input_luma_height;
                context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->chroma_width = (context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->max_input_luma_width >> 1);
                context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->chroma_height = (context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->max_input_luma_height >> 1);

                context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->pad_right = context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->max_input_pad_right;
                context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->cropping_right_offset = context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->pad_right;
                context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->pad_bottom = context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->max_input_pad_bottom;
                context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->cropping_bottom_offset = context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->pad_bottom;

                input_size = context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->luma_width * context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr->luma_height;
            }

            // Copy previous Active sequence_control_set_ptr to a place holder
            previoussequence_control_set_wrapper_ptr = context_ptr->sequence_control_set_active_array[instance_index];

            // Get empty SequenceControlSet [BLOCKING]
            eb_vp9_get_empty_object(
                context_ptr->sequence_control_set_empty_fifo_ptr,
                &context_ptr->sequence_control_set_active_array[instance_index]);

            // Copy the contents of the active SequenceControlSet into the new empty SequenceControlSet
            eb_vp9_copy_sequence_control_set(
                (SequenceControlSet *) context_ptr->sequence_control_set_active_array[instance_index]->object_ptr,
                context_ptr->sequence_control_set_instance_array[instance_index]->sequence_control_set_ptr);

            // Disable releaseFlag of new SequenceControlSet
            eb_vp9_object_release_disable(
                context_ptr->sequence_control_set_active_array[instance_index]);

            if(previoussequence_control_set_wrapper_ptr != EB_NULL) {

                // Enable releaseFlag of old SequenceControlSet
                eb_vp9_object_release_enable(
                    previoussequence_control_set_wrapper_ptr);

                // Check to see if previous SequenceControlSet is already inactive, if TRUE then release the SequenceControlSet
                if(previoussequence_control_set_wrapper_ptr->live_count == 0) {
                    eb_vp9_release_object(
                        previoussequence_control_set_wrapper_ptr);
                }
            }
        }
        eb_vp9_release_mutex(context_ptr->sequence_control_set_instance_array[instance_index]->config_mutex);

        // Sequence Control Set is released by Rate Control after passing through MDC->MD->ENCDEC->Packetization->RateControl
        //   and in the PictureManager
        eb_vp9_object_inc_live_count(
            context_ptr->sequence_control_set_active_array[instance_index],
            2);

        // Set the current SequenceControlSet
        sequence_control_set_ptr   = (SequenceControlSet*) context_ptr->sequence_control_set_active_array[instance_index]->object_ptr;

        // Init LCU Params
        if (context_ptr->sequence_control_set_instance_array[instance_index]->encode_context_ptr->initial_picture) {
            eb_vp9_derive_input_resolution(
                sequence_control_set_ptr,
                input_size);

            eb_vp9_sb_params_init(sequence_control_set_ptr);
        }

        //Get a New ParentPCS where we will hold the new inputPicture
        eb_vp9_get_empty_object(
            context_ptr->picture_control_set_fifo_ptr_array[instance_index],
            &picture_control_set_wrapper_ptr);

        // Parent PCS is released by the Rate Control after passing through MDC->MD->ENCDEC->Packetization
        eb_vp9_object_inc_live_count(
            picture_control_set_wrapper_ptr,
            1);

        picture_control_set_ptr        = (PictureParentControlSet*) picture_control_set_wrapper_ptr->object_ptr;

        picture_control_set_ptr->p_pcs_wrapper_ptr = picture_control_set_wrapper_ptr;

        // VP9 code
        // Hsan - to clean up

        picture_control_set_ptr->cpi->alt_fb_idx = 2;
        picture_control_set_ptr->cpi->common.error_resilient_mode = 0;
        picture_control_set_ptr->cpi->common.reset_frame_context = 0;
        picture_control_set_ptr->cpi->max_mv_magnitude = 0;
        picture_control_set_ptr->cpi->common.allow_high_precision_mv = 0;
#if 0 // Hsan: switchable interp_filter not supported
        picture_control_set_ptr->cpi->common.interp_filter = 0;
#endif
        memset(picture_control_set_ptr->cpi->interp_filter_selected, 0, sizeof(picture_control_set_ptr->cpi->interp_filter_selected[0][0]) * SWITCHABLE);
        picture_control_set_ptr->cpi->sf.mv.auto_mv_step_size = 0;

        eb_vp9_entropy_mv_init();

        // variable
        picture_control_set_ptr->cpi->common.bit_depth = VPX_BITS_8;
        picture_control_set_ptr->cpi->common.width = sequence_control_set_ptr->luma_width;
        picture_control_set_ptr->cpi->common.height = sequence_control_set_ptr->luma_height;
        picture_control_set_ptr->cpi->common.render_width = picture_control_set_ptr->cpi->common.width;
        picture_control_set_ptr->cpi->common.render_height = picture_control_set_ptr->cpi->common.height;

        eb_vp9_set_mb_mi(
            &picture_control_set_ptr->cpi->common,
            picture_control_set_ptr->cpi->common.width,
            picture_control_set_ptr->cpi->common.height);

        picture_control_set_ptr->cpi->common.profile = PROFILE_0;
        picture_control_set_ptr->cpi->common.color_space = VPX_CS_UNKNOWN;
        picture_control_set_ptr->cpi->common.color_range = VPX_CR_STUDIO_RANGE;
        picture_control_set_ptr->cpi->common.subsampling_x = 1;
        picture_control_set_ptr->cpi->common.subsampling_y = 1;

        picture_control_set_ptr->cpi->common.frame_parallel_decoding_mode = 1;
        picture_control_set_ptr->cpi->common.frame_context_idx = 0;
        picture_control_set_ptr->cpi->common.y_dc_delta_q = 0;
#if CHROMA_QP_OFFSET
        if (sequence_control_set_ptr->static_config.tune == TUNE_OQ) {
            picture_control_set_ptr->cpi->common.uv_dc_delta_q = -15;
            picture_control_set_ptr->cpi->common.uv_ac_delta_q = -15;
        }
        else {
            picture_control_set_ptr->cpi->common.uv_dc_delta_q = 0;
            picture_control_set_ptr->cpi->common.uv_ac_delta_q = 0;
        }
#else
        picture_control_set_ptr->cpi->common.uv_dc_delta_q = 0;
        picture_control_set_ptr->cpi->common.uv_ac_delta_q = 0;
#endif
        picture_control_set_ptr->cpi->common.seg.enabled = 0;
        picture_control_set_ptr->cpi->common.seg.update_map = 0;
        picture_control_set_ptr->cpi->common.log2_tile_cols = 0;
        picture_control_set_ptr->cpi->common.log2_tile_rows = 0;
        picture_control_set_ptr->cpi->use_svc = 0;
        picture_control_set_ptr->cpi->common.tx_mode = ALLOW_32X32;

        // Hsan: is it the right spot ?
        eb_vp9_default_coef_probs(&picture_control_set_ptr->cpi->common);
        eb_vp9_init_mv_probs(&picture_control_set_ptr->cpi->common);

        eb_vp9_init_mode_probs(picture_control_set_ptr->cpi->common.fc);
        vp9_zero(*picture_control_set_ptr->cpi->td.counts);        // Hsan  could be completely removed
        vp9_zero(picture_control_set_ptr->cpi->td.rd_counts);      // Hsan  could be completely removed

        eb_vp9_init_intra_predictors();

        /* eb_vp9_init_quantizer() `is first called here. Add check in
         * vp9_frame_init_quantizer() so that eb_vp9_init_quantizer is only
         * called later when needed. This will avoid unnecessary calls of
         * eb_vp9_init_quantizer() for every frame.
         */
        eb_vp9_init_quantizer(picture_control_set_ptr->cpi);

        eb_vp9_rc_init_minq_luts();
#if SEG_SUPPORT
        eb_vp9_reset_segment_features(&picture_control_set_ptr->cpi->common.seg);
#endif
        // Set the Encoder mode
        picture_control_set_ptr->enc_mode = sequence_control_set_ptr->static_config.enc_mode;

        // Keep track of the previous input for the ZZ SADs computation
        picture_control_set_ptr->previous_picture_control_set_wrapper_ptr = (context_ptr->sequence_control_set_instance_array[instance_index]->encode_context_ptr->initial_picture) ?
            picture_control_set_wrapper_ptr :
            sequence_control_set_ptr->encode_context_ptr->previous_picture_control_set_wrapper_ptr;

        sequence_control_set_ptr->encode_context_ptr->previous_picture_control_set_wrapper_ptr = picture_control_set_wrapper_ptr;

        // Copy data from the buffer to the input frame
        // *Note - Assumes 4:2:0 planar

        input_picture_wrapper_ptr = ebInputWrapperPtr;
        picture_control_set_ptr->enhanced_picture_ptr = (EbPictureBufferDesc*)eb_input_ptr->p_buffer;
        picture_control_set_ptr->eb_input_ptr = eb_input_ptr;
        end_of_sequence_flag = (picture_control_set_ptr->eb_input_ptr->flags & EB_BUFFERFLAG_EOS) ? EB_TRUE : EB_FALSE;
        svt_vp9_get_time(&picture_control_set_ptr->start_time_seconds,
                        &picture_control_set_ptr->start_timeu_seconds);

        picture_control_set_ptr->sequence_control_set_wrapper_ptr    = context_ptr->sequence_control_set_active_array[instance_index];
        picture_control_set_ptr->input_picture_wrapper_ptr          = input_picture_wrapper_ptr;
        picture_control_set_ptr->end_of_sequence_flag               = end_of_sequence_flag;

        // Set Picture Control Flags
        picture_control_set_ptr->idr_flag = sequence_control_set_ptr->encode_context_ptr->initial_picture || (picture_control_set_ptr->eb_input_ptr->pic_type == EB_IDR_PICTURE);
        picture_control_set_ptr->scene_change_flag                 = EB_FALSE;

        picture_control_set_ptr->qp_on_the_fly                      = EB_FALSE;

        picture_control_set_ptr->sb_total_count                      = sequence_control_set_ptr->sb_total_count;

        if (sequence_control_set_ptr->static_config.speed_control_flag) {
            eb_vp9_SpeedBufferControl(
                context_ptr,
                picture_control_set_ptr,
                sequence_control_set_ptr);
        }
        else {
            picture_control_set_ptr->enc_mode = (EB_ENC_MODE)sequence_control_set_ptr->static_config.enc_mode;
        }

        // Set the SCD Mode
        sequence_control_set_ptr->scd_mode = SCD_MODE_0;

        // Pre-Analysis Signal(s) derivation
        if (sequence_control_set_ptr->static_config.tune == TUNE_SQ) {
            eb_vp9_signal_derivation_pre_analysis_sq(
                sequence_control_set_ptr,
                picture_control_set_ptr);
        }
        else if (sequence_control_set_ptr->static_config.tune == TUNE_VMAF) {
            eb_vp9_signal_derivation_pre_analysis_vmaf(
                sequence_control_set_ptr,
                picture_control_set_ptr);
        }
        else {
            eb_vp9_signal_derivation_pre_analysis_oq(
                sequence_control_set_ptr,
                picture_control_set_ptr);
        }

        // Rate Control
        // Set the ME Distortion and OIS Historgrams to zero
        if (sequence_control_set_ptr->static_config.rate_control_mode){
                EB_MEMSET(picture_control_set_ptr->me_distortion_histogram, 0, NUMBER_OF_SAD_INTERVALS*sizeof(uint16_t));
                EB_MEMSET(picture_control_set_ptr->ois_distortion_histogram, 0, NUMBER_OF_INTRA_SAD_INTERVALS*sizeof(uint16_t));
        }
        picture_control_set_ptr->full_sb_count                    = 0;

        if (sequence_control_set_ptr->static_config.use_qp_file == 1) {
            picture_control_set_ptr->qp_on_the_fly = EB_TRUE;
            if (picture_control_set_ptr->eb_input_ptr->qp > MAX_QP_VALUE) {
                SVT_LOG("SVT [WARNING]: INPUT QP OUTSIDE OF RANGE\n");
                picture_control_set_ptr->qp_on_the_fly = EB_FALSE;
                picture_control_set_ptr->picture_qp = (uint8_t)sequence_control_set_ptr->qp;
            }
            picture_control_set_ptr->picture_qp = (uint8_t)picture_control_set_ptr->eb_input_ptr->qp;
        }
        else {
            picture_control_set_ptr->qp_on_the_fly = EB_FALSE;
            picture_control_set_ptr->picture_qp = (uint8_t)sequence_control_set_ptr->qp;
        }

        // Picture Stats
        picture_control_set_ptr->picture_number                   = context_ptr->picture_number_array[instance_index]++;

        // Set the picture structure: 0: progressive, 1: top, 2: bottom
        picture_control_set_ptr->pict_struct = PROGRESSIVE_PIC_STRUCT;

        sequence_control_set_ptr->encode_context_ptr->initial_picture = EB_FALSE;

        // Get Empty Reference Picture Object
        eb_vp9_get_empty_object(
            sequence_control_set_ptr->encode_context_ptr->pa_reference_picture_pool_fifo_ptr,
            &reference_picture_wrapper_ptr);

        picture_control_set_ptr->pareference_picture_wrapper_ptr = reference_picture_wrapper_ptr;

        // Give the new Reference a nominal live_count of 1
        eb_vp9_object_inc_live_count(
            picture_control_set_ptr->pareference_picture_wrapper_ptr,
            2);

        eb_vp9_object_inc_live_count(
            picture_control_set_wrapper_ptr,
            2);

        ((EbPaReferenceObject*)picture_control_set_ptr->pareference_picture_wrapper_ptr->object_ptr)->input_padded_picture_ptr->buffer_y = picture_control_set_ptr->enhanced_picture_ptr->buffer_y;
        // Get Empty Output Results Object
        if (picture_control_set_ptr->picture_number > 0 && prev_picture_control_set_wrapper_ptr != NULL)
        {
            ((PictureParentControlSet*)prev_picture_control_set_wrapper_ptr->object_ptr)->end_of_sequence_flag = end_of_sequence_flag;
            eb_vp9_get_empty_object(
                context_ptr->resource_coordination_results_output_fifo_ptr,
                &output_wrapper_ptr);
            output_results_ptr = (ResourceCoordinationResults*)output_wrapper_ptr->object_ptr;
            output_results_ptr->picture_control_set_wrapper_ptr = prev_picture_control_set_wrapper_ptr;

            // Post the finished Results Object
            eb_vp9_post_full_object(output_wrapper_ptr);
        }
        prev_picture_control_set_wrapper_ptr = picture_control_set_wrapper_ptr;

    }

    return EB_NULL;
}
