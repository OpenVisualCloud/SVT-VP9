/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPictureDecisionQueue.h"

EbErrorType eb_vp9_pa_eb_vp9_reference_queue_entry_ctor(
    PaReferenceQueueEntry **entry_dbl_ptr)
{
    PaReferenceQueueEntry *entry_ptr;
    EB_MALLOC(PaReferenceQueueEntry*, entry_ptr, sizeof(PaReferenceQueueEntry), EB_N_PTR);
    *entry_dbl_ptr = entry_ptr;

    entry_ptr->input_object_ptr        = (EbObjectWrapper*) EB_NULL;
    entry_ptr->picture_number          = 0;
    entry_ptr->reference_entry_index   = 0;
    entry_ptr->dependent_count         = 0;
    entry_ptr->list0_ptr               = (ReferenceList*) EB_NULL;
    entry_ptr->list1_ptr               = (ReferenceList*) EB_NULL;
    EB_MALLOC(int32_t*, entry_ptr->list0.list, sizeof(int32_t) * (1 << MAX_TEMPORAL_LAYERS) , EB_N_PTR);

    EB_MALLOC(int32_t*, entry_ptr->list1.list, sizeof(int32_t) * (1 << MAX_TEMPORAL_LAYERS) , EB_N_PTR);

    return EB_ErrorNone;
}
