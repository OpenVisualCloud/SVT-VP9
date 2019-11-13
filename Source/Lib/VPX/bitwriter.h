/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VPX_DSP_BITWRITER_H_
#define VPX_VPX_DSP_BITWRITER_H_

#define INLINE __inline

#include <stdint.h>
#include "prob.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VpxWriter {
  unsigned int lowvalue;
  unsigned int range;
  int count;
  unsigned int pos;
  uint8_t *buffer;
} VpxWriter;

void eb_vp9_start_encode(VpxWriter *bc, uint8_t *buffer);
void eb_vp9_stop_encode(VpxWriter *bc);

static INLINE void vpx_write(VpxWriter *br, int bit, int probability) {
  unsigned int split;
  int count = br->count;
  unsigned int range = br->range;
  unsigned int lowvalue = br->lowvalue;
  int shift;

  split = 1 + (((range - 1) * probability) >> 8);

  range = split;

  if (bit) {
    lowvalue += split;
    range = br->range - split;
  }

  shift = eb_vp9_norm[range];

  range <<= shift;
  count += shift;

  if (count >= 0) {
    int offset = shift - count;

    if ((lowvalue << (offset - 1)) & 0x80000000) {
      int x = br->pos - 1;

      while (x >= 0 && br->buffer[x] == 0xff) {
        br->buffer[x] = 0;
        x--;
      }

      br->buffer[x] += 1;
    }

    br->buffer[br->pos++] = (uint8_t)((lowvalue >> (24 - offset)));
    lowvalue <<= offset;
    shift = count;
    lowvalue &= 0xffffff;
    count -= 8;
  }

  lowvalue <<= shift;
  br->count = count;
  br->lowvalue = lowvalue;
  br->range = range;
}

static INLINE void vpx_write_bit(VpxWriter *w, int bit) {
  vpx_write(w, bit, 128);  // vpx_prob_half
}

static INLINE void vpx_write_literal(VpxWriter *w, int data, int bits) {
  int bit;

  for (bit = bits - 1; bit >= 0; bit--) vpx_write_bit(w, 1 & (data >> bit));
}

#define vpx_write_prob(w, v) vpx_write_literal((w), (v), 8)

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VPX_VPX_DSP_BITWRITER_H_
