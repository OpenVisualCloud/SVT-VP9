/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbPictureAnalysisResults.h"

EbErrorType eb_vp9_picture_analysis_result_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    PictureAnalysisResults *object_ptr;
    EB_MALLOC(PictureAnalysisResults*, object_ptr, sizeof(PictureAnalysisResults), EB_N_PTR);

    *object_dbl_ptr = (EbPtr) object_ptr;
    object_init_data_ptr = 0;
    (void) object_init_data_ptr;

    return EB_ErrorNone;
}
