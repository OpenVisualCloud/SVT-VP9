/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbResourceCoordinationResults.h"

EbErrorType eb_vp9_resource_coordination_result_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    ResourceCoordinationResults *object_ptr;
    EB_MALLOC(ResourceCoordinationResults*, object_ptr, sizeof(ResourceCoordinationResults), EB_N_PTR);

    *object_dbl_ptr = object_ptr;

    object_init_data_ptr = 0;
    (void)object_init_data_ptr;

    return EB_ErrorNone;
}
