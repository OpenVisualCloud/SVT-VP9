/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbNeighborArrays.h"
#include "EbUtility.h"
#include "EbDefinitions.h"

/*************************************************
 * Neighbor Array Unit Ctor
 *************************************************/
EbErrorType eb_vp9_neighbor_array_unit_ctor(NeighborArrayUnit **na_unit_dbl_ptr, uint32_t maxpicture_width,
                                            uint32_t maxpicture_height, uint32_t unit_size, uint32_t granularity_normal,
                                            uint32_t granularity_top_left, uint32_t type_mask) {
    NeighborArrayUnit *na_unit_ptr;
    EB_MALLOC(NeighborArrayUnit *, na_unit_ptr, sizeof(NeighborArrayUnit), EB_N_PTR);

    *na_unit_dbl_ptr                       = na_unit_ptr;
    na_unit_ptr->unit_size                 = (uint8_t)(unit_size);
    na_unit_ptr->granularity_normal        = (uint8_t)(granularity_normal);
    na_unit_ptr->granularity_normal_log2   = (uint8_t)(Log2f(na_unit_ptr->granularity_normal));
    na_unit_ptr->granularity_top_left      = (uint8_t)(granularity_top_left);
    na_unit_ptr->granularity_top_left_log2 = (uint8_t)(Log2f(na_unit_ptr->granularity_top_left));
    na_unit_ptr->left_array_size           = (uint16_t)((type_mask & NEIGHBOR_ARRAY_UNIT_LEFT_MASK)
                                                            ? maxpicture_height >> na_unit_ptr->granularity_normal_log2
                                                            : 0);
    na_unit_ptr->top_array_size            = (uint16_t)((type_mask & NEIGHBOR_ARRAY_UNIT_TOP_MASK)
                                                            ? maxpicture_width >> na_unit_ptr->granularity_normal_log2
                                                            : 0);
    na_unit_ptr->top_left_array_size       = (uint16_t)((type_mask & NEIGHBOR_ARRAY_UNIT_TOPLEFT_MASK)
                                                            ? (maxpicture_width + maxpicture_height) >>
                                                          na_unit_ptr->granularity_top_left_log2
                                                            : 0);

    if (na_unit_ptr->left_array_size) {
        EB_MALLOC(uint8_t *, na_unit_ptr->left_array, na_unit_ptr->unit_size * na_unit_ptr->left_array_size, EB_N_PTR);
    } else {
        na_unit_ptr->left_array = (uint8_t *)NULL;
    }

    if (na_unit_ptr->top_array_size) {
        EB_MALLOC(uint8_t *, na_unit_ptr->top_array, na_unit_ptr->unit_size * na_unit_ptr->top_array_size, EB_N_PTR);
    } else {
        na_unit_ptr->top_array = (uint8_t *)NULL;
    }

    if (na_unit_ptr->top_left_array_size) {
        EB_MALLOC(uint8_t *,
                  na_unit_ptr->top_left_array,
                  na_unit_ptr->unit_size * na_unit_ptr->top_left_array_size,
                  EB_N_PTR);
    } else {
        na_unit_ptr->top_left_array = (uint8_t *)NULL;
    }

    return EB_ErrorNone;
}

/*************************************************
 * Neighbor Array Unit Reset
 *************************************************/
void eb_vp9_neighbor_array_unit_reset(NeighborArrayUnit *na_unit_ptr) {
    if (na_unit_ptr->left_array) {
        memset(na_unit_ptr->left_array, ~0, na_unit_ptr->unit_size * na_unit_ptr->left_array_size);
    }

    if (na_unit_ptr->top_array) {
        memset(na_unit_ptr->top_array, ~0, na_unit_ptr->unit_size * na_unit_ptr->top_array_size);
    }

    if (na_unit_ptr->top_left_array) {
        memset(na_unit_ptr->top_left_array, ~0, na_unit_ptr->unit_size * na_unit_ptr->top_left_array_size);
    }

    return;
}

/*************************************************
 * Neighbor Array Unit Get Left Index
 *************************************************/
static uint32_t get_neighbor_array_unit_left_index(NeighborArrayUnit *na_unit_ptr, uint32_t loc_y) {
    return (loc_y >> na_unit_ptr->granularity_normal_log2);
}

/*************************************************
 * Neighbor Array Unit Get Top Index
 *************************************************/
static uint32_t get_neighbor_array_unit_top_index(NeighborArrayUnit *na_unit_ptr, uint32_t loc_x) {
    return (loc_x >> na_unit_ptr->granularity_normal_log2);
}

/*************************************************
 * Neighbor Array Unit Get Top Index
 *************************************************/
static uint32_t get_neighbor_array_unit_top_left_index(NeighborArrayUnit *na_unit_ptr, int32_t loc_x, int32_t loc_y) {
    return na_unit_ptr->left_array_size + (loc_x >> na_unit_ptr->granularity_top_left_log2) -
        (loc_y >> na_unit_ptr->granularity_top_left_log2);
}

/*************************************************
 * Neighbor Array Sample Update
 *************************************************/
void eb_vp9_neighbor_array_unit_sample_write(NeighborArrayUnit *na_unit_ptr, uint8_t *src_ptr, uint32_t stride,
                                             uint32_t srcorigin_x, uint32_t srcorigin_y, uint32_t picorigin_x,
                                             uint32_t picorigin_y, uint32_t block_width, uint32_t block_height,
                                             uint32_t neighbor_array_type_mask) {
    uint32_t idx;
    uint8_t *dst_ptr;
    uint8_t *read_ptr;

    int32_t  dst_step;
    int32_t  read_step;
    uint32_t count;

    // Adjust the Source ptr to start at the origin of the block being updated.
    src_ptr += ((srcorigin_y * stride) + srcorigin_x) * na_unit_ptr->unit_size;

    if (neighbor_array_type_mask & NEIGHBOR_ARRAY_UNIT_TOP_MASK) {
        //
        //     ----------12345678---------------------  Top Neighbor Array
        //                ^    ^
        //                |    |
        //                |    |
        //               xxxxxxxx
        //               x      x
        //               x      x
        //               12345678
        //
        //  The top neighbor array is updated with the samples from the
        //    bottom row of the source block
        //
        //  Index = origin_x

        // Adjust read_ptr to the bottom-row
        read_ptr = src_ptr + ((block_height - 1) * stride);

        dst_ptr = na_unit_ptr->top_array +
            get_neighbor_array_unit_top_index(na_unit_ptr, picorigin_x) * na_unit_ptr->unit_size;

        dst_step  = na_unit_ptr->unit_size;
        read_step = na_unit_ptr->unit_size;
        count     = block_width;

        for (idx = 0; idx < count; ++idx) {
            *dst_ptr = *read_ptr;

            dst_ptr += dst_step;
            read_ptr += read_step;
        }
    }

    if (neighbor_array_type_mask & NEIGHBOR_ARRAY_UNIT_LEFT_MASK) {
        //   Left Neighbor Array
        //
        //    |
        //    |
        //    1         xxxxxxx1
        //    2  <----  x      2
        //    3  <----  x      3
        //    4         xxxxxxx4
        //    |
        //    |
        //
        //  The left neighbor array is updated with the samples from the
        //    right column of the source block
        //
        //  Index = origin_y

        // Adjust read_ptr to the right-column
        read_ptr = src_ptr + (block_width - 1);

        dst_ptr = na_unit_ptr->left_array +
            get_neighbor_array_unit_left_index(na_unit_ptr, picorigin_y) * na_unit_ptr->unit_size;

        dst_step  = 1;
        read_step = stride;
        count     = block_height;

        for (idx = 0; idx < count; ++idx) {
            *dst_ptr = *read_ptr;

            dst_ptr += dst_step;
            read_ptr += read_step;
        }
    }

    if (neighbor_array_type_mask & NEIGHBOR_ARRAY_UNIT_TOPLEFT_MASK) {
        /*
        //   Top-left Neighbor Array
        //
        //    4-5--6--7--------------
        //    3 \      \
        //    2  \      \
        //    1   \      \
        //    |\   xxxxxx7
        //    | \  x     6
        //    |  \ x     5
        //    |   \1x2x3x4
        //    |
        //
        //  The top-left neighbor array is updated with the reversed samples
        //    from the right column and bottom row of the source block
        //
        // Index = origin_x - origin_y
        */

        // Adjust read_ptr to the bottom-row
        read_ptr = src_ptr + ((block_height - 1) * stride);

        // Copy bottom row
        dst_ptr = na_unit_ptr->top_left_array +
            get_neighbor_array_unit_top_left_index(na_unit_ptr, picorigin_x, picorigin_y + (block_width - 1)) *
                na_unit_ptr->unit_size;

        memcpy(dst_ptr, read_ptr, block_width);

        // Reset read_ptr to the right-column
        read_ptr = src_ptr + (block_width - 1);

        // Copy right column
        dst_ptr = na_unit_ptr->top_left_array +
            get_neighbor_array_unit_top_left_index(na_unit_ptr, picorigin_x + (block_width - 1), picorigin_y) *
                na_unit_ptr->unit_size;

        dst_step  = -1;
        read_step = stride;
        count     = block_height;

        for (idx = 0; idx < count; ++idx) {
            *dst_ptr = *read_ptr;

            dst_ptr += dst_step;
            read_ptr += read_step;
        }
    }

    return;
}
