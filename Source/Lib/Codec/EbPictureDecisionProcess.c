/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"

#include "EbPictureAnalysisResults.h"
#include "EbPictureDecisionProcess.h"
#include "EbPictureDecisionResults.h"
#include "EbReferenceObject.h"
#include "EbComputeSAD.h"
#include "EbMeSadCalculation.h"

#include "EbSvtVp9ErrorCodes.h"
#include "vp9_pred_common.h"

/************************************************
 * Defines
 ************************************************/
#define POC_CIRCULAR_ADD(base, offset/*, bits*/)             (/*(((int32_t) (base)) + ((int32_t) (offset)) > ((int32_t) (1 << (bits))))   ? ((base) + (offset) - (1 << (bits))) : \
                                                             (((int32_t) (base)) + ((int32_t) (offset)) < 0)                           ? ((base) + (offset) + (1 << (bits))) : \
                                                                                                                                       */((base) + (offset)))
#define FUTURE_WINDOW_WIDTH                 4
#define FLASH_TH                            5
#define FADE_TH                             3
#define SCENE_TH                            3000
#define NOISY_SCENE_TH                      4500    // SCD TH in presence of noise
#define HIGH_PICTURE_VARIANCE_TH            1500
#define NUM64x64INPIC(w,h)          ((w*h)>> (LOG2F(MAX_SB_SIZE)<<1))
#define QUEUE_GET_PREVIOUS_SPOT(h)  ((h == 0) ? PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH - 1 : h - 1)
#define QUEUE_GET_NEXT_SPOT(h,off)  (( (h+off) >= PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH) ? h+off - PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH  : h + off)

#define WTH 64
#define OTH 64

/************************************************
 * Picture Analysis Context Constructor
 ************************************************/
EbErrorType eb_vp9_picture_decision_context_ctor(
    PictureDecisionContext **context_dbl_ptr,
    EbFifo                  *picture_analysis_results_input_fifo_ptr,
    EbFifo                  *picture_decision_results_output_fifo_ptr)
{
    PictureDecisionContext *context_ptr;
    uint32_t array_index;
    uint32_t array_row , arrow_column;
    EB_MALLOC(PictureDecisionContext*, context_ptr, sizeof(PictureDecisionContext), EB_N_PTR);
    *context_dbl_ptr = context_ptr;

    context_ptr->picture_analysis_results_input_fifo_ptr  = picture_analysis_results_input_fifo_ptr;
    context_ptr->picture_decision_results_output_fifo_ptr = picture_decision_results_output_fifo_ptr;

    EB_MALLOC(uint32_t**, context_ptr->ahd_running_avg_cb, sizeof(uint32_t*) * MAX_NUMBER_OF_REGIONS_IN_WIDTH, EB_N_PTR);

    EB_MALLOC(uint32_t**, context_ptr->ahd_running_avg_cr, sizeof(uint32_t*) * MAX_NUMBER_OF_REGIONS_IN_WIDTH, EB_N_PTR);

    EB_MALLOC(uint32_t**, context_ptr->ahd_running_avg, sizeof(uint32_t*) * MAX_NUMBER_OF_REGIONS_IN_WIDTH, EB_N_PTR);

    for (array_index = 0; array_index < MAX_NUMBER_OF_REGIONS_IN_WIDTH; array_index++)
    {
        EB_MALLOC(uint32_t*, context_ptr->ahd_running_avg_cb[array_index], sizeof(uint32_t) * MAX_NUMBER_OF_REGIONS_IN_HEIGHT, EB_N_PTR);

        EB_MALLOC(uint32_t*, context_ptr->ahd_running_avg_cr[array_index], sizeof(uint32_t) * MAX_NUMBER_OF_REGIONS_IN_HEIGHT, EB_N_PTR);

        EB_MALLOC(uint32_t*, context_ptr->ahd_running_avg[array_index], sizeof(uint32_t) * MAX_NUMBER_OF_REGIONS_IN_HEIGHT, EB_N_PTR);
    }

    for (array_row = 0; array_row < MAX_NUMBER_OF_REGIONS_IN_HEIGHT; array_row++)
    {
        for (arrow_column = 0; arrow_column < MAX_NUMBER_OF_REGIONS_IN_WIDTH; arrow_column++) {
            context_ptr->ahd_running_avg_cb[arrow_column][array_row] = 0;
            context_ptr->ahd_running_avg_cr[arrow_column][array_row] = 0;
            context_ptr->ahd_running_avg[arrow_column][array_row] = 0;
        }
    }

    context_ptr->reset_running_avg = EB_TRUE;

    return EB_ErrorNone;
}

EB_BOOL eb_vp9_SceneTransitionDetector(
    PictureDecisionContext   *context_ptr,
    SequenceControlSet       *sequence_control_set_ptr,
    PictureParentControlSet **parent_pcs_window,
    uint32_t                  window_width_future)
{
    PictureParentControlSet *previouspicture_control_set_ptr = parent_pcs_window[0];
    PictureParentControlSet *currentpicture_control_set_ptr = parent_pcs_window[1];
    PictureParentControlSet *futurepicture_control_set_ptr = parent_pcs_window[2];

    // calculating the frame threshold based on the number of 64x64 blocks in the frame
    uint32_t  region_thresh_hold;
    uint32_t  region_thresh_hold_chroma;
    // this variable determines whether the running average should be reset to equal the ahd or not after detecting a scene change.
    //EB_BOOL reset_running_avg = context_ptr->reset_running_avg;

    EB_BOOL is_abrupt_change;  // this variable signals an abrubt change (scene change or flash)
    EB_BOOL is_scene_change;   // this variable signals a frame representing a scene change
    EB_BOOL is_flash;          // this variable signals a frame that contains a flash
    EB_BOOL is_fade;           // this variable signals a frame that contains a fade
    EB_BOOL gradual_change;    // this signals the detection of a light scene change a small/localized flash or the start of a fade

    uint32_t  ahd;             // accumulative histogram (absolute) differences between the past and current frame

    uint32_t  ahd_cb;
    uint32_t  ahd_cr;

    uint32_t  ahd_error_cb = 0;
    uint32_t  ahd_error_cr = 0;

    uint32_t **ahd_running_avg_cb = context_ptr->ahd_running_avg_cb;
    uint32_t **ahd_running_avg_cr = context_ptr->ahd_running_avg_cr;
    uint32_t **ahd_running_avg = context_ptr->ahd_running_avg;

    uint32_t  ahd_error = 0; // the difference between the ahd and the running average at the current frame.

    uint8_t   aid_future_past = 0; // this variable denotes the average intensity difference between the next and the past frames
    uint8_t   aid_future_present = 0;
    uint8_t   aid_present_past = 0;

    uint32_t  bin = 0; // variable used to iterate through the bins of the histograms

    uint32_t  region_in_picture_width_index;
    uint32_t  region_in_picture_height_index;

    uint32_t  region_width;
    uint32_t  region_height;
    uint32_t  region_width_offset;
    uint32_t  region_height_offset;

    uint32_t  is_abrupt_change_count = 0;
    uint32_t  is_scene_change_count = 0;

    uint32_t  region_count_threshold = (sequence_control_set_ptr->scd_mode == SCD_MODE_2) ?
        (uint32_t)(((float)((sequence_control_set_ptr->picture_analysis_number_of_regions_per_width * sequence_control_set_ptr->picture_analysis_number_of_regions_per_height) * 75) / 100) + 0.5) :
        (uint32_t)(((float)((sequence_control_set_ptr->picture_analysis_number_of_regions_per_width * sequence_control_set_ptr->picture_analysis_number_of_regions_per_height) * 50) / 100) + 0.5) ;

    region_width = parent_pcs_window[1]->enhanced_picture_ptr->width / sequence_control_set_ptr->picture_analysis_number_of_regions_per_width;
    region_height = parent_pcs_window[1]->enhanced_picture_ptr->height / sequence_control_set_ptr->picture_analysis_number_of_regions_per_height;

    // Loop over regions inside the picture
    for (region_in_picture_width_index = 0; region_in_picture_width_index < sequence_control_set_ptr->picture_analysis_number_of_regions_per_width; region_in_picture_width_index++){  // loop over horizontal regions
        for (region_in_picture_height_index = 0; region_in_picture_height_index < sequence_control_set_ptr->picture_analysis_number_of_regions_per_height; region_in_picture_height_index++){ // loop over vertical regions

            is_abrupt_change = EB_FALSE;
            is_scene_change = EB_FALSE;
            is_flash = EB_FALSE;
            gradual_change = EB_FALSE;

            // Reset accumulative histogram (absolute) differences between the past and current frame
            ahd = 0;
            ahd_cb = 0;
            ahd_cr = 0;

            region_width_offset = (region_in_picture_width_index == sequence_control_set_ptr->picture_analysis_number_of_regions_per_width - 1) ?
                parent_pcs_window[1]->enhanced_picture_ptr->width - (sequence_control_set_ptr->picture_analysis_number_of_regions_per_width * region_width) :
                0;

            region_height_offset = (region_in_picture_height_index == sequence_control_set_ptr->picture_analysis_number_of_regions_per_height - 1) ?
                parent_pcs_window[1]->enhanced_picture_ptr->height - (sequence_control_set_ptr->picture_analysis_number_of_regions_per_height * region_height) :
                0;

            region_width += region_width_offset;
            region_height += region_height_offset;

            region_thresh_hold = (
                // Noise insertion/removal detection
                ((ABS((int64_t)currentpicture_control_set_ptr->pic_avg_variance - (int64_t)previouspicture_control_set_ptr->pic_avg_variance)) > NOISE_VARIANCE_TH) &&
                (currentpicture_control_set_ptr->pic_avg_variance > HIGH_PICTURE_VARIANCE_TH || previouspicture_control_set_ptr->pic_avg_variance > HIGH_PICTURE_VARIANCE_TH)) ?
                NOISY_SCENE_TH * NUM64x64INPIC(region_width, region_height) : // SCD TH function of noise insertion/removal.
                SCENE_TH * NUM64x64INPIC(region_width, region_height) ;

            region_thresh_hold_chroma = region_thresh_hold / 4;

            for (bin = 0; bin < HISTOGRAM_NUMBER_OF_BINS; ++bin) {
                ahd += ABS((int32_t)currentpicture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][0][bin] - (int32_t)previouspicture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][0][bin]);
                ahd_cb += ABS((int32_t)currentpicture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][1][bin] - (int32_t)previouspicture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][1][bin]);
                ahd_cr += ABS((int32_t)currentpicture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][2][bin] - (int32_t)previouspicture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][2][bin]);

            }

            if (context_ptr->reset_running_avg){
                ahd_running_avg[region_in_picture_width_index][region_in_picture_height_index] = ahd;
                ahd_running_avg_cb[region_in_picture_width_index][region_in_picture_height_index] = ahd_cb;
                ahd_running_avg_cr[region_in_picture_width_index][region_in_picture_height_index] = ahd_cr;
            }

            ahd_error = ABS((int32_t)ahd_running_avg[region_in_picture_width_index][region_in_picture_height_index] - (int32_t)ahd);
            ahd_error_cb = ABS((int32_t)ahd_running_avg_cb[region_in_picture_width_index][region_in_picture_height_index] - (int32_t)ahd_cb);
            ahd_error_cr = ABS((int32_t)ahd_running_avg_cr[region_in_picture_width_index][region_in_picture_height_index] - (int32_t)ahd_cr);

            if ((ahd_error   > region_thresh_hold       && ahd >= ahd_error) ||
                (ahd_error_cb > region_thresh_hold_chroma && ahd_cb >= ahd_error_cb) ||
                (ahd_error_cr > region_thresh_hold_chroma && ahd_cr >= ahd_error_cr)){

                is_abrupt_change = EB_TRUE;

            }
            else if ((ahd_error > (region_thresh_hold >> 1)) && ahd >= ahd_error){
                gradual_change = EB_TRUE;
            }

            if (is_abrupt_change)
            {
                aid_future_past = (uint8_t) ABS((int16_t)futurepicture_control_set_ptr->average_intensity_per_region[region_in_picture_width_index][region_in_picture_height_index][0] - (int16_t)previouspicture_control_set_ptr->average_intensity_per_region[region_in_picture_width_index][region_in_picture_height_index][0]);
                aid_future_present = (uint8_t)ABS((int16_t)futurepicture_control_set_ptr->average_intensity_per_region[region_in_picture_width_index][region_in_picture_height_index][0] - (int16_t)currentpicture_control_set_ptr->average_intensity_per_region[region_in_picture_width_index][region_in_picture_height_index][0]);
                aid_present_past = (uint8_t)ABS((int16_t)currentpicture_control_set_ptr->average_intensity_per_region[region_in_picture_width_index][region_in_picture_height_index][0] - (int16_t)previouspicture_control_set_ptr->average_intensity_per_region[region_in_picture_width_index][region_in_picture_height_index][0]);

                if (aid_future_past < FLASH_TH && aid_future_present >= FLASH_TH && aid_present_past >= FLASH_TH){
                    is_flash = EB_TRUE;
                    //SVT_LOG ("\nFlash in frame# %i , %i\n", currentpicture_control_set_ptr->picture_number,aid_future_past);
                }
                else if (aid_future_present < FADE_TH && aid_present_past < FADE_TH){
                    is_fade = EB_TRUE;
                    //SVT_LOG ("\nFlash in frame# %i , %i\n", currentpicture_control_set_ptr->picture_number,aid_future_past);
                } else {
                    is_scene_change = EB_TRUE;
                    //SVT_LOG ("\nScene Change in frame# %i , %i\n", currentpicture_control_set_ptr->picture_number,aid_future_past);
                }

            }
            else if (gradual_change){

                aid_future_past = (uint8_t) ABS((int16_t)futurepicture_control_set_ptr->average_intensity_per_region[region_in_picture_width_index][region_in_picture_height_index][0] - (int16_t)previouspicture_control_set_ptr->average_intensity_per_region[region_in_picture_width_index][region_in_picture_height_index][0]);
                if (aid_future_past < FLASH_TH){
                    // proper action to be signalled
                    //SVT_LOG ("\nLight Flash in frame# %i , %i\n", currentpicture_control_set_ptr->picture_number,aid_future_past);
                    ahd_running_avg[region_in_picture_width_index][region_in_picture_height_index] = (3 * ahd_running_avg[region_in_picture_width_index][region_in_picture_height_index] + ahd) / 4;
                }
                else{
                    // proper action to be signalled
                    //SVT_LOG ("\nLight Scene Change / fade detected in frame# %i , %i\n", currentpicture_control_set_ptr->picture_number,aid_future_past);
                    ahd_running_avg[region_in_picture_width_index][region_in_picture_height_index] = (3 * ahd_running_avg[region_in_picture_width_index][region_in_picture_height_index] + ahd) / 4;
                }

            }
            else{
                ahd_running_avg[region_in_picture_width_index][region_in_picture_height_index] = (3 * ahd_running_avg[region_in_picture_width_index][region_in_picture_height_index] + ahd) / 4;
            }

            is_abrupt_change_count += is_abrupt_change;
            is_scene_change_count += is_scene_change;
        }
    }

    (void)window_width_future;
    (void)is_flash;
    (void)is_fade;

    if (is_abrupt_change_count >= region_count_threshold) {
        context_ptr->reset_running_avg = EB_TRUE;
    }
    else {
        context_ptr->reset_running_avg = EB_FALSE;
    }

    if ((is_scene_change_count >= region_count_threshold)){

        return(EB_TRUE);
    }
    else {
        return(EB_FALSE);
    }

}

/***************************************************************************************************
* release_prev_picture_from_reorder_queue
***************************************************************************************************/
static EbErrorType release_prev_picture_from_reorder_queue(
    EncodeContext *encode_context_ptr) {

    EbErrorType return_error = EB_ErrorNone;

    PictureDecisionReorderEntry *queue_previous_entry_ptr;
    int32_t                      previous_entry_index;

    // Get the previous entry from the Picture Decision Reordering Queue (Entry N-1)
    // P.S. The previous entry in display order is needed for Scene Change Detection
    previous_entry_index = (encode_context_ptr->picture_decision_reorder_queue_head_index == 0) ? PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH - 1 : encode_context_ptr->picture_decision_reorder_queue_head_index - 1;
    queue_previous_entry_ptr = encode_context_ptr->picture_decision_reorder_queue[previous_entry_index];

    // LCU activity classification based on (0,0) SAD & picture activity derivation
    if (queue_previous_entry_ptr->parent_pcs_wrapper_ptr) {

        // Reset the Picture Decision Reordering Queue Entry
        // P.S. The reset of the Picture Decision Reordering Queue Entry could not be done before running the Scene Change Detector
        queue_previous_entry_ptr->picture_number += PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH;
        queue_previous_entry_ptr->parent_pcs_wrapper_ptr = (EbObjectWrapper *)EB_NULL;
    }

    return return_error;
}
#if NEW_PRED_STRUCT

/***************************************************************************************************
* Initializes mini GOP activity array
*
***************************************************************************************************/
EbErrorType eb_vp9_initialize_mini_gop_activity_array(
    PictureDecisionContext        *context_ptr) {

    EbErrorType return_error = EB_ErrorNone;

    uint32_t mini_gop_index;

    // Loop over all mini GOPs
    for (mini_gop_index = 0; mini_gop_index < MINI_GOP_MAX_COUNT; ++mini_gop_index) {

        context_ptr->mini_gop_activity_array[mini_gop_index] = (eb_vp9_get_mini_gop_stats(mini_gop_index)->hierarchical_levels == MIN_HIERARCHICAL_LEVEL) ?
            EB_FALSE :
            EB_TRUE;

    }

    return return_error;
}

/***************************************************************************************************
* Generates block picture map
*
*
***************************************************************************************************/
EbErrorType eb_vp9_generate_picture_window_split(
    PictureDecisionContext        *context_ptr,
    EncodeContext                 *encode_context_ptr) {

    EbErrorType return_error = EB_ErrorNone;

    uint32_t    mini_gop_index;

    context_ptr->total_number_of_mini_gops = 0;

    // Loop over all mini GOPs
    mini_gop_index = 0;
    while (mini_gop_index < MINI_GOP_MAX_COUNT) {

        // Only for a valid mini GOP
        if (eb_vp9_get_mini_gop_stats(mini_gop_index)->end_index < encode_context_ptr->pre_assignment_buffer_count && context_ptr->mini_gop_activity_array[mini_gop_index] == EB_FALSE) {

            context_ptr->mini_gop_start_index[context_ptr->total_number_of_mini_gops] = eb_vp9_get_mini_gop_stats(mini_gop_index)->start_index;
            context_ptr->mini_gop_end_index[context_ptr->total_number_of_mini_gops] = eb_vp9_get_mini_gop_stats(mini_gop_index)->end_index;
            context_ptr->mini_gop_length[context_ptr->total_number_of_mini_gops] = eb_vp9_get_mini_gop_stats(mini_gop_index)->lenght;
            context_ptr->mini_gop_hierarchical_levels[context_ptr->total_number_of_mini_gops] = eb_vp9_get_mini_gop_stats(mini_gop_index)->hierarchical_levels;
            context_ptr->mini_gop_intra_count[context_ptr->total_number_of_mini_gops] = 0;
            context_ptr->mini_gop_idr_count[context_ptr->total_number_of_mini_gops] = 0;

            context_ptr->total_number_of_mini_gops++;
        }

        mini_gop_index += context_ptr->mini_gop_activity_array[mini_gop_index] ?
            1 :
            mini_gop_offset[eb_vp9_get_mini_gop_stats(mini_gop_index)->hierarchical_levels - MIN_HIERARCHICAL_LEVEL];

    }

    // Only in presence of at least 1 valid mini GOP
    if (context_ptr->total_number_of_mini_gops != 0) {
        context_ptr->mini_gop_intra_count[context_ptr->total_number_of_mini_gops - 1] = encode_context_ptr->pre_assignment_buffer_intra_count;
        context_ptr->mini_gop_idr_count[context_ptr->total_number_of_mini_gops - 1] = encode_context_ptr->pre_assignment_buffer_idr_count;
    }

    return return_error;
}

/***************************************************************************************************
* Handles an incomplete picture window map
*
*
***************************************************************************************************/
EbErrorType eb_vp9_handle_incomplete_picture_window_map(
    PictureDecisionContext        *context_ptr,
    EncodeContext                 *encode_context_ptr) {

    EbErrorType return_error = EB_ErrorNone;

    if (context_ptr->total_number_of_mini_gops == 0) {

        context_ptr->mini_gop_start_index[context_ptr->total_number_of_mini_gops] = 0;
        context_ptr->mini_gop_end_index[context_ptr->total_number_of_mini_gops] = encode_context_ptr->pre_assignment_buffer_count - 1;
        context_ptr->mini_gop_length[context_ptr->total_number_of_mini_gops] = encode_context_ptr->pre_assignment_buffer_count - context_ptr->mini_gop_start_index[context_ptr->total_number_of_mini_gops];
        context_ptr->mini_gop_hierarchical_levels[context_ptr->total_number_of_mini_gops] = 3;// MIN_HIERARCHICAL_LEVEL; // AMIR to be updated after other predictions are supported

        context_ptr->total_number_of_mini_gops++;

    }
    else if (context_ptr->mini_gop_end_index[context_ptr->total_number_of_mini_gops - 1] < encode_context_ptr->pre_assignment_buffer_count - 1) {

        context_ptr->mini_gop_start_index[context_ptr->total_number_of_mini_gops] = context_ptr->mini_gop_end_index[context_ptr->total_number_of_mini_gops - 1] + 1;
        context_ptr->mini_gop_end_index[context_ptr->total_number_of_mini_gops] = encode_context_ptr->pre_assignment_buffer_count - 1;
        context_ptr->mini_gop_length[context_ptr->total_number_of_mini_gops] = encode_context_ptr->pre_assignment_buffer_count - context_ptr->mini_gop_start_index[context_ptr->total_number_of_mini_gops];
        context_ptr->mini_gop_hierarchical_levels[context_ptr->total_number_of_mini_gops] = 3;// MIN_HIERARCHICAL_LEVEL;// AMIR
        context_ptr->mini_gop_intra_count[context_ptr->total_number_of_mini_gops - 1] = 0;
        context_ptr->mini_gop_idr_count[context_ptr->total_number_of_mini_gops - 1] = 0;

        context_ptr->total_number_of_mini_gops++;
    }

    context_ptr->mini_gop_intra_count[context_ptr->total_number_of_mini_gops - 1] = encode_context_ptr->pre_assignment_buffer_intra_count;
    context_ptr->mini_gop_idr_count[context_ptr->total_number_of_mini_gops - 1] = encode_context_ptr->pre_assignment_buffer_idr_count;

    return return_error;
}
/***************************************************************************************************
* If a switch happens, then update the RPS of the base layer frame separating the 2 different prediction structures
* Clean up the reference queue dependant counts of the base layer frame separating the 2 different prediction structures
*
***************************************************************************************************/
static EbErrorType update_base_layer_reference_queue_dependent_count(
    PictureDecisionContext        *context_ptr,
    EncodeContext                 *encode_context_ptr,
    uint32_t                       mini_gop_index) {

    EbErrorType return_error = EB_ErrorNone;

    PaReferenceQueueEntry         *inputEntryPtr;
    uint32_t                       inputQueueIndex;

    PredictionStructure          *nextPredStructPtr;
    PredictionStructureEntry      *nextBaseLayerPredPositionPtr;

    uint32_t                       dependantListPositiveEntries;
    uint32_t                       dependantListRemovedEntries;
    uint32_t                       depListCount;

    uint32_t                       depIdx;
    uint64_t                       depPoc;

    PictureParentControlSet       *picture_control_set_ptr;

    // Get the 1st PCS mini GOP
    picture_control_set_ptr = (PictureParentControlSet*)encode_context_ptr->pre_assignment_buffer[context_ptr->mini_gop_start_index[mini_gop_index]]->object_ptr;

    // Derive the temporal layer difference between the current mini GOP and the previous mini GOP
    picture_control_set_ptr->hierarchical_layers_diff = (uint8_t)(encode_context_ptr->previous_mini_gop_hierarchical_levels - picture_control_set_ptr->hierarchical_levels);

    // Set init_pred_struct_position_flag to TRUE if mini GOP switch
    picture_control_set_ptr->init_pred_struct_position_flag = (picture_control_set_ptr->hierarchical_layers_diff != 0) ?
        EB_TRUE :
        EB_FALSE;

    // If the current mini GOP is different than the previous mini GOP update then update the positive dependant counts of the reference entry separating the 2 mini GOPs
    if (picture_control_set_ptr->hierarchical_layers_diff != 0) {

        inputQueueIndex = encode_context_ptr->picture_decision_pa_reference_queue_head_index;

        while (inputQueueIndex != encode_context_ptr->picture_decision_pa_reference_queue_tail_index) {

            inputEntryPtr = encode_context_ptr->picture_decision_pa_reference_queue[inputQueueIndex];

            // Find the reference entry separating the 2 mini GOPs  (picture_control_set_ptr->picture_number is the POC of the first isput in the mini GOP)
            if (inputEntryPtr->picture_number == (picture_control_set_ptr->picture_number - 1)) {

                // Update the positive dependant counts

                // 1st step: remove all positive entries from the dependant list0 and dependant list1
                dependantListPositiveEntries = 0;
                for (depIdx = 0; depIdx < inputEntryPtr->list0.list_count; ++depIdx) {
                    if (inputEntryPtr->list0.list[depIdx] >= 0) {
                        dependantListPositiveEntries++;
                    }
                }
                inputEntryPtr->list0.list_count = inputEntryPtr->list0.list_count - dependantListPositiveEntries;
                dependantListPositiveEntries = 0;
                for (depIdx = 0; depIdx < inputEntryPtr->list1.list_count; ++depIdx) {
                    if (inputEntryPtr->list1.list[depIdx] >= 0) {
                        dependantListPositiveEntries++;
                    }
                }
                inputEntryPtr->list1.list_count = inputEntryPtr->list1.list_count - dependantListPositiveEntries;

                // 2nd step: inherit the positive dependant counts of the current mini GOP
                // Get the RPS set of the current mini GOP
                nextPredStructPtr = eb_vp9_get_prediction_structure(
                    encode_context_ptr->prediction_structure_group_ptr,
                    picture_control_set_ptr->pred_structure,
                    1,
                    picture_control_set_ptr->hierarchical_levels);            // Number of temporal layer in the current mini GOP

                // Get the RPS of a base layer input
                nextBaseLayerPredPositionPtr = nextPredStructPtr->pred_struct_entry_ptr_array[nextPredStructPtr->pred_struct_entry_count - 1];

                for (depIdx = 0; depIdx < nextBaseLayerPredPositionPtr->dep_list0.list_count; ++depIdx) {
                    if (nextBaseLayerPredPositionPtr->dep_list0.list[depIdx] >= 0) {
                        inputEntryPtr->list0.list[inputEntryPtr->list0.list_count++] = nextBaseLayerPredPositionPtr->dep_list0.list[depIdx];
                    }
                }

                for (depIdx = 0; depIdx < nextBaseLayerPredPositionPtr->dep_list1.list_count; ++depIdx) {
                    if (nextBaseLayerPredPositionPtr->dep_list1.list[depIdx] >= 0) {
                        inputEntryPtr->list1.list[inputEntryPtr->list1.list_count++] = nextBaseLayerPredPositionPtr->dep_list1.list[depIdx];
                    }
                }

                // 3rd step: update the dependant count
                dependantListRemovedEntries = inputEntryPtr->dep_list0_count + inputEntryPtr->dep_list1_count - inputEntryPtr->dependent_count;
                inputEntryPtr->dep_list0_count = inputEntryPtr->list0.list_count;
                inputEntryPtr->dep_list1_count = inputEntryPtr->list1.list_count;
                inputEntryPtr->dependent_count = inputEntryPtr->dep_list0_count + inputEntryPtr->dep_list1_count - dependantListRemovedEntries;

            }
            else {

                // Modify Dependent List0
                depListCount = inputEntryPtr->list0.list_count;
                for (depIdx = 0; depIdx < depListCount; ++depIdx) {

                    // Adjust the latest currentInputPoc in case we're in a POC rollover scenario
                    // currentInputPoc += (currentInputPoc < inputEntryPtr->pocNumber) ? (1 << sequence_control_set_ptr->bitsForPictureOrderCount) : 0;

                    depPoc = POC_CIRCULAR_ADD(
                        inputEntryPtr->picture_number, // can't use a value that gets reset
                        inputEntryPtr->list0.list[depIdx]/*,
                                                         sequence_control_set_ptr->bitsForPictureOrderCount*/);

                                                         // If Dependent POC is greater or equal to the IDR POC
                    if (depPoc >= picture_control_set_ptr->picture_number && inputEntryPtr->list0.list[depIdx]) {

                        inputEntryPtr->list0.list[depIdx] = 0;

                        // Decrement the Reference's reference_count
                        --inputEntryPtr->dependent_count;

                        CHECK_REPORT_ERROR(
                            (inputEntryPtr->dependent_count != ~0u),
                            encode_context_ptr->app_callback_ptr,
                            EB_ENC_PD_ERROR3);
                    }
                }

                // Modify Dependent List1
                depListCount = inputEntryPtr->list1.list_count;
                for (depIdx = 0; depIdx < depListCount; ++depIdx) {

                    // Adjust the latest currentInputPoc in case we're in a POC rollover scenario
                    // currentInputPoc += (currentInputPoc < inputEntryPtr->pocNumber) ? (1 << sequence_control_set_ptr->bitsForPictureOrderCount) : 0;

                    depPoc = POC_CIRCULAR_ADD(
                        inputEntryPtr->picture_number,
                        inputEntryPtr->list1.list[depIdx]/*,
                                                         sequence_control_set_ptr->bitsForPictureOrderCount*/);

                                                         // If Dependent POC is greater or equal to the IDR POC
                    if ((depPoc >= picture_control_set_ptr->picture_number) && inputEntryPtr->list1.list[depIdx]) {
                        inputEntryPtr->list1.list[depIdx] = 0;

                        // Decrement the Reference's reference_count
                        --inputEntryPtr->dependent_count;

                        CHECK_REPORT_ERROR(
                            (inputEntryPtr->dependent_count != ~0u),
                            encode_context_ptr->app_callback_ptr,
                            EB_ENC_PD_ERROR3);
                    }
                }
            }

            // Increment the inputQueueIndex Iterator
            inputQueueIndex = (inputQueueIndex == PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : inputQueueIndex + 1;
        }
    }

    return return_error;
}

#endif

/***************************************************************************************************
* Generates mini GOP RPSs
*
*
***************************************************************************************************/
static EbErrorType generate_mini_gop_rps(
    PictureDecisionContext *context_ptr,
    EncodeContext          *encode_context_ptr) {

    EbErrorType return_error = EB_ErrorNone;

    uint32_t                 mini_gop_index;
    PictureParentControlSet *picture_control_set_ptr;
    uint32_t                 picture_index;

    SequenceControlSet      *sequence_control_set_ptr;

    // Loop over all mini GOPs
    for (mini_gop_index = 0; mini_gop_index < context_ptr->total_number_of_mini_gops; ++mini_gop_index) {

        // Loop over picture within the mini GOP
        for (picture_index = context_ptr->mini_gop_start_index[mini_gop_index]; picture_index <= context_ptr->mini_gop_end_index[mini_gop_index]; picture_index++) {

            picture_control_set_ptr    = (PictureParentControlSet  *)    encode_context_ptr->pre_assignment_buffer[picture_index]->object_ptr;
            sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
            picture_control_set_ptr->pred_structure = sequence_control_set_ptr->static_config.pred_structure;

            picture_control_set_ptr->hierarchical_levels = (uint8_t)context_ptr->mini_gop_hierarchical_levels[mini_gop_index];

            picture_control_set_ptr->pred_struct_ptr = eb_vp9_get_prediction_structure(
                encode_context_ptr->prediction_structure_group_ptr,
                picture_control_set_ptr->pred_structure,
                1,
                picture_control_set_ptr->hierarchical_levels);
        }
    }
    return return_error;
}

/***************************************************************************
* Set the default subPel enble/disable flag for each frame
****************************************************************************/

uint8_t picture_level_sub_pel_settings_oq(
    uint8_t input_resolution,
    uint8_t enc_mode,
    uint8_t temporal_layer_index,
    EB_BOOL is_used_as_reference_flag) {

    uint8_t sub_pel_mode;

    if (enc_mode <= ENC_MODE_8) {
        sub_pel_mode = 1;
    }
    else if (enc_mode <= ENC_MODE_9) {
        if (input_resolution >= INPUT_SIZE_4K_RANGE) {
            sub_pel_mode = (temporal_layer_index == 0) ? 1 : 0;
        }
        else {
            sub_pel_mode = 1;
        }
    }
    else {
        if (input_resolution >= INPUT_SIZE_4K_RANGE) {
            sub_pel_mode = (temporal_layer_index == 0) ? 1 : 0;
        }
        else {
            sub_pel_mode = is_used_as_reference_flag ? 1 : 0;
        }
    }

    return sub_pel_mode;
}

uint8_t picture_level_sub_pel_settings_vmaf(
    uint8_t input_resolution,
    uint8_t enc_mode,
    uint8_t temporal_layer_index) {

    uint8_t sub_pel_mode;

    if (enc_mode <= ENC_MODE_8) {
        sub_pel_mode = 1;
    }
    else {
        if (input_resolution >= INPUT_SIZE_4K_RANGE) {
            sub_pel_mode = (temporal_layer_index == 0) ? 1 : 0;
        }
        else {
            sub_pel_mode = 1;
        }
    }

    return sub_pel_mode;
}

uint8_t picture_level_sub_pel_settings_sq(
    uint8_t input_resolution,
    uint8_t enc_mode,
    uint8_t temporal_layer_index,
    EB_BOOL is_used_as_reference_flag) {
    uint8_t sub_pel_mode;

    if (input_resolution >= INPUT_SIZE_4K_RANGE) {
        if (enc_mode <= ENC_MODE_4) {
            sub_pel_mode = 1;
        }
        else if (enc_mode <= ENC_MODE_7) {
            sub_pel_mode = is_used_as_reference_flag ? 1 : 0;
        }
        else if (enc_mode <= ENC_MODE_10) {
            sub_pel_mode = (temporal_layer_index == 0) ? 1 : 0;
        }
        else {
            sub_pel_mode = 0;
        }
    }
    else {
        if (enc_mode <= ENC_MODE_4) {
            sub_pel_mode = 1;
        }
        else if (enc_mode <= ENC_MODE_8) {
            sub_pel_mode = is_used_as_reference_flag ? 1 : 0;
        }
        else if (enc_mode <= ENC_MODE_9) {
            sub_pel_mode = (temporal_layer_index == 0) ? 1 : 0;
        }
        else {
            sub_pel_mode = 0;
        }
    }

    return sub_pel_mode;
}

/******************************************************
* Derive Multi-Processes Settings for SQ
Input   : encoder mode and tune
Output  : Multi-Processes signal(s)
******************************************************/
EbErrorType eb_vp9_signal_derivation_multi_processes_sq(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr) {

    EbErrorType return_error = EB_ErrorNone;

    // Set MD Partitioning Method
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_3) {
        if (picture_control_set_ptr->slice_type == I_SLICE) {
            picture_control_set_ptr->pic_depth_mode = PIC_FULL84_DEPTH_MODE;
        }
        else {
            picture_control_set_ptr->pic_depth_mode = PIC_FULL85_DEPTH_MODE;
        }
    }
    else if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
        if (sequence_control_set_ptr->input_resolution == INPUT_SIZE_4K_RANGE) {
            if (picture_control_set_ptr->slice_type == I_SLICE) {
                picture_control_set_ptr->pic_depth_mode = PIC_FULL84_DEPTH_MODE;
            }
            else {
                picture_control_set_ptr->pic_depth_mode = PIC_FULL85_DEPTH_MODE;
            }
        }
        else {
            if (picture_control_set_ptr->slice_type == I_SLICE) {
                picture_control_set_ptr->pic_depth_mode = PIC_FULL84_DEPTH_MODE;
            }
            else {
                picture_control_set_ptr->pic_depth_mode = PIC_SB_SWITCH_DEPTH_MODE;
            }
        }
    }
    else if (picture_control_set_ptr->enc_mode <= ENC_MODE_9) {
        if (picture_control_set_ptr->slice_type == I_SLICE) {
            picture_control_set_ptr->pic_depth_mode = PIC_FULL84_DEPTH_MODE;
        }
        else {
            picture_control_set_ptr->pic_depth_mode = PIC_SB_SWITCH_DEPTH_MODE;
        }
    }
    else {
        if (picture_control_set_ptr->slice_type == I_SLICE) {
            if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080p_RANGE) {
                picture_control_set_ptr->pic_depth_mode = PIC_FULL84_DEPTH_MODE;
            }
            else {
                picture_control_set_ptr->pic_depth_mode = PIC_BDP_DEPTH_MODE;
            }
        }
        else {
            picture_control_set_ptr->pic_depth_mode = PIC_SB_SWITCH_DEPTH_MODE;
        }
    }

    // Set the default settings of  subpel
    picture_control_set_ptr->use_subpel_flag = picture_level_sub_pel_settings_sq(
        sequence_control_set_ptr->input_resolution,
        picture_control_set_ptr->enc_mode,
        picture_control_set_ptr->temporal_layer_index,
        picture_control_set_ptr->is_used_as_reference_flag);

    // CU_8x8 Search Mode
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_1) {
        picture_control_set_ptr->cu8x8_mode = CU_8x8_MODE_0;
    }
    else if (picture_control_set_ptr->enc_mode <= ENC_MODE_6) {
        picture_control_set_ptr->cu8x8_mode = (picture_control_set_ptr->is_used_as_reference_flag) ? CU_8x8_MODE_0 : CU_8x8_MODE_1;
    }
    else if (picture_control_set_ptr->enc_mode == ENC_MODE_7) {
        picture_control_set_ptr->cu8x8_mode = (picture_control_set_ptr->temporal_layer_index == 0) ? CU_8x8_MODE_0 : CU_8x8_MODE_1;
    }
    else {
        picture_control_set_ptr->cu8x8_mode = CU_8x8_MODE_1;
    }

    // CU_16x16 Search Mode
    picture_control_set_ptr->cu16x16_mode = CU_16x16_MODE_0;

    return return_error;
}

/******************************************************
* Derive Multi-Processes Settings for OQ
Input   : encoder mode and tune
Output  : Multi-Processes signal(s)
******************************************************/
EbErrorType eb_vp9_signal_derivation_multi_processes_oq(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr) {

    EbErrorType return_error = EB_ErrorNone;

    // Set MD Partitioning Method
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_3) {
        if (picture_control_set_ptr->slice_type == I_SLICE) {
            picture_control_set_ptr->pic_depth_mode = PIC_FULL84_DEPTH_MODE;
        }
        else {
            picture_control_set_ptr->pic_depth_mode = PIC_FULL85_DEPTH_MODE;
        }
    }
    else {
        if (picture_control_set_ptr->slice_type == I_SLICE) {
            picture_control_set_ptr->pic_depth_mode = PIC_FULL84_DEPTH_MODE;

        }
        else {
            picture_control_set_ptr->pic_depth_mode = PIC_SB_SWITCH_DEPTH_MODE;
        }
    }

    // Set the default settings of  subpel
    picture_control_set_ptr->use_subpel_flag = picture_level_sub_pel_settings_oq(
        sequence_control_set_ptr->input_resolution,
        picture_control_set_ptr->enc_mode,
        picture_control_set_ptr->temporal_layer_index,
        picture_control_set_ptr->is_used_as_reference_flag);

    // CU_8x8 Search Mode
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_1) {
        picture_control_set_ptr->cu8x8_mode = CU_8x8_MODE_0;
    }
    else if (picture_control_set_ptr->enc_mode <= ENC_MODE_6) {
        picture_control_set_ptr->cu8x8_mode = (picture_control_set_ptr->is_used_as_reference_flag) ? CU_8x8_MODE_0 : CU_8x8_MODE_1;
    }
    else if (picture_control_set_ptr->enc_mode == ENC_MODE_7) {
        picture_control_set_ptr->cu8x8_mode = (picture_control_set_ptr->temporal_layer_index == 0) ? CU_8x8_MODE_0 : CU_8x8_MODE_1;
    }
    else {
        picture_control_set_ptr->cu8x8_mode = CU_8x8_MODE_1;
    }

    // CU_16x16 Search Mode
    picture_control_set_ptr->cu16x16_mode = CU_16x16_MODE_0;

    return return_error;
}

/******************************************************
* Derive Multi-Processes Settings for VMAF
Input   : encoder mode and tune
Output  : Multi-Processes signal(s)
******************************************************/
EbErrorType eb_vp9_signal_derivation_multi_processes_vmaf(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr) {

    EbErrorType return_error = EB_ErrorNone;

    // Set MD Partitioning Method
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_1) {
        if (picture_control_set_ptr->slice_type == I_SLICE) {
            picture_control_set_ptr->pic_depth_mode = PIC_FULL84_DEPTH_MODE;
        }
        else {
            picture_control_set_ptr->pic_depth_mode = PIC_FULL85_DEPTH_MODE;
        }
    }
    else {
        if (picture_control_set_ptr->slice_type == I_SLICE) {
            picture_control_set_ptr->pic_depth_mode = PIC_FULL84_DEPTH_MODE;
        }
        else {
            picture_control_set_ptr->pic_depth_mode = PIC_SB_SWITCH_DEPTH_MODE;
        }
    }

    // Set the default settings of  subpel
    picture_control_set_ptr->use_subpel_flag = picture_level_sub_pel_settings_vmaf(
        sequence_control_set_ptr->input_resolution,
        picture_control_set_ptr->enc_mode,
        picture_control_set_ptr->temporal_layer_index);

    // CU_8x8 Search Mode
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_1) {
        picture_control_set_ptr->cu8x8_mode = CU_8x8_MODE_0;
    }
    else if (picture_control_set_ptr->enc_mode <= ENC_MODE_6) {
        picture_control_set_ptr->cu8x8_mode = (picture_control_set_ptr->is_used_as_reference_flag) ? CU_8x8_MODE_0 : CU_8x8_MODE_1;
    }
    else if (picture_control_set_ptr->enc_mode == ENC_MODE_7) {
        picture_control_set_ptr->cu8x8_mode = (picture_control_set_ptr->temporal_layer_index == 0) ? CU_8x8_MODE_0 : CU_8x8_MODE_1;
    }
    else {
        picture_control_set_ptr->cu8x8_mode = CU_8x8_MODE_1;
    }

    // CU_16x16 Search Mode
    picture_control_set_ptr->cu16x16_mode = CU_16x16_MODE_0;

    return return_error;
}

/*************************************************
* Reference Picture Signalling:
* Stateless derivation of RPS info to be stored in
* Picture Header
*
* This function uses the picture index from the just
* collected miniGop to derive the RPS(refIndexes+refresh)
* the miniGop is always 4L but could be complete (8 pictures)
or non-complete (less than 8 pictures).
* We get to this function if the picture is:
* 1) first Key frame
* 2) part of a complete RA MiniGop where the last frame could be a regular I for open GOP
* 3) part of complete LDP MiniGop where the last frame could be Key frame for closed GOP
* 4) part of non-complete LDP MiniGop where the last frame is a regularI.
This miniGOP has P frames with predStruct=LDP, and the last frame=I with pred struct=RA.
* 5) part of non-complete LDP MiniGop at the end of the stream.This miniGOP has P frames with
predStruct=LDP, and the last frame=I with pred struct=RA.
*
*************************************************/
void  generate_rps_info(
    PictureParentControlSet *picture_control_set_ptr,
    PictureDecisionContext  *context_ptr,
    uint32_t                 picture_index,
    uint32_t                 mini_gop_index
)
{
    RpsNode *rps = &picture_control_set_ptr->ref_signal;

    // Set restrictRefMvsDerivation
    picture_control_set_ptr->cpi->common.use_prev_frame_mvs = EB_TRUE;

    // Set Frame Type
    if (picture_control_set_ptr->slice_type == I_SLICE) {
        picture_control_set_ptr->cpi->common.frame_type = (picture_control_set_ptr->idr_flag) ? KEY_FRAME : INTER_FRAME;
        picture_control_set_ptr->cpi->common.intra_only = 1;
    }
    else {
        picture_control_set_ptr->cpi->common.frame_type = INTER_FRAME;
        picture_control_set_ptr->cpi->common.intra_only = 0;
    }

    // RPS for Flat GOP
    if (picture_control_set_ptr->hierarchical_levels == 0)
    {

        memset(rps->ref_dpb_index, 0, 3);
        rps->refresh_frame_mask = 1;
        picture_control_set_ptr->cpi->common.show_frame = EB_TRUE;
        picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;

    }
    else if (picture_control_set_ptr->hierarchical_levels == 3)//RPS for 4L GOP
    {

        //Reset miniGop Toggling. The first miniGop after a KEY frame has toggle=0
        if (picture_control_set_ptr->cpi->common.frame_type == KEY_FRAME)
        {
            context_ptr->mini_gop_toggle = 0;
            picture_control_set_ptr->cpi->common.show_frame = EB_TRUE;
            picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
            return;
        }

        //picture_index has this order:
        //        0      2    4      6
        //            1          5
        //                 3
        //                             8(could be an I)

        //DPB: Loc7|Loc6|Loc5|Loc4|Loc3|Loc2|Loc1|Loc0
        //Layer 0 : toggling bwteween DPB Location 0, and  locations 3-4-5-6-7
        //Layer 1 : DPB Location 1
        //Layer 2 : DPB Location 2

        //         1     3    5      7
        //            2          6
        //                 4
        //base0:0                      base1:8
        const uint8_t  base0_idx = context_ptr->mini_gop_toggle ? 0 : 3; //Base layer for prediction from past
        const uint8_t  base1_idx = context_ptr->mini_gop_toggle ? 3 : 0; //Base layer for prediction from future
        const uint8_t  layer1_idx = 1;
        const uint8_t  layer2_idx = 2;
        const uint8_t  layer3_idx1 = 4;
        const uint8_t  layer3_idx2 = 5;

        switch (picture_control_set_ptr->temporal_layer_index) {

        case 0:
            rps->ref_dpb_index[0] = base0_idx;
            rps->ref_dpb_index[2] = base0_idx;
            rps->refresh_frame_mask = context_ptr->mini_gop_toggle ? 200 : 1;
            break;
        case 1:
            rps->ref_dpb_index[0] = base0_idx;
            rps->ref_dpb_index[2] = base1_idx;
            rps->refresh_frame_mask = 2;
            break;
        case 2:

            if (picture_index == 1) {
                rps->ref_dpb_index[0] = base0_idx;
                rps->ref_dpb_index[2] = layer1_idx;
            }
            else if (picture_index == 5) {
                rps->ref_dpb_index[0] = layer1_idx;
                rps->ref_dpb_index[2] = base1_idx;
            }
            else {
                SVT_LOG("Error in GOP indexing\n");
            }
            rps->refresh_frame_mask = 4;
            break;
        case 3:
            if (picture_index == 0) {
                rps->ref_dpb_index[0] = base0_idx;
                rps->ref_dpb_index[2] = layer2_idx;
                rps->refresh_frame_mask = 16;
            }
            else if (picture_index == 2) {
                rps->ref_dpb_index[0] = layer2_idx;
                rps->ref_dpb_index[2] = layer1_idx;
                rps->refresh_frame_mask = 32;
            }
            else if (picture_index == 4) {
                rps->ref_dpb_index[0] = layer1_idx;
                rps->ref_dpb_index[2] = layer2_idx;
                rps->refresh_frame_mask = 16;
            }
            else if (picture_index == 6) {
                rps->ref_dpb_index[0] = layer2_idx;
                rps->ref_dpb_index[2] = base1_idx;
                rps->refresh_frame_mask = 32;
            }
            else {
                SVT_LOG("Error in GOp indexing\n");
            }
            break;
        default:
            SVT_LOG("Error: unexpected picture mini Gop number\n");
            break;
        }
        picture_control_set_ptr->cpi->common.use_prev_frame_mvs = EB_FALSE;

        if (picture_control_set_ptr->pred_struct_ptr->pred_type == EB_PRED_LOW_DELAY_P)
        {
            //P frames.
            rps->ref_dpb_index[1] = rps->ref_dpb_index[0];
            picture_control_set_ptr->cpi->common.show_frame = EB_TRUE;
            picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
        }
        else if (picture_control_set_ptr->pred_struct_ptr->pred_type == EB_PRED_RANDOM_ACCESS)
        {

            rps->ref_dpb_index[1] = rps->ref_dpb_index[0];

            //Decide on Show Mecanism
            if (picture_control_set_ptr->slice_type == I_SLICE)
            {
                //3 cases for I slice:  1:Key Frame treated above.  2: broken MiniGop due to sc or intra refresh  3: complete miniGop due to sc or intra refresh
                if (context_ptr->mini_gop_length[mini_gop_index] < picture_control_set_ptr->pred_struct_ptr->pred_struct_period)
                {
                    //Scene Change that breaks the mini gop and switch to LDP (if I scene change happens to be aligned with a complete miniGop, then we do not break the pred structure)
                    picture_control_set_ptr->cpi->common.show_frame = EB_TRUE;
                    picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
                }
                else
                {
                    picture_control_set_ptr->cpi->common.show_frame = EB_FALSE;
                    picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
                }
            }
            else//B pic
            {
                if (context_ptr->mini_gop_length[0] != picture_control_set_ptr->pred_struct_ptr->pred_struct_period)
                    SVT_LOG("Error in GOp indexing3\n");

                if (picture_control_set_ptr->is_used_as_reference_flag)
                {
                    picture_control_set_ptr->cpi->common.show_frame = EB_FALSE;
                    picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
                }
                else
                {

                    picture_control_set_ptr->cpi->common.show_frame = EB_FALSE;
                    if (picture_index == 0) {
                        picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
                    }
                    else if (picture_index == 2) {
                        picture_control_set_ptr->cpi->common.show_existing_frame = EB_TRUE;
                        picture_control_set_ptr->show_existing_frame_index_array[0] = layer3_idx1;
                        picture_control_set_ptr->show_existing_frame_index_array[1] = layer2_idx;
                        picture_control_set_ptr->show_existing_frame_index_array[2] = layer3_idx2;
                        picture_control_set_ptr->show_existing_frame_index_array[3] = layer1_idx;
                    }
                    else if (picture_index == 4) {
                        picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
                    }
                    else if (picture_index == 6) {
                        picture_control_set_ptr->cpi->common.show_existing_frame = EB_TRUE;
                        picture_control_set_ptr->show_existing_frame_index_array[0] = layer3_idx1;
                        picture_control_set_ptr->show_existing_frame_index_array[1] = layer2_idx;
                        picture_control_set_ptr->show_existing_frame_index_array[2] = layer3_idx2;
                        picture_control_set_ptr->show_existing_frame_index_array[3] = base1_idx;
                    }
                    else {
                        SVT_LOG("Error in GOp indexing2\n");
                    }

                }

            }

        }

        else {
            SVT_LOG("SVT [ERROR]: Not supported GOP structure!");
            return;
        }

        //last pic in MiniGop: mGop Toggling
        //mini GOP toggling since last Key Frame.
        //a regular I keeps the toggling process and does not reset the toggle.  K-0-1-0-1-0-K-0-1-0-1-K-0-1.....
        if (picture_index == context_ptr->mini_gop_end_index[mini_gop_index])
            context_ptr->mini_gop_toggle = 1 - context_ptr->mini_gop_toggle;

    }
#if NEW_PRED_STRUCT
    else if (picture_control_set_ptr->hierarchical_levels == 4)//RPS for 4L GOP
    {

        //Reset miniGop Toggling. The first miniGop after a KEY frame has toggle=0
        if (picture_control_set_ptr->cpi->common.frame_type == KEY_FRAME)
        {
            context_ptr->mini_gop_toggle = 0;
            picture_control_set_ptr->cpi->common.show_frame = EB_TRUE;
            picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
            return;
        }

        //         0     2    4      6    8     10     12      14
        //            1          5           9            13
        //                 3                        11
        //                              7

        //DPB: Loc7|Loc6|Loc5|Loc4|Loc3|Loc2|Loc1|Loc0
        //Layer 0 : toggling bwteween DPB Location 0, and  locations 3-4-5-6-7
        //Layer 1 : DPB Location 1
        //Layer 2 : DPB Location 2
        //Layer 3 : DPB Location 3

        //         1     3    5      7    9     11     13      15
        //            2          6           10            14
        //                 4                        12
        //                              8
        //base0:0                                               base1:16
        const uint8_t  base0_idx = context_ptr->mini_gop_toggle ? 0 : 3; //Base layer for prediction from past
        const uint8_t  base1_idx = context_ptr->mini_gop_toggle ? 3 : 0; //Base layer for prediction from future
        const uint8_t  layer1_idx = 1;
        const uint8_t  layer2_idx = 2;
        const uint8_t  layer3_idx1 = 4;
        const uint8_t  layer3_idx2 = 5;
        const uint8_t  layer4_idx1 = 6;
        const uint8_t  layer4_idx2 = 7;

        switch (picture_control_set_ptr->temporal_layer_index) {

        case 0:
            rps->ref_dpb_index[0] = base0_idx;
            rps->ref_dpb_index[2] = base0_idx;
            rps->refresh_frame_mask = context_ptr->mini_gop_toggle ? 8 : 1;
            break;
        case 1:
            rps->ref_dpb_index[0] = base0_idx;
            rps->ref_dpb_index[2] = base1_idx;
            rps->refresh_frame_mask = 2;
            break;
        case 2:

            if (picture_index == 3) {
                rps->ref_dpb_index[0] = base0_idx;
                rps->ref_dpb_index[2] = layer1_idx;
            }
            else if (picture_index == 11) {
                rps->ref_dpb_index[0] = layer1_idx;
                rps->ref_dpb_index[2] = base1_idx;
            }
            rps->refresh_frame_mask = 4;
            break;
        case 3:

            if (picture_index == 1) {
                rps->ref_dpb_index[0] = base0_idx;
                rps->ref_dpb_index[2] = layer2_idx;
                rps->refresh_frame_mask = 16;
            }
            else if (picture_index == 5) {
                rps->ref_dpb_index[0] = layer2_idx;
                rps->ref_dpb_index[2] = layer1_idx;
                rps->refresh_frame_mask = 32;
            }
            else if (picture_index == 9) {
                rps->ref_dpb_index[0] = layer1_idx;
                rps->ref_dpb_index[2] = layer2_idx;
                rps->refresh_frame_mask = 16;
            }
            else if (picture_index == 13) {
                rps->ref_dpb_index[0] = layer2_idx;
                rps->ref_dpb_index[2] = base1_idx;
                rps->refresh_frame_mask = 32;
            }
            else {
                SVT_LOG("Error in GOP indexing\n");
            }
            break;
        case 4:
            if (picture_index == 0) {
                rps->ref_dpb_index[0] = base0_idx;
                rps->ref_dpb_index[2] = layer3_idx1;
                rps->refresh_frame_mask = 64;
            }
            else if (picture_index == 2) {
                rps->ref_dpb_index[0] = layer3_idx1;
                rps->ref_dpb_index[2] = layer2_idx;
                rps->refresh_frame_mask = 128;

            }
            else if (picture_index == 4) {
                rps->ref_dpb_index[0] = layer2_idx;
                rps->ref_dpb_index[2] = layer3_idx2;
                rps->refresh_frame_mask = 64;

            }
            else if (picture_index == 6) {
                rps->ref_dpb_index[0] = layer3_idx2;
                rps->ref_dpb_index[2] = layer1_idx;
                rps->refresh_frame_mask = 128;

            }
            else if (picture_index == 8) {
                rps->ref_dpb_index[0] = layer1_idx;
                rps->ref_dpb_index[2] = layer3_idx1;
                rps->refresh_frame_mask = 64;

            }
            else if (picture_index == 10) {
                rps->ref_dpb_index[0] = layer3_idx1;
                rps->ref_dpb_index[2] = layer2_idx;
                rps->refresh_frame_mask = 128;

            }
            else if (picture_index == 12) {
                rps->ref_dpb_index[0] = layer2_idx;
                rps->ref_dpb_index[2] = layer3_idx2;
                rps->refresh_frame_mask = 64;

            }
            else if (picture_index == 14) {
                rps->ref_dpb_index[0] = layer3_idx2;
                rps->ref_dpb_index[2] = base1_idx;
                rps->refresh_frame_mask = 128;

            }
            else {
                SVT_LOG("Error in GOp indexing\n");
            }

            break;
        default:
            SVT_LOG("Error: unexpected picture mini Gop number\n");
            break;
        }
        picture_control_set_ptr->cpi->common.use_prev_frame_mvs = EB_FALSE;

        if (picture_control_set_ptr->pred_struct_ptr->pred_type == EB_PRED_LOW_DELAY_P)
        {
            //P frames.
            rps->ref_dpb_index[1] = rps->ref_dpb_index[0];
            picture_control_set_ptr->cpi->common.show_frame = EB_TRUE;
            picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
        }
        else if (picture_control_set_ptr->pred_struct_ptr->pred_type == EB_PRED_RANDOM_ACCESS)
        {

            rps->ref_dpb_index[1] = rps->ref_dpb_index[0];

            //Decide on Show Mecanism
            if (picture_control_set_ptr->slice_type == I_SLICE)
            {
                //3 cases for I slice:  1:Key Frame treated above.  2: broken MiniGop due to sc or intra refresh  3: complete miniGop due to sc or intra refresh
                if (context_ptr->mini_gop_length[0] < picture_control_set_ptr->pred_struct_ptr->pred_struct_period)
                {
                    //Scene Change that breaks the mini gop and switch to LDP (if I scene change happens to be aligned with a complete miniGop, then we do not break the pred structure)
                    picture_control_set_ptr->cpi->common.show_frame = EB_TRUE;
                    picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
                }
                else
                {
                    picture_control_set_ptr->cpi->common.show_frame = EB_FALSE;
                    picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
                }
            }
            else//B pic
            {
                if (context_ptr->mini_gop_length[0] != picture_control_set_ptr->pred_struct_ptr->pred_struct_period)
                    SVT_LOG("Error in GOp indexing3\n");

                if (picture_control_set_ptr->is_used_as_reference_flag)
                {
                    picture_control_set_ptr->cpi->common.show_frame = EB_FALSE;
                    picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
                }
                else
                {

                    picture_control_set_ptr->cpi->common.show_frame = EB_FALSE;
                    if (picture_index == 0) {
                        picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
                    }
                    else if (picture_index == 2) {
                        picture_control_set_ptr->cpi->common.show_existing_frame = EB_TRUE;
                        picture_control_set_ptr->show_existing_frame_index_array[0] = layer4_idx1;
                        picture_control_set_ptr->show_existing_frame_index_array[1] = layer3_idx1;
                        picture_control_set_ptr->show_existing_frame_index_array[2] = layer4_idx2;
                        picture_control_set_ptr->show_existing_frame_index_array[3] = layer2_idx;
                    }
                    else if (picture_index == 4) {
                        picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
                    }
                    else if (picture_index == 6) {
                        picture_control_set_ptr->cpi->common.show_existing_frame = EB_TRUE;
                        picture_control_set_ptr->show_existing_frame_index_array[0] = layer4_idx1;
                        picture_control_set_ptr->show_existing_frame_index_array[1] = layer3_idx2;
                        picture_control_set_ptr->show_existing_frame_index_array[2] = layer4_idx2;
                        picture_control_set_ptr->show_existing_frame_index_array[3] = layer1_idx;
                    }
                    else if (picture_index == 8) {
                        picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
                    }
                    else if (picture_index == 10) {
                        picture_control_set_ptr->cpi->common.show_existing_frame = EB_TRUE;
                        picture_control_set_ptr->show_existing_frame_index_array[0] = layer4_idx1;
                        picture_control_set_ptr->show_existing_frame_index_array[1] = layer3_idx1;
                        picture_control_set_ptr->show_existing_frame_index_array[2] = layer4_idx2;
                        picture_control_set_ptr->show_existing_frame_index_array[3] = layer2_idx;
                    }
                    else if (picture_index == 12) {
                        picture_control_set_ptr->cpi->common.show_existing_frame = EB_FALSE;
                    }
                    else if (picture_index == 14) {
                        picture_control_set_ptr->cpi->common.show_existing_frame = EB_TRUE;
                        picture_control_set_ptr->show_existing_frame_index_array[0] = layer4_idx1;
                        picture_control_set_ptr->show_existing_frame_index_array[1] = layer3_idx2;
                        picture_control_set_ptr->show_existing_frame_index_array[2] = layer4_idx2;
                        picture_control_set_ptr->show_existing_frame_index_array[3] = base1_idx;
                    }
                    else {
                        SVT_LOG("Error in GOp indexing2\n");
                    }

                }

            }

        }

        else {
            SVT_LOG("SVT [ERROR]: Not supported GOP structure!");
            return;
        }

        //last pic in MiniGop: mGop Toggling
        //mini GOP toggling since last Key Frame.
        //a regular I keeps the toggling process and does not reset the toggle.  K-0-1-0-1-0-K-0-1-0-1-K-0-1.....
        if (picture_index == context_ptr->mini_gop_end_index[mini_gop_index])
            context_ptr->mini_gop_toggle = 1 - context_ptr->mini_gop_toggle;
    }

#endif
    else
    {
        SVT_LOG("SVT [ERROR]: Not supported GOP structure!");
        return;
    }
}

/***************************************************************************************************
 * Picture Decision Kernel
 *
 * Notes on the Picture Decision:
 *
 * The Picture Decision process performs multi-picture level decisions, including setting of the prediction structure,
 * setting the picture type and scene change detection.
 *
 * Inputs:
 * Input Picture
 *   -Input Picture Data
 *
 *  Outputs:
 *   -Picture Control Set with fully available PA Reference List
 *
 *  For Low Delay Sequences, pictures are started into the encoder pipeline immediately.
 *
 *  For Random Access Sequences, pictures are held for up to a PredictionStructurePeriod
 *    in order to determine if a Scene Change or Intra Frame is forthcoming. Either of
 *    those events (and additionally a End of Sequence Flag) will change the expected
 *    prediction structure.
 *
 *  Below is an example worksheet for how Intra Flags and Scene Change Flags interact
 *    together to affect the prediction structure.
 *
 *  The base prediction structure for this example is a 3-Level Hierarchical Random Access,
 *    Single Reference Prediction Structure:
 *
 *        b   b
 *       / \ / \
 *      /   B   \
 *     /   / \   \
 *    I-----------B
 *
 *  From this base structure, the following RPS positions are derived:
 *
 *    p   p       b   b       p   p
 *     \   \     / \ / \     /   /
 *      P   \   /   B   \   /   P
 *       \   \ /   / \   \ /   /
 *        ----I-----------B----
 *
 *    L L L   I  [ Normal ]   T T T
 *    2 1 0   n               0 1 2
 *            t
 *            r
 *            a
 *
 *  The RPS is composed of Leading Picture [L2-L0], Intra (CRA), Base/Normal Pictures,
 *    and Trailing Pictures [T0-T2]. Generally speaking, Leading Pictures are useful
 *    for handling scene changes without adding extraneous I-pictures and the Trailing
 *    pictures are useful for terminating GOPs.
 *
 *  Here is a table of possible combinations of pictures needed to handle intra and
 *    scene changes happening in quick succession.
 *
 *        Distance to scene change ------------>
 *
 *                  0              1                 2                3+
 *   I
 *   n
 *   t   0        I   I           n/a               n/a              n/a
 *   r
 *   a              p              p
 *                   \            /
 *   P   1        I   I          I   I              n/a              n/a
 *   e
 *   r               p                               p
 *   i                \                             /
 *   o            p    \         p   p             /   p
 *   d             \    \       /     \           /   /
 *       2     I    -----I     I       I         I----    I          n/a
 *   |
 *   |            p   p           p   p            p   p            p   p
 *   |             \   \         /     \          /     \          /   /
 *   |              P   \       /   p   \        /   p   \        /   P
 *   |               \   \     /     \   \      /   /     \      /   /
 *   V   3+   I       ----I   I       ----I    I----       I    I----       I
 *
 *   The table is interpreted as follows:
 *
 *   If there are no SCs or Intras encountered for a PredPeriod, then the normal
 *     prediction structure is applied.
 *
 *   If there is an intra in the PredPeriod, then one of the above combinations of
 *     Leading and Trailing pictures is used.  If there is no scene change, the last
 *     valid column consisting of Trailing Pictures only is used.  However, if there
 *     is an upcoming scene change before the next intra, then one of the above patterns
 *     is used. In the case of End of Sequence flags, only the last valid column of Trailing
 *     Pictures is used. The intention here is that any combination of Intra Flag and Scene
 *     Change flag can be coded.
 *
 ***************************************************************************************************/
void* eb_vp9_picture_decision_kernel(void *input_ptr)
{
    PictureDecisionContext      *context_ptr = (PictureDecisionContext*) input_ptr;

    PictureParentControlSet     *picture_control_set_ptr;

    EncodeContext               *encode_context_ptr;
    SequenceControlSet          *sequence_control_set_ptr;

    EbObjectWrapper             *input_results_wrapper_ptr;
    PictureAnalysisResults      *input_results_ptr;

    EbObjectWrapper             *output_results_wrapper_ptr;
    PictureDecisionResults      *output_results_ptr;

    PredictionStructureEntry    *pred_position_ptr;

    EB_BOOL                      pre_assignment_buffer_first_pass_flag;
    EB_SLICE                     picture_type;

    PictureDecisionReorderEntry *queue_entry_ptr;
    int32_t                      queue_entry_index;

    int32_t                      previous_entry_index;

    PaReferenceQueueEntry       *input_entry_ptr;
    uint32_t                     input_queue_index;

    PaReferenceQueueEntry       *pa_reference_entry_ptr;
    uint32_t                     pa_reference_queue_index;

    uint64_t                     ref_poc;

    uint32_t                     dep_idx;
    uint64_t                     dep_poc;

    uint32_t                     dep_list_count;

    // Dynamic GOP
    uint32_t                     mini_gop_index;
    uint32_t                     picture_index;

    EB_BOOL                      window_avail,frame_passe_thru;
    uint32_t                     window_index;
    uint32_t                     entry_index;
    PictureParentControlSet     *parent_pcs_window[FUTURE_WINDOW_WIDTH+2];
    UNUSED(parent_pcs_window);
    // Debug
    uint64_t                     loop_count = 0;

    for(;;) {

        // Get Input Full Object
        eb_vp9_get_full_object(
            context_ptr->picture_analysis_results_input_fifo_ptr,
            &input_results_wrapper_ptr);

        input_results_ptr         = (PictureAnalysisResults*)   input_results_wrapper_ptr->object_ptr;
        picture_control_set_ptr    = (PictureParentControlSet  *)  input_results_ptr->picture_control_set_wrapper_ptr->object_ptr;
        sequence_control_set_ptr   = (SequenceControlSet *)       picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
        encode_context_ptr        = (EncodeContext*)            sequence_control_set_ptr->encode_context_ptr;

        loop_count ++;

        // Input Picture Analysis Results into the Picture Decision Reordering Queue
        // P.S. Since the prior Picture Analysis processes stage is multithreaded, inputs to the Picture Decision Process
        // can arrive out-of-display-order, so a the Picture Decision Reordering Queue is used to enforce processing of
        // pictures in display order

        queue_entry_index                                         =   (int32_t) (picture_control_set_ptr->picture_number - encode_context_ptr->picture_decision_reorder_queue[encode_context_ptr->picture_decision_reorder_queue_head_index]->picture_number);
        queue_entry_index                                         +=  encode_context_ptr->picture_decision_reorder_queue_head_index;
        queue_entry_index                                         =   (queue_entry_index > PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH - 1) ? queue_entry_index - PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH : queue_entry_index;
        queue_entry_ptr                                           =   encode_context_ptr->picture_decision_reorder_queue[queue_entry_index];
        if(queue_entry_ptr->parent_pcs_wrapper_ptr != NULL){
            CHECK_REPORT_ERROR_NC(
                encode_context_ptr->app_callback_ptr,
                EB_ENC_PD_ERROR7);
        }else{
            queue_entry_ptr->parent_pcs_wrapper_ptr                      =   input_results_ptr->picture_control_set_wrapper_ptr;
            queue_entry_ptr->picture_number                            =   picture_control_set_ptr->picture_number;
        }
        // Process the head of the Picture Decision Reordering Queue (Entry N)
        // P.S. The Picture Decision Reordering Queue should be parsed in the display order to be able to construct a pred structure
        queue_entry_ptr = encode_context_ptr->picture_decision_reorder_queue[encode_context_ptr->picture_decision_reorder_queue_head_index];

        while(queue_entry_ptr->parent_pcs_wrapper_ptr != EB_NULL) {

            if(  queue_entry_ptr->picture_number == 0  ||
                ((PictureParentControlSet   *)(queue_entry_ptr->parent_pcs_wrapper_ptr->object_ptr))->end_of_sequence_flag == EB_TRUE){
                    frame_passe_thru = EB_TRUE;
            }else{
                 frame_passe_thru = EB_FALSE;
            }
            window_avail                = EB_TRUE;
            previous_entry_index         = QUEUE_GET_PREVIOUS_SPOT(encode_context_ptr->picture_decision_reorder_queue_head_index);

            if(encode_context_ptr->picture_decision_reorder_queue[previous_entry_index]->parent_pcs_wrapper_ptr == NULL){
                window_avail = EB_FALSE;
            }else{
                parent_pcs_window[0] =(PictureParentControlSet   *) encode_context_ptr->picture_decision_reorder_queue[previous_entry_index]->parent_pcs_wrapper_ptr->object_ptr;
                parent_pcs_window[1] =(PictureParentControlSet   *) encode_context_ptr->picture_decision_reorder_queue[encode_context_ptr->picture_decision_reorder_queue_head_index]->parent_pcs_wrapper_ptr->object_ptr;
                for(window_index=0;    window_index<FUTURE_WINDOW_WIDTH;     window_index++){
                    entry_index = QUEUE_GET_NEXT_SPOT(encode_context_ptr->picture_decision_reorder_queue_head_index , window_index+1 ) ;

                    if(encode_context_ptr->picture_decision_reorder_queue[entry_index]->parent_pcs_wrapper_ptr == NULL ){
                        window_avail = EB_FALSE;
                        break;
                    }
                    else if (((PictureParentControlSet   *)(encode_context_ptr->picture_decision_reorder_queue[entry_index]->parent_pcs_wrapper_ptr->object_ptr))->end_of_sequence_flag == EB_TRUE) {
                        window_avail = EB_FALSE;
                        frame_passe_thru = EB_TRUE;
                        break;
                    }else{
                        parent_pcs_window[2+window_index] =(PictureParentControlSet   *) encode_context_ptr->picture_decision_reorder_queue[entry_index]->parent_pcs_wrapper_ptr->object_ptr;
                    }
                }
            }
            picture_control_set_ptr                        = (PictureParentControlSet  *)  queue_entry_ptr->parent_pcs_wrapper_ptr->object_ptr;

           if(picture_control_set_ptr->idr_flag == EB_TRUE)
                 context_ptr->last_solid_color_frame_poc = 0xFFFFFFFF;

            if(window_avail == EB_TRUE){
                picture_control_set_ptr->scene_change_flag           = EB_FALSE;
            }

            if(window_avail == EB_TRUE ||frame_passe_thru == EB_TRUE)
            {
            // Place the PCS into the Pre-Assignment Buffer
            // P.S. The Pre-Assignment Buffer is used to store a whole pre-structure
            encode_context_ptr->pre_assignment_buffer[encode_context_ptr->pre_assignment_buffer_count] = queue_entry_ptr->parent_pcs_wrapper_ptr;

            // Setup the PCS & SCS
            picture_control_set_ptr                        = (PictureParentControlSet  *)  encode_context_ptr->pre_assignment_buffer[encode_context_ptr->pre_assignment_buffer_count]->object_ptr;

            // Set the POC Number
            picture_control_set_ptr->picture_number    = (encode_context_ptr->current_input_poc + 1) /*& ((1 << sequence_control_set_ptr->bitsForPictureOrderCount)-1)*/;
            encode_context_ptr->current_input_poc      = picture_control_set_ptr->picture_number;

            picture_control_set_ptr->pred_structure = sequence_control_set_ptr->static_config.pred_structure;

            picture_control_set_ptr->hierarchical_layers_diff = 0;

           picture_control_set_ptr->init_pred_struct_position_flag    = EB_FALSE;

           picture_control_set_ptr->target_bit_rate          = sequence_control_set_ptr->static_config.target_bit_rate;

           release_prev_picture_from_reorder_queue(
               encode_context_ptr);

           // If the Intra period length is 0, then introduce an intra for every picture
           if (sequence_control_set_ptr->intra_period == 0 || picture_control_set_ptr->picture_number == 0 ) {
               picture_control_set_ptr->idr_flag = EB_TRUE;
           }
           // If an #IntraPeriodLength has passed since the last Intra, then introduce a CRA or IDR based on Intra Refresh type
           else if (sequence_control_set_ptr->intra_period != -1) {
               if ((encode_context_ptr->intra_period_position == (uint32_t)sequence_control_set_ptr->intra_period) ||
                   (picture_control_set_ptr->scene_change_flag == EB_TRUE)) {
                   picture_control_set_ptr->idr_flag = EB_TRUE;
               }
           }

            encode_context_ptr->pre_assignment_buffer_eos_flag             = (picture_control_set_ptr->end_of_sequence_flag) ? (uint32_t)EB_TRUE : encode_context_ptr->pre_assignment_buffer_eos_flag;

            // Increment the Pre-Assignment Buffer Intra Count
            encode_context_ptr->pre_assignment_buffer_intra_count         += picture_control_set_ptr->idr_flag;
            encode_context_ptr->pre_assignment_buffer_idr_count           += picture_control_set_ptr->idr_flag;
            encode_context_ptr->pre_assignment_buffer_count              += 1;

            if (sequence_control_set_ptr->static_config.rate_control_mode)
            {
                // Increment the Intra Period Position
                encode_context_ptr->intra_period_position = (encode_context_ptr->intra_period_position == (uint32_t)sequence_control_set_ptr->intra_period) ? 0 : encode_context_ptr->intra_period_position + 1;
            }
            else
            {
                // Increment the Intra Period Position
#if BEA_SCENE_CHANGE
                encode_context_ptr->intra_period_position = ((encode_context_ptr->intra_period_position == (uint32_t)sequence_control_set_ptr->intra_period) ) ? 0 : encode_context_ptr->intra_period_position + 1;

#else
                encode_context_ptr->intra_period_position = ((encode_context_ptr->intra_period_position == (uint32_t)sequence_control_set_ptr->intra_period) || (picture_control_set_ptr->scene_change_flag == EB_TRUE)) ? 0 : encode_context_ptr->intra_period_position + 1;
#endif
            }

            // Determine if Pictures can be released from the Pre-Assignment Buffer
            if ((encode_context_ptr->pre_assignment_buffer_intra_count > 0) ||
                (encode_context_ptr->pre_assignment_buffer_count == (uint32_t) (1 << sequence_control_set_ptr->hierarchical_levels)) ||
                (encode_context_ptr->pre_assignment_buffer_eos_flag == EB_TRUE) ||
                (picture_control_set_ptr->pred_structure == EB_PRED_LOW_DELAY_P) ||
                (picture_control_set_ptr->pred_structure == EB_PRED_LOW_DELAY_B))
            {

                // Initialize Picture Block Params
                context_ptr->mini_gop_start_index[0]         = 0;
                context_ptr->mini_gop_end_index     [0]         = encode_context_ptr->pre_assignment_buffer_count - 1;
                context_ptr->mini_gop_length     [0]         = encode_context_ptr->pre_assignment_buffer_count;

                context_ptr->mini_gop_hierarchical_levels[0] = sequence_control_set_ptr->hierarchical_levels;
                context_ptr->mini_gop_intra_count[0]         = encode_context_ptr->pre_assignment_buffer_intra_count;
                context_ptr->mini_gop_idr_count  [0]         = encode_context_ptr->pre_assignment_buffer_idr_count;
                context_ptr->total_number_of_mini_gops         = 1;

                encode_context_ptr->previous_mini_gop_hierarchical_levels = (picture_control_set_ptr->picture_number == 0) ?
                    sequence_control_set_ptr->hierarchical_levels :
                    encode_context_ptr->previous_mini_gop_hierarchical_levels;

#if NEW_PRED_STRUCT
                {
                    if (encode_context_ptr->pre_assignment_buffer_count > 1)
                    {
                        eb_vp9_initialize_mini_gop_activity_array(
                            context_ptr);

                        if (encode_context_ptr->pre_assignment_buffer_count == 16)
                            context_ptr->mini_gop_activity_array[L5_0_INDEX] = EB_FALSE;
                        else {
                            context_ptr->mini_gop_activity_array[L4_0_INDEX] = EB_FALSE;
                            context_ptr->mini_gop_activity_array[L4_1_INDEX] = EB_FALSE;
                        }
                        eb_vp9_generate_picture_window_split(
                            context_ptr,
                            encode_context_ptr);

                        eb_vp9_handle_incomplete_picture_window_map(
                            context_ptr,
                            encode_context_ptr);
                    }
                }
#endif
                generate_mini_gop_rps(
                    context_ptr,
                    encode_context_ptr);

                // Loop over Mini GOPs

                for (mini_gop_index = 0; mini_gop_index < context_ptr->total_number_of_mini_gops; ++mini_gop_index) {

                    pre_assignment_buffer_first_pass_flag = EB_TRUE;
#if NEW_PRED_STRUCT
                    {
                        update_base_layer_reference_queue_dependent_count(
                            context_ptr,
                            encode_context_ptr,
                            mini_gop_index);

                        // Keep track of the number of hierarchical levels of the latest implemented mini GOP
                        encode_context_ptr->previous_mini_gop_hierarchical_levels = context_ptr->mini_gop_hierarchical_levels[mini_gop_index];
                    }
#endif
                    // 1st Loop over Pictures in the Pre-Assignment Buffer
                    for (picture_index = context_ptr->mini_gop_start_index[mini_gop_index]; picture_index <= context_ptr->mini_gop_end_index[mini_gop_index]; ++picture_index) {

                        picture_control_set_ptr = (PictureParentControlSet  *)encode_context_ptr->pre_assignment_buffer[picture_index]->object_ptr;
                        sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;

                        // Keep track of the mini GOP size to which the input picture belongs - needed @ PictureManagerProcess()
                        picture_control_set_ptr->pre_assignment_buffer_count = context_ptr->mini_gop_length[mini_gop_index];

                        // Update the Pred Structure if cutting short a Random Access period
                        if ((context_ptr->mini_gop_length[mini_gop_index] < picture_control_set_ptr->pred_struct_ptr->pred_struct_period || context_ptr->mini_gop_idr_count[mini_gop_index] > 0) &&

                            picture_control_set_ptr->pred_struct_ptr->pred_type == EB_PRED_RANDOM_ACCESS &&
                            picture_control_set_ptr->idr_flag == EB_FALSE)
                        {
                            // Correct the Pred Index before switching structures
                            if (pre_assignment_buffer_first_pass_flag == EB_TRUE) {
                                encode_context_ptr->pred_struct_position -= picture_control_set_ptr->pred_struct_ptr->init_pic_index;
                            }

                            picture_control_set_ptr->pred_struct_ptr = eb_vp9_get_prediction_structure(
                                encode_context_ptr->prediction_structure_group_ptr,
                                EB_PRED_LOW_DELAY_P,
                                1,
                                picture_control_set_ptr->hierarchical_levels);

                            // Set the RPS Override Flag - this current only will convert a Random Access structure to a Low Delay structure
                            picture_control_set_ptr->use_rps_in_sps = EB_FALSE;

                            picture_type = P_SLICE;

                        }
                        else {

                            picture_control_set_ptr->use_rps_in_sps = EB_FALSE;

                            // Set the Picture Type
                            picture_type =
                                (picture_control_set_ptr->idr_flag) ? I_SLICE :
                                (picture_control_set_ptr->pred_structure == EB_PRED_LOW_DELAY_P) ? P_SLICE :
                                (picture_control_set_ptr->pred_structure == EB_PRED_LOW_DELAY_B) ? B_SLICE :
                                (picture_control_set_ptr->pre_assignment_buffer_count == picture_control_set_ptr->pred_struct_ptr->pred_struct_period) ? ((picture_index == context_ptr->mini_gop_end_index[mini_gop_index] && sequence_control_set_ptr->static_config.base_layer_switch_mode) ? P_SLICE : B_SLICE) :

                                (encode_context_ptr->pre_assignment_buffer_eos_flag) ? P_SLICE :
                                B_SLICE;
                        }
#if NEW_PRED_STRUCT
                        // If mini GOP switch, reset position
                        encode_context_ptr->pred_struct_position = (picture_control_set_ptr->init_pred_struct_position_flag) ?
                            picture_control_set_ptr->pred_struct_ptr->init_pic_index :
                            encode_context_ptr->pred_struct_position;
#endif
                        // If Intra, reset position
                        if (picture_control_set_ptr->idr_flag == EB_TRUE) {
                            encode_context_ptr->pred_struct_position = picture_control_set_ptr->pred_struct_ptr->init_pic_index;
                        }
                        else if (encode_context_ptr->elapsed_non_idr_count == 0) {
                            // If we are the picture directly after a IDR, we have to not use references that violate the IDR
                            encode_context_ptr->pred_struct_position = picture_control_set_ptr->pred_struct_ptr->init_pic_index + 1;
                        }
                        // Elif Scene Change, determine leading and trailing pictures
                        //else if (encode_context_ptr->pre_assignment_buffer_scene_change_count > 0) {
                        //    if(buffer_index < encode_context_ptr->pre_assignment_buffer_scene_change_index) {
                        //        ++encode_context_ptr->pred_struct_position;
                        //        picture_type = P_SLICE;
                        //    }
                        //    else {
                        //        encode_context_ptr->pred_struct_position = picture_control_set_ptr->pred_struct_ptr->init_pic_index + encode_context_ptr->pre_assignment_buffer_count - buffer_index - 1;
                        //    }
                        //}
                        // Else, Increment the position normally
                        else {
                            ++encode_context_ptr->pred_struct_position;
                        }

                        // The poc number of the latest IDR picture is stored so that last_idr_picture (present in PCS) for the incoming pictures can be updated.
                        // The last_idr_picture is used in reseting the poc (in entropy coding) whenever IDR is encountered.
                        // Note IMP: This logic only works when display and decode order are the same. Currently for Random Access, IDR is inserted (similar to CRA) by using trailing P pictures (low delay fashion) and breaking prediction structure.
                        // Note: When leading P pictures are implemented, this logic has to change..
                        if (picture_control_set_ptr->idr_flag == EB_TRUE) {
                            encode_context_ptr->last_idr_picture = picture_control_set_ptr->picture_number;
                        }
                        else {
                            picture_control_set_ptr->last_idr_picture = encode_context_ptr->last_idr_picture;
                        }

                        // Cycle the pred_struct_position if its overflowed
                        encode_context_ptr->pred_struct_position = (encode_context_ptr->pred_struct_position == picture_control_set_ptr->pred_struct_ptr->pred_struct_entry_count) ?
                            encode_context_ptr->pred_struct_position - picture_control_set_ptr->pred_struct_ptr->pred_struct_period :
                            encode_context_ptr->pred_struct_position;

                        pred_position_ptr = picture_control_set_ptr->pred_struct_ptr->pred_struct_entry_ptr_array[encode_context_ptr->pred_struct_position];

                        // Set the Slice type
                        picture_control_set_ptr->slice_type = picture_type;
                        ((EbPaReferenceObject  *)picture_control_set_ptr->pareference_picture_wrapper_ptr->object_ptr)->slice_type = picture_control_set_ptr->slice_type;

                        switch (picture_type) {

                        case I_SLICE:

                            // Reset Prediction Structure Position & Reference Struct Position
                            if (picture_control_set_ptr->picture_number == 0){
                                encode_context_ptr->intra_period_position = 0;
                            }

                            //-------------------------------
                            // IDR
                            //-------------------------------
                            if (picture_control_set_ptr->idr_flag == EB_TRUE) {

                                // Reset the pictures since last IDR counter
                                encode_context_ptr->elapsed_non_idr_count = 0;

                            }
                            break;

                        case P_SLICE:
                        case B_SLICE:

                            // Reset IDR Flag
                            picture_control_set_ptr->idr_flag = EB_FALSE;

                            // Increment & Clip the elapsed Non-IDR Counter. This is clipped rather than allowed to free-run
                            // inorder to avoid rollover issues.  This assumes that any the GOP period is less than MAX_ELAPSED_IDR_COUNT
                            encode_context_ptr->elapsed_non_idr_count = MIN(encode_context_ptr->elapsed_non_idr_count + 1, MAX_ELAPSED_IDR_COUNT);

                            CHECK_REPORT_ERROR(
                                (picture_control_set_ptr->pred_struct_ptr->pred_struct_entry_count < MAX_ELAPSED_IDR_COUNT),
                                encode_context_ptr->app_callback_ptr,
                                EB_ENC_PD_ERROR1);

                            break;

                        default:

                            CHECK_REPORT_ERROR_NC(
                                encode_context_ptr->app_callback_ptr,
                                EB_ENC_PD_ERROR2);

                            break;
                        }
                        picture_control_set_ptr->pred_struct_index = (uint8_t)encode_context_ptr->pred_struct_position;
                        picture_control_set_ptr->temporal_layer_index = (uint8_t)pred_position_ptr->temporal_layer_index;
                        picture_control_set_ptr->is_used_as_reference_flag = pred_position_ptr->is_referenced;

                        // Set the Decode Order
                        if ((context_ptr->mini_gop_idr_count[mini_gop_index] == 0) &&
                            (context_ptr->mini_gop_length[mini_gop_index] == picture_control_set_ptr->pred_struct_ptr->pred_struct_period))

                        {
                            picture_control_set_ptr->decode_order = encode_context_ptr->decode_base_number + pred_position_ptr->decode_order;
                        }
                        else {
                            picture_control_set_ptr->decode_order = picture_control_set_ptr->picture_number;
                        }

                        encode_context_ptr->terminating_sequence_flag_received = (picture_control_set_ptr->end_of_sequence_flag == EB_TRUE) ?
                            EB_TRUE :
                            encode_context_ptr->terminating_sequence_flag_received;

                        encode_context_ptr->terminating_picture_number = (picture_control_set_ptr->end_of_sequence_flag == EB_TRUE) ?
                            picture_control_set_ptr->picture_number :
                            encode_context_ptr->terminating_picture_number;

                        pre_assignment_buffer_first_pass_flag = EB_FALSE;

                        generate_rps_info(
                            picture_control_set_ptr,
                            context_ptr,
#if NEW_PRED_STRUCT
                            picture_index - context_ptr->mini_gop_start_index[mini_gop_index],
                            mini_gop_index);
#else
                            picture_index,
                            mini_gop_index);
#endif
                        picture_control_set_ptr->cpi->allow_comp_inter_inter = 0;
                        picture_control_set_ptr->cpi->common.reference_mode = (REFERENCE_MODE)0xFF;
                        if (picture_control_set_ptr->slice_type != I_SLICE) {
                            picture_control_set_ptr->cpi->allow_comp_inter_inter = 1;
                            if (picture_control_set_ptr->temporal_layer_index == 0 || picture_control_set_ptr->slice_type == P_SLICE) {
                                picture_control_set_ptr->cpi->common.reference_mode = SINGLE_REFERENCE;
                            }
                            else {
                                picture_control_set_ptr->cpi->common.reference_mode = REFERENCE_MODE_SELECT;
                            }
                        }
                        picture_control_set_ptr->cpi->common.refresh_frame_context = 1; /* Two state 0 = NO, 1 = YES */
                        memset(picture_control_set_ptr->cpi->common.ref_frame_sign_bias, 0, MAX_REF_FRAMES * sizeof(int));

                        if (picture_control_set_ptr->cpi->common.reference_mode == REFERENCE_MODE_SELECT)
                        {
                            picture_control_set_ptr->cpi->common.ref_frame_sign_bias[ALTREF_FRAME] = 1;
                        }

                        if (picture_control_set_ptr->cpi->common.reference_mode != SINGLE_REFERENCE)
                            eb_vp9_setup_compound_reference_mode(&picture_control_set_ptr->cpi->common);

#if USE_SRC_REF
                        picture_control_set_ptr->use_src_ref = (picture_control_set_ptr->temporal_layer_index > 0) ?
                            EB_TRUE :
                            EB_FALSE;
#else
                        picture_control_set_ptr->use_src_ref = (sequence_control_set_ptr->input_resolution == INPUT_SIZE_4K_RANGE && picture_control_set_ptr->temporal_layer_index > 0) ?
                            EB_TRUE :
                            EB_FALSE;
#endif

                        // Set QP Scaling Mode
                        picture_control_set_ptr->qp_scaling_mode = (picture_control_set_ptr->slice_type == I_SLICE) ?
                            QP_SCALING_MODE_1 :
                            QP_SCALING_MODE_0 ;

                        // ME Kernel Multi-Processes Signal(s) derivation
                        if (sequence_control_set_ptr->static_config.tune == TUNE_SQ) {
                            eb_vp9_signal_derivation_multi_processes_sq(
                                sequence_control_set_ptr,
                                picture_control_set_ptr);
                        }
                        else if (sequence_control_set_ptr->static_config.tune == TUNE_VMAF) {
                            eb_vp9_signal_derivation_multi_processes_vmaf(
                                sequence_control_set_ptr,
                                picture_control_set_ptr);
                        }
                        else {
                            eb_vp9_signal_derivation_multi_processes_oq(
                                sequence_control_set_ptr,
                                picture_control_set_ptr);
                        }

                        // Update the Dependant List Count - If there was an I-frame or Scene Change, then cleanup the Picture Decision PA Reference Queue Dependent Counts
                        if (picture_control_set_ptr->slice_type == I_SLICE)
                        {

                            input_queue_index = encode_context_ptr->picture_decision_pa_reference_queue_head_index;

                            while(input_queue_index != encode_context_ptr->picture_decision_pa_reference_queue_tail_index) {

                                input_entry_ptr = encode_context_ptr->picture_decision_pa_reference_queue[input_queue_index];

                                // Modify Dependent List0
                                dep_list_count = input_entry_ptr->list0.list_count;
                                for(dep_idx=0; dep_idx < dep_list_count; ++dep_idx) {

                                    // Adjust the latest current_input_poc in case we're in a POC rollover scenario
                                    // current_input_poc += (current_input_poc < input_entry_ptr->pocNumber) ? (1 << sequence_control_set_ptr->bitsForPictureOrderCount) : 0;

                                    dep_poc = POC_CIRCULAR_ADD(
                                        input_entry_ptr->picture_number, // can't use a value that gets reset
                                        input_entry_ptr->list0.list[dep_idx]/*,
                                        sequence_control_set_ptr->bitsForPictureOrderCount*/);

                                    // If Dependent POC is greater or equal to the IDR POC
                                    if(dep_poc >= picture_control_set_ptr->picture_number && input_entry_ptr->list0.list[dep_idx]) {

                                        input_entry_ptr->list0.list[dep_idx] = 0;

                                        // Decrement the Reference's referenceCount
                                        --input_entry_ptr->dependent_count;

                                        CHECK_REPORT_ERROR(
                                            (input_entry_ptr->dependent_count != ~0u),
                                            encode_context_ptr->app_callback_ptr,
                                            EB_ENC_PD_ERROR3);
                                    }
                                }

                                // Modify Dependent List1
                                dep_list_count = input_entry_ptr->list1.list_count;
                                for(dep_idx=0; dep_idx < dep_list_count; ++dep_idx) {

                                    // Adjust the latest current_input_poc in case we're in a POC rollover scenario
                                    // current_input_poc += (current_input_poc < input_entry_ptr->pocNumber) ? (1 << sequence_control_set_ptr->bitsForPictureOrderCount) : 0;

                                    dep_poc = POC_CIRCULAR_ADD(
                                        input_entry_ptr->picture_number,
                                        input_entry_ptr->list1.list[dep_idx]/*,
                                        sequence_control_set_ptr->bitsForPictureOrderCount*/);

                                    // If Dependent POC is greater or equal to the IDR POC
                                    if(((dep_poc >= picture_control_set_ptr->picture_number) || (((picture_control_set_ptr->pre_assignment_buffer_count != picture_control_set_ptr->pred_struct_ptr->pred_struct_period) || (picture_control_set_ptr->idr_flag == EB_TRUE)) && (dep_poc >  (picture_control_set_ptr->picture_number - picture_control_set_ptr->pre_assignment_buffer_count)))) && input_entry_ptr->list1.list[dep_idx]) {
                                        input_entry_ptr->list1.list[dep_idx] = 0;

                                        // Decrement the Reference's referenceCount
                                        --input_entry_ptr->dependent_count;

                                        CHECK_REPORT_ERROR(
                                            (input_entry_ptr->dependent_count != ~0u),
                                            encode_context_ptr->app_callback_ptr,
                                            EB_ENC_PD_ERROR3);
                                    }

                                }

                                // Increment the input_queue_index Iterator
                                input_queue_index = (input_queue_index == PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : input_queue_index + 1;
                            }

                        } else if(picture_control_set_ptr->idr_flag == EB_TRUE) {

                            // Set the Picture Decision PA Reference Entry pointer
                            input_entry_ptr                           = (PaReferenceQueueEntry*) EB_NULL;
                        }

                        // Place Picture in Picture Decision PA Reference Queue
                        input_entry_ptr                                       = encode_context_ptr->picture_decision_pa_reference_queue[encode_context_ptr->picture_decision_pa_reference_queue_tail_index];
                        input_entry_ptr->input_object_ptr                       = picture_control_set_ptr->pareference_picture_wrapper_ptr;
                        input_entry_ptr->picture_number                        = picture_control_set_ptr->picture_number;
                        input_entry_ptr->reference_entry_index                  = encode_context_ptr->picture_decision_pa_reference_queue_tail_index;
                        input_entry_ptr->p_pcs_ptr = picture_control_set_ptr;
                        encode_context_ptr->picture_decision_pa_reference_queue_tail_index     =
                            (encode_context_ptr->picture_decision_pa_reference_queue_tail_index == PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : encode_context_ptr->picture_decision_pa_reference_queue_tail_index + 1;

                        // Check if the Picture Decision PA Reference is full
                        CHECK_REPORT_ERROR(
                            (((encode_context_ptr->picture_decision_pa_reference_queue_head_index != encode_context_ptr->picture_decision_pa_reference_queue_tail_index) ||
                            (encode_context_ptr->picture_decision_pa_reference_queue[encode_context_ptr->picture_decision_pa_reference_queue_head_index]->input_object_ptr == EB_NULL))),
                            encode_context_ptr->app_callback_ptr,
                            EB_ENC_PD_ERROR4);

                        // Copy the reference lists into the inputEntry and
                        // set the Reference Counts Based on Temporal Layer and how many frames are active
                        picture_control_set_ptr->ref_list0_count = (picture_type == I_SLICE) ? 0 : (uint8_t)pred_position_ptr->ref_list0.reference_list_count;
                        picture_control_set_ptr->ref_list1_count = (picture_type == I_SLICE) ? 0 : (uint8_t)pred_position_ptr->ref_list1.reference_list_count;

                        input_entry_ptr->list0_ptr             = &pred_position_ptr->ref_list0;
                        input_entry_ptr->list1_ptr             = &pred_position_ptr->ref_list1;

                        {

                            // Copy the Dependent Lists
                            // *Note - we are removing any leading picture dependencies for now
                            input_entry_ptr->list0.list_count = 0;
                            for(dep_idx = 0; dep_idx < pred_position_ptr->dep_list0.list_count; ++dep_idx) {
                                if(pred_position_ptr->dep_list0.list[dep_idx] >= 0) {
                                    input_entry_ptr->list0.list[input_entry_ptr->list0.list_count++] = pred_position_ptr->dep_list0.list[dep_idx];
                                }
                            }

                            input_entry_ptr->list1.list_count = pred_position_ptr->dep_list1.list_count;
                            for(dep_idx = 0; dep_idx < pred_position_ptr->dep_list1.list_count; ++dep_idx) {
                                input_entry_ptr->list1.list[dep_idx] = pred_position_ptr->dep_list1.list[dep_idx];
                            }

                            input_entry_ptr->dep_list0_count                = input_entry_ptr->list0.list_count;
                            input_entry_ptr->dep_list1_count                = input_entry_ptr->list1.list_count;
                            input_entry_ptr->dependent_count               = input_entry_ptr->dep_list0_count + input_entry_ptr->dep_list1_count;

                        }

                        ((EbPaReferenceObject  *)picture_control_set_ptr->pareference_picture_wrapper_ptr->object_ptr)->dependent_pictures_count = input_entry_ptr->dependent_count;

                        /* uint32_t depCnt = ((EbPaReferenceObject  *)picture_control_set_ptr->pareference_picture_wrapper_ptr->object_ptr)->dependent_pictures_count;
                        if (picture_control_set_ptr->picture_number>0 && picture_control_set_ptr->slice_type==I_SLICE && depCnt!=8 )
                        SVT_LOG("depCnt Error1  POC:%i  TL:%i   is needed:%i\n",picture_control_set_ptr->picture_number,picture_control_set_ptr->temporal_layer_index,input_entry_ptr->dependent_count);
                        else if (picture_control_set_ptr->slice_type==B_SLICE && picture_control_set_ptr->temporal_layer_index == 0 && depCnt!=8)
                        SVT_LOG("depCnt Error2  POC:%i  TL:%i   is needed:%i\n",picture_control_set_ptr->picture_number,picture_control_set_ptr->temporal_layer_index,input_entry_ptr->dependent_count);
                        else if (picture_control_set_ptr->slice_type==B_SLICE && picture_control_set_ptr->temporal_layer_index == 1 && depCnt!=4)
                        SVT_LOG("depCnt Error3  POC:%i  TL:%i   is needed:%i\n",picture_control_set_ptr->picture_number,picture_control_set_ptr->temporal_layer_index,input_entry_ptr->dependent_count);
                        else if (picture_control_set_ptr->slice_type==B_SLICE && picture_control_set_ptr->temporal_layer_index == 2 && depCnt!=2)
                        SVT_LOG("depCnt Error4  POC:%i  TL:%i   is needed:%i\n",picture_control_set_ptr->picture_number,picture_control_set_ptr->temporal_layer_index,input_entry_ptr->dependent_count);
                        else if (picture_control_set_ptr->slice_type==B_SLICE && picture_control_set_ptr->temporal_layer_index == 3 && depCnt!=0)
                        SVT_LOG("depCnt Error5  POC:%i  TL:%i   is needed:%i\n",picture_control_set_ptr->picture_number,picture_control_set_ptr->temporal_layer_index,input_entry_ptr->dependent_count);*/
                        //if (picture_control_set_ptr->slice_type==P_SLICE )
                        //     SVT_LOG("POC:%i  TL:%i   is needed:%i\n",picture_control_set_ptr->picture_number,picture_control_set_ptr->temporal_layer_index,input_entry_ptr->dependent_count);

                        CHECK_REPORT_ERROR(
                            (picture_control_set_ptr->pred_struct_ptr->pred_struct_period < MAX_ELAPSED_IDR_COUNT),
                            encode_context_ptr->app_callback_ptr,
                            EB_ENC_PD_ERROR5);

                        // Reset the PA Reference Lists
                        EB_MEMSET(picture_control_set_ptr->ref_pa_pic_ptr_array, 0, 2 * sizeof(EbObjectWrapper*));

                        EB_MEMSET(picture_control_set_ptr->ref_pa_pic_ptr_array, 0, 2 * sizeof(uint32_t));

                    }

                    // 2nd Loop over Pictures in the Pre-Assignment Buffer
                    for (picture_index = context_ptr->mini_gop_start_index[mini_gop_index]; picture_index <= context_ptr->mini_gop_end_index[mini_gop_index]; ++picture_index) {

                        picture_control_set_ptr = (PictureParentControlSet  *)    encode_context_ptr->pre_assignment_buffer[picture_index]->object_ptr;

                        // Find the Reference in the Picture Decision PA Reference Queue
                        input_queue_index = encode_context_ptr->picture_decision_pa_reference_queue_head_index;

                        do {

                            // Setup the Picture Decision PA Reference Queue Entry
                            input_entry_ptr   = encode_context_ptr->picture_decision_pa_reference_queue[input_queue_index];

                            // Increment the reference_queue_index Iterator
                            input_queue_index = (input_queue_index == PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : input_queue_index + 1;

                        } while ((input_queue_index != encode_context_ptr->picture_decision_pa_reference_queue_tail_index) && (input_entry_ptr->picture_number != picture_control_set_ptr->picture_number));

                        CHECK_REPORT_ERROR(
                            (input_entry_ptr->picture_number == picture_control_set_ptr->picture_number),
                            encode_context_ptr->app_callback_ptr,
                            EB_ENC_PD_ERROR6);

                        // Reset the PA Reference Lists
                        EB_MEMSET(picture_control_set_ptr->ref_pa_pic_ptr_array, 0, 2 * sizeof(EbObjectWrapper*));

                        EB_MEMSET(picture_control_set_ptr->ref_pic_poc_array, 0, 2 * sizeof(uint64_t));

                        // Configure List0
                        if ((picture_control_set_ptr->slice_type == P_SLICE) || (picture_control_set_ptr->slice_type == B_SLICE)) {

                            if (picture_control_set_ptr->ref_list0_count){
                                pa_reference_queue_index = (uint32_t) CIRCULAR_ADD(
                                    ((int32_t) input_entry_ptr->reference_entry_index) - input_entry_ptr->list0_ptr->reference_list,
                                    PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH);                                                                                             // Max

                                pa_reference_entry_ptr = encode_context_ptr->picture_decision_pa_reference_queue[pa_reference_queue_index];

                                // Calculate the Ref POC
                                ref_poc = POC_CIRCULAR_ADD(
                                    picture_control_set_ptr->picture_number,
                                    -input_entry_ptr->list0_ptr->reference_list/*,
                                    sequence_control_set_ptr->bitsForPictureOrderCount*/);

                                // Set the Reference Object
                                picture_control_set_ptr->ref_pa_pic_ptr_array[REF_LIST_0] = pa_reference_entry_ptr->input_object_ptr;
                                picture_control_set_ptr->ref_pic_poc_array[REF_LIST_0] = ref_poc;
                                picture_control_set_ptr->ref_pa_pcs_array[REF_LIST_0] = pa_reference_entry_ptr->p_pcs_ptr;

                                // Increment the PA Reference's live_count by the number of tiles in the input picture
                                eb_vp9_object_inc_live_count(
                                    pa_reference_entry_ptr->input_object_ptr,
                                    1);

                                ((EbPaReferenceObject  *)picture_control_set_ptr->ref_pa_pic_ptr_array[REF_LIST_0]->object_ptr)->p_pcs_ptr = pa_reference_entry_ptr->p_pcs_ptr;

                                eb_vp9_object_inc_live_count(
                                    pa_reference_entry_ptr->p_pcs_ptr->p_pcs_wrapper_ptr,
                                    1);

                                --pa_reference_entry_ptr->dependent_count;
                            }
                        }

                        // Configure List1
                        if (picture_control_set_ptr->slice_type == B_SLICE) {

                            if (picture_control_set_ptr->ref_list1_count){
                                pa_reference_queue_index = (uint32_t) CIRCULAR_ADD(
                                    ((int32_t) input_entry_ptr->reference_entry_index) - input_entry_ptr->list1_ptr->reference_list,
                                    PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH);                                                                                             // Max

                                pa_reference_entry_ptr = encode_context_ptr->picture_decision_pa_reference_queue[pa_reference_queue_index];

                                // Calculate the Ref POC
                                ref_poc = POC_CIRCULAR_ADD(
                                    picture_control_set_ptr->picture_number,
                                    -input_entry_ptr->list1_ptr->reference_list/*,
                                    sequence_control_set_ptr->bitsForPictureOrderCount*/);
                                picture_control_set_ptr->ref_pa_pcs_array[REF_LIST_1] = pa_reference_entry_ptr->p_pcs_ptr;
                                // Set the Reference Object
                                picture_control_set_ptr->ref_pa_pic_ptr_array[REF_LIST_1] = pa_reference_entry_ptr->input_object_ptr;
                                picture_control_set_ptr->ref_pic_poc_array[REF_LIST_1] = ref_poc;

                                // Increment the PA Reference's live_count by the number of tiles in the input picture
                                eb_vp9_object_inc_live_count(
                                    pa_reference_entry_ptr->input_object_ptr,
                                    1);

                                ((EbPaReferenceObject  *)picture_control_set_ptr->ref_pa_pic_ptr_array[REF_LIST_1]->object_ptr)->p_pcs_ptr = pa_reference_entry_ptr->p_pcs_ptr;

                                eb_vp9_object_inc_live_count(
                                    pa_reference_entry_ptr->p_pcs_ptr->p_pcs_wrapper_ptr,
                                    1);

                                --pa_reference_entry_ptr->dependent_count;
                            }
                        }

                        // Initialize Segments
                        picture_control_set_ptr->me_segments_column_count    =  (uint8_t)(sequence_control_set_ptr->me_segment_column_count_array[picture_control_set_ptr->temporal_layer_index]);
                        picture_control_set_ptr->me_segments_row_count       =  (uint8_t)(sequence_control_set_ptr->me_segment_row_count_array[picture_control_set_ptr->temporal_layer_index]);
                        picture_control_set_ptr->me_segments_total_count     =  (uint16_t)(picture_control_set_ptr->me_segments_column_count  * picture_control_set_ptr->me_segments_row_count);
                        picture_control_set_ptr->me_segments_completion_mask = 0;

                        // Post the results to the ME processes
                        {
                            uint32_t segment_index;

                            for(segment_index=0; segment_index < picture_control_set_ptr->me_segments_total_count; ++segment_index)
                            {
                                // Get Empty Results Object
                                eb_vp9_get_empty_object(
                                    context_ptr->picture_decision_results_output_fifo_ptr,
                                    &output_results_wrapper_ptr);

                                output_results_ptr = (PictureDecisionResults*) output_results_wrapper_ptr->object_ptr;

                                output_results_ptr->picture_control_set_wrapper_ptr = encode_context_ptr->pre_assignment_buffer[picture_index];

                                output_results_ptr->segment_index = segment_index;

                                // Post the Full Results Object
                                eb_vp9_post_full_object(output_results_wrapper_ptr);
                            }
                        }

                        if (picture_index == context_ptr->mini_gop_end_index[mini_gop_index]) {

                            // Increment the Decode Base Number
                            encode_context_ptr->decode_base_number  += context_ptr->mini_gop_length[mini_gop_index];
                        }

                        if (picture_index == encode_context_ptr->pre_assignment_buffer_count - 1) {

                            // Reset the Pre-Assignment Buffer
                            encode_context_ptr->pre_assignment_buffer_count              = 0;
                            encode_context_ptr->pre_assignment_buffer_idr_count           = 0;
                            encode_context_ptr->pre_assignment_buffer_intra_count         = 0;
                            encode_context_ptr->pre_assignment_buffer_scene_change_count   = 0;
                            encode_context_ptr->pre_assignment_buffer_eos_flag            = EB_FALSE;
                            encode_context_ptr->number_of_active_pictures                = 0;
                        }
                    }

                } // End MINI GOPs loop
            }

            // Walk the picture_decision_pa_reference_queue and remove entries that have been completely referenced.
            input_queue_index = encode_context_ptr->picture_decision_pa_reference_queue_head_index;

            while(input_queue_index != encode_context_ptr->picture_decision_pa_reference_queue_tail_index) {

                input_entry_ptr = encode_context_ptr->picture_decision_pa_reference_queue[input_queue_index];

                // Remove the entry
                if((input_entry_ptr->dependent_count == 0) &&
                   (input_entry_ptr->input_object_ptr)) {
                    eb_vp9_release_object(input_entry_ptr->p_pcs_ptr->p_pcs_wrapper_ptr);
                       // Release the nominal live_count value
                       eb_vp9_release_object(input_entry_ptr->input_object_ptr);
                       input_entry_ptr->input_object_ptr = (EbObjectWrapper*) EB_NULL;
                }

                // Increment the head_index if the head is null
                encode_context_ptr->picture_decision_pa_reference_queue_head_index =
                    (encode_context_ptr->picture_decision_pa_reference_queue[encode_context_ptr->picture_decision_pa_reference_queue_head_index]->input_object_ptr)   ? encode_context_ptr->picture_decision_pa_reference_queue_head_index :
                    (encode_context_ptr->picture_decision_pa_reference_queue_head_index == PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH - 1)                 ? 0
                                                                                                                                                        : encode_context_ptr->picture_decision_pa_reference_queue_head_index + 1;

                 CHECK_REPORT_ERROR(
                    (((encode_context_ptr->picture_decision_pa_reference_queue_head_index != encode_context_ptr->picture_decision_pa_reference_queue_tail_index) ||
                    (encode_context_ptr->picture_decision_pa_reference_queue[encode_context_ptr->picture_decision_pa_reference_queue_head_index]->input_object_ptr == EB_NULL))),
                    encode_context_ptr->app_callback_ptr,
                    EB_ENC_PD_ERROR4);

                // Increment the input_queue_index Iterator
                input_queue_index = (input_queue_index == PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : input_queue_index + 1;
            }

            // Increment the Picture Decision Reordering Queue Head Ptr
            encode_context_ptr->picture_decision_reorder_queue_head_index  =   (encode_context_ptr->picture_decision_reorder_queue_head_index == PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encode_context_ptr->picture_decision_reorder_queue_head_index + 1;

            // Get the next entry from the Picture Decision Reordering Queue (Entry N+1)
            queue_entry_ptr                                           = encode_context_ptr->picture_decision_reorder_queue[encode_context_ptr->picture_decision_reorder_queue_head_index];
        }
            if(window_avail == EB_FALSE  && frame_passe_thru == EB_FALSE)
                break;
        }

        // Release the Input Results
        eb_vp9_release_object(input_results_wrapper_ptr);
    }

    return EB_NULL;
}
void unused_variable_void_func_pic_decision()
{
    (void)n_x_m_sad_kernel_func_ptr_array;
    (void)nx_m_sad_loop_kernel_func_ptr_array;
    (void)nx_m_sad_averaging_kernel_func_ptr_array;
    (void)eb_vp9_sad_calculation_8x8_16x16_func_ptr_array;
    (void)eb_vp9_sad_calculation_32x32_64x64_func_ptr_array;
    (void)eb_vp9_initialize_buffer_32bits_func_ptr_array;
}
