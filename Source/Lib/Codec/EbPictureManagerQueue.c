/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPictureManagerQueue.h"

EbErrorType eb_vp9_input_queue_entry_ctor(
    InputQueueEntry **entry_dbl_ptr)
{
    InputQueueEntry *entry_ptr;
    EB_MALLOC(InputQueueEntry*, entry_ptr, sizeof(InputQueueEntry), EB_N_PTR);
    *entry_dbl_ptr = entry_ptr;

    entry_ptr->input_object_ptr      = (EbObjectWrapper*) EB_NULL;
    entry_ptr->reference_entry_index = 0;
    entry_ptr->dependent_count      = 0;

    entry_ptr->list0_ptr = (ReferenceList*) EB_NULL;
    entry_ptr->list1_ptr = (ReferenceList*) EB_NULL;

    return EB_ErrorNone;
}

EbErrorType eb_vp9_reference_queue_entry_ctor(
    ReferenceQueueEntry **entry_dbl_ptr)
{
    ReferenceQueueEntry *entry_ptr;
    EB_MALLOC(ReferenceQueueEntry*, entry_ptr, sizeof(ReferenceQueueEntry), EB_N_PTR);
    *entry_dbl_ptr = entry_ptr;

    entry_ptr->reference_object_ptr  = (EbObjectWrapper*) EB_NULL;
    entry_ptr->picture_number       = ~0u;
    entry_ptr->dependent_count      = 0;
    entry_ptr->reference_available  = EB_FALSE;

    EB_MALLOC(int32_t*, entry_ptr->list0.list, sizeof(int32_t) * (1 << MAX_TEMPORAL_LAYERS) , EB_N_PTR);

    EB_MALLOC(int32_t*, entry_ptr->list1.list, sizeof(int32_t) * (1 << MAX_TEMPORAL_LAYERS) , EB_N_PTR);

    return EB_ErrorNone;
}
