/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vpx_codec.h"
#include "vp9_quant_common.h"
#include "vp9_onyxc_int.h"
#include "vp9_encoder.h"

#define MIN_BPB_FACTOR 0.005
#define MAX_BPB_FACTOR 50

#ifdef AGGRESSIVE_VBR
static int gf_high = 2400;
static int gf_low  = 400;
static int kf_high = 4000;
static int kf_low  = 400;
#else
static int gf_high = 2000;
static int gf_low  = 400;
static int kf_high = 4800;
static int kf_low  = 300;
#endif

#define ASSIGN_MINQ_TABLE(bit_depth, name) \
    do {                                   \
        (void)bit_depth;                   \
        name = name##_8;                   \
    } while (0)

// Tables relating active max Q to active min Q
static int kf_low_motion_minq_8[QINDEX_RANGE];
static int kf_high_motion_minq_8[QINDEX_RANGE];
static int arfgf_low_motion_minq_8[QINDEX_RANGE];
static int arfgf_high_motion_minq_8[QINDEX_RANGE];
static int inter_minq_8[QINDEX_RANGE];
static int rtc_minq_8[QINDEX_RANGE];

// Functions to compute the active minq lookup table entries based on a
// formulaic approach to facilitate easier adjustment of the Q tables.
// The formulae were derived from computing a 3rd order polynomial best
// fit to the original data (after plotting real maxq vs minq (not q index))
static int get_minq_index(double maxq, double x3, double x2, double x1, vpx_bit_depth_t bit_depth) {
    int          i;
    const double minqtarget = VPXMIN(((x3 * maxq + x2) * maxq + x1) * maxq, maxq);

    // Special case handling to deal with the step from q2.0
    // down to lossless mode represented by q 1.0.
    if (minqtarget <= 2.0)
        return 0;

    for (i = 0; i < QINDEX_RANGE; i++) {
        if (minqtarget <= eb_vp9_convert_qindex_to_q(i, bit_depth))
            return i;
    }

    return QINDEX_RANGE - 1;
}

static void init_minq_luts(int *kf_low_m, int *kf_high_m, int *arfgf_low, int *arfgf_high, int *inter, int *rtc,
                           vpx_bit_depth_t bit_depth) {
    int i;
    for (i = 0; i < QINDEX_RANGE; i++) {
        const double maxq = eb_vp9_convert_qindex_to_q(i, bit_depth);
        kf_low_m[i]       = get_minq_index(maxq, 0.000001, -0.0004, 0.150, bit_depth);
        kf_high_m[i]      = get_minq_index(maxq, 0.0000021, -0.00125, 0.45, bit_depth);
#ifdef AGGRESSIVE_VBR
        arfgf_low[i] = get_minq_index(maxq, 0.0000015, -0.0009, 0.275, bit_depth);
        inter[i]     = get_minq_index(maxq, 0.00000271, -0.00113, 0.80, bit_depth);
#else
        arfgf_low[i] = get_minq_index(maxq, 0.0000015, -0.0009, 0.30, bit_depth);
        inter[i]     = get_minq_index(maxq, 0.00000271, -0.00113, 0.70, bit_depth);
#endif
        arfgf_high[i] = get_minq_index(maxq, 0.0000021, -0.00125, 0.55, bit_depth);
        rtc[i]        = get_minq_index(maxq, 0.00000271, -0.00113, 0.70, bit_depth);
    }
}

void eb_vp9_rc_init_minq_luts(void) {
    init_minq_luts(kf_low_motion_minq_8,
                   kf_high_motion_minq_8,
                   arfgf_low_motion_minq_8,
                   arfgf_high_motion_minq_8,
                   inter_minq_8,
                   rtc_minq_8,
                   VPX_BITS_8);
}

// These functions use formulaic calculations to make playing with the
// quantizer tables easier. If necessary they can be replaced by lookup
// tables if and when things settle down in the experimental bitstream
double eb_vp9_convert_qindex_to_q(int qindex, vpx_bit_depth_t bit_depth) {
    // Convert the index to a real Q value (scaled down to match old Q values)
    return eb_vp9_ac_quant(qindex, 0, bit_depth) / 4.0;
}

int eb_vp9_convert_q_to_qindex(double q_val, vpx_bit_depth_t bit_depth) {
    int i;

    for (i = 0; i < QINDEX_RANGE; ++i)
        if (eb_vp9_convert_qindex_to_q(i, bit_depth) >= q_val)
            break;

    if (i == QINDEX_RANGE)
        i--;

    return i;
}

int eb_vp9_rc_bits_per_mb(FRAME_TYPE frame_type, int qindex, double correction_factor, vpx_bit_depth_t bit_depth) {
    const double q          = eb_vp9_convert_qindex_to_q(qindex, bit_depth);
    int          enumerator = frame_type == KEY_FRAME ? 2700000 : 1800000;

    assert(correction_factor <= MAX_BPB_FACTOR && correction_factor >= MIN_BPB_FACTOR);

    // q based adjustment to baseline enumerator
    enumerator += (int)(enumerator * q) >> 12;
    return (int)(enumerator * correction_factor / q);
}
static int get_active_quality(int q, int gfu_boost, int low, int high, int *low_motion_minq, int *high_motion_minq) {
    if (gfu_boost > high) {
        return low_motion_minq[q];
    } else if (gfu_boost < low) {
        return high_motion_minq[q];
    } else {
        const int gap        = high - low;
        const int offset     = high - gfu_boost;
        const int qdiff      = high_motion_minq[q] - low_motion_minq[q];
        const int adjustment = ((offset * qdiff) + (gap >> 1)) / gap;
        return low_motion_minq[q] + adjustment;
    }
}

int get_kf_active_quality(const RATE_CONTROL *const rc, int q, vpx_bit_depth_t bit_depth) {
    int *kf_low_motion_minq;
    int *kf_high_motion_minq;
    ASSIGN_MINQ_TABLE(bit_depth, kf_low_motion_minq);
    ASSIGN_MINQ_TABLE(bit_depth, kf_high_motion_minq);
    return get_active_quality(q, rc->kf_boost, kf_low, kf_high, kf_low_motion_minq, kf_high_motion_minq);
}

int get_gf_active_quality(struct VP9_COMP *cpi, int q, vpx_bit_depth_t bit_depth) {
    const RATE_CONTROL *const rc = &cpi->rc;

    int      *arfgf_low_motion_minq;
    int      *arfgf_high_motion_minq;
    const int gfu_boost = rc->gfu_boost;
    ASSIGN_MINQ_TABLE(bit_depth, arfgf_low_motion_minq);
    ASSIGN_MINQ_TABLE(bit_depth, arfgf_high_motion_minq);
    return get_active_quality(q, gfu_boost, gf_low, gf_high, arfgf_low_motion_minq, arfgf_high_motion_minq);
}
int eb_vp9_frame_type_qdelta(struct VP9_COMP *cpi, int rf_level, int q) {
    static const double rate_factor_deltas[RATE_FACTOR_LEVELS] = {
        1.00, // INTER_NORMAL
        1.00, // INTER_HIGH
        1.50, // GF_ARF_LOW
        1.75, // GF_ARF_STD
        2.00, // KF_STD
    };
    const VP9_COMMON *const cm = &cpi->common;

    int qdelta = eb_vp9_compute_qdelta_by_rate(
        &cpi->rc, cm->frame_type, q, rate_factor_deltas[rf_level], cm->bit_depth);
    return qdelta;
}
int eb_vp9_compute_qdelta(const RATE_CONTROL *rc, double qstart, double qtarget, vpx_bit_depth_t bit_depth) {
    int start_index  = rc->worst_quality;
    int target_index = rc->worst_quality;
    int i;

    // Convert the average q value to an index.
    for (i = rc->best_quality; i < rc->worst_quality; ++i) {
        start_index = i;
        if (eb_vp9_convert_qindex_to_q(i, bit_depth) >= qstart)
            break;
    }

    // Convert the q target to an index
    for (i = rc->best_quality; i < rc->worst_quality; ++i) {
        target_index = i;
        if (eb_vp9_convert_qindex_to_q(i, bit_depth) >= qtarget)
            break;
    }

    return target_index - start_index;
}

int eb_vp9_compute_qdelta_by_rate(const RATE_CONTROL *rc, FRAME_TYPE frame_type, int qindex, double rate_target_ratio,
                                  vpx_bit_depth_t bit_depth) {
    int target_index = rc->worst_quality;
    int i;

    // Look up the current projected bits per block for the base index
    const int base_bits_per_mb = eb_vp9_rc_bits_per_mb(frame_type, qindex, 1.0, bit_depth);

    // Find the target bits per mb based on the base value and given ratio.
    const int target_bits_per_mb = (int)(rate_target_ratio * base_bits_per_mb);

    // Convert the q target to an index
    for (i = rc->best_quality; i < rc->worst_quality; ++i) {
        if (eb_vp9_rc_bits_per_mb(frame_type, i, 1.0, bit_depth) <= target_bits_per_mb) {
            target_index = i;
            break;
        }
    }
    return target_index - qindex;
}
