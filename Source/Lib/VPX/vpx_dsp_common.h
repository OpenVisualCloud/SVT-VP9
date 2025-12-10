/*
 *  Copyright (c) 2015 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VPX_DSP_VPX_DSP_COMMON_H_
#define VPX_VPX_DSP_VPX_DSP_COMMON_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VPXMIN(x, y) (((x) < (y)) ? (x) : (y))
#define VPXMAX(x, y) (((x) > (y)) ? (x) : (y))

#define VPX_SWAP(type, a, b) \
    do {                     \
        type c = (b);        \
        b      = a;          \
        a      = c;          \
    } while (0)

// Note:
// tran_low_t  is the datatype used for final transform coefficients.
// tran_high_t is the datatype used for intermediate transform stages.
typedef int32_t tran_high_t;
typedef int16_t tran_low_t;

typedef int16_t tran_coef_t;

static inline int clip_pixel(int val) { return (uint8_t)((val > 255) ? 255 : (val < 0) ? 0 : val); }

static inline int clamp(int value, int low, int high) { return value < low ? low : (value > high ? high : value); }

static inline double fclamp(double value, double low, double high) {
    return value < low ? low : (value > high ? high : value);
}
#ifdef __cplusplus
} // extern "C"
#endif

#endif // VPX_VPX_DSP_VPX_DSP_COMMON_H_
