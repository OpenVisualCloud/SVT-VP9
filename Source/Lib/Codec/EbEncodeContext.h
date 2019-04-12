/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEncodeContext_h
#define EbEncodeContext_h

#include <stdio.h>

#include "EbDefinitions.h"
#include "EbThreads.h"
#include "EbSvtVp9Enc.h"
#include "EbPictureDecisionReorderQueue.h"
#include "EbPictureDecisionQueue.h"
#include "EbPictureManagerQueue.h"
#include "EbPacketizationReorderQueue.h"
#include "EbInitialRateControlReorderQueue.h"
#include "EbPictureManagerReorderQueue.h"

#include "EbPredictionStructure.h"
#include "EbRateControlTables.h"

// *Note - the queues are small for testing purposes.  They should be increased when they are done.
#define PRE_ASSIGNMENT_MAX_DEPTH                    128     // should be large enough to hold an entire prediction period
#define INPUT_QUEUE_MAX_DEPTH                       5000
#define REFERENCE_QUEUE_MAX_DEPTH                   5000
#define PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH   5000

#define PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH            2048
#define INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH        2048
#define PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH             2048
#define HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH   10000 //2048 AMIR to be changed
#define PACKETIZATION_REORDER_QUEUE_MAX_DEPTH               2048

// RC Groups: They should be a power of 2, so we can replace % by &. 
// Instead of using x % y, we use x && (y-1)
#define PARALLEL_GOP_MAX_NUMBER                         256 
#define RC_GROUP_IN_GOP_MAX_NUMBER                      512
#define PICTURE_IN_RC_GROUP_MAX_NUMBER                  64

typedef struct EncodeContext 
{
    // Callback Functions
    EbCallback                        *app_callback_ptr;
                                      
    // Port Active                      
    EB_BOOL                            recon_port_active;
                                      
    EbHandle                           total_number_of_recon_frame_mutex;
    uint64_t                           total_number_of_recon_frames;
                                      
    // Output Buffer Fifos              
    EbFifo                            *stream_output_fifo_ptr;
    EbFifo                            *recon_output_fifo_ptr;
    // Picture Buffer Fifos              
    EbFifo                            *input_picture_pool_fifo_ptr;
    EbFifo                            *reference_picture_pool_fifo_ptr;
    EbFifo                            *pa_reference_picture_pool_fifo_ptr;
    
    // Picture Decision Reorder Queue
    PictureDecisionReorderEntry      **picture_decision_reorder_queue;
    uint32_t                           picture_decision_reorder_queue_head_index;
    
    // Picture Manager Reorder Queue
    PictureManagerReorderEntry        **picture_manager_reorder_queue;
    uint32_t                            picture_manager_reorder_queue_head_index;

    // Picture Manager Pre-Assignment Buffer
    uint32_t                            pre_assignment_buffer_intra_count;
    uint32_t                            pre_assignment_buffer_idr_count;
    uint32_t                            pre_assignment_buffer_scene_change_count;
    uint32_t                            pre_assignment_buffer_scene_change_index;
    uint32_t                            pre_assignment_buffer_eos_flag;
    uint64_t                            decode_base_number;
    
    EbObjectWrapper                   **pre_assignment_buffer;
    uint32_t                            pre_assignment_buffer_count;
    uint32_t                            number_of_active_pictures;

    // Picture Decision Circular Queues
    PaReferenceQueueEntry             **picture_decision_pa_reference_queue;
    uint32_t                            picture_decision_pa_reference_queue_head_index;
    uint32_t                            picture_decision_pa_reference_queue_tail_index;

    // Picture Manager Circular Queues
    InputQueueEntry                   **input_picture_queue;
    uint32_t                            input_picture_queue_head_index;
    uint32_t                            input_picture_queue_tail_index;

    ReferenceQueueEntry               **reference_picture_queue;
    uint32_t                            reference_picture_queue_head_index;
    uint32_t                            reference_picture_queue_tail_index;
    
    // Initial Rate Control Reorder Queue
    InitialRateControlReorderEntry    **initial_rate_control_reorder_queue;
    uint32_t                            initial_rate_control_reorder_queue_head_index;

    // High Level Rate Control Histogram Queue
    HlRateControlHistogramEntry       **hl_rate_control_historgram_queue;
    uint32_t                            hl_rate_control_historgram_queue_head_index;
    EbHandle                            hl_rate_control_historgram_queue_mutex;

    // Packetization Reorder Queue
    PacketizationReorderEntry         **packetization_reorder_queue;
    uint32_t                            packetization_reorder_queue_head_index;
    
    // GOP Counters
    uint32_t                            intra_period_position;        // Current position in intra period
    uint32_t                            pred_struct_position;         // Current position within a prediction structure

    uint32_t                            elapsed_non_idr_count;
    uint32_t                            elapsed_non_cra_count;
    int64_t                             current_input_poc;
    EB_BOOL                             initial_picture;

    uint64_t                            last_idr_picture; // the most recently occured IDR picture (in decode order)

    // Sequence Termination Flags
    uint64_t                            terminating_picture_number;
    EB_BOOL                             terminating_sequence_flag_received;

    // Prediction Structure
    PredictionStructureGroup           *prediction_structure_group_ptr;

    // Rate Control Bit Tables
    RateControlTables                  *rate_control_tables_array;
    EB_BOOL                             rate_control_tables_array_updated;
    EbHandle                            rate_table_update_mutex;

    // Speed Control
    int64_t                             sc_buffer;
    int64_t                             sc_frame_in;
    int64_t                             sc_frame_out;
    EbHandle                            sc_buffer_mutex;

    EB_ENC_MODE                         enc_mode;

    // Rate Control
    uint32_t                            available_target_bitrate;
    EB_BOOL                             available_target_bitrate_changed;

    uint32_t                            previous_selected_ref_qp;
    uint64_t                            max_coded_poc;
    uint32_t                            max_coded_poc_selected_ref_qp;

    // Dynamic GOP
    uint32_t                            previous_mini_gop_hierarchical_levels;

    EbObjectWrapper                    *previous_picture_control_set_wrapper_ptr;
    EbHandle                            shared_reference_mutex;


} EncodeContext;

typedef struct EncodeContextInitData {
    int junk;
} EncodeContextInitData;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType encode_context_ctor(
    EbPtr  *object_dbl_ptr, 
    EbPtr   object_init_data_ptr);
    

#endif // EbEncodeContext_h
