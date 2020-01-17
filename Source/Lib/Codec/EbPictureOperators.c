/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/*********************************
 * Includes
 *********************************/

#include "EbPictureOperators.h"
#define VARIANCE_PRECISION      16
#define MEAN_PRECISION      (VARIANCE_PRECISION >> 1)

#include "EbDefinitions.h"
#include "EbPackUnPack.h"

/*********************************
 * x86 implememtation of Picture Addition
 *********************************/
void eb_vp9_picture_addition(
    uint8_t  *pred_ptr,
    uint32_t  pred_stride,
    int16_t  *residual_ptr,
    uint32_t  residual_stride,
    uint8_t  *recon_ptr,
    uint32_t  recon_stride,
    uint32_t  width,
    uint32_t  height)
{

    addition_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][width >> 3](
        pred_ptr,
        pred_stride,
        residual_ptr,
        residual_stride,
        recon_ptr,
        recon_stride,
        width,
        height
    );

    return;
}

/*********************************
 * Picture Copy 8bit Elements
 *********************************/
EbErrorType picture_copy8_bit(
    EbPictureBufferDesc *src,
    uint32_t             src_luma_origin_index,
    uint32_t             src_chroma_origin_index,
    EbPictureBufferDesc *dst,
    uint32_t             dst_luma_origin_index,
    uint32_t             dst_chroma_origin_index,
    uint32_t             area_width,
    uint32_t             area_height,
    uint32_t             chroma_area_width,
    uint32_t             chroma_area_height,
    uint32_t             component_mask)
{
    EbErrorType return_error = EB_ErrorNone;

    // Execute the Kernels
    if (component_mask & PICTURE_BUFFER_DESC_Y_FLAG) {

        pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][area_width>>3](
            &(src->buffer_y[src_luma_origin_index]),
            src->stride_y,
            &(dst->buffer_y[dst_luma_origin_index]),
            dst->stride_y,
            area_width,
            area_height);
    }

    if (component_mask & PICTURE_BUFFER_DESC_Cb_FLAG) {

        pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][chroma_area_width >> 3](
            &(src->buffer_cb[src_chroma_origin_index]),
            src->stride_cb,
            &(dst->buffer_cb[dst_chroma_origin_index]),
            dst->stride_cb,
            chroma_area_width,
            chroma_area_height);
    }

    if (component_mask & PICTURE_BUFFER_DESC_Cr_FLAG) {

        pic_copy_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][chroma_area_width >> 3](
            &(src->buffer_cr[src_chroma_origin_index]),
            src->stride_cr,
            &(dst->buffer_cr[dst_chroma_origin_index]),
            dst->stride_cr,
            chroma_area_width,
            chroma_area_height);
    }

    return return_error;
}

/*******************************************
* Picture Residue : subsampled version
  Computes the residual data
*******************************************/
void picture_sub_sampled_residual(
    uint8_t  *input,
    uint32_t  input_stride,
    uint8_t  *pred,
    uint32_t  pred_stride,
    int16_t  *residual,
    uint32_t  residual_stride,
    uint32_t  area_width,
    uint32_t  area_height,
    uint8_t   last_line)    //the last line has correct prediction data, so no duplication to be done.
{

    eb_vp9_residual_kernel_sub_sampled_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][area_width>>3](
        input,
        input_stride,
        pred,
        pred_stride,
        residual,
        residual_stride,
        area_width,
        area_height,
        last_line);

    return;
}
/*******************************************
* Pciture Residue
  Computes the residual data
*******************************************/
void picture_residual(
    uint8_t  *input,
    uint32_t  input_stride,
    uint8_t  *pred,
    uint32_t  pred_stride,
    int16_t  *residual,
    uint32_t  residual_stride,
    uint32_t  area_width,
    uint32_t  area_height)
{

    eb_vp9_residual_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][area_width>>3](
        input,
        input_stride,
        pred,
        pred_stride,
        residual,
        residual_stride,
        area_width,
        area_height);

    return;
}

/*******************************************
 * Pciture Residue 16bit input
   Computes the residual data
 *******************************************/
void picture_residual16bit(
    uint16_t *input,
    uint32_t  input_stride,
    uint16_t *pred,
    uint32_t  pred_stride,
    int16_t  *residual,
    uint32_t  residual_stride,
    uint32_t  area_width,
    uint32_t  area_height)
{

    eb_vp9_residual_kernel_func_ptr_array16_bit[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](
        input,
        input_stride,
        pred,
        pred_stride,
        residual,
        residual_stride,
        area_width,
        area_height);

    return;
}

/*******************************************
 * Picture Full Distortion
 *  Used in the Full Mode Decision Loop
 *******************************************/

EbErrorType picture_full_distortion(
    EbPictureBufferDesc *coeff,
    uint32_t             coeff_origin_index,
    EbPictureBufferDesc *recon_coeff,
    uint32_t             recon_coeff_origin_index,
    uint32_t             area_size,
    uint64_t             distortion[DIST_CALC_TOTAL],
    uint32_t             eob)
{
    EbErrorType return_error = EB_ErrorNone;

    //TODO due to a change in full kernel distortion , ASM has to be updated to not accumulate the input distortion by the output
    distortion[0]   = 0;
    distortion[1]   = 0;
    // Y
    full_distortion_intrinsic_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1][eob != 0][0][area_size >> 3](
        &(((int16_t*) coeff->buffer_y)[coeff_origin_index]),
        coeff->stride_y,
        &(((int16_t*) recon_coeff->buffer_y)[recon_coeff_origin_index]),
        recon_coeff->stride_y,
        distortion,
        area_size,
        area_size);

    return return_error;
}

void eb_vp9_extract_8bit_data(
    uint16_t *in16_bit_buffer,
    uint32_t  in_stride,
    uint8_t  *out8_bit_buffer,
    uint32_t  out8_stride,
    uint32_t  width,
    uint32_t  height
    )
{

    unpack_8bit_func_ptr_array_16bit[((width & 3) == 0) && ((height & 1)== 0)][(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](
        in16_bit_buffer,
        in_stride,
        out8_bit_buffer,
        out8_stride,
        width,
        height);
}
void eb_vp9_unpack_l0l1_avg(
        uint16_t *ref16_l0,
        uint32_t  ref_l0_stride,
        uint16_t *ref16_l1,
        uint32_t  ref_l1_stride,
        uint8_t  *dst_ptr,
        uint32_t  dst_stride,
        uint32_t  width,
        uint32_t  height)
 {

     eb_vp9_unpack_avg_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
        ref16_l0,
        ref_l0_stride,
        ref16_l1,
        ref_l1_stride,
        dst_ptr,
        dst_stride,
        width,
        height);

 }
void eb_vp9_extract8_bitdata_safe_sub(
    uint16_t *in16_bit_buffer,
    uint32_t  in_stride,
    uint8_t  *out8_bit_buffer,
    uint32_t  out8_stride,
    uint32_t  width,
    uint32_t  height
    )
{

    unpack_8bit_safe_sub_func_ptr_array_16bit[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
        in16_bit_buffer,
        in_stride,
        out8_bit_buffer,
        out8_stride,
        width,
        height
        );
}
void eb_vp9_unpack_l0l1_avg_safe_sub(
        uint16_t *ref16_l0,
        uint32_t  ref_l0_stride,
        uint16_t *ref16_l1,
        uint32_t  ref_l1_stride,
        uint8_t  *dst_ptr,
        uint32_t  dst_stride,
        uint32_t  width,
        uint32_t  height)
 {
     //fix C

     eb_vp9_unpack_avg_safe_sub_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
        ref16_l0,
        ref_l0_stride,
        ref16_l1,
        ref_l1_stride,
        dst_ptr,
        dst_stride,
        width,
        height);

 }
void unpack_2d(
    uint16_t *in16_bit_buffer,
    uint32_t  in_stride,
    uint8_t  *out8_bit_buffer,
    uint32_t  out8_stride,
    uint8_t  *outn_bit_buffer,
    uint32_t  outn_stride,
    uint32_t  width,
    uint32_t  height
    )
{

    unpack2_d_func_ptr_array_16_bit[((width & 3) == 0) && ((height & 1)== 0)][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
        in16_bit_buffer,
        in_stride,
        out8_bit_buffer,
        outn_bit_buffer,
        out8_stride,
        outn_stride,
        width,
        height);
}

void pack_2d_src(
    uint8_t  *in8_bit_buffer,
    uint32_t  in8_stride,
    uint8_t  *inn_bit_buffer,
    uint32_t  inn_stride,
    uint16_t *out16_bit_buffer,
    uint32_t  out_stride,
    uint32_t  width,
    uint32_t  height
   )
{

    pack2_d_func_ptr_array_16_bit_src[((width & 3) == 0) && ((height & 1)== 0)][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
        in8_bit_buffer,
        in8_stride,
        inn_bit_buffer,
        out16_bit_buffer,
        inn_stride,
        out_stride,
        width,
        height);
}

void compressed_pack_blk(
    uint8_t  *in8_bit_buffer,
    uint32_t  in8_stride,
    uint8_t  *inn_bit_buffer,
    uint32_t  inn_stride,
    uint16_t *out16_bit_buffer,
    uint32_t  out_stride,
    uint32_t  width,
    uint32_t  height
    )
{

    compressed_pack_func_ptr_array[((width == 64 || width == 32 || width == 16 || width == 8) ? ((eb_vp9_ASM_TYPES & AVX2_MASK) && 1) : ASM_NON_AVX2)](
        in8_bit_buffer,
        in8_stride,
        inn_bit_buffer,
        out16_bit_buffer,
        inn_stride,
        out_stride,
        width,
        height);

}

/*******************************************
 * eb_vp9_memset16bit
 *******************************************/
void eb_vp9_memset16bit(
    uint16_t *in_ptr,
    uint16_t  value,
    uint64_t  num_of_elements )
{
    uint64_t i;

    for(i = 0; i < num_of_elements; i++) {
        in_ptr[i]  = value;
    }
}
/*******************************************
 * eb_vp9_memcpy16bit
 *******************************************/
void eb_vp9_memcpy16bit(
    uint16_t *out_ptr,
    uint16_t *in_ptr,
    uint64_t  num_of_elements )
{
    uint64_t i;

    for( i =0;  i<num_of_elements;   i++) {
        out_ptr[i]  =  in_ptr[i]  ;
    }
}

int32_t  sum_residual(
    int16_t *in_ptr,
    uint32_t size,
    uint32_t stride_in )
{

    int32_t sum_block = 0;
    uint32_t i,j;

    for(j=0; j<size;    j++)
         for(i=0; i<size;    i++)
             sum_block+=in_ptr[j*stride_in + i];

    return sum_block;

}

void memset_16bit_block (
    int16_t *in_ptr,
    uint32_t stride_in,
    uint32_t size,
    int16_t  value)
{

    uint32_t i;
    for (i = 0; i < size; i++)
       eb_vp9_memset16bit((uint16_t*)in_ptr + i*stride_in, value, size);

}
