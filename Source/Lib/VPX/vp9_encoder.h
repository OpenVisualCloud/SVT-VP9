/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VP9_ENCODER_VP9_ENCODER_H_
#define VPX_VP9_ENCODER_VP9_ENCODER_H_

#include <stdio.h>

#include <stdint.h>
#include "vp9_onyxc_int.h"
#include "vp9_tokenize.h"
#include "vp9_block.h"
#include "vp9_speed_features.h"
#include "vp9_quantize.h"
#include "vp9_rd.h"
#include "vp9_ratectrl.h"

#if CONFIG_VP9_TEMPORAL_DENOISING
#include "vp9/encoder/vp9_denoiser.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    TOKENEXTRA  *start;
    TOKENEXTRA  *stop;
    unsigned int count;
} TOKENLIST;
typedef struct RD_COUNTS {
    vp9_coeff_count coef_counts[TX_SIZES][PLANE_TYPES];
    int64_t         comp_pred_diff[REFERENCE_MODES];
    int64_t         filter_diff[SWITCHABLE_FILTER_CONTEXTS];
} RD_COUNTS;

typedef struct ThreadData {
    MACROBLOCK    mb;
    RD_COUNTS     rd_counts;
    FRAME_COUNTS *counts;
} ThreadData;
struct EncWorkerData;

typedef struct ActiveMap {
    int            enabled;
    int            update;
    unsigned char *map;
} ActiveMap;

typedef enum { Y, U, V, ALL } STAT_TYPE;

typedef struct IMAGE_STAT {
    double stat[ALL + 1];
    double worst;
} ImageStat;

// Kf noise filtering currently disabled by default in build.
// #define ENABLE_KF_DENOISE 1

#define CPB_WINDOW_SIZE 4
#define FRAME_WINDOW_SIZE 128
#define SAMPLE_RATE_GRACE_P 0.015
#define VP9_LEVELS 14

typedef enum {
    LEVEL_UNKNOWN = 0,
    LEVEL_AUTO    = 1,
    LEVEL_1       = 10,
    LEVEL_1_1     = 11,
    LEVEL_2       = 20,
    LEVEL_2_1     = 21,
    LEVEL_3       = 30,
    LEVEL_3_1     = 31,
    LEVEL_4       = 40,
    LEVEL_4_1     = 41,
    LEVEL_5       = 50,
    LEVEL_5_1     = 51,
    LEVEL_5_2     = 52,
    LEVEL_6       = 60,
    LEVEL_6_1     = 61,
    LEVEL_6_2     = 62,
    LEVEL_MAX     = 255
} VP9_LEVEL;

typedef struct {
    VP9_LEVEL level;
    uint64_t  max_luma_sample_rate;
    uint32_t  max_luma_picture_size;
    uint32_t  max_luma_picture_breadth;
    double    average_bitrate; // in kilobits per second
    double    max_cpb_size; // in kilobits
    double    compression_ratio;
    uint8_t   max_col_tiles;
    uint32_t  min_altref_distance;
    uint8_t   max_ref_frame_buffers;
} Vp9LevelSpec;

extern const Vp9LevelSpec vp9_level_defs[VP9_LEVELS];

typedef struct {
    int64_t  ts; // timestamp
    uint32_t luma_samples;
    uint32_t size; // in bytes
} FrameRecord;

typedef struct {
    FrameRecord buf[FRAME_WINDOW_SIZE];
    uint8_t     start;
    uint8_t     len;
} FrameWindowBuffer;

typedef struct {
    uint8_t           seen_first_altref;
    uint32_t          frames_since_last_altref;
    uint64_t          total_compressed_size;
    uint64_t          total_uncompressed_size;
    double            time_encoded; // in seconds
    FrameWindowBuffer frame_window_buffer;
    int               ref_refresh_map;
} Vp9LevelStats;

typedef struct {
    Vp9LevelStats level_stats;
    Vp9LevelSpec  level_spec;
} Vp9LevelInfo;

typedef enum {
    BITRATE_TOO_LARGE = 0,
    LUMA_PIC_SIZE_TOO_LARGE,
    LUMA_PIC_BREADTH_TOO_LARGE,
    LUMA_SAMPLE_RATE_TOO_LARGE,
    CPB_TOO_LARGE,
    COMPRESSION_RATIO_TOO_SMALL,
    TOO_MANY_COLUMN_TILE,
    ALTREF_DIST_TOO_SMALL,
    TOO_MANY_REF_BUFFER,
    TARGET_LEVEL_FAIL_IDS
} TARGET_LEVEL_FAIL_ID;

typedef struct {
    int8_t  level_index;
    uint8_t rc_config_updated;
    uint8_t fail_flag;
    int     max_frame_size; // in bits
    double  max_cpb_size; // in bits
} LevelConstraint;
typedef struct VP9_COMP {
    QUANTS quants;

    ThreadData td;

    DECLARE_ALIGNED(16, int16_t, y_dequant[QINDEX_RANGE][8]);
    DECLARE_ALIGNED(16, int16_t, uv_dequant[QINDEX_RANGE][8]);

    VP9_COMMON common;

    int lst_fb_idx;
    int gld_fb_idx;
    int alt_fb_idx;

    YV12_BUFFER_CONFIG last_frame_uf;

    RD_OPT rd;

    int *nmvcosts[2];
    int *nmvcosts_hp[2];
    int *nmvsadcosts[2];
    int *nmvsadcosts_hp[2];

    RATE_CONTROL rc;
    int          interp_filter_selected[REF_FRAMES][SWITCHABLE];

    SPEED_FEATURES sf;

    uint32_t max_mv_magnitude;

    int allow_comp_inter_inter;

    uint8_t  *segmentation_map;
    ActiveMap active_map;

    int use_svc;

    int          mbmode_cost[INTRA_MODES];
    unsigned int inter_mode_cost[INTER_MODE_CONTEXTS][INTER_MODES];
    int          intra_uv_mode_cost[FRAME_TYPES][INTRA_MODES][INTRA_MODES];
    int          y_mode_costs[INTRA_MODES][INTRA_MODES][INTRA_MODES];
    int          switchable_interp_costs[SWITCHABLE_FILTER_CONTEXTS][SWITCHABLE_FILTERS];
    int          partition_cost[PARTITION_CONTEXTS][PARTITION_TYPES];
    // Indices are:  max_tx_size-1,  tx_size_ctx,    tx_size
    int tx_size_cost[TX_SIZES - 1][TX_SIZE_CONTEXTS][TX_SIZES];
} VP9_COMP;
static inline int get_ref_frame_map_idx(const VP9_COMP *cpi, MV_REFERENCE_FRAME ref_frame) {
    if (ref_frame == LAST_FRAME) {
        return cpi->lst_fb_idx;
    } else if (ref_frame == GOLDEN_FRAME) {
        return cpi->gld_fb_idx;
    } else {
        return cpi->alt_fb_idx;
    }
}
static inline int get_token_alloc(int mb_rows, int mb_cols) {
    // TODO(JBB): double check we can't exceed this token count if we have a
    // 32x32 transform crossing a boundary at a multiple of 16.
    // mb_rows, cols are in units of 16 pixels. We assume 3 planes all at full
    // resolution. We assume up to 1 token per pixel, and then allow
    // a head room of 4.
    return mb_rows * mb_cols * (16 * 16 * 3 + 4);
}

/***********************************************************************
 * Read before modifying 'cal_nmvjointsadcost' or 'cal_nmvsadcosts'    *
 ***********************************************************************
 * The following 2 functions ('cal_nmvjointsadcost' and                *
 * 'cal_nmvsadcosts') are used to calculate cost lookup tables         *
 * used by 'eb_vp9_diamond_search_sad'. The C implementation of the       *
 * function is generic, but the AVX intrinsics optimised version       *
 * relies on the following properties of the computed tables:          *
 * For cal_nmvjointsadcost:                                            *
 *   - mvjointsadcost[1] == mvjointsadcost[2] == mvjointsadcost[3]     *
 * For cal_nmvsadcosts:                                                *
 *   - For all i: mvsadcost[0][i] == mvsadcost[1][i]                   *
 *         (Equal costs for both components)                           *
 *   - For all i: mvsadcost[0][i] == mvsadcost[0][-i]                  *
 *         (Cost function is even)                                     *
 * If these do not hold, then the AVX optimised version of the         *
 * 'eb_vp9_diamond_search_sad' function cannot be used as it is, in which *
 * case you can revert to using the C function instead.                *
 ***********************************************************************/

void cal_nmvjointsadcost(int *mvjointsadcost);

void cal_nmvsadcosts(int *mvsadcost[2]);

void cal_nmvsadcosts_hp(int *mvsadcost[2]);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VPX_VP9_ENCODER_VP9_ENCODER_H_
