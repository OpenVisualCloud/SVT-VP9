/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbPictureOperators_C.h"
#include "EbUtility.h"

/*********************************
* Picture Average
*********************************/
void eb_vp9_picture_average_kernel(
    EbByte    src0,
    uint32_t  src0_stride,
    EbByte    src1,
    uint32_t  src1_stride,
    EbByte    dst,
    uint32_t  dst_stride,
    uint32_t  area_width,
    uint32_t  area_height)
{
    uint32_t x, y;

    for (y = 0; y < area_height; y++) {
        for (x = 0; x < area_width; x++) {
            dst[x] = (src0[x] + src1[x] + 1) >> 1;
        }
        src0 += src0_stride;
        src1 += src1_stride;
        dst += dst_stride;
    }
}

void eb_vp9_picture_average_kernel1_line_c(
    EbByte    src0,
    EbByte    src1,
    EbByte    dst,
    uint32_t  area_width)
{
    uint32_t i;
    for (i = 0; i < area_width; i++)
        dst[i] = (src0[i] + src1[i] + 1) / 2;

}

/*********************************
* Picture Copy Kernel
*********************************/
void eb_vp9_picture_copy_kernel(
    EbByte    src,
    uint32_t  src_stride,
    EbByte    dst,
    uint32_t  dst_stride,
    uint32_t  area_width,
    uint32_t  area_height,
    uint32_t  bytes_per_sample)  //=1 always)
{
    uint32_t sample_count = 0;
    const uint32_t sample_total_count = area_width*area_height;
    const uint32_t copy_length = area_width * bytes_per_sample;

    src_stride *= bytes_per_sample;
    dst_stride *= bytes_per_sample;

    while (sample_count < sample_total_count) {
        EB_MEMCPY(dst, src, copy_length);
        src += src_stride;
        dst += dst_stride;
        sample_count += area_width;
    }

    return;
}

/*********************************
* Picture Single Channel Kernel
*********************************/
void eb_vp9_picture_addition_kernel(
    uint8_t  *pred_ptr,
    uint32_t  pred_stride,
    int16_t  *residual_ptr,
    uint32_t  residual_stride,
    uint8_t  *recon_ptr,
    uint32_t  recon_stride,
    uint32_t  width,
    uint32_t  height)
{
    uint32_t          column_index;
    uint32_t          row_index = 0;
    const int32_t    max_value = 0xFF;

    while (row_index < height) {

        column_index = 0;
        while (column_index < width) {
            recon_ptr[column_index] = (uint8_t)CLIP3(0, max_value, ((int32_t)residual_ptr[column_index]) + ((int32_t)pred_ptr[column_index]));
            ++column_index;
        }

        residual_ptr += residual_stride;
        pred_ptr     += pred_stride;
        recon_ptr    += recon_stride;
        ++row_index;
    }

    return;
}

/*********************************
* Picture addtion 16bit Kernel
*********************************/
void eb_vp9_picture_addition_kernel16bit(
    uint16_t  *pred_ptr,
    uint32_t  pred_stride,
    int16_t  *residual_ptr,
    uint32_t  residual_stride,
    uint16_t  *recon_ptr,
    uint32_t  recon_stride,
    uint32_t  width,
    uint32_t  height)
{
    uint32_t          column_index;
    uint32_t          row_index = 0;
    const int32_t    max_value = 0x3FF;//0xFF;//CHKN

    while (row_index < height) {

        column_index = 0;
        while (column_index < width) {
            recon_ptr[column_index] = (uint16_t)CLIP3(0, max_value, ((int32_t)residual_ptr[column_index]) + ((int32_t)pred_ptr[column_index]));
            ++column_index;
        }

        residual_ptr += residual_stride;
        pred_ptr += pred_stride;
        recon_ptr += recon_stride;
        ++row_index;
    }

    return;
}

/*********************************
* Copy Kernel 8 bits
*********************************/
void copy_kernel8_bit(
    EbByte     src,
    uint32_t   src_stride,
    EbByte     dst,
    uint32_t   dst_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{

    uint32_t                   bytes_per_sample = 1;

    uint32_t sample_count = 0;
    const uint32_t sample_total_count = area_width*area_height;
    const uint32_t copy_length = area_width * bytes_per_sample;

    src_stride *= bytes_per_sample;
    dst_stride *= bytes_per_sample;

    while (sample_count < sample_total_count) {
        EB_MEMCPY(dst, src, copy_length);
        src += src_stride;
        dst += dst_stride;
        sample_count += area_width;
    }

    return;
}
/*********************************
* Copy Kernel 16 bits
*********************************/
void copy_kernel16_bit(
    EbByte     src,
    uint32_t   src_stride,
    EbByte     dst,
    uint32_t   dst_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{

    uint32_t                   bytes_per_sample = 2;

    uint32_t sample_count = 0;
    const uint32_t sample_total_count = area_width*area_height;
    const uint32_t copy_length = area_width * bytes_per_sample;

    src_stride *= bytes_per_sample;
    dst_stride *= bytes_per_sample;

    while (sample_count < sample_total_count) {
        EB_MEMCPY(dst, src, copy_length);
        src += src_stride;
        dst += dst_stride;
        sample_count += area_width;
    }

    return;
}

//computes a subsampled residual and then duplicate it
void eb_vp9_residual_kernel_sub_sampled(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height,
    uint8_t    last_line)
{
    uint32_t  column_index;
    uint32_t  row_index = 0;

    uint8_t   *input_O = input;
    uint8_t   *pred_O = pred;
    int16_t  *residual_O = residual;

    //hard code subampling dimensions, keep residual_stride
    area_height >>= 1;
    input_stride <<= 1;
    pred_stride <<= 1;

    while (row_index < area_height) {
        column_index = 0;
        while (column_index < area_width) {
            residual[column_index] = ((int16_t)input[column_index]) - ((int16_t)pred[column_index]);
            residual[column_index + residual_stride] = ((int16_t)input[column_index]) - ((int16_t)pred[column_index]);
            ++column_index;
        }

        input += input_stride;
        pred += pred_stride;
        residual += (residual_stride << 1);
        ++row_index;
    }

    //do the last line:
    if (last_line) {
        input_stride = input_stride / 2;
        pred_stride = pred_stride / 2;
        area_height = area_height * 2;
        column_index = 0;
        while (column_index < area_width) {
            residual_O[(area_height - 1)*residual_stride + column_index] = ((int16_t)input_O[(area_height - 1)*input_stride + column_index]) - ((int16_t)pred_O[(area_height - 1)*pred_stride + column_index]);
            ++column_index;
        }

    }
    return;
}

/*******************************************
* Residual Kernel
Computes the residual data
*******************************************/
void eb_vp9_residual_kernel(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{
    uint32_t  column_index;
    uint32_t  row_index = 0;

    while (row_index < area_height) {
        column_index = 0;
        while (column_index < area_width) {
            residual[column_index] = ((int16_t)input[column_index]) - ((int16_t)pred[column_index]);
            ++column_index;
        }

        input += input_stride;
        pred += pred_stride;
        residual += residual_stride;
        ++row_index;
    }

    return;
}

/*******************************************
* Residual Kernel 16bit
Computes the residual data
*******************************************/
void eb_vp9_residual_kernel16bit(
    uint16_t   *input,
    uint32_t   input_stride,
    uint16_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{
    uint32_t  column_index;
    uint32_t  row_index = 0;

    while (row_index < area_height) {
        column_index = 0;
        while (column_index < area_width) {
            residual[column_index] = ((int16_t)input[column_index]) - ((int16_t)pred[column_index]);
            ++column_index;
        }

        input += input_stride;
        pred += pred_stride;
        residual += residual_stride;
        ++row_index;
    }

    return;
}

/*********************************
* Zero Out Coeff Kernel
*********************************/
void zero_out_coeff_kernel(
    int16_t   *coeff_buffer,
    uint32_t   coeff_stride,
    uint32_t   coeff_origin_index,
    uint32_t   area_width,
    uint32_t   area_height)
{
    uint32_t i, j;

    for (j = 0; j < area_height; ++j) {
        for (i = 0; i < area_width; ++i) {
            coeff_buffer[j*coeff_stride + i + coeff_origin_index] = 0;
        }
    }

}

// C equivalents
static int32_t sq_r16to32(int16_t x)
{
    return x * x;
}

EB_EXTERN void full_distortion_kernel_intra32bit(
    int16_t   *coeff,
    uint32_t   coeff_stride,
    int16_t   *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height)
{
    uint32_t  column_index;
    uint32_t  row_index = 0;
    uint32_t  residual_distortion = 0;

    while (row_index < area_height) {

        column_index = 0;
        while (column_index < area_width) {
            residual_distortion += sq_r16to32(coeff[column_index] - recon_coeff[column_index]);
            ++column_index;
        }

        coeff += coeff_stride;
        recon_coeff += recon_coeff_stride;
        ++row_index;
    }

    distortion_result[0] = residual_distortion;
    distortion_result[1] = residual_distortion;
}

EB_EXTERN void full_distortion_kernel32bit(
    int16_t   *coeff,
    uint32_t   coeff_stride,
    int16_t   *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height)
{
    uint32_t  column_index;
    uint32_t  row_index = 0;
    uint32_t  residual_distortion = 0;
    uint32_t  prediction_distortion = 0;

    while (row_index < area_height) {

        column_index = 0;
        while (column_index < area_width) {
            residual_distortion += sq_r16to32(coeff[column_index] - recon_coeff[column_index]);
            prediction_distortion += sq_r16to32(coeff[column_index]);
            ++column_index;
        }

        coeff += coeff_stride;
        recon_coeff += recon_coeff_stride;
        ++row_index;
    }

    distortion_result[0] = residual_distortion;
    distortion_result[1] = prediction_distortion;
}

EB_EXTERN void full_distortion_kernel_eob_zero32bit(
    int16_t   *coeff,
    uint32_t   coeff_stride,
    int16_t   *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height)
{
    uint32_t  column_index;
    uint32_t  row_index = 0;
    uint32_t  prediction_distortion = 0;

    while (row_index < area_height) {

        column_index = 0;
        while (column_index < area_width) {
            prediction_distortion += sq_r16to32(coeff[column_index]);
            ++column_index;
        }

        coeff += coeff_stride;
        recon_coeff += recon_coeff_stride;
        ++row_index;
    }

    distortion_result[0] = prediction_distortion;
    distortion_result[1] = prediction_distortion;
}

uint64_t eb_vp9_spatial_full_distortion_kernel(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *recon,
    uint32_t   recon_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{
    uint32_t  column_index;
    uint32_t  row_index = 0;

    uint64_t  spatial_distortion = 0;

    while (row_index < area_height) {

        column_index = 0;
        while (column_index < area_width) {
            spatial_distortion += (int64_t)SQR((int64_t)(input[column_index]) - (recon[column_index]));
            ++column_index;
        }

        input += input_stride;
        recon += recon_stride;
        ++row_index;
    }
    return spatial_distortion;
}
