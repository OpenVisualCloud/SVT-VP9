/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbRateControlResults.h"

EbErrorType eb_vp9_rate_control_results_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    RateControlResults *context_ptr;
    EB_MALLOC(RateControlResults*, context_ptr, sizeof(RateControlResults), EB_N_PTR);
    *object_dbl_ptr = (EbPtr) context_ptr;

    object_init_data_ptr = 0;

    (void) object_init_data_ptr;

    return EB_ErrorNone;
}
