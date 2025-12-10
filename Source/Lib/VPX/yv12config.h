/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VPX_SCALE_YV12CONFIG_H_
#define VPX_VPX_SCALE_YV12CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "vpx_codec.h"

#define VP8BORDERINPIXELS 32
#define VP9INNERBORDERINPIXELS 96
#define VP9_INTERP_EXTEND 4
#define VP9_ENC_BORDER_IN_PIXELS 160
#define VP9_DEC_BORDER_IN_PIXELS 32

typedef struct yv12_buffer_config {
    int y_width;
    int y_height;
    int y_crop_width;
    int y_crop_height;
    int y_stride;

    int uv_width;
    int uv_height;
    int uv_crop_width;
    int uv_crop_height;
    int uv_stride;

    int alpha_width;
    int alpha_height;
    int alpha_stride;

    uint8_t *y_buffer;
    uint8_t *u_buffer;
    uint8_t *v_buffer;
    uint8_t *alpha_buffer;

    uint8_t          *buffer_alloc;
    int               buffer_alloc_sz;
    int               border;
    int               frame_size;
    int               subsampling_x;
    int               subsampling_y;
    unsigned int      bit_depth;
    vpx_color_space_t color_space;
    vpx_color_range_t color_range;
    int               render_width;
    int               render_height;

    int corrupted;
    int flags;
} YV12_BUFFER_CONFIG;

#define YV12_FLAG_HIGHBITDEPTH 8

#ifdef __cplusplus
}
#endif

#endif // VPX_VPX_SCALE_YV12CONFIG_H_
