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

extern void eb_vp9_enc_msb_pack2_d_avx2_intrin_al(
    uint8_t    *in8_bit_buffer,
    uint32_t    in8_stride,
    uint8_t    *inn_bit_buffer,
    uint16_t   *out16_bit_buffer,
    uint32_t    inn_stride,
    uint32_t    out_stride,
    uint32_t    width,
    uint32_t    height);

extern void eb_vp9_compressed_packmsb_avx2_intrin(
    uint8_t    *in8_bit_buffer,
    uint32_t    in8_stride,
    uint8_t    *inn_bit_buffer,
    uint16_t   *out16_bit_buffer,
    uint32_t    inn_stride,
    uint32_t    out_stride,
    uint32_t    width,
    uint32_t    height);

void eb_vp9_c_pack_avx2_intrin(
    const uint8_t  *inn_bit_buffer,
    uint32_t        inn_stride,
    uint8_t        *in_compn_bit_buffer,
    uint32_t        out_stride,
    uint8_t        *local_cache,
    uint32_t        width,
    uint32_t        height);

void eb_vp9_unpack_avg_avx2_intrin(
    uint16_t  *ref16_l0,
    uint32_t   ref_l0_stride,
    uint16_t  *ref16_l1,
    uint32_t   ref_l1_stride,
    uint8_t   *dst_ptr,
    uint32_t   dst_stride,
    uint32_t   width,
    uint32_t   height);

int32_t  eb_vp9_sum_residual8bit_avx2_intrin(
    int16_t *in_ptr,
    uint32_t size,
    uint32_t stride_in );

void eb_vp9_eb_vp9_memset16bit_block_avx2_intrin (
    int16_t  *in_ptr,
    uint32_t  stride_in,
    uint32_t  size,
    int16_t   value
    );

void eb_vp9_unpack_avg_safe_sub_avx2_intrin(
    uint16_t *ref16_l0,
    uint32_t  ref_l0_stride,
    uint16_t *ref16_l1,
    uint32_t  ref_l1_stride,
    uint8_t  *dst_ptr,
    uint32_t  dst_stride,
    uint32_t  width,
    uint32_t  height);

void full_distortion_kernel_eob_zero_4x4_32bit_bt_avx2(
    int16_t   *coeff,
    uint32_t   coeff_stride,
    int16_t   *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height);

void full_distortion_kernel_eob_zero_8x8_32bit_bt_avx2(
    int16_t   *coeff,
    uint32_t   coeff_stride,
    int16_t   *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height);

void full_distortion_kernel_eob_zero16_mxn_32bit_bt_avx2(
    int16_t   *coeff,
    uint32_t   coeff_stride,
    int16_t   *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height);

void full_distortion_kernel_4x4_32bit_bt_avx2(
    int16_t   *coeff,
    uint32_t   coeff_stride,
    int16_t   *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height);

void full_distortion_kernel_8x8_32bit_bt_avx2(
    int16_t   *coeff,
    uint32_t   coeff_stride,
    int16_t   *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height);

void full_distortion_kernel_16_32_bit_bt_avx2(
    int16_t   *coeff,
    uint32_t   coeff_stride,
    int16_t   *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height);

void eb_vp9_picture_average_kernel_avx2_intrin(
    EbByte   src0,
    uint32_t src0_stride,
    EbByte   src1,
    uint32_t src1_stride,
    EbByte   dst,
    uint32_t dst_stride,
    uint32_t area_width,
    uint32_t area_height);

void eb_vp9_residual_kernel4x4_avx2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height);

void eb_vp9_residual_kernel8x8_avx2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height);

void eb_vp9_residual_kernel16x16_avx2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height);

void eb_vp9_residual_kernel32x32_avx2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height);

void eb_vp9_residual_kernel64x64_avx2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height);

#ifdef __cplusplus
}
#endif
#endif // EbPictureOperators_AVX2
