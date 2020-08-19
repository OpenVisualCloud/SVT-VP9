/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbPictureBufferDesc.h"

#include "EbResourceCoordinationResults.h"
#include "EbPictureAnalysisProcess.h"
#include "EbPictureAnalysisResults.h"
#include "EbMcp.h"
#include "EbMotionEstimation.h"
#include "EbReferenceObject.h"

#include "EbComputeMean.h"
#include "EbMeSadCalculation.h"
#include "EbComputeMean_SSE2.h"
#include "EbCombinedAveragingSAD_Intrinsic_AVX2.h"

#include "stdint.h"

#define VARIANCE_PRECISION        16
#define  SB_LOW_VAR_TH                5
#define  PIC_LOW_VAR_PERCENTAGE_TH    60
#define    FLAT_MAX_VAR            50
#define FLAT_MAX_VAR_DECIM        (50-00)
#define    NOISE_MIN_LEVEL_0         70000//120000
#define NOISE_MIN_LEVEL_DECIM_0 (70000+000000)//(120000+000000)
#define    NOISE_MIN_LEVEL_1        120000
#define NOISE_MIN_LEVEL_DECIM_1    (120000+000000)
#define DENOISER_QP_TH            29
#define DENOISER_BITRATE_TH        14000000
#define SAMPLE_THRESHOLD_PRECENT_BORDER_LINE      15
#define SAMPLE_THRESHOLD_PRECENT_TWO_BORDER_LINES 10

/************************************************
* Picture Analysis Context Constructor
************************************************/
EbErrorType eb_vp9_picture_analysis_context_ctor(
    EbPictureBufferDescInitData *input_picture_buffer_desc_init_data,
    EB_BOOL                      denoise_flag,
    PictureAnalysisContext     **context_dbl_ptr,
    EbFifo                      *resource_coordination_results_input_fifo_ptr,
    EbFifo                      *picture_analysis_results_output_fifo_ptr,
    uint16_t                     sb_total_count)
{
    PictureAnalysisContext *context_ptr;
    EB_MALLOC(PictureAnalysisContext*, context_ptr, sizeof(PictureAnalysisContext), EB_N_PTR);
    *context_dbl_ptr = context_ptr;

    context_ptr->resource_coordination_results_input_fifo_ptr = resource_coordination_results_input_fifo_ptr;
    context_ptr->picture_analysis_results_output_fifo_ptr = picture_analysis_results_output_fifo_ptr;

    EbErrorType return_error = EB_ErrorNone;

    if (denoise_flag == EB_TRUE){

        //denoised
        return_error = eb_vp9_picture_buffer_desc_ctor(
            (EbPtr *)&(context_ptr->denoised_picture_ptr),
            (EbPtr)input_picture_buffer_desc_init_data);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

        //luma buffer could re-used to process chroma
        context_ptr->denoised_picture_ptr->buffer_cb = context_ptr->denoised_picture_ptr->buffer_y;
        context_ptr->denoised_picture_ptr->buffer_cr = context_ptr->denoised_picture_ptr->buffer_y + context_ptr->denoised_picture_ptr->chroma_size;

        // noise
        input_picture_buffer_desc_init_data->max_height = MAX_SB_SIZE;
        input_picture_buffer_desc_init_data->buffer_enable_mask = PICTURE_BUFFER_DESC_Y_FLAG;

        return_error = eb_vp9_picture_buffer_desc_ctor(
            (EbPtr *)&(context_ptr->noise_picture_ptr),
            (EbPtr)input_picture_buffer_desc_init_data);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    EB_MALLOC(uint16_t**, context_ptr->grad, sizeof(uint16_t*) * sb_total_count, EB_N_PTR);
    for (uint16_t sb_index = 0; sb_index < sb_total_count; ++sb_index) {
        EB_MALLOC(uint16_t*, context_ptr->grad[sb_index], sizeof(uint16_t) * PA_BLOCK_MAX_COUNT, EB_N_PTR);
    }

    return EB_ErrorNone;
}

/************************************************
 * Picture Analysis Context Destructor
 ************************************************/

/********************************************
 * eb_vp9_decimation_2d
 *      decimates the input
 ********************************************/
void eb_vp9_decimation_2d(
    uint8_t  *input_samples,      // input parameter, input samples Ptr
    uint32_t  input_stride,       // input parameter, input stride
    uint32_t  input_area_width,    // input parameter, input area width
    uint32_t  input_area_height,   // input parameter, input area height
    uint8_t  *decim_samples,      // output parameter, decimated samples Ptr
    uint32_t  decim_stride,       // input parameter, output stride
    uint32_t  decim_step)        // input parameter, area height
{

    uint32_t horizontal_index;
    uint32_t vertical_index;

    for (vertical_index = 0; vertical_index < input_area_height; vertical_index += decim_step) {
        for (horizontal_index = 0; horizontal_index < input_area_width; horizontal_index += decim_step) {

            decim_samples[(horizontal_index >> (decim_step >> 1))] = input_samples[horizontal_index];

        }
        input_samples += (input_stride << (decim_step >> 1));
        decim_samples += decim_stride;
    }

    return;
}

/********************************************
* calculate_histogram
*      creates n-bins histogram for the input
********************************************/
static void calculate_histogram(
    uint8_t  *input_samples,      // input parameter, input samples Ptr
    uint32_t  input_area_width,    // input parameter, input area width
    uint32_t  input_area_height,   // input parameter, input area height
    uint32_t  stride,            // input parameter, input stride
    uint8_t   decim_step,         // input parameter, area height
    uint32_t *histogram,            // output parameter, output histogram
    uint64_t *sum)

{

    uint32_t horizontal_index;
    uint32_t vertical_index;
    *sum = 0;

    for (vertical_index = 0; vertical_index < input_area_height; vertical_index += decim_step) {
        for (horizontal_index = 0; horizontal_index < input_area_width; horizontal_index += decim_step) {
            ++(histogram[input_samples[horizontal_index]]);
            *sum += input_samples[horizontal_index];
        }
        input_samples += (stride << (decim_step >> 1));
    }

    return;
}

static uint64_t compute_variance32x32(
    EbPictureBufferDesc *input_padded_picture_ptr,         // input parameter, Input Padded Picture
    uint32_t             input_luma_origin_index,          // input parameter, LCU index, used to point to source/reference samples
    uint64_t            *variance8x8)
{

    uint32_t block_index;

    uint64_t mean_of8x8_blocks[16];
    uint64_t mean_of8x8_squared_values_blocks[16];

    uint64_t mean_of16x16_blocks[4];
    uint64_t mean_of16x16_squared_values_blocks[4];

    uint64_t mean_of32x32_blocks;
    uint64_t mean_of32x32_squared_values_blocks;
    /////////////////////////////////////////////
    // (0,0)
    block_index = input_luma_origin_index;

    mean_of8x8_blocks[0] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[0] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (0,1)
    block_index = block_index + 8;
    mean_of8x8_blocks[1] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[1] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (0,2)
    block_index = block_index + 8;
    mean_of8x8_blocks[2] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[2] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (0,3)
    block_index = block_index + 8;
    mean_of8x8_blocks[3] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[3] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (1,0)
    block_index = input_luma_origin_index + (input_padded_picture_ptr->stride_y << 3);
    mean_of8x8_blocks[4] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[4] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (1,1)
    block_index = block_index + 8;
    mean_of8x8_blocks[5] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[5] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (1,2)
    block_index = block_index + 8;
    mean_of8x8_blocks[6] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[6] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (1,3)
    block_index = block_index + 8;
    mean_of8x8_blocks[7] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[7] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (2,0)
    block_index = input_luma_origin_index + (input_padded_picture_ptr->stride_y << 4);
    mean_of8x8_blocks[8] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[8] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (2,1)
    block_index = block_index + 8;
    mean_of8x8_blocks[9] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[9] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (2,2)
    block_index = block_index + 8;
    mean_of8x8_blocks[10] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[10] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (2,3)
    block_index = block_index + 8;
    mean_of8x8_blocks[11] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[11] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (3,0)
    block_index = input_luma_origin_index + (input_padded_picture_ptr->stride_y << 3) + (input_padded_picture_ptr->stride_y << 4);
    mean_of8x8_blocks[12] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[12] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (3,1)
    block_index = block_index + 8;
    mean_of8x8_blocks[13] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[13] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (3,2)
    block_index = block_index + 8;
    mean_of8x8_blocks[14] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[14] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (3,3)
    block_index = block_index + 8;
    mean_of8x8_blocks[15] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[15] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    /////////////////////////////////////////////

    variance8x8[0] = mean_of8x8_squared_values_blocks[0] - (mean_of8x8_blocks[0] * mean_of8x8_blocks[0]);
    variance8x8[1] = mean_of8x8_squared_values_blocks[1] - (mean_of8x8_blocks[1] * mean_of8x8_blocks[1]);
    variance8x8[2] = mean_of8x8_squared_values_blocks[2] - (mean_of8x8_blocks[2] * mean_of8x8_blocks[2]);
    variance8x8[3] = mean_of8x8_squared_values_blocks[3] - (mean_of8x8_blocks[3] * mean_of8x8_blocks[3]);
    variance8x8[4] = mean_of8x8_squared_values_blocks[4] - (mean_of8x8_blocks[4] * mean_of8x8_blocks[4]);
    variance8x8[5] = mean_of8x8_squared_values_blocks[5] - (mean_of8x8_blocks[5] * mean_of8x8_blocks[5]);
    variance8x8[6] = mean_of8x8_squared_values_blocks[6] - (mean_of8x8_blocks[6] * mean_of8x8_blocks[6]);
    variance8x8[7] = mean_of8x8_squared_values_blocks[7] - (mean_of8x8_blocks[7] * mean_of8x8_blocks[7]);
    variance8x8[8] = mean_of8x8_squared_values_blocks[8] - (mean_of8x8_blocks[8] * mean_of8x8_blocks[8]);
    variance8x8[9] = mean_of8x8_squared_values_blocks[9] - (mean_of8x8_blocks[9] * mean_of8x8_blocks[9]);
    variance8x8[10] = mean_of8x8_squared_values_blocks[10] - (mean_of8x8_blocks[10] * mean_of8x8_blocks[10]);
    variance8x8[11] = mean_of8x8_squared_values_blocks[11] - (mean_of8x8_blocks[11] * mean_of8x8_blocks[11]);
    variance8x8[12] = mean_of8x8_squared_values_blocks[12] - (mean_of8x8_blocks[12] * mean_of8x8_blocks[12]);
    variance8x8[13] = mean_of8x8_squared_values_blocks[13] - (mean_of8x8_blocks[13] * mean_of8x8_blocks[13]);
    variance8x8[14] = mean_of8x8_squared_values_blocks[14] - (mean_of8x8_blocks[14] * mean_of8x8_blocks[14]);
    variance8x8[15] = mean_of8x8_squared_values_blocks[15] - (mean_of8x8_blocks[15] * mean_of8x8_blocks[15]);

    // 16x16
    mean_of16x16_blocks[0] = (mean_of8x8_blocks[0] + mean_of8x8_blocks[1] + mean_of8x8_blocks[8] + mean_of8x8_blocks[9]) >> 2;
    mean_of16x16_blocks[1] = (mean_of8x8_blocks[2] + mean_of8x8_blocks[3] + mean_of8x8_blocks[10] + mean_of8x8_blocks[11]) >> 2;
    mean_of16x16_blocks[2] = (mean_of8x8_blocks[4] + mean_of8x8_blocks[5] + mean_of8x8_blocks[12] + mean_of8x8_blocks[13]) >> 2;
    mean_of16x16_blocks[3] = (mean_of8x8_blocks[6] + mean_of8x8_blocks[7] + mean_of8x8_blocks[14] + mean_of8x8_blocks[15]) >> 2;

    mean_of16x16_squared_values_blocks[0] = (mean_of8x8_squared_values_blocks[0] + mean_of8x8_squared_values_blocks[1] + mean_of8x8_squared_values_blocks[8] + mean_of8x8_squared_values_blocks[9]) >> 2;
    mean_of16x16_squared_values_blocks[1] = (mean_of8x8_squared_values_blocks[2] + mean_of8x8_squared_values_blocks[3] + mean_of8x8_squared_values_blocks[10] + mean_of8x8_squared_values_blocks[11]) >> 2;
    mean_of16x16_squared_values_blocks[2] = (mean_of8x8_squared_values_blocks[4] + mean_of8x8_squared_values_blocks[5] + mean_of8x8_squared_values_blocks[12] + mean_of8x8_squared_values_blocks[13]) >> 2;
    mean_of16x16_squared_values_blocks[3] = (mean_of8x8_squared_values_blocks[6] + mean_of8x8_squared_values_blocks[7] + mean_of8x8_squared_values_blocks[14] + mean_of8x8_squared_values_blocks[15]) >> 2;

    // 32x32
    mean_of32x32_blocks = (mean_of16x16_blocks[0] + mean_of16x16_blocks[1] + mean_of16x16_blocks[2] + mean_of16x16_blocks[3]) >> 2;

    mean_of32x32_squared_values_blocks = (mean_of16x16_squared_values_blocks[0] + mean_of16x16_squared_values_blocks[1] + mean_of16x16_squared_values_blocks[2] + mean_of16x16_squared_values_blocks[3]) >> 2;

    return (mean_of32x32_squared_values_blocks - (mean_of32x32_blocks * mean_of32x32_blocks));
}

static uint64_t compute_variance16x16(
    EbPictureBufferDesc *input_padded_picture_ptr,         // input parameter, Input Padded Picture
    uint32_t             input_luma_origin_index,          // input parameter, LCU index, used to point to source/reference samples
    uint64_t             *variance8x8)
{

    uint32_t block_index;

    uint64_t mean_of8x8_blocks[4];
    uint64_t mean_of8x8_squared_values_blocks[4];

    uint64_t mean_of16x16_blocks;
    uint64_t mean_of16x16_squared_values_blocks;

    // (0,0)
    block_index = input_luma_origin_index;

    mean_of8x8_blocks[0] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[0] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (0,1)
    block_index = block_index + 8;
    mean_of8x8_blocks[1] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[1] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (1,0)
    block_index = input_luma_origin_index + (input_padded_picture_ptr->stride_y << 3);
    mean_of8x8_blocks[2] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[2] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    // (1,1)
    block_index = block_index + 8;
    mean_of8x8_blocks[3] = compute_mean_func[0][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);
    mean_of8x8_squared_values_blocks[3] = compute_mean_func[1][(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](&(input_padded_picture_ptr->buffer_y[block_index]), input_padded_picture_ptr->stride_y, 8, 8);

    variance8x8[0] = mean_of8x8_squared_values_blocks[0] - (mean_of8x8_blocks[0] * mean_of8x8_blocks[0]);
    variance8x8[1] = mean_of8x8_squared_values_blocks[1] - (mean_of8x8_blocks[1] * mean_of8x8_blocks[1]);
    variance8x8[2] = mean_of8x8_squared_values_blocks[2] - (mean_of8x8_blocks[2] * mean_of8x8_blocks[2]);
    variance8x8[3] = mean_of8x8_squared_values_blocks[3] - (mean_of8x8_blocks[3] * mean_of8x8_blocks[3]);

    // 16x16
    mean_of16x16_blocks = (mean_of8x8_blocks[0] + mean_of8x8_blocks[1] + mean_of8x8_blocks[2] + mean_of8x8_blocks[3]) >> 2;
    mean_of16x16_squared_values_blocks = (mean_of8x8_squared_values_blocks[0] + mean_of8x8_squared_values_blocks[1] + mean_of8x8_squared_values_blocks[2] + mean_of8x8_squared_values_blocks[3]) >> 2;

    return (mean_of16x16_squared_values_blocks - (mean_of16x16_blocks * mean_of16x16_blocks));
}

/*******************************************
compute_variance64x64
this function is exactly same as
PictureAnalysisComputeVarianceLcu excpet it
does not store data for every block,
just returns the 64x64 data point
*******************************************/
static uint64_t compute_variance64x64(
    EbPictureBufferDesc *input_padded_picture_ptr,         // input parameter, Input Padded Picture
    uint32_t             input_luma_origin_index,          // input parameter, LCU index, used to point to source/reference samples
    uint64_t            *variance32x32)
{

    uint32_t block_index;

    uint64_t mean_of8x8_blocks[64];
    uint64_t mean_of8x8_squared_values_blocks[64];

    uint64_t mean_of16x16_blocks[16];
    uint64_t mean_of16x16_squared_values_blocks[16];

    uint64_t mean_of32x32_blocks[4];
    uint64_t mean_of32x32_squared_values_blocks[4];

    uint64_t mean_of64x64_blocks;
    uint64_t mean_of64x64_squared_values_blocks;

    // (0,0)
    block_index = input_luma_origin_index;
    const uint16_t stride_y = input_padded_picture_ptr->stride_y;

    if ((eb_vp9_ASM_TYPES & AVX2_MASK) && 1) {

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[0], &mean_of8x8_squared_values_blocks[0]);

        // (0,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[4], &mean_of8x8_squared_values_blocks[4]);
        // (0,5)
        block_index = block_index + 24;

        // (1,0)
        block_index = input_luma_origin_index + (stride_y << 3);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[8], &mean_of8x8_squared_values_blocks[8]);

        // (1,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[12], &mean_of8x8_squared_values_blocks[12]);

        // (1,5)
        block_index = block_index + 24;

        // (2,0)
        block_index = input_luma_origin_index + (stride_y << 4);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[16], &mean_of8x8_squared_values_blocks[16]);

        // (2,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[20], &mean_of8x8_squared_values_blocks[20]);

        // (2,5)
        block_index = block_index + 24;

        // (3,0)
        block_index = input_luma_origin_index + (stride_y << 3) + (stride_y << 4);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[24], &mean_of8x8_squared_values_blocks[24]);

        // (3,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[28], &mean_of8x8_squared_values_blocks[28]);

        // (3,5)
        block_index = block_index + 24;

        // (4,0)
        block_index = input_luma_origin_index + (stride_y << 5);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[32], &mean_of8x8_squared_values_blocks[32]);

        // (4,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[36], &mean_of8x8_squared_values_blocks[36]);

        // (4,5)
        block_index = block_index + 24;

        // (5,0)
        block_index = input_luma_origin_index + (stride_y << 3) + (stride_y << 5);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[40], &mean_of8x8_squared_values_blocks[40]);

        // (5,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[44], &mean_of8x8_squared_values_blocks[44]);

        // (5,5)
        block_index = block_index + 24;

        // (6,0)
        block_index = input_luma_origin_index + (stride_y << 4) + (stride_y << 5);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[48], &mean_of8x8_squared_values_blocks[48]);

        // (6,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[52], &mean_of8x8_squared_values_blocks[52]);

        // (6,5)
        block_index = block_index + 24;

        // (7,0)
        block_index = input_luma_origin_index + (stride_y << 3) + (stride_y << 4) + (stride_y << 5);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[56], &mean_of8x8_squared_values_blocks[56]);

        // (7,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[60], &mean_of8x8_squared_values_blocks[60]);

    }
    else{
        mean_of8x8_blocks[0] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[0] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[1] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[1] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[2] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[2] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[3] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[3] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[4] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[4] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[5] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[5] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[6] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[6] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[7] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[7] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (1,0)
        block_index = input_luma_origin_index + (stride_y << 3);
        mean_of8x8_blocks[8] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[8] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (1,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[9] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[9] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (1,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[10] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[10] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (1,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[11] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[11] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (1,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[12] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[12] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (1,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[13] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[13] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (1,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[14] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[14] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (1,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[15] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[15] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (2,0)
        block_index = input_luma_origin_index + (stride_y << 4);
        mean_of8x8_blocks[16] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[16] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (2,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[17] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[17] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (2,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[18] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[18] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (2,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[19] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[19] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        /// (2,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[20] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[20] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (2,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[21] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[21] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (2,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[22] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[22] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (2,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[23] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[23] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (3,0)
        block_index = input_luma_origin_index + (stride_y << 3) + (stride_y << 4);
        mean_of8x8_blocks[24] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[24] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (3,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[25] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[25] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (3,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[26] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[26] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (3,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[27] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[27] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (3,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[28] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[28] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (3,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[29] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[29] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (3,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[30] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[30] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (3,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[31] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[31] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (4,0)
        block_index = input_luma_origin_index + (stride_y << 5);
        mean_of8x8_blocks[32] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[32] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (4,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[33] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[33] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (4,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[34] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[34] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (4,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[35] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[35] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (4,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[36] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[36] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (4,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[37] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[37] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (4,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[38] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[38] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (4,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[39] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[39] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (5,0)
        block_index = input_luma_origin_index + (stride_y << 3) + (stride_y << 5);
        mean_of8x8_blocks[40] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[40] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (5,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[41] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[41] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (5,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[42] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[42] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (5,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[43] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[43] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (5,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[44] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[44] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (5,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[45] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[45] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (5,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[46] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[46] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (5,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[47] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[47] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (6,0)
        block_index = input_luma_origin_index + (stride_y << 4) + (stride_y << 5);
        mean_of8x8_blocks[48] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[48] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (6,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[49] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[49] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (6,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[50] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[50] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (6,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[51] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[51] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (6,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[52] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[52] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (6,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[53] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[53] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (6,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[54] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[54] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (6,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[55] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[55] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (7,0)
        block_index = input_luma_origin_index + (stride_y << 3) + (stride_y << 4) + (stride_y << 5);
        mean_of8x8_blocks[56] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[56] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (7,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[57] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[57] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (7,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[58] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[58] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (7,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[59] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[59] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (7,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[60] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[60] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (7,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[61] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[61] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (7,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[62] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[62] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

        // (7,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[63] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);
        mean_of8x8_squared_values_blocks[63] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]),  stride_y);

    }

    // 16x16
    mean_of16x16_blocks[0] = (mean_of8x8_blocks[0] + mean_of8x8_blocks[1] + mean_of8x8_blocks[8] + mean_of8x8_blocks[9]) >> 2;
    mean_of16x16_blocks[1] = (mean_of8x8_blocks[2] + mean_of8x8_blocks[3] + mean_of8x8_blocks[10] + mean_of8x8_blocks[11]) >> 2;
    mean_of16x16_blocks[2] = (mean_of8x8_blocks[4] + mean_of8x8_blocks[5] + mean_of8x8_blocks[12] + mean_of8x8_blocks[13]) >> 2;
    mean_of16x16_blocks[3] = (mean_of8x8_blocks[6] + mean_of8x8_blocks[7] + mean_of8x8_blocks[14] + mean_of8x8_blocks[15]) >> 2;

    mean_of16x16_blocks[4] = (mean_of8x8_blocks[16] + mean_of8x8_blocks[17] + mean_of8x8_blocks[24] + mean_of8x8_blocks[25]) >> 2;
    mean_of16x16_blocks[5] = (mean_of8x8_blocks[18] + mean_of8x8_blocks[19] + mean_of8x8_blocks[26] + mean_of8x8_blocks[27]) >> 2;
    mean_of16x16_blocks[6] = (mean_of8x8_blocks[20] + mean_of8x8_blocks[21] + mean_of8x8_blocks[28] + mean_of8x8_blocks[29]) >> 2;
    mean_of16x16_blocks[7] = (mean_of8x8_blocks[22] + mean_of8x8_blocks[23] + mean_of8x8_blocks[30] + mean_of8x8_blocks[31]) >> 2;

    mean_of16x16_blocks[8] = (mean_of8x8_blocks[32] + mean_of8x8_blocks[33] + mean_of8x8_blocks[40] + mean_of8x8_blocks[41]) >> 2;
    mean_of16x16_blocks[9] = (mean_of8x8_blocks[34] + mean_of8x8_blocks[35] + mean_of8x8_blocks[42] + mean_of8x8_blocks[43]) >> 2;
    mean_of16x16_blocks[10] = (mean_of8x8_blocks[36] + mean_of8x8_blocks[37] + mean_of8x8_blocks[44] + mean_of8x8_blocks[45]) >> 2;
    mean_of16x16_blocks[11] = (mean_of8x8_blocks[38] + mean_of8x8_blocks[39] + mean_of8x8_blocks[46] + mean_of8x8_blocks[47]) >> 2;

    mean_of16x16_blocks[12] = (mean_of8x8_blocks[48] + mean_of8x8_blocks[49] + mean_of8x8_blocks[56] + mean_of8x8_blocks[57]) >> 2;
    mean_of16x16_blocks[13] = (mean_of8x8_blocks[50] + mean_of8x8_blocks[51] + mean_of8x8_blocks[58] + mean_of8x8_blocks[59]) >> 2;
    mean_of16x16_blocks[14] = (mean_of8x8_blocks[52] + mean_of8x8_blocks[53] + mean_of8x8_blocks[60] + mean_of8x8_blocks[61]) >> 2;
    mean_of16x16_blocks[15] = (mean_of8x8_blocks[54] + mean_of8x8_blocks[55] + mean_of8x8_blocks[62] + mean_of8x8_blocks[63]) >> 2;

    mean_of16x16_squared_values_blocks[0] = (mean_of8x8_squared_values_blocks[0] + mean_of8x8_squared_values_blocks[1] + mean_of8x8_squared_values_blocks[8] + mean_of8x8_squared_values_blocks[9]) >> 2;
    mean_of16x16_squared_values_blocks[1] = (mean_of8x8_squared_values_blocks[2] + mean_of8x8_squared_values_blocks[3] + mean_of8x8_squared_values_blocks[10] + mean_of8x8_squared_values_blocks[11]) >> 2;
    mean_of16x16_squared_values_blocks[2] = (mean_of8x8_squared_values_blocks[4] + mean_of8x8_squared_values_blocks[5] + mean_of8x8_squared_values_blocks[12] + mean_of8x8_squared_values_blocks[13]) >> 2;
    mean_of16x16_squared_values_blocks[3] = (mean_of8x8_squared_values_blocks[6] + mean_of8x8_squared_values_blocks[7] + mean_of8x8_squared_values_blocks[14] + mean_of8x8_squared_values_blocks[15]) >> 2;

    mean_of16x16_squared_values_blocks[4] = (mean_of8x8_squared_values_blocks[16] + mean_of8x8_squared_values_blocks[17] + mean_of8x8_squared_values_blocks[24] + mean_of8x8_squared_values_blocks[25]) >> 2;
    mean_of16x16_squared_values_blocks[5] = (mean_of8x8_squared_values_blocks[18] + mean_of8x8_squared_values_blocks[19] + mean_of8x8_squared_values_blocks[26] + mean_of8x8_squared_values_blocks[27]) >> 2;
    mean_of16x16_squared_values_blocks[6] = (mean_of8x8_squared_values_blocks[20] + mean_of8x8_squared_values_blocks[21] + mean_of8x8_squared_values_blocks[28] + mean_of8x8_squared_values_blocks[29]) >> 2;
    mean_of16x16_squared_values_blocks[7] = (mean_of8x8_squared_values_blocks[22] + mean_of8x8_squared_values_blocks[23] + mean_of8x8_squared_values_blocks[30] + mean_of8x8_squared_values_blocks[31]) >> 2;

    mean_of16x16_squared_values_blocks[8] = (mean_of8x8_squared_values_blocks[32] + mean_of8x8_squared_values_blocks[33] + mean_of8x8_squared_values_blocks[40] + mean_of8x8_squared_values_blocks[41]) >> 2;
    mean_of16x16_squared_values_blocks[9] = (mean_of8x8_squared_values_blocks[34] + mean_of8x8_squared_values_blocks[35] + mean_of8x8_squared_values_blocks[42] + mean_of8x8_squared_values_blocks[43]) >> 2;
    mean_of16x16_squared_values_blocks[10] = (mean_of8x8_squared_values_blocks[36] + mean_of8x8_squared_values_blocks[37] + mean_of8x8_squared_values_blocks[44] + mean_of8x8_squared_values_blocks[45]) >> 2;
    mean_of16x16_squared_values_blocks[11] = (mean_of8x8_squared_values_blocks[38] + mean_of8x8_squared_values_blocks[39] + mean_of8x8_squared_values_blocks[46] + mean_of8x8_squared_values_blocks[47]) >> 2;

    mean_of16x16_squared_values_blocks[12] = (mean_of8x8_squared_values_blocks[48] + mean_of8x8_squared_values_blocks[49] + mean_of8x8_squared_values_blocks[56] + mean_of8x8_squared_values_blocks[57]) >> 2;
    mean_of16x16_squared_values_blocks[13] = (mean_of8x8_squared_values_blocks[50] + mean_of8x8_squared_values_blocks[51] + mean_of8x8_squared_values_blocks[58] + mean_of8x8_squared_values_blocks[59]) >> 2;
    mean_of16x16_squared_values_blocks[14] = (mean_of8x8_squared_values_blocks[52] + mean_of8x8_squared_values_blocks[53] + mean_of8x8_squared_values_blocks[60] + mean_of8x8_squared_values_blocks[61]) >> 2;
    mean_of16x16_squared_values_blocks[15] = (mean_of8x8_squared_values_blocks[54] + mean_of8x8_squared_values_blocks[55] + mean_of8x8_squared_values_blocks[62] + mean_of8x8_squared_values_blocks[63]) >> 2;

    // 32x32
    mean_of32x32_blocks[0] = (mean_of16x16_blocks[0] + mean_of16x16_blocks[1] + mean_of16x16_blocks[4] + mean_of16x16_blocks[5]) >> 2;
    mean_of32x32_blocks[1] = (mean_of16x16_blocks[2] + mean_of16x16_blocks[3] + mean_of16x16_blocks[6] + mean_of16x16_blocks[7]) >> 2;
    mean_of32x32_blocks[2] = (mean_of16x16_blocks[8] + mean_of16x16_blocks[9] + mean_of16x16_blocks[12] + mean_of16x16_blocks[13]) >> 2;
    mean_of32x32_blocks[3] = (mean_of16x16_blocks[10] + mean_of16x16_blocks[11] + mean_of16x16_blocks[14] + mean_of16x16_blocks[15]) >> 2;

    mean_of32x32_squared_values_blocks[0] = (mean_of16x16_squared_values_blocks[0] + mean_of16x16_squared_values_blocks[1] + mean_of16x16_squared_values_blocks[4] + mean_of16x16_squared_values_blocks[5]) >> 2;
    mean_of32x32_squared_values_blocks[1] = (mean_of16x16_squared_values_blocks[2] + mean_of16x16_squared_values_blocks[3] + mean_of16x16_squared_values_blocks[6] + mean_of16x16_squared_values_blocks[7]) >> 2;
    mean_of32x32_squared_values_blocks[2] = (mean_of16x16_squared_values_blocks[8] + mean_of16x16_squared_values_blocks[9] + mean_of16x16_squared_values_blocks[12] + mean_of16x16_squared_values_blocks[13]) >> 2;
    mean_of32x32_squared_values_blocks[3] = (mean_of16x16_squared_values_blocks[10] + mean_of16x16_squared_values_blocks[11] + mean_of16x16_squared_values_blocks[14] + mean_of16x16_squared_values_blocks[15]) >> 2;

    variance32x32[0] = mean_of32x32_squared_values_blocks[0] - (mean_of32x32_blocks[0] * mean_of32x32_blocks[0]);
    variance32x32[1] = mean_of32x32_squared_values_blocks[1] - (mean_of32x32_blocks[1] * mean_of32x32_blocks[1]);
    variance32x32[2] = mean_of32x32_squared_values_blocks[2] - (mean_of32x32_blocks[2] * mean_of32x32_blocks[2]);
    variance32x32[3] = mean_of32x32_squared_values_blocks[3] - (mean_of32x32_blocks[3] * mean_of32x32_blocks[3]);

    // 64x64
    mean_of64x64_blocks = (mean_of32x32_blocks[0] + mean_of32x32_blocks[1] + mean_of32x32_blocks[2] + mean_of32x32_blocks[3]) >> 2;
    mean_of64x64_squared_values_blocks = (mean_of32x32_squared_values_blocks[0] + mean_of32x32_squared_values_blocks[1] + mean_of32x32_squared_values_blocks[2] + mean_of32x32_squared_values_blocks[3]) >> 2;

    return (mean_of64x64_squared_values_blocks - (mean_of64x64_blocks * mean_of64x64_blocks));
}

#if !TURN_OFF_PRE_PROCESSING
void check_input_for_borders_and_preprocess(
    EbPictureBufferDesc  *input_picture_ptr)
{

    uint32_t  row_index, col_index;
    uint32_t  pic_height;
    uint32_t  pic_width;
    uint32_t  input_origin_index;
    uint64_t  avg_line_curr, avg_line_neigh;

    uint8_t *ptr_in;
    uint8_t *ptr_src;
    uint32_t stride_in, twice_stride_in;

    EB_BOOL filter_top = EB_FALSE, filterBot = EB_FALSE, filterLeft = EB_FALSE, filter_right = EB_FALSE;
    EB_BOOL filter_top_two = EB_FALSE, filter_left_two = EB_FALSE;

    uint64_t avg_line0, avg_line1, avg_line2;
    EB_BOOL  filter2_lines = EB_FALSE, filter1_line = EB_FALSE;

    (void)filter_right;

    //Luma
    pic_height         = input_picture_ptr->height;
    pic_width          = input_picture_ptr->width;

    stride_in          = input_picture_ptr->stride_y;
    twice_stride_in    = stride_in + stride_in;
    input_origin_index = input_picture_ptr->origin_x + input_picture_ptr->origin_y * input_picture_ptr->stride_y;
    ptr_in             = &(input_picture_ptr->buffer_y[input_origin_index]);

    {

        // Top
        ptr_src = ptr_in;
        avg_line_curr = 0; avg_line_neigh = 0;
        // First check the second row of pixels against the first row
        for (row_index = 0; row_index < pic_width; row_index++){
            avg_line_curr += ptr_src[row_index + stride_in];
            avg_line_neigh += ptr_src[row_index + twice_stride_in];
        }

        avg_line_curr = avg_line_curr / pic_width;
        avg_line_neigh = avg_line_neigh / pic_width;

        filter_top_two = (ABS((int64_t)avg_line_neigh - (int64_t)avg_line_curr) > (int64_t)(avg_line_neigh * SAMPLE_THRESHOLD_PRECENT_TWO_BORDER_LINES / 100)) ? EB_TRUE : EB_FALSE;

        // Next check the first row of pixels against the second
        avg_line_curr = 0; avg_line_neigh = 0;
        for (row_index = 0; row_index < pic_width; row_index++){
            avg_line_curr += ptr_src[row_index];
            avg_line_neigh += ptr_src[row_index + stride_in];
        }

        avg_line_curr = avg_line_curr / pic_width;
        avg_line_neigh = avg_line_neigh / pic_width;

        filter_top = (ABS((int64_t)avg_line_neigh - (int64_t)avg_line_curr) > (int64_t)(avg_line_neigh * SAMPLE_THRESHOLD_PRECENT_BORDER_LINE / 100)) ? EB_TRUE : EB_FALSE;

        //Bottom
        ptr_src = &ptr_in[(pic_height - 1)*stride_in];
        avg_line_curr = 0; avg_line_neigh = 0;
        for (row_index = 0; row_index < pic_width; row_index++){
            avg_line_curr += ptr_src[row_index];
            avg_line_neigh += ptr_src[(int32_t)row_index - (int32_t)stride_in];
        }

        avg_line_curr = avg_line_curr / pic_width;
        avg_line_neigh = avg_line_neigh / pic_width;

        filterBot = (ABS((int64_t)avg_line_neigh - (int64_t)avg_line_curr) > (int64_t)(avg_line_neigh * SAMPLE_THRESHOLD_PRECENT_BORDER_LINE / 100)) ? EB_TRUE : EB_FALSE;

        //Left
        ptr_src = &ptr_in[0];
        // First check the second col of pixels against the first col
        avg_line_curr = 0; avg_line_neigh = 0;
        for (col_index = 0; col_index < pic_height; col_index++){
            avg_line_curr += ptr_src[col_index*stride_in + 1];
            avg_line_neigh += ptr_src[col_index*stride_in + 2];
        }

        avg_line_curr = avg_line_curr / pic_height;
        avg_line_neigh = avg_line_neigh / pic_height;

        filter_left_two = (ABS((int64_t)avg_line_neigh - (int64_t)avg_line_curr) > (int64_t)(avg_line_neigh * SAMPLE_THRESHOLD_PRECENT_TWO_BORDER_LINES / 100)) ? EB_TRUE : EB_FALSE;

        // Next check the first col of pixels against the second col
        avg_line_curr = 0; avg_line_neigh = 0;
        for (col_index = 0; col_index < pic_height; col_index++){
            avg_line_curr += ptr_src[col_index*stride_in];
            avg_line_neigh += ptr_src[col_index*stride_in + 1];
        }
        avg_line_curr = avg_line_curr / pic_height;
        avg_line_neigh = avg_line_neigh / pic_height;

        filterLeft = (ABS((int64_t)avg_line_neigh - (int64_t)avg_line_curr) > (int64_t)(avg_line_neigh * SAMPLE_THRESHOLD_PRECENT_BORDER_LINE / 100)) ? EB_TRUE : EB_FALSE;

        //Right
        ptr_src = &ptr_in[pic_width - 1];
        avg_line0 = 0; avg_line1 = 0; avg_line2 = 0;
        //---L2--L1--L0
        for (col_index = 0; col_index < pic_height; col_index++){
            avg_line0 += ptr_src[col_index*stride_in];
            avg_line1 += ptr_src[(int32_t)col_index*(int32_t)stride_in - 1];
            avg_line2 += ptr_src[(int32_t)col_index*(int32_t)stride_in - 2];
        }

        avg_line0 = avg_line0 / pic_height;
        avg_line1 = avg_line1 / pic_height;
        avg_line2 = avg_line2 / pic_height;

        filter2_lines = (ABS((int64_t)avg_line1 - (int64_t)avg_line2) > (int64_t)(avg_line2 * SAMPLE_THRESHOLD_PRECENT_TWO_BORDER_LINES / 100)) ? EB_TRUE : EB_FALSE;
        filter1_line = (ABS((int64_t)avg_line1 - (int64_t)avg_line0) > (int64_t)(avg_line1 * SAMPLE_THRESHOLD_PRECENT_BORDER_LINE / 100)) ? EB_TRUE : EB_FALSE;

        //SVT_LOG("--filter2_lines %i--filter1_line% i\n",filter2_lines,filter1_line);

    }

    // TOP
    ptr_src = ptr_in;
    if (filter_top_two && filter_top)
    {
        // Replace the two rows of border pixels with third row
        for (row_index = 0; row_index < pic_width; row_index++){
            ptr_src[row_index + stride_in] = ptr_src[row_index + twice_stride_in];
            ptr_src[row_index] = ptr_src[row_index + stride_in];
        }
    }
    else if (filter_top && !filter_top_two)
    {
        // Replace the first row of pixels with second row
        for (row_index = 0; row_index < pic_width; row_index++){
            ptr_src[row_index] = ptr_src[row_index + stride_in];
        }
    }

    // LEFT
    ptr_src = &ptr_in[0];
    if (filter_left_two && filterLeft) {
        // Replace the two cols of border pixels with third col
        for (col_index = 0; col_index < pic_height; col_index++){
            ptr_src[col_index*stride_in + 1] = ptr_src[col_index*stride_in + 2];
            ptr_src[col_index*stride_in] = ptr_src[col_index*stride_in + 1];
        }
    }
    else if (filterLeft && !filter_left_two) {
        // Replace the first col of border pixels with second col
        for (col_index = 0; col_index < pic_height; col_index++){
            ptr_src[col_index*stride_in] = ptr_src[col_index*stride_in + 1];
        }
    }

    // BOTTOM
    ptr_src = &ptr_in[(pic_height - 1)*stride_in];
    if (filterBot) {
        for (row_index = 0; row_index < pic_width; row_index++){
            ptr_src[row_index] = ptr_src[(int32_t)row_index - (int32_t)stride_in];
        }
    }

    // RIGHT
    ptr_src = &ptr_in[pic_width - 1];
    if (filter2_lines) {
        for (col_index = 0; col_index < pic_height; col_index++){
            ptr_src[col_index*stride_in] = ptr_src[(int32_t)col_index*(int32_t)stride_in - 2];
            ptr_src[(int32_t)col_index*(int32_t)stride_in - 1] = ptr_src[(int32_t)col_index*(int32_t)stride_in - 2];
        }
    }
    else if (filter1_line) {

        for (col_index = 0; col_index < pic_height; col_index++){
            ptr_src[col_index*stride_in] = ptr_src[(int32_t)col_index*(int32_t)stride_in - 1];
        }
    }

}
#endif

static uint8_t get_filtered_types(
    uint8_t  *ptr,
    uint32_t  stride,
    uint8_t   filter_type)
{
    uint8_t *p = ptr - 1 - stride;

    uint32_t a = 0;

    if (filter_type == 0){

        //Luma
        a = (p[1] +
            p[0 + stride] + 4 * p[1 + stride] + p[2 + stride] +
            p[1 + 2 * stride]) / 8;

    }
    else if (filter_type == 1){
        a = (                    2 * p[1] +
             2 * p[0 + stride] + 4 * p[1 + stride] + 2 * p[2 + stride] +
                                 2 * p[1 + 2 * stride]  );

        a =  (( (uint32_t)((a *2730) >> 14) + 1) >> 1) & 0xFFFF;

        //fixed point version of a=a/12 to mimic x86 instruction _mm256_mulhrs_epi16;
        //a= (a*2730)>>15;
    }
    else if (filter_type == 2){

        a = (4 * p[1] +
            4 * p[0 + stride] + 4 * p[1 + stride] + 4 * p[2 + stride] +
            4 * p[1 + 2 * stride]) / 20;
    }
    else if (filter_type == 3){

        a = (1 * p[0] + 1 * p[1] + 1 * p[2] +
            1 * p[0 + stride] + 4 * p[1 + stride] + 1 * p[2 + stride] +
            1 * p[0 + 2 * stride] + 1 * p[1 + 2 * stride] + 1 * p[2 + 2 * stride]) / 12;

    }
    else if (filter_type == 4){

        //gaussian matrix(Chroma)
        a = (1 * p[0] + 2 * p[1] + 1 * p[2] +
            2 * p[0 + stride] + 4 * p[1 + stride] + 2 * p[2 + stride] +
            1 * p[0 + 2 * stride] + 2 * p[1 + 2 * stride] + 1 * p[2 + 2 * stride]) / 16;

    }
    else if (filter_type == 5){

        a = (2 * p[0] + 2 * p[1] + 2 * p[2] +
            2 * p[0 + stride] + 4 * p[1 + stride] + 2 * p[2 + stride] +
            2 * p[0 + 2 * stride] + 2 * p[1 + 2 * stride] + 2 * p[2 + 2 * stride]) / 20;

    }
    else if (filter_type == 6){

        a = (4 * p[0] + 4 * p[1] + 4 * p[2] +
            4 * p[0 + stride] + 4 * p[1 + stride] + 4 * p[2 + stride] +
            4 * p[0 + 2 * stride] + 4 * p[1 + 2 * stride] + 4 * p[2 + 2 * stride]) / 36;

    }

    return  (uint8_t)CLIP3EQ(0, 255, a);

}

/*******************************************
* eb_vp9_noise_extract_luma_strong
*  strong filter Luma.
*******************************************/
void eb_vp9_noise_extract_luma_strong(
    EbPictureBufferDesc *input_picture_ptr,
    EbPictureBufferDesc *denoised_picture_ptr,
    uint32_t             sb_origin_y,
    uint32_t             sb_origin_x
    )
{
    uint32_t  ii, jj;
    uint32_t  pic_height, sb_height;
    uint32_t  pic_width;
    uint32_t  input_origin_index;
    uint32_t  input_origin_index_pad;

    uint8_t *ptr_in;
    uint32_t stride_in;
    uint8_t *ptr_denoised;

    uint32_t stride_out;
    uint32_t idx = (sb_origin_x + MAX_SB_SIZE > input_picture_ptr->width) ? sb_origin_x : 0;

    //Luma
    {
        pic_height = input_picture_ptr->height;
        pic_width = input_picture_ptr->width;
        sb_height = MIN(MAX_SB_SIZE, pic_height - sb_origin_y);

        stride_in = input_picture_ptr->stride_y;
        input_origin_index = input_picture_ptr->origin_x + (input_picture_ptr->origin_y + sb_origin_y)* input_picture_ptr->stride_y;
        ptr_in = &(input_picture_ptr->buffer_y[input_origin_index]);

        input_origin_index_pad = denoised_picture_ptr->origin_x + (denoised_picture_ptr->origin_y + sb_origin_y) * denoised_picture_ptr->stride_y;
        stride_out = denoised_picture_ptr->stride_y;
        ptr_denoised = &(denoised_picture_ptr->buffer_y[input_origin_index_pad]);

        for (jj = 0; jj < sb_height; jj++){
            for (ii = idx; ii < pic_width; ii++){

                if ((jj>0 || sb_origin_y > 0) && (jj < sb_height - 1 || sb_origin_y + sb_height < pic_height) && ii>0 && ii < pic_width - 1){

                    ptr_denoised[ii + jj*stride_out] = get_filtered_types(&ptr_in[ii + jj*stride_in], stride_in, 4);

                }
                else{
                    ptr_denoised[ii + jj*stride_out] = ptr_in[ii + jj*stride_in];

                }

            }
        }
    }

}
/*******************************************
* eb_vp9_noise_extract_chroma_strong
*  strong filter chroma.
*******************************************/
void eb_vp9_noise_extract_chroma_strong(
    EbPictureBufferDesc *input_picture_ptr,
    EbPictureBufferDesc *denoised_picture_ptr,
    uint32_t             sb_origin_y,
    uint32_t             sb_origin_x
    )
{
    uint32_t  ii, jj;
    uint32_t  pic_height, sb_height;
    uint32_t  pic_width;
    uint32_t  input_origin_index;
    uint32_t  input_origin_index_pad;

    uint8_t *ptr_in;
    uint32_t stride_in;
    uint8_t *ptr_denoised;

    uint32_t stride_out;
    uint32_t idx = (sb_origin_x + MAX_SB_SIZE > input_picture_ptr->width) ? sb_origin_x : 0;

    //Cb
    {
        pic_height = input_picture_ptr->height / 2;
        pic_width = input_picture_ptr->width / 2;
        sb_height = MIN(MAX_SB_SIZE / 2, pic_height - sb_origin_y);

        stride_in = input_picture_ptr->stride_cb;
        input_origin_index = input_picture_ptr->origin_x / 2 + (input_picture_ptr->origin_y / 2 + sb_origin_y)  * input_picture_ptr->stride_cb;
        ptr_in = &(input_picture_ptr->buffer_cb[input_origin_index]);

        input_origin_index_pad = denoised_picture_ptr->origin_x / 2 + (denoised_picture_ptr->origin_y / 2 + sb_origin_y)  * denoised_picture_ptr->stride_cb;
        stride_out = denoised_picture_ptr->stride_cb;
        ptr_denoised = &(denoised_picture_ptr->buffer_cb[input_origin_index_pad]);

        for (jj = 0; jj < sb_height; jj++){
            for (ii = idx; ii < pic_width; ii++){

                if ((jj>0 || sb_origin_y > 0) && (jj < sb_height - 1 || (sb_origin_y + sb_height) < pic_height) && ii > 0 && ii < pic_width - 1){
                    ptr_denoised[ii + jj*stride_out] = get_filtered_types(&ptr_in[ii + jj*stride_in], stride_in, 6);
                }
                else{
                    ptr_denoised[ii + jj*stride_out] = ptr_in[ii + jj*stride_in];
                }

            }
        }
    }

    //Cr
    {
        pic_height = input_picture_ptr->height / 2;
        pic_width = input_picture_ptr->width / 2;
        sb_height = MIN(MAX_SB_SIZE / 2, pic_height - sb_origin_y);

        stride_in = input_picture_ptr->stride_cr;
        input_origin_index = input_picture_ptr->origin_x / 2 + (input_picture_ptr->origin_y / 2 + sb_origin_y)  * input_picture_ptr->stride_cr;
        ptr_in = &(input_picture_ptr->buffer_cr[input_origin_index]);

        input_origin_index_pad = denoised_picture_ptr->origin_x / 2 + (denoised_picture_ptr->origin_y / 2 + sb_origin_y)  * denoised_picture_ptr->stride_cr;
        stride_out = denoised_picture_ptr->stride_cr;
        ptr_denoised = &(denoised_picture_ptr->buffer_cr[input_origin_index_pad]);

        for (jj = 0; jj < sb_height; jj++){
            for (ii = idx; ii < pic_width; ii++){

                if ((jj>0 || sb_origin_y > 0) && (jj < sb_height - 1 || (sb_origin_y + sb_height) < pic_height) && ii > 0 && ii < pic_width - 1){
                    ptr_denoised[ii + jj*stride_out] = get_filtered_types(&ptr_in[ii + jj*stride_in], stride_in, 6);
                }
                else{
                    ptr_denoised[ii + jj*stride_out] = ptr_in[ii + jj*stride_in];
                }

            }
        }
    }

}

/*******************************************
* eb_vp9_noise_extract_chroma_weak
*  weak filter chroma.
*******************************************/
void eb_vp9_noise_extract_chroma_weak(
    EbPictureBufferDesc *input_picture_ptr,
    EbPictureBufferDesc *denoised_picture_ptr,
    uint32_t             sb_origin_y,
    uint32_t             sb_origin_x
)
{
    uint32_t  ii, jj;
    uint32_t  pic_height, sb_height;
    uint32_t  pic_width;
    uint32_t  input_origin_index;
    uint32_t  input_origin_index_pad;

    uint8_t *ptr_in;
    uint32_t stride_in;
    uint8_t *ptr_denoised;

    uint32_t stride_out;

    uint32_t idx = (sb_origin_x + MAX_SB_SIZE > input_picture_ptr->width) ? sb_origin_x : 0;

    //Cb
    {
        pic_height = input_picture_ptr->height / 2;
        pic_width = input_picture_ptr->width / 2;

        sb_height = MIN(MAX_SB_SIZE / 2, pic_height - sb_origin_y);

        stride_in = input_picture_ptr->stride_cb;
        input_origin_index = input_picture_ptr->origin_x / 2 + (input_picture_ptr->origin_y / 2 + sb_origin_y)* input_picture_ptr->stride_cb;
        ptr_in = &(input_picture_ptr->buffer_cb[input_origin_index]);

        input_origin_index_pad = denoised_picture_ptr->origin_x / 2 + (denoised_picture_ptr->origin_y / 2 + sb_origin_y)* denoised_picture_ptr->stride_cb;
        stride_out = denoised_picture_ptr->stride_cb;
        ptr_denoised = &(denoised_picture_ptr->buffer_cb[input_origin_index_pad]);

        for (jj = 0; jj < sb_height; jj++){
            for (ii = idx; ii < pic_width; ii++){

                if ((jj>0 || sb_origin_y > 0) && (jj < sb_height - 1 || (sb_origin_y + sb_height) < pic_height) && ii > 0 && ii < pic_width - 1){
                    ptr_denoised[ii + jj*stride_out] = get_filtered_types(&ptr_in[ii + jj*stride_in], stride_in, 4);
                }
                else{
                    ptr_denoised[ii + jj*stride_out] = ptr_in[ii + jj*stride_in];
                }

            }
        }
    }

    //Cr
    {
        pic_height = input_picture_ptr->height / 2;
        pic_width = input_picture_ptr->width / 2;
        sb_height = MIN(MAX_SB_SIZE / 2, pic_height - sb_origin_y);

        stride_in = input_picture_ptr->stride_cr;
        input_origin_index = input_picture_ptr->origin_x / 2 + (input_picture_ptr->origin_y / 2 + sb_origin_y)* input_picture_ptr->stride_cr;
        ptr_in = &(input_picture_ptr->buffer_cr[input_origin_index]);

        input_origin_index_pad = denoised_picture_ptr->origin_x / 2 + (denoised_picture_ptr->origin_y / 2 + sb_origin_y)* denoised_picture_ptr->stride_cr;
        stride_out = denoised_picture_ptr->stride_cr;
        ptr_denoised = &(denoised_picture_ptr->buffer_cr[input_origin_index_pad]);

        for (jj = 0; jj < sb_height; jj++){
            for (ii = idx; ii < pic_width; ii++){

                if ((jj>0 || sb_origin_y > 0) && (jj < sb_height - 1 || (sb_origin_y + sb_height) < pic_height) && ii > 0 && ii < pic_width - 1){
                    ptr_denoised[ii + jj*stride_out] = get_filtered_types(&ptr_in[ii + jj*stride_in], stride_in, 4);
                }
                else{
                    ptr_denoised[ii + jj*stride_out] = ptr_in[ii + jj*stride_in];
                }

            }
        }
    }

}

/*******************************************
* eb_vp9_noise_extract_luma_weak
*  weak filter Luma and store noise.
*******************************************/
void eb_vp9_noise_extract_luma_weak(
    EbPictureBufferDesc *input_picture_ptr,
    EbPictureBufferDesc *denoised_picture_ptr,
    EbPictureBufferDesc *noise_picture_ptr,
    uint32_t             sb_origin_y,
    uint32_t             sb_origin_x
    )
{
    uint32_t  ii, jj;
    uint32_t  pic_height, sb_height;
    uint32_t  pic_width;
    uint32_t  input_origin_index;
    uint32_t  input_origin_index_pad;
    uint32_t  noise_origin_index;

    uint8_t *ptr_in;
    uint32_t stride_in;
    uint8_t *ptr_denoised;

    uint8_t *ptr_noise;
    uint32_t stride_out;

    uint32_t idx = (sb_origin_x + MAX_SB_SIZE > input_picture_ptr->width) ? sb_origin_x : 0;

    //Luma
    {
        pic_height = input_picture_ptr->height;
        pic_width = input_picture_ptr->width;
        sb_height = MIN(MAX_SB_SIZE, pic_height - sb_origin_y);

        stride_in = input_picture_ptr->stride_y;
        input_origin_index = input_picture_ptr->origin_x + (input_picture_ptr->origin_y + sb_origin_y) * input_picture_ptr->stride_y;
        ptr_in = &(input_picture_ptr->buffer_y[input_origin_index]);

        input_origin_index_pad = denoised_picture_ptr->origin_x + (denoised_picture_ptr->origin_y + sb_origin_y) * denoised_picture_ptr->stride_y;
        stride_out = denoised_picture_ptr->stride_y;
        ptr_denoised = &(denoised_picture_ptr->buffer_y[input_origin_index_pad]);

        noise_origin_index = noise_picture_ptr->origin_x + noise_picture_ptr->origin_y * noise_picture_ptr->stride_y;
        ptr_noise = &(noise_picture_ptr->buffer_y[noise_origin_index]);

        for (jj = 0; jj < sb_height; jj++){
            for (ii = idx; ii < pic_width; ii++){

                if ((jj>0 || sb_origin_y > 0) && (jj < sb_height - 1 || sb_origin_y + sb_height < pic_height) && ii>0 && ii < pic_width - 1){

                    ptr_denoised[ii + jj*stride_out] = get_filtered_types(&ptr_in[ii + jj*stride_in], stride_in, 0);
                    ptr_noise[ii + jj*stride_out] = CLIP3EQ(0, 255, ptr_in[ii + jj*stride_in] - ptr_denoised[ii + jj*stride_out]);

                }
                else{
                    ptr_denoised[ii + jj*stride_out] = ptr_in[ii + jj*stride_in];
                    ptr_noise[ii + jj*stride_out] = 0;
                }

            }
        }
    }

}

void eb_vp9_noise_extract_luma_weak_sb(
    EbPictureBufferDesc *input_picture_ptr,
    EbPictureBufferDesc *denoised_picture_ptr,
    EbPictureBufferDesc *noise_picture_ptr,
    uint32_t             sb_origin_y,
    uint32_t             sb_origin_x)
{
    uint32_t  ii, jj;
    uint32_t  pic_height, sb_height;
    uint32_t  pic_width, sb_width;
    uint32_t  input_origin_index;
    uint32_t  input_origin_index_pad;
    uint32_t  noise_origin_index;

    uint8_t *ptr_in;
    uint32_t stride_in;
    uint8_t *ptr_denoised;

    uint8_t *ptr_noise;
    uint32_t stride_out;

    uint32_t idx = (sb_origin_x + MAX_SB_SIZE > input_picture_ptr->width) ? sb_origin_x : 0;

    //Luma
    {
        pic_height = input_picture_ptr->height;
        pic_width = input_picture_ptr->width;

        sb_height = MIN(MAX_SB_SIZE, pic_height - sb_origin_y);
        sb_width = MIN(MAX_SB_SIZE, pic_width - sb_origin_x);

        stride_in = input_picture_ptr->stride_y;
        input_origin_index = input_picture_ptr->origin_x + sb_origin_x + (input_picture_ptr->origin_y + sb_origin_y) * input_picture_ptr->stride_y;
        ptr_in = &(input_picture_ptr->buffer_y[input_origin_index]);

        input_origin_index_pad = denoised_picture_ptr->origin_x + sb_origin_x + (denoised_picture_ptr->origin_y + sb_origin_y) * denoised_picture_ptr->stride_y;
        stride_out = denoised_picture_ptr->stride_y;
        ptr_denoised = &(denoised_picture_ptr->buffer_y[input_origin_index_pad]);

        noise_origin_index = noise_picture_ptr->origin_x + sb_origin_x + noise_picture_ptr->origin_y * noise_picture_ptr->stride_y;
        ptr_noise = &(noise_picture_ptr->buffer_y[noise_origin_index]);

        for (jj = 0; jj < sb_height; jj++){
            for (ii = idx; ii < sb_width; ii++){

                if ((jj>0 || sb_origin_y > 0) && (jj < sb_height - 1 || sb_origin_y + sb_height < pic_height) && (ii>0 || sb_origin_x>0) && (ii + sb_origin_x) < pic_width - 1/* & ii < sb_width - 1*/){

                    ptr_denoised[ii + jj*stride_out] = get_filtered_types(&ptr_in[ii + jj*stride_in], stride_in, 0);
                    ptr_noise[ii + jj*stride_out] = CLIP3EQ(0, 255, ptr_in[ii + jj*stride_in] - ptr_denoised[ii + jj*stride_out]);

                }
                else{
                    ptr_denoised[ii + jj*stride_out] = ptr_in[ii + jj*stride_in];
                    ptr_noise[ii + jj*stride_out] = 0;
                }

            }
        }
    }

}

static EbErrorType zero_out_chroma_block_mean(
    PictureParentControlSet     *picture_control_set_ptr,          // input parameter, Picture Control Set Ptr
    uint32_t                       sb_index                // input parameter, LCU address
    )
{

    EbErrorType return_error = EB_ErrorNone;
    // 16x16 mean
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_0] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_1] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_2] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_3] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_4] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_5] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_6] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_7] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_8] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_9] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_10] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_11] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_12] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_13] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_14] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_15] = 0;

    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_0] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_1] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_2] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_3] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_4] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_5] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_6] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_7] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_8] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_9] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_10] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_11] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_12] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_13] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_14] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_15] = 0;

    // 32x32 mean
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_32x32_0] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_32x32_1] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_32x32_2] = 0;
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_32x32_3] = 0;

    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_32x32_0] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_32x32_1] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_32x32_2] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_32x32_3] = 0;

    // 64x64 mean
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_64x64] = 0;
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_64x64] = 0;

    return return_error;

}
/*******************************************
* compute_chroma_block_mean
*   computes the chroma block mean for 64x64, 32x32 and 16x16 CUs inside the tree block
*******************************************/
static EbErrorType compute_chroma_block_mean(
    PictureParentControlSet *picture_control_set_ptr,          // input parameter, Picture Control Set Ptr
    EbPictureBufferDesc     *input_padded_picture_ptr,         // input parameter, Input Padded Picture
    uint32_t                 sb_index,                      // input parameter, LCU address
    uint32_t                 input_cb_origin_index,            // input parameter, LCU index, used to point to source/reference samples
    uint32_t                 input_cr_origin_index)            // input parameter, LCU index, used to point to source/reference samples
{

    EbErrorType return_error = EB_ErrorNone;

    uint32_t cb_block_index, cr_block_index;

    uint64_t cb_mean_of16x16_blocks[16];
    uint64_t cr_mean_of16x16_blocks[16];

    uint64_t cb_mean_of32x32_blocks[4];
    uint64_t cr_mean_of32x32_blocks[4];

    uint64_t cb_mean_of64x64_blocks;
    uint64_t cr_mean_of64x64_blocks;

    // (0,0) 16x16 block
    cb_block_index = input_cb_origin_index;
    cr_block_index = input_cr_origin_index;

    const uint16_t stride_cb = input_padded_picture_ptr->stride_cb;
    const uint16_t stride_cr = input_padded_picture_ptr->stride_cr;

    cb_mean_of16x16_blocks[0] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[0] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (0,1)
    cb_block_index = cb_block_index + 8;
    cr_block_index = cr_block_index + 8;
    cb_mean_of16x16_blocks[1] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[1] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (0,2)
    cb_block_index = cb_block_index + 8;
    cr_block_index = cr_block_index + 8;
    cb_mean_of16x16_blocks[2] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[2] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (0,3)
    cb_block_index = cb_block_index + 8;
    cr_block_index = cr_block_index + 8;
    cb_mean_of16x16_blocks[3] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[3] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (1,0)
    cb_block_index = input_cb_origin_index + (stride_cb << 3);
    cr_block_index = input_cr_origin_index + (stride_cr << 3);
    cb_mean_of16x16_blocks[4] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[4] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (1,1)
    cb_block_index = cb_block_index + 8;
    cr_block_index = cr_block_index + 8;
    cb_mean_of16x16_blocks[5] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[5] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (1,2)
    cb_block_index = cb_block_index + 8;
    cr_block_index = cr_block_index + 8;
    cb_mean_of16x16_blocks[6] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[6] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (1,3)
    cb_block_index = cb_block_index + 8;
    cr_block_index = cr_block_index + 8;
    cb_mean_of16x16_blocks[7] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[7] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (2,0)
    cb_block_index = input_cb_origin_index + (stride_cb << 4);
    cr_block_index = input_cr_origin_index + (stride_cr << 4);
    cb_mean_of16x16_blocks[8] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[8] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (2,1)
    cb_block_index = cb_block_index + 8;
    cr_block_index = cr_block_index + 8;
    cb_mean_of16x16_blocks[9] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[9] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (2,2)
    cb_block_index = cb_block_index + 8;
    cr_block_index = cr_block_index + 8;
    cb_mean_of16x16_blocks[10] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[10] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (2,3)
    cb_block_index = cb_block_index + 8;
    cr_block_index = cr_block_index + 8;
    cb_mean_of16x16_blocks[11] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[11] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (3,0)
    cb_block_index = input_cb_origin_index + (stride_cb * 24);
    cr_block_index = input_cr_origin_index + (stride_cr * 24);
    cb_mean_of16x16_blocks[12] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[12] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (3,1)
    cb_block_index = cb_block_index + 8;
    cr_block_index = cr_block_index + 8;
    cb_mean_of16x16_blocks[13] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[13] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (3,2)
    cb_block_index = cb_block_index + 8;
    cr_block_index = cr_block_index + 8;
    cb_mean_of16x16_blocks[14] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[14] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // (3,3)
    cb_block_index = cb_block_index + 8;
    cr_block_index = cr_block_index + 8;
    cb_mean_of16x16_blocks[15] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cb[cb_block_index]), stride_cb);
    cr_mean_of16x16_blocks[15] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_cr[cr_block_index]), stride_cr);

    // 32x32
    cb_mean_of32x32_blocks[0] = (cb_mean_of16x16_blocks[0] + cb_mean_of16x16_blocks[1] + cb_mean_of16x16_blocks[4] + cb_mean_of16x16_blocks[5]) >> 2;
    cr_mean_of32x32_blocks[0] = (cr_mean_of16x16_blocks[0] + cr_mean_of16x16_blocks[1] + cr_mean_of16x16_blocks[4] + cr_mean_of16x16_blocks[5]) >> 2;

    cb_mean_of32x32_blocks[1] = (cb_mean_of16x16_blocks[2] + cb_mean_of16x16_blocks[3] + cb_mean_of16x16_blocks[6] + cb_mean_of16x16_blocks[7]) >> 2;
    cr_mean_of32x32_blocks[1] = (cr_mean_of16x16_blocks[2] + cr_mean_of16x16_blocks[3] + cr_mean_of16x16_blocks[6] + cr_mean_of16x16_blocks[7]) >> 2;

    cb_mean_of32x32_blocks[2] = (cb_mean_of16x16_blocks[8] + cb_mean_of16x16_blocks[9] + cb_mean_of16x16_blocks[12] + cb_mean_of16x16_blocks[13]) >> 2;
    cr_mean_of32x32_blocks[2] = (cr_mean_of16x16_blocks[8] + cr_mean_of16x16_blocks[9] + cr_mean_of16x16_blocks[12] + cr_mean_of16x16_blocks[13]) >> 2;

    cb_mean_of32x32_blocks[3] = (cb_mean_of16x16_blocks[10] + cb_mean_of16x16_blocks[11] + cb_mean_of16x16_blocks[14] + cb_mean_of16x16_blocks[15]) >> 2;
    cr_mean_of32x32_blocks[3] = (cr_mean_of16x16_blocks[10] + cr_mean_of16x16_blocks[11] + cr_mean_of16x16_blocks[14] + cr_mean_of16x16_blocks[15]) >> 2;

    // 64x64
    cb_mean_of64x64_blocks = (cb_mean_of32x32_blocks[0] + cb_mean_of32x32_blocks[1] + cb_mean_of32x32_blocks[3] + cb_mean_of32x32_blocks[3]) >> 2;
    cr_mean_of64x64_blocks = (cr_mean_of32x32_blocks[0] + cr_mean_of32x32_blocks[1] + cr_mean_of32x32_blocks[3] + cr_mean_of32x32_blocks[3]) >> 2;
    // 16x16 mean
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_0] = (uint8_t) (cb_mean_of16x16_blocks[0] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_1] = (uint8_t) (cb_mean_of16x16_blocks[1] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_2] = (uint8_t) (cb_mean_of16x16_blocks[2] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_3] = (uint8_t) (cb_mean_of16x16_blocks[3] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_4] = (uint8_t) (cb_mean_of16x16_blocks[4] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_5] = (uint8_t) (cb_mean_of16x16_blocks[5] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_6] = (uint8_t) (cb_mean_of16x16_blocks[6] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_7] = (uint8_t) (cb_mean_of16x16_blocks[7] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_8] = (uint8_t) (cb_mean_of16x16_blocks[8] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_9] = (uint8_t) (cb_mean_of16x16_blocks[9] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_10] = (uint8_t) (cb_mean_of16x16_blocks[10] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_11] = (uint8_t) (cb_mean_of16x16_blocks[11] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_12] = (uint8_t) (cb_mean_of16x16_blocks[12] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_13] = (uint8_t) (cb_mean_of16x16_blocks[13] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_14] = (uint8_t) (cb_mean_of16x16_blocks[14] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_16x16_15] = (uint8_t) (cb_mean_of16x16_blocks[15] >> MEAN_PRECISION);

    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_0] = (uint8_t) (cr_mean_of16x16_blocks[0] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_1] = (uint8_t) (cr_mean_of16x16_blocks[1] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_2] = (uint8_t) (cr_mean_of16x16_blocks[2] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_3] = (uint8_t) (cr_mean_of16x16_blocks[3] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_4] = (uint8_t) (cr_mean_of16x16_blocks[4] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_5] = (uint8_t) (cr_mean_of16x16_blocks[5] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_6] = (uint8_t) (cr_mean_of16x16_blocks[6] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_7] = (uint8_t) (cr_mean_of16x16_blocks[7] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_8] = (uint8_t) (cr_mean_of16x16_blocks[8] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_9] = (uint8_t) (cr_mean_of16x16_blocks[9] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_10] = (uint8_t) (cr_mean_of16x16_blocks[10] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_11] = (uint8_t) (cr_mean_of16x16_blocks[11] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_12] = (uint8_t) (cr_mean_of16x16_blocks[12] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_13] = (uint8_t) (cr_mean_of16x16_blocks[13] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_14] = (uint8_t) (cr_mean_of16x16_blocks[14] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_16x16_15] = (uint8_t) (cr_mean_of16x16_blocks[15] >> MEAN_PRECISION);

    // 32x32 mean
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_32x32_0] = (uint8_t) (cb_mean_of32x32_blocks[0] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_32x32_1] = (uint8_t) (cb_mean_of32x32_blocks[1] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_32x32_2] = (uint8_t) (cb_mean_of32x32_blocks[2] >> MEAN_PRECISION);
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_32x32_3] = (uint8_t) (cb_mean_of32x32_blocks[3] >> MEAN_PRECISION);

    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_32x32_0] = (uint8_t)(cr_mean_of32x32_blocks[0] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_32x32_1] = (uint8_t)(cr_mean_of32x32_blocks[1] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_32x32_2] = (uint8_t)(cr_mean_of32x32_blocks[2] >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_32x32_3] = (uint8_t)(cr_mean_of32x32_blocks[3] >> MEAN_PRECISION);

    // 64x64 mean
    picture_control_set_ptr->cb_mean[sb_index][ME_TIER_ZERO_PU_64x64] = (uint8_t) (cb_mean_of64x64_blocks >> MEAN_PRECISION);
    picture_control_set_ptr->cr_mean[sb_index][ME_TIER_ZERO_PU_64x64] = (uint8_t) (cr_mean_of64x64_blocks >> MEAN_PRECISION);

    return return_error;
}

/*******************************************
* compute_block_mean_compute_variance
*   computes the variance and the block mean of all CUs inside the tree block
*******************************************/
static EbErrorType compute_block_mean_compute_variance(
    PictureParentControlSet *picture_control_set_ptr,          // input parameter, Picture Control Set Ptr
    EbPictureBufferDesc     *input_padded_picture_ptr,         // input parameter, Input Padded Picture
    uint32_t                 sb_index,                      // input parameter, LCU address
    uint32_t                 input_luma_origin_index)          // input parameter, LCU index, used to point to source/reference samples
{

    EbErrorType return_error = EB_ErrorNone;

    uint32_t block_index;

    uint64_t mean_of8x8_blocks[64];
    uint64_t mean_of8x8_squared_values_blocks[64];

    uint64_t mean_of16x16_blocks[16];
    uint64_t mean_of16x16_squared_values_blocks[16];

    uint64_t mean_of32x32_blocks[4];
    uint64_t mean_of32x32_squared_values_blocks[4];

    uint64_t mean_of64x64_blocks;
    uint64_t mean_of64x64_squared_values_blocks;

    // (0,0)
    block_index = input_luma_origin_index;

    const uint16_t stride_y = input_padded_picture_ptr->stride_y;

    if ((eb_vp9_ASM_TYPES & AVX2_MASK) && 1) {

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[0], &mean_of8x8_squared_values_blocks[0]);

        // (0,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[4], &mean_of8x8_squared_values_blocks[4]);

        // (0,5)
        block_index = block_index + 24;

        // (1,0)
        block_index = input_luma_origin_index + (stride_y << 3);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[8], &mean_of8x8_squared_values_blocks[8]);

        // (1,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[12], &mean_of8x8_squared_values_blocks[12]);

        // (1,5)
        block_index = block_index + 24;

        // (2,0)
        block_index = input_luma_origin_index + (stride_y << 4);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[16], &mean_of8x8_squared_values_blocks[16]);

        // (2,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[20], &mean_of8x8_squared_values_blocks[20]);

        // (2,5)
        block_index = block_index + 24;

        // (3,0)
        block_index = input_luma_origin_index + (stride_y << 3) + (stride_y << 4);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[24], &mean_of8x8_squared_values_blocks[24]);

        // (3,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[28], &mean_of8x8_squared_values_blocks[28]);

        // (3,5)
        block_index = block_index + 24;

        // (4,0)
        block_index = input_luma_origin_index + (stride_y << 5);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[32], &mean_of8x8_squared_values_blocks[32]);

        // (4,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[36], &mean_of8x8_squared_values_blocks[36]);

        // (4,5)
        block_index = block_index + 24;

        // (5,0)
        block_index = input_luma_origin_index + (stride_y << 3) + (stride_y << 5);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[40], &mean_of8x8_squared_values_blocks[40]);

        // (5,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[44], &mean_of8x8_squared_values_blocks[44]);

        // (5,5)
        block_index = block_index + 24;

        // (6,0)
        block_index = input_luma_origin_index + (stride_y << 4) + (stride_y << 5);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[48], &mean_of8x8_squared_values_blocks[48]);

        // (6,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[52], &mean_of8x8_squared_values_blocks[52]);

        // (6,5)
        block_index = block_index + 24;

        // (7,0)
        block_index = input_luma_origin_index + (stride_y << 3) + (stride_y << 4) + (stride_y << 5);

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[56], &mean_of8x8_squared_values_blocks[56]);

        // (7,1)
        block_index = block_index + 32;

        eb_vp9_compute_interm_var_four8x8_avx2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y, &mean_of8x8_blocks[60], &mean_of8x8_squared_values_blocks[60]);

    }
    else {
        mean_of8x8_blocks[0] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[0] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[1] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[1] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[2] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[2] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[3] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[3] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[4] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[4] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[5] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[5] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[6] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[6] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (0,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[7] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[7] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (1,0)
        block_index = input_luma_origin_index + (stride_y << 3);
        mean_of8x8_blocks[8] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[8] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (1,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[9] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[9] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (1,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[10] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[10] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (1,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[11] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[11] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (1,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[12] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[12] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (1,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[13] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[13] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (1,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[14] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[14] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (1,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[15] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[15] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (2,0)
        block_index = input_luma_origin_index + (stride_y << 4);
        mean_of8x8_blocks[16] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[16] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (2,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[17] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[17] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (2,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[18] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[18] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (2,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[19] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[19] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        /// (2,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[20] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[20] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (2,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[21] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[21] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (2,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[22] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[22] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (2,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[23] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[23] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (3,0)
        block_index = input_luma_origin_index + (stride_y << 3) + (stride_y << 4);
        mean_of8x8_blocks[24] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[24] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (3,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[25] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[25] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (3,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[26] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[26] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (3,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[27] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[27] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (3,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[28] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[28] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (3,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[29] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[29] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (3,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[30] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[30] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (3,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[31] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[31] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (4,0)
        block_index = input_luma_origin_index + (stride_y << 5);
        mean_of8x8_blocks[32] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[32] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (4,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[33] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[33] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (4,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[34] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[34] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (4,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[35] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[35] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (4,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[36] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[36] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (4,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[37] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[37] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (4,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[38] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[38] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (4,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[39] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[39] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (5,0)
        block_index = input_luma_origin_index + (stride_y << 3) + (stride_y << 5);
        mean_of8x8_blocks[40] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[40] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (5,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[41] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[41] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (5,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[42] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[42] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (5,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[43] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[43] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (5,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[44] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[44] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (5,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[45] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[45] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (5,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[46] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[46] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (5,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[47] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[47] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (6,0)
        block_index = input_luma_origin_index + (stride_y << 4) + (stride_y << 5);
        mean_of8x8_blocks[48] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[48] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (6,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[49] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[49] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (6,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[50] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[50] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (6,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[51] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[51] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (6,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[52] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[52] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (6,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[53] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[53] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (6,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[54] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[54] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (6,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[55] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[55] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (7,0)
        block_index = input_luma_origin_index + (stride_y << 3) + (stride_y << 4) + (stride_y << 5);
        mean_of8x8_blocks[56] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[56] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (7,1)
        block_index = block_index + 8;
        mean_of8x8_blocks[57] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[57] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (7,2)
        block_index = block_index + 8;
        mean_of8x8_blocks[58] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[58] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (7,3)
        block_index = block_index + 8;
        mean_of8x8_blocks[59] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[59] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (7,4)
        block_index = block_index + 8;
        mean_of8x8_blocks[60] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[60] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (7,5)
        block_index = block_index + 8;
        mean_of8x8_blocks[61] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[61] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (7,6)
        block_index = block_index + 8;
        mean_of8x8_blocks[62] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[62] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);

        // (7,7)
        block_index = block_index + 8;
        mean_of8x8_blocks[63] = eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
        mean_of8x8_squared_values_blocks[63] = eb_vp9_compute_subd_mean_of_squared_values8x8_sse2_intrin(&(input_padded_picture_ptr->buffer_y[block_index]), stride_y);
    }

    // 16x16
    mean_of16x16_blocks[0] = (mean_of8x8_blocks[0] + mean_of8x8_blocks[1] + mean_of8x8_blocks[8] + mean_of8x8_blocks[9]) >> 2;
    mean_of16x16_blocks[1] = (mean_of8x8_blocks[2] + mean_of8x8_blocks[3] + mean_of8x8_blocks[10] + mean_of8x8_blocks[11]) >> 2;
    mean_of16x16_blocks[2] = (mean_of8x8_blocks[4] + mean_of8x8_blocks[5] + mean_of8x8_blocks[12] + mean_of8x8_blocks[13]) >> 2;
    mean_of16x16_blocks[3] = (mean_of8x8_blocks[6] + mean_of8x8_blocks[7] + mean_of8x8_blocks[14] + mean_of8x8_blocks[15]) >> 2;

    mean_of16x16_blocks[4] = (mean_of8x8_blocks[16] + mean_of8x8_blocks[17] + mean_of8x8_blocks[24] + mean_of8x8_blocks[25]) >> 2;
    mean_of16x16_blocks[5] = (mean_of8x8_blocks[18] + mean_of8x8_blocks[19] + mean_of8x8_blocks[26] + mean_of8x8_blocks[27]) >> 2;
    mean_of16x16_blocks[6] = (mean_of8x8_blocks[20] + mean_of8x8_blocks[21] + mean_of8x8_blocks[28] + mean_of8x8_blocks[29]) >> 2;
    mean_of16x16_blocks[7] = (mean_of8x8_blocks[22] + mean_of8x8_blocks[23] + mean_of8x8_blocks[30] + mean_of8x8_blocks[31]) >> 2;

    mean_of16x16_blocks[8] = (mean_of8x8_blocks[32] + mean_of8x8_blocks[33] + mean_of8x8_blocks[40] + mean_of8x8_blocks[41]) >> 2;
    mean_of16x16_blocks[9] = (mean_of8x8_blocks[34] + mean_of8x8_blocks[35] + mean_of8x8_blocks[42] + mean_of8x8_blocks[43]) >> 2;
    mean_of16x16_blocks[10] = (mean_of8x8_blocks[36] + mean_of8x8_blocks[37] + mean_of8x8_blocks[44] + mean_of8x8_blocks[45]) >> 2;
    mean_of16x16_blocks[11] = (mean_of8x8_blocks[38] + mean_of8x8_blocks[39] + mean_of8x8_blocks[46] + mean_of8x8_blocks[47]) >> 2;

    mean_of16x16_blocks[12] = (mean_of8x8_blocks[48] + mean_of8x8_blocks[49] + mean_of8x8_blocks[56] + mean_of8x8_blocks[57]) >> 2;
    mean_of16x16_blocks[13] = (mean_of8x8_blocks[50] + mean_of8x8_blocks[51] + mean_of8x8_blocks[58] + mean_of8x8_blocks[59]) >> 2;
    mean_of16x16_blocks[14] = (mean_of8x8_blocks[52] + mean_of8x8_blocks[53] + mean_of8x8_blocks[60] + mean_of8x8_blocks[61]) >> 2;
    mean_of16x16_blocks[15] = (mean_of8x8_blocks[54] + mean_of8x8_blocks[55] + mean_of8x8_blocks[62] + mean_of8x8_blocks[63]) >> 2;

    mean_of16x16_squared_values_blocks[0] = (mean_of8x8_squared_values_blocks[0] + mean_of8x8_squared_values_blocks[1] + mean_of8x8_squared_values_blocks[8] + mean_of8x8_squared_values_blocks[9]) >> 2;
    mean_of16x16_squared_values_blocks[1] = (mean_of8x8_squared_values_blocks[2] + mean_of8x8_squared_values_blocks[3] + mean_of8x8_squared_values_blocks[10] + mean_of8x8_squared_values_blocks[11]) >> 2;
    mean_of16x16_squared_values_blocks[2] = (mean_of8x8_squared_values_blocks[4] + mean_of8x8_squared_values_blocks[5] + mean_of8x8_squared_values_blocks[12] + mean_of8x8_squared_values_blocks[13]) >> 2;
    mean_of16x16_squared_values_blocks[3] = (mean_of8x8_squared_values_blocks[6] + mean_of8x8_squared_values_blocks[7] + mean_of8x8_squared_values_blocks[14] + mean_of8x8_squared_values_blocks[15]) >> 2;

    mean_of16x16_squared_values_blocks[4] = (mean_of8x8_squared_values_blocks[16] + mean_of8x8_squared_values_blocks[17] + mean_of8x8_squared_values_blocks[24] + mean_of8x8_squared_values_blocks[25]) >> 2;
    mean_of16x16_squared_values_blocks[5] = (mean_of8x8_squared_values_blocks[18] + mean_of8x8_squared_values_blocks[19] + mean_of8x8_squared_values_blocks[26] + mean_of8x8_squared_values_blocks[27]) >> 2;
    mean_of16x16_squared_values_blocks[6] = (mean_of8x8_squared_values_blocks[20] + mean_of8x8_squared_values_blocks[21] + mean_of8x8_squared_values_blocks[28] + mean_of8x8_squared_values_blocks[29]) >> 2;
    mean_of16x16_squared_values_blocks[7] = (mean_of8x8_squared_values_blocks[22] + mean_of8x8_squared_values_blocks[23] + mean_of8x8_squared_values_blocks[30] + mean_of8x8_squared_values_blocks[31]) >> 2;

    mean_of16x16_squared_values_blocks[8] = (mean_of8x8_squared_values_blocks[32] + mean_of8x8_squared_values_blocks[33] + mean_of8x8_squared_values_blocks[40] + mean_of8x8_squared_values_blocks[41]) >> 2;
    mean_of16x16_squared_values_blocks[9] = (mean_of8x8_squared_values_blocks[34] + mean_of8x8_squared_values_blocks[35] + mean_of8x8_squared_values_blocks[42] + mean_of8x8_squared_values_blocks[43]) >> 2;
    mean_of16x16_squared_values_blocks[10] = (mean_of8x8_squared_values_blocks[36] + mean_of8x8_squared_values_blocks[37] + mean_of8x8_squared_values_blocks[44] + mean_of8x8_squared_values_blocks[45]) >> 2;
    mean_of16x16_squared_values_blocks[11] = (mean_of8x8_squared_values_blocks[38] + mean_of8x8_squared_values_blocks[39] + mean_of8x8_squared_values_blocks[46] + mean_of8x8_squared_values_blocks[47]) >> 2;

    mean_of16x16_squared_values_blocks[12] = (mean_of8x8_squared_values_blocks[48] + mean_of8x8_squared_values_blocks[49] + mean_of8x8_squared_values_blocks[56] + mean_of8x8_squared_values_blocks[57]) >> 2;
    mean_of16x16_squared_values_blocks[13] = (mean_of8x8_squared_values_blocks[50] + mean_of8x8_squared_values_blocks[51] + mean_of8x8_squared_values_blocks[58] + mean_of8x8_squared_values_blocks[59]) >> 2;
    mean_of16x16_squared_values_blocks[14] = (mean_of8x8_squared_values_blocks[52] + mean_of8x8_squared_values_blocks[53] + mean_of8x8_squared_values_blocks[60] + mean_of8x8_squared_values_blocks[61]) >> 2;
    mean_of16x16_squared_values_blocks[15] = (mean_of8x8_squared_values_blocks[54] + mean_of8x8_squared_values_blocks[55] + mean_of8x8_squared_values_blocks[62] + mean_of8x8_squared_values_blocks[63]) >> 2;

    // 32x32
    mean_of32x32_blocks[0] = (mean_of16x16_blocks[0] + mean_of16x16_blocks[1] + mean_of16x16_blocks[4] + mean_of16x16_blocks[5]) >> 2;
    mean_of32x32_blocks[1] = (mean_of16x16_blocks[2] + mean_of16x16_blocks[3] + mean_of16x16_blocks[6] + mean_of16x16_blocks[7]) >> 2;
    mean_of32x32_blocks[2] = (mean_of16x16_blocks[8] + mean_of16x16_blocks[9] + mean_of16x16_blocks[12] + mean_of16x16_blocks[13]) >> 2;
    mean_of32x32_blocks[3] = (mean_of16x16_blocks[10] + mean_of16x16_blocks[11] + mean_of16x16_blocks[14] + mean_of16x16_blocks[15]) >> 2;

    mean_of32x32_squared_values_blocks[0] = (mean_of16x16_squared_values_blocks[0] + mean_of16x16_squared_values_blocks[1] + mean_of16x16_squared_values_blocks[4] + mean_of16x16_squared_values_blocks[5]) >> 2;
    mean_of32x32_squared_values_blocks[1] = (mean_of16x16_squared_values_blocks[2] + mean_of16x16_squared_values_blocks[3] + mean_of16x16_squared_values_blocks[6] + mean_of16x16_squared_values_blocks[7]) >> 2;
    mean_of32x32_squared_values_blocks[2] = (mean_of16x16_squared_values_blocks[8] + mean_of16x16_squared_values_blocks[9] + mean_of16x16_squared_values_blocks[12] + mean_of16x16_squared_values_blocks[13]) >> 2;
    mean_of32x32_squared_values_blocks[3] = (mean_of16x16_squared_values_blocks[10] + mean_of16x16_squared_values_blocks[11] + mean_of16x16_squared_values_blocks[14] + mean_of16x16_squared_values_blocks[15]) >> 2;

    // 64x64
    mean_of64x64_blocks = (mean_of32x32_blocks[0] + mean_of32x32_blocks[1] + mean_of32x32_blocks[2] + mean_of32x32_blocks[3]) >> 2;
    mean_of64x64_squared_values_blocks = (mean_of32x32_squared_values_blocks[0] + mean_of32x32_squared_values_blocks[1] + mean_of32x32_squared_values_blocks[2] + mean_of32x32_squared_values_blocks[3]) >> 2;

    // 8x8 means
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_0] = (uint8_t)(mean_of8x8_blocks[0] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_1] = (uint8_t)(mean_of8x8_blocks[1] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_2] = (uint8_t)(mean_of8x8_blocks[2] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_3] = (uint8_t)(mean_of8x8_blocks[3] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_4] = (uint8_t)(mean_of8x8_blocks[4] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_5] = (uint8_t)(mean_of8x8_blocks[5] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_6] = (uint8_t)(mean_of8x8_blocks[6] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_7] = (uint8_t)(mean_of8x8_blocks[7] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_8] = (uint8_t)(mean_of8x8_blocks[8] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_9] = (uint8_t)(mean_of8x8_blocks[9] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_10] = (uint8_t)(mean_of8x8_blocks[10] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_11] = (uint8_t)(mean_of8x8_blocks[11] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_12] = (uint8_t)(mean_of8x8_blocks[12] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_13] = (uint8_t)(mean_of8x8_blocks[13] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_14] = (uint8_t)(mean_of8x8_blocks[14] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_15] = (uint8_t)(mean_of8x8_blocks[15] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_16] = (uint8_t)(mean_of8x8_blocks[16] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_17] = (uint8_t)(mean_of8x8_blocks[17] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_18] = (uint8_t)(mean_of8x8_blocks[18] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_19] = (uint8_t)(mean_of8x8_blocks[19] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_20] = (uint8_t)(mean_of8x8_blocks[20] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_21] = (uint8_t)(mean_of8x8_blocks[21] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_22] = (uint8_t)(mean_of8x8_blocks[22] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_23] = (uint8_t)(mean_of8x8_blocks[23] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_24] = (uint8_t)(mean_of8x8_blocks[24] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_25] = (uint8_t)(mean_of8x8_blocks[25] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_26] = (uint8_t)(mean_of8x8_blocks[26] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_27] = (uint8_t)(mean_of8x8_blocks[27] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_28] = (uint8_t)(mean_of8x8_blocks[28] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_29] = (uint8_t)(mean_of8x8_blocks[29] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_30] = (uint8_t)(mean_of8x8_blocks[30] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_31] = (uint8_t)(mean_of8x8_blocks[31] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_32] = (uint8_t)(mean_of8x8_blocks[32] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_33] = (uint8_t)(mean_of8x8_blocks[33] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_34] = (uint8_t)(mean_of8x8_blocks[34] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_35] = (uint8_t)(mean_of8x8_blocks[35] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_36] = (uint8_t)(mean_of8x8_blocks[36] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_37] = (uint8_t)(mean_of8x8_blocks[37] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_38] = (uint8_t)(mean_of8x8_blocks[38] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_39] = (uint8_t)(mean_of8x8_blocks[39] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_40] = (uint8_t)(mean_of8x8_blocks[40] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_41] = (uint8_t)(mean_of8x8_blocks[41] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_42] = (uint8_t)(mean_of8x8_blocks[42] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_43] = (uint8_t)(mean_of8x8_blocks[43] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_44] = (uint8_t)(mean_of8x8_blocks[44] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_45] = (uint8_t)(mean_of8x8_blocks[45] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_46] = (uint8_t)(mean_of8x8_blocks[46] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_47] = (uint8_t)(mean_of8x8_blocks[47] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_48] = (uint8_t)(mean_of8x8_blocks[48] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_49] = (uint8_t)(mean_of8x8_blocks[49] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_50] = (uint8_t)(mean_of8x8_blocks[50] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_51] = (uint8_t)(mean_of8x8_blocks[51] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_52] = (uint8_t)(mean_of8x8_blocks[52] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_53] = (uint8_t)(mean_of8x8_blocks[53] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_54] = (uint8_t)(mean_of8x8_blocks[54] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_55] = (uint8_t)(mean_of8x8_blocks[55] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_56] = (uint8_t)(mean_of8x8_blocks[56] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_57] = (uint8_t)(mean_of8x8_blocks[57] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_58] = (uint8_t)(mean_of8x8_blocks[58] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_59] = (uint8_t)(mean_of8x8_blocks[59] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_60] = (uint8_t)(mean_of8x8_blocks[60] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_61] = (uint8_t)(mean_of8x8_blocks[61] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_62] = (uint8_t)(mean_of8x8_blocks[62] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_8x8_63] = (uint8_t)(mean_of8x8_blocks[63] >> MEAN_PRECISION);

    // 16x16 mean
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_0] = (uint8_t)(mean_of16x16_blocks[0] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_1] = (uint8_t)(mean_of16x16_blocks[1] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_2] = (uint8_t)(mean_of16x16_blocks[2] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_3] = (uint8_t)(mean_of16x16_blocks[3] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_4] = (uint8_t)(mean_of16x16_blocks[4] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_5] = (uint8_t)(mean_of16x16_blocks[5] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_6] = (uint8_t)(mean_of16x16_blocks[6] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_7] = (uint8_t)(mean_of16x16_blocks[7] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_8] = (uint8_t)(mean_of16x16_blocks[8] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_9] = (uint8_t)(mean_of16x16_blocks[9] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_10] = (uint8_t)(mean_of16x16_blocks[10] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_11] = (uint8_t)(mean_of16x16_blocks[11] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_12] = (uint8_t)(mean_of16x16_blocks[12] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_13] = (uint8_t)(mean_of16x16_blocks[13] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_14] = (uint8_t)(mean_of16x16_blocks[14] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_16x16_15] = (uint8_t)(mean_of16x16_blocks[15] >> MEAN_PRECISION);

    // 32x32 mean
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_32x32_0] = (uint8_t)(mean_of32x32_blocks[0] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_32x32_1] = (uint8_t)(mean_of32x32_blocks[1] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_32x32_2] = (uint8_t)(mean_of32x32_blocks[2] >> MEAN_PRECISION);
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_32x32_3] = (uint8_t)(mean_of32x32_blocks[3] >> MEAN_PRECISION);

    // 64x64 mean
    picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_64x64] = (uint8_t)(mean_of64x64_blocks >> MEAN_PRECISION);

    // 8x8 variances
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_0] = (uint16_t)((mean_of8x8_squared_values_blocks[0] - (mean_of8x8_blocks[0] * mean_of8x8_blocks[0])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_1] = (uint16_t)((mean_of8x8_squared_values_blocks[1] - (mean_of8x8_blocks[1] * mean_of8x8_blocks[1])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_2] = (uint16_t)((mean_of8x8_squared_values_blocks[2] - (mean_of8x8_blocks[2] * mean_of8x8_blocks[2])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_3] = (uint16_t)((mean_of8x8_squared_values_blocks[3] - (mean_of8x8_blocks[3] * mean_of8x8_blocks[3])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_4] = (uint16_t)((mean_of8x8_squared_values_blocks[4] - (mean_of8x8_blocks[4] * mean_of8x8_blocks[4])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_5] = (uint16_t)((mean_of8x8_squared_values_blocks[5] - (mean_of8x8_blocks[5] * mean_of8x8_blocks[5])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_6] = (uint16_t)((mean_of8x8_squared_values_blocks[6] - (mean_of8x8_blocks[6] * mean_of8x8_blocks[6])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_7] = (uint16_t)((mean_of8x8_squared_values_blocks[7] - (mean_of8x8_blocks[7] * mean_of8x8_blocks[7])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_8] = (uint16_t)((mean_of8x8_squared_values_blocks[8] - (mean_of8x8_blocks[8] * mean_of8x8_blocks[8])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_9] = (uint16_t)((mean_of8x8_squared_values_blocks[9] - (mean_of8x8_blocks[9] * mean_of8x8_blocks[9])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_10] = (uint16_t)((mean_of8x8_squared_values_blocks[10] - (mean_of8x8_blocks[10] * mean_of8x8_blocks[10])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_11] = (uint16_t)((mean_of8x8_squared_values_blocks[11] - (mean_of8x8_blocks[11] * mean_of8x8_blocks[11])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_12] = (uint16_t)((mean_of8x8_squared_values_blocks[12] - (mean_of8x8_blocks[12] * mean_of8x8_blocks[12])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_13] = (uint16_t)((mean_of8x8_squared_values_blocks[13] - (mean_of8x8_blocks[13] * mean_of8x8_blocks[13])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_14] = (uint16_t)((mean_of8x8_squared_values_blocks[14] - (mean_of8x8_blocks[14] * mean_of8x8_blocks[14])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_15] = (uint16_t)((mean_of8x8_squared_values_blocks[15] - (mean_of8x8_blocks[15] * mean_of8x8_blocks[15])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_16] = (uint16_t)((mean_of8x8_squared_values_blocks[16] - (mean_of8x8_blocks[16] * mean_of8x8_blocks[16])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_17] = (uint16_t)((mean_of8x8_squared_values_blocks[17] - (mean_of8x8_blocks[17] * mean_of8x8_blocks[17])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_18] = (uint16_t)((mean_of8x8_squared_values_blocks[18] - (mean_of8x8_blocks[18] * mean_of8x8_blocks[18])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_19] = (uint16_t)((mean_of8x8_squared_values_blocks[19] - (mean_of8x8_blocks[19] * mean_of8x8_blocks[19])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_20] = (uint16_t)((mean_of8x8_squared_values_blocks[20] - (mean_of8x8_blocks[20] * mean_of8x8_blocks[20])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_21] = (uint16_t)((mean_of8x8_squared_values_blocks[21] - (mean_of8x8_blocks[21] * mean_of8x8_blocks[21])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_22] = (uint16_t)((mean_of8x8_squared_values_blocks[22] - (mean_of8x8_blocks[22] * mean_of8x8_blocks[22])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_23] = (uint16_t)((mean_of8x8_squared_values_blocks[23] - (mean_of8x8_blocks[23] * mean_of8x8_blocks[23])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_24] = (uint16_t)((mean_of8x8_squared_values_blocks[24] - (mean_of8x8_blocks[24] * mean_of8x8_blocks[24])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_25] = (uint16_t)((mean_of8x8_squared_values_blocks[25] - (mean_of8x8_blocks[25] * mean_of8x8_blocks[25])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_26] = (uint16_t)((mean_of8x8_squared_values_blocks[26] - (mean_of8x8_blocks[26] * mean_of8x8_blocks[26])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_27] = (uint16_t)((mean_of8x8_squared_values_blocks[27] - (mean_of8x8_blocks[27] * mean_of8x8_blocks[27])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_28] = (uint16_t)((mean_of8x8_squared_values_blocks[28] - (mean_of8x8_blocks[28] * mean_of8x8_blocks[28])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_29] = (uint16_t)((mean_of8x8_squared_values_blocks[29] - (mean_of8x8_blocks[29] * mean_of8x8_blocks[29])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_30] = (uint16_t)((mean_of8x8_squared_values_blocks[30] - (mean_of8x8_blocks[30] * mean_of8x8_blocks[30])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_31] = (uint16_t)((mean_of8x8_squared_values_blocks[31] - (mean_of8x8_blocks[31] * mean_of8x8_blocks[31])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_32] = (uint16_t)((mean_of8x8_squared_values_blocks[32] - (mean_of8x8_blocks[32] * mean_of8x8_blocks[32])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_33] = (uint16_t)((mean_of8x8_squared_values_blocks[33] - (mean_of8x8_blocks[33] * mean_of8x8_blocks[33])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_34] = (uint16_t)((mean_of8x8_squared_values_blocks[34] - (mean_of8x8_blocks[34] * mean_of8x8_blocks[34])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_35] = (uint16_t)((mean_of8x8_squared_values_blocks[35] - (mean_of8x8_blocks[35] * mean_of8x8_blocks[35])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_36] = (uint16_t)((mean_of8x8_squared_values_blocks[36] - (mean_of8x8_blocks[36] * mean_of8x8_blocks[36])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_37] = (uint16_t)((mean_of8x8_squared_values_blocks[37] - (mean_of8x8_blocks[37] * mean_of8x8_blocks[37])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_38] = (uint16_t)((mean_of8x8_squared_values_blocks[38] - (mean_of8x8_blocks[38] * mean_of8x8_blocks[38])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_39] = (uint16_t)((mean_of8x8_squared_values_blocks[39] - (mean_of8x8_blocks[39] * mean_of8x8_blocks[39])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_40] = (uint16_t)((mean_of8x8_squared_values_blocks[40] - (mean_of8x8_blocks[40] * mean_of8x8_blocks[40])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_41] = (uint16_t)((mean_of8x8_squared_values_blocks[41] - (mean_of8x8_blocks[41] * mean_of8x8_blocks[41])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_42] = (uint16_t)((mean_of8x8_squared_values_blocks[42] - (mean_of8x8_blocks[42] * mean_of8x8_blocks[42])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_43] = (uint16_t)((mean_of8x8_squared_values_blocks[43] - (mean_of8x8_blocks[43] * mean_of8x8_blocks[43])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_44] = (uint16_t)((mean_of8x8_squared_values_blocks[44] - (mean_of8x8_blocks[44] * mean_of8x8_blocks[44])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_45] = (uint16_t)((mean_of8x8_squared_values_blocks[45] - (mean_of8x8_blocks[45] * mean_of8x8_blocks[45])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_46] = (uint16_t)((mean_of8x8_squared_values_blocks[46] - (mean_of8x8_blocks[46] * mean_of8x8_blocks[46])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_47] = (uint16_t)((mean_of8x8_squared_values_blocks[47] - (mean_of8x8_blocks[47] * mean_of8x8_blocks[47])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_48] = (uint16_t)((mean_of8x8_squared_values_blocks[48] - (mean_of8x8_blocks[48] * mean_of8x8_blocks[48])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_49] = (uint16_t)((mean_of8x8_squared_values_blocks[49] - (mean_of8x8_blocks[49] * mean_of8x8_blocks[49])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_50] = (uint16_t)((mean_of8x8_squared_values_blocks[50] - (mean_of8x8_blocks[50] * mean_of8x8_blocks[50])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_51] = (uint16_t)((mean_of8x8_squared_values_blocks[51] - (mean_of8x8_blocks[51] * mean_of8x8_blocks[51])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_52] = (uint16_t)((mean_of8x8_squared_values_blocks[52] - (mean_of8x8_blocks[52] * mean_of8x8_blocks[52])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_53] = (uint16_t)((mean_of8x8_squared_values_blocks[53] - (mean_of8x8_blocks[53] * mean_of8x8_blocks[53])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_54] = (uint16_t)((mean_of8x8_squared_values_blocks[54] - (mean_of8x8_blocks[54] * mean_of8x8_blocks[54])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_55] = (uint16_t)((mean_of8x8_squared_values_blocks[55] - (mean_of8x8_blocks[55] * mean_of8x8_blocks[55])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_56] = (uint16_t)((mean_of8x8_squared_values_blocks[56] - (mean_of8x8_blocks[56] * mean_of8x8_blocks[56])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_57] = (uint16_t)((mean_of8x8_squared_values_blocks[57] - (mean_of8x8_blocks[57] * mean_of8x8_blocks[57])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_58] = (uint16_t)((mean_of8x8_squared_values_blocks[58] - (mean_of8x8_blocks[58] * mean_of8x8_blocks[58])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_59] = (uint16_t)((mean_of8x8_squared_values_blocks[59] - (mean_of8x8_blocks[59] * mean_of8x8_blocks[59])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_60] = (uint16_t)((mean_of8x8_squared_values_blocks[60] - (mean_of8x8_blocks[60] * mean_of8x8_blocks[60])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_61] = (uint16_t)((mean_of8x8_squared_values_blocks[61] - (mean_of8x8_blocks[61] * mean_of8x8_blocks[61])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_62] = (uint16_t)((mean_of8x8_squared_values_blocks[62] - (mean_of8x8_blocks[62] * mean_of8x8_blocks[62])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_8x8_63] = (uint16_t)((mean_of8x8_squared_values_blocks[63] - (mean_of8x8_blocks[63] * mean_of8x8_blocks[63])) >> VARIANCE_PRECISION);

    // 16x16 variances
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_0] = (uint16_t)((mean_of16x16_squared_values_blocks[0] - (mean_of16x16_blocks[0] * mean_of16x16_blocks[0])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_1] = (uint16_t)((mean_of16x16_squared_values_blocks[1] - (mean_of16x16_blocks[1] * mean_of16x16_blocks[1])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_2] = (uint16_t)((mean_of16x16_squared_values_blocks[2] - (mean_of16x16_blocks[2] * mean_of16x16_blocks[2])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_3] = (uint16_t)((mean_of16x16_squared_values_blocks[3] - (mean_of16x16_blocks[3] * mean_of16x16_blocks[3])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_4] = (uint16_t)((mean_of16x16_squared_values_blocks[4] - (mean_of16x16_blocks[4] * mean_of16x16_blocks[4])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_5] = (uint16_t)((mean_of16x16_squared_values_blocks[5] - (mean_of16x16_blocks[5] * mean_of16x16_blocks[5])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_6] = (uint16_t)((mean_of16x16_squared_values_blocks[6] - (mean_of16x16_blocks[6] * mean_of16x16_blocks[6])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_7] = (uint16_t)((mean_of16x16_squared_values_blocks[7] - (mean_of16x16_blocks[7] * mean_of16x16_blocks[7])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_8] = (uint16_t)((mean_of16x16_squared_values_blocks[8] - (mean_of16x16_blocks[8] * mean_of16x16_blocks[8])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_9] = (uint16_t)((mean_of16x16_squared_values_blocks[9] - (mean_of16x16_blocks[9] * mean_of16x16_blocks[9])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_10] = (uint16_t)((mean_of16x16_squared_values_blocks[10] - (mean_of16x16_blocks[10] * mean_of16x16_blocks[10])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_11] = (uint16_t)((mean_of16x16_squared_values_blocks[11] - (mean_of16x16_blocks[11] * mean_of16x16_blocks[11])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_12] = (uint16_t)((mean_of16x16_squared_values_blocks[12] - (mean_of16x16_blocks[12] * mean_of16x16_blocks[12])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_13] = (uint16_t)((mean_of16x16_squared_values_blocks[13] - (mean_of16x16_blocks[13] * mean_of16x16_blocks[13])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_14] = (uint16_t)((mean_of16x16_squared_values_blocks[14] - (mean_of16x16_blocks[14] * mean_of16x16_blocks[14])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_16x16_15] = (uint16_t)((mean_of16x16_squared_values_blocks[15] - (mean_of16x16_blocks[15] * mean_of16x16_blocks[15])) >> VARIANCE_PRECISION);

    // 32x32 variances
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_32x32_0] = (uint16_t)((mean_of32x32_squared_values_blocks[0] - (mean_of32x32_blocks[0] * mean_of32x32_blocks[0])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_32x32_1] = (uint16_t)((mean_of32x32_squared_values_blocks[1] - (mean_of32x32_blocks[1] * mean_of32x32_blocks[1])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_32x32_2] = (uint16_t)((mean_of32x32_squared_values_blocks[2] - (mean_of32x32_blocks[2] * mean_of32x32_blocks[2])) >> VARIANCE_PRECISION);
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_32x32_3] = (uint16_t)((mean_of32x32_squared_values_blocks[3] - (mean_of32x32_blocks[3] * mean_of32x32_blocks[3])) >> VARIANCE_PRECISION);

    // 64x64 variance
    picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_64x64] = (uint16_t)((mean_of64x64_squared_values_blocks - (mean_of64x64_blocks * mean_of64x64_blocks)) >> VARIANCE_PRECISION);

    return return_error;
}

static EbErrorType denoise_input_picture(
    PictureAnalysisContext  *context_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *input_picture_ptr,
    EbPictureBufferDesc     *denoised_picture_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    uint32_t       sb_index;
    uint32_t       sb_origin_x;
    uint32_t       sb_origin_y;
    uint16_t       vertical_idx;
    //use denoised input if the source is extremly noisy
    if (picture_control_set_ptr->pic_noise_class >= PIC_NOISE_CLASS_4){

        uint32_t in_luma_off_set = input_picture_ptr->origin_x + input_picture_ptr->origin_y      * input_picture_ptr->stride_y;
        uint32_t in_chroma_off_set = input_picture_ptr->origin_x / 2 + input_picture_ptr->origin_y / 2 * input_picture_ptr->stride_cb;
        uint32_t den_luma_off_set = denoised_picture_ptr->origin_x + denoised_picture_ptr->origin_y   * denoised_picture_ptr->stride_y;
        uint32_t den_chroma_off_set = denoised_picture_ptr->origin_x / 2 + denoised_picture_ptr->origin_y / 2 * denoised_picture_ptr->stride_cb;

        //filter Luma
        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

            sb_origin_x = sb_params->origin_x;
            sb_origin_y = sb_params->origin_y;

            if (sb_origin_x == 0)
                strong_luma_filter_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
                input_picture_ptr,
                denoised_picture_ptr,
                sb_origin_y,
                sb_origin_x);

            if (sb_origin_x + MAX_SB_SIZE > input_picture_ptr->width)
            {
                eb_vp9_noise_extract_luma_strong(
                    input_picture_ptr,
                    denoised_picture_ptr,
                    sb_origin_y,
                    sb_origin_x);
            }

        }

        //copy Luma
        for (vertical_idx = 0; vertical_idx < input_picture_ptr->height; ++vertical_idx) {
            EB_MEMCPY(input_picture_ptr->buffer_y + in_luma_off_set + vertical_idx * input_picture_ptr->stride_y,
                denoised_picture_ptr->buffer_y + den_luma_off_set + vertical_idx * denoised_picture_ptr->stride_y,
                sizeof(uint8_t) * input_picture_ptr->width);
        }

        //copy chroma
        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

            sb_origin_x = sb_params->origin_x;
            sb_origin_y = sb_params->origin_y;

            if (sb_origin_x == 0)
                strong_chroma_filter_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
                input_picture_ptr,
                denoised_picture_ptr,
                sb_origin_y / 2,
                sb_origin_x / 2);

            if (sb_origin_x + MAX_SB_SIZE > input_picture_ptr->width)
            {
                eb_vp9_noise_extract_chroma_strong(
                    input_picture_ptr,
                    denoised_picture_ptr,
                    sb_origin_y / 2,
                    sb_origin_x / 2);
            }

        }

        //copy chroma
        for (vertical_idx = 0; vertical_idx < input_picture_ptr->height / 2; ++vertical_idx) {

            EB_MEMCPY(input_picture_ptr->buffer_cb + in_chroma_off_set + vertical_idx * input_picture_ptr->stride_cb,
                denoised_picture_ptr->buffer_cb + den_chroma_off_set + vertical_idx * denoised_picture_ptr->stride_cb,
                sizeof(uint8_t) * input_picture_ptr->width / 2);

            EB_MEMCPY(input_picture_ptr->buffer_cr + in_chroma_off_set + vertical_idx * input_picture_ptr->stride_cr,
                denoised_picture_ptr->buffer_cr + den_chroma_off_set + vertical_idx * denoised_picture_ptr->stride_cr,
                sizeof(uint8_t) * input_picture_ptr->width / 2);
        }

    }
    else if (picture_control_set_ptr->pic_noise_class >= PIC_NOISE_CLASS_3_1){

        uint32_t in_luma_off_set = input_picture_ptr->origin_x + input_picture_ptr->origin_y      * input_picture_ptr->stride_y;
        uint32_t in_chroma_off_set = input_picture_ptr->origin_x / 2 + input_picture_ptr->origin_y / 2 * input_picture_ptr->stride_cb;
        uint32_t den_luma_off_set = denoised_picture_ptr->origin_x + denoised_picture_ptr->origin_y   * denoised_picture_ptr->stride_y;
        uint32_t den_chroma_off_set = denoised_picture_ptr->origin_x / 2 + denoised_picture_ptr->origin_y / 2 * denoised_picture_ptr->stride_cb;

        for (vertical_idx = 0; vertical_idx < input_picture_ptr->height; ++vertical_idx) {
            EB_MEMCPY(input_picture_ptr->buffer_y + in_luma_off_set + vertical_idx * input_picture_ptr->stride_y,
                denoised_picture_ptr->buffer_y + den_luma_off_set + vertical_idx * denoised_picture_ptr->stride_y,
                sizeof(uint8_t) * input_picture_ptr->width);
        }

        //copy chroma
        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

            sb_origin_x = sb_params->origin_x;
            sb_origin_y = sb_params->origin_y;

            if (sb_origin_x == 0)
                weak_chroma_filter_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
                input_picture_ptr,
                denoised_picture_ptr,
                sb_origin_y / 2,
                sb_origin_x / 2);

            if (sb_origin_x + MAX_SB_SIZE > input_picture_ptr->width)
            {
                eb_vp9_noise_extract_chroma_weak(
                    input_picture_ptr,
                    denoised_picture_ptr,
                    sb_origin_y / 2,
                    sb_origin_x / 2);
            }

        }

        for (vertical_idx = 0; vertical_idx < input_picture_ptr->height / 2; ++vertical_idx) {

            EB_MEMCPY(input_picture_ptr->buffer_cb + in_chroma_off_set + vertical_idx * input_picture_ptr->stride_cb,
                denoised_picture_ptr->buffer_cb + den_chroma_off_set + vertical_idx * denoised_picture_ptr->stride_cb,
                sizeof(uint8_t) * input_picture_ptr->width / 2);

            EB_MEMCPY(input_picture_ptr->buffer_cr + in_chroma_off_set + vertical_idx * input_picture_ptr->stride_cr,
                denoised_picture_ptr->buffer_cr + den_chroma_off_set + vertical_idx * denoised_picture_ptr->stride_cr,
                sizeof(uint8_t) * input_picture_ptr->width / 2);
        }

    }

    else if (context_ptr->pic_noise_variance_float >= 1.0  && sequence_control_set_ptr->input_resolution == INPUT_SIZE_4K_RANGE) {

        //Luma : use filtered only for flatNoise LCUs
        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

            sb_origin_x = sb_params->origin_x;
            sb_origin_y = sb_params->origin_y;
            // to use sb params here
            uint32_t  sb_height = MIN(MAX_SB_SIZE, input_picture_ptr->height - sb_origin_y);
            uint32_t  sb_width = MIN(MAX_SB_SIZE, input_picture_ptr->width - sb_origin_x);

            uint32_t in_luma_off_set = input_picture_ptr->origin_x + sb_origin_x + (input_picture_ptr->origin_y + sb_origin_y) * input_picture_ptr->stride_y;
            uint32_t den_luma_off_set = denoised_picture_ptr->origin_x + sb_origin_x + (denoised_picture_ptr->origin_y + sb_origin_y) * denoised_picture_ptr->stride_y;

            if (picture_control_set_ptr->sb_flat_noise_array[sb_index] == 1){

                for (vertical_idx = 0; vertical_idx < sb_height; ++vertical_idx) {

                    EB_MEMCPY(input_picture_ptr->buffer_y + in_luma_off_set + vertical_idx * input_picture_ptr->stride_y,
                        denoised_picture_ptr->buffer_y + den_luma_off_set + vertical_idx * denoised_picture_ptr->stride_y,
                        sizeof(uint8_t) * sb_width);

                }
            }
        }
    }

    return return_error;
}

static EbErrorType detect_input_picture_noise(
    PictureAnalysisContext  *context_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *input_picture_ptr,
    EbPictureBufferDesc     *noise_picture_ptr,
    EbPictureBufferDesc     *denoised_picture_ptr)
{

    EbErrorType return_error = EB_ErrorNone;
    uint32_t sb_index;

    uint64_t pic_noise_variance;

    uint32_t tot_sb_count, noise_th;

    uint32_t sb_origin_x;
    uint32_t sb_origin_y;
    uint32_t input_luma_origin_index;

    pic_noise_variance = 0;
    tot_sb_count = 0;

    //Variance calc for noise picture
    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

        sb_origin_x = sb_params->origin_x;
        sb_origin_y = sb_params->origin_y;
        input_luma_origin_index = (noise_picture_ptr->origin_y + sb_origin_y) * noise_picture_ptr->stride_y +
            noise_picture_ptr->origin_x + sb_origin_x;

        uint32_t  noise_origin_index = noise_picture_ptr->origin_x + sb_origin_x + noise_picture_ptr->origin_y * noise_picture_ptr->stride_y;

        if (sb_origin_x == 0)
            weak_luma_filter_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
            input_picture_ptr,
            denoised_picture_ptr,
            noise_picture_ptr,
            sb_origin_y,
            sb_origin_x);

        if (sb_origin_x + MAX_SB_SIZE > input_picture_ptr->width)
        {
            eb_vp9_noise_extract_luma_weak(
                input_picture_ptr,
                denoised_picture_ptr,
                noise_picture_ptr,
                sb_origin_y,
                sb_origin_x);
        }

        //do it only for complete 64x64 blocks
        if (sb_params->is_complete_sb)
        {

            uint64_t noiseBlkVar32x32[4], denoiseBlkVar32x32[4];

            uint64_t noiseBlkVar = compute_variance64x64(
                noise_picture_ptr,
                noise_origin_index,
                noiseBlkVar32x32);

            uint64_t noiseBlkVarTh ;
            uint64_t denBlkVarTh = FLAT_MAX_VAR;

            if (picture_control_set_ptr->noise_detection_th == 1)
                noiseBlkVarTh = NOISE_MIN_LEVEL_0;
            else
                noiseBlkVarTh = NOISE_MIN_LEVEL_1;

            pic_noise_variance += (noiseBlkVar >> 16);

            uint64_t den_blk_var = compute_variance64x64(
                denoised_picture_ptr,
                input_luma_origin_index,
                denoiseBlkVar32x32) >> 16;

            if (den_blk_var < denBlkVarTh && noiseBlkVar > noiseBlkVarTh) {
                picture_control_set_ptr->sb_flat_noise_array[sb_index] = 1;
            }

            tot_sb_count++;
        }

    }

    if (tot_sb_count > 0) {
        context_ptr->pic_noise_variance_float = (double)pic_noise_variance / (double)tot_sb_count;
        pic_noise_variance = pic_noise_variance / tot_sb_count;
    }

    //the variance of a 64x64 noise area tends to be bigger for small resolutions.
    if (sequence_control_set_ptr->luma_height <= 720)
        noise_th = 25;
    else
        noise_th = 0;

    if (pic_noise_variance >= 80 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_10;
    else if (pic_noise_variance >= 70 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_9;
    else if (pic_noise_variance >= 60 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_8;
    else if (pic_noise_variance >= 50 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_7;
    else if (pic_noise_variance >= 40 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_6;
    else if (pic_noise_variance >= 30 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_5;
    else if (pic_noise_variance >= 20 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_4;
    else if (pic_noise_variance >= 17 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_3_1;
    else if (pic_noise_variance >= 10 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_3;
    else if (pic_noise_variance >= 5 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_2;
    else
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_1;

    if (picture_control_set_ptr->pic_noise_class >= PIC_NOISE_CLASS_4)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_3_1;

    return return_error;

}

static EbErrorType full_sample_denoise(
    PictureAnalysisContext  *context_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    uint32_t                 sb_total_count,
    EB_BOOL                  denoise_flag)
{

    EbErrorType return_error = EB_ErrorNone;

    uint32_t                  sb_index;
    EbPictureBufferDesc      *input_picture_ptr = picture_control_set_ptr->enhanced_picture_ptr;
    EbPictureBufferDesc      *denoised_picture_ptr = context_ptr->denoised_picture_ptr;
    EbPictureBufferDesc      *noise_picture_ptr = context_ptr->noise_picture_ptr;

    //Reset the flat noise flag array to False for both RealTime/HighComplexity Modes
    for (sb_index = 0; sb_index < sb_total_count; ++sb_index) {
        picture_control_set_ptr->sb_flat_noise_array[sb_index] = 0;
    }

    picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_INV; //this init is for both REAL-TIME and BEST-QUALITY

    detect_input_picture_noise(
        context_ptr,
        sequence_control_set_ptr,
        picture_control_set_ptr,
        input_picture_ptr,
        noise_picture_ptr,
        denoised_picture_ptr);

    if (denoise_flag == EB_TRUE)
    {
        denoise_input_picture(
            context_ptr,
            sequence_control_set_ptr,
            picture_control_set_ptr,
            input_picture_ptr,
            denoised_picture_ptr);
    }

    return return_error;

}

static EbErrorType sub_sample_filter_noise(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *input_picture_ptr,
    EbPictureBufferDesc     *noise_picture_ptr,
    EbPictureBufferDesc     *denoised_picture_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    uint32_t       sb_index;
    uint32_t       sb_origin_x;
    uint32_t       sb_origin_y;
    uint16_t       vertical_idx;
    if (picture_control_set_ptr->pic_noise_class == PIC_NOISE_CLASS_3_1) {

        uint32_t in_luma_off_set = input_picture_ptr->origin_x + input_picture_ptr->origin_y      * input_picture_ptr->stride_y;
        uint32_t in_chroma_off_set = input_picture_ptr->origin_x / 2 + input_picture_ptr->origin_y / 2 * input_picture_ptr->stride_cb;
        uint32_t den_luma_off_set = denoised_picture_ptr->origin_x + denoised_picture_ptr->origin_y   * denoised_picture_ptr->stride_y;
        uint32_t den_chroma_off_set = denoised_picture_ptr->origin_x / 2 + denoised_picture_ptr->origin_y / 2 * denoised_picture_ptr->stride_cb;

        //filter Luma
        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

            sb_origin_x = sb_params->origin_x;
            sb_origin_y = sb_params->origin_y;

            if (sb_origin_x == 0)
                weak_luma_filter_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
                input_picture_ptr,
                denoised_picture_ptr,
                noise_picture_ptr,
                sb_origin_y,
                sb_origin_x);

            if (sb_origin_x + MAX_SB_SIZE > input_picture_ptr->width)
            {
                eb_vp9_noise_extract_luma_weak(
                    input_picture_ptr,
                    denoised_picture_ptr,
                    noise_picture_ptr,
                    sb_origin_y,
                    sb_origin_x);
            }
        }

        //copy luma
        for (vertical_idx = 0; vertical_idx < input_picture_ptr->height; ++vertical_idx) {
            EB_MEMCPY(input_picture_ptr->buffer_y + in_luma_off_set + vertical_idx * input_picture_ptr->stride_y,
                denoised_picture_ptr->buffer_y + den_luma_off_set + vertical_idx * denoised_picture_ptr->stride_y,
                sizeof(uint8_t) * input_picture_ptr->width);
        }

        //filter chroma
        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

            sb_origin_x = sb_params->origin_x;
            sb_origin_y = sb_params->origin_y;

            if (sb_origin_x == 0)
                weak_chroma_filter_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
                input_picture_ptr,
                denoised_picture_ptr,
                sb_origin_y / 2,
                sb_origin_x / 2);

            if (sb_origin_x + MAX_SB_SIZE > input_picture_ptr->width)
            {
                eb_vp9_noise_extract_chroma_weak(
                    input_picture_ptr,
                    denoised_picture_ptr,
                    sb_origin_y / 2,
                    sb_origin_x / 2);
            }

        }

        //copy chroma
        for (vertical_idx = 0; vertical_idx < input_picture_ptr->height / 2; ++vertical_idx) {

            EB_MEMCPY(input_picture_ptr->buffer_cb + in_chroma_off_set + vertical_idx * input_picture_ptr->stride_cb,
                denoised_picture_ptr->buffer_cb + den_chroma_off_set + vertical_idx * denoised_picture_ptr->stride_cb,
                sizeof(uint8_t) * input_picture_ptr->width / 2);

            EB_MEMCPY(input_picture_ptr->buffer_cr + in_chroma_off_set + vertical_idx * input_picture_ptr->stride_cr,
                denoised_picture_ptr->buffer_cr + den_chroma_off_set + vertical_idx * denoised_picture_ptr->stride_cr,
                sizeof(uint8_t) * input_picture_ptr->width / 2);
        }

    }
    else if (picture_control_set_ptr->pic_noise_class == PIC_NOISE_CLASS_2){

        uint32_t new_tot_fn = 0;

        //for each LCU ,re check the FN information for only the FNdecim ones
        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

            sb_origin_x = sb_params->origin_x;
            sb_origin_y = sb_params->origin_y;
            uint32_t  input_luma_origin_index = noise_picture_ptr->origin_x + sb_origin_x + (noise_picture_ptr->origin_y + sb_origin_y) * noise_picture_ptr->stride_y;
            uint32_t  noise_origin_index = noise_picture_ptr->origin_x + sb_origin_x + (noise_picture_ptr->origin_y * noise_picture_ptr->stride_y);

            if (sb_params->is_complete_sb && picture_control_set_ptr->sb_flat_noise_array[sb_index] == 1)
            {

                weak_luma_filter_sb_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
                    input_picture_ptr,
                    denoised_picture_ptr,
                    noise_picture_ptr,
                    sb_origin_y,
                    sb_origin_x);

                if (sb_origin_x + MAX_SB_SIZE > input_picture_ptr->width)
                {
                    eb_vp9_noise_extract_luma_weak_sb(
                        input_picture_ptr,
                        denoised_picture_ptr,
                        noise_picture_ptr,
                        sb_origin_y,
                        sb_origin_x);
                }

                uint64_t noiseBlkVar32x32[4], denoiseBlkVar32x32[4];
                uint64_t noiseBlkVar = compute_variance64x64(
                    noise_picture_ptr, noise_origin_index, noiseBlkVar32x32);
                uint64_t den_blk_var = compute_variance64x64(
                    denoised_picture_ptr, input_luma_origin_index, denoiseBlkVar32x32) >> 16;

                uint64_t noiseBlkVarTh ;
                uint64_t denBlkVarTh = FLAT_MAX_VAR;

                if (picture_control_set_ptr->noise_detection_th == 1)
                    noiseBlkVarTh = NOISE_MIN_LEVEL_0;
                else
                    noiseBlkVarTh = NOISE_MIN_LEVEL_1;

                if (den_blk_var<denBlkVarTh && noiseBlkVar> noiseBlkVarTh) {
                    picture_control_set_ptr->sb_flat_noise_array[sb_index] = 1;
                    //SVT_LOG("POC %i (%i,%i) den_blk_var: %i  noiseBlkVar :%i\n", picture_control_set_ptr->picture_number,sb_origin_x,sb_origin_y, den_blk_var, noiseBlkVar);
                    new_tot_fn++;

                }
                else{
                    picture_control_set_ptr->sb_flat_noise_array[sb_index] = 0;
                }
            }
        }

        //filter Luma
        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

            sb_origin_x = sb_params->origin_x;
            sb_origin_y = sb_params->origin_y;

            if (sb_origin_x + 64 <= input_picture_ptr->width && sb_origin_y + 64 <= input_picture_ptr->height)
            {
                // use the denoised for FN LCUs
                if (picture_control_set_ptr->sb_flat_noise_array[sb_index] == 1){

                    uint32_t  sb_height = MIN(MAX_SB_SIZE, input_picture_ptr->height - sb_origin_y);
                    uint32_t  sb_width = MIN(MAX_SB_SIZE, input_picture_ptr->width - sb_origin_x);

                    uint32_t in_luma_off_set = input_picture_ptr->origin_x + sb_origin_x + (input_picture_ptr->origin_y + sb_origin_y) * input_picture_ptr->stride_y;
                    uint32_t den_luma_off_set = denoised_picture_ptr->origin_x + sb_origin_x + (denoised_picture_ptr->origin_y + sb_origin_y) * denoised_picture_ptr->stride_y;

                    for (vertical_idx = 0; vertical_idx < sb_height; ++vertical_idx) {

                        EB_MEMCPY(input_picture_ptr->buffer_y + in_luma_off_set + vertical_idx * input_picture_ptr->stride_y,
                            denoised_picture_ptr->buffer_y + den_luma_off_set + vertical_idx * denoised_picture_ptr->stride_y,
                            sizeof(uint8_t) * sb_width);

                    }
                }

            }

        }

    }
    return return_error;
}

static EbErrorType quarter_sample_detect_noise(
    PictureAnalysisContext  *context_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *quarter_decimated_picture_ptr,
    EbPictureBufferDesc     *noise_picture_ptr,
    EbPictureBufferDesc     *denoised_picture_ptr,
    uint32_t                 picture_width_in_sb)
{

    EbErrorType return_error = EB_ErrorNone;

    uint64_t     pic_noise_variance;

    uint32_t     tot_sb_count, noise_th;

    uint32_t     block_index;

    pic_noise_variance = 0;
    tot_sb_count = 0;

    uint16_t vert64x64_index;
    uint16_t horz64x64_index;
    uint32_t block64x64_x;
    uint32_t block64x64_y;
    uint32_t vert32x32_index;
    uint32_t horz32x32_index;
    uint32_t block32x32_x;
    uint32_t block32x32_y;
    uint32_t noise_origin_index;
    uint32_t sb_index;

    // Loop over 64x64 blocks on the downsampled domain (each block would contain 16 LCUs on the full sampled domain)
    for (vert64x64_index = 0; vert64x64_index < (quarter_decimated_picture_ptr->height / 64); vert64x64_index++){
        for (horz64x64_index = 0; horz64x64_index < (quarter_decimated_picture_ptr->width / 64); horz64x64_index++){

            block64x64_x = horz64x64_index * 64;
            block64x64_y = vert64x64_index * 64;

            if (block64x64_x == 0)
                weak_luma_filter_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
                quarter_decimated_picture_ptr,
                denoised_picture_ptr,
                noise_picture_ptr,
                block64x64_y,
                block64x64_x);

            if (block64x64_y + MAX_SB_SIZE > quarter_decimated_picture_ptr->width)
            {
                eb_vp9_noise_extract_luma_weak(
                    quarter_decimated_picture_ptr,
                    denoised_picture_ptr,
                    noise_picture_ptr,
                    block64x64_y,
                    block64x64_x);
            }

            // Loop over 32x32 blocks (i.e, 64x64 blocks in full resolution)
            for (vert32x32_index = 0; vert32x32_index < 2; vert32x32_index++){
                for (horz32x32_index = 0; horz32x32_index < 2; horz32x32_index++){

                    block32x32_x = block64x64_x + horz32x32_index * 32;
                    block32x32_y = block64x64_y + vert32x32_index * 32;

                    //do it only for complete 32x32 blocks (i.e, complete 64x64 blocks in full resolution)
                    if ((block32x32_x + 32 <= quarter_decimated_picture_ptr->width) && (block32x32_y + 32 <= quarter_decimated_picture_ptr->height))
                    {

                        sb_index = ((vert64x64_index * 2) + vert32x32_index) * picture_width_in_sb + ((horz64x64_index * 2) + horz32x32_index);

                        uint64_t noiseBlkVar8x8[16], denoiseBlkVar8x8[16];

                        noise_origin_index = noise_picture_ptr->origin_x + block32x32_x + noise_picture_ptr->origin_y * noise_picture_ptr->stride_y;

                        uint64_t noiseBlkVar = compute_variance32x32(
                            noise_picture_ptr,
                            noise_origin_index,
                            noiseBlkVar8x8);

                        pic_noise_variance += (noiseBlkVar >> 16);

                        block_index = (noise_picture_ptr->origin_y + block32x32_y) * noise_picture_ptr->stride_y + noise_picture_ptr->origin_x + block32x32_x;

                        uint64_t den_blk_var = compute_variance32x32(
                            denoised_picture_ptr,
                            block_index,
                            denoiseBlkVar8x8) >> 16;

                        uint64_t den_blk_var_dec_th;

                        if (picture_control_set_ptr->noise_detection_th == 0){
                            den_blk_var_dec_th = NOISE_MIN_LEVEL_DECIM_1;
                        }
                        else{
                            den_blk_var_dec_th = NOISE_MIN_LEVEL_DECIM_0;
                        }

                        if (den_blk_var < FLAT_MAX_VAR_DECIM && noiseBlkVar> den_blk_var_dec_th) {
                            picture_control_set_ptr->sb_flat_noise_array[sb_index] = 1;
                        }

                        tot_sb_count++;
                    }
                }
            }
        }
    }

    if (tot_sb_count > 0) {
        context_ptr->pic_noise_variance_float = (double)pic_noise_variance / (double)tot_sb_count;
        pic_noise_variance = pic_noise_variance / tot_sb_count;
    }

    //the variance of a 64x64 noise area tends to be bigger for small resolutions.
    //if (sequence_control_set_ptr->luma_height <= 720)
    //    noise_th = 25;
    //else if (sequence_control_set_ptr->luma_height <= 1080)
    //    noise_th = 10;
    //else
    noise_th = 0;

    //look for extreme noise or big enough flat noisy area to be denoised.
    if (pic_noise_variance > 60)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_3_1; //Noise+Edge information is too big, so may be this is all noise (action: frame based denoising)
    else if (pic_noise_variance >= 10 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_3;   //Noise+Edge information is big enough, so there is no big enough flat noisy area (action : no denoising)
    else if (pic_noise_variance >= 5 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_2;   //Noise+Edge information is relatively small, so there might be a big enough flat noisy area(action : denoising only for FN blocks)
    else
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_1;   //Noise+Edge information is very small, so no noise nor edge area (action : no denoising)

    return return_error;

}

static EbErrorType sub_sample_detect_noise(
    PictureAnalysisContext  *context_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *sixteenth_decimated_picture_ptr,
    EbPictureBufferDesc     *noise_picture_ptr,
    EbPictureBufferDesc     *denoised_picture_ptr,
    uint32_t                 picture_width_in_sb)
{

    EbErrorType return_error = EB_ErrorNone;

    uint64_t    pic_noise_variance;

    uint32_t    tot_sb_count, noise_th;

    uint32_t    block_index;

    pic_noise_variance = 0;
    tot_sb_count = 0;

    uint16_t vert64x64_index;
    uint16_t horz64x64_index;
    uint32_t block64x64_x;
    uint32_t block64x64_y;
    uint32_t vert16x16Index;
    uint32_t horz16x16Index;
    uint32_t block16x16X;
    uint32_t block16x16Y;
    uint32_t noise_origin_index;
    uint32_t sb_index;

    // Loop over 64x64 blocks on the downsampled domain (each block would contain 16 LCUs on the full sampled domain)
    for (vert64x64_index = 0; vert64x64_index < (sixteenth_decimated_picture_ptr->height / 64); vert64x64_index++){
        for (horz64x64_index = 0; horz64x64_index < (sixteenth_decimated_picture_ptr->width / 64); horz64x64_index++){

            block64x64_x = horz64x64_index * 64;
            block64x64_y = vert64x64_index * 64;

            if (block64x64_x == 0)
                weak_luma_filter_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1](
                sixteenth_decimated_picture_ptr,
                denoised_picture_ptr,
                noise_picture_ptr,
                block64x64_y,
                block64x64_x);

            if (block64x64_y + MAX_SB_SIZE > sixteenth_decimated_picture_ptr->width)
            {
                eb_vp9_noise_extract_luma_weak(
                    sixteenth_decimated_picture_ptr,
                    denoised_picture_ptr,
                    noise_picture_ptr,
                    block64x64_y,
                    block64x64_x);
            }

            // Loop over 16x16 blocks (i.e, 64x64 blocks in full resolution)
            for (vert16x16Index = 0; vert16x16Index < 4; vert16x16Index++){
                for (horz16x16Index = 0; horz16x16Index < 4; horz16x16Index++){

                    block16x16X = block64x64_x + horz16x16Index * 16;
                    block16x16Y = block64x64_y + vert16x16Index * 16;

                    //do it only for complete 16x16 blocks (i.e, complete 64x64 blocks in full resolution)
                    if (block16x16X + 16 <= sixteenth_decimated_picture_ptr->width && block16x16Y + 16 <= sixteenth_decimated_picture_ptr->height)
                    {

                        sb_index = ((vert64x64_index * 4) + vert16x16Index) * picture_width_in_sb + ((horz64x64_index * 4) + horz16x16Index);

                        uint64_t noiseBlkVar8x8[4], denoiseBlkVar8x8[4];

                        noise_origin_index = noise_picture_ptr->origin_x + block16x16X + noise_picture_ptr->origin_y * noise_picture_ptr->stride_y;

                        uint64_t noiseBlkVar = compute_variance16x16(
                            noise_picture_ptr,
                            noise_origin_index,
                            noiseBlkVar8x8);

                        pic_noise_variance += (noiseBlkVar >> 16);

                        block_index = (noise_picture_ptr->origin_y + block16x16Y) * noise_picture_ptr->stride_y + noise_picture_ptr->origin_x + block16x16X;

                        uint64_t den_blk_var = compute_variance16x16(
                            denoised_picture_ptr,
                            block_index,
                            denoiseBlkVar8x8) >> 16;

                        uint64_t  noise_blk_var_dec_th;
                        uint64_t den_blk_var_dec_th = FLAT_MAX_VAR_DECIM;

                        if (picture_control_set_ptr->noise_detection_th == 1) {
                            noise_blk_var_dec_th = NOISE_MIN_LEVEL_DECIM_0;
                        }
                        else {
                            noise_blk_var_dec_th = NOISE_MIN_LEVEL_DECIM_1;
                        }

                        if (den_blk_var < den_blk_var_dec_th && noiseBlkVar> noise_blk_var_dec_th) {
                            picture_control_set_ptr->sb_flat_noise_array[sb_index] = 1;
                        }
                        tot_sb_count++;
                    }
                }
            }
        }
    }

    if (tot_sb_count > 0) {
        context_ptr->pic_noise_variance_float = (double)pic_noise_variance / (double)tot_sb_count;
        pic_noise_variance = pic_noise_variance / tot_sb_count;
    }

    //the variance of a 64x64 noise area tends to be bigger for small resolutions.
    if (sequence_control_set_ptr->luma_height <= 720)
        noise_th = 25;
    else if (sequence_control_set_ptr->luma_height <= 1080)
        noise_th = 10;
    else
        noise_th = 0;

    //look for extreme noise or big enough flat noisy area to be denoised.
    if (pic_noise_variance >= 55 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_3_1; //Noise+Edge information is too big, so may be this is all noise (action: frame based denoising)
    else if (pic_noise_variance >= 10 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_3;   //Noise+Edge information is big enough, so there is no big enough flat noisy area (action : no denoising)
    else if (pic_noise_variance >= 5 + noise_th)
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_2;   //Noise+Edge information is relatively small, so there might be a big enough flat noisy area(action : denoising only for FN blocks)
    else
        picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_1;   //Noise+Edge information is very small, so no noise nor edge area (action : no denoising)

    return return_error;

}

static EbErrorType quarter_sample_denoise(
    PictureAnalysisContext  *context_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *quarter_decimated_picture_ptr,
    uint32_t                 sb_total_count,
    EB_BOOL                  denoise_flag,
    uint32_t                 picture_width_in_sb)
{

    EbErrorType return_error = EB_ErrorNone;

    uint32_t                  sb_index;
    EbPictureBufferDesc      *input_picture_ptr = picture_control_set_ptr->enhanced_picture_ptr;
    EbPictureBufferDesc      *denoised_picture_ptr = context_ptr->denoised_picture_ptr;
    EbPictureBufferDesc      *noise_picture_ptr = context_ptr->noise_picture_ptr;

    //Reset the flat noise flag array to False for both RealTime/HighComplexity Modes
    for (sb_index = 0; sb_index < sb_total_count; ++sb_index) {
        picture_control_set_ptr->sb_flat_noise_array[sb_index] = 0;
    }

    picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_INV; //this init is for both REAL-TIME and BEST-QUALITY

    eb_vp9_decimation_2d(
        &input_picture_ptr->buffer_y[input_picture_ptr->origin_x + input_picture_ptr->origin_y * input_picture_ptr->stride_y],
        input_picture_ptr->stride_y,
        input_picture_ptr->width,
        input_picture_ptr->height,
        &quarter_decimated_picture_ptr->buffer_y[quarter_decimated_picture_ptr->origin_x + (quarter_decimated_picture_ptr->origin_y * quarter_decimated_picture_ptr->stride_y)],
        quarter_decimated_picture_ptr->stride_y,
        2);

    quarter_sample_detect_noise(
        context_ptr,
        picture_control_set_ptr,
        quarter_decimated_picture_ptr,
        noise_picture_ptr,
        denoised_picture_ptr,
        picture_width_in_sb);

    if (denoise_flag == EB_TRUE) {

        // Turn OFF the de-noiser for Class 2 at QP=29 and lower (for Fixed_QP) and at the target rate of 14Mbps and higher (for RC=ON)
        if ((picture_control_set_ptr->pic_noise_class == PIC_NOISE_CLASS_3_1) || ((picture_control_set_ptr->pic_noise_class == PIC_NOISE_CLASS_2) && ((sequence_control_set_ptr->static_config.rate_control_mode == 0 && sequence_control_set_ptr->qp > DENOISER_QP_TH) || (sequence_control_set_ptr->static_config.rate_control_mode != 0 && sequence_control_set_ptr->static_config.target_bit_rate < DENOISER_BITRATE_TH)))) {

            sub_sample_filter_noise(
                sequence_control_set_ptr,
                picture_control_set_ptr,
                input_picture_ptr,
                noise_picture_ptr,
                denoised_picture_ptr);
        }
    }

    return return_error;

}

EbErrorType half_sample_denoise(
    PictureAnalysisContext  *context_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *sixteenth_decimated_picture_ptr,
    uint32_t                 sb_total_count,
    EB_BOOL                  denoise_flag,
    uint32_t                 picture_width_in_sb)
{

    EbErrorType return_error = EB_ErrorNone;

    uint32_t                  sb_index;
    EbPictureBufferDesc      *input_picture_ptr = picture_control_set_ptr->enhanced_picture_ptr;
    EbPictureBufferDesc      *denoised_picture_ptr = context_ptr->denoised_picture_ptr;
    EbPictureBufferDesc      *noise_picture_ptr = context_ptr->noise_picture_ptr;

    //Reset the flat noise flag array to False for both RealTime/HighComplexity Modes
    for (sb_index = 0; sb_index < sb_total_count; ++sb_index) {
        picture_control_set_ptr->sb_flat_noise_array[sb_index] = 0;
    }

    picture_control_set_ptr->pic_noise_class = PIC_NOISE_CLASS_INV; //this init is for both REAL-TIME and BEST-QUALITY

    eb_vp9_decimation_2d(
        &input_picture_ptr->buffer_y[input_picture_ptr->origin_x + input_picture_ptr->origin_y * input_picture_ptr->stride_y],
        input_picture_ptr->stride_y,
        input_picture_ptr->width,
        input_picture_ptr->height,
        &sixteenth_decimated_picture_ptr->buffer_y[sixteenth_decimated_picture_ptr->origin_x + (sixteenth_decimated_picture_ptr->origin_y * sixteenth_decimated_picture_ptr->stride_y)],
        sixteenth_decimated_picture_ptr->stride_y,
        4);

    sub_sample_detect_noise(
        context_ptr,
        sequence_control_set_ptr,
        picture_control_set_ptr,
        sixteenth_decimated_picture_ptr,
        noise_picture_ptr,
        denoised_picture_ptr,
        picture_width_in_sb);

    if (denoise_flag == EB_TRUE) {

        // Turn OFF the de-noiser for Class 2 at QP=29 and lower (for Fixed_QP) and at the target rate of 14Mbps and higher (for RC=ON)
        if ((picture_control_set_ptr->pic_noise_class == PIC_NOISE_CLASS_3_1) || ((picture_control_set_ptr->pic_noise_class == PIC_NOISE_CLASS_2) && ((sequence_control_set_ptr->static_config.rate_control_mode == 0 && sequence_control_set_ptr->qp > DENOISER_QP_TH) || (sequence_control_set_ptr->static_config.rate_control_mode != 0 && sequence_control_set_ptr->static_config.target_bit_rate < DENOISER_BITRATE_TH)))) {

            sub_sample_filter_noise(
                sequence_control_set_ptr,
                picture_control_set_ptr,
                input_picture_ptr,
                noise_picture_ptr,
                denoised_picture_ptr);
        }
    }

    return return_error;

}

/************************************************
 * Set Picture Parameters based on input configuration
 ** Setting Number of regions per resolution
 ** Setting width and height for subpicture and when picture scan type is 1
 ************************************************/
static void set_picture_parameters_for_statistics_gathering(
    SequenceControlSet *sequence_control_set_ptr
    )
{
    sequence_control_set_ptr->picture_analysis_number_of_regions_per_width = HIGHER_THAN_CLASS_1_REGION_SPLIT_PER_WIDTH;
    sequence_control_set_ptr->picture_analysis_number_of_regions_per_height = HIGHER_THAN_CLASS_1_REGION_SPLIT_PER_HEIGHT;

    return;
}
/************************************************
 * Picture Pre Processing Operations *
 *** A function that groups all of the Pre proceesing
 * operations performed on the input picture
 *** Operations included at this point:
 ***** Borders preprocessing
 ***** Denoising
 ************************************************/
static void picture_pre_processing_operations(
    PictureParentControlSet  *picture_control_set_ptr,
#if !TURN_OFF_PRE_PROCESSING
    EbPictureBufferDesc      *input_picture_ptr,
#endif
    PictureAnalysisContext   *context_ptr,
    SequenceControlSet       *sequence_control_set_ptr,
    EbPictureBufferDesc      *quarter_decimated_picture_ptr,
    EbPictureBufferDesc      *sixteenth_decimated_picture_ptr,
    uint32_t                  sb_total_count,
    uint32_t                  picture_width_in_sb){

#if !TURN_OFF_PRE_PROCESSING
    if (sequence_control_set_ptr->static_config.tune == TUNE_SQ) {
        check_input_for_borders_and_preprocess(
            input_picture_ptr);
    }
#endif

    if (picture_control_set_ptr->noise_detection_method == NOISE_DETECT_HALF_PRECISION) {

        half_sample_denoise(
            context_ptr,
            sequence_control_set_ptr,
            picture_control_set_ptr,
            sixteenth_decimated_picture_ptr,
            sb_total_count,
            picture_control_set_ptr->enable_denoise_src_flag,
            picture_width_in_sb);
    }
    else if (picture_control_set_ptr->noise_detection_method == NOISE_DETECT_QUARTER_PRECISION) {
        quarter_sample_denoise(
            context_ptr,
            sequence_control_set_ptr,
            picture_control_set_ptr,
            quarter_decimated_picture_ptr,
            sb_total_count,
            picture_control_set_ptr->enable_denoise_src_flag,
            picture_width_in_sb);
    } else {
        full_sample_denoise(
            context_ptr,
            sequence_control_set_ptr,
            picture_control_set_ptr,
            sb_total_count,
            picture_control_set_ptr->enable_denoise_src_flag
        );
    }
    return;

}

/**************************************************************
* Generate picture histogram bins for YUV pixel intensity *
* Calculation is done on a region based (Set previously, resolution dependent)
**************************************************************/
static void sub_sample_luma_generate_pixel_intensity_histogram_bins(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *input_picture_ptr,
    uint64_t                *sum_average_intensity_total_regions_luma){

    uint32_t                          region_width;
    uint32_t                          region_height;
    uint32_t                          region_width_offset;
    uint32_t                          region_height_offset;
    uint32_t                          region_in_picture_width_index;
    uint32_t                          region_in_picture_height_index;
    uint32_t                          histogram_bin;
    uint64_t                          sum;

    region_width = input_picture_ptr->width / sequence_control_set_ptr->picture_analysis_number_of_regions_per_width;
    region_height = input_picture_ptr->height / sequence_control_set_ptr->picture_analysis_number_of_regions_per_height;

    // Loop over regions inside the picture
    for (region_in_picture_width_index = 0; region_in_picture_width_index < sequence_control_set_ptr->picture_analysis_number_of_regions_per_width; region_in_picture_width_index++){  // loop over horizontal regions
        for (region_in_picture_height_index = 0; region_in_picture_height_index < sequence_control_set_ptr->picture_analysis_number_of_regions_per_height; region_in_picture_height_index++){ // loop over vertical regions

            // Initialize bins to 1
            eb_vp9_initialize_buffer_32bits_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][0], 64, 0, 1);

            region_width_offset = (region_in_picture_width_index == sequence_control_set_ptr->picture_analysis_number_of_regions_per_width - 1) ?
                input_picture_ptr->width - (sequence_control_set_ptr->picture_analysis_number_of_regions_per_width * region_width) :
                0;

            region_height_offset = (region_in_picture_height_index == sequence_control_set_ptr->picture_analysis_number_of_regions_per_height - 1) ?
                input_picture_ptr->height - (sequence_control_set_ptr->picture_analysis_number_of_regions_per_height * region_height) :
                0;

            // Y Histogram
            calculate_histogram(
                &input_picture_ptr->buffer_y[(input_picture_ptr->origin_x + region_in_picture_width_index * region_width) + ((input_picture_ptr->origin_y + region_in_picture_height_index * region_height) * input_picture_ptr->stride_y)],
                region_width + region_width_offset,
                region_height + region_height_offset,
                input_picture_ptr->stride_y,
                1,
                picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][0],
                &sum);

            picture_control_set_ptr->average_intensity_per_region[region_in_picture_width_index][region_in_picture_height_index][0] = (uint8_t)((sum + (((region_width + region_width_offset)*(region_height + region_height_offset)) >> 1)) / ((region_width + region_width_offset)*(region_height + region_height_offset)));
            (*sum_average_intensity_total_regions_luma) += (sum << 4);
            for (histogram_bin = 0; histogram_bin < HISTOGRAM_NUMBER_OF_BINS; histogram_bin++){ // Loop over the histogram bins
                picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][0][histogram_bin] =
                    picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][0][histogram_bin] << 4;
            }
        }
    }

    return;
}

static void sub_sample_chroma_generate_pixel_intensity_histogram_bins(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *input_picture_ptr,
    uint64_t                *sum_average_intensity_total_regions_cb,
    uint64_t                *sum_average_intensity_total_regions_cr)
{
    uint64_t                          sum;
    uint32_t                          region_width;
    uint32_t                          region_height;
    uint32_t                          region_width_offset;
    uint32_t                          region_height_offset;
    uint32_t                          region_in_picture_width_index;
    uint32_t                          region_in_picture_height_index;

    uint16_t                          histogram_bin;
    uint8_t                           decim_step = 4;

    region_width  = input_picture_ptr->width / sequence_control_set_ptr->picture_analysis_number_of_regions_per_width;
    region_height = input_picture_ptr->height / sequence_control_set_ptr->picture_analysis_number_of_regions_per_height;

    // Loop over regions inside the picture
    for (region_in_picture_width_index = 0; region_in_picture_width_index < sequence_control_set_ptr->picture_analysis_number_of_regions_per_width; region_in_picture_width_index++){  // loop over horizontal regions
        for (region_in_picture_height_index = 0; region_in_picture_height_index < sequence_control_set_ptr->picture_analysis_number_of_regions_per_height; region_in_picture_height_index++){ // loop over vertical regions

            // Initialize bins to 1
            eb_vp9_initialize_buffer_32bits_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][1], 64, 0, 1);
            eb_vp9_initialize_buffer_32bits_func_ptr_array[(eb_vp9_ASM_TYPES & PREAVX2_MASK) && 1](picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][2], 64, 0, 1);

            region_width_offset = (region_in_picture_width_index == sequence_control_set_ptr->picture_analysis_number_of_regions_per_width - 1) ?
                input_picture_ptr->width - (sequence_control_set_ptr->picture_analysis_number_of_regions_per_width * region_width) :
                0;

            region_height_offset = (region_in_picture_height_index == sequence_control_set_ptr->picture_analysis_number_of_regions_per_height - 1) ?
                input_picture_ptr->height - (sequence_control_set_ptr->picture_analysis_number_of_regions_per_height * region_height) :
                0;

            // U Histogram
            calculate_histogram(
                &input_picture_ptr->buffer_cb[((input_picture_ptr->origin_x + region_in_picture_width_index * region_width) >> 1) + (((input_picture_ptr->origin_y + region_in_picture_height_index * region_height) >> 1) * input_picture_ptr->stride_cb)],
                (region_width + region_width_offset) >> 1,
                (region_height + region_height_offset) >> 1,
                input_picture_ptr->stride_cb,
                decim_step,
                picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][1],
                &sum);

            sum = (sum << decim_step);
            *sum_average_intensity_total_regions_cb += sum;
            picture_control_set_ptr->average_intensity_per_region[region_in_picture_width_index][region_in_picture_height_index][1] = (uint8_t)((sum + (((region_width + region_width_offset)*(region_height + region_height_offset)) >> 3)) / (((region_width + region_width_offset)*(region_height + region_height_offset)) >> 2));

            for (histogram_bin = 0; histogram_bin < HISTOGRAM_NUMBER_OF_BINS; histogram_bin++){ // Loop over the histogram bins
                picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][1][histogram_bin] =
                    picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][1][histogram_bin] << decim_step;
            }

            // V Histogram
            calculate_histogram(
                &input_picture_ptr->buffer_cr[((input_picture_ptr->origin_x + region_in_picture_width_index * region_width) >> 1) + (((input_picture_ptr->origin_y + region_in_picture_height_index * region_height) >> 1) * input_picture_ptr->stride_cr)],
                (region_width + region_width_offset) >> 1,
                (region_height + region_height_offset) >> 1,
                input_picture_ptr->stride_cr,
                decim_step,
                picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][2],
                &sum);

            sum = (sum << decim_step);
            *sum_average_intensity_total_regions_cr += sum;
            picture_control_set_ptr->average_intensity_per_region[region_in_picture_width_index][region_in_picture_height_index][2] = (uint8_t)((sum + (((region_width + region_width_offset)*(region_height + region_height_offset)) >> 3)) / (((region_width + region_width_offset)*(region_height + region_height_offset)) >> 2));

            for (histogram_bin = 0; histogram_bin < HISTOGRAM_NUMBER_OF_BINS; histogram_bin++){ // Loop over the histogram bins
                picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][2][histogram_bin] =
                    picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][2][histogram_bin] << decim_step;
            }
        }
    }
    return;

}

void edge_detection_mean_luma_chroma16x16(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    PictureAnalysisContext  *context_ptr,
    uint32_t                 total_sb_count)
{

    uint32_t sb_index;
    uint32_t max_grad = 1;

    // The values are calculated for every 4th frame
    if ((picture_control_set_ptr->picture_number & 3) == 0){
        for (sb_index = 0; sb_index < total_sb_count; sb_index++) {

            SbStat *sb_stat_ptr = &picture_control_set_ptr->sb_stat_array[sb_index];

            EB_MEMSET(sb_stat_ptr, 0, sizeof(SbStat));
            SbParams     *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
            if (sb_params->potential_logo_sb &&sb_params->is_complete_sb)

            {
                uint8_t *y_mean_ptr = picture_control_set_ptr->y_mean[sb_index];
                uint8_t *cr_mean_ptr = picture_control_set_ptr->cr_mean[sb_index];
                uint8_t *cb_mean_ptr = picture_control_set_ptr->cb_mean[sb_index];

                uint8_t raster_scan_block_index;

                for (raster_scan_block_index = PA_RASTER_SCAN_CU_INDEX_16x16_0; raster_scan_block_index <= PA_RASTER_SCAN_CU_INDEX_16x16_15; raster_scan_block_index++) {
                    uint8_t block_index = raster_scan_block_index - 5;
                    uint8_t x = block_index & 3;
                    uint8_t y = (block_index >> 2);
                    int32_t gradx = 0;
                    int32_t grady = 0;
                    int32_t nbcompx = 0;
                    int32_t nbcompy = 0;
                    if (x != 0)
                    {
                        gradx += ABS((int32_t)(y_mean_ptr[raster_scan_block_index]) - (int32_t)(y_mean_ptr[raster_scan_block_index - 1]));
                        gradx += ABS((int32_t)(cr_mean_ptr[raster_scan_block_index]) - (int32_t)(cr_mean_ptr[raster_scan_block_index - 1]));
                        gradx += ABS((int32_t)(cb_mean_ptr[raster_scan_block_index]) - (int32_t)(cb_mean_ptr[raster_scan_block_index - 1]));
                        nbcompx++;
                    }
                    if (x != 3)
                    {
                        gradx += ABS((int32_t)(y_mean_ptr[raster_scan_block_index + 1]) - (int32_t)(y_mean_ptr[raster_scan_block_index]));
                        gradx += ABS((int32_t)(cr_mean_ptr[raster_scan_block_index + 1]) - (int32_t)(cr_mean_ptr[raster_scan_block_index]));
                        gradx += ABS((int32_t)(cb_mean_ptr[raster_scan_block_index + 1]) - (int32_t)(cb_mean_ptr[raster_scan_block_index]));
                        nbcompx++;
                    }
                    gradx = gradx / nbcompx;

                    if (y != 0)
                    {
                        grady += ABS((int32_t)(y_mean_ptr[raster_scan_block_index]) - (int32_t)(y_mean_ptr[raster_scan_block_index - 4]));
                        grady += ABS((int32_t)(cr_mean_ptr[raster_scan_block_index]) - (int32_t)(cr_mean_ptr[raster_scan_block_index - 4]));
                        grady += ABS((int32_t)(cb_mean_ptr[raster_scan_block_index]) - (int32_t)(cb_mean_ptr[raster_scan_block_index - 4]));
                        nbcompy++;
                    }
                    if (y != 3)
                    {
                        grady += ABS((int32_t)(y_mean_ptr[raster_scan_block_index + 4]) - (int32_t)(y_mean_ptr[raster_scan_block_index]));
                        grady += ABS((int32_t)(cr_mean_ptr[raster_scan_block_index + 4]) - (int32_t)(cr_mean_ptr[raster_scan_block_index]));
                        grady += ABS((int32_t)(cb_mean_ptr[raster_scan_block_index + 4]) - (int32_t)(cb_mean_ptr[raster_scan_block_index]));

                        nbcompy++;
                    }

                    grady = grady / nbcompy;

                    context_ptr->grad[sb_index][raster_scan_block_index] = (uint16_t) (ABS(gradx) + ABS(grady));
                    if (context_ptr->grad[sb_index][raster_scan_block_index] > max_grad){
                        max_grad = context_ptr->grad[sb_index][raster_scan_block_index];
                    }
                }
            }
        }

        for (sb_index = 0; sb_index < total_sb_count; sb_index++) {
            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
            if (sb_params->potential_logo_sb &&sb_params->is_complete_sb){
                SbStat *sb_stat_ptr = &picture_control_set_ptr->sb_stat_array[sb_index];

                uint32_t raster_scan_block_index;
                for (raster_scan_block_index = PA_RASTER_SCAN_CU_INDEX_16x16_0; raster_scan_block_index <= PA_RASTER_SCAN_CU_INDEX_16x16_15; raster_scan_block_index++) {
                    sb_stat_ptr->cu_stat_array[raster_scan_block_index].edge_cu = (uint16_t)MIN(((context_ptr->grad[sb_index][raster_scan_block_index] * (255*3)) / max_grad), 255) < 30 ? 0 : 1;
                }
            }
        }
    }
    else{
        for (sb_index = 0; sb_index < total_sb_count; sb_index++) {

            SbStat *sb_stat_ptr = &picture_control_set_ptr->sb_stat_array[sb_index];

            EB_MEMSET(sb_stat_ptr, 0, sizeof(SbStat));
        }
    }
}

/******************************************************
* Edge map derivation
******************************************************/
void edge_detection(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr)
{

    uint16_t *variance_ptr;
    uint64_t thrsld_level0 = (picture_control_set_ptr->pic_avg_variance * 70) / 100;
    uint8_t  *mean_ptr;
    uint32_t picture_width_in_sb = (sequence_control_set_ptr->luma_width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE;
    uint32_t picture_height_in_sb = (sequence_control_set_ptr->luma_height + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE;
    uint32_t neighboursb_index = 0;
    uint64_t similarity_count = 0;
    uint64_t similarity_count0 = 0;
    uint64_t similarity_count1 = 0;
    uint64_t similarity_count2 = 0;
    uint64_t similarity_count3 = 0;
    uint32_t sb_x = 0;
    uint32_t sb_y = 0;
    uint32_t sb_index;
    EB_BOOL high_variance_sb_flag;

    uint32_t raster_scan_block_index = 0;
    uint32_t number_of_edge_sb = 0;
    EB_BOOL  high_intensity_sb_flag;

    uint64_t neighbour_sb_mean;
    int32_t i, j;

    uint8_t high_intensity_th = 180;
    uint8_t low_intensity_th  = 120;
    uint8_t high_intensity_th1   = 200;
    uint8_t very_low_intensity_th =  20;

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

        sb_x = sb_params->horizontal_index;
        sb_y = sb_params->vertical_index;

        EdgeSbResults *edge_results_ptr = picture_control_set_ptr->edge_results_ptr;
        picture_control_set_ptr->edge_results_ptr[sb_index].edge_block_num = 0;
        picture_control_set_ptr->edge_results_ptr[sb_index].isolated_high_intensity_sb = 0;
        picture_control_set_ptr->sharp_edge_sb_flag[sb_index] = 0;

        if (sb_x >  0 && sb_x < (uint32_t)(picture_width_in_sb - 1) && sb_y >  0 && sb_y < (uint32_t)(picture_height_in_sb - 1)){

            variance_ptr = picture_control_set_ptr->variance[sb_index];
            mean_ptr = picture_control_set_ptr->y_mean[sb_index];

            similarity_count = 0;

            high_variance_sb_flag =
                (variance_ptr[PA_RASTER_SCAN_CU_INDEX_64x64] > thrsld_level0) ? EB_TRUE : EB_FALSE;
            edge_results_ptr[sb_index].edge_block_num = high_variance_sb_flag;
            if (variance_ptr[0] > high_intensity_th1){
                uint8_t sharpEdge = 0;
                for (raster_scan_block_index = PA_RASTER_SCAN_CU_INDEX_16x16_0; raster_scan_block_index <= PA_RASTER_SCAN_CU_INDEX_16x16_15; raster_scan_block_index++) {
                    sharpEdge = (variance_ptr[raster_scan_block_index] < very_low_intensity_th) ? sharpEdge + 1 : sharpEdge;

                }
                if (sharpEdge > 4)
                {
                    picture_control_set_ptr->sharp_edge_sb_flag[sb_index] = 1;
                }
            }

            if (sb_x > 3 && sb_x < (uint32_t)(picture_width_in_sb - 4) && sb_y >  3 && sb_y < (uint32_t)(picture_height_in_sb - 4)){

                high_intensity_sb_flag =
                    (mean_ptr[PA_RASTER_SCAN_CU_INDEX_64x64] > high_intensity_th) ? EB_TRUE : EB_FALSE;

                if (high_intensity_sb_flag){

                    neighboursb_index = sb_index - 1;
                    neighbour_sb_mean = picture_control_set_ptr->y_mean[neighboursb_index][PA_RASTER_SCAN_CU_INDEX_64x64];

                    similarity_count0 = (neighbour_sb_mean < low_intensity_th) ? 1 : 0;

                    neighboursb_index = sb_index + 1;

                    neighbour_sb_mean = picture_control_set_ptr->y_mean[neighboursb_index][PA_RASTER_SCAN_CU_INDEX_64x64];
                    similarity_count1 = (neighbour_sb_mean < low_intensity_th) ? 1 : 0;

                    neighboursb_index = sb_index - picture_width_in_sb;
                    neighbour_sb_mean = picture_control_set_ptr->y_mean[neighboursb_index][PA_RASTER_SCAN_CU_INDEX_64x64];
                    similarity_count2 = (neighbour_sb_mean < low_intensity_th) ? 1 : 0;

                    neighboursb_index = sb_index + picture_width_in_sb;
                    neighbour_sb_mean = picture_control_set_ptr->y_mean[neighboursb_index][PA_RASTER_SCAN_CU_INDEX_64x64];
                    similarity_count3 = (neighbour_sb_mean < low_intensity_th) ? 1 : 0;

                    similarity_count = similarity_count0 + similarity_count1 + similarity_count2 + similarity_count3;

                    if (similarity_count > 0){

                        for (i = -4; i < 5; i++){
                            for (j = -4; j < 5; j++){
                                neighboursb_index = sb_index + (i * picture_width_in_sb) + j;
                                picture_control_set_ptr->edge_results_ptr[neighboursb_index].isolated_high_intensity_sb = 1;
                            }
                        }
                    }
                }
            }

            if (high_variance_sb_flag){
                number_of_edge_sb += edge_results_ptr[sb_index].edge_block_num;
            }
        }
    }

    return;
}
/******************************************************
* Calculate the variance of variance to determine Homogeneous regions. Note: Variance calculation should be on.
******************************************************/
static void determine_homogeneous_region_in_picture(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr)
{

    uint16_t *variance_ptr;
    uint32_t sb_index;

    uint32_t cu_num, cu_size, block_index_offset, cu_h, cu_w;
    uint64_t null_var_cnt = 0;
    uint64_t very_low_var_cnt = 0;
    uint64_t var_sb_cnt = 0;
    uint32_t sb_total_count = picture_control_set_ptr->sb_total_count;

    for (sb_index = 0; sb_index < sb_total_count; ++sb_index) {
        uint64_t mean_sqr_variance32x32_based[4] = { 0 }, mean_variance32x32_based[4] = { 0 };

        uint64_t mean_sqr_variance64x64_based = 0, mean_variance64x64_based = 0;
        uint64_t var_of_var64x64_based = 0;

        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

        // Initialize
        picture_control_set_ptr->sb_homogeneous_area_array[sb_index] = EB_TRUE;

        variance_ptr = picture_control_set_ptr->variance[sb_index];

        if (sb_params->is_complete_sb){

            null_var_cnt += (variance_ptr[ME_TIER_ZERO_PU_64x64] == 0) ? 1 : 0;

            var_sb_cnt++;

            very_low_var_cnt += ((variance_ptr[ME_TIER_ZERO_PU_64x64]) < SB_LOW_VAR_TH) ? 1 : 0;
            cu_size = 8;
            block_index_offset = ME_TIER_ZERO_PU_8x8_0;
            cu_num = 64 / cu_size;

            //Variance of 8x8 blocks in a 32x32
            for (cu_h = 0; cu_h < (cu_num / 2); cu_h++){
                for (cu_w = 0; cu_w < (cu_num / 2); cu_w++){

                    mean_sqr_variance32x32_based[0] += (variance_ptr[block_index_offset + cu_h*cu_num + cu_w])*(variance_ptr[block_index_offset + cu_h*cu_num + cu_w]);
                    mean_variance32x32_based[0] += (variance_ptr[block_index_offset + cu_h*cu_num + cu_w]);

                    mean_sqr_variance32x32_based[1] += (variance_ptr[block_index_offset + cu_h*cu_num + cu_w + 4])*(variance_ptr[block_index_offset + cu_h*cu_num + cu_w + 4]);
                    mean_variance32x32_based[1] += (variance_ptr[block_index_offset + cu_h*cu_num + cu_w + 4]);

                    mean_sqr_variance32x32_based[2] += (variance_ptr[block_index_offset + (cu_h + 4)*cu_num + cu_w])*(variance_ptr[block_index_offset + (cu_h + 4)*cu_num + cu_w]);
                    mean_variance32x32_based[2] += (variance_ptr[block_index_offset + (cu_h + 4)*cu_num + cu_w]);

                    mean_sqr_variance32x32_based[3] += (variance_ptr[block_index_offset + (cu_h + 4)*cu_num + cu_w + 4])*(variance_ptr[block_index_offset + (cu_h + 4)*cu_num + cu_w + 4]);
                    mean_variance32x32_based[3] += (variance_ptr[block_index_offset + (cu_h + 4)*cu_num + cu_w + 4]);

                }
            }

            mean_sqr_variance32x32_based[0] = mean_sqr_variance32x32_based[0] >> 4;
            mean_variance32x32_based[0] = mean_variance32x32_based[0] >> 4;
            picture_control_set_ptr->var_of_var_32x32_based_sb_array[sb_index][0] = mean_sqr_variance32x32_based[0] - mean_variance32x32_based[0] * mean_variance32x32_based[0];

            mean_sqr_variance32x32_based[1] = mean_sqr_variance32x32_based[1] >> 4;
            mean_variance32x32_based[1] = mean_variance32x32_based[1] >> 4;
            picture_control_set_ptr->var_of_var_32x32_based_sb_array[sb_index][1] = mean_sqr_variance32x32_based[1] - mean_variance32x32_based[1] * mean_variance32x32_based[1];

            mean_sqr_variance32x32_based[2] = mean_sqr_variance32x32_based[2] >> 4;
            mean_variance32x32_based[2] = mean_variance32x32_based[2] >> 4;
            picture_control_set_ptr->var_of_var_32x32_based_sb_array[sb_index][2] = mean_sqr_variance32x32_based[2] - mean_variance32x32_based[2] * mean_variance32x32_based[2];

            mean_sqr_variance32x32_based[3] = mean_sqr_variance32x32_based[3] >> 4;
            mean_variance32x32_based[3] = mean_variance32x32_based[3] >> 4;
            picture_control_set_ptr->var_of_var_32x32_based_sb_array[sb_index][3] = mean_sqr_variance32x32_based[3] - mean_variance32x32_based[3] * mean_variance32x32_based[3];

            // Compute the 64x64 based variance of variance
            {
                uint32_t var_index;
                // Loop over all 8x8s in a 64x64
                for (var_index = ME_TIER_ZERO_PU_8x8_0; var_index <= ME_TIER_ZERO_PU_8x8_63; var_index++) {
                    mean_sqr_variance64x64_based += variance_ptr[var_index] * variance_ptr[var_index];
                    mean_variance64x64_based += variance_ptr[var_index];
                }

                mean_sqr_variance64x64_based = mean_sqr_variance64x64_based >> 6;
                mean_variance64x64_based = mean_variance64x64_based >> 6;

                // Compute variance
                var_of_var64x64_based = mean_sqr_variance64x64_based - mean_variance64x64_based * mean_variance64x64_based;

                // Turn off detail preservation if the varOfVar is greater than a threshold
                if (var_of_var64x64_based > VAR_BASED_DETAIL_PRESERVATION_SELECTOR_THRSLHD)
                {
                    picture_control_set_ptr->sb_homogeneous_area_array[sb_index] = EB_FALSE;
                }
            }

        }
        else{

            // Should be re-calculated and scaled properly
            picture_control_set_ptr->var_of_var_32x32_based_sb_array[sb_index][0] = 0xFFFFFFFFFFFFFFFF;
            picture_control_set_ptr->var_of_var_32x32_based_sb_array[sb_index][1] = 0xFFFFFFFFFFFFFFFF;
            picture_control_set_ptr->var_of_var_32x32_based_sb_array[sb_index][2] = 0xFFFFFFFFFFFFFFFF;
            picture_control_set_ptr->var_of_var_32x32_based_sb_array[sb_index][3] = 0xFFFFFFFFFFFFFFFF;
        }
    }
    picture_control_set_ptr->very_low_var_pic_flag = EB_FALSE;
    if (var_sb_cnt > 0 && (((very_low_var_cnt * 100) / var_sb_cnt) > PIC_LOW_VAR_PERCENTAGE_TH)){
        picture_control_set_ptr->very_low_var_pic_flag = EB_TRUE;
    }

    picture_control_set_ptr->logo_pic_flag = EB_FALSE;
    if (var_sb_cnt > 0 && (((very_low_var_cnt * 100) / var_sb_cnt) > 80)){
        picture_control_set_ptr->logo_pic_flag = EB_TRUE;
    }

    return;
}
/************************************************
 * compute_picture_spatial_statistics
 ** Compute Block Variance
 ** Compute Picture Variance
 ** Compute Block Mean for all blocks in the picture
 ************************************************/
static void compute_picture_spatial_statistics(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    PictureAnalysisContext  *context_ptr,
    EbPictureBufferDesc     *input_picture_ptr,
    EbPictureBufferDesc     *input_padded_picture_ptr,
    uint32_t                 sb_total_count)
{
    uint32_t sb_index;
    uint32_t sb_origin_x;        // to avoid using child PCS
    uint32_t sb_origin_y;
    uint32_t input_luma_origin_index;
    uint32_t input_cb_origin_index;
    uint32_t input_cr_origin_index;
    uint64_t pic_tot_variance;

    // Variance
    pic_tot_variance = 0;

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {
        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

        sb_origin_x = sb_params->origin_x;
        sb_origin_y = sb_params->origin_y;
        input_luma_origin_index = (input_padded_picture_ptr->origin_y + sb_origin_y) * input_padded_picture_ptr->stride_y +
            input_padded_picture_ptr->origin_x + sb_origin_x;

        input_cb_origin_index = ((input_picture_ptr->origin_y + sb_origin_y) >> 1) * input_picture_ptr->stride_cb + ((input_picture_ptr->origin_x + sb_origin_x) >> 1);
        input_cr_origin_index = ((input_picture_ptr->origin_y + sb_origin_y) >> 1) * input_picture_ptr->stride_cr + ((input_picture_ptr->origin_x + sb_origin_x) >> 1);

        compute_block_mean_compute_variance(
            picture_control_set_ptr,
            input_padded_picture_ptr,
            sb_index,
            input_luma_origin_index);

        if (sb_params->is_complete_sb){

            compute_chroma_block_mean(
                picture_control_set_ptr,
                input_picture_ptr,
                sb_index,
                input_cb_origin_index,
                input_cr_origin_index);
        }
        else{
            zero_out_chroma_block_mean(
                picture_control_set_ptr,
                sb_index);
        }

        pic_tot_variance += (picture_control_set_ptr->variance[sb_index][PA_RASTER_SCAN_CU_INDEX_64x64]);
    }

    picture_control_set_ptr->pic_avg_variance = (uint16_t) (pic_tot_variance / sb_total_count);
    // Calculate the variance of variance to determine Homogeneous regions. Note: Variance calculation should be on.
    determine_homogeneous_region_in_picture(
        sequence_control_set_ptr,
        picture_control_set_ptr);

    edge_detection_mean_luma_chroma16x16(
        sequence_control_set_ptr,
        picture_control_set_ptr,
        context_ptr,
        sequence_control_set_ptr->sb_total_count);

    edge_detection(
        sequence_control_set_ptr,
        picture_control_set_ptr);

    return;
}

static void calculate_input_average_intensity(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *input_picture_ptr,
    uint64_t                 sum_average_intensity_total_regions_luma,
    uint64_t                 sum_average_intensity_total_regions_cb,
    uint64_t                 sum_average_intensity_total_regions_cr)
{

    if (sequence_control_set_ptr->scd_mode == SCD_MODE_0){
        uint16_t block_index_in_width;
        uint16_t block_index_in_height;
        uint64_t mean = 0;

        const uint16_t stride_y = input_picture_ptr->stride_y;

        // Loop over 8x8 blocks and calculates the mean value
        for (block_index_in_height = 0; block_index_in_height < input_picture_ptr->height >> 3; ++block_index_in_height) {
            for (block_index_in_width = 0; block_index_in_width < input_picture_ptr->width >> 3; ++block_index_in_width) {
                mean += eb_vp9_compute_sub_mean8x8_sse2_intrin(&(input_picture_ptr->buffer_y[(block_index_in_width << 3) + (block_index_in_height << 3) * stride_y]), stride_y);
            }
        }

        mean = ((mean + ((input_picture_ptr->height* input_picture_ptr->width) >> 7)) / ((input_picture_ptr->height* input_picture_ptr->width) >> 6));
        mean = (mean + (1 << (MEAN_PRECISION - 1))) >> MEAN_PRECISION;
        picture_control_set_ptr->average_intensity[0] = (uint8_t)mean;
    }

    else{
        picture_control_set_ptr->average_intensity[0] = (uint8_t)((sum_average_intensity_total_regions_luma + ((input_picture_ptr->width*input_picture_ptr->height) >> 1)) / (input_picture_ptr->width*input_picture_ptr->height));
        picture_control_set_ptr->average_intensity[1] = (uint8_t)((sum_average_intensity_total_regions_cb + ((input_picture_ptr->width*input_picture_ptr->height) >> 3)) / ((input_picture_ptr->width*input_picture_ptr->height) >> 2));
        picture_control_set_ptr->average_intensity[2] = (uint8_t)((sum_average_intensity_total_regions_cr + ((input_picture_ptr->width*input_picture_ptr->height) >> 3)) / ((input_picture_ptr->width*input_picture_ptr->height) >> 2));
    }

    return;
}

/************************************************
 * Gathering statistics per picture
 ** Calculating the pixel intensity histogram bins per picture needed for SCD
 ** Computing Picture Variance
 ************************************************/
static void gathering_picture_statistics(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    PictureAnalysisContext  *context_ptr,
    EbPictureBufferDesc     *input_picture_ptr,
    EbPictureBufferDesc     *input_padded_picture_ptr,
    EbPictureBufferDesc     *sixteenth_decimated_picture_ptr,
    uint32_t                 sb_total_count)
{

    uint64_t sum_average_intensity_total_regions_luma = 0;
    uint64_t sum_average_intensity_total_regions_cb = 0;
    uint64_t sum_average_intensity_total_regions_cr = 0;

    // Histogram bins
   // Use 1/16 Luma for Histogram generation
   // 1/16 input ready
   sub_sample_luma_generate_pixel_intensity_histogram_bins(
       sequence_control_set_ptr,
       picture_control_set_ptr,
       sixteenth_decimated_picture_ptr,
       &sum_average_intensity_total_regions_luma);

   // Use 1/4 Chroma for Histogram generation
   // 1/4 input not ready => perform operation on the fly
   sub_sample_chroma_generate_pixel_intensity_histogram_bins(
       sequence_control_set_ptr,
       picture_control_set_ptr,
       input_picture_ptr,
       &sum_average_intensity_total_regions_cb,
       &sum_average_intensity_total_regions_cr);

    // Calculate the LUMA average intensity
    calculate_input_average_intensity(
        sequence_control_set_ptr,
        picture_control_set_ptr,
        input_picture_ptr,
        sum_average_intensity_total_regions_luma,
        sum_average_intensity_total_regions_cb,
        sum_average_intensity_total_regions_cr);

    compute_picture_spatial_statistics(
        sequence_control_set_ptr,
        picture_control_set_ptr,
        context_ptr,
        input_picture_ptr,
        input_padded_picture_ptr,
        sb_total_count);

    return;
}
/************************************************
 * Pad Picture at the right and bottom sides
 ** To match a multiple of min CU size in width and height
 ************************************************/
void pad_picture_to_multiple_of_min_cu_size_dimensions(
    SequenceControlSet  *sequence_control_set_ptr,
    EbPictureBufferDesc *input_picture_ptr)
{
    EB_BOOL is_16bit_input = (EB_BOOL)(sequence_control_set_ptr->static_config.encoder_bit_depth > EB_8BIT);

    // Input Picture Padding
    eb_vp9_pad_input_picture(
        &input_picture_ptr->buffer_y[input_picture_ptr->origin_x + (input_picture_ptr->origin_y * input_picture_ptr->stride_y)],
        input_picture_ptr->stride_y,
        (input_picture_ptr->width - sequence_control_set_ptr->pad_right),
        (input_picture_ptr->height - sequence_control_set_ptr->pad_bottom),
        sequence_control_set_ptr->pad_right,
        sequence_control_set_ptr->pad_bottom);

    eb_vp9_pad_input_picture(
        &input_picture_ptr->buffer_cb[(input_picture_ptr->origin_x >> 1) + ((input_picture_ptr->origin_y >> 1) * input_picture_ptr->stride_cb)],
        input_picture_ptr->stride_cb,
        (input_picture_ptr->width - sequence_control_set_ptr->pad_right) >> 1,
        (input_picture_ptr->height - sequence_control_set_ptr->pad_bottom) >> 1,
        sequence_control_set_ptr->pad_right >> 1,
        sequence_control_set_ptr->pad_bottom >> 1);

    eb_vp9_pad_input_picture(
        &input_picture_ptr->buffer_cr[(input_picture_ptr->origin_x >> 1) + ((input_picture_ptr->origin_y >> 1) * input_picture_ptr->stride_cr)],
        input_picture_ptr->stride_cr,
        (input_picture_ptr->width - sequence_control_set_ptr->pad_right) >> 1,
        (input_picture_ptr->height - sequence_control_set_ptr->pad_bottom) >> 1,
        sequence_control_set_ptr->pad_right >> 1,
        sequence_control_set_ptr->pad_bottom >> 1);

    if (is_16bit_input)
    {
        eb_vp9_pad_input_picture(
            &input_picture_ptr->buffer_bit_inc_y[input_picture_ptr->origin_x + (input_picture_ptr->origin_y * input_picture_ptr->stride_bit_inc_y)],
            input_picture_ptr->stride_bit_inc_y,
            (input_picture_ptr->width - sequence_control_set_ptr->pad_right),
            (input_picture_ptr->height - sequence_control_set_ptr->pad_bottom),
            sequence_control_set_ptr->pad_right,
            sequence_control_set_ptr->pad_bottom);

        eb_vp9_pad_input_picture(
            &input_picture_ptr->buffer_bit_inc_cb[(input_picture_ptr->origin_x >> 1) + ((input_picture_ptr->origin_y >> 1) * input_picture_ptr->stride_bit_inc_cb)],
            input_picture_ptr->stride_bit_inc_cb,
            (input_picture_ptr->width - sequence_control_set_ptr->pad_right) >> 1,
            (input_picture_ptr->height - sequence_control_set_ptr->pad_bottom) >> 1,
            sequence_control_set_ptr->pad_right >> 1,
            sequence_control_set_ptr->pad_bottom >> 1);

        eb_vp9_pad_input_picture(
            &input_picture_ptr->buffer_bit_inc_cr[(input_picture_ptr->origin_x >> 1) + ((input_picture_ptr->origin_y >> 1) * input_picture_ptr->stride_bit_inc_cr)],
            input_picture_ptr->stride_bit_inc_cr,
            (input_picture_ptr->width - sequence_control_set_ptr->pad_right) >> 1,
            (input_picture_ptr->height - sequence_control_set_ptr->pad_bottom) >> 1,
            sequence_control_set_ptr->pad_right >> 1,
            sequence_control_set_ptr->pad_bottom >> 1);

    }

    return;
}

/************************************************
 * Pad Picture at the right and bottom sides
 ** To complete border LCU smaller than LCU size
 ************************************************/
static void pad_picture_to_multiple_of_sb_dimensions(
    EbPictureBufferDesc *input_padded_picture_ptr)
{

    // Generate Padding
    eb_vp9_generate_padding(
        &input_padded_picture_ptr->buffer_y[0],
        input_padded_picture_ptr->stride_y,
        input_padded_picture_ptr->width,
        input_padded_picture_ptr->height,
        input_padded_picture_ptr->origin_x,
        input_padded_picture_ptr->origin_y);

    return;
}

/************************************************
* 1/4 & 1/16 input picture decimation
************************************************/
void decimate_input_picture(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *input_padded_picture_ptr,
    EbPictureBufferDesc     *quarter_decimated_picture_ptr,
    EbPictureBufferDesc     *sixteenth_decimated_picture_ptr) {

    // Decimate input picture for HME L1
    EB_BOOL  preform_quarter_pell_decimation_flag;
    if (sequence_control_set_ptr->static_config.speed_control_flag){
        preform_quarter_pell_decimation_flag = EB_TRUE;
    }
    else{
        if (picture_control_set_ptr->enable_hme_level_1_flag == 1){
            preform_quarter_pell_decimation_flag = EB_TRUE;
        }
        else{
            preform_quarter_pell_decimation_flag = EB_FALSE;
        }
    }

    if (preform_quarter_pell_decimation_flag) {
        eb_vp9_decimation_2d(
                &input_padded_picture_ptr->buffer_y[input_padded_picture_ptr->origin_x + input_padded_picture_ptr->origin_y * input_padded_picture_ptr->stride_y],
                input_padded_picture_ptr->stride_y,
                input_padded_picture_ptr->width ,
                input_padded_picture_ptr->height,
                &quarter_decimated_picture_ptr->buffer_y[quarter_decimated_picture_ptr->origin_x+quarter_decimated_picture_ptr->origin_x*quarter_decimated_picture_ptr->stride_y],
                quarter_decimated_picture_ptr->stride_y,
                2);

            eb_vp9_generate_padding(
                &quarter_decimated_picture_ptr->buffer_y[0],
                quarter_decimated_picture_ptr->stride_y,
                quarter_decimated_picture_ptr->width,
                quarter_decimated_picture_ptr->height,
                quarter_decimated_picture_ptr->origin_x,
                quarter_decimated_picture_ptr->origin_y);

    }

    // Decimate input picture for HME L0
    // Sixteenth Input Picture Decimation
    eb_vp9_decimation_2d(
        &input_padded_picture_ptr->buffer_y[input_padded_picture_ptr->origin_x + input_padded_picture_ptr->origin_y * input_padded_picture_ptr->stride_y],
        input_padded_picture_ptr->stride_y,
        input_padded_picture_ptr->width ,
        input_padded_picture_ptr->height ,
        &sixteenth_decimated_picture_ptr->buffer_y[sixteenth_decimated_picture_ptr->origin_x+sixteenth_decimated_picture_ptr->origin_x*sixteenth_decimated_picture_ptr->stride_y],
        sixteenth_decimated_picture_ptr->stride_y,
        4);

    eb_vp9_generate_padding(
        &sixteenth_decimated_picture_ptr->buffer_y[0],
        sixteenth_decimated_picture_ptr->stride_y,
        sixteenth_decimated_picture_ptr->width,
        sixteenth_decimated_picture_ptr->height,
        sixteenth_decimated_picture_ptr->origin_x,
        sixteenth_decimated_picture_ptr->origin_y);

}

/************************************************
 * Picture Analysis Kernel
 * The Picture Analysis Process pads & decimates the input pictures.
 * The Picture Analysis also includes creating an n-bin Histogram,
 * gathering picture 1st and 2nd moment statistics for each 8x8 block,
 * which are used to compute variance.
 * The Picture Analysis process is multithreaded, so pictures can be
 * processed out of order as long as all inputs are available.
 ************************************************/
void* eb_vp9_picture_analysis_kernel(void *input_ptr)
{
    PictureAnalysisContext      *context_ptr = (PictureAnalysisContext*)input_ptr;
    PictureParentControlSet     *picture_control_set_ptr;
    SequenceControlSet          *sequence_control_set_ptr;

    EbObjectWrapper             *input_results_wrapper_ptr;
    ResourceCoordinationResults *input_results_ptr;
    EbObjectWrapper             *output_results_wrapper_ptr;
    PictureAnalysisResults      *output_results_ptr;
    EbPaReferenceObject         *pa_reference_object;

    EbPictureBufferDesc         *input_padded_picture_ptr;
    EbPictureBufferDesc         *quarter_decimated_picture_ptr;
    EbPictureBufferDesc         *sixteenth_decimated_picture_ptr;
    EbPictureBufferDesc         *input_picture_ptr;

    // Variance
    uint32_t                     picture_width_in_sb;
    uint32_t                     pictureHeighInLcu;
    uint32_t                     sb_total_count;

    for (;;) {

        // Get Input Full Object
        eb_vp9_get_full_object(
            context_ptr->resource_coordination_results_input_fifo_ptr,
            &input_results_wrapper_ptr);

        input_results_ptr = (ResourceCoordinationResults  *)input_results_wrapper_ptr->object_ptr;
        picture_control_set_ptr = (PictureParentControlSet  *)input_results_ptr->picture_control_set_wrapper_ptr->object_ptr;
        sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
        input_picture_ptr = picture_control_set_ptr->enhanced_picture_ptr;

        pa_reference_object = (EbPaReferenceObject  *)picture_control_set_ptr->pareference_picture_wrapper_ptr->object_ptr;
        input_padded_picture_ptr = (EbPictureBufferDesc  *)pa_reference_object->input_padded_picture_ptr;
        quarter_decimated_picture_ptr = (EbPictureBufferDesc  *)pa_reference_object->quarter_decimated_picture_ptr;
        sixteenth_decimated_picture_ptr = (EbPictureBufferDesc  *)pa_reference_object->sixteenth_decimated_picture_ptr;

        // Variance
        picture_width_in_sb = (sequence_control_set_ptr->luma_width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE;
        pictureHeighInLcu = (sequence_control_set_ptr->luma_height + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE;
        sb_total_count = picture_width_in_sb * pictureHeighInLcu;

        // Set picture parameters to account for subpicture, picture scantype, and set regions by resolutions
        set_picture_parameters_for_statistics_gathering(
            sequence_control_set_ptr);

        // Pad pictures to multiple min cu size
        pad_picture_to_multiple_of_min_cu_size_dimensions(
            sequence_control_set_ptr,
            input_picture_ptr);

        // Pre processing operations performed on the input picture
        picture_pre_processing_operations(
            picture_control_set_ptr,
#if !TURN_OFF_PRE_PROCESSING
            input_picture_ptr,
#endif
            context_ptr,
            sequence_control_set_ptr,
            quarter_decimated_picture_ptr,
            sixteenth_decimated_picture_ptr,
            sb_total_count,
            picture_width_in_sb);

        // Pad input picture to complete border LCUs
        pad_picture_to_multiple_of_sb_dimensions(
            input_padded_picture_ptr
        );

        // 1/4 & 1/16 input picture decimation
        decimate_input_picture(
            sequence_control_set_ptr,
            picture_control_set_ptr,
            input_padded_picture_ptr,
            quarter_decimated_picture_ptr,
            sixteenth_decimated_picture_ptr);

        // Gathering statistics of input picture, including Variance Calculation, Histogram Bins
        gathering_picture_statistics(
            sequence_control_set_ptr,
            picture_control_set_ptr,
            context_ptr,
            input_picture_ptr,
            input_padded_picture_ptr,
            sixteenth_decimated_picture_ptr,
            sb_total_count);

        // Hold the 64x64 variance and mean in the reference frame
        uint32_t sb_index;
        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index){
            pa_reference_object->variance[sb_index] = picture_control_set_ptr->variance[sb_index][ME_TIER_ZERO_PU_64x64];
            pa_reference_object->y_mean[sb_index] = picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_64x64];

        }

        // Get Empty Results Object
        eb_vp9_get_empty_object(
            context_ptr->picture_analysis_results_output_fifo_ptr,
            &output_results_wrapper_ptr);

        output_results_ptr = (PictureAnalysisResults*)output_results_wrapper_ptr->object_ptr;
        output_results_ptr->picture_control_set_wrapper_ptr = input_results_ptr->picture_control_set_wrapper_ptr;

        // Release the Input Results
        eb_vp9_release_object(input_results_wrapper_ptr);

        // Post the Full Results Object
        eb_vp9_post_full_object(output_results_wrapper_ptr);

    }
    return EB_NULL;
}

void unused_variablevoid_func_pa()
{
    (void)eb_vp9_sad_calculation_8x8_16x16_func_ptr_array;
    (void)eb_vp9_sad_calculation_32x32_64x64_func_ptr_array;
}
