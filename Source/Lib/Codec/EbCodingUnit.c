/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbCodingUnit.h"
#include "EbUtility.h"
/*
Tasks & Questions
    -Need a GetEmptyChain function for testing sub partitions.  Tie it to an Itr?
    -How many empty CUs do we need?  We need to have enough for the max CU count AND
       enough for temp storage when calculating a subdivision.
    -Where do we store temp reconstructed picture data while deciding to subdivide or not?
    -Need a ReconPicture for each candidate.
    -I don't see a way around doing the copies in temp memory and then copying it in...
*/
EbErrorType sb_unit_ctor(
    SbUnit                  **sb_unit_dbl_ptr,
    uint32_t                  picture_width,
    uint32_t                  picture_height,
    uint16_t                  sb_origin_x,
    uint16_t                  sb_origin_y,
    uint16_t                  sb_index,
    struct PictureControlSet *picture_control_set)

{
    EbErrorType return_error = EB_ErrorNone;
    uint32_t border_largest_block_size;
    uint16_t block_index;
#if !VP9_PERFORM_EP
    EbPictureBufferDescInitData coeff_init_data;
#endif
    SbUnit *sb_unit_ptr;
    EB_MALLOC(SbUnit*, sb_unit_ptr, sizeof(SbUnit), EB_N_PTR);

    *sb_unit_dbl_ptr = sb_unit_ptr;

    // ************ LCU ***************
    if((picture_width - sb_origin_x) < MAX_SB_SIZE) {
        border_largest_block_size = picture_width - sb_origin_x;
        // Which border_largest_block_size is not a power of two
        while((border_largest_block_size & (border_largest_block_size - 1)) > 0) {
            border_largest_block_size -= (border_largest_block_size & ((~0u) << Log2f(border_largest_block_size)));
        }
    }

    if((picture_height - sb_origin_y) < MAX_SB_SIZE) {
        border_largest_block_size = picture_height - sb_origin_y;
        // Which border_largest_block_size is not a power of two
        while((border_largest_block_size & (border_largest_block_size - 1)) > 0) {
            border_largest_block_size -= (border_largest_block_size & ((~0u) << Log2f(border_largest_block_size)));
        }
    }
    sb_unit_ptr->picture_control_set_ptr = picture_control_set;
    sb_unit_ptr->origin_x                = sb_origin_x;
    sb_unit_ptr->origin_y                = sb_origin_y;
    sb_unit_ptr->sb_index                = sb_index;

    EB_MALLOC(CodingUnit**, sb_unit_ptr->coded_block_array_ptr, sizeof(CodingUnit*) * EP_BLOCK_MAX_COUNT, EB_N_PTR);

    for(block_index=0; block_index < EP_BLOCK_MAX_COUNT; ++block_index) {
        EB_MALLOC(CodingUnit*, sb_unit_ptr->coded_block_array_ptr[block_index], sizeof(CodingUnit), EB_N_PTR);
        if (ep_get_block_stats(block_index)->bwidth == ep_get_block_stats(block_index)->bheight) {
            EB_MALLOC(MbModeInfoExt*, sb_unit_ptr->coded_block_array_ptr[block_index]->mbmi_ext, sizeof(MbModeInfoExt), EB_N_PTR);
        }
    }

#if VP9_PERFORM_EP
    EB_MALLOC(int16_t *, sb_unit_ptr->quantized_coeff_buffer[0], MAX_SB_SIZE * MAX_SB_SIZE * sizeof(uint64_t), EB_N_PTR);
    EB_MALLOC(int16_t *, sb_unit_ptr->quantized_coeff_buffer[1], (MAX_SB_SIZE >> 1) * (MAX_SB_SIZE >> 1) * sizeof(uint64_t), EB_N_PTR);
    EB_MALLOC(int16_t *, sb_unit_ptr->quantized_coeff_buffer[2], (MAX_SB_SIZE >> 1) * (MAX_SB_SIZE >> 1) * sizeof(uint64_t), EB_N_PTR);
#else
    coeff_init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_FULL_MASK;
    coeff_init_data.bit_depth = EB_16BIT;
    coeff_init_data.left_padding = 0;
    coeff_init_data.right_padding = 0;
    coeff_init_data.top_padding = 0;
    coeff_init_data.bot_padding = 0;
    coeff_init_data.split_mode = EB_FALSE;
    coeff_init_data.max_width = MAX_SB_SIZE;
    coeff_init_data.max_height = MAX_SB_SIZE;

    return_error = eb_vp9_picture_buffer_desc_ctor(
        (EbPtr *) &(sb_unit_ptr->quantized_coeff),
        (EbPtr)&coeff_init_data);

    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }
#endif

    return return_error;
}
