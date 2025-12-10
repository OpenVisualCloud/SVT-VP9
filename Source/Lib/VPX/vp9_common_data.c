/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdint.h>
#include "vp9_enums.h" //#include "vp9_enums.h" NO

// Log 2 conversion lookup tables for block width and height
const uint8_t eb_vp9_b_width_log2_lookup[BLOCK_SIZES]        = {0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4};
const uint8_t eb_vp9_b_height_log2_lookup[BLOCK_SIZES]       = {0, 1, 0, 1, 2, 1, 2, 3, 2, 3, 4, 3, 4};
const uint8_t eb_vp9_num_4x4_blocks_wide_lookup[BLOCK_SIZES] = {1, 1, 2, 2, 2, 4, 4, 4, 8, 8, 8, 16, 16};
const uint8_t eb_vp9_num_4x4_blocks_high_lookup[BLOCK_SIZES] = {1, 2, 1, 2, 4, 2, 4, 8, 4, 8, 16, 8, 16};
// Log 2 conversion lookup tables for ModeInfo width and height
const uint8_t eb_vp9_mi_width_log2_lookup[BLOCK_SIZES]       = {0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3};
const uint8_t eb_vp9_num_8x8_blocks_wide_lookup[BLOCK_SIZES] = {1, 1, 1, 1, 1, 2, 2, 2, 4, 4, 4, 8, 8};
const uint8_t eb_vp9_num_8x8_blocks_high_lookup[BLOCK_SIZES] = {1, 1, 1, 1, 2, 1, 2, 4, 2, 4, 8, 4, 8};

// VPXMIN(3, VPXMIN(eb_vp9_b_width_log2_lookup(bsize), eb_vp9_b_height_log2_lookup(bsize)))
const uint8_t eb_vp9_size_group_lookup[BLOCK_SIZES] = {0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3};

const BLOCK_SIZE eb_vp9_subsize_lookup[PARTITION_TYPES][BLOCK_SIZES] = {{// PARTITION_NONE
                                                                         BLOCK_4X4,
                                                                         BLOCK_4X8,
                                                                         BLOCK_8X4,
                                                                         BLOCK_8X8,
                                                                         BLOCK_8X16,
                                                                         BLOCK_16X8,
                                                                         BLOCK_16X16,
                                                                         BLOCK_16X32,
                                                                         BLOCK_32X16,
                                                                         BLOCK_32X32,
                                                                         BLOCK_32X64,
                                                                         BLOCK_64X32,
                                                                         BLOCK_64X64},
                                                                        {// PARTITION_HORZ
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_8X4,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_16X8,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_32X16,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_64X32},
                                                                        {// PARTITION_VERT
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_4X8,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_8X16,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_16X32,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_32X64},
                                                                        {// PARTITION_SPLIT
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_4X4,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_8X8,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_16X16,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_INVALID,
                                                                         BLOCK_32X32}};

const TX_SIZE eb_vp9_max_txsize_lookup[BLOCK_SIZES] = {TX_4X4,
                                                       TX_4X4,
                                                       TX_4X4,
                                                       TX_8X8,
                                                       TX_8X8,
                                                       TX_8X8,
                                                       TX_16X16,
                                                       TX_16X16,
                                                       TX_16X16,
                                                       TX_32X32,
                                                       TX_32X32,
                                                       TX_32X32,
                                                       TX_32X32};

const TX_SIZE eb_vp9_tx_mode_to_biggest_tx_size[TX_MODES] = {
    TX_4X4, // ONLY_4X4
    TX_8X8, // ALLOW_8X8
    TX_16X16, // ALLOW_16X16
    TX_32X32, // ALLOW_32X32
    TX_32X32, // TX_MODE_SELECT
};

const BLOCK_SIZE eb_vp9_ss_size_lookup[BLOCK_SIZES][2][2] = {
    //  ss_x == 0    ss_x == 0        ss_x == 1      ss_x == 1
    //  ss_y == 0    ss_y == 1        ss_y == 0      ss_y == 1
    {{BLOCK_4X4, BLOCK_INVALID}, {BLOCK_INVALID, BLOCK_INVALID}},
    {{BLOCK_4X8, BLOCK_4X4}, {BLOCK_INVALID, BLOCK_INVALID}},
    {{BLOCK_8X4, BLOCK_INVALID}, {BLOCK_4X4, BLOCK_INVALID}},
    {{BLOCK_8X8, BLOCK_8X4}, {BLOCK_4X8, BLOCK_4X4}},
    {{BLOCK_8X16, BLOCK_8X8}, {BLOCK_INVALID, BLOCK_4X8}},
    {{BLOCK_16X8, BLOCK_INVALID}, {BLOCK_8X8, BLOCK_8X4}},
    {{BLOCK_16X16, BLOCK_16X8}, {BLOCK_8X16, BLOCK_8X8}},
    {{BLOCK_16X32, BLOCK_16X16}, {BLOCK_INVALID, BLOCK_8X16}},
    {{BLOCK_32X16, BLOCK_INVALID}, {BLOCK_16X16, BLOCK_16X8}},
    {{BLOCK_32X32, BLOCK_32X16}, {BLOCK_16X32, BLOCK_16X16}},
    {{BLOCK_32X64, BLOCK_32X32}, {BLOCK_INVALID, BLOCK_16X32}},
    {{BLOCK_64X32, BLOCK_INVALID}, {BLOCK_32X32, BLOCK_32X16}},
    {{BLOCK_64X64, BLOCK_64X32}, {BLOCK_32X64, BLOCK_32X32}},
};

const TX_SIZE eb_vp9_uv_txsize_lookup[BLOCK_SIZES][TX_SIZES][2][2] = {
    //  ss_x == 0    ss_x == 0        ss_x == 1      ss_x == 1
    //  ss_y == 0    ss_y == 1        ss_y == 0      ss_y == 1
    {
        // BLOCK_4X4
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
    },
    {
        // BLOCK_4X8
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
    },
    {
        // BLOCK_8X4
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
    },
    {
        // BLOCK_8X8
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_4X4}, {TX_4X4, TX_4X4}},
    },
    {
        // BLOCK_8X16
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_8X8}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_8X8}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_8X8}, {TX_4X4, TX_4X4}},
    },
    {
        // BLOCK_16X8
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_4X4}, {TX_8X8, TX_4X4}},
        {{TX_8X8, TX_4X4}, {TX_8X8, TX_8X8}},
        {{TX_8X8, TX_4X4}, {TX_8X8, TX_8X8}},
    },
    {
        // BLOCK_16X16
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_8X8}, {TX_8X8, TX_8X8}},
        {{TX_16X16, TX_8X8}, {TX_8X8, TX_8X8}},
        {{TX_16X16, TX_8X8}, {TX_8X8, TX_8X8}},
    },
    {
        // BLOCK_16X32
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_8X8}, {TX_8X8, TX_8X8}},
        {{TX_16X16, TX_16X16}, {TX_8X8, TX_8X8}},
        {{TX_16X16, TX_16X16}, {TX_8X8, TX_8X8}},
    },
    {
        // BLOCK_32X16
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_8X8}, {TX_8X8, TX_8X8}},
        {{TX_16X16, TX_8X8}, {TX_16X16, TX_8X8}},
        {{TX_16X16, TX_8X8}, {TX_16X16, TX_8X8}},
    },
    {
        // BLOCK_32X32
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_8X8}, {TX_8X8, TX_8X8}},
        {{TX_16X16, TX_16X16}, {TX_16X16, TX_16X16}},
        {{TX_32X32, TX_16X16}, {TX_16X16, TX_16X16}},
    },
    {
        // BLOCK_32X64
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_8X8}, {TX_8X8, TX_8X8}},
        {{TX_16X16, TX_16X16}, {TX_16X16, TX_16X16}},
        {{TX_32X32, TX_32X32}, {TX_16X16, TX_16X16}},
    },
    {
        // BLOCK_64X32
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_8X8}, {TX_8X8, TX_8X8}},
        {{TX_16X16, TX_16X16}, {TX_16X16, TX_16X16}},
        {{TX_32X32, TX_16X16}, {TX_32X32, TX_16X16}},
    },
    {
        // BLOCK_64X64
        {{TX_4X4, TX_4X4}, {TX_4X4, TX_4X4}},
        {{TX_8X8, TX_8X8}, {TX_8X8, TX_8X8}},
        {{TX_16X16, TX_16X16}, {TX_16X16, TX_16X16}},
        {{TX_32X32, TX_32X32}, {TX_32X32, TX_32X32}},
    },
};
