/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VP9_ENCODER_VP9_TOKENIZE_H_
#define VPX_VP9_ENCODER_VP9_TOKENIZE_H_

#define INLINE __inline

#include <stdint.h>
#include <stdlib.h>

#include "prob.h"
#include "vp9_enums.h"
#include "vp9_entropy.h"
#include "vp9_treewriter.h"
#include "vp9_blockd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EOSB_TOKEN 127  // Not signalled, encoder only

#if CONFIG_VP9_HIGHBITDEPTH
typedef int32_t EXTRABIT;
#else
typedef int16_t EXTRABIT;
#endif

typedef struct {
  int16_t token;
  EXTRABIT extra;
} TOKENVALUE;

typedef struct {
  const vpx_prob *context_tree;
  int16_t token;
  EXTRABIT extra;
} TOKENEXTRA;

extern const vpx_tree_index eb_vp9_coef_tree[];
extern const vpx_tree_index eb_vp9_coef_con_tree[];
extern const struct vp9_token eb_vp9_coef_encodings[];
#if 0
int vp9_is_skippable_in_plane(MACROBLOCK *x, BLOCK_SIZE bsize, int plane);
int vp9_has_high_freq_in_plane(MACROBLOCK *x, BLOCK_SIZE bsize, int plane);
#endif
struct VP9_COMP;
struct ThreadData;

void eb_vp9_tokenize_sb(struct VP9_COMP *cpi, MACROBLOCKD *const xd, struct ThreadData *td,
                     TOKENEXTRA **t, int dry_run, int seg_skip,
                     BLOCK_SIZE bsize);

typedef struct {
  const vpx_prob *prob;
  int len;
  int base_val;
  const int16_t *cost;
} vp9_extra_bit;

// indexed by token value
extern const vp9_extra_bit eb_vp9_extra_bits[ENTROPY_TOKENS];
#if CONFIG_VP9_HIGHBITDEPTH
extern const vp9_extra_bit eb_vp9_extra_bits_high10[ENTROPY_TOKENS];
extern const vp9_extra_bit eb_vp9_extra_bits_high12[ENTROPY_TOKENS];
#endif  // CONFIG_VP9_HIGHBITDEPTH

extern const int16_t *vp9_dct_value_cost_ptr;
/* TODO: The Token field should be broken out into a separate char array to
 *  improve cache locality, since it's needed for costing when the rest of the
 *  fields are not.
 */
extern const TOKENVALUE *vp9_dct_value_tokens_ptr;
extern const TOKENVALUE *eb_vp9_dct_cat_lt_10_value_tokens;
extern const int *eb_vp9_dct_cat_lt_10_value_cost;
extern const int16_t eb_vp9_cat6_low_cost[256];
extern const uint16_t eb_vp9_cat6_high_cost[64];
extern const uint16_t vp9_cat6_high10_high_cost[256];
extern const uint16_t vp9_cat6_high12_high_cost[1024];

#if CONFIG_VP9_HIGHBITDEPTH
static INLINE const uint16_t *vp9_get_high_cost_table(int bit_depth) {
  return bit_depth == 8 ? eb_vp9_cat6_high_cost
                        : (bit_depth == 10 ? vp9_cat6_high10_high_cost
                                           : vp9_cat6_high12_high_cost);
}
#else
static INLINE const uint16_t *vp9_get_high_cost_table(int bit_depth) {
  (void)bit_depth;
  return eb_vp9_cat6_high_cost;
}
#endif  // CONFIG_VP9_HIGHBITDEPTH

static INLINE void vp9_get_token_extra(int v, int16_t *token, EXTRABIT *extra) {
  if (v >= CAT6_MIN_VAL || v <= -CAT6_MIN_VAL) {
    *token = CATEGORY6_TOKEN;
    if (v >= CAT6_MIN_VAL)
      *extra = (EXTRABIT)( 2 * v - 2 * CAT6_MIN_VAL);
    else
      *extra = (EXTRABIT) (-2 * v - 2 * CAT6_MIN_VAL + 1);
    return;
  }
  *token = eb_vp9_dct_cat_lt_10_value_tokens[v].token;
  *extra = eb_vp9_dct_cat_lt_10_value_tokens[v].extra;
}
static INLINE int16_t vp9_get_token(int v) {
  if (v >= CAT6_MIN_VAL || v <= -CAT6_MIN_VAL) return 10;
  return eb_vp9_dct_cat_lt_10_value_tokens[v].token;
}

static INLINE int vp9_get_token_cost(int v, int16_t *token,
                                     const uint16_t *cat6_high_table) {
  if (v >= CAT6_MIN_VAL || v <= -CAT6_MIN_VAL) {
    EXTRABIT extra_bits;
    *token = CATEGORY6_TOKEN;
    extra_bits = (EXTRABIT)(abs(v) - CAT6_MIN_VAL);
    return eb_vp9_cat6_low_cost[extra_bits & 0xff] +
           cat6_high_table[extra_bits >> 8];
  }
  *token = eb_vp9_dct_cat_lt_10_value_tokens[v].token;
  return eb_vp9_dct_cat_lt_10_value_cost[v];
}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VPX_VP9_ENCODER_VP9_TOKENIZE_H_
