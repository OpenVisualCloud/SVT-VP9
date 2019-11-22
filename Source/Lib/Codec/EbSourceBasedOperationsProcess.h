/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSourceBasedOperations_h
#define EbSourceBasedOperations_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbNoiseExtractAVX2.h"

/**************************************
 * Context
 **************************************/

typedef struct SourceBasedOperationsContext
{
    EbFifo   *initial_rate_control_results_input_fifo_ptr;
    EbFifo   *picture_demux_results_output_fifo_ptr;

     // local zz cost array
    uint32_t  picture_num_grass_sb;
    uint32_t  high_contrast_num;

    EB_BOOL   high_dist;
    uint8_t  *y_mean_ptr;
    uint8_t  *cr_mean_ptr;
    uint8_t  *cb_mean_ptr;

} SourceBasedOperationsContext;

/***************************************
 * Extern Function Declaration
 ***************************************/

extern EbErrorType eb_vp9_source_based_operations_context_ctor(
    SourceBasedOperationsContext **context_dbl_ptr,
    EbFifo                        *initial_rate_control_results_input_fifo_ptr,
    EbFifo                        *picture_demux_results_output_fifo_ptr);

extern void* eb_vp9_source_based_operations_kernel(void *input_ptr);

#endif // EbSourceBasedOperations_h
