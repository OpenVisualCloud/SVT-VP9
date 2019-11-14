/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureDecisionQueue_h
#define EbPictureDecisionQueue_h

#include "EbDefinitions.h"
#include "EbSei.h"
#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPredictionStructure.h"
#include "EbApiSei.h"
#include"EbPictureControlSet.h"
/************************************************
 * PA Reference Queue Entry
 ************************************************/
typedef struct PaReferenceQueueEntry
{
    EbObjectWrapper         *input_object_ptr;
    uint64_t                 picture_number;
    uint32_t                 dependent_count;
    uint32_t                 reference_entry_index;
    ReferenceList           *list0_ptr;
    ReferenceList           *list1_ptr;
    uint32_t                 dep_list0_count;
    uint32_t                 dep_list1_count;
    DependentList            list0;
    DependentList            list1;
    PictureParentControlSet *p_pcs_ptr;

} PaReferenceQueueEntry;

extern EbErrorType eb_vp9_pa_eb_vp9_reference_queue_entry_ctor(
    PaReferenceQueueEntry  **entry_dbl_ptr);

#endif // EbPictureDecisionQueue_h
