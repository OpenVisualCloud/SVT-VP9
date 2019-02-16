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

#if INTER_INTRA_BIAS
#define NEW_MV_DISCOUNT_FACTOR 8
int discount_newmv_test(const VP9_COMP *cpi, int this_mode,
    int_mv this_mv,
    int_mv(*mode_mv)[MAX_REF_FRAMES],
    int ref_frame);
#endif
#if 0
void vp9_rd_pick_intra_mode_sb(struct VP9_COMP *cpi, struct macroblock *x,
                               struct RD_COST *rd_cost, BLOCK_SIZE bsize,
                               PICK_MODE_CONTEXT *ctx, int64_t best_rd);

void vp9_rd_pick_inter_mode_sb(struct VP9_COMP *cpi,
                               struct TileDataEnc *tile_data,
                               struct macroblock *x, int mi_row, int mi_col,
                               struct RD_COST *rd_cost, BLOCK_SIZE bsize,
                               PICK_MODE_CONTEXT *ctx, int64_t best_rd_so_far);

void vp9_rd_pick_inter_mode_sb_seg_skip(
    struct VP9_COMP *cpi, struct TileDataEnc *tile_data, struct macroblock *x,
    struct RD_COST *rd_cost, BLOCK_SIZE bsize, PICK_MODE_CONTEXT *ctx,
    int64_t best_rd_so_far);

int vp9_internal_image_edge(struct VP9_COMP *cpi);
int vp9_active_h_edge(struct VP9_COMP *cpi, int mi_row, int mi_step);
int vp9_active_v_edge(struct VP9_COMP *cpi, int mi_col, int mi_step);
int vp9_active_edge_sb(struct VP9_COMP *cpi, int mi_row, int mi_col);

void vp9_rd_pick_inter_mode_sub8x8(struct VP9_COMP *cpi,
                                   struct TileDataEnc *tile_data,
                                   struct macroblock *x, int mi_row, int mi_col,
                                   struct RD_COST *rd_cost, BLOCK_SIZE bsize,
                                   PICK_MODE_CONTEXT *ctx,
                                   int64_t best_rd_so_far);
#endif

extern void estimate_ref_frame_costs(const VP9_COMMON *cm,
    const MACROBLOCKD *xd, int segment_id,
    unsigned int *ref_costs_single,
    unsigned int *ref_costs_comp,
    vpx_prob *comp_mode_p);

int cost_mv_ref(const VP9_COMP *cpi, PREDICTION_MODE mode,
    int mode_context);

#if 0
int rate_block(int plane, int block, TX_SIZE tx_size, int coeff_ctx,
    struct rdcost_block_args *args);
#endif

int cost_coeffs(MACROBLOCK *x, int plane, int block, TX_SIZE tx_size,
    int pt, const int16_t *scan, const int16_t *nb,
    int use_fast_coef_costing);
#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VPX_VP9_ENCODER_VP9_RDOPT_H_
