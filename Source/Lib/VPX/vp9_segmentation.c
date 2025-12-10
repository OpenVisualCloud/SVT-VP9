/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

//#include "vpx_mem.h"

#include "vp9_pred_common.h"
#include "vp9_tile_common.h"

#include <stdint.h>
#include "vp9_segmentation.h"

void eb_vp9_enable_segmentation(struct segmentation *seg) {
    seg->enabled     = 1;
    seg->update_map  = 1;
    seg->update_data = 1;
}

void eb_vp9_disable_segmentation(struct segmentation *seg) {
    seg->enabled     = 0;
    seg->update_map  = 0;
    seg->update_data = 0;
}

void eb_vp9_set_segment_data(struct segmentation *seg, signed char *feature_data, unsigned char abs_delta) {
    seg->abs_delta = abs_delta;

    memcpy(seg->feature_data, feature_data, sizeof(seg->feature_data));
}
void eb_vp9_disable_segfeature(struct segmentation *seg, int segment_id, SEG_LVL_FEATURES feature_id) {
    seg->feature_mask[segment_id] &= ~(1 << feature_id);
}

void eb_vp9_clear_segdata(struct segmentation *seg, int segment_id, SEG_LVL_FEATURES feature_id) {
    seg->feature_data[segment_id][feature_id] = 0;
}

// Based on set of segment counts calculate a probability tree
void calc_segtree_probs(int *segcounts, vpx_prob *segment_tree_probs) {
    // Work out probabilities of each segment
    const int c01 = segcounts[0] + segcounts[1];
    const int c23 = segcounts[2] + segcounts[3];
    const int c45 = segcounts[4] + segcounts[5];
    const int c67 = segcounts[6] + segcounts[7];

    segment_tree_probs[0] = get_binary_prob(c01 + c23, c45 + c67);
    segment_tree_probs[1] = get_binary_prob(c01, c23);
    segment_tree_probs[2] = get_binary_prob(c45, c67);
    segment_tree_probs[3] = get_binary_prob(segcounts[0], segcounts[1]);
    segment_tree_probs[4] = get_binary_prob(segcounts[2], segcounts[3]);
    segment_tree_probs[5] = get_binary_prob(segcounts[4], segcounts[5]);
    segment_tree_probs[6] = get_binary_prob(segcounts[6], segcounts[7]);
}
void eb_vp9_reset_segment_features(struct segmentation *seg) {
    // Set up default state for MB feature flags
    seg->enabled     = 0;
    seg->update_map  = 0;
    seg->update_data = 0;
    memset(seg->tree_probs, 255, sizeof(seg->tree_probs));
    eb_vp9_clearall_segfeatures(seg);
}
