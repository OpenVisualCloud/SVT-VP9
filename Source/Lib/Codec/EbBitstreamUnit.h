/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbBitstreamUnit_h
#define EbBitstreamUnit_h

#include "EbDefinitions.h"
#include "EbUtility.h"
#ifdef __cplusplus
extern "C" {
#endif
// Bistream Slice Buffer Size
#define EB_BITSTREAM_SLICE_BUFFER_SIZE          0x300000
#define SLICE_HEADER_COUNT                         256

/**********************************
 * Bitstream Unit Types
 **********************************/
typedef struct OutputBitstreamUnit
{
    uint32_t size;                                 // allocated buffer size
    uint32_t byte_holder;                          // holds bytes and partial bytes
    int32_t  valid_bits_count;                     // count of valid bits in byte_holder
    uint32_t written_bits_count;                   // count of written bits
    uint32_t slice_num;                            // Number of slices
    uint32_t slice_location[SLICE_HEADER_COUNT];   // Location of each slice in byte
    uint8_t *buffer_begin;                         // the byte buffer
    uint8_t *buffer;                               // the byte buffe

} OutputBitstreamUnit;

/**********************************
 * Extern Function Declarations
 **********************************/
extern EbErrorType eb_vp9_output_bitstream_unit_ctor(
    OutputBitstreamUnit *bitstream_ptr,
    uint32_t             buffer_size );

extern EbErrorType eb_vp9_output_bitstream_reset(OutputBitstreamUnit *bitstream_ptr);

extern EbErrorType output_bitstream_write (
    OutputBitstreamUnit *bitstream_ptr,
    uint32_t             bits,
    uint32_t             number_of_bits );

extern EbErrorType output_bitstream_write_byte(
    OutputBitstreamUnit *bitstream_ptr,
    uint32_t             bits);

extern EbErrorType output_bitstream_write_align_zero(OutputBitstreamUnit *bitstream_ptr);

extern EbErrorType eb_vp9_output_bitstream_rbsp_to_payload(
    OutputBitstreamUnit *bitstream_ptr,
    EbByte               output_buffer,
    uint32_t            *output_buffer_index,
    uint32_t            *output_buffer_size,
    uint32_t             start_location);

#ifdef __cplusplus
}
#endif

#endif // EbBitstreamUnit_h
