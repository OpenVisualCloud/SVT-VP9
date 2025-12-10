/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureOperators_SSE2_h
#define EbPictureOperators_SSE2_h

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

void full_distortion_kernel_8x8_32bit_bt_sse2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                              uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                              uint32_t area_width, uint32_t area_height);

void full_distortion_kernel_intra_16_mxn_32_bit_bt_sse2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                                        uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                                        uint32_t area_width, uint32_t area_height);

void eb_vp9_picture_copy_kernel4x4_sse_intrin(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride,
                                              uint32_t area_width, uint32_t area_height);

void eb_vp9_picture_copy_kernel8x8_sse2_intrin(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride,
                                               uint32_t area_width, uint32_t area_height);

void eb_vp9_picture_copy_kernel16x16_sse2_intrin(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride,
                                                 uint32_t area_width, uint32_t area_height);

void eb_vp9_picture_copy_kernel32x32_sse2_intrin(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride,
                                                 uint32_t area_width, uint32_t area_height);

void eb_vp9_picture_copy_kernel64x64_sse2_intrin(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride,
                                                 uint32_t area_width, uint32_t area_height);

#ifdef __cplusplus
}
#endif
#endif // EbPictureOperators_SSE2_h
