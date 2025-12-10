/*
 * Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <math.h>

#include "vp9_encoder.h"
#ifndef M_LOG2_E
#define M_LOG2_E 0.693147180559945309417
#endif
#define log2f(x) (log(x) / (float)M_LOG2_E)

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

void cal_nmvjointsadcost(int *mvjointsadcost) {
    /*********************************************************************
   * Warning: Read the comments above before modifying this function   *
   *********************************************************************/
    mvjointsadcost[0] = 600;
    mvjointsadcost[1] = 300;
    mvjointsadcost[2] = 300;
    mvjointsadcost[3] = 300;
}

void cal_nmvsadcosts(int *mvsadcost[2]) {
    /*********************************************************************
   * Warning: Read the comments above before modifying this function   *
   *********************************************************************/
    int i = 1;

    mvsadcost[0][0] = 0;
    mvsadcost[1][0] = 0;

    do {
        double z         = 256 * (2 * (log2f(8 * i) + .6));
        mvsadcost[0][i]  = (int)z;
        mvsadcost[1][i]  = (int)z;
        mvsadcost[0][-i] = (int)z;
        mvsadcost[1][-i] = (int)z;
    } while (++i <= MV_MAX);
}

void cal_nmvsadcosts_hp(int *mvsadcost[2]) {
    int i = 1;

    mvsadcost[0][0] = 0;
    mvsadcost[1][0] = 0;

    do {
        double z         = 256 * (2 * (log2f(8 * i) + .6));
        mvsadcost[0][i]  = (int)z;
        mvsadcost[1][i]  = (int)z;
        mvsadcost[0][-i] = (int)z;
        mvsadcost[1][-i] = (int)z;
    } while (++i <= MV_MAX);
}
