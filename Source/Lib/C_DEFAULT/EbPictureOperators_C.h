/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureOperators_C_h
#define EbPictureOperators_C_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

void eb_vp9_picture_average_kernel(EbByte src0, uint32_t src0_stride, EbByte src1, uint32_t src1_stride, EbByte dst,
                                   uint32_t dst_stride, uint32_t area_width, uint32_t area_height);

void eb_vp9_picture_copy_kernel(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride, uint32_t area_width,
                                uint32_t area_height, uint32_t bytes_per_sample);

void copy_kernel8_bit(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride, uint32_t area_width,
                      uint32_t area_height);

void eb_vp9_residual_kernel(uint8_t *input, uint32_t input_stride, uint8_t *pred, uint32_t pred_stride,
                            int16_t *residual, uint32_t residual_stride, uint32_t area_width, uint32_t area_height);

void full_distortion_kernel32bit(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                 uint32_t recon_coeff_stride, uint64_t distortion_result[2], uint32_t area_width,
                                 uint32_t area_height);

void full_distortion_kernel_eob_zero32bit(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                          uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                          uint32_t area_width, uint32_t area_height);

void full_distortion_kernel_intra32bit(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                       uint32_t recon_coeff_stride, uint64_t distortion_result[2], uint32_t area_width,
                                       uint32_t area_height);

uint64_t eb_vp9_spatial_full_distortion_kernel(uint8_t *input, uint32_t input_stride, uint8_t *recon,
                                               uint32_t recon_stride, uint32_t area_width, uint32_t area_height);

#ifdef __cplusplus
}
#endif
#endif // EbPictureOperators_C_h
