/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureOperators_h
#define EbPictureOperators_h

#include "EbPictureOperators_C.h"
#include "EbPictureOperators_SSE2.h"
#include "EbPictureOperators_SSE4_1.h"
#include "EbPictureOperators_AVX2.h"
#include "EbDefinitions.h"
#include "EbPictureBufferDesc.h"
#include "EbSequenceControlSet.h"
#ifdef __cplusplus
extern "C" {
#endif

void eb_vp9_memset16bit(uint16_t *in_ptr, uint16_t value, uint64_t num_of_elements);

static void pic_copy_void_func() {}
static void pic_resd_void_func() {}
static void full_distortion_void_func() {}

typedef int32_t (*EbSumRes)(int16_t *in_ptr, uint32_t size, uint32_t stride_in);

typedef void (*EbMemset16BitBlk)(int16_t *in_ptr, uint32_t stride_in, uint32_t size, int16_t value);

/***************************************
* Function Types
***************************************/
typedef void (*EbAddKernelType16Bit)(uint16_t *pred_ptr, uint32_t pred_stride, int16_t *residual_ptr,
                                     uint32_t residual_stride, uint16_t *recon_ptr, uint32_t recon_stride,
                                     uint32_t width, uint32_t height);

typedef void (*EbPicCopyType)(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride, uint32_t area_width,
                              uint32_t area_height);

typedef void (*EbResdKernelType)(uint8_t *input, uint32_t input_stride, uint8_t *pred, uint32_t pred_stride,
                                 int16_t *residual, uint32_t residual_stride, uint32_t area_width,
                                 uint32_t area_height);

typedef void (*EbZeroCoeffType)(int16_t *coeff_buffer, uint32_t coeff_stride, uint32_t coeff_origin_index,
                                uint32_t area_width, uint32_t area_height);

typedef void (*EbFullDistType)(int16_t *coeff, uint32_t coeff_stride, int16_t *recon_coeff, uint32_t recon_coeff_stride,
                               uint64_t distortion_result[DIST_CALC_TOTAL], uint32_t area_width, uint32_t area_height);

/***************************************
* Function Tables
***************************************/

static EbPicCopyType FUNC_TABLE pic_copy_kernel_func_ptr_array[ASM_TYPE_TOTAL][9] = {
    // C_DEFAULT
    {/*0 4x4   */ copy_kernel8_bit,
     /*1 8x8   */ copy_kernel8_bit,
     /*2 16x16 */ copy_kernel8_bit,
     /*3       */ (EbPicCopyType)pic_copy_void_func,
     /*4 32x32 */ copy_kernel8_bit,
     /*5       */ (EbPicCopyType)pic_copy_void_func,
     /*6       */ (EbPicCopyType)pic_copy_void_func,
     /*7       */ (EbPicCopyType)pic_copy_void_func,
     /*8 64x64 */ copy_kernel8_bit},
    // AVX2
    {
        /*0 4x4   */ eb_vp9_picture_copy_kernel4x4_sse_intrin,
        /*1 8x8   */ eb_vp9_picture_copy_kernel8x8_sse2_intrin,
        /*2 16x16 */ eb_vp9_picture_copy_kernel16x16_sse2_intrin,
        /*3       */ (EbPicCopyType)pic_copy_void_func,
        /*4 32x32 */ eb_vp9_picture_copy_kernel32x32_sse2_intrin,
        /*5       */ (EbPicCopyType)pic_copy_void_func,
        /*6       */ (EbPicCopyType)pic_copy_void_func,
        /*7       */ (EbPicCopyType)pic_copy_void_func,
        /*8 64x64 */ eb_vp9_picture_copy_kernel64x64_sse2_intrin,
    },
};

static EbResdKernelType FUNC_TABLE eb_vp9_residual_kernel_func_ptr_array[ASM_TYPE_TOTAL][9] = {
    // C_DEFAULT
    {/*0 4x4  */ eb_vp9_residual_kernel,
     /*1 8x8  */ eb_vp9_residual_kernel,
     /*2 16x16 */ eb_vp9_residual_kernel,
     /*3  */ (EbResdKernelType)pic_resd_void_func,
     /*4 32x32 */ eb_vp9_residual_kernel,
     /*5      */ (EbResdKernelType)pic_resd_void_func,
     /*6  */ (EbResdKernelType)pic_resd_void_func,
     /*7      */ (EbResdKernelType)pic_resd_void_func,
     /*8 64x64 */ eb_vp9_residual_kernel},
    // AVX2
    {
        /*0 4x4  */ eb_vp9_residual_kernel4x4_avx2_intrin,
        /*1 8x8  */ eb_vp9_residual_kernel8x8_avx2_intrin,
        /*2 16x16 */ eb_vp9_residual_kernel16x16_avx2_intrin,
        /*3  */ (EbResdKernelType)pic_resd_void_func,
        /*4 32x32 */ eb_vp9_residual_kernel32x32_avx2_intrin,
        /*5      */ (EbResdKernelType)pic_resd_void_func,
        /*6  */ (EbResdKernelType)pic_resd_void_func,
        /*7      */ (EbResdKernelType)pic_resd_void_func,
        /*8 64x64 */ eb_vp9_residual_kernel64x64_avx2_intrin,
    },
};

static EbFullDistType FUNC_TABLE full_distortion_intrinsic_func_ptr_array[ASM_TYPE_TOTAL][2][2][9] = {
    // C_DEFAULT
    // It was found that the SSE2 intrinsic code is much faster (~2x) than the SSE4.1 code
    {{{
          /*0 4x4   */ full_distortion_kernel_eob_zero32bit,
          /*1 8x8   */ full_distortion_kernel_eob_zero32bit,
          /*2 16x16 */ full_distortion_kernel_eob_zero32bit,
          /*3       */ (EbFullDistType)full_distortion_void_func,
          /*4 32x32 */ full_distortion_kernel_eob_zero32bit,
          /*5       */ (EbFullDistType)full_distortion_void_func,
          /*6       */ (EbFullDistType)full_distortion_void_func,
          /*7       */ (EbFullDistType)full_distortion_void_func,
          /*8 64x64 */ full_distortion_kernel_eob_zero32bit,
      },
      {
          /*0 4x4   */ full_distortion_kernel_eob_zero32bit,
          /*1 8x8   */ full_distortion_kernel_eob_zero32bit,
          /*2 16x16 */ full_distortion_kernel_eob_zero32bit,
          /*3       */ (EbFullDistType)full_distortion_void_func,
          /*4 32x32 */ full_distortion_kernel_eob_zero32bit,
          /*5       */ (EbFullDistType)full_distortion_void_func,
          /*6       */ (EbFullDistType)full_distortion_void_func,
          /*7       */ (EbFullDistType)full_distortion_void_func,
          /*8 64x64 */ full_distortion_kernel_eob_zero32bit,
      }},
     {{
          /*0 4x4   */ full_distortion_kernel32bit,
          /*1 8x8   */ full_distortion_kernel32bit,
          /*2 16x16 */ full_distortion_kernel32bit,
          /*3       */ (EbFullDistType)full_distortion_void_func,
          /*4 32x32 */ full_distortion_kernel32bit,
          /*5       */ (EbFullDistType)full_distortion_void_func,
          /*6       */ (EbFullDistType)full_distortion_void_func,
          /*7       */ (EbFullDistType)full_distortion_void_func,
          /*8 64x64 */ full_distortion_kernel32bit,
      },
      {
          /*0 4x4   */ full_distortion_kernel_intra32bit,
          /*1 8x8   */ full_distortion_kernel_intra32bit,
          /*2 16x16 */ full_distortion_kernel_intra32bit,
          /*3       */ (EbFullDistType)full_distortion_void_func,
          /*4 32x32 */ full_distortion_kernel_intra32bit,
          /*5       */ (EbFullDistType)full_distortion_void_func,
          /*6       */ (EbFullDistType)full_distortion_void_func,
          /*7       */ (EbFullDistType)full_distortion_void_func,
          /*8 64x64 */ full_distortion_kernel_intra32bit,
      }

     }},
    {{{
          /*0 4x4   */ full_distortion_kernel_eob_zero_4x4_32bit_bt_avx2,
          /*1 8x8   */ full_distortion_kernel_eob_zero_8x8_32bit_bt_avx2,
          /*2 16x16 */ full_distortion_kernel_eob_zero16_mxn_32bit_bt_avx2,
          /*3       */ (EbFullDistType)full_distortion_void_func,
          /*4 32x32 */ full_distortion_kernel_eob_zero16_mxn_32bit_bt_avx2,
          /*5       */ (EbFullDistType)full_distortion_void_func,
          /*6       */ (EbFullDistType)full_distortion_void_func,
          /*7       */ (EbFullDistType)full_distortion_void_func,
          /*8 64x64 */ full_distortion_kernel_eob_zero16_mxn_32bit_bt_avx2,
      },
      {
          /*0 4x4   */ full_distortion_kernel_eob_zero_4x4_32bit_bt_avx2,
          /*1 8x8   */ full_distortion_kernel_eob_zero_8x8_32bit_bt_avx2,
          /*2 16x16 */ full_distortion_kernel_eob_zero16_mxn_32bit_bt_avx2,
          /*3       */ (EbFullDistType)full_distortion_void_func,
          /*4 32x32 */ full_distortion_kernel_eob_zero16_mxn_32bit_bt_avx2,
          /*5       */ (EbFullDistType)full_distortion_void_func,
          /*6       */ (EbFullDistType)full_distortion_void_func,
          /*7       */ (EbFullDistType)full_distortion_void_func,
          /*8 64x64 */ full_distortion_kernel_eob_zero16_mxn_32bit_bt_avx2,
      }},
     {{
          /*0 4x4   */ full_distortion_kernel_4x4_32bit_bt_avx2,
          /*1 8x8   */ full_distortion_kernel_8x8_32bit_bt_avx2,
          /*2 16x16 */ full_distortion_kernel_16_32_bit_bt_avx2,
          /*3       */ (EbFullDistType)full_distortion_void_func,
          /*4 32x32 */ full_distortion_kernel_16_32_bit_bt_avx2,
          /*5       */ (EbFullDistType)full_distortion_void_func,
          /*6       */ (EbFullDistType)full_distortion_void_func,
          /*7       */ (EbFullDistType)full_distortion_void_func,
          /*8 64x64 */ full_distortion_kernel_16_32_bit_bt_avx2,
      },
      {
          /*1 8x8   */ full_distortion_kernel_8x8_32bit_bt_sse2,
          /*2 16x16 */ full_distortion_kernel_intra_16_mxn_32_bit_bt_sse2,
          /*3       */ (EbFullDistType)full_distortion_void_func,
          /*4 32x32 */ full_distortion_kernel_intra_16_mxn_32_bit_bt_sse2,
          /*5       */ (EbFullDistType)full_distortion_void_func,
          /*6       */ (EbFullDistType)full_distortion_void_func,
          /*7       */ (EbFullDistType)full_distortion_void_func,
          /*8 64x64 */ full_distortion_kernel_intra_16_mxn_32_bit_bt_sse2,
      }}},

};

typedef uint64_t (*EbSpatialFullDistType)(uint8_t *input, uint32_t input_stride, uint8_t *recon, uint32_t recon_stride,
                                          uint32_t area_width, uint32_t area_height);

static EbSpatialFullDistType FUNC_TABLE spatialfull_distortion_kernel_func_ptr_array[ASM_TYPE_TOTAL][5] = {
    // C_DEFAULT
    {// 4x4
     eb_vp9_spatial_full_distortion_kernel,
     // 8x8
     eb_vp9_spatial_full_distortion_kernel,
     // 16x16
     eb_vp9_spatial_full_distortion_kernel,
     // 32x32
     eb_vp9_spatial_full_distortion_kernel},
    // ASM_AVX2
    {// 4x4
     spatialfull_distortion_kernel4x4_ssse3_intrin,
     // 8x8
     spatialfull_distortion_kernel8x8_ssse3_intrin,
     // 16x16
     spatialfull_distortion_kernel16_mx_n_ssse3_intrin,
     // 32x32
     spatialfull_distortion_kernel16_mx_n_ssse3_intrin,
     // 64x64
     spatialfull_distortion_kernel16_mx_n_ssse3_intrin},
};

#ifdef __cplusplus
}
#endif
#endif // EbPictureOperators_h
