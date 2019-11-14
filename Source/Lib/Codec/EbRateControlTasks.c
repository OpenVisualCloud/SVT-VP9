/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbRateControlTasks.h"

EbErrorType eb_vp9_rate_control_tasks_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    RateControlTasks *context_ptr;
    EB_MALLOC(RateControlTasks*, context_ptr, sizeof(RateControlTasks), EB_N_PTR);

    *object_dbl_ptr = (EbPtr) context_ptr;
    object_init_data_ptr = EB_NULL;

    (void) object_init_data_ptr;

    return EB_ErrorNone;
}
