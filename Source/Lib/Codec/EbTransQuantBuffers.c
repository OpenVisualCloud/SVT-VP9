/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include "EbTransQuantBuffers.h"
#include "EbPictureBufferDesc.h"

EbErrorType eb_vp9_trans_quant_buffers_ctor(
    EbTransQuantBuffers *trans_quant_buffers_ptr)
{
    EbErrorType return_error = EB_ErrorNone;
    EbPictureBufferDescInitData trans_coeff_init_array;

    trans_coeff_init_array.max_width            = MAX_SB_SIZE;
    trans_coeff_init_array.max_height           = MAX_SB_SIZE;
    trans_coeff_init_array.bit_depth            = EB_16BIT;
    trans_coeff_init_array.buffer_enable_mask    = PICTURE_BUFFER_DESC_FULL_MASK;
    trans_coeff_init_array.left_padding            = 0;
    trans_coeff_init_array.right_padding        = 0;
    trans_coeff_init_array.top_padding            = 0;
    trans_coeff_init_array.bot_padding            = 0;
    trans_coeff_init_array.split_mode           = EB_FALSE;

    return_error = eb_vp9_picture_buffer_desc_ctor(
        (EbPtr *) &(trans_quant_buffers_ptr->tu_trans_coeff2_nx2_n_ptr),
        (EbPtr  ) &trans_coeff_init_array);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    return_error = eb_vp9_picture_buffer_desc_ctor(
        (EbPtr *) &(trans_quant_buffers_ptr->tu_trans_coeff_nx_n_ptr),
        (EbPtr  ) &trans_coeff_init_array);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    return_error = eb_vp9_picture_buffer_desc_ctor(
        (EbPtr *) &(trans_quant_buffers_ptr->tu_trans_coeff_n2x_n2_ptr),
        (EbPtr  ) &trans_coeff_init_array);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = eb_vp9_picture_buffer_desc_ctor(
        (EbPtr *) &(trans_quant_buffers_ptr->tu_quant_coeff_nx_n_ptr),
        (EbPtr  ) &trans_coeff_init_array);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    return_error = eb_vp9_picture_buffer_desc_ctor(
        (EbPtr *) &(trans_quant_buffers_ptr->tu_quant_coeff_n2x_n2_ptr),
        (EbPtr  ) &trans_coeff_init_array);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    return EB_ErrorNone;
}
