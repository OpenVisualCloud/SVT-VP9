/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBAVCSTYLEMCP_C_H
#define EBAVCSTYLEMCP_C_H

#include "EbDefinitions.h"
#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

void avc_style_copy_new(
    EbByte   ref_pic,
    uint32_t src_stride,
    EbByte   dst,
    uint32_t dst_stride,
    uint32_t pu_width,
    uint32_t pu_height,
    EbByte   temp_buf,
    uint32_t frac_pos);

void eb_vp9_avc_style_luma_interpolation_filter_horizontal(
    EbByte   ref_pic,
    uint32_t src_stride,
    EbByte   dst,
    uint32_t dst_stride,
    uint32_t pu_width,
    uint32_t pu_height,
    EbByte   temp_buf,
    uint32_t frac_pos);

void eb_vp9_avc_style_luma_interpolation_filter_vertical(
    EbByte   ref_pic,
    uint32_t src_stride,
    EbByte   dst,
    uint32_t dst_stride,
    uint32_t pu_width,
    uint32_t pu_height,
    EbByte   temp_buf,
    uint32_t frac_pos);

#ifdef __cplusplus
}
#endif
#endif //EBAVCSTYLEMCP_C_H
