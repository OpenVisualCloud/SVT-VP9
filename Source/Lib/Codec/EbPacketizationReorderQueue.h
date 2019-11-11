/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPacketizationReorderQueue_h
#define EbPacketizationReorderQueue_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPredictionStructure.h"

#include "vp9_blockd.h"
#ifdef __cplusplus
extern "C" {
#endif
/************************************************
 * Packetization Reorder Queue Entry
 ************************************************/
typedef struct PacketizationReorderEntry
{
    uint64_t          picture_number;
    EbObjectWrapper  *output_stream_wrapper_ptr;

    uint64_t          start_time_seconds;
    uint64_t          start_timeu_seconds;

    FRAME_TYPE        frame_type;
    uint8_t           intra_only;
    uint64_t          poc;
    uint64_t          total_num_bits;

    uint8_t           slice_type;
    uint64_t          ref_poc_list0;
    uint64_t          ref_poc_list1;
    RpsNode           ref_signal;
    EB_BOOL           show_frame;
    int               show_existing_frame;
    uint8_t           show_existing_frame_index_array[4];
    uint64_t          actual_bits;
} PacketizationReorderEntry;

extern EbErrorType eb_vp9_packetization_reorder_entry_ctor(
    PacketizationReorderEntry **entry_dbl_ptr,
    uint32_t                    picture_number);

#ifdef __cplusplus
}
#endif
#endif //EbPacketizationReorderQueue_h
