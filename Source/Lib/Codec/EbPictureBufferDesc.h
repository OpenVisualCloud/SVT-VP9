/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureBuffer_h
#define EbPictureBuffer_h

#include <stdio.h>

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif
#define PICTURE_BUFFER_DESC_Y_FLAG              (1 << 0)
#define PICTURE_BUFFER_DESC_Cb_FLAG             (1 << 1)
#define PICTURE_BUFFER_DESC_Cr_FLAG             (1 << 2)
#define PICTURE_BUFFER_DESC_LUMA_MASK           PICTURE_BUFFER_DESC_Y_FLAG
#define PICTURE_BUFFER_DESC_CHROMA_MASK         (PICTURE_BUFFER_DESC_Cb_FLAG | PICTURE_BUFFER_DESC_Cr_FLAG)
#define PICTURE_BUFFER_DESC_FULL_MASK           (PICTURE_BUFFER_DESC_Y_FLAG | PICTURE_BUFFER_DESC_Cb_FLAG | PICTURE_BUFFER_DESC_Cr_FLAG)

/************************************
 * EbPictureBufferDesc
 ************************************/
typedef struct EbPictureBufferDesc
{
    // Buffer Ptrs
    EbByte         buffer_y;          // pointer to the Y luma buffer
    EbByte         buffer_cb;         // pointer to the U chroma buffer
    EbByte         buffer_cr;         // pointer to the V chroma buffer

    //Bit increment
    EbByte         buffer_bit_inc_y;  // pointer to the Y luma buffer Bit increment
    EbByte         buffer_bit_inc_cb; // pointer to the U chroma buffer Bit increment
    EbByte         buffer_bit_inc_cr; // pointer to the V chroma buffer Bit increment

    uint16_t       stride_y;          // pointer to the Y luma buffer
    uint16_t       stride_cb;         // pointer to the U chroma buffer
    uint16_t       stride_cr;         // pointer to the V chroma buffer

    uint16_t       stride_bit_inc_y;  // pointer to the Y luma buffer Bit increment
    uint16_t       stride_bit_inc_cb; // pointer to the U chroma buffer Bit increment
    uint16_t       stride_bit_inc_cr; // pointer to the V chroma buffer Bit increment

    // Picture Parameters
    uint16_t       origin_x;          // Horizontal padding distance
    uint16_t       origin_y;          // Vertical padding distance
    uint16_t       width;             // Luma picture width which excludes the padding
    uint16_t       height;            // Luma picture height which excludes the padding
    uint16_t       max_width;         // Luma picture width
    uint16_t       max_height;        // Luma picture height
    EbBitDepth     bit_depth;         // Pixel Bit Depth

    // Buffer Parameters
    uint32_t       luma_size;         // Size of the luma buffer
    uint32_t       chroma_size;       // Size of the chroma buffers
    EB_BOOL        packed_flag;       // Indicates if sample buffers are packed or not

} EbPictureBufferDesc;

/************************************
 * EbPictureBufferDesc Init Data
 ************************************/
typedef struct EbPictureBufferDescInitData
{
    uint16_t   max_width;
    uint16_t   max_height;
    EbBitDepth bit_depth;
    uint32_t   buffer_enable_mask;
    uint16_t   left_padding;
    uint16_t   right_padding;
    uint16_t   top_padding;
    uint16_t   bot_padding;
    EB_BOOL    split_mode;         //ON: allocate 8bit data separately from nbit data

} EbPictureBufferDescInitData;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType eb_vp9_picture_buffer_desc_ctor(
    EbPtr  *object_dbl_ptr,
    EbPtr   object_init_data_ptr);

extern EbErrorType eb_vp9_recon_picture_buffer_desc_ctor(
    EbPtr  *object_dbl_ptr,
    EbPtr   object_init_data_ptr);

#ifdef __cplusplus
}
#endif
#endif // EbPictureBuffer_h
