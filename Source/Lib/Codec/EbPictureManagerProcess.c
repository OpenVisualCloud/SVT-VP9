/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"

#include "EbPictureManagerProcess.h"
#include "EbReferenceObject.h"

#include "EbPictureDemuxResults.h"
#include "EbPictureManagerQueue.h"
#include "EbPredictionStructure.h"
#include "EbRateControlTasks.h"
#include "EbSvtVp9ErrorCodes.h"

/************************************************
 * Defines
 ************************************************/
#define POC_CIRCULAR_ADD(base, offset)             (((base) + (offset)))

/************************************************
 * Picture Manager Context Constructor
 ************************************************/
EbErrorType eb_vp9_picture_manager_context_ctor(
    PictureManagerContext **context_dbl_ptr,
    EbFifo                 *picture_input_fifo_ptr,
    EbFifo                 *picture_manager_output_fifo_ptr,
    EbFifo                **picture_control_set_fifo_ptr_array)
{
    PictureManagerContext *context_ptr;
    EB_MALLOC(PictureManagerContext*, context_ptr, sizeof(PictureManagerContext), EB_N_PTR);

    *context_dbl_ptr = context_ptr;

    context_ptr->picture_input_fifo_ptr         = picture_input_fifo_ptr;
    context_ptr->picture_manager_output_fifo_ptr   = picture_manager_output_fifo_ptr;
    context_ptr->picture_control_set_fifo_ptr_array = picture_control_set_fifo_ptr_array;

    return EB_ErrorNone;
}

/***************************************************************************************************
 * Picture Manager Kernel
 *
 * Notes on the Picture Manager:
 *
 * The Picture Manager Process performs the function of managing both the Input Picture buffers and
 * the Reference Picture buffers and subdividing the Input Picture into Tiles. Both the Input Picture
 * and Reference Picture buffers particular management depends on the GoP structure already implemented in
 * the Picture decision. Also note that the Picture Manager sets up the RPS for Entropy Coding as well.
 *
 * Inputs:
 * Input Picture
 *   -Input Picture Data
 *
 *  Reference Picture
 *   -Reference Picture Data
 *
 *  Outputs:
 *   -Picture Control Set with fully available Reference List
 *
 ***************************************************************************************************/
void* eb_vp9_PictureManagerKernel(void *input_ptr)
{
    PictureManagerContext        *context_ptr = (PictureManagerContext*) input_ptr;

    EbObjectWrapper              *child_picture_control_set_wrapper_ptr;
    PictureControlSet            *child_picture_control_set_ptr;
    PictureParentControlSet      *picture_control_set_ptr;
    SequenceControlSet           *sequence_control_set_ptr;
    EncodeContext                *encode_context_ptr;

    EbObjectWrapper              *input_picture_demux_wrapper_ptr;
    PictureDemuxResults          *input_picture_demux_ptr;

    EbObjectWrapper              *output_wrapper_ptr;
    RateControlTasks             *rate_control_tasks_ptr;

    EB_BOOL                       availability_flag;

    PredictionStructureEntry     *pred_position_ptr;

    // Dynamic GOP
    PredictionStructure          *nextpred_struct_ptr;
    PredictionStructureEntry     *next_base_layer_pred_position_ptr;

    uint32_t                      dependant_list_positive_entries;
    uint32_t                      dependantListRemovedEntries;

    InputQueueEntry              *input_entry_ptr;
    uint32_t                      input_queue_index;
    uint64_t                      current_input_poc;

    ReferenceQueueEntry          *reference_entry_ptr;
    uint32_t                      reference_queue_index;
    uint64_t                      ref_poc;

    uint32_t                      dep_idx;
    uint64_t                      dep_poc;
    uint32_t                      dep_list_count;

    PictureParentControlSet      *entry_picture_control_set_ptr;
    SequenceControlSet           *entrysequence_control_set_ptr;

    // Initialization
    uint8_t                       picture_width_in_sb;
    uint8_t                       picture_height_in_sb;

    PictureManagerReorderEntry   *queue_entry_ptr;
    int32_t                       queue_entry_index;

    // Debug
    uint32_t                      loop_count = 0;

    for(;;) {

        // Get Input Full Object
        eb_vp9_get_full_object(
            context_ptr->picture_input_fifo_ptr,
            &input_picture_demux_wrapper_ptr);

        input_picture_demux_ptr = (PictureDemuxResults*) input_picture_demux_wrapper_ptr->object_ptr;

        // *Note - This should be overhauled and/or replaced when we
        //   need hierarchical support.
        loop_count++;

        switch(input_picture_demux_ptr->picture_type) {

        case EB_PIC_INPUT:

            picture_control_set_ptr           = (PictureParentControlSet*) input_picture_demux_ptr->picture_control_set_wrapper_ptr->object_ptr;
            sequence_control_set_ptr          = (SequenceControlSet*) picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
            encode_context_ptr                = sequence_control_set_ptr->encode_context_ptr;

           //SVT_LOG("\nPicture Manager Process @ %d \n ", picture_control_set_ptr->picture_number);

           queue_entry_index = (int32_t)(picture_control_set_ptr->picture_number - encode_context_ptr->picture_manager_reorder_queue[encode_context_ptr->picture_manager_reorder_queue_head_index]->picture_number);
           queue_entry_index += encode_context_ptr->picture_manager_reorder_queue_head_index;
           queue_entry_index = (queue_entry_index > PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH - 1) ? queue_entry_index - PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH : queue_entry_index;
           queue_entry_ptr = encode_context_ptr->picture_manager_reorder_queue[queue_entry_index];
           if (queue_entry_ptr->parent_pcs_wrapper_ptr != NULL){
               CHECK_REPORT_ERROR_NC(
                   encode_context_ptr->app_callback_ptr,
                   EB_ENC_PD_ERROR7);
           }
           else{
               queue_entry_ptr->parent_pcs_wrapper_ptr = input_picture_demux_ptr->picture_control_set_wrapper_ptr;
               queue_entry_ptr->picture_number = picture_control_set_ptr->picture_number;
           }
           // Process the head of the Picture Manager Reorder Queue
           queue_entry_ptr = encode_context_ptr->picture_manager_reorder_queue[encode_context_ptr->picture_manager_reorder_queue_head_index];

           while (queue_entry_ptr->parent_pcs_wrapper_ptr != EB_NULL) {

               picture_control_set_ptr = (PictureParentControlSet  *)queue_entry_ptr->parent_pcs_wrapper_ptr->object_ptr;

               pred_position_ptr = picture_control_set_ptr->pred_struct_ptr->pred_struct_entry_ptr_array[picture_control_set_ptr->pred_struct_index];
#if NEW_PRED_STRUCT
               // If there was a change in the number of temporal layers, then cleanup the Reference Queue's Dependent Counts
               if (picture_control_set_ptr->hierarchical_layers_diff != 0) {

                   // Dynamic GOP
                   PredictionStructure          *nextPredStructPtr;
                   PredictionStructureEntry     *nextBaseLayerPredPositionPtr;

                   uint32_t                        dependantListPositiveEntries;
                   uint32_t                        dependantListRemovedEntries;

                   reference_queue_index = encode_context_ptr->reference_picture_queue_head_index;

                   while (reference_queue_index != encode_context_ptr->reference_picture_queue_tail_index) {

                       reference_entry_ptr = encode_context_ptr->reference_picture_queue[reference_queue_index];

                       if (reference_entry_ptr->picture_number == (picture_control_set_ptr->picture_number - 1)) { // Picture where the change happened

                           // Get the prediction struct entry of the next GOP structure
                           nextPredStructPtr = eb_vp9_get_prediction_structure(
                               encode_context_ptr->prediction_structure_group_ptr,
                               picture_control_set_ptr->pred_structure,
                               1,
                               picture_control_set_ptr->hierarchical_levels);

                           // Get the prediction struct of a picture in temporal layer 0 (from the new GOP structure)
                           nextBaseLayerPredPositionPtr = nextPredStructPtr->pred_struct_entry_ptr_array[nextPredStructPtr->pred_struct_entry_count - 1];

                           // Remove all positive entries from the dependant lists
                           dependantListPositiveEntries = 0;
                           for (dep_idx = 0; dep_idx < reference_entry_ptr->list0.list_count; ++dep_idx) {
                               if (reference_entry_ptr->list0.list[dep_idx] >= 0) {
                                   dependantListPositiveEntries++;
                               }
                           }
                           reference_entry_ptr->list0.list_count = reference_entry_ptr->list0.list_count - dependantListPositiveEntries;

                           dependantListPositiveEntries = 0;
                           for (dep_idx = 0; dep_idx < reference_entry_ptr->list1.list_count; ++dep_idx) {
                               if (reference_entry_ptr->list1.list[dep_idx] >= 0) {
                                   dependantListPositiveEntries++;
                               }
                           }
                           reference_entry_ptr->list1.list_count = reference_entry_ptr->list1.list_count - dependantListPositiveEntries;

                           for (dep_idx = 0; dep_idx < nextBaseLayerPredPositionPtr->dep_list0.list_count; ++dep_idx) {
                               if (nextBaseLayerPredPositionPtr->dep_list0.list[dep_idx] >= 0) {
                                   reference_entry_ptr->list0.list[reference_entry_ptr->list0.list_count++] = nextBaseLayerPredPositionPtr->dep_list0.list[dep_idx];
                               }
                           }

                           for (dep_idx = 0; dep_idx < nextBaseLayerPredPositionPtr->dep_list1.list_count; ++dep_idx) {
                               if (nextBaseLayerPredPositionPtr->dep_list1.list[dep_idx] >= 0) {
                                   reference_entry_ptr->list1.list[reference_entry_ptr->list1.list_count++] = nextBaseLayerPredPositionPtr->dep_list1.list[dep_idx];
                               }
                           }

                           // Update the dependant count update
                           dependantListRemovedEntries = reference_entry_ptr->dep_list0_count + reference_entry_ptr->dep_list1_count - reference_entry_ptr->dependent_count;

                           reference_entry_ptr->dep_list0_count = reference_entry_ptr->list0.list_count;
                           reference_entry_ptr->dep_list1_count = reference_entry_ptr->list1.list_count;
                           reference_entry_ptr->dependent_count = reference_entry_ptr->dep_list0_count + reference_entry_ptr->dep_list1_count - dependantListRemovedEntries;

                       }
                       else {

                           // Modify Dependent List0
                           dep_list_count = reference_entry_ptr->list0.list_count;
                           for (dep_idx = 0; dep_idx < dep_list_count; ++dep_idx) {

                               // Adjust the latest currentInputPoc in case we're in a POC rollover scenario
                               // currentInputPoc += (currentInputPoc < reference_entry_ptr->pocNumber) ? (1 << sequence_control_set_ptr->bitsForPictureOrderCount) : 0;

                               dep_poc = POC_CIRCULAR_ADD(
                                   reference_entry_ptr->picture_number, // can't use a value that gets reset
                                   reference_entry_ptr->list0.list[dep_idx]/*,
                                   sequence_control_set_ptr->bitsForPictureOrderCount*/);

                                   // If Dependent POC is greater or equal to the IDR POC
                               if (dep_poc >= picture_control_set_ptr->picture_number && reference_entry_ptr->list0.list[dep_idx]) {

                                   reference_entry_ptr->list0.list[dep_idx] = 0;

                                   // Decrement the Reference's referenceCount
                                   --reference_entry_ptr->dependent_count;

                                   CHECK_REPORT_ERROR(
                                       (reference_entry_ptr->dependent_count != ~0u),
                                       encode_context_ptr->app_callback_ptr,
                                       EB_ENC_PD_ERROR3);
                               }
                           }

                           // Modify Dependent List1
                           dep_list_count = reference_entry_ptr->list1.list_count;
                           for (dep_idx = 0; dep_idx < dep_list_count; ++dep_idx) {

                               // Adjust the latest currentInputPoc in case we're in a POC rollover scenario
                               // currentInputPoc += (currentInputPoc < reference_entry_ptr->pocNumber) ? (1 << sequence_control_set_ptr->bitsForPictureOrderCount) : 0;

                               dep_poc = POC_CIRCULAR_ADD(
                                   reference_entry_ptr->picture_number,
                                   reference_entry_ptr->list1.list[dep_idx]/*,
                                   sequence_control_set_ptr->bitsForPictureOrderCount*/);

                                   // If Dependent POC is greater or equal to the IDR POC
                               if ((dep_poc >= picture_control_set_ptr->picture_number) && reference_entry_ptr->list1.list[dep_idx]) {
                                   reference_entry_ptr->list1.list[dep_idx] = 0;

                                   // Decrement the Reference's referenceCount
                                   --reference_entry_ptr->dependent_count;

                                   CHECK_REPORT_ERROR(
                                       (reference_entry_ptr->dependent_count != ~0u),
                                       encode_context_ptr->app_callback_ptr,
                                       EB_ENC_PD_ERROR3);
                               }
                           }
                       }

                       // Increment the reference_queue_index Iterator
                       reference_queue_index = (reference_queue_index == REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : reference_queue_index + 1;
                   }
               }
#endif
               // If there was a change in the number of temporal layers, then cleanup the Reference Queue's Dependent Counts
               if (picture_control_set_ptr->hierarchical_layers_diff != 0) {

                   reference_queue_index = encode_context_ptr->reference_picture_queue_head_index;

                   while (reference_queue_index != encode_context_ptr->reference_picture_queue_tail_index) {

                       reference_entry_ptr = encode_context_ptr->reference_picture_queue[reference_queue_index];

                       if (reference_entry_ptr->picture_number == (picture_control_set_ptr->picture_number - 1)) { // Picture where the change happened

                           // Get the prediction struct entry of the next GOP structure
                           nextpred_struct_ptr = eb_vp9_get_prediction_structure(
                               encode_context_ptr->prediction_structure_group_ptr,
                               picture_control_set_ptr->pred_structure,
                               1,
                               picture_control_set_ptr->hierarchical_levels);

                           // Get the prediction struct of a picture in temporal layer 0 (from the new GOP structure)
                           next_base_layer_pred_position_ptr = nextpred_struct_ptr->pred_struct_entry_ptr_array[nextpred_struct_ptr->pred_struct_entry_count - 1];

                           // Remove all positive entries from the dependant lists
                           dependant_list_positive_entries = 0;
                           for (dep_idx = 0; dep_idx < reference_entry_ptr->list0.list_count; ++dep_idx) {
                               if (reference_entry_ptr->list0.list[dep_idx] >= 0) {
                                   dependant_list_positive_entries++;
                               }
                           }
                           reference_entry_ptr->list0.list_count = reference_entry_ptr->list0.list_count - dependant_list_positive_entries;

                           dependant_list_positive_entries = 0;
                           for (dep_idx = 0; dep_idx < reference_entry_ptr->list1.list_count; ++dep_idx) {
                               if (reference_entry_ptr->list1.list[dep_idx] >= 0) {
                                   dependant_list_positive_entries++;
                               }
                           }
                           reference_entry_ptr->list1.list_count = reference_entry_ptr->list1.list_count - dependant_list_positive_entries;

                           for (dep_idx = 0; dep_idx < next_base_layer_pred_position_ptr->dep_list0.list_count; ++dep_idx) {
                               if (next_base_layer_pred_position_ptr->dep_list0.list[dep_idx] >= 0) {
                                   reference_entry_ptr->list0.list[reference_entry_ptr->list0.list_count++] = next_base_layer_pred_position_ptr->dep_list0.list[dep_idx];
                               }
                           }

                           for (dep_idx = 0; dep_idx < next_base_layer_pred_position_ptr->dep_list1.list_count; ++dep_idx) {
                               if (next_base_layer_pred_position_ptr->dep_list1.list[dep_idx] >= 0) {
                                   reference_entry_ptr->list1.list[reference_entry_ptr->list1.list_count++] = next_base_layer_pred_position_ptr->dep_list1.list[dep_idx];
                               }
                           }

                           // Update the dependant count update
                           dependantListRemovedEntries = reference_entry_ptr->dep_list0_count + reference_entry_ptr->dep_list1_count - reference_entry_ptr->dependent_count;

                           reference_entry_ptr->dep_list0_count = reference_entry_ptr->list0.list_count;
                           reference_entry_ptr->dep_list1_count = reference_entry_ptr->list1.list_count;
                           reference_entry_ptr->dependent_count = reference_entry_ptr->dep_list0_count + reference_entry_ptr->dep_list1_count - dependantListRemovedEntries;

                       }
                       else {

                           // Modify Dependent List0
                           dep_list_count = reference_entry_ptr->list0.list_count;
                           for (dep_idx = 0; dep_idx < dep_list_count; ++dep_idx) {

                               // Adjust the latest current_input_poc in case we're in a POC rollover scenario
                               // current_input_poc += (current_input_poc < reference_entry_ptr->pocNumber) ? (1 << sequence_control_set_ptr->bitsForPictureOrderCount) : 0;

                               dep_poc = POC_CIRCULAR_ADD(
                                   reference_entry_ptr->picture_number, // can't use a value that gets reset
                                   reference_entry_ptr->list0.list[dep_idx]/*,
                                   sequence_control_set_ptr->bitsForPictureOrderCount*/);

                               // If Dependent POC is greater or equal to the IDR POC
                               if (dep_poc >= picture_control_set_ptr->picture_number && reference_entry_ptr->list0.list[dep_idx]) {

                                   reference_entry_ptr->list0.list[dep_idx] = 0;

                                   // Decrement the Reference's referenceCount
                                   --reference_entry_ptr->dependent_count;

                                   CHECK_REPORT_ERROR(
                                       (reference_entry_ptr->dependent_count != ~0u),
                                       encode_context_ptr->app_callback_ptr,
                                       EB_ENC_PD_ERROR3);
                               }
                           }

                           // Modify Dependent List1
                           dep_list_count = reference_entry_ptr->list1.list_count;
                           for (dep_idx = 0; dep_idx < dep_list_count; ++dep_idx) {

                               // Adjust the latest current_input_poc in case we're in a POC rollover scenario
                               // current_input_poc += (current_input_poc < reference_entry_ptr->pocNumber) ? (1 << sequence_control_set_ptr->bitsForPictureOrderCount) : 0;

                               dep_poc = POC_CIRCULAR_ADD(
                                   reference_entry_ptr->picture_number,
                                   reference_entry_ptr->list1.list[dep_idx]/*,
                                   sequence_control_set_ptr->bitsForPictureOrderCount*/);

                               // If Dependent POC is greater or equal to the IDR POC
                               if ((dep_poc >= picture_control_set_ptr->picture_number) && reference_entry_ptr->list1.list[dep_idx]) {
                                   reference_entry_ptr->list1.list[dep_idx] = 0;

                                   // Decrement the Reference's referenceCount
                                   --reference_entry_ptr->dependent_count;

                                   CHECK_REPORT_ERROR(
                                       (reference_entry_ptr->dependent_count != ~0u),
                                       encode_context_ptr->app_callback_ptr,
                                       EB_ENC_PD_ERROR3);
                               }
                           }
                       }

                       // Increment the reference_queue_index Iterator
                       reference_queue_index = (reference_queue_index == REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : reference_queue_index + 1;
                   }
               }

               // If there was an I-frame or Scene Change, then cleanup the Reference Queue's Dependent Counts
               if (picture_control_set_ptr->slice_type == I_SLICE)
               {

                   reference_queue_index = encode_context_ptr->reference_picture_queue_head_index;
                   while (reference_queue_index != encode_context_ptr->reference_picture_queue_tail_index) {

                       reference_entry_ptr = encode_context_ptr->reference_picture_queue[reference_queue_index];

                       // Modify Dependent List0
                       dep_list_count = reference_entry_ptr->list0.list_count;
                       for (dep_idx = 0; dep_idx < dep_list_count; ++dep_idx) {

                           current_input_poc = picture_control_set_ptr->picture_number;

                           // Adjust the latest current_input_poc in case we're in a POC rollover scenario
                           // current_input_poc += (current_input_poc < reference_entry_ptr->picture_number) ? (1 << sequence_control_set_ptr->bitsForPictureOrderCount) : 0;

                           dep_poc = POC_CIRCULAR_ADD(
                               reference_entry_ptr->picture_number, // can't use a value that gets reset
                               reference_entry_ptr->list0.list[dep_idx]/*,
                               sequence_control_set_ptr->bitsForPictureOrderCount*/);

                           // If Dependent POC is greater or equal to the IDR POC
                           if (dep_poc >= current_input_poc && reference_entry_ptr->list0.list[dep_idx]) {

                               reference_entry_ptr->list0.list[dep_idx] = 0;

                               // Decrement the Reference's referenceCount
                               --reference_entry_ptr->dependent_count;
                               CHECK_REPORT_ERROR(
                                   (reference_entry_ptr->dependent_count != ~0u),
                                   encode_context_ptr->app_callback_ptr,
                                   EB_ENC_PM_ERROR1);
                           }
                       }

                       // Modify Dependent List1
                       dep_list_count = reference_entry_ptr->list1.list_count;
                       for (dep_idx = 0; dep_idx < dep_list_count; ++dep_idx) {

                           current_input_poc = picture_control_set_ptr->picture_number;

                           // Adjust the latest current_input_poc in case we're in a POC rollover scenario
                           // current_input_poc += (current_input_poc < reference_entry_ptr->picture_number) ? (1 << sequence_control_set_ptr->bitsForPictureOrderCount) : 0;

                           dep_poc = POC_CIRCULAR_ADD(
                               reference_entry_ptr->picture_number,
                               reference_entry_ptr->list1.list[dep_idx]/*,
                               sequence_control_set_ptr->bitsForPictureOrderCount*/);

                           // If Dependent POC is greater or equal to the IDR POC or if we inserted trailing Ps
                           if (((dep_poc >= current_input_poc) || (((picture_control_set_ptr->pre_assignment_buffer_count != picture_control_set_ptr->pred_struct_ptr->pred_struct_period) || (picture_control_set_ptr->idr_flag == EB_TRUE)) && (dep_poc > (current_input_poc - picture_control_set_ptr->pre_assignment_buffer_count)))) && reference_entry_ptr->list1.list[dep_idx]) {

                               reference_entry_ptr->list1.list[dep_idx] = 0;

                               // Decrement the Reference's referenceCount
                               --reference_entry_ptr->dependent_count;
                               CHECK_REPORT_ERROR(
                                   (reference_entry_ptr->dependent_count != ~0u),
                                   encode_context_ptr->app_callback_ptr,
                                   EB_ENC_PM_ERROR1);
                           }

                       }

                       // Increment the reference_queue_index Iterator
                       reference_queue_index = (reference_queue_index == REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : reference_queue_index + 1;
                   }

               }
               else if (picture_control_set_ptr->idr_flag == EB_TRUE) {

                   // Set Reference Entry pointer
                   reference_entry_ptr = (ReferenceQueueEntry*)EB_NULL;
               }

               // Check if the EnhancedPictureQueue is full.
               // *Note - Having the number of Enhanced Pictures less than the InputQueueSize should ensure this never gets hit

               CHECK_REPORT_ERROR(
                   (((encode_context_ptr->input_picture_queue_head_index != encode_context_ptr->input_picture_queue_tail_index) || (encode_context_ptr->input_picture_queue[encode_context_ptr->input_picture_queue_head_index]->input_object_ptr == EB_NULL))),
                   encode_context_ptr->app_callback_ptr,
                   EB_ENC_PM_ERROR2);

               // Place Picture in input queue
               input_entry_ptr = encode_context_ptr->input_picture_queue[encode_context_ptr->input_picture_queue_tail_index];
               input_entry_ptr->input_object_ptr = queue_entry_ptr->parent_pcs_wrapper_ptr;
               input_entry_ptr->reference_entry_index = encode_context_ptr->reference_picture_queue_tail_index;
               encode_context_ptr->input_picture_queue_tail_index =
                   (encode_context_ptr->input_picture_queue_tail_index == INPUT_QUEUE_MAX_DEPTH - 1) ? 0 : encode_context_ptr->input_picture_queue_tail_index + 1;

               // Copy the reference lists into the inputEntry and
               // set the Reference Counts Based on Temporal Layer and how many frames are active
               picture_control_set_ptr->ref_list0_count = (picture_control_set_ptr->slice_type == I_SLICE) ? 0 : (uint8_t)pred_position_ptr->ref_list0.reference_list_count;
               picture_control_set_ptr->ref_list1_count = (picture_control_set_ptr->slice_type == I_SLICE) ? 0 : (uint8_t)pred_position_ptr->ref_list1.reference_list_count;
               input_entry_ptr->list0_ptr = &pred_position_ptr->ref_list0;
               input_entry_ptr->list1_ptr = &pred_position_ptr->ref_list1;

               // Check if the reference_picture_queue is full.
               CHECK_REPORT_ERROR(
                   (((encode_context_ptr->reference_picture_queue_head_index != encode_context_ptr->reference_picture_queue_tail_index) || (encode_context_ptr->reference_picture_queue[encode_context_ptr->reference_picture_queue_head_index]->reference_object_ptr == EB_NULL))),
                   encode_context_ptr->app_callback_ptr,
                   EB_ENC_PM_ERROR3);

               // Create Reference Queue Entry even if picture will not be referenced
               reference_entry_ptr = encode_context_ptr->reference_picture_queue[encode_context_ptr->reference_picture_queue_tail_index];
               reference_entry_ptr->picture_number = picture_control_set_ptr->picture_number;
               reference_entry_ptr->reference_object_ptr = (EbObjectWrapper*)EB_NULL;
               reference_entry_ptr->release_enable = EB_TRUE;
               reference_entry_ptr->reference_available = EB_FALSE;
               reference_entry_ptr->feedback_arrived = EB_FALSE;
               reference_entry_ptr->is_used_as_reference_flag = picture_control_set_ptr->is_used_as_reference_flag;
               encode_context_ptr->reference_picture_queue_tail_index =
                   (encode_context_ptr->reference_picture_queue_tail_index == REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : encode_context_ptr->reference_picture_queue_tail_index + 1;

               // Copy the Dependent Lists
               // *Note - we are removing any leading picture dependencies for now
               reference_entry_ptr->list0.list_count = 0;
               for (dep_idx = 0; dep_idx < pred_position_ptr->dep_list0.list_count; ++dep_idx) {
                   if (pred_position_ptr->dep_list0.list[dep_idx] >= 0) {
                       reference_entry_ptr->list0.list[reference_entry_ptr->list0.list_count++] = pred_position_ptr->dep_list0.list[dep_idx];
                   }
               }

               reference_entry_ptr->list1.list_count = pred_position_ptr->dep_list1.list_count;
               for (dep_idx = 0; dep_idx < pred_position_ptr->dep_list1.list_count; ++dep_idx) {
                   reference_entry_ptr->list1.list[dep_idx] = pred_position_ptr->dep_list1.list[dep_idx];
               }

               reference_entry_ptr->dep_list0_count = reference_entry_ptr->list0.list_count;
               reference_entry_ptr->dep_list1_count = reference_entry_ptr->list1.list_count;
               reference_entry_ptr->dependent_count = reference_entry_ptr->dep_list0_count + reference_entry_ptr->dep_list1_count;

               CHECK_REPORT_ERROR(
                   (picture_control_set_ptr->pred_struct_ptr->pred_struct_period < MAX_ELAPSED_IDR_COUNT),
                   encode_context_ptr->app_callback_ptr,
                   EB_ENC_PM_ERROR4);

               // Release the Reference Buffer once we know it is not a reference
               if (picture_control_set_ptr->is_used_as_reference_flag == EB_FALSE){
                   // Release the nominal live_count value
                   eb_vp9_release_object(picture_control_set_ptr->reference_picture_wrapper_ptr);
                   picture_control_set_ptr->reference_picture_wrapper_ptr = (EbObjectWrapper*)EB_NULL;
               }

               // Release the Picture Manager Reorder Queue
               queue_entry_ptr->parent_pcs_wrapper_ptr = (EbObjectWrapper*)EB_NULL;
               queue_entry_ptr->picture_number += PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH;

               // Increment the Picture Manager Reorder Queue
               encode_context_ptr->picture_manager_reorder_queue_head_index = (encode_context_ptr->picture_manager_reorder_queue_head_index == PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encode_context_ptr->picture_manager_reorder_queue_head_index + 1;

               // Get the next entry from the Picture Manager Reorder Queue (Entry N+1)
               queue_entry_ptr = encode_context_ptr->picture_manager_reorder_queue[encode_context_ptr->picture_manager_reorder_queue_head_index];

           }
           break;

        case EB_PIC_REFERENCE:

            sequence_control_set_ptr   = (SequenceControlSet *) input_picture_demux_ptr->sequence_control_set_wrapper_ptr->object_ptr;
            encode_context_ptr        = sequence_control_set_ptr->encode_context_ptr;

            // Check if Reference Queue is full
            CHECK_REPORT_ERROR(
                (encode_context_ptr->reference_picture_queue_head_index != encode_context_ptr->reference_picture_queue_tail_index),
                encode_context_ptr->app_callback_ptr,
                EB_ENC_PM_ERROR5);

            reference_queue_index = encode_context_ptr->reference_picture_queue_head_index;

            // Find the Reference in the Reference Queue
            do {

                reference_entry_ptr = encode_context_ptr->reference_picture_queue[reference_queue_index];

                if(reference_entry_ptr->picture_number == input_picture_demux_ptr->picture_number) {

                    // Assign the reference object if there is a match
                    reference_entry_ptr->reference_object_ptr = input_picture_demux_ptr->reference_picture_wrapper_ptr;

                    // Set the reference availability
                    reference_entry_ptr->reference_available = EB_TRUE;
                }

                // Increment the reference_queue_index Iterator
                reference_queue_index = (reference_queue_index == REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : reference_queue_index + 1;

            } while ((reference_queue_index != encode_context_ptr->reference_picture_queue_tail_index) && (reference_entry_ptr->picture_number != input_picture_demux_ptr->picture_number));

            CHECK_REPORT_ERROR(
                (reference_entry_ptr->picture_number == input_picture_demux_ptr->picture_number),
                encode_context_ptr->app_callback_ptr,
                EB_ENC_PM_ERROR6);

            //keep the relase of SCS here because we still need the encodeContext strucutre here
            // Release the Reference's SequenceControlSet
            eb_vp9_release_object(input_picture_demux_ptr->sequence_control_set_wrapper_ptr);

            break;

        default:

            sequence_control_set_ptr   = (SequenceControlSet *) input_picture_demux_ptr->sequence_control_set_wrapper_ptr->object_ptr;
            encode_context_ptr        = sequence_control_set_ptr->encode_context_ptr;

            CHECK_REPORT_ERROR_NC(
                encode_context_ptr->app_callback_ptr,
                EB_ENC_PM_ERROR7);

            picture_control_set_ptr    = (PictureParentControlSet  *) EB_NULL;
            encode_context_ptr        = (EncodeContext*) EB_NULL;

            break;
        }

        // ***********************************
        //  Common Code
        // *************************************

        // Walk the input queue and start all ready pictures.  Mark entry as null after started.  Increment the head as you go.
        if (encode_context_ptr != (EncodeContext*)EB_NULL) {
        input_queue_index = encode_context_ptr->input_picture_queue_head_index;
        while (input_queue_index != encode_context_ptr->input_picture_queue_tail_index) {

            input_entry_ptr = encode_context_ptr->input_picture_queue[input_queue_index];

            if(input_entry_ptr->input_object_ptr != EB_NULL) {

                entry_picture_control_set_ptr   = (PictureParentControlSet*)   input_entry_ptr->input_object_ptr->object_ptr;
                entrysequence_control_set_ptr  = (SequenceControlSet*)  entry_picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;

                availability_flag = EB_TRUE;

                // Check ref_list0 Availability
                 if (entry_picture_control_set_ptr->ref_list0_count){
                    reference_queue_index = (uint32_t) CIRCULAR_ADD(
                        ((int32_t) input_entry_ptr->reference_entry_index) -     // Base
                        input_entry_ptr->list0_ptr->reference_list,     // Offset
                        REFERENCE_QUEUE_MAX_DEPTH);                         // Max

                    reference_entry_ptr = encode_context_ptr->reference_picture_queue[reference_queue_index];

                    CHECK_REPORT_ERROR(
                        (reference_entry_ptr),
                        encode_context_ptr->app_callback_ptr,
                        EB_ENC_PM_ERROR8);

                    ref_poc = POC_CIRCULAR_ADD(
                        entry_picture_control_set_ptr->picture_number,
                        -input_entry_ptr->list0_ptr->reference_list/*,
                        entrysequence_control_set_ptr->bitsForPictureOrderCount*/);

                    // Increment the current_input_poc is the case of POC rollover
                    current_input_poc = encode_context_ptr->current_input_poc;
                    //current_input_poc += ((current_input_poc < ref_poc) && (input_entry_ptr->list0_ptr->reference_list[ref_idx] > 0)) ?
                    //    (1 << entrysequence_control_set_ptr->bitsForPictureOrderCount) :
                    //    0;

                    availability_flag =
                        (availability_flag == EB_FALSE)          ? EB_FALSE  :   // Don't update if already False
                        (ref_poc > current_input_poc)              ? EB_FALSE  :   // The Reference has not been received as an Input Picture yet, then its availability is false
                        (!encode_context_ptr->terminating_sequence_flag_received && (sequence_control_set_ptr->static_config.rate_control_mode && entry_picture_control_set_ptr->slice_type != I_SLICE && entry_picture_control_set_ptr->temporal_layer_index == 0 && !reference_entry_ptr->feedback_arrived)) ? EB_FALSE :
                        (reference_entry_ptr->reference_available) ? EB_TRUE   :   // The Reference has been completed
                                                                  EB_FALSE;     // The Reference has not been completed
                }

                // Check ref_list1 Availability
                if(entry_picture_control_set_ptr->slice_type == B_SLICE) {
                    if (entry_picture_control_set_ptr->ref_list1_count){
                        // If Reference is valid (non-zero), update the availability
                        if(input_entry_ptr->list1_ptr->reference_list != (int32_t) INVALID_POC) {

                            reference_queue_index = (uint32_t) CIRCULAR_ADD(
                                ((int32_t) input_entry_ptr->reference_entry_index) -     // Base
                                input_entry_ptr->list1_ptr->reference_list,     // Offset
                                REFERENCE_QUEUE_MAX_DEPTH);                         // Max

                            reference_entry_ptr = encode_context_ptr->reference_picture_queue[reference_queue_index];

                            CHECK_REPORT_ERROR(
                                (reference_entry_ptr),
                                encode_context_ptr->app_callback_ptr,
                                EB_ENC_PM_ERROR8);

                            ref_poc = POC_CIRCULAR_ADD(
                                entry_picture_control_set_ptr->picture_number,
                                -input_entry_ptr->list1_ptr->reference_list/*,
                                entrysequence_control_set_ptr->bitsForPictureOrderCount*/);

                            // Increment the current_input_poc is the case of POC rollover
                            current_input_poc = encode_context_ptr->current_input_poc;
                            //current_input_poc += ((current_input_poc < ref_poc && input_entry_ptr->list1_ptr->reference_list[ref_idx] > 0)) ?
                            //    (1 << entrysequence_control_set_ptr->bitsForPictureOrderCount) :
                            //    0;

                            availability_flag =
                                (availability_flag == EB_FALSE)          ? EB_FALSE  :   // Don't update if already False
                                (ref_poc > current_input_poc)              ? EB_FALSE  :   // The Reference has not been received as an Input Picture yet, then its availability is false
                                (!encode_context_ptr->terminating_sequence_flag_received && (sequence_control_set_ptr->static_config.rate_control_mode && entry_picture_control_set_ptr->slice_type != I_SLICE && entry_picture_control_set_ptr->temporal_layer_index == 0 && !reference_entry_ptr->feedback_arrived)) ? EB_FALSE :
                                (reference_entry_ptr->reference_available) ? EB_TRUE   :   // The Reference has been completed
                                                                          EB_FALSE;     // The Reference has not been completed
                        }
                    }
                }

                if(availability_flag == EB_TRUE) {
                    // Get New  Empty Child PCS from PCS Pool
                    eb_vp9_get_empty_object(
                        context_ptr->picture_control_set_fifo_ptr_array[0],
                        &child_picture_control_set_wrapper_ptr);

                    // Child PCS is released by Packetization
                    eb_vp9_object_inc_live_count(
                        child_picture_control_set_wrapper_ptr,
                        1);

                    child_picture_control_set_ptr     = (PictureControlSet  *) child_picture_control_set_wrapper_ptr->object_ptr;

                    //1.Link The Child PCS to its Parent
                    child_picture_control_set_ptr->picture_parent_control_set_wrapper_ptr = input_entry_ptr->input_object_ptr;
                    child_picture_control_set_ptr->parent_pcs_ptr                      = entry_picture_control_set_ptr;

                    //2. Have some common information between  ChildPCS and ParentPCS.
                    child_picture_control_set_ptr->sequence_control_set_wrapper_ptr             = entry_picture_control_set_ptr->sequence_control_set_wrapper_ptr;
                    child_picture_control_set_ptr->picture_qp                                = entry_picture_control_set_ptr->picture_qp;
                    child_picture_control_set_ptr->picture_number                            = entry_picture_control_set_ptr->picture_number;
                    child_picture_control_set_ptr->slice_type                                = entry_picture_control_set_ptr->slice_type ;
                    child_picture_control_set_ptr->temporal_layer_index                       = entry_picture_control_set_ptr->temporal_layer_index ;

                    child_picture_control_set_ptr->parent_pcs_ptr->total_num_bits               = 0;
                    child_picture_control_set_ptr->parent_pcs_ptr->picture_qp                  = entry_picture_control_set_ptr->picture_qp;
                    child_picture_control_set_ptr->parent_pcs_ptr->sad_me                      = 0;
                    child_picture_control_set_ptr->parent_pcs_ptr->quantized_coeff_num_bits      = 0;
                    child_picture_control_set_ptr->enc_mode                                  = entry_picture_control_set_ptr->enc_mode;

                    //3.make all  init for ChildPCS
                    picture_width_in_sb  = (uint8_t)((entrysequence_control_set_ptr->luma_width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE);
                    picture_height_in_sb = (uint8_t)((entrysequence_control_set_ptr->luma_height + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE);

                    // EncDec Segments
                    eb_vp9_enc_dec_segments_init(
                        child_picture_control_set_ptr->enc_dec_segment_ctrl,
                        entrysequence_control_set_ptr->enc_dec_segment_col_count_array[entry_picture_control_set_ptr->temporal_layer_index],
                        entrysequence_control_set_ptr->enc_dec_segment_row_count_array[entry_picture_control_set_ptr->temporal_layer_index],
                        picture_width_in_sb,
                        picture_height_in_sb);

                    // Entropy Coding Rows
                    {
                        unsigned row_index;

                        child_picture_control_set_ptr->entropy_coding_current_row = 0;
                        child_picture_control_set_ptr->entropy_coding_current_available_row = 0;
                        child_picture_control_set_ptr->entropy_coding_row_count = picture_height_in_sb;
                        child_picture_control_set_ptr->entropy_coding_in_progress = EB_FALSE;

                        for(row_index=0; row_index < MAX_SB_ROWS; ++row_index) {
                            child_picture_control_set_ptr->entropy_coding_row_array[row_index] = EB_FALSE;
                        }
                    }

#if SEG_SUPPORT
                    EB_MEMSET(&child_picture_control_set_ptr->segment_counts[0], 0, MAX_SEGMENTS*sizeof(int));
#endif
                    // is_low_delay
                    child_picture_control_set_ptr->is_low_delay          = (EB_BOOL)(child_picture_control_set_ptr->parent_pcs_ptr->pred_struct_ptr->pred_struct_entry_ptr_array[child_picture_control_set_ptr->parent_pcs_ptr->pred_struct_index]->positive_ref_pics_total_count == 0);

                    // Reset the Reference Lists
                    EB_MEMSET(child_picture_control_set_ptr->ref_pic_ptr_array, 0, 2 * sizeof(EbObjectWrapper*));

                    EB_MEMSET(child_picture_control_set_ptr->ref_pic_qp_array, 0,  2 * sizeof(uint8_t));

                    EB_MEMSET(child_picture_control_set_ptr->ref_slice_type_array, 0,  2 * sizeof(EB_SLICE));

                    // Configure List0
                    if ((entry_picture_control_set_ptr->slice_type == P_SLICE) || (entry_picture_control_set_ptr->slice_type == B_SLICE)) {

                        if (entry_picture_control_set_ptr->ref_list0_count){
                            reference_queue_index = (uint32_t) CIRCULAR_ADD(
                                ((int32_t) input_entry_ptr->reference_entry_index) - input_entry_ptr->list0_ptr->reference_list,
                                REFERENCE_QUEUE_MAX_DEPTH);                                                                                             // Max

                            reference_entry_ptr = encode_context_ptr->reference_picture_queue[reference_queue_index];

                            // Set the Reference Object
                            child_picture_control_set_ptr->ref_pic_ptr_array[REF_LIST_0] = reference_entry_ptr->reference_object_ptr;

                            child_picture_control_set_ptr->ref_pic_qp_array[REF_LIST_0]  = ((EbReferenceObject  *) reference_entry_ptr->reference_object_ptr->object_ptr)->qp;
                            child_picture_control_set_ptr->ref_slice_type_array[REF_LIST_0] = ((EbReferenceObject  *) reference_entry_ptr->reference_object_ptr->object_ptr)->slice_type;

                            // Increment the Reference's live_count by the number of tiles in the input picture
                            eb_vp9_object_inc_live_count(
                                reference_entry_ptr->reference_object_ptr,
                                1);

                            // Decrement the Reference's dependent_count Count
                            --reference_entry_ptr->dependent_count;

                            CHECK_REPORT_ERROR(
                                (reference_entry_ptr->dependent_count != ~0u),
                                encode_context_ptr->app_callback_ptr,
                                EB_ENC_PM_ERROR0);

                        }
                    }

                    // Configure List1
                    if (entry_picture_control_set_ptr->slice_type == B_SLICE) {

                        if (entry_picture_control_set_ptr->ref_list1_count){
                            reference_queue_index = (uint32_t) CIRCULAR_ADD(
                                ((int32_t) input_entry_ptr->reference_entry_index) - input_entry_ptr->list1_ptr->reference_list,
                                REFERENCE_QUEUE_MAX_DEPTH);                                                                                             // Max

                            reference_entry_ptr = encode_context_ptr->reference_picture_queue[reference_queue_index];

                            // Set the Reference Object
                            child_picture_control_set_ptr->ref_pic_ptr_array[REF_LIST_1] = reference_entry_ptr->reference_object_ptr;

                            child_picture_control_set_ptr->ref_pic_qp_array[REF_LIST_1]  = ((EbReferenceObject  *) reference_entry_ptr->reference_object_ptr->object_ptr)->qp;
                            child_picture_control_set_ptr->ref_slice_type_array[REF_LIST_1] = ((EbReferenceObject  *) reference_entry_ptr->reference_object_ptr->object_ptr)->slice_type;

                            // Increment the Reference's live_count by the number of tiles in the input picture
                            eb_vp9_object_inc_live_count(
                                reference_entry_ptr->reference_object_ptr,
                                1);

                            // Decrement the Reference's dependent_count Count
                            --reference_entry_ptr->dependent_count;

                            CHECK_REPORT_ERROR(
                                (reference_entry_ptr->dependent_count != ~0u),
                                encode_context_ptr->app_callback_ptr,
                                EB_ENC_PM_ERROR0);
                        }
                    }

                    // Adjust the Slice-type if the Lists are Empty, but don't reset the Prediction Structure
                    entry_picture_control_set_ptr->slice_type =
                        (entry_picture_control_set_ptr->ref_list1_count > 0) ? B_SLICE :
                        (entry_picture_control_set_ptr->ref_list0_count > 0) ? P_SLICE :
                                                                         I_SLICE;

                    // Increment the sequenceControlSet Wrapper's live count by 1 for only the pictures which are used as reference
                    if(child_picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag) {
                        eb_vp9_object_inc_live_count(
                            child_picture_control_set_ptr->parent_pcs_ptr->sequence_control_set_wrapper_ptr,
                            1);
                    }

                    // Get Empty Results Object
                    eb_vp9_get_empty_object(
                        context_ptr->picture_manager_output_fifo_ptr,
                        &output_wrapper_ptr);

                    rate_control_tasks_ptr                              = (RateControlTasks*) output_wrapper_ptr->object_ptr;
                    rate_control_tasks_ptr->picture_control_set_wrapper_ptr = child_picture_control_set_wrapper_ptr;
                    rate_control_tasks_ptr->task_type                    = RC_PICTURE_MANAGER_RESULT;

                    // Post the Full Results Object
                    eb_vp9_post_full_object(output_wrapper_ptr);

                    // Remove the Input Entry from the Input Queue
                    input_entry_ptr->input_object_ptr = (EbObjectWrapper*) EB_NULL;

                }
            }

            // Increment the head_index if the head is null
            encode_context_ptr->input_picture_queue_head_index =
                (encode_context_ptr->input_picture_queue[encode_context_ptr->input_picture_queue_head_index]->input_object_ptr) ? encode_context_ptr->input_picture_queue_head_index :
                (encode_context_ptr->input_picture_queue_head_index == INPUT_QUEUE_MAX_DEPTH - 1)                         ? 0
                                                                                                                    : encode_context_ptr->input_picture_queue_head_index + 1;

            // Increment the input_queue_index Iterator
            input_queue_index = (input_queue_index == INPUT_QUEUE_MAX_DEPTH - 1) ? 0 : input_queue_index + 1;

        }

        // Walk the reference queue and remove entries that have been completely referenced.
        reference_queue_index = encode_context_ptr->reference_picture_queue_head_index;
        while(reference_queue_index != encode_context_ptr->reference_picture_queue_tail_index) {

            reference_entry_ptr = encode_context_ptr->reference_picture_queue[reference_queue_index];

            // Remove the entry & release the reference if there are no remaining references
            if((reference_entry_ptr->dependent_count == 0) &&
               (reference_entry_ptr->reference_available)  &&
               (reference_entry_ptr->release_enable)       &&
               (reference_entry_ptr->reference_object_ptr))
            {
                // Release the nominal live_count value
                eb_vp9_release_object(reference_entry_ptr->reference_object_ptr);

                reference_entry_ptr->reference_object_ptr = (EbObjectWrapper*) EB_NULL;
                reference_entry_ptr->reference_available = EB_FALSE;
                reference_entry_ptr->is_used_as_reference_flag = EB_FALSE;
            }

            // Increment the head_index if the head is empty
            encode_context_ptr->reference_picture_queue_head_index =
                (encode_context_ptr->reference_picture_queue[encode_context_ptr->reference_picture_queue_head_index]->release_enable         == EB_FALSE)? encode_context_ptr->reference_picture_queue_head_index:
                (encode_context_ptr->reference_picture_queue[encode_context_ptr->reference_picture_queue_head_index]->reference_available    == EB_FALSE &&
                 encode_context_ptr->reference_picture_queue[encode_context_ptr->reference_picture_queue_head_index]->is_used_as_reference_flag == EB_TRUE) ? encode_context_ptr->reference_picture_queue_head_index:
                (encode_context_ptr->reference_picture_queue[encode_context_ptr->reference_picture_queue_head_index]->dependent_count > 0)               ? encode_context_ptr->reference_picture_queue_head_index:
                (encode_context_ptr->reference_picture_queue_head_index == REFERENCE_QUEUE_MAX_DEPTH - 1)                                           ? 0
                                                                                                                                              : encode_context_ptr->reference_picture_queue_head_index + 1;
            // Increment the reference_queue_index Iterator
            reference_queue_index = (reference_queue_index == REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : reference_queue_index + 1;
        }
        }

        // Release the Input Picture Demux Results
        eb_vp9_release_object(input_picture_demux_wrapper_ptr);

    }
return EB_NULL;
}
