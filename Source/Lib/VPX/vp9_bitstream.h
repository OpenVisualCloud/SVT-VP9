/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VP9_ENCODER_VP9_BITSTREAM_H_
#define VPX_VP9_ENCODER_VP9_BITSTREAM_H_

#ifdef __cplusplus
extern "C" {
#endif

#define INLINE __inline

#include <stdint.h>
#include "vp9_encoder.h"
#include "bitwriter.h"
#include "EbPictureControlSet.h"
#include "EbBitstreamUnit.h"
#include "EbEntropyCodingProcess.h"

typedef struct VP9BitstreamWorkerData {
  uint8_t *dest;
  int dest_size;
  VpxWriter bit_writer;
  int tile_idx;
  unsigned int max_mv_magnitude;
  // The size of interp_filter_selected in VP9_COMP is actually
  // MAX_REFERENCE_FRAMES x SWITCHABLE. But when encoding tiles, all we ever do
  // is increment the very first index (index 0) for the first dimension. Hence
  // this is sufficient.
  int interp_filter_selected[1][SWITCHABLE];
  DECLARE_ALIGNED(16, MACROBLOCKD, xd);
} VP9BitstreamWorkerData;
#if 0
int vp9_get_refresh_mask(VP9_COMP *cpi);
#endif
void vp9_bitstream_encode_tiles_buffer_dealloc(VP9_COMP *const cpi);

void eb_vp9_pack_bitstream(
    PictureControlSet   *picture_control_set_ptr,
    VP9_COMP            *cpi,
    uint8_t             *dest,
    size_t              *size,
    int                  show_existing_frame,
    int                  show_existing_frame_index);

#if 0
static INLINE int vp9_preserve_existing_gf(VP9_COMP *cpi) {
  return cpi->refresh_golden_frame && cpi->rc.is_src_frame_alt_ref &&
         !cpi->use_svc;
}
#endif

void write_partition(const VP9_COMMON *const cm,
    const MACROBLOCKD *const xd, int hbs, int mi_row,
    int mi_col, PARTITION_TYPE p, BLOCK_SIZE bsize,
    VpxWriter *w);

void eb_vp9_write_modes_b(
    EntropyCodingContext *context_ptr,
    VP9_COMP *cpi, MACROBLOCKD *const xd, const TileInfo *const tile,
    VpxWriter *w, TOKENEXTRA **tok, const TOKENEXTRA *const tok_end,
    int mi_row, int mi_col, unsigned int *const max_mv_magnitude,
    int interp_filter_selected[MAX_REF_FRAMES][SWITCHABLE]);

size_t write_compressed_header(VP9_COMP *cpi, uint8_t *data);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VPX_VP9_ENCODER_VP9_BITSTREAM_H_
