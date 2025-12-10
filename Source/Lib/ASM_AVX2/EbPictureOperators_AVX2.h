/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureOperators_AVX2
#define EbPictureOperators_AVX2

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

void full_distortion_kernel_eob_zero_4x4_32bit_bt_avx2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                                       uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                                       uint32_t area_width, uint32_t area_height);

void full_distortion_kernel_eob_zero_8x8_32bit_bt_avx2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                                       uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                                       uint32_t area_width, uint32_t area_height);

void full_distortion_kernel_eob_zero16_mxn_32bit_bt_avx2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                                         uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                                         uint32_t area_width, uint32_t area_height);

void full_distortion_kernel_4x4_32bit_bt_avx2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                              uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                              uint32_t area_width, uint32_t area_height);

void full_distortion_kernel_8x8_32bit_bt_avx2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                              uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                              uint32_t area_width, uint32_t area_height);

void full_distortion_kernel_16_32_bit_bt_avx2(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff,
                                              uint32_t recon_coeff_stride, uint64_t distortion_result[2],
                                              uint32_t area_width, uint32_t area_height);

void eb_vp9_picture_average_kernel_avx2_intrin(EbByte src0, uint32_t src0_stride, EbByte src1, uint32_t src1_stride,
                                               EbByte dst, uint32_t dst_stride, uint32_t area_width,
                                               uint32_t area_height);

void eb_vp9_residual_kernel4x4_avx2_intrin(uint8_t *input, uint32_t input_stride, uint8_t *pred, uint32_t pred_stride,
                                           int16_t *residual, uint32_t residual_stride, uint32_t area_width,
                                           uint32_t area_height);

void eb_vp9_residual_kernel8x8_avx2_intrin(uint8_t *input, uint32_t input_stride, uint8_t *pred, uint32_t pred_stride,
                                           int16_t *residual, uint32_t residual_stride, uint32_t area_width,
                                           uint32_t area_height);

void eb_vp9_residual_kernel16x16_avx2_intrin(uint8_t *input, uint32_t input_stride, uint8_t *pred, uint32_t pred_stride,
                                             int16_t *residual, uint32_t residual_stride, uint32_t area_width,
                                             uint32_t area_height);

void eb_vp9_residual_kernel32x32_avx2_intrin(uint8_t *input, uint32_t input_stride, uint8_t *pred, uint32_t pred_stride,
                                             int16_t *residual, uint32_t residual_stride, uint32_t area_width,
                                             uint32_t area_height);

void eb_vp9_residual_kernel64x64_avx2_intrin(uint8_t *input, uint32_t input_stride, uint8_t *pred, uint32_t pred_stride,
                                             int16_t *residual, uint32_t residual_stride, uint32_t area_width,
                                             uint32_t area_height);

#ifdef __cplusplus
}
#endif
#endif // EbPictureOperators_AVX2
