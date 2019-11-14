/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMotionEstimationResults_h
#define EbMotionEstimationResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Process Results
 **************************************/
typedef struct MotionEstimationResults
{
    EbObjectWrapper *picture_control_set_wrapper_ptr;
    uint32_t         segment_index;
} MotionEstimationResults;

typedef struct MotionEstimationResultsInitData {
    int junk;
} MotionEstimationResultsInitData;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType eb_vp9_motion_estimation_results_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr);

#ifdef __cplusplus
}
#endif
#endif // EbMotionEstimationResults_h
