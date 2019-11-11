/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbAvcStyleMcp_C.h"
#include "EbPictureOperators_C.h"
#include "EbDefinitions.h"
#include "EbUtility.h"

static const int8_t avc_style_luma_if_coeff[4][4] = {
        { 0, 0, 0, 0 },
        { -1, 25, 9, -1 },
        { -2, 18, 18, -2 },
        { -1, 9, 25, -1 },
};

void avc_style_copy_new(
    EbByte    ref_pic,
    uint32_t  src_stride,
    EbByte    dst,
    uint32_t  dst_stride,
    uint32_t  pu_width,
    uint32_t  pu_height,
    EbByte    temp_buf,
    uint32_t  frac_pos)
{
    (void)temp_buf;
    (void)frac_pos;
    eb_vp9_picture_copy_kernel(ref_pic, src_stride, dst, dst_stride, pu_width, pu_height, 1);
}

void eb_vp9_avc_style_luma_interpolation_filter_horizontal(
    EbByte    ref_pic,
    uint32_t  src_stride,
    EbByte    dst,
    uint32_t  dst_stride,
    uint32_t  pu_width,
    uint32_t  pu_height,
    EbByte    temp_buf,
    uint32_t  frac_pos)
{
    const int8_t  *if_coeff = avc_style_luma_if_coeff[frac_pos];
    const int32_t  if_init_pos_offset = -1;
    const uint8_t   if_shift = 5;
    const int16_t  if_offset = POW2(if_shift - 1);
    uint32_t        x, y;
    (void)temp_buf;

    ref_pic += if_init_pos_offset;
    for (y = 0; y < pu_height; y++) {
        for (x = 0; x < pu_width; x++) {
            dst[x] = (uint8_t)CLIP3(0, MAX_SAMPLE_VALUE,
                (ref_pic[x] * if_coeff[0] + ref_pic[x + 1] * if_coeff[1] + ref_pic[x + 2] * if_coeff[2] + ref_pic[x + 3] * if_coeff[3] + if_offset) >> if_shift);
        }
        ref_pic += src_stride;
        dst += dst_stride;
    }
}

void eb_vp9_avc_style_luma_interpolation_filter_vertical(
    EbByte    ref_pic,
    uint32_t  src_stride,
    EbByte    dst,
    uint32_t  dst_stride,
    uint32_t  pu_width,
    uint32_t  pu_height,
    EbByte    temp_buf,
    uint32_t  frac_pos)
{
    const int8_t  *if_coeff = avc_style_luma_if_coeff[frac_pos];
    const int32_t  if_stride = src_stride;
    const int32_t  if_init_pos_offset = -(int32_t)src_stride;
    const uint8_t   if_shift = 5;
    const int32_t  if_offset = POW2(if_shift - 1);
    uint32_t        x, y;
    (void)temp_buf;

    ref_pic += if_init_pos_offset;
    for (y = 0; y < pu_height; y++) {
        for (x = 0; x < pu_width; x++) {
            dst[x] = (uint8_t)CLIP3(0, MAX_SAMPLE_VALUE,
                (ref_pic[x] * if_coeff[0] + ref_pic[x + if_stride] * if_coeff[1] + ref_pic[x + 2 * if_stride] * if_coeff[2] + ref_pic[x + 3 * if_stride] * if_coeff[3] + if_offset) >> if_shift);
        }
        ref_pic += src_stride;
        dst += dst_stride;
    }
}
