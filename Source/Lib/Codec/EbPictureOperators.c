/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/*********************************
 * Includes
 *********************************/

#include "EbPictureOperators.h"
#include <stdint.h>

/*******************************************
 * eb_vp9_memset16bit
 *******************************************/
void eb_vp9_memset16bit(uint16_t *in_ptr, uint16_t value, uint64_t num_of_elements) {
    uint64_t i;

    for (i = 0; i < num_of_elements; i++) { in_ptr[i] = value; }
}
