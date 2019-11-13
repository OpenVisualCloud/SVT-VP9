/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VP9_ENCODER_VP9_ENCODEMV_H_
#define VPX_VP9_ENCODER_VP9_ENCODEMV_H_
#if 1
#define INLINE __inline

#include <stdint.h>
#include "vp9_encoder.h"
#else
#include "vp9/encoder/vp9_encoder.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

void eb_vp9_entropy_mv_init(void);

void eb_vp9_write_nmv_probs(VP9_COMMON *cm, int usehp, VpxWriter *w,
                         nmv_context_counts *const counts);

void eb_vp9_encode_mv(VP9_COMP *cpi, VpxWriter *w, const MV *mv, const MV *ref,
                   const nmv_context *mvctx, int usehp,
                   unsigned int *const max_mv_magnitude);

void eb_vp9_build_nmv_cost_table(int *mvjoint, int *mvcost[2],
                              const nmv_context *mvctx, int usehp);

void vp9_update_mv_count(ThreadData *td);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VPX_VP9_ENCODER_VP9_ENCODEMV_H_
