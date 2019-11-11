/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEntropyCodingObject_h
#define EbEntropyCodingObject_h

#include <stdint.h>
#include "EbDefinitions.h"
#include "bitwriter.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct Bitstream {
    EbPtr output_bitstream_ptr;
} Bitstream;

typedef struct EntropyCoder
{
    VpxWriter residual_bc;
    EbPtr     ec_output_bitstream_ptr;

} EntropyCoder;

extern EbErrorType eb_vp9_bitstream_ctor(
    Bitstream **bitstream_dbl_ptr,
    uint32_t    buffer_size);

extern EbErrorType eb_vp9_entropy_coder_ctor(
    EntropyCoder **entropy_coder_dbl_ptr,
    uint32_t       buffer_size);

#ifdef __cplusplus
}
#endif
#endif // EbEntropyCodingObject_h
