/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPictureDecisionReorderQueue.h"

EbErrorType eb_vp9_picture_decision_reorder_entry_ctor(
    PictureDecisionReorderEntry **entry_dbl_ptr,
    uint32_t                      picture_number)
{
    EB_MALLOC(PictureDecisionReorderEntry*, *entry_dbl_ptr, sizeof(PictureDecisionReorderEntry), EB_N_PTR);

    (*entry_dbl_ptr)->picture_number       = picture_number;
    (*entry_dbl_ptr)->parent_pcs_wrapper_ptr = (EbObjectWrapper *)EB_NULL;

    return EB_ErrorNone;
}
