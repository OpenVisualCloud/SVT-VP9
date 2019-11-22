/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbAvcStyleMcp.h"
#include "EbPictureOperators.h"

void unpack_uni_pred_ref10_bit(
    EbPictureBufferDesc *ref_frame_pic_list0,
    uint32_t             pos_x,
    uint32_t             pos_y,
    uint32_t             pu_width,
    uint32_t             pu_height,
    EbPictureBufferDesc *dst,
    uint32_t             dst_luma_index,
    uint32_t             dst_chroma_index,          //input parameter, please refer to the detailed explanation above.
    uint32_t             component_mask,
    EbByte               temp_buf)
{
    uint32_t   chroma_pu_width      = pu_width >> 1;
    uint32_t   chroma_pu_height     = pu_height >> 1;

    //doing the luma interpolation
    if (component_mask & PICTURE_BUFFER_DESC_LUMA_MASK)
    {
        (void) temp_buf;
        uint32_t in_posx = (pos_x >> 2);
        uint32_t in_posy = (pos_y >> 2);
        uint16_t *ptr16    =  (uint16_t *)ref_frame_pic_list0->buffer_y  + in_posx + in_posy*ref_frame_pic_list0->stride_y;

          eb_vp9_extract8_bitdata_safe_sub(
            ptr16,
            ref_frame_pic_list0->stride_y,
            dst->buffer_y + dst_luma_index,
            dst->stride_y,
            pu_width,
            pu_height
            );

    }

    //chroma
    if (component_mask & PICTURE_BUFFER_DESC_CHROMA_MASK) {

        {
        uint32_t in_posx = (pos_x >> 3);
        uint32_t in_posy = (pos_y >> 3);
        uint16_t *ptr16    =  (uint16_t *)ref_frame_pic_list0->buffer_cb  + in_posx + in_posy*ref_frame_pic_list0->stride_cb;

          eb_vp9_extract_8bit_data(
            ptr16,
            ref_frame_pic_list0->stride_cb,
            dst->buffer_cb + dst_chroma_index,
            dst->stride_cb,
            chroma_pu_width,
            chroma_pu_height
            );

        ptr16    =  (uint16_t *)ref_frame_pic_list0->buffer_cr  + in_posx + in_posy*ref_frame_pic_list0->stride_cr;

        eb_vp9_extract_8bit_data(
            ptr16,
            ref_frame_pic_list0->stride_cr,
            dst->buffer_cr + dst_chroma_index,
            dst->stride_cr,
            chroma_pu_width,
            chroma_pu_height
            );

        }
    }

}

void unpack_bi_pred_ref10_bit(
    EbPictureBufferDesc *ref_frame_pic_list0,
    EbPictureBufferDesc *ref_frame_pic_list1,
    uint32_t             ref_list0_pos_x,
    uint32_t             ref_list0_pos_y,
    uint32_t             ref_list1_pos_x,
    uint32_t             ref_list1_pos_y,
    uint32_t             pu_width,
    uint32_t             pu_height,
    EbPictureBufferDesc *bi_dst,
    uint32_t             dst_luma_index,
    uint32_t             dst_chroma_index,
    uint32_t             component_mask,
    EbByte               ref_list0_temp_dst,
    EbByte               ref_list1_temp_dst,
    EbByte               first_pass_if_temp_dst)
{
    uint32_t   chroma_pu_width      = pu_width >> 1;
    uint32_t   chroma_pu_height     = pu_height >> 1;

    //Luma
    if (component_mask & PICTURE_BUFFER_DESC_LUMA_MASK) {

        (void) first_pass_if_temp_dst;
        (void) ref_list0_temp_dst;
        (void) ref_list1_temp_dst;
       eb_vp9_unpack_l0l1_avg_safe_sub(
            (uint16_t *)ref_frame_pic_list0->buffer_y  + (ref_list0_pos_x >> 2) + (ref_list0_pos_y >> 2)*ref_frame_pic_list0->stride_y,
            ref_frame_pic_list0->stride_y,
            (uint16_t *)ref_frame_pic_list1->buffer_y  + (ref_list1_pos_x >> 2) + (ref_list1_pos_y >> 2)*ref_frame_pic_list1->stride_y,
            ref_frame_pic_list1->stride_y,
            bi_dst->buffer_y + dst_luma_index,
            bi_dst->stride_y,
            pu_width,
            pu_height
            );

    }

    //uni-prediction List0 chroma
    if (component_mask & PICTURE_BUFFER_DESC_CHROMA_MASK) {

         eb_vp9_unpack_l0l1_avg(
            (uint16_t *)ref_frame_pic_list0->buffer_cb  + (ref_list0_pos_x >> 3) + (ref_list0_pos_y >> 3)*ref_frame_pic_list0->stride_cb,
            ref_frame_pic_list0->stride_cb,
            (uint16_t *)ref_frame_pic_list1->buffer_cb  + (ref_list1_pos_x >> 3) + (ref_list1_pos_y >> 3)*ref_frame_pic_list1->stride_cb,
            ref_frame_pic_list1->stride_cb,
            bi_dst->buffer_cb + dst_chroma_index,
            bi_dst->stride_cb ,
            chroma_pu_width,
            chroma_pu_height
            );

        eb_vp9_unpack_l0l1_avg(
            (uint16_t *)ref_frame_pic_list0->buffer_cr  + (ref_list0_pos_x >> 3) + (ref_list0_pos_y >> 3)*ref_frame_pic_list0->stride_cr,
            ref_frame_pic_list0->stride_cr,
            (uint16_t *)ref_frame_pic_list1->buffer_cr  + (ref_list1_pos_x >> 3) + (ref_list1_pos_y >> 3)*ref_frame_pic_list1->stride_cr,
            ref_frame_pic_list1->stride_cr,
            bi_dst->buffer_cr + dst_chroma_index,
            bi_dst->stride_cr,
            chroma_pu_width,
            chroma_pu_height
            );

     }
}

void bi_pred_average_kernel_c(
    EbByte   src0,
    uint32_t src0_stride,
    EbByte   src1,
    uint32_t src1_stride,
    EbByte   dst,
    uint32_t dst_stride,
    uint32_t area_width,
    uint32_t area_height
    )
{

    uint32_t i, j;

    for (j = 0; j < area_height; j++)
    {
        for (i = 0; i < area_width; i++)
        {
            dst[i + j * dst_stride] = (src0[i + j * src0_stride] + src1[i + j * src1_stride] + 1) / 2;
        }
    }

}
