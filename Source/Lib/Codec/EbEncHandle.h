/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEncHandle_h
#define EbEncHandle_h

#include "EbDefinitions.h"
#include "EbSvtVp9Enc.h"

#include "EbPictureBufferDesc.h"
#include "EbSystemResourceManager.h"
#include "EbSequenceControlSet.h"

#include "EbResourceCoordinationResults.h"
#include "EbPictureDemuxResults.h"
#include "EbRateControlResults.h"

/**************************************
 * Component Private Data
 **************************************/  
typedef struct EbEncHandle
{
    // Encode Instances & Compute Segments
    uint32_t                              encode_instance_total_count;
    uint32_t                             *compute_segments_total_count_array;
    
    // Config Set Counts
    uint32_t                              sequence_control_set_pool_total_count; 
    
    // Full Results Count
    uint32_t                              picture_control_set_pool_total_count;   
        
    // Picture Buffer Counts
    uint32_t                              reference_picture_pool_total_count;
    
    // Config Set Pool & Active Array
    EbSystemResource                     *sequence_control_set_pool_ptr;
    EbFifo                              **sequence_control_set_pool_producer_fifo_ptr_array;
    EbSequenceControlSetInstance        **sequence_control_set_instance_array;
    
    // Full Results 
    EbSystemResource                    **picture_control_set_pool_ptr_array;
    EbFifo                             ***picture_control_set_pool_producer_fifo_ptr_dbl_array;

    //ParentControlSet
    EbSystemResource                    **picture_parent_control_set_pool_ptr_array;
    EbFifo                             ***picture_parent_control_set_pool_producer_fifo_ptr_dbl_array;
        
    // Picture Buffers
    EbSystemResource                    **reference_picture_pool_ptr_array;
    EbSystemResource                    **pa_reference_picture_pool_ptr_array;
    
    // Picture Buffer Producer Fifos   
    EbFifo                             ***reference_picture_pool_producer_fifo_ptr_dbl_array;
    EbFifo                             ***pa_reference_picture_pool_producer_fifo_ptr_dbl_array;
    
    // Thread Handles
    EbHandle                              resource_coordination_thread_handle;
    EbHandle                             *picture_analysis_thread_handle_array;
    EbHandle                              picture_decision_thread_handle;
    EbHandle                             *motion_estimation_thread_handle_array;
    EbHandle                              initial_rate_control_thread_handle;
    EbHandle                             *source_based_operations_thread_handle_array;
    EbHandle                              picture_manager_thread_handle;
    EbHandle                              rate_control_thread_handle;
    EbHandle                             *mode_decision_configuration_thread_handle_array;
    EbHandle                             *enc_dec_thread_handle_array;
    EbHandle                             *entropy_coding_thread_handle_array;
    EbHandle                              packetization_thread_handle;
        
    // Contexts
    EbPtr                                 resource_coordination_context_ptr;
    EbPtr                                *picture_analysis_context_ptr_array;
    EbPtr                                 picture_decision_context_ptr;
    EbPtr                                *motion_estimation_context_ptr_array;
    EbPtr                                 initial_rate_control_context_ptr;
    EbPtr                                *source_based_operations_context_ptr_array;
    EbPtr                                 picture_manager_context_ptr;
    EbPtr                                 rate_control_context_ptr;
    EbPtr                                *mode_decision_configuration_context_ptr_array;
    EbPtr                                *enc_dec_context_ptr_array;
    EbPtr                                *entropy_coding_context_ptr_array;
    EbPtr                                 packetization_context_ptr;
    
    // System Resource Managers
    EbSystemResource                     *input_buffer_resource_ptr;
    EbSystemResource                    **output_stream_buffer_resource_ptr_array;
    EbSystemResource                     *resource_coordination_results_resource_ptr;
    EbSystemResource                     *picture_analysis_results_resource_ptr;
    EbSystemResource                     *picture_decision_results_resource_ptr;
    EbSystemResource                     *motion_estimation_results_resource_ptr;
    EbSystemResource                     *initial_rate_control_results_resource_ptr;
    EbSystemResource                     *picture_demux_results_resource_ptr;
    EbSystemResource                     *rate_control_tasks_resource_ptr;
    EbSystemResource                     *rate_control_results_resource_ptr;
    EbSystemResource                     *enc_dec_tasks_resource_ptr;
    EbSystemResource                     *enc_dec_results_resource_ptr;
    EbSystemResource                     *entropy_coding_results_resource_ptr;
    EbSystemResource                    **output_recon_buffer_resource_ptr_array;

    // Inter-Process Producer Fifos
    EbFifo                              **input_buffer_producer_fifo_ptr_array;
    EbFifo                             ***output_stream_buffer_producer_fifo_ptr_dbl_array;
    EbFifo                              **resource_coordination_results_producer_fifo_ptr_array;
    EbFifo                              **picture_analysis_results_producer_fifo_ptr_array;
    EbFifo                              **picture_decision_results_producer_fifo_ptr_array;
    EbFifo                              **motion_estimation_results_producer_fifo_ptr_array;
    EbFifo                              **initial_rate_control_results_producer_fifo_ptr_array;
    EbFifo                              **picture_demux_results_producer_fifo_ptr_array;
    EbFifo                              **picture_manager_results_producer_fifo_ptr_array;
    EbFifo                              **rate_control_tasks_producer_fifo_ptr_array;
    EbFifo                              **rate_control_results_producer_fifo_ptr_array;
    EbFifo                              **enc_dec_tasks_producer_fifo_ptr_array;
    EbFifo                              **enc_dec_results_producer_fifo_ptr_array;
    EbFifo                              **entropy_coding_results_producer_fifo_ptr_array;
    EbFifo                             ***output_recon_buffer_producer_fifo_ptr_dbl_array;

    // Inter-Process Consumer Fifos
    EbFifo                              **input_buffer_consumer_fifo_ptr_array;
    EbFifo                             ***output_stream_buffer_consumer_fifo_ptr_dbl_array;
    EbFifo                              **resource_coordination_results_consumer_fifo_ptr_array;
    EbFifo                              **picture_analysis_results_consumer_fifo_ptr_array;
    EbFifo                              **picture_decision_results_consumer_fifo_ptr_array;
    EbFifo                              **motion_estimation_results_consumer_fifo_ptr_array;
    EbFifo                              **initial_rate_control_results_consumer_fifo_ptr_array;
    EbFifo                              **picture_demux_results_consumer_fifo_ptr_array;
    EbFifo                              **rate_control_tasks_consumer_fifo_ptr_array;
    EbFifo                              **rate_control_results_consumer_fifo_ptr_array;
    EbFifo                              **enc_dec_tasks_consumer_fifo_ptr_array;
    EbFifo                              **enc_dec_results_consumer_fifo_ptr_array;
    EbFifo                              **entropy_coding_results_consumer_fifo_ptr_array;
    EbFifo                             ***output_recon_buffer_consumer_fifo_ptr_dbl_array;

    // Callbacks
    EbCallback                          **app_callback_ptr_array;
    // Memory Map
    EbMemoryMapEntry                     *memory_map; 
    uint32_t                              memory_map_index;
    uint64_t                              total_lib_memory;

} EbEncHandle;

/**************************************
 * EbBufferHeaderType Constructor
 **************************************/  
extern EbErrorType eb_buffer_header_ctor(
    EbPtr *object_dbl_ptr, 
    EbPtr  object_init_data_ptr);
    
#endif // EbEncHandle_h
