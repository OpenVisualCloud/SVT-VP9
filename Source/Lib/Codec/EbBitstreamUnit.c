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
EbErrorType eb_vp9_output_bitstream_unit_ctor(
    OutputBitstreamUnit *bitstream_ptr,
    uint32_t             buffer_size )
{

    uint32_t slice_index;

    if(buffer_size) {
        bitstream_ptr->size             = buffer_size / sizeof(unsigned int);
        EB_MALLOC(unsigned char*, bitstream_ptr->buffer_begin, sizeof(unsigned char) * bitstream_ptr->size, EB_N_PTR);
        bitstream_ptr->buffer           = bitstream_ptr->buffer_begin;
    }
    else {
        bitstream_ptr->size             = 0;
        bitstream_ptr->buffer_begin      = 0;
        bitstream_ptr->buffer           = 0;
    }

    bitstream_ptr->valid_bits_count   = 32;
    bitstream_ptr->byte_holder       = 0;
    bitstream_ptr->written_bits_count = 0;
    bitstream_ptr->slice_num         = 0;

    for(slice_index=0; slice_index < SLICE_HEADER_COUNT; ++slice_index) {
        bitstream_ptr->slice_location[slice_index] = 0;
    }

    return EB_ErrorNone;
}

/**********************************
 * Reset Bitstream
 **********************************/
EbErrorType eb_vp9_output_bitstream_reset(
    OutputBitstreamUnit *bitstream_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    uint32_t slice_index;

    bitstream_ptr->valid_bits_count   = 32;
    bitstream_ptr->byte_holder        = 0;
    bitstream_ptr->written_bits_count = 0;
    bitstream_ptr->buffer             = bitstream_ptr->buffer_begin;
    bitstream_ptr->slice_num          = 0;

    for(slice_index=0; slice_index < SLICE_HEADER_COUNT; ++slice_index) {
        bitstream_ptr->slice_location[slice_index] = 0;
    }

    return return_error;
}

/**********************************
 * Write to bitstream
 *   Intended to be used in CABAC
 **********************************/
EbErrorType output_bitstream_write (
    OutputBitstreamUnit *bitstream_ptr,
    uint32_t             bits,
    uint32_t             number_of_bits)
{
    EbErrorType return_error = EB_ErrorNone;
    uint32_t shift_count;

    bitstream_ptr->written_bits_count += number_of_bits;

    // If number of bits is less than Valid bits, one word
    if( (int32_t)number_of_bits < bitstream_ptr->valid_bits_count) {
        bitstream_ptr->valid_bits_count -= number_of_bits;
        bitstream_ptr->byte_holder     |= bits << bitstream_ptr->valid_bits_count;
    } else {
        shift_count = number_of_bits - bitstream_ptr->valid_bits_count;

        // add the last bits
        bitstream_ptr->byte_holder |= bits >> shift_count;
        *bitstream_ptr->buffer++   = (uint8_t)eb_vp9_endian_swap( bitstream_ptr->byte_holder );

        // note: there is a problem with left shift with 32
        bitstream_ptr->valid_bits_count = 32 - shift_count;
        bitstream_ptr->byte_holder  = ( 0 == shift_count ) ?
                                    0 :
                                    (bits << bitstream_ptr->valid_bits_count);

    }

    return return_error;
}

EbErrorType output_bitstream_write_byte(
    OutputBitstreamUnit *bitstream_ptr,
    uint32_t             bits)
{
    EbErrorType return_error = EB_ErrorNone;

    bitstream_ptr->written_bits_count += 8;

    if (8 < bitstream_ptr->valid_bits_count)
    {
        bitstream_ptr->valid_bits_count -= 8;
        bitstream_ptr->byte_holder |= bits << bitstream_ptr->valid_bits_count;
    }
    else
    {
        *bitstream_ptr->buffer++ = (uint8_t)eb_vp9_endian_swap( bitstream_ptr->byte_holder | bits );

        bitstream_ptr->valid_bits_count = 32;
        bitstream_ptr->byte_holder  = 0;
    }

    return return_error;
}

/**********************************
 * Write allign zero to bitstream
 *   Intended to be used in CABAC
 **********************************/
EbErrorType output_bitstream_write_align_zero(OutputBitstreamUnit *bitstream_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    output_bitstream_write(
        bitstream_ptr,
        0,
        bitstream_ptr->valid_bits_count & 0x7 );

    return return_error;
}

/**********************************
 * Output RBSP to payload
 *   Intended to be used in CABAC
 **********************************/
EbErrorType eb_vp9_output_bitstream_rbsp_to_payload(
    OutputBitstreamUnit *bitstream_ptr,
    EbByte               output_buffer,
    uint32_t            *output_buffer_index,
    uint32_t            *output_buffer_size,
    uint32_t             start_location)
{

    EbErrorType return_error          = EB_ErrorNone;
    uint32_t  buffer_written_bytes_count = (uint32_t)(bitstream_ptr->buffer - bitstream_ptr->buffer_begin);
    uint32_t  write_location             = start_location;
    uint32_t  read_location              = start_location;

    EbByte read_byte_ptr;
    EbByte write_byte_ptr;

    read_byte_ptr  = (EbByte) bitstream_ptr->buffer_begin;
    write_byte_ptr = &output_buffer[*output_buffer_index];

    while ((read_location < buffer_written_bytes_count)) {

        if ((*output_buffer_index) < (*output_buffer_size)) {
            write_byte_ptr[write_location++] = read_byte_ptr[read_location];
            *output_buffer_index += 1;
        }
        read_location++;
    }

    return return_error;
}
