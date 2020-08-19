/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbPictureBufferDesc.h"
#include "EbReferenceObject.h"

static void initialize_samples_neighboring_reference_picture16_bit(
    EbByte   recon_samples_buffer_ptr,
    uint16_t stride,
    uint16_t recon_width,
    uint16_t recon_height,
    uint16_t left_padding,
    uint16_t top_padding) {

    uint16_t  *recon_samples_ptr;
    uint16_t   sample_count;

    // 1. Zero out the top row
    recon_samples_ptr = (uint16_t*)recon_samples_buffer_ptr + (top_padding - 1) * stride + left_padding - 1;
    EB_MEMSET((uint8_t*)recon_samples_ptr, 0, sizeof(uint16_t)*(1 + recon_width + 1));

    // 2. Zero out the bottom row
    recon_samples_ptr = (uint16_t*)recon_samples_buffer_ptr + (top_padding + recon_height) * stride + left_padding - 1;
    EB_MEMSET((uint8_t*)recon_samples_ptr, 0, sizeof(uint16_t)*(1 + recon_width + 1));

    // 3. Zero out the left column
    recon_samples_ptr = (uint16_t*)recon_samples_buffer_ptr + top_padding * stride + left_padding - 1;
    for (sample_count = 0; sample_count < recon_height; sample_count++) {
        recon_samples_ptr[sample_count * stride] = 0;
    }

    // 4. Zero out the right column
    recon_samples_ptr = (uint16_t*)recon_samples_buffer_ptr + top_padding * stride + left_padding + recon_width;
    for (sample_count = 0; sample_count < recon_height; sample_count++) {
        recon_samples_ptr[sample_count * stride] = 0;
    }
}

static void initialize_samples_neighboring_reference_picture8_bit(
    EbByte   recon_samples_buffer_ptr,
    uint16_t stride,
    uint16_t recon_width,
    uint16_t recon_height,
    uint16_t left_padding,
    uint16_t top_padding) {

    uint8_t *recon_samples_ptr;
    uint16_t sample_count;

    // 1. Zero out the top row
    recon_samples_ptr = recon_samples_buffer_ptr + (top_padding - 1) * stride + left_padding - 1;
    EB_MEMSET(recon_samples_ptr, 0, sizeof(uint8_t)*(1 + recon_width + 1));

    // 2. Zero out the bottom row
    recon_samples_ptr = recon_samples_buffer_ptr + (top_padding + recon_height) * stride + left_padding - 1;
    EB_MEMSET(recon_samples_ptr, 0, sizeof(uint8_t)*(1 + recon_width + 1));

    // 3. Zero out the left column
    recon_samples_ptr = recon_samples_buffer_ptr + top_padding * stride + left_padding - 1;
    for (sample_count = 0; sample_count < recon_height; sample_count++) {
        recon_samples_ptr[sample_count * stride] = 0;
    }

    // 4. Zero out the right column
    recon_samples_ptr = recon_samples_buffer_ptr + top_padding * stride + left_padding + recon_width;
    for (sample_count = 0; sample_count < recon_height; sample_count++) {
        recon_samples_ptr[sample_count * stride] = 0;
    }
}

static void initialize_samples_neighboring_reference_picture(
    EbReferenceObject           *reference_object,
    EbPictureBufferDescInitData *picture_buffer_desc_init_data_ptr,
    EbBitDepth                   bit_depth) {

    if (bit_depth == EB_10BIT){

        initialize_samples_neighboring_reference_picture16_bit(
            reference_object->reference_picture16bit->buffer_y,
            reference_object->reference_picture16bit->stride_y,
            reference_object->reference_picture16bit->width,
            reference_object->reference_picture16bit->height,
            picture_buffer_desc_init_data_ptr->left_padding,
            picture_buffer_desc_init_data_ptr->top_padding);

        initialize_samples_neighboring_reference_picture16_bit(
            reference_object->reference_picture16bit->buffer_cb,
            reference_object->reference_picture16bit->stride_cb,
            reference_object->reference_picture16bit->width >> 1,
            reference_object->reference_picture16bit->height >> 1,
            picture_buffer_desc_init_data_ptr->left_padding >> 1,
            picture_buffer_desc_init_data_ptr->top_padding >> 1);

        initialize_samples_neighboring_reference_picture16_bit(
            reference_object->reference_picture16bit->buffer_cr,
            reference_object->reference_picture16bit->stride_cr,
            reference_object->reference_picture16bit->width >> 1,
            reference_object->reference_picture16bit->height >> 1,
            picture_buffer_desc_init_data_ptr->left_padding >> 1,
            picture_buffer_desc_init_data_ptr->top_padding >> 1);
    }
    else {

        initialize_samples_neighboring_reference_picture8_bit(
            reference_object->reference_picture->buffer_y,
            reference_object->reference_picture->stride_y,
            reference_object->reference_picture->width,
            reference_object->reference_picture->height,
            picture_buffer_desc_init_data_ptr->left_padding,
            picture_buffer_desc_init_data_ptr->top_padding);

        initialize_samples_neighboring_reference_picture8_bit(
            reference_object->reference_picture->buffer_cb,
            reference_object->reference_picture->stride_cb,
            reference_object->reference_picture->width >> 1,
            reference_object->reference_picture->height >> 1,
            picture_buffer_desc_init_data_ptr->left_padding >> 1,
            picture_buffer_desc_init_data_ptr->top_padding >> 1);

        initialize_samples_neighboring_reference_picture8_bit(
            reference_object->reference_picture->buffer_cr,
            reference_object->reference_picture->stride_cr,
            reference_object->reference_picture->width >> 1,
            reference_object->reference_picture->height >> 1,
            picture_buffer_desc_init_data_ptr->left_padding >> 1,
            picture_buffer_desc_init_data_ptr->top_padding >> 1);
    }
}

/*****************************************
 * eb_vp9_picture_buffer_desc_ctor
 *  Initializes the Buffer Descriptor's
 *  values that are fixed for the life of
 *  the descriptor.
 *****************************************/
EbErrorType eb_vp9_reference_object_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{

    EbReferenceObject             *reference_object;
    EbPictureBufferDescInitData   *picture_buffer_desc_init_data_ptr = (EbPictureBufferDescInitData*)  object_init_data_ptr;
    EbPictureBufferDescInitData    picture_buffer_desc_init_data16_bit_ptr = *picture_buffer_desc_init_data_ptr;

    EbErrorType return_error = EB_ErrorNone;
    EB_MALLOC(EbReferenceObject  *, reference_object, sizeof(EbReferenceObject  ), EB_N_PTR);

    *object_dbl_ptr = (EbPtr) reference_object;

    if (picture_buffer_desc_init_data16_bit_ptr.bit_depth == EB_10BIT){

        return_error = eb_vp9_picture_buffer_desc_ctor(
            (EbPtr *)&(reference_object->reference_picture16bit),
            (EbPtr)&picture_buffer_desc_init_data16_bit_ptr);

        initialize_samples_neighboring_reference_picture(
            reference_object,
            &picture_buffer_desc_init_data16_bit_ptr,
            picture_buffer_desc_init_data16_bit_ptr.bit_depth);

    }
    else{

        return_error = eb_vp9_picture_buffer_desc_ctor(
            (EbPtr *)&(reference_object->reference_picture),
            (EbPtr)picture_buffer_desc_init_data_ptr);

        initialize_samples_neighboring_reference_picture(
            reference_object,
            picture_buffer_desc_init_data_ptr,
            picture_buffer_desc_init_data16_bit_ptr.bit_depth);
    }

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    //RESTRICT THIS TO M4
    {
        EbPictureBufferDescInitData buf_desc;

        buf_desc.max_width = picture_buffer_desc_init_data_ptr->max_width;
        buf_desc.max_height = picture_buffer_desc_init_data_ptr->max_height;
        buf_desc.bit_depth = EB_8BIT;
        buf_desc.buffer_enable_mask = PICTURE_BUFFER_DESC_FULL_MASK;
        buf_desc.left_padding  = picture_buffer_desc_init_data_ptr->left_padding;
        buf_desc.right_padding = picture_buffer_desc_init_data_ptr->right_padding;
        buf_desc.top_padding   = picture_buffer_desc_init_data_ptr->top_padding;
        buf_desc.bot_padding   = picture_buffer_desc_init_data_ptr->bot_padding;
        buf_desc.split_mode    = 0;

        return_error = eb_vp9_picture_buffer_desc_ctor((EbPtr *)&(reference_object->ref_den_src_picture),
                                                (EbPtr)&buf_desc);
        if (return_error == EB_ErrorInsufficientResources)
            return EB_ErrorInsufficientResources;
    }

    return EB_ErrorNone;
}

/*****************************************
 * eb_vp9_pa_reference_object_ctor
 *  Initializes the Buffer Descriptor's
 *  values that are fixed for the life of
 *  the descriptor.
 *****************************************/
EbErrorType eb_vp9_pa_reference_object_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{

    EbPaReferenceObject         *pa_reference_object;
    EbPictureBufferDescInitData *picture_buffer_desc_init_data_ptr   = (EbPictureBufferDescInitData*) object_init_data_ptr;
    EbErrorType return_error                                           = EB_ErrorNone;
    EB_MALLOC(EbPaReferenceObject*, pa_reference_object, sizeof(EbPaReferenceObject  ), EB_N_PTR);
    *object_dbl_ptr = (EbPtr) pa_reference_object;

    // Reference picture constructor
    return_error = eb_vp9_picture_buffer_desc_ctor(
        (EbPtr *) &(pa_reference_object->input_padded_picture_ptr),
        (EbPtr  )   picture_buffer_desc_init_data_ptr);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    // Quarter Decim reference picture constructor
    pa_reference_object->quarter_decimated_picture_ptr = (EbPictureBufferDesc  *)EB_NULL;
        return_error = eb_vp9_picture_buffer_desc_ctor(
            (EbPtr *) &(pa_reference_object->quarter_decimated_picture_ptr),
            (EbPtr  )  (picture_buffer_desc_init_data_ptr + 1));
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

    // Sixteenth Decim reference picture constructor
    pa_reference_object->sixteenth_decimated_picture_ptr = (EbPictureBufferDesc  *)EB_NULL;
        return_error = eb_vp9_picture_buffer_desc_ctor(
            (EbPtr *) &(pa_reference_object->sixteenth_decimated_picture_ptr),
            (EbPtr  )  (picture_buffer_desc_init_data_ptr + 2));
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

    return EB_ErrorNone;
}
