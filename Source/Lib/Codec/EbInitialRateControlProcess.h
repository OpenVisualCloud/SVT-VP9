/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbInitialRateControl_h
#define EbInitialRateControl_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbRateControlProcess.h"

/**************************************
 * Context
 **************************************/
typedef struct InitialRateControlContext
{
    EbFifo *motion_estimation_results_input_fifo_ptr;
    EbFifo *initialrate_control_results_output_fifo_ptr;

} InitialRateControlContext;

/***************************************
 * Extern Function Declaration
 ***************************************/
extern EbErrorType eb_vp9_initial_eb_vp9_rate_control_context_ctor(
    InitialRateControlContext **context_dbl_ptr,
    EbFifo                     *motion_estimation_results_input_fifo_ptr,
    EbFifo                     *picture_demux_results_output_fifo_ptr);

extern void* eb_vp9_initial_eb_vp9_rate_control_kernel(void *input_ptr);

#endif // EbInitialRateControl_h
