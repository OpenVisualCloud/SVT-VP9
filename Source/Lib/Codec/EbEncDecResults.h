/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEncDecResults_h
#define EbEncDecResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Process Results
 **************************************/
typedef struct EncDecResults
{
    EbObjectWrapper *picture_control_set_wrapper_ptr;
    uint32_t         completed_sb_row_index_start;
    uint32_t         completed_sb_row_count;

} EncDecResults;

typedef struct EncDecResultsInitData {
    uint32_t         junk;
} EncDecResultsInitData;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType eb_vp9_enc_dec_results_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr);

#ifdef __cplusplus
}
#endif
#endif // EbEncDecResults_h
