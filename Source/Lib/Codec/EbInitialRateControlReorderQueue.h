/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbInitialRateControlReorderQueue_h
#define EbInitialRateControlReorderQueue_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbRateControlTables.h"
#include "EbPictureControlSet.h"

/************************************************
 * Initial Rate Control Reorder Queue Entry
 ************************************************/
typedef struct InitialRateControlReorderEntry
{
    uint64_t         picture_number;
    EbObjectWrapper *parent_pcs_wrapper_ptr;

} InitialRateControlReorderEntry;

extern EbErrorType eb_vp9_initial_rate_control_reorder_entry_ctor(
    InitialRateControlReorderEntry **entry_dbl_ptr,
    uint32_t                         picture_number);

/************************************************
 * High Level Rate Control Histogram Queue Entry
 ************************************************/
typedef struct HlRateControlHistogramEntry
{
    uint64_t         picture_number;
    int16_t          life_count;
    EB_BOOL          passed_to_hlrc;
    EB_BOOL          is_coded;
    uint64_t         total_num_bitsCoded;
    EbObjectWrapper *parent_pcs_wrapper_ptr;
    EB_BOOL          end_of_sequence_flag;
    uint64_t         pred_bits_ref_qp[MAX_REF_QP_NUM];
    EB_SLICE         slice_type;
    uint32_t         temporal_layer_index;
    uint32_t         frames_in_sw;

    // Motion Estimation Distortion and OIS Historgram
    uint16_t        *me_distortion_histogram;
    uint16_t        *ois_distortion_histogram;
    uint32_t         full_sb_count;

} HlRateControlHistogramEntry;

extern EbErrorType eb_vp9_hl_rate_control_histogram_entry_ctor(
    HlRateControlHistogramEntry **entry_dbl_ptr,
    uint32_t                      picture_number);

#endif //EbInitialRateControlReorderQueue_h
