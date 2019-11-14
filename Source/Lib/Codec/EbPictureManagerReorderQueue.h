/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef  EbPictureManagerReorderQueue_h
#define  EbPictureManagerReorderQueue_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"

/************************************************
         * Packetization Reorder Queue Entry
         ************************************************/
typedef struct PictureManagerReorderEntry
{
    uint64_t         picture_number;
    EbObjectWrapper *parent_pcs_wrapper_ptr;
} PictureManagerReorderEntry;

extern EbErrorType eb_vp9_picture_manager_reorder_entry_ctor(
    PictureManagerReorderEntry **entry_dbl_ptr,
    uint32_t                     picture_number);

#endif   //EbPictureManagerReorderQueue_h
