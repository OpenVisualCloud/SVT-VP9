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

#include "mem.h"

#include "vp9_loopfilter.h"
#include "vp9_onyxc_int.h"
#include "vp9_quant_common.h"

#include "vp9_encoder.h"
#include "vp9_picklpf.h"
#include "vp9_blockd.h"
#include "vpx_dsp_common.h"

static int get_max_filter_level(const VP9_COMP *cpi) {
    (void)cpi;
#if 1 // Hsan: filter_level derivation
    return MAX_LOOP_FILTER;
#else
  if (cpi->oxcf.pass == 2) {
    return cpi->twopass.section_intra_rating > 8 ? MAX_LOOP_FILTER * 3 / 4
                                                 : MAX_LOOP_FILTER;
  } else {
    return MAX_LOOP_FILTER;
  }
#endif
}

void eb_vp9_pick_filter_level(
#if 0
    const YV12_BUFFER_CONFIG *sd,
#endif
    VP9_COMP *cpi,
    LPF_PICK_METHOD method) {

  VP9_COMMON *const cm = &cpi->common;
  struct loop_filter *const lf = &cm->lf;

  lf->sharpness_level = 0;

  if (method == LPF_PICK_MINIMAL_LPF && lf->filter_level) {
    lf->filter_level = 0;
  } else if (method >= LPF_PICK_FROM_Q) {
    const int min_filter_level = 0;
    const int max_filter_level = get_max_filter_level(cpi);
    const int q = eb_vp9_ac_quant(cm->base_qindex, 0, cm->bit_depth);
// These values were determined by linear fitting the result of the
// searched level, filt_guess = q * 0.316206 + 3.87252
#if CONFIG_VP9_HIGHBITDEPTH
    int filt_guess;
    switch (cm->bit_depth) {
      case VPX_BITS_8:
        filt_guess = ROUND_POWER_OF_TWO(q * 20723 + 1015158, 18);
        break;
      case VPX_BITS_10:
        filt_guess = ROUND_POWER_OF_TWO(q * 20723 + 4060632, 20);
        break;
      default:
        assert(cm->bit_depth == VPX_BITS_12);
        filt_guess = ROUND_POWER_OF_TWO(q * 20723 + 16242526, 22);
        break;
    }
#else
    int filt_guess = ROUND_POWER_OF_TWO(q * 20723 + 1015158, 18);
#endif  // CONFIG_VP9_HIGHBITDEPTH

#if 0 // Hsan: condition not satisfied for SVT-VP9
    if (cpi->oxcf.pass == 0 && cpi->oxcf.rc_mode == VPX_CBR &&
        cpi->oxcf.aq_mode == CYCLIC_REFRESH_AQ && cm->seg.enabled &&
        cpi->oxcf.content != VP9E_CONTENT_SCREEN && cm->frame_type != KEY_FRAME)
      filt_guess = 5 * filt_guess >> 3;
#endif

    if (cm->frame_type == KEY_FRAME) filt_guess -= 4;

    lf->filter_level = clamp(filt_guess, min_filter_level, max_filter_level);

  } else {
#if 1 // Hsan: not supported
      assert(0);
#else
    lf->filter_level =
        search_filter_level(sd, cpi, method == LPF_PICK_FROM_SUBIMAGE);
#endif
  }

}
