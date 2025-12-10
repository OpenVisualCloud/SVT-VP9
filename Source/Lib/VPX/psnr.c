/*
 *  Copyright (c) 2016 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <math.h>
#include <assert.h>
#include "vpx_dsp_rtcd.h"
#include "psnr.h"
#include "yv12config.h"

double eb_vp9_sse_to_psnr(double samples, double peak, double sse) {
    if (sse > 0.0) {
        const double psnr = 10.0 * log10(samples * peak * peak / sse);
        return psnr > MAX_PSNR ? MAX_PSNR : psnr;
    } else {
        return MAX_PSNR;
    }
}

/* TODO(yaowu): The block_variance calls the unoptimized versions of variance()
 * and highbd_8_variance(). It should not.
 */
static void encoder_variance(const uint8_t *a, int a_stride, const uint8_t *b, int b_stride, int w, int h,
                             unsigned int *sse, int *sum) {
    int i, j;

    *sum = 0;
    *sse = 0;

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            const int diff = a[j] - b[j];
            *sum += diff;
            *sse += diff * diff;
        }

        a += a_stride;
        b += b_stride;
    }
}

static int64_t get_sse(const uint8_t *a, int a_stride, const uint8_t *b, int b_stride, int width, int height) {
    const int    dw        = width % 16;
    const int    dh        = height % 16;
    int64_t      total_sse = 0;
    unsigned int sse       = 0;
    int          sum       = 0;
    int          x, y;

    if (dw > 0) {
        encoder_variance(&a[width - dw], a_stride, &b[width - dw], b_stride, dw, height, &sse, &sum);
        total_sse += sse;
    }

    if (dh > 0) {
        encoder_variance(
            &a[(height - dh) * a_stride], a_stride, &b[(height - dh) * b_stride], b_stride, width - dw, dh, &sse, &sum);
        total_sse += sse;
    }

    for (y = 0; y < height / 16; ++y) {
        const uint8_t *pa = a;
        const uint8_t *pb = b;
        for (x = 0; x < width / 16; ++x) {
            eb_vp9_mse16x16(pa, a_stride, pb, b_stride, &sse);
            total_sse += sse;

            pa += 16;
            pb += 16;
        }

        a += 16 * a_stride;
        b += 16 * b_stride;
    }

    return total_sse;
}

int64_t eb_vp9_get_y_sse(const YV12_BUFFER_CONFIG *a, const YV12_BUFFER_CONFIG *b) {
    assert(a->y_crop_width == b->y_crop_width);
    assert(a->y_crop_height == b->y_crop_height);

    return get_sse(a->y_buffer, a->y_stride, b->y_buffer, b->y_stride, a->y_crop_width, a->y_crop_height);
}

void eb_vp9_calc_psnr(const YV12_BUFFER_CONFIG *a, const YV12_BUFFER_CONFIG *b, PSNR_STATS *psnr) {
    static const double peak         = 255.0;
    const int           widths[3]    = {a->y_crop_width, a->uv_crop_width, a->uv_crop_width};
    const int           heights[3]   = {a->y_crop_height, a->uv_crop_height, a->uv_crop_height};
    const uint8_t      *a_planes[3]  = {a->y_buffer, a->u_buffer, a->v_buffer};
    const int           a_strides[3] = {a->y_stride, a->uv_stride, a->uv_stride};
    const uint8_t      *b_planes[3]  = {b->y_buffer, b->u_buffer, b->v_buffer};
    const int           b_strides[3] = {b->y_stride, b->uv_stride, b->uv_stride};
    int                 i;
    uint64_t            total_sse     = 0;
    uint32_t            total_samples = 0;

    for (i = 0; i < 3; ++i) {
        const int      w       = widths[i];
        const int      h       = heights[i];
        const uint32_t samples = w * h;
        const uint64_t sse     = get_sse(a_planes[i], a_strides[i], b_planes[i], b_strides[i], w, h);
        psnr->sse[1 + i]       = sse;
        psnr->samples[1 + i]   = samples;
        psnr->psnr[1 + i]      = eb_vp9_sse_to_psnr(samples, peak, (double)sse);

        total_sse += sse;
        total_samples += samples;
    }

    psnr->sse[0]     = total_sse;
    psnr->samples[0] = total_samples;
    psnr->psnr[0]    = eb_vp9_sse_to_psnr((double)total_samples, peak, (double)total_sse);
}
