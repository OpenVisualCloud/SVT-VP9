/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VP9_ENCODER_VP9_RDOPT_H_
#define VPX_VP9_ENCODER_VP9_RDOPT_H_

#include "vp9_blockd.h"

#include "vp9_block.h"
#include "vp9_context_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TileInfo;
struct VP9_COMP;
struct macroblock;
struct RD_COST;

extern void estimate_ref_frame_costs(const VP9_COMMON *cm, const MACROBLOCKD *xd, int segment_id,
                                     unsigned int *ref_costs_single, unsigned int *ref_costs_comp,
                                     vpx_prob *comp_mode_p);

int cost_mv_ref(const VP9_COMP *cpi, PREDICTION_MODE mode, int mode_context);

int cost_coeffs(MACROBLOCK *x, int plane, int block, TX_SIZE tx_size, int pt, const int16_t *scan, const int16_t *nb,
                int use_fast_coef_costing);
#ifdef __cplusplus
} // extern "C"
#endif

#endif // VPX_VP9_ENCODER_VP9_RDOPT_H_
