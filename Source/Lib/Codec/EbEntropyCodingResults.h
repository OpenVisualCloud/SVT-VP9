/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEntropyCodingResults_h
#define EbEntropyCodingResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Process Results
 **************************************/
typedef struct EntropyCodingResults {
    EbObjectWrapper *picture_control_set_wrapper_ptr;
} EntropyCodingResults;

typedef struct EntropyCodingResultsInitData {
    uint32_t junk;
} EntropyCodingResultsInitData;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType eb_vp9_entropy_coding_results_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr);

#ifdef __cplusplus
}
#endif
#endif // EbEntropyCodingResults_h
