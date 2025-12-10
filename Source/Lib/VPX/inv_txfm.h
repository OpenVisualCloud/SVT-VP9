/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VPX_DSP_INV_TXFM_H_
#define VPX_VPX_DSP_INV_TXFM_H_

#include <assert.h>

#include "txfm_common.h"
#include "mem.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline tran_high_t check_range(tran_high_t input) {
#if CONFIG_COEFFICIENT_RANGE_CHECKING
    // For valid VP9 input streams, intermediate stage coefficients should always
    // stay within the range of a signed 16 bit integer. Coefficients can go out
    // of this range for invalid/corrupt VP9 streams. However, strictly checking
    // this range for every intermediate coefficient can burdensome for a decoder,
    // therefore the following assertion is only enabled when configured with
    // --enable-coefficient-range-checking.
    assert(INT16_MIN <= input);
    assert(input <= INT16_MAX);
#endif // CONFIG_COEFFICIENT_RANGE_CHECKING
    return input;
}

static inline tran_high_t dct_const_round_shift(tran_high_t input) {
    tran_high_t rv = ROUND_POWER_OF_TWO(input, DCT_CONST_BITS);
    return (tran_high_t)rv;
}

#if CONFIG_EMULATE_HARDWARE
// When CONFIG_EMULATE_HARDWARE is 1 the transform performs a
// non-normative method to handle overflows. A stream that causes
// overflows  in the inverse transform is considered invalid in VP9,
// and a hardware implementer is free to choose any reasonable
// method to handle overflows. However to aid in hardware
// verification they can use a specific implementation of the
// WRAPLOW() macro below that is identical to their intended
// hardware implementation (and also use configure options to trigger
// the C-implementation of the transform).
//
// The particular WRAPLOW implementation below performs strict
// overflow wrapping to match common hardware implementations.
// bd of 8 uses trans_low with 16bits, need to remove 16bits
// bd of 10 uses trans_low with 18bits, need to remove 14bits
// bd of 12 uses trans_low with 20bits, need to remove 12bits
// bd of x uses trans_low with 8+x bits, need to remove 24-x bits
#define WRAPLOW(x) ((((int32_t)check_range(x)) << 16) >> 16)

#else // CONFIG_EMULATE_HARDWARE

#define WRAPLOW(x) ((int32_t)check_range(x))
#endif // CONFIG_EMULATE_HARDWARE

void eb_vp9_idct4_c(const tran_low_t *input, tran_low_t *output);
void eb_vp9_idct8_c(const tran_low_t *input, tran_low_t *output);
void eb_vp9_idct16_c(const tran_low_t *input, tran_low_t *output);
void eb_vp9_iadst4_c(const tran_low_t *input, tran_low_t *output);
void eb_vp9_iadst8_c(const tran_low_t *input, tran_low_t *output);
void eb_vp9_iadst16_c(const tran_low_t *input, tran_low_t *output);

static inline uint8_t clip_pixel_add(uint8_t dest, tran_high_t trans) {
    trans = WRAPLOW(trans);
    return (uint8_t)clip_pixel(dest + (int)trans);
}
#ifdef __cplusplus
} // extern "C"
#endif

#endif // VPX_VPX_DSP_INV_TXFM_H_
