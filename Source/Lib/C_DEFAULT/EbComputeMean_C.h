/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbComputeMean_C_h
#define EbComputeMean_C_h
#ifdef __cplusplus
extern "C" {
#endif

#include "EbDefinitions.h"

uint64_t compute_mean(
    uint8_t   *input_samples,       // input parameter, input samples Ptr
    uint32_t   input_stride,        // input parameter, input stride
    uint32_t   input_area_width,    // input parameter, input area width
    uint32_t   input_area_height);  // input parameter, input area height

uint64_t compute_mean_of_squared_values(
    uint8_t   *input_samples,       // input parameter, input samples Ptr
    uint32_t   input_stride,        // input parameter, input stride
    uint32_t   input_area_width,    // input parameter, input area width
    uint32_t   input_area_height);  // input parameter, input area height

#ifdef __cplusplus
}
#endif

#endif
