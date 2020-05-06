/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbEncodeContext.h"
#include "EbPictureManagerQueue.h"

EbErrorType eb_vp9_encode_context_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    uint32_t picture_index;
    EbErrorType return_error = EB_ErrorNone;

    EncodeContext *encode_context_ptr;
    (void)object_init_data_ptr;
    EB_MALLOC(EncodeContext*, encode_context_ptr, sizeof(EncodeContext), EB_N_PTR);
    *object_dbl_ptr = (EbPtr) encode_context_ptr;

    // Callback Functions
    encode_context_ptr->app_callback_ptr                                          = (EbCallback  *) EB_NULL;

    // Port Active State
    encode_context_ptr->recon_port_active                                         = EB_FALSE;

    // Port Active State
    EB_CREATEMUTEX(EbHandle, encode_context_ptr->total_number_of_recon_frame_mutex, sizeof(EbHandle), EB_MUTEX);

    encode_context_ptr->total_number_of_recon_frames                              = 0;
    // Output Buffer Fifos
    encode_context_ptr->stream_output_fifo_ptr                                    = (EbFifo*) EB_NULL;
    encode_context_ptr->recon_output_fifo_ptr                                     = (EbFifo*)EB_NULL;

    // Picture Buffer Fifos
    encode_context_ptr->input_picture_pool_fifo_ptr                               = (EbFifo*) EB_NULL;
    encode_context_ptr->reference_picture_pool_fifo_ptr                           = (EbFifo*) EB_NULL;
    encode_context_ptr->pa_reference_picture_pool_fifo_ptr                        = (EbFifo*) EB_NULL;

    // Picture Decision Reordering Queue
    encode_context_ptr->picture_decision_reorder_queue_head_index                 = 0;
    EB_MALLOC(PictureDecisionReorderEntry**, encode_context_ptr->picture_decision_reorder_queue, sizeof(PictureDecisionReorderEntry*) * PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH, EB_N_PTR);

    for(picture_index=0; picture_index < PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH; ++picture_index) {
        return_error = eb_vp9_picture_decision_reorder_entry_ctor(
            &(encode_context_ptr->picture_decision_reorder_queue[picture_index]),
            picture_index);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // Picture Manager Reordering Queue
    encode_context_ptr->picture_manager_reorder_queue_head_index = 0;
    EB_MALLOC(PictureManagerReorderEntry**, encode_context_ptr->picture_manager_reorder_queue, sizeof(PictureManagerReorderEntry*) * PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH, EB_N_PTR);

    for (picture_index = 0; picture_index < PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH; ++picture_index) {
        return_error = eb_vp9_picture_manager_reorder_entry_ctor(
            &(encode_context_ptr->picture_manager_reorder_queue[picture_index]),
            picture_index);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // Picture Manager Pre-Assignment Buffer
    encode_context_ptr->pre_assignment_buffer_intra_count                     = 0;
    encode_context_ptr->pre_assignment_buffer_idr_count                       = 0;
    encode_context_ptr->pre_assignment_buffer_scene_change_count               = 0;
    encode_context_ptr->pre_assignment_buffer_scene_change_index               = 0;
    encode_context_ptr->pre_assignment_buffer_eos_flag                        = EB_FALSE;
    encode_context_ptr->decode_base_number                                  = 0;

    encode_context_ptr->pre_assignment_buffer_count                          = 0;
    encode_context_ptr->number_of_active_pictures                            = 0;

    EB_MALLOC(EbObjectWrapper**, encode_context_ptr->pre_assignment_buffer, sizeof(EbObjectWrapper*) * PRE_ASSIGNMENT_MAX_DEPTH, EB_N_PTR);

    for(picture_index=0; picture_index < PRE_ASSIGNMENT_MAX_DEPTH; ++picture_index) {
        encode_context_ptr->pre_assignment_buffer[picture_index] = (EbObjectWrapper*) EB_NULL;
    }

    // Picture Manager Input Queue
    encode_context_ptr->input_picture_queue_head_index                        = 0;
    encode_context_ptr->input_picture_queue_tail_index                        = 0;
    EB_MALLOC(InputQueueEntry**, encode_context_ptr->input_picture_queue, sizeof(InputQueueEntry*) * INPUT_QUEUE_MAX_DEPTH, EB_N_PTR);

    for(picture_index=0; picture_index < INPUT_QUEUE_MAX_DEPTH; ++picture_index) {
        return_error = eb_vp9_input_queue_entry_ctor(
            &(encode_context_ptr->input_picture_queue[picture_index]));
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // Picture Manager Reference Queue
    encode_context_ptr->reference_picture_queue_head_index                    = 0;
    encode_context_ptr->reference_picture_queue_tail_index                    = 0;
    EB_MALLOC(ReferenceQueueEntry**, encode_context_ptr->reference_picture_queue, sizeof(ReferenceQueueEntry*) * REFERENCE_QUEUE_MAX_DEPTH, EB_N_PTR);

    for(picture_index=0; picture_index < REFERENCE_QUEUE_MAX_DEPTH; ++picture_index) {
        return_error = eb_vp9_reference_queue_entry_ctor(
            &(encode_context_ptr->reference_picture_queue[picture_index]));
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // Picture Decision PA Reference Queue
    encode_context_ptr->picture_decision_pa_reference_queue_head_index                     = 0;
    encode_context_ptr->picture_decision_pa_reference_queue_tail_index                     = 0;
    EB_MALLOC(PaReferenceQueueEntry**, encode_context_ptr->picture_decision_pa_reference_queue, sizeof(PaReferenceQueueEntry*) * PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH, EB_N_PTR);

    for(picture_index=0; picture_index < PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH; ++picture_index) {
        return_error = eb_vp9_pa_eb_vp9_reference_queue_entry_ctor(
            &(encode_context_ptr->picture_decision_pa_reference_queue[picture_index]));
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // Initial Rate Control Reordering Queue
    encode_context_ptr->initial_rate_control_reorder_queue_head_index                 = 0;
    EB_MALLOC(InitialRateControlReorderEntry**, encode_context_ptr->initial_rate_control_reorder_queue, sizeof(InitialRateControlReorderEntry*) * INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH, EB_N_PTR);

    for(picture_index=0; picture_index < INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH; ++picture_index) {
        return_error = eb_vp9_initial_rate_control_reorder_entry_ctor(
            &(encode_context_ptr->initial_rate_control_reorder_queue[picture_index]),
            picture_index);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // High level Rate Control histogram Queue
    encode_context_ptr->hl_rate_control_historgram_queue_head_index                 = 0;

    EB_MALLOC(HlRateControlHistogramEntry**, encode_context_ptr->hl_rate_control_historgram_queue, sizeof(HlRateControlHistogramEntry*) * HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH, EB_N_PTR);

    for(picture_index=0; picture_index < HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH; ++picture_index) {
        return_error = eb_vp9_hl_rate_control_histogram_entry_ctor(
            &(encode_context_ptr->hl_rate_control_historgram_queue[picture_index]),
            picture_index);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    // HLRateControl Historgram Queue Mutex
    EB_CREATEMUTEX(EbHandle, encode_context_ptr->hl_rate_control_historgram_queue_mutex, sizeof(EbHandle), EB_MUTEX);

    // Packetization Reordering Queue
    encode_context_ptr->packetization_reorder_queue_head_index                 = 0;
    EB_MALLOC(PacketizationReorderEntry**, encode_context_ptr->packetization_reorder_queue, sizeof(PacketizationReorderEntry*) * PACKETIZATION_REORDER_QUEUE_MAX_DEPTH, EB_N_PTR);

    for(picture_index=0; picture_index < PACKETIZATION_REORDER_QUEUE_MAX_DEPTH; ++picture_index) {
        return_error = eb_vp9_packetization_reorder_entry_ctor(
            &(encode_context_ptr->packetization_reorder_queue[picture_index]),
            picture_index);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    encode_context_ptr->intra_period_position                               = 0;
    encode_context_ptr->pred_struct_position                                = 0;
    encode_context_ptr->current_input_poc                                   = -1;
    encode_context_ptr->elapsed_non_idr_count                                = 0;
    encode_context_ptr->initial_picture                                    = EB_TRUE;

    encode_context_ptr->last_idr_picture                                    = 0;

    // Sequence Termination Flags
    encode_context_ptr->terminating_picture_number                          = ~0u;
    encode_context_ptr->terminating_sequence_flag_received                   = EB_FALSE;

    // Prediction Structure Group
    encode_context_ptr->prediction_structure_group_ptr                       = (PredictionStructureGroup*) EB_NULL;

    // Rate Control
    encode_context_ptr->available_target_bitrate                            = 10000000;
    encode_context_ptr->available_target_bitrate_changed                     = EB_FALSE;
    encode_context_ptr->buffer_fill                                            = 0;
    encode_context_ptr->vbv_buf_size                                           = 0;
    encode_context_ptr->vbv_max_rate                                           = 0;

    // Rate Control Bit Tables
    EB_MALLOC(RateControlTables*, encode_context_ptr->rate_control_tables_array, sizeof(RateControlTables) * TOTAL_NUMBER_OF_INITIAL_RC_TABLES_ENTRY, EB_N_PTR);

    return_error = rate_control_tables_ctor(encode_context_ptr->rate_control_tables_array);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // RC Rate Table Update Mutex
    EB_CREATEMUTEX(EbHandle, encode_context_ptr->rate_table_update_mutex, sizeof(EbHandle), EB_MUTEX);

    encode_context_ptr->rate_control_tables_array_updated = EB_FALSE;

    EB_CREATEMUTEX(EbHandle, encode_context_ptr->sc_buffer_mutex, sizeof(EbHandle), EB_MUTEX);
    encode_context_ptr->sc_buffer      = 0;
    encode_context_ptr->sc_frame_in     = 0;
    encode_context_ptr->sc_frame_out    = 0;
    encode_context_ptr->enc_mode = SPEED_CONTROL_INIT_MOD;
    encode_context_ptr->previous_selected_ref_qp = 32;
    encode_context_ptr->max_coded_poc = 0;
    encode_context_ptr->max_coded_poc_selected_ref_qp = 32;

     encode_context_ptr->shared_reference_mutex = eb_vp9_create_mutex();
    if (encode_context_ptr->shared_reference_mutex  == (EbHandle) EB_NULL){
        return EB_ErrorInsufficientResources;
    }else {
        memory_map[*(memory_map_index)].ptr_type          = EB_MUTEX;
        memory_map[(*(memory_map_index))++].ptr          = encode_context_ptr->shared_reference_mutex ;
        *total_lib_memory                              += (sizeof(EbHandle));
    }

    return EB_ErrorNone;
}
