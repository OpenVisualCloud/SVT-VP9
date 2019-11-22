/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbPictureBufferDesc.h"
#include "EbDefinitions.h"
/*****************************************
 * eb_vp9_picture_buffer_desc_ctor
 *  Initializes the Buffer Descriptor's
 *  values that are fixed for the life of
 *  the descriptor.
 *****************************************/
EbErrorType eb_vp9_picture_buffer_desc_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    EbPictureBufferDesc          *picture_buffer_desc_ptr;
    EbPictureBufferDescInitData  *picture_buffer_desc_init_data_ptr = (EbPictureBufferDescInitData*) object_init_data_ptr;

    uint32_t bytesPerPixel = (picture_buffer_desc_init_data_ptr->bit_depth == EB_8BIT) ? 1 : 2;

    EB_MALLOC(EbPictureBufferDesc  *, picture_buffer_desc_ptr, sizeof(EbPictureBufferDesc  ), EB_N_PTR);

    // Allocate the PictureBufferDesc Object
    *object_dbl_ptr       = (EbPtr) picture_buffer_desc_ptr;

    // Set the Picture Buffer Static variables
    picture_buffer_desc_ptr->max_width        = picture_buffer_desc_init_data_ptr->max_width;
    picture_buffer_desc_ptr->max_height       = picture_buffer_desc_init_data_ptr->max_height;
    picture_buffer_desc_ptr->width            = picture_buffer_desc_init_data_ptr->max_width;
    picture_buffer_desc_ptr->height           = picture_buffer_desc_init_data_ptr->max_height;
    picture_buffer_desc_ptr->bit_depth        = picture_buffer_desc_init_data_ptr->bit_depth;
    picture_buffer_desc_ptr->stride_y         = picture_buffer_desc_init_data_ptr->max_width + picture_buffer_desc_init_data_ptr->left_padding + picture_buffer_desc_init_data_ptr->right_padding;
    picture_buffer_desc_ptr->stride_cb        = picture_buffer_desc_ptr->stride_cr = picture_buffer_desc_ptr->stride_y >> 1;
    picture_buffer_desc_ptr->origin_x         = picture_buffer_desc_init_data_ptr->left_padding;
    picture_buffer_desc_ptr->origin_y         = picture_buffer_desc_init_data_ptr->top_padding;

    picture_buffer_desc_ptr->luma_size        = (picture_buffer_desc_init_data_ptr->max_width + picture_buffer_desc_init_data_ptr->left_padding + picture_buffer_desc_init_data_ptr->right_padding) *
                                          (picture_buffer_desc_init_data_ptr->max_height + picture_buffer_desc_init_data_ptr->top_padding + picture_buffer_desc_init_data_ptr->bot_padding);
    picture_buffer_desc_ptr->chroma_size      = picture_buffer_desc_ptr->luma_size >> 2;
    picture_buffer_desc_ptr->packed_flag      = EB_FALSE;

    if(picture_buffer_desc_init_data_ptr->split_mode == EB_TRUE) {
        picture_buffer_desc_ptr->stride_bit_inc_y  = picture_buffer_desc_ptr->stride_y;
        picture_buffer_desc_ptr->stride_bit_inc_cb = picture_buffer_desc_ptr->stride_cb;
        picture_buffer_desc_ptr->stride_bit_inc_cr = picture_buffer_desc_ptr->stride_cr;
    }
    else {
        picture_buffer_desc_ptr->stride_bit_inc_y  = 0;
        picture_buffer_desc_ptr->stride_bit_inc_cb = 0;
        picture_buffer_desc_ptr->stride_bit_inc_cr = 0;
    }

    // Allocate the Picture Buffers (luma & chroma)
    if(picture_buffer_desc_init_data_ptr->buffer_enable_mask & PICTURE_BUFFER_DESC_Y_FLAG) {
        EB_ALLIGN_MALLOC(EbByte, picture_buffer_desc_ptr->buffer_y, picture_buffer_desc_ptr->luma_size      * bytesPerPixel * sizeof(uint8_t), EB_A_PTR);
        //picture_buffer_desc_ptr->buffer_y = (EbByte) EB_aligned_malloc( picture_buffer_desc_ptr->luma_size      * bytesPerPixel * sizeof(uint8_t),ALVALUE);
        picture_buffer_desc_ptr->buffer_bit_inc_y = 0;
        if(picture_buffer_desc_init_data_ptr->split_mode == EB_TRUE ) {
            EB_ALLIGN_MALLOC(EbByte, picture_buffer_desc_ptr->buffer_bit_inc_y, picture_buffer_desc_ptr->luma_size      * bytesPerPixel * sizeof(uint8_t), EB_A_PTR);
            //picture_buffer_desc_ptr->buffer_bit_inc_y = (EbByte) EB_aligned_malloc( picture_buffer_desc_ptr->luma_size      * bytesPerPixel * sizeof(uint8_t),ALVALUE);
        }
    }
    else {
        picture_buffer_desc_ptr->buffer_y   = 0;
        picture_buffer_desc_ptr->buffer_bit_inc_y = 0;
    }

    if(picture_buffer_desc_init_data_ptr->buffer_enable_mask & PICTURE_BUFFER_DESC_Cb_FLAG) {
        EB_ALLIGN_MALLOC(EbByte, picture_buffer_desc_ptr->buffer_cb, picture_buffer_desc_ptr->chroma_size    * bytesPerPixel * sizeof(uint8_t), EB_A_PTR);
        //picture_buffer_desc_ptr->buffer_cb = (EbByte) EB_aligned_malloc(picture_buffer_desc_ptr->chroma_size    * bytesPerPixel * sizeof(uint8_t),ALVALUE);
        picture_buffer_desc_ptr->buffer_bit_inc_cb = 0;
        if(picture_buffer_desc_init_data_ptr->split_mode == EB_TRUE ) {
            EB_ALLIGN_MALLOC(EbByte, picture_buffer_desc_ptr->buffer_bit_inc_cb, picture_buffer_desc_ptr->chroma_size      * bytesPerPixel * sizeof(uint8_t), EB_A_PTR);
            //picture_buffer_desc_ptr->buffer_bit_inc_cb = (EbByte) EB_aligned_malloc(picture_buffer_desc_ptr->chroma_size    * bytesPerPixel * sizeof(uint8_t),ALVALUE);
        }
    }
    else {
        picture_buffer_desc_ptr->buffer_cb   = 0;
        picture_buffer_desc_ptr->buffer_bit_inc_cb= 0;
    }

    if(picture_buffer_desc_init_data_ptr->buffer_enable_mask & PICTURE_BUFFER_DESC_Cr_FLAG) {
        EB_ALLIGN_MALLOC(EbByte, picture_buffer_desc_ptr->buffer_cr, picture_buffer_desc_ptr->chroma_size    * bytesPerPixel * sizeof(uint8_t), EB_A_PTR);
        //picture_buffer_desc_ptr->buffer_cr = (EbByte) EB_aligned_malloc(picture_buffer_desc_ptr->chroma_size    * bytesPerPixel * sizeof(uint8_t),ALVALUE);
        picture_buffer_desc_ptr->buffer_bit_inc_cr = 0;
        if(picture_buffer_desc_init_data_ptr->split_mode == EB_TRUE ) {
            EB_ALLIGN_MALLOC(EbByte, picture_buffer_desc_ptr->buffer_bit_inc_cr, picture_buffer_desc_ptr->chroma_size      * bytesPerPixel * sizeof(uint8_t), EB_A_PTR);
            //picture_buffer_desc_ptr->buffer_bit_inc_cr = (EbByte) EB_aligned_malloc(picture_buffer_desc_ptr->chroma_size    * bytesPerPixel * sizeof(uint8_t),ALVALUE);
        }
    }
    else {
        picture_buffer_desc_ptr->buffer_cr   = 0;
        picture_buffer_desc_ptr->buffer_bit_inc_cr = 0;
    }

    return EB_ErrorNone;
}

/*****************************************
 * eb_vp9_recon_picture_buffer_desc_ctor
 *  Initializes the Buffer Descriptor's
 *  values that are fixed for the life of
 *  the descriptor.
 *****************************************/
EbErrorType eb_vp9_recon_picture_buffer_desc_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    EbPictureBufferDesc          *picture_buffer_desc_ptr;
    EbPictureBufferDescInitData  *picture_buffer_desc_init_data_ptr = (EbPictureBufferDescInitData*) object_init_data_ptr;

    uint32_t bytesPerPixel = (picture_buffer_desc_init_data_ptr->bit_depth == EB_8BIT) ? 1 : 2;

    EB_MALLOC(EbPictureBufferDesc*, picture_buffer_desc_ptr, sizeof(EbPictureBufferDesc), EB_N_PTR);

    // Allocate the PictureBufferDesc Object
    *object_dbl_ptr       = (EbPtr) picture_buffer_desc_ptr;

    // Set the Picture Buffer Static variables
    picture_buffer_desc_ptr->max_width         = picture_buffer_desc_init_data_ptr->max_width;
    picture_buffer_desc_ptr->max_height        = picture_buffer_desc_init_data_ptr->max_height;
    picture_buffer_desc_ptr->width             = picture_buffer_desc_init_data_ptr->max_width;
    picture_buffer_desc_ptr->height            = picture_buffer_desc_init_data_ptr->max_height;
    picture_buffer_desc_ptr->bit_depth         = picture_buffer_desc_init_data_ptr->bit_depth;
    picture_buffer_desc_ptr->stride_y          = picture_buffer_desc_init_data_ptr->max_width + picture_buffer_desc_init_data_ptr->left_padding + picture_buffer_desc_init_data_ptr->right_padding;
    picture_buffer_desc_ptr->stride_cb         = picture_buffer_desc_ptr->stride_cr = picture_buffer_desc_ptr->stride_y >> 1;
    picture_buffer_desc_ptr->origin_x          = picture_buffer_desc_init_data_ptr->left_padding;
    picture_buffer_desc_ptr->origin_y          = picture_buffer_desc_init_data_ptr->top_padding;

    picture_buffer_desc_ptr->luma_size         = (picture_buffer_desc_init_data_ptr->max_width + picture_buffer_desc_init_data_ptr->left_padding + picture_buffer_desc_init_data_ptr->right_padding) *
                                          (picture_buffer_desc_init_data_ptr->max_height + picture_buffer_desc_init_data_ptr->top_padding + picture_buffer_desc_init_data_ptr->bot_padding);
    picture_buffer_desc_ptr->chroma_size       = picture_buffer_desc_ptr->luma_size >> 2;
    picture_buffer_desc_ptr->packed_flag       = EB_FALSE;

    picture_buffer_desc_ptr->stride_bit_inc_y  = 0;
    picture_buffer_desc_ptr->stride_bit_inc_cb = 0;
    picture_buffer_desc_ptr->stride_bit_inc_cr = 0;

    // Allocate the Picture Buffers (luma & chroma)
    if(picture_buffer_desc_init_data_ptr->buffer_enable_mask & PICTURE_BUFFER_DESC_Y_FLAG) {
        EB_CALLOC(EbByte, picture_buffer_desc_ptr->buffer_y, picture_buffer_desc_ptr->luma_size      * bytesPerPixel * sizeof(uint8_t) + (picture_buffer_desc_ptr->width + 1) * 2 * bytesPerPixel*sizeof(uint8_t), 1, EB_N_PTR);
        picture_buffer_desc_ptr->buffer_y += (picture_buffer_desc_ptr->width+1) * bytesPerPixel;
    }
    else {
        picture_buffer_desc_ptr->buffer_y   = 0;
    }

    if(picture_buffer_desc_init_data_ptr->buffer_enable_mask & PICTURE_BUFFER_DESC_Cb_FLAG) {
        EB_CALLOC(EbByte, picture_buffer_desc_ptr->buffer_cb, picture_buffer_desc_ptr->chroma_size    * bytesPerPixel * sizeof(uint8_t) + ((picture_buffer_desc_ptr->width >> 1) + 1) * 2 * bytesPerPixel*sizeof(uint8_t), 1, EB_N_PTR);
        picture_buffer_desc_ptr->buffer_cb += ((picture_buffer_desc_ptr->width >> 1) +1) * bytesPerPixel;
    }
    else {
        picture_buffer_desc_ptr->buffer_cb   = 0;
    }

    if(picture_buffer_desc_init_data_ptr->buffer_enable_mask & PICTURE_BUFFER_DESC_Cr_FLAG) {
        EB_CALLOC(EbByte, picture_buffer_desc_ptr->buffer_cr, picture_buffer_desc_ptr->chroma_size    * bytesPerPixel * sizeof(uint8_t) + ((picture_buffer_desc_ptr->width >> 1) + 1) * 2 * bytesPerPixel*sizeof(uint8_t), 1, EB_N_PTR);
        picture_buffer_desc_ptr->buffer_cr += ((picture_buffer_desc_ptr->width >> 1)+1) * bytesPerPixel;
    }
    else {
        picture_buffer_desc_ptr->buffer_cr   = 0;
    }

    return EB_ErrorNone;
}
