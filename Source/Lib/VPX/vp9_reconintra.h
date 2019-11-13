/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VP9_COMMON_VP9_RECONINTRA_H_
#define VPX_VP9_COMMON_VP9_RECONINTRA_H_

#define INLINE __inline

#include <stdint.h>
#include "vp9_blockd.h"
#include "EbSequenceControlSet.h"
#include "EbEncDecProcess.h"

#ifdef __cplusplus
extern "C" {
#endif

void eb_vp9_init_intra_predictors(void);

void eb_vp9_predict_intra_block(EncDecContext   *context_ptr,
#if 0 // Hsan: reference samples generation done per block prior to fast loop @ generate_intra_reference_samples()
                             const MACROBLOCKD *xd,
#endif
                             TX_SIZE tx_size,
                             PREDICTION_MODE mode,
#if 0 // Hsan: reference samples generation done per block prior to fast loop @ generate_intra_reference_samples()
                             const uint8_t *ref,
                             int ref_stride,
#endif
                             uint8_t *dst, int dst_stride,
#if 0
                             int aoff, int loff,
#endif
                             int plane);
#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VPX_VP9_COMMON_VP9_RECONINTRA_H_
