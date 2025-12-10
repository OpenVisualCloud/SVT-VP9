/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VP9_COMMON_VP9_RECONINTER_H_
#define VPX_VP9_COMMON_VP9_RECONINTER_H_

#include "vp9_filter.h"
#include "vp9_onyxc_int.h"
#include "vpx_filter.h"
#include "EbEncDecProcess.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void inter_predictor(const uint8_t *src, int src_stride, uint8_t *dst, int dst_stride, const int subpel_x,
                                   const int subpel_y, const struct scale_factors *sf, int w, int h, int ref,
                                   const InterpKernel *kernel, int xs, int ys) {
    sf->predict[subpel_x != 0][subpel_y != 0][ref](
        src, src_stride, dst, dst_stride, kernel, subpel_x, xs, subpel_y, ys, w, h);
}

void build_inter_predictors(EncDecContext *context_ptr, EbByte pred_buffer, uint16_t pred_stride, MACROBLOCKD *xd,
                            int plane, int block, int bw, int bh, int x, int y, int w, int h, int mi_x, int mi_y);

static inline int scaled_buffer_offset(int x_offset, int y_offset, int stride, const struct scale_factors *sf) {
    const int x = sf ? sf->scale_value_x(x_offset, sf) : x_offset;
    const int y = sf ? sf->scale_value_y(y_offset, sf) : y_offset;
    return y * stride + x;
}

static inline void setup_pred_plane(struct buf_2d *dst, uint8_t *src, int stride, int mi_row, int mi_col,
                                    const struct scale_factors *scale, int subsampling_x, int subsampling_y) {
    const int x = (MI_SIZE * mi_col) >> subsampling_x;
    const int y = (MI_SIZE * mi_row) >> subsampling_y;
    dst->buf    = src + scaled_buffer_offset(x, y, stride, scale);
    dst->stride = stride;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VPX_VP9_COMMON_VP9_RECONINTER_H_
