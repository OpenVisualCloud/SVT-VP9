/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbRateControlTasks_h
#define EbRateControlTasks_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"

/**************************************
 * Tasks Types
 **************************************/
typedef enum RateControlTaskTypes
{
    RC_PICTURE_MANAGER_RESULT,
    RC_PACKETIZATION_FEEDBACK_RESULT,
    RC_ENTROPY_CODING_ROW_FEEDBACK_RESULT,
    RC_INVALID_TASK
} RateControlTaskTypes;

/**************************************
 * Process Results
 **************************************/
typedef struct RateControlTasks
{
    RateControlTaskTypes  task_type;
    EbObjectWrapper      *picture_control_set_wrapper_ptr;
    uint32_t              segment_index;

    // Following are valid for RC_ENTROPY_CODING_ROW_FEEDBACK_RESULT only
    uint64_t              picture_number;
    uint32_t              row_number;
    uint32_t              bit_count;

} RateControlTasks;

typedef struct RateControlTasksInitData {
    int junk;
} RateControlTasksInitData;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType eb_vp9_rate_control_tasks_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr);

#endif // EbRateControlTasks_h
