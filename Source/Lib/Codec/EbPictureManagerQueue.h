/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD    - 2 - Clause - Patent
*/

#ifndef EbPictureManagerQueue_h
#define EbPictureManagerQueue_h

#include "EbDefinitions.h"
#include "EbSei.h"
#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPredictionStructure.h"
#include "EbApiSei.h"

#ifdef __cplusplus
extern "C" {
#endif
/************************************************
 * Input Queue Entry
 ************************************************/
struct ReferenceQueueEntry;   //    empty struct definition

typedef struct InputQueueEntry
{
    EbObjectWrapper *input_object_ptr;
    uint32_t         dependent_count;
    uint32_t         reference_entry_index;
    ReferenceList   *list0_ptr;
    ReferenceList   *list1_ptr;

} InputQueueEntry;

/************************************************
 * Reference Queue Entry
 ************************************************/
typedef struct ReferenceQueueEntry
{

    uint64_t         picture_number;
    uint64_t         decode_order;
    EbObjectWrapper *reference_object_ptr;
    uint32_t         dependent_count;
    EB_BOOL          release_enable;
    EB_BOOL          reference_available;
    uint32_t         dep_list0_count;
    uint32_t         dep_list1_count;
    DependentList    list0;
    DependentList    list1;
    EB_BOOL          is_used_as_reference_flag;
    EB_BOOL          feedback_arrived;
} ReferenceQueueEntry;

/************************************************
 * Rate Control Input Queue Entry
 ************************************************/

typedef struct RcInputQueueEntry
{
    uint64_t         picture_number;
    EbObjectWrapper *input_object_ptr;
    EB_BOOL          release_enabled;
    uint32_t         gop_index;

} RcInputQueueEntry;

/************************************************
 * Rate Control FeedBack  Queue Entry
 ************************************************/
typedef struct RcFeedbackQueueEntry
{
    uint64_t picture_number;
    EB_BOOL  release_enabled;
    uint32_t gop_index;

} RcFeedbackQueueEntry;

extern EbErrorType eb_vp9_input_queue_entry_ctor(
    InputQueueEntry **entry_dbl_ptr);

extern EbErrorType eb_vp9_reference_queue_entry_ctor(
    ReferenceQueueEntry **entry_dbl_ptr);

#ifdef __cplusplus
}
#endif
#endif // EbPictureManagerQueue_h
