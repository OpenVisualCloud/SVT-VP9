/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbComputeMean_C.h"
#include "stdint.h"

#define VARIANCE_PRECISION      16

/*******************************************
* compute_mean
*   returns the mean of a block
*******************************************/
uint64_t compute_mean(
    uint8_t  *input_samples,      // input parameter, input samples Ptr
    uint32_t  input_stride,       // input parameter, input stride
    uint32_t  input_area_width,    // input parameter, input area width
    uint32_t  input_area_height)   // input parameter, input area height
{

    uint32_t horizontal_index;
    uint32_t vertical_index;
    uint64_t block_mean = 0;

    for (vertical_index = 0; vertical_index < input_area_height; vertical_index++) {
        for (horizontal_index = 0; horizontal_index < input_area_width; horizontal_index++) {

            block_mean += input_samples[horizontal_index];

        }
        input_samples += input_stride;
    }

    block_mean = (block_mean << (VARIANCE_PRECISION >> 1)) / (input_area_width * input_area_height);

    return block_mean;
}

/*******************************************
* compute_mean_of_squared_values
*   returns the Mean of Squared Values
*******************************************/
uint64_t compute_mean_of_squared_values(
    uint8_t  *input_samples,      // input parameter, input samples Ptr
    uint32_t  input_stride,       // input parameter, input stride
    uint32_t  input_area_width,    // input parameter, input area width
    uint32_t  input_area_height)   // input parameter, input area height
{

    uint32_t horizontal_index;
    uint32_t vertical_index;
    uint64_t block_mean = 0;

    for (vertical_index = 0; vertical_index < input_area_height; vertical_index++) {
        for (horizontal_index = 0; horizontal_index < input_area_width; horizontal_index++) {

            block_mean += input_samples[horizontal_index] * input_samples[horizontal_index];;

        }
        input_samples += input_stride;
    }

    block_mean = (block_mean << VARIANCE_PRECISION) / (input_area_width * input_area_height);

    return block_mean;
}

uint64_t eb_vp9_ComputeSubMeanOfSquaredValues(
    uint8_t  *input_samples,      // input parameter, input samples Ptr
    uint32_t  input_stride,       // input parameter, input stride
    uint32_t  input_area_width,    // input parameter, input area width
    uint32_t  input_area_height)   // input parameter, input area height
{

    uint32_t horizontal_index;
    uint32_t vertical_index = 0;
    uint64_t block_mean = 0;
    uint16_t skip = 0;

    for (vertical_index = 0; skip < input_area_height; skip =vertical_index + vertical_index) {
        for (horizontal_index = 0; horizontal_index < input_area_width; horizontal_index++) {

            block_mean += input_samples[horizontal_index] * input_samples[horizontal_index];

        }
        input_samples += 2*input_stride;
        vertical_index++;
    }

    block_mean = block_mean << 11; //VARIANCE_PRECISION) / (input_area_width * input_area_height);

    return block_mean;
}

uint64_t eb_vp9_ComputeSubMean8x8(
    uint8_t  *input_samples,      // input parameter, input samples Ptr
    uint32_t  input_stride,       // input parameter, input stride
    uint32_t  input_area_width,    // input parameter, input area width
    uint32_t  input_area_height)   // input parameter, input area height
{

    uint32_t horizontal_index;
    uint32_t vertical_index = 0;
    uint64_t block_mean = 0;
    uint16_t skip = 0;

    for (vertical_index = 0; skip < input_area_height; skip =vertical_index + vertical_index) {
        for (horizontal_index = 0; horizontal_index < input_area_width; horizontal_index++) {

            block_mean += input_samples[horizontal_index] * input_samples[horizontal_index];

        }
        input_samples += 2*input_stride;
        vertical_index++;
    }

    block_mean = block_mean << 11; //VARIANCE_PRECISION) / (input_area_width * input_area_height);

    return block_mean;
}
