/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPacketizationReorderQueue.h"

EbErrorType eb_vp9_packetization_reorder_entry_ctor(
    PacketizationReorderEntry **entry_dbl_ptr,
    uint32_t                    picture_number)
{
    EB_MALLOC(PacketizationReorderEntry*, *entry_dbl_ptr, sizeof(PacketizationReorderEntry), EB_N_PTR);

    (*entry_dbl_ptr)->picture_number                     = picture_number;
    (*entry_dbl_ptr)->output_stream_wrapper_ptr          = (EbObjectWrapper *)EB_NULL;

    return EB_ErrorNone;
}
