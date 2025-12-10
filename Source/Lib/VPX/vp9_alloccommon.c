/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "mem.h"
#include "vp9_alloccommon.h"
#include "vp9_enums.h" //#include "vp9_blockd.h"
#include "vp9_onyxc_int.h"

void eb_vp9_set_mb_mi(VP9_COMMON *cm, int width, int height) {
    const int aligned_width  = ALIGN_POWER_OF_TWO(width, MI_SIZE_LOG2);
    const int aligned_height = ALIGN_POWER_OF_TWO(height, MI_SIZE_LOG2);

    cm->mi_cols   = aligned_width >> MI_SIZE_LOG2;
    cm->mi_rows   = aligned_height >> MI_SIZE_LOG2;
    cm->mi_stride = cm->mi_cols;

    cm->mb_cols = (cm->mi_cols + 1) >> 1;
    cm->mb_rows = (cm->mi_rows + 1) >> 1;
    cm->MBs     = cm->mb_rows * cm->mb_cols;
}
