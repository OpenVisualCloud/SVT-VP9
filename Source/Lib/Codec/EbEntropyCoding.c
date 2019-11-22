/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <stdint.h>

#include "EbEntropyCoding.h"
#include "EbDefinitions.h"
#include "EbThreads.h"
#include "EbEntropyCodingObject.h"

#define DECODED_PICTURE_HASH 132

EbErrorType eb_vp9_reset_bitstream(
    EbPtr bitstream_ptr)
{
    EbErrorType return_error = EB_ErrorNone;
    OutputBitstreamUnit *output_bitstream_ptr = (OutputBitstreamUnit*)bitstream_ptr;

    eb_vp9_output_bitstream_reset(
        output_bitstream_ptr);

    return return_error;
}

EbErrorType eb_vp9_bitstream_ctor(
    Bitstream **bitstream_dbl_ptr,
    uint32_t    buffer_size)
{
    EbErrorType return_error = EB_ErrorNone;
    EB_MALLOC(Bitstream*, *bitstream_dbl_ptr, sizeof(Bitstream), EB_N_PTR);

    EB_MALLOC(EbPtr, (*bitstream_dbl_ptr)->output_bitstream_ptr, sizeof(OutputBitstreamUnit), EB_N_PTR);

    return_error = eb_vp9_output_bitstream_unit_ctor(
        (OutputBitstreamUnit*)(*bitstream_dbl_ptr)->output_bitstream_ptr,
        buffer_size);

    return return_error;
}

EbErrorType eb_vp9_entropy_coder_ctor(
    EntropyCoder **entropy_coder_dbl_ptr,
    uint32_t       buffer_size)
{
    EbErrorType return_error = EB_ErrorNone;
    EB_MALLOC(EntropyCoder*, *entropy_coder_dbl_ptr, sizeof(EntropyCoder), EB_N_PTR);

    EB_MALLOC(EbPtr, (*entropy_coder_dbl_ptr)->ec_output_bitstream_ptr, sizeof(OutputBitstreamUnit  ), EB_N_PTR);

    return_error = eb_vp9_output_bitstream_unit_ctor(
        (OutputBitstreamUnit*)(*entropy_coder_dbl_ptr)->ec_output_bitstream_ptr,
        buffer_size);

    return return_error;
}
