/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VP9_COMMON_VP9_IDCT_H_
#define VPX_VP9_COMMON_VP9_IDCT_H_

#include <assert.h>

#include "vp9_common.h"
#include "vp9_enums.h"
#include "inv_txfm.h"
#include "txfm_common.h"
#include "mem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*transform_1d)(const tran_low_t *, tran_low_t *);

typedef struct {
    transform_1d cols, rows; // vertical and horizontal
} transform_2d;

void eb_vp9_idct4x4_add(const tran_low_t *input, uint8_t *dest, int stride, int eob);
void eb_vp9_idct32x32_add(const tran_low_t *input, uint8_t *dest, int stride, int eob);

void eb_vp9_iht8x8_add(TX_TYPE tx_type, const tran_low_t *input, uint8_t *dest, int stride, int eob);
void eb_vp9_iht16x16_add(TX_TYPE tx_type, const tran_low_t *input, uint8_t *dest, int stride, int eob);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VPX_VP9_COMMON_VP9_IDCT_H_
