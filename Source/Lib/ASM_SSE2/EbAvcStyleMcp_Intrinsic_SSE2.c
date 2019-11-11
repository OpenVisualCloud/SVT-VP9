/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbAvcStyleMcp_SSE2.h"
#include "EbMcp_SSE2.h" // THIS SHOULD BE _SSE2 in the future

void eb_vp9_avc_style_copy_sse2(
    EbByte   ref_pic,
    uint32_t src_stride,
    EbByte   dst,
    uint32_t dst_stride,
    uint32_t pu_width,
    uint32_t pu_height,
    EbByte   temp_buf,
    uint32_t frac_pos)
{
    (void)temp_buf;
    (void)frac_pos;

    eb_vp9_picture_copy_kernel_sse2(ref_pic, src_stride, dst, dst_stride, pu_width, pu_height);
}
