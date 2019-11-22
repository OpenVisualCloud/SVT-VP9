/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbInitialRateControlResults_h
#define EbInitialRateControlResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"

/**************************************
 * Process Results
 **************************************/
typedef struct InitialRateControlResults {
    EbObjectWrapper *picture_control_set_wrapper_ptr;
} InitialRateControlResults;

typedef struct InitialRateControlResultInitData {
    int junk;
} InitialRateControlResultInitData;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType eb_vp9_initial_eb_vp9_rate_control_results_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr);

#endif //EbInitialRateControlResults_h
