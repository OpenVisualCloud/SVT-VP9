/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbBitstreamUnit.h"

/**********************************
 * Constructor
 **********************************/
EbErrorType eb_vp9_output_bitstream_unit_ctor(OutputBitstreamUnit *bitstream_ptr, uint32_t buffer_size) {
    uint32_t slice_index;

    if (buffer_size) {
        bitstream_ptr->size = buffer_size / sizeof(unsigned int);
        EB_MALLOC(unsigned char *, bitstream_ptr->buffer_begin, sizeof(unsigned char) * bitstream_ptr->size, EB_N_PTR);
        bitstream_ptr->buffer = bitstream_ptr->buffer_begin;
    } else {
        bitstream_ptr->size         = 0;
        bitstream_ptr->buffer_begin = 0;
        bitstream_ptr->buffer       = 0;
    }

    bitstream_ptr->valid_bits_count   = 32;
    bitstream_ptr->byte_holder        = 0;
    bitstream_ptr->written_bits_count = 0;
    bitstream_ptr->slice_num          = 0;

    for (slice_index = 0; slice_index < SLICE_HEADER_COUNT; ++slice_index) {
        bitstream_ptr->slice_location[slice_index] = 0;
    }

    return EB_ErrorNone;
}

/**********************************
 * Reset Bitstream
 **********************************/
EbErrorType eb_vp9_output_bitstream_reset(OutputBitstreamUnit *bitstream_ptr) {
    EbErrorType return_error = EB_ErrorNone;

    uint32_t slice_index;

    bitstream_ptr->valid_bits_count   = 32;
    bitstream_ptr->byte_holder        = 0;
    bitstream_ptr->written_bits_count = 0;
    bitstream_ptr->buffer             = bitstream_ptr->buffer_begin;
    bitstream_ptr->slice_num          = 0;

    for (slice_index = 0; slice_index < SLICE_HEADER_COUNT; ++slice_index) {
        bitstream_ptr->slice_location[slice_index] = 0;
    }

    return return_error;
}
