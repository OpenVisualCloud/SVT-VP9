/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VP9_ENCODER_VP9_RATECTRL_H_
#define VPX_VP9_ENCODER_VP9_RATECTRL_H_

#include "vpx_codec.h"
#include "vp9_blockd.h"
#include "vp9_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif
// Threshold used to define a KF group as static (e.g. a slide show).
// Essentially this means that no frame in the group has more than 1% of MBs
// that are not marked as coded with 0,0 motion in the first pass.
#define STATIC_KF_GROUP_THRESH 99

// The maximum duration of a GF group that is static (for example a slide show).
#define MAX_STATIC_GF_GROUP_LENGTH 250

typedef enum {
    INTER_NORMAL       = 0,
    INTER_HIGH         = 1,
    GF_ARF_LOW         = 2,
    GF_ARF_STD         = 3,
    KF_STD             = 4,
    RATE_FACTOR_LEVELS = 5
} RATE_FACTOR_LEVEL;

typedef struct {
    int gfu_boost;
    int kf_boost;
    int worst_quality;
    int best_quality;
} RATE_CONTROL;
double eb_vp9_convert_qindex_to_q(int qindex, vpx_bit_depth_t bit_depth);
void   eb_vp9_rc_init_minq_luts(void);
// Computes a q delta (in "q index" terms) to get from a starting q value
// to a target q value
int eb_vp9_compute_qdelta(const RATE_CONTROL *rc, double qstart, double qtarget, vpx_bit_depth_t bit_depth);

// Computes a q delta (in "q index" terms) to get from a starting q value
// to a value that should equate to the given rate ratio.
int eb_vp9_compute_qdelta_by_rate(const RATE_CONTROL *rc, FRAME_TYPE frame_type, int qindex, double rate_target_ratio,
                                  vpx_bit_depth_t bit_depth);
int eb_vp9_frame_type_qdelta(struct VP9_COMP *cpi, int rf_level, int q);

int get_kf_active_quality(const RATE_CONTROL *const rc, int q, vpx_bit_depth_t bit_depth);

int get_gf_active_quality(struct VP9_COMP *cpi, int q, vpx_bit_depth_t bit_depth);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VPX_VP9_ENCODER_VP9_RATECTRL_H_
