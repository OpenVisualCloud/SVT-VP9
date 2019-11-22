/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBAVCSTYLEMCP_H
#define EBAVCSTYLEMCP_H

#include "EbAvcStyleMcp_C.h"

#include "EbAvcStyleMcp_AVX2.h"
#include "EbAvcStyleMcp_SSE2.h"
#include "EbAvcStyleMcp_SSSE3.h"

#include "EbPictureOperators.h"

#include "EbMcp.h"

#include "EbDefinitions.h"
#include "EbPictureBufferDesc.h"
#include "EbPictureControlSet.h"

#include "EbCombinedAveragingSAD_Intrinsic_AVX2.h"
#include "EbCombinedAveragingSAD_Intrinsic_AVX512.h"
#ifdef __cplusplus
extern "C" {
#endif

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
    EbByte               temp_buf);

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
    EbByte               first_pass_if_temp_dst);

typedef void(*AvcStyleInterpolationFilterNew)(
    EbByte   ref_pic,
    uint32_t src_stride,
    EbByte   dst,
    uint32_t dst_stride,
    uint32_t pu_width,
    uint32_t pu_height,
    EbByte   temp_buf,
    uint32_t frac_pos);

typedef void(*PictureAverage)(
    EbByte   src0,
    uint32_t src0_stride,
    EbByte   src1,
    uint32_t src1_stride,
    EbByte   dst,
    uint32_t dst_stride,
    uint32_t area_width,
    uint32_t area_height);

/***************************************
* Function Tables
***************************************/
static const AvcStyleInterpolationFilterNew FUNC_TABLE avc_style_uni_pred_luma_if_function_ptr_array[ASM_TYPE_TOTAL][3] = {
    // C_DEFAULT
    {
        avc_style_copy_new,                                     //copy
        eb_vp9_avc_style_luma_interpolation_filter_horizontal,           //a
        eb_vp9_avc_style_luma_interpolation_filter_vertical,             //d
    },
    // AVX2
    {
        eb_vp9_avc_style_copy_sse2,                                         //copy
        eb_vp9_avc_style_luma_interpolation_filter_horizontal_avx2_intrin,     //a
        eb_vp9_avc_style_luma_interpolation_filter_vertical_avx2_intrin,       //d

    }
};

static const PictureAverage FUNC_TABLE picture_average_array[ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    eb_vp9_picture_average_kernel,
    // AVX2
    eb_vp9_picture_average_kernel_avx2_intrin,
};

typedef void(*PictureAverage1Line)(
    EbByte   src0,
    EbByte   src1,
    EbByte   dst,
    uint32_t area_width);

#ifdef __cplusplus
}
#endif
#endif //EBAVCSTYLEMCP_H
