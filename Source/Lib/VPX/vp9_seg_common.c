/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>

#define INLINE __inline

#include <stdint.h>
#include "vp9_seg_common.h"
#include "vp9_loopfilter.h"
#include "prob.h"
#include "string.h"
#include "vp9_common.h"
#include "vp9_quant_common.h"

static const int seg_feature_data_signed[SEG_LVL_MAX] = { 1, 1, 0, 0 };
#if 1// SEG_SUPPORT
static const int seg_feature_data_max[SEG_LVL_MAX] = { MAXQ, MAX_LOOP_FILTER, 3,
                                                       0 };
#endif
// These functions provide access to new segment level features.
// Eventually these function may be "optimized out" but for the moment,
// the coding mechanism is still subject to change so these provide a
// convenient single point of change.

void eb_vp9_clearall_segfeatures(struct segmentation *seg) {
  vp9_zero(seg->feature_data);
  vp9_zero(seg->feature_mask);
  seg->aq_av_offset = 0;
}

void eb_vp9_enable_segfeature(struct segmentation *seg, int segment_id,
                           SEG_LVL_FEATURES feature_id) {
  seg->feature_mask[segment_id] |= 1 << feature_id;
}
#if 1// SEG_SUPPORT
int eb_vp9_seg_feature_data_max(SEG_LVL_FEATURES feature_id) {
  return seg_feature_data_max[feature_id];
}
#endif
int eb_vp9_is_segfeature_signed(SEG_LVL_FEATURES feature_id) {
  return seg_feature_data_signed[feature_id];
}
#if 1// SEG_SUPPORT
void eb_vp9_set_segdata(struct segmentation *seg, int segment_id,
                     SEG_LVL_FEATURES feature_id, int seg_data) {
  assert(seg_data <= seg_feature_data_max[feature_id]);
  if (seg_data < 0) {
    assert(seg_feature_data_signed[feature_id]);
    assert(-seg_data <= seg_feature_data_max[feature_id]);
  }

  seg->feature_data[segment_id][feature_id] = (int16_t)seg_data;
}
#endif

const vpx_tree_index eb_vp9_segment_tree[TREE_SIZE(MAX_SEGMENTS)] = {
  2, 4, 6, 8, 10, 12, 0, -1, -2, -3, -4, -5, -6, -7
};

// TBD? Functions to read and write segment data with range / validity checking
