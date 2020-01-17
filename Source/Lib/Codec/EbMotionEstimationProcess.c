/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbUtility.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbPictureDecisionResults.h"
#include "EbMotionEstimationProcess.h"
#include "EbMotionEstimationResults.h"
#include "EbReferenceObject.h"
#include "EbMotionEstimation.h"
#include "EbDefinitions.h"
#include "EbComputeSAD.h"

#include "emmintrin.h"

#define SQUARE_PU_NUM  85
#define BUFF_CHECK_SIZE    128

#define DERIVE_INTRA_32_FROM_16   0 //CHKN 1

/* --32x32-
|00||01|
|02||03|
--------*/
/* ------16x16-----
|00||01||04||05|
|02||03||06||07|
|08||09||12||13|
|10||11||14||15|
----------------*/
/* ------8x8----------------------------
|00||01||04||05|     |16||17||20||21|
|02||03||06||07|     |18||19||22||23|
|08||09||12||13|     |24||25||28||29|
|10||11||14||15|     |26||27||30||31|

|32||33||36||37|     |48||49||52||53|
|34||35||38||39|     |50||51||54||55|
|40||41||44||45|     |56||57||60||61|
|42||43||46||47|     |58||59||62||63|
-------------------------------------*/
EbErrorType check_zero_zero_center(
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *ref_pic_ptr,
    MeContext               *context_ptr,
    uint32_t                 sb_origin_x,
    uint32_t                 sb_origin_y,
    uint32_t                 sb_width,
    uint32_t                 sb_height,
    int16_t                 *x_search_center,
    int16_t                 *y_search_center);

/************************************************
 * Set ME/HME Params
 ************************************************/
void* set_me_hme_params_sq(
    MeContext               *me_context_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    EB_INPUT_RESOLUTION      input_resolution)
{

    uint8_t  hme_me_level = picture_control_set_ptr->enc_mode;

    uint32_t input_ratio = sequence_control_set_ptr->luma_width / sequence_control_set_ptr->luma_height;

    uint8_t resolution_index = input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER ? 0 : // 480P
        (input_resolution <= INPUT_SIZE_1080i_RANGE && input_ratio < 2) ? 1 : // 720P
        (input_resolution <= INPUT_SIZE_1080i_RANGE && input_ratio > 3) ? 2 : // 1080I
        (input_resolution <= INPUT_SIZE_1080p_RANGE) ? 3 : // 1080I
        4;  // 4K

// HME/ME default settings
    me_context_ptr->number_hme_search_region_in_width = 2;
    me_context_ptr->number_hme_search_region_in_height = 2;

    // HME Level0
    me_context_ptr->hme_level0_total_search_area_width = hme_level0_total_search_area_width_sq[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_total_search_area_height = hme_level0_total_search_area_height_sq[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_search_area_in_width_array[0] = hme_level0_search_area_in_width_array_right_sq[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_search_area_in_width_array[1] = hme_level0_search_area_in_width_array_left_sq[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_search_area_in_height_array[0] = hme_level0_search_area_in_height_array_top_sq[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_search_area_in_height_array[1] = hme_level0_search_area_in_height_array_bottom_sq[resolution_index][hme_me_level];
    // HME Level1
    me_context_ptr->hme_level1_search_area_in_width_array[0] = hme_level1_search_area_in_width_array_right_sq[resolution_index][hme_me_level];
    me_context_ptr->hme_level1_search_area_in_width_array[1] = hme_level1_search_area_in_width_array_left_sq[resolution_index][hme_me_level];
    me_context_ptr->hme_level1_search_area_in_height_array[0] = hme_level1_search_area_in_height_array_top_sq[resolution_index][hme_me_level];
    me_context_ptr->hme_level1_search_area_in_height_array[1] = hme_level1_search_area_in_height_array_bottom_sq[resolution_index][hme_me_level];
    // HME Level2
    me_context_ptr->hme_level2_search_area_in_width_array[0] = hme_level2_search_area_in_width_array_right_sq[resolution_index][hme_me_level];
    me_context_ptr->hme_level2_search_area_in_width_array[1] = hme_level2_search_area_in_width_array_left_sq[resolution_index][hme_me_level];
    me_context_ptr->hme_level2_search_area_in_height_array[0] = hme_level2_search_area_in_height_array_top_sq[resolution_index][hme_me_level];
    me_context_ptr->hme_level2_search_area_in_height_array[1] = hme_level2_search_area_in_height_array_bottom_sq[resolution_index][hme_me_level];

    // ME
    me_context_ptr->search_area_width = search_area_width_sq[resolution_index][hme_me_level];
    me_context_ptr->search_area_height = search_area_height_sq[resolution_index][hme_me_level];

    // HME Level0 adjustment for low frame rate contents (frame rate <= 30)
    if (input_resolution == INPUT_SIZE_4K_RANGE) {
        if ((sequence_control_set_ptr->static_config.frame_rate >> 16) <= 30) {

            if (hme_me_level == ENC_MODE_5 || hme_me_level == ENC_MODE_6) {
                me_context_ptr->hme_level0_total_search_area_width = MAX(128, me_context_ptr->hme_level0_total_search_area_width);
                me_context_ptr->hme_level0_total_search_area_height = MAX(64, me_context_ptr->hme_level0_total_search_area_height);
                me_context_ptr->hme_level0_search_area_in_width_array[0] = MAX(64, me_context_ptr->hme_level0_search_area_in_width_array[0]);
                me_context_ptr->hme_level0_search_area_in_width_array[1] = MAX(64, me_context_ptr->hme_level0_search_area_in_width_array[1]);
                me_context_ptr->hme_level0_search_area_in_height_array[0] = MAX(32, me_context_ptr->hme_level0_search_area_in_height_array[0]);
                me_context_ptr->hme_level0_search_area_in_height_array[1] = MAX(32, me_context_ptr->hme_level0_search_area_in_height_array[1]);
            }
            else if (hme_me_level == ENC_MODE_7 || hme_me_level == ENC_MODE_8) {
                me_context_ptr->hme_level0_total_search_area_width = MAX(96, me_context_ptr->hme_level0_total_search_area_width);
                me_context_ptr->hme_level0_total_search_area_height = MAX(64, me_context_ptr->hme_level0_total_search_area_height);
                me_context_ptr->hme_level0_search_area_in_width_array[0] = MAX(48, me_context_ptr->hme_level0_search_area_in_width_array[0]);
                me_context_ptr->hme_level0_search_area_in_width_array[1] = MAX(48, me_context_ptr->hme_level0_search_area_in_width_array[1]);
                me_context_ptr->hme_level0_search_area_in_height_array[0] = MAX(32, me_context_ptr->hme_level0_search_area_in_height_array[0]);
                me_context_ptr->hme_level0_search_area_in_height_array[1] = MAX(32, me_context_ptr->hme_level0_search_area_in_height_array[1]);
            }
            else if (hme_me_level >= ENC_MODE_9) {
                me_context_ptr->hme_level0_total_search_area_width = MAX(64, me_context_ptr->hme_level0_total_search_area_width);
                me_context_ptr->hme_level0_total_search_area_height = MAX(48, me_context_ptr->hme_level0_total_search_area_height);
                me_context_ptr->hme_level0_search_area_in_width_array[0] = MAX(32, me_context_ptr->hme_level0_search_area_in_width_array[0]);
                me_context_ptr->hme_level0_search_area_in_width_array[1] = MAX(32, me_context_ptr->hme_level0_search_area_in_width_array[1]);
                me_context_ptr->hme_level0_search_area_in_height_array[0] = MAX(24, me_context_ptr->hme_level0_search_area_in_height_array[0]);
                me_context_ptr->hme_level0_search_area_in_height_array[1] = MAX(24, me_context_ptr->hme_level0_search_area_in_height_array[1]);
            }
        }
    }

    return EB_NULL;
};

/************************************************
 * Set ME/HME Params
 ************************************************/
void* eb_vp9_set_me_hme_params_oq(
    MeContext               *me_context_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    EB_INPUT_RESOLUTION      input_resolution)
{

    uint8_t  hme_me_level = picture_control_set_ptr->enc_mode;

    uint32_t input_ratio = sequence_control_set_ptr->luma_width / sequence_control_set_ptr->luma_height;

    uint8_t resolution_index = input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER ? 0 : // 480P
        (input_resolution <= INPUT_SIZE_1080i_RANGE && input_ratio < 2) ? 1 : // 720P
        (input_resolution <= INPUT_SIZE_1080i_RANGE && input_ratio > 3) ? 2 : // 1080I
        (input_resolution <= INPUT_SIZE_1080p_RANGE) ? 3 : // 1080I
        4;  // 4K

// HME/ME default settings
    me_context_ptr->number_hme_search_region_in_width = 2;
    me_context_ptr->number_hme_search_region_in_height = 2;

    // HME Level0
    me_context_ptr->hme_level0_total_search_area_width = hme_level0_total_search_area_width_oq[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_total_search_area_height = hme_level0_total_search_area_height_oq[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_search_area_in_width_array[0] = hme_level0_search_area_in_width_array_right_oq[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_search_area_in_width_array[1] = hme_level0_search_area_in_width_array_left_oq[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_search_area_in_height_array[0] = hme_level0_search_area_in_height_array_top_oq[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_search_area_in_height_array[1] = hme_level0_search_area_in_height_array_bottom_oq[resolution_index][hme_me_level];
    // HME Level1
    me_context_ptr->hme_level1_search_area_in_width_array[0] = hme_level1_search_area_in_width_array_right_oq[resolution_index][hme_me_level];
    me_context_ptr->hme_level1_search_area_in_width_array[1] = hme_level1_search_area_in_width_array_left_oq[resolution_index][hme_me_level];
    me_context_ptr->hme_level1_search_area_in_height_array[0] = hme_level1_search_area_in_height_array_top_oq[resolution_index][hme_me_level];
    me_context_ptr->hme_level1_search_area_in_height_array[1] = hme_level1_search_area_in_height_array_bottom_oq[resolution_index][hme_me_level];
    // HME Level2
    me_context_ptr->hme_level2_search_area_in_width_array[0] = hme_level2_search_area_in_width_array_right_oq[resolution_index][hme_me_level];
    me_context_ptr->hme_level2_search_area_in_width_array[1] = hme_level2_search_area_in_width_array_left_oq[resolution_index][hme_me_level];
    me_context_ptr->hme_level2_search_area_in_height_array[0] = hme_level2_search_area_in_height_array_top_oq[resolution_index][hme_me_level];
    me_context_ptr->hme_level2_search_area_in_height_array[1] = hme_level2_search_area_in_height_array_bottom_oq[resolution_index][hme_me_level];

    // ME
    me_context_ptr->search_area_width = search_area_width_oq[resolution_index][hme_me_level];
    me_context_ptr->search_area_height = search_area_height_oq[resolution_index][hme_me_level];

    // HME Level0 adjustment for low frame rate contents (frame rate <= 30)
    if (input_resolution == INPUT_SIZE_4K_RANGE) {
        if ((sequence_control_set_ptr->static_config.frame_rate >> 16) <= 30) {

            if (hme_me_level == ENC_MODE_6 || hme_me_level == ENC_MODE_7) {
                me_context_ptr->hme_level0_total_search_area_width = MAX(96, me_context_ptr->hme_level0_total_search_area_width);
                me_context_ptr->hme_level0_total_search_area_height = MAX(64, me_context_ptr->hme_level0_total_search_area_height);
                me_context_ptr->hme_level0_search_area_in_width_array[0] = MAX(48, me_context_ptr->hme_level0_search_area_in_width_array[0]);
                me_context_ptr->hme_level0_search_area_in_width_array[1] = MAX(48, me_context_ptr->hme_level0_search_area_in_width_array[1]);
                me_context_ptr->hme_level0_search_area_in_height_array[0] = MAX(32, me_context_ptr->hme_level0_search_area_in_height_array[0]);
                me_context_ptr->hme_level0_search_area_in_height_array[1] = MAX(32, me_context_ptr->hme_level0_search_area_in_height_array[1]);
            }
            else if (hme_me_level >= ENC_MODE_8) {
                me_context_ptr->hme_level0_total_search_area_width = MAX(64, me_context_ptr->hme_level0_total_search_area_width);
                me_context_ptr->hme_level0_total_search_area_height = MAX(48, me_context_ptr->hme_level0_total_search_area_height);
                me_context_ptr->hme_level0_search_area_in_width_array[0] = MAX(32, me_context_ptr->hme_level0_search_area_in_width_array[0]);
                me_context_ptr->hme_level0_search_area_in_width_array[1] = MAX(32, me_context_ptr->hme_level0_search_area_in_width_array[1]);
                me_context_ptr->hme_level0_search_area_in_height_array[0] = MAX(24, me_context_ptr->hme_level0_search_area_in_height_array[0]);
                me_context_ptr->hme_level0_search_area_in_height_array[1] = MAX(24, me_context_ptr->hme_level0_search_area_in_height_array[1]);
            }
        }
    }

    return EB_NULL;
};

/************************************************
 * Set ME/HME Params
 ************************************************/
void* set_me_hme_params_vmaf(
    MeContext               *me_context_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    EB_INPUT_RESOLUTION      input_resolution)
{

    uint8_t  hme_me_level = picture_control_set_ptr->enc_mode;

    uint32_t input_ratio = sequence_control_set_ptr->luma_width / sequence_control_set_ptr->luma_height;

    uint8_t resolution_index = input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER ? 0 : // 480P
        (input_resolution <= INPUT_SIZE_1080i_RANGE && input_ratio < 2) ? 1 : // 720P
        (input_resolution <= INPUT_SIZE_1080i_RANGE && input_ratio > 3) ? 2 : // 1080I
        (input_resolution <= INPUT_SIZE_1080p_RANGE) ? 3 : // 1080I
        4;  // 4K

    // HME/ME default settings
    me_context_ptr->number_hme_search_region_in_width = 2;
    me_context_ptr->number_hme_search_region_in_height = 2;
    resolution_index = 3;
    // HME Level0
    me_context_ptr->hme_level0_total_search_area_width = hme_level0_total_search_area_width_vmaf[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_total_search_area_height = hme_level0_total_search_area_height_vmaf[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_search_area_in_width_array[0] = hme_level0_search_area_in_width_array_right_vmaf[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_search_area_in_width_array[1] = hme_level0_search_area_in_width_array_left_vmaf[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_search_area_in_height_array[0] = hme_level0_search_area_in_height_array_top_vmaf[resolution_index][hme_me_level];
    me_context_ptr->hme_level0_search_area_in_height_array[1] = hme_level0_search_area_in_height_array_bottom_vmaf[resolution_index][hme_me_level];
    // HME Level1
    me_context_ptr->hme_level1_search_area_in_width_array[0] = hme_level1_search_area_in_width_array_right_vmaf[resolution_index][hme_me_level];
    me_context_ptr->hme_level1_search_area_in_width_array[1] = hme_level1_search_area_in_width_array_left_vmaf[resolution_index][hme_me_level];
    me_context_ptr->hme_level1_search_area_in_height_array[0] = hme_level1_search_area_in_height_array_top_vmaf[resolution_index][hme_me_level];
    me_context_ptr->hme_level1_search_area_in_height_array[1] = hme_level1_search_area_in_height_array_bottom_vmaf[resolution_index][hme_me_level];
    // HME Level2
    me_context_ptr->hme_level2_search_area_in_width_array[0] = hme_level2_search_area_in_width_array_right_vmaf[resolution_index][hme_me_level];
    me_context_ptr->hme_level2_search_area_in_width_array[1] = hme_level2_search_area_in_width_array_left_vmaf[resolution_index][hme_me_level];
    me_context_ptr->hme_level2_search_area_in_height_array[0] = hme_level2_search_area_in_height_array_top_vmaf[resolution_index][hme_me_level];
    me_context_ptr->hme_level2_search_area_in_height_array[1] = hme_level2_search_area_in_height_array_bottom_vmaf[resolution_index][hme_me_level];

    // ME
    me_context_ptr->search_area_width = search_area_width_vmaf[resolution_index][hme_me_level];
    me_context_ptr->search_area_height = search_area_height_vmaf[resolution_index][hme_me_level];

    // HME Level0 adjustment for low frame rate contents (frame rate <= 30)

    return EB_NULL;
};

/************************************************
 * Set ME/HME Params from Config
 ************************************************/
void* eb_vp9_set_me_hme_params_from_confi(
    SequenceControlSet *sequence_control_set_ptr,
    MeContext          *me_context_ptr)
{

    me_context_ptr->search_area_width = (uint8_t)sequence_control_set_ptr->static_config.search_area_width;
    me_context_ptr->search_area_height = (uint8_t)sequence_control_set_ptr->static_config.search_area_height;

    return EB_NULL;
}

/************************************************
 * Motion Analysis Context Constructor
 ************************************************/

EbErrorType eb_vp9_motion_estimation_context_ctor(
    MotionEstimationContext **context_dbl_ptr,
    EbFifo                   *picture_decision_results_input_fifo_ptr,
    EbFifo                   *motion_estimation_results_output_fifo_ptr) {

    EbErrorType return_error = EB_ErrorNone;
    MotionEstimationContext *context_ptr;
    EB_MALLOC(MotionEstimationContext*, context_ptr, sizeof(MotionEstimationContext), EB_N_PTR);

    *context_dbl_ptr = context_ptr;

    context_ptr->picture_decision_results_input_fifo_ptr = picture_decision_results_input_fifo_ptr;
    context_ptr->motion_estimation_results_output_fifo_ptr = motion_estimation_results_output_fifo_ptr;

    return_error = eb_vp9_me_context_ctor(&(context_ptr->me_context_ptr));
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    return EB_ErrorNone;

}

/***************************************************************************************************
* ZZ SAD
***************************************************************************************************/
int non_moving_th_shift[4] = { 4, 2, 0, 0};

#if 0//ADAPTIVE_QP_INDEX_GEN
EbErrorType compute_zz_sad(
    MotionEstimationContext *context_ptr,
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    EbPictureBufferDesc     *input_padded_picture_ptr,
    uint32_t                 x_sb_start_index,
    uint32_t                 x_sb_end_index,
    uint32_t                 y_sb_start_index,
    uint32_t                 y_sb_end_index) {

    EbErrorType return_error = EB_ErrorNone;

    PictureParentControlSet      *previous_picture_control_set_wrapper_ptr = ((PictureParentControlSet  *)picture_control_set_ptr->previous_picture_control_set_wrapper_ptr->object_ptr);
    EbPictureBufferDesc          *previous_input = previous_picture_control_set_wrapper_ptr->enhanced_picture_ptr;

    uint32_t sb_index;

    uint32_t sb_origin_x;
    uint32_t sb_origin_y;

    uint32_t block_index_sub;
    uint32_t block_index;

    uint32_t zz_sad;

    uint32_t xsb_index;
    uint32_t ysb_index;

    for (ysb_index = y_sb_start_index; ysb_index < y_sb_end_index; ++ysb_index) {
        for (xsb_index = x_sb_start_index; xsb_index < x_sb_end_index; ++xsb_index) {

            sb_index = xsb_index + ysb_index * sequence_control_set_ptr->picture_width_in_sb;
            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

            sb_origin_x = sb_params->origin_x;
            sb_origin_y = sb_params->origin_y;

            uint32_t zz_sad = 0;

            if (sb_params->is_complete_sb)
            {

                block_index_sub = (input_padded_picture_ptr->origin_y    + sb_origin_y) * input_padded_picture_ptr->stride_y    + (input_padded_picture_ptr->origin_x    + sb_origin_x);
                block_index      = (previous_input->origin_y + sb_origin_y) * previous_input->stride_y + (previous_input->origin_x + sb_origin_x);

                // ZZ SAD between current and collocated
                zz_sad = n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][MAX_SB_SIZE >> 3](
                    &(input_padded_picture_ptr->buffer_y[block_index_sub]),
                    input_padded_picture_ptr->stride_y,
                    &(previous_input->buffer_y[block_index]),
                    previous_input->stride_y,
                    MAX_SB_SIZE, MAX_SB_SIZE);
            }
            else {
                zz_sad = (uint32_t)~0;
            }

            if (zz_sad < ((MAX_SB_SIZE * MAX_SB_SIZE) * 2) >> non_moving_th_shift[sequence_control_set_ptr->input_resolution]) {
                previous_picture_control_set_wrapper_ptr->non_moving_index_array[sb_index] = NON_MOVING_SCORE_0;
            }
            else if (zz_sad < ((MAX_SB_SIZE * MAX_SB_SIZE) * 4) >> non_moving_th_shift[sequence_control_set_ptr->input_resolution]) {
                previous_picture_control_set_wrapper_ptr->non_moving_index_array[sb_index] = NON_MOVING_SCORE_1;
            }
            else if (zz_sad < ((MAX_SB_SIZE * MAX_SB_SIZE) * 8) >> non_moving_th_shift[sequence_control_set_ptr->input_resolution]) {
                previous_picture_control_set_wrapper_ptr->non_moving_index_array[sb_index] = NON_MOVING_SCORE_2;
            }
            else {
                previous_picture_control_set_wrapper_ptr->non_moving_index_array[sb_index] = NON_MOVING_SCORE_3;
            }
        }
    }

    return return_error;
}
#else
EbErrorType compute_zz_sad(
    MotionEstimationContext   *context_ptr,
    SequenceControlSet        *sequence_control_set_ptr,
    PictureParentControlSet     *picture_control_set_ptr,
    EbPictureBufferDesc         *sixteenth_decimated_picture_ptr,
    uint32_t                         x_sb_start_index,
    uint32_t                         x_sb_end_index,
    uint32_t                         y_sb_start_index,
    uint32_t                         y_sb_end_index) {

    EbErrorType return_error = EB_ErrorNone;

    PictureParentControlSet *previous_picture_control_set_wrapper_ptr = ((PictureParentControlSet  *)picture_control_set_ptr->previous_picture_control_set_wrapper_ptr->object_ptr);
    EbPictureBufferDesc     *previous_input = previous_picture_control_set_wrapper_ptr->enhanced_picture_ptr;

    uint32_t sb_index;

    uint32_t sb_width;
    uint32_t sb_height;

    uint32_t block_width;
    uint32_t block_height;

    uint32_t sb_origin_x;
    uint32_t sb_origin_y;

    uint32_t block_index_sub;
    uint32_t block_index;

    uint32_t zz_sad;

    uint32_t xsb_index;
    uint32_t ysb_index;

    for (ysb_index = y_sb_start_index; ysb_index < y_sb_end_index; ++ysb_index) {
        for (xsb_index = x_sb_start_index; xsb_index < x_sb_end_index; ++xsb_index) {

            sb_index = xsb_index + ysb_index * sequence_control_set_ptr->picture_width_in_sb;
            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

            sb_width = sb_params->width;
            sb_height = sb_params->height;

            sb_origin_x = sb_params->origin_x;
            sb_origin_y = sb_params->origin_y;

            sb_width = sb_params->width;
            sb_height = sb_params->height;

            block_width = sb_width >> 2;
            block_height = sb_height >> 2;

            zz_sad = 0;

            if (sb_params->is_complete_sb)
            {

                block_index_sub = (sixteenth_decimated_picture_ptr->origin_y + (sb_origin_y >> 2)) * sixteenth_decimated_picture_ptr->stride_y + sixteenth_decimated_picture_ptr->origin_x + (sb_origin_x >> 2);
                block_index = (previous_input->origin_y + sb_origin_y)* previous_input->stride_y + (previous_input->origin_x + sb_origin_x);

                // 1/16 collocated LCU decimation
                eb_vp9_decimation_2d(
                    &previous_input->buffer_y[block_index],
                    previous_input->stride_y,
                    MAX_SB_SIZE,
                    MAX_SB_SIZE,
                    context_ptr->me_context_ptr->sixteenth_sb_buffer,
                    context_ptr->me_context_ptr->sixteenth_sb_buffer_stride,
                    4);

                // ZZ SAD between 1/16 current & 1/16 collocated
                zz_sad = n_x_m_sad_kernel_func_ptr_array[(eb_vp9_ASM_TYPES & AVX2_MASK) && 1][2](
                    &(sixteenth_decimated_picture_ptr->buffer_y[block_index_sub]),
                    sixteenth_decimated_picture_ptr->stride_y,
                    context_ptr->me_context_ptr->sixteenth_sb_buffer,
                    context_ptr->me_context_ptr->sixteenth_sb_buffer_stride,
                    16, 16);
            }
            else {
                zz_sad = (uint32_t)~0;
            }
#if ADAPTIVE_QP_INDEX_GEN
            if (zz_sad < (((block_width * block_height) * 2) >> non_moving_th_shift[sequence_control_set_ptr->input_resolution])) {
                previous_picture_control_set_wrapper_ptr->non_moving_index_array[sb_index] = NON_MOVING_SCORE_0;
            }
            else if (zz_sad < (((block_width * block_height) * 4) >> non_moving_th_shift[sequence_control_set_ptr->input_resolution])) {
                previous_picture_control_set_wrapper_ptr->non_moving_index_array[sb_index] = NON_MOVING_SCORE_1;
            }
            else if (zz_sad < (((block_width * block_height) * 8) >> non_moving_th_shift[sequence_control_set_ptr->input_resolution])) {
                previous_picture_control_set_wrapper_ptr->non_moving_index_array[sb_index] = NON_MOVING_SCORE_2;
            }
            else {
                previous_picture_control_set_wrapper_ptr->non_moving_index_array[sb_index] = NON_MOVING_SCORE_3;
            }
#else
            if (zz_sad < (((block_width * block_height) * 2))) {
                previous_picture_control_set_wrapper_ptr->non_moving_index_array[sb_index] = NON_MOVING_SCORE_0;
            }
            else if (zz_sad < (((block_width * block_height) * 4))) {
                previous_picture_control_set_wrapper_ptr->non_moving_index_array[sb_index] = NON_MOVING_SCORE_1;
            }
            else if (zz_sad < (((block_width * block_height) * 8))) {
                previous_picture_control_set_wrapper_ptr->non_moving_index_array[sb_index] = NON_MOVING_SCORE_2;
            }
            else {
                previous_picture_control_set_wrapper_ptr->non_moving_index_array[sb_index] = NON_MOVING_SCORE_3;
            }
#endif
        }
    }

    return return_error;
}
#endif
/******************************************************
* Derive ME Settings for SQ
  Input   : encoder mode and tune
  Output  : ME Kernel signal(s)
******************************************************/
EbErrorType eb_vp9_signal_derivation_me_kernel_sq(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    MotionEstimationContext *context_ptr)
{

    EbErrorType return_error = EB_ErrorNone;

    // Set ME/HME search regions
    if (sequence_control_set_ptr->static_config.use_default_me_hme) {
        set_me_hme_params_sq(
            context_ptr->me_context_ptr,
            picture_control_set_ptr,
            sequence_control_set_ptr,
            sequence_control_set_ptr->input_resolution);
    }
    else {
        eb_vp9_set_me_hme_params_from_confi(
            sequence_control_set_ptr,
            context_ptr->me_context_ptr);
    }

    // Set number of quadrant(s)
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_7) {
        context_ptr->me_context_ptr->single_hme_quadrant = EB_FALSE;
    }
    else {
        if (sequence_control_set_ptr->input_resolution >= INPUT_SIZE_4K_RANGE) {
            context_ptr->me_context_ptr->single_hme_quadrant = EB_TRUE;
        }
        else {
            context_ptr->me_context_ptr->single_hme_quadrant = EB_FALSE;
        }
    }

    // Set ME Fractional Search Method
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
        context_ptr->me_context_ptr->fractional_search_method = SSD_SEARCH;
    }
    else {
        context_ptr->me_context_ptr->fractional_search_method = SUB_SAD_SEARCH;
    }

    // Set 64x64 Fractional Search Flag
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_2) {
        context_ptr->me_context_ptr->fractional_search64x64 = EB_TRUE;
    }
    else {
        context_ptr->me_context_ptr->fractional_search64x64 = EB_FALSE;
    }

    // Set fractional search model
    // 0: search all blocks
    // 1: selective based on Full-Search SAD & MV.
    // 2: off
    if (picture_control_set_ptr->use_subpel_flag == 1) {
        if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
            context_ptr->me_context_ptr->fractional_search_model = 0;
        }
        else {
            context_ptr->me_context_ptr->fractional_search_model = 1;
        }
    }
    else {
        context_ptr->me_context_ptr->fractional_search_model = 2;
    }

    return return_error;
}

/******************************************************
* Derive ME Settings for OQ
  Input   : encoder mode and tune
  Output  : ME Kernel signal(s)
******************************************************/
EbErrorType eb_vp9_signal_derivation_me_kernel_oq(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    MotionEstimationContext *context_ptr)
{

    EbErrorType return_error = EB_ErrorNone;

    // Set ME/HME search regions
    if (sequence_control_set_ptr->static_config.use_default_me_hme) {
        eb_vp9_set_me_hme_params_oq(
            context_ptr->me_context_ptr,
            picture_control_set_ptr,
            sequence_control_set_ptr,
            sequence_control_set_ptr->input_resolution);
    }
    else {
        eb_vp9_set_me_hme_params_from_confi(
            sequence_control_set_ptr,
            context_ptr->me_context_ptr);
    }

    // Set number of quadrant(s)
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_7) {
        context_ptr->me_context_ptr->single_hme_quadrant = EB_FALSE;
    }
    else {
        if (sequence_control_set_ptr->input_resolution >= INPUT_SIZE_4K_RANGE) {
            context_ptr->me_context_ptr->single_hme_quadrant = EB_TRUE;
        }
        else {
            context_ptr->me_context_ptr->single_hme_quadrant = EB_FALSE;
        }
    }

    // Set ME Fractional Search Method
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
        context_ptr->me_context_ptr->fractional_search_method = SSD_SEARCH;
    }
    else {
        context_ptr->me_context_ptr->fractional_search_method = SUB_SAD_SEARCH;
    }

    // Set 64x64 Fractional Search Flag
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_2) {
        context_ptr->me_context_ptr->fractional_search64x64 = EB_TRUE;
    }
    else {
        context_ptr->me_context_ptr->fractional_search64x64 = EB_FALSE;
    }

    // Set fractional search model
    // 0: search all blocks
    // 1: selective based on Full-Search SAD & MV.
    // 2: off
    if (picture_control_set_ptr->use_subpel_flag == 1) {
        if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
            context_ptr->me_context_ptr->fractional_search_model = 0;
        }
        else {
            context_ptr->me_context_ptr->fractional_search_model = 1;
        }
    }
    else {
        context_ptr->me_context_ptr->fractional_search_model = 2;
    }

    return return_error;
}

/******************************************************
* Derive ME Settings for VMAF
  Input   : encoder mode and tune
  Output  : ME Kernel signal(s)
******************************************************/
EbErrorType eb_vp9_signal_derivation_me_kernel_vmaf(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    MotionEstimationContext *context_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    // Set ME/HME search regions
    if (sequence_control_set_ptr->static_config.use_default_me_hme) {
        set_me_hme_params_vmaf(
            context_ptr->me_context_ptr,
            picture_control_set_ptr,
            sequence_control_set_ptr,
            sequence_control_set_ptr->input_resolution);
    }
    else {
        eb_vp9_set_me_hme_params_from_confi(
            sequence_control_set_ptr,
            context_ptr->me_context_ptr);
    }

    // Set number of quadrant(s)
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_7) {
        context_ptr->me_context_ptr->single_hme_quadrant = EB_FALSE;
    }
    else {
        if (sequence_control_set_ptr->input_resolution >= INPUT_SIZE_4K_RANGE) {
            context_ptr->me_context_ptr->single_hme_quadrant = EB_TRUE;
        }
        else {
            context_ptr->me_context_ptr->single_hme_quadrant = EB_FALSE;
        }
    }

    // Set ME Fractional Search Method
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
        context_ptr->me_context_ptr->fractional_search_method = SSD_SEARCH;
    }
    else {
        context_ptr->me_context_ptr->fractional_search_method = SUB_SAD_SEARCH;
    }

    // Set 64x64 Fractional Search Flag
    if (picture_control_set_ptr->enc_mode <= ENC_MODE_2) {
        context_ptr->me_context_ptr->fractional_search64x64 = EB_TRUE;
    }
    else {
        context_ptr->me_context_ptr->fractional_search64x64 = EB_FALSE;
    }

    // Set fractional search model
    // 0: search all blocks
    // 1: selective based on Full-Search SAD & MV.
    // 2: off
    if (picture_control_set_ptr->use_subpel_flag == 1) {
        if (picture_control_set_ptr->enc_mode <= ENC_MODE_4) {
            context_ptr->me_context_ptr->fractional_search_model = 0;
        }
        else {
            context_ptr->me_context_ptr->fractional_search_model = 1;
        }
    }
    else {
        context_ptr->me_context_ptr->fractional_search_model = 2;
    }

    return return_error;
}

/******************************************************
* eb_vp9_get_mv
  Input   : LCU Index
  Output  : List0 MV
******************************************************/
void eb_vp9_get_mv(
    PictureParentControlSet *picture_control_set_ptr,
    uint32_t                 sb_index,
    int32_t                 *x_current_mv,
    int32_t                 *y_current_mv)
{
    MeCuResults * cu_results = &picture_control_set_ptr->me_results[sb_index][0];

    *x_current_mv = cu_results->x_mv_l0;
    *y_current_mv = cu_results->y_mv_l0;
}

/******************************************************
* get_me_dist
 Input   : LCU Index
 Output  : Best ME Distortion
******************************************************/
static void get_me_dist(
    PictureParentControlSet *picture_control_set_ptr,
    uint32_t                 sb_index,
    uint32_t                *distortion) {
    *distortion = (uint32_t)(picture_control_set_ptr->me_results[sb_index][0].distortion_direction[0].distortion);
}

/******************************************************
* Derive Similar Collocated Flag
******************************************************/
void eb_vp9_derive_similar_collocated_flag(
    PictureParentControlSet *picture_control_set_ptr,
    uint32_t                 sb_index)

{
   // Similairty detector for collocated LCU
   picture_control_set_ptr->similar_colocated_sb_array[sb_index] = EB_FALSE;

   // Similairty detector for collocated LCU -- all layers
   picture_control_set_ptr->similar_colocated_sb_array_all_layers[sb_index] = EB_FALSE;

   if (picture_control_set_ptr->slice_type != I_SLICE) {

       uint8_t  ref_mean, cur_mean;
       uint16_t ref_var, cur_var;

       EbPaReferenceObject *ref_obj_l0;

       ref_obj_l0 = (EbPaReferenceObject*)picture_control_set_ptr->ref_pa_pic_ptr_array[REF_LIST_0]->object_ptr;
       ref_mean = ref_obj_l0->y_mean[sb_index];

       ref_var = ref_obj_l0->variance[sb_index];

       cur_mean = picture_control_set_ptr->y_mean[sb_index][PA_RASTER_SCAN_CU_INDEX_64x64];

       cur_var = picture_control_set_ptr->variance[sb_index][PA_RASTER_SCAN_CU_INDEX_64x64];

       ref_var = MAX(ref_var, 1);
       if ((ABS((int64_t)cur_mean - (int64_t)ref_mean) < MEAN_DIFF_THRSHOLD) &&
           ((ABS((int64_t)cur_var * 100 / (int64_t)ref_var - 100) < VAR_DIFF_THRSHOLD) || (ABS((int64_t)cur_var - (int64_t)ref_var) < VAR_DIFF_THRSHOLD))) {

           if (picture_control_set_ptr->is_used_as_reference_flag) {
               picture_control_set_ptr->similar_colocated_sb_array[sb_index] = EB_TRUE;
           }
           picture_control_set_ptr->similar_colocated_sb_array_all_layers[sb_index] = EB_TRUE;
       }
   }

    return;
}

static void stationary_edge_over_update_over_time_sb_part1(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    uint32_t                 sb_index)
{
    int32_t                 x_current_mv = 0;
    int32_t                 y_current_mv = 0;

    SbParams *sb_params  = &sequence_control_set_ptr->sb_params_array[sb_index];
    SbStat   *sb_stat_ptr = &picture_control_set_ptr->sb_stat_array[sb_index];

    if (sb_params->potential_logo_sb && sb_params->is_complete_sb) {

        // Current MV
        if (picture_control_set_ptr->temporal_layer_index > 0)
            eb_vp9_get_mv(picture_control_set_ptr, sb_index, &x_current_mv, &y_current_mv);

        EB_BOOL lowMotion = picture_control_set_ptr->temporal_layer_index == 0 ? EB_TRUE : (ABS(x_current_mv) < 16) && (ABS(y_current_mv) < 16) ? EB_TRUE : EB_FALSE;
        uint16_t *yVariancePtr = picture_control_set_ptr->variance[sb_index];
        uint64_t var0 = yVariancePtr[ME_TIER_ZERO_PU_32x32_0];
        uint64_t var1 = yVariancePtr[ME_TIER_ZERO_PU_32x32_1];
        uint64_t var2 = yVariancePtr[ME_TIER_ZERO_PU_32x32_2];
        uint64_t var3 = yVariancePtr[ME_TIER_ZERO_PU_32x32_3];

        uint64_t averageVar = (var0 + var1 + var2 + var3) >> 2;
        uint64_t varOfVar = (((int32_t)(var0 - averageVar) * (int32_t)(var0 - averageVar)) +
            ((int32_t)(var1 - averageVar) * (int32_t)(var1 - averageVar)) +
            ((int32_t)(var2 - averageVar) * (int32_t)(var2 - averageVar)) +
            ((int32_t)(var3 - averageVar) * (int32_t)(var3 - averageVar))) >> 2;

        if ((varOfVar <= 50000) || !lowMotion) {
            sb_stat_ptr->check1_for_logo_stationary_edge_over_time_flag = 0;
        }
        else {
            sb_stat_ptr->check1_for_logo_stationary_edge_over_time_flag = 1;
        }

        if ((varOfVar <= 1000)) {
            sb_stat_ptr->pm_check1_for_logo_stationary_edge_over_time_flag = 0;
        }
        else {
            sb_stat_ptr->pm_check1_for_logo_stationary_edge_over_time_flag = 1;
        }
    }
    else {
        sb_stat_ptr->check1_for_logo_stationary_edge_over_time_flag = 0;

        sb_stat_ptr->pm_check1_for_logo_stationary_edge_over_time_flag = 0;

    }
}

static void stationary_edge_over_update_over_time_sb_part2(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr,
    uint32_t                sb_index)
{

    uint32_t low_sad_th = (sequence_control_set_ptr->input_resolution < INPUT_SIZE_1080p_RANGE) ? 5 : 2;

    SbParams *sb_params  = &sequence_control_set_ptr->sb_params_array[sb_index];
    SbStat   *sb_stat_ptr = &picture_control_set_ptr->sb_stat_array[sb_index];

    if (sb_params->potential_logo_sb && sb_params->is_complete_sb) {
        uint32_t me_dist = 0;

        EB_BOOL lowSad = EB_FALSE;

        if (picture_control_set_ptr->slice_type == B_SLICE) {
            get_me_dist(picture_control_set_ptr, sb_index, &me_dist);
        }
        lowSad = (picture_control_set_ptr->slice_type != B_SLICE) ?

            EB_FALSE : (me_dist < 64 * 64 * low_sad_th) ? EB_TRUE : EB_FALSE;

        if (lowSad) {
            sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag = 0;
            sb_stat_ptr->low_dist_logo = 1;
        }
        else {
            sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag = 1;

            sb_stat_ptr->low_dist_logo = 0;
        }
    }
    else {
        sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag = 0;

        sb_stat_ptr->low_dist_logo = 0;
    }
    sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag = 1;

}

/************************************************
 * Motion Analysis Kernel
 * The Motion Analysis performs  Motion Estimation
 * This process has access to the current input picture as well as
 * the input pictures, which the current picture references according
 * to the prediction structure pattern.  The Motion Analysis process is multithreaded,
 * so pictures can be processed out of order as long as all inputs are available.
 ************************************************/
void* eb_vp9_motion_estimation_kernel(void *input_ptr)
{
    MotionEstimationContext *context_ptr = (MotionEstimationContext*)input_ptr;

    PictureParentControlSet *picture_control_set_ptr;
    SequenceControlSet      *sequence_control_set_ptr;

    EbObjectWrapper         *input_results_wrapper_ptr;
    PictureDecisionResults  *input_results_ptr;

    EbObjectWrapper         *output_results_wrapper_ptr;
    MotionEstimationResults *output_results_ptr;

    EbPictureBufferDesc     *input_picture_ptr;

    EbPictureBufferDesc     *input_padded_picture_ptr;

    uint32_t                 buffer_index;

    uint32_t                 sb_index;
    uint32_t                 xsb_index;
    uint32_t                 ysb_index;
    uint32_t                 picture_width_in_sb;
    uint32_t                 picture_height_in_sb;
    uint32_t                 sb_origin_x;
    uint32_t                 sb_origin_y;
    uint32_t                 sb_width;
    uint32_t                 sb_height;
    uint32_t                 sb_row;

    EbPaReferenceObject     *pa_reference_object;
    EbPictureBufferDesc     *quarter_decimated_picture_ptr;
    EbPictureBufferDesc     *sixteenth_decimated_picture_ptr;

    // Segments
    uint32_t                 segment_index;
    uint32_t                 xSegmentIndex;
    uint32_t                 ySegmentIndex;
    uint32_t                 x_sb_start_index;
    uint32_t                 x_sb_end_index;
    uint32_t                 y_sb_start_index;
    uint32_t                 y_sb_end_index;

    uint32_t                 intra_sad_interval_index;

    for (;;) {

        // Get Input Full Object
        eb_vp9_get_full_object(
            context_ptr->picture_decision_results_input_fifo_ptr,
            &input_results_wrapper_ptr);

        input_results_ptr = (PictureDecisionResults*)input_results_wrapper_ptr->object_ptr;
        picture_control_set_ptr = (PictureParentControlSet  *)input_results_ptr->picture_control_set_wrapper_ptr->object_ptr;
        sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
        pa_reference_object = (EbPaReferenceObject  *)picture_control_set_ptr->pareference_picture_wrapper_ptr->object_ptr;
        quarter_decimated_picture_ptr = (EbPictureBufferDesc  *)pa_reference_object->quarter_decimated_picture_ptr;
        sixteenth_decimated_picture_ptr = (EbPictureBufferDesc  *)pa_reference_object->sixteenth_decimated_picture_ptr;
        input_padded_picture_ptr = (EbPictureBufferDesc  *)pa_reference_object->input_padded_picture_ptr;
        input_picture_ptr = picture_control_set_ptr->enhanced_picture_ptr;

        // Segments
        segment_index = input_results_ptr->segment_index;
        picture_width_in_sb = (sequence_control_set_ptr->luma_width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE;
        picture_height_in_sb = (sequence_control_set_ptr->luma_height + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE;
        SEGMENT_CONVERT_IDX_TO_XY(segment_index, xSegmentIndex, ySegmentIndex, picture_control_set_ptr->me_segments_column_count);
        x_sb_start_index = SEGMENT_START_IDX(xSegmentIndex, picture_width_in_sb, picture_control_set_ptr->me_segments_column_count);
        x_sb_end_index = SEGMENT_END_IDX(xSegmentIndex, picture_width_in_sb, picture_control_set_ptr->me_segments_column_count);
        y_sb_start_index = SEGMENT_START_IDX(ySegmentIndex, picture_height_in_sb, picture_control_set_ptr->me_segments_row_count);
        y_sb_end_index = SEGMENT_END_IDX(ySegmentIndex, picture_height_in_sb, picture_control_set_ptr->me_segments_row_count);

        // ME Kernel Signal(s) derivation
        if (sequence_control_set_ptr->static_config.tune == TUNE_SQ) {
            eb_vp9_signal_derivation_me_kernel_sq(
                sequence_control_set_ptr,
                picture_control_set_ptr,
                context_ptr);
        }
        else if (sequence_control_set_ptr->static_config.tune == TUNE_VMAF) {
            eb_vp9_signal_derivation_me_kernel_vmaf(
                sequence_control_set_ptr,
                picture_control_set_ptr,
                context_ptr);
        }
        else {
            eb_vp9_signal_derivation_me_kernel_oq(
                sequence_control_set_ptr,
                picture_control_set_ptr,
                context_ptr);
        }

        // Motion Estimation
        if (picture_control_set_ptr->slice_type != I_SLICE) {

            // LCU Loop
            for (ysb_index = y_sb_start_index; ysb_index < y_sb_end_index; ++ysb_index) {
                for (xsb_index = x_sb_start_index; xsb_index < x_sb_end_index; ++xsb_index) {

                    sb_index = (uint16_t)(xsb_index + ysb_index * picture_width_in_sb);

                    SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

                    sb_origin_x = xsb_index * MAX_SB_SIZE;
                    sb_origin_y = ysb_index * MAX_SB_SIZE;

                    sb_width = sb_params->width;
                    sb_height = sb_params->height;

                    // Load the LCU from the input to the intermediate LCU buffer
                    buffer_index = (input_picture_ptr->origin_y + sb_origin_y) * input_picture_ptr->stride_y + input_picture_ptr->origin_x + sb_origin_x;

                    context_ptr->me_context_ptr->hme_search_type = HME_RECTANGULAR;

                    for (sb_row = 0; sb_row < MAX_SB_SIZE; sb_row++) {
                        EB_MEMCPY((&(context_ptr->me_context_ptr->sb_buffer[sb_row * MAX_SB_SIZE])), (&(input_picture_ptr->buffer_y[buffer_index + sb_row * input_picture_ptr->stride_y])), MAX_SB_SIZE * sizeof(uint8_t));

                    }

                    uint8_t * src_ptr = &input_padded_picture_ptr->buffer_y[buffer_index];

                    //_MM_HINT_T0     //_MM_HINT_T1    //_MM_HINT_T2//_MM_HINT_NTA
                    uint32_t i;
                    for (i = 0; i < sb_height; i++)
                    {
                        char const* p = (char const*)(src_ptr + i*input_padded_picture_ptr->stride_y);
                        _mm_prefetch(p, _MM_HINT_T2);
                    }

                    context_ptr->me_context_ptr->sb_src_ptr = &input_padded_picture_ptr->buffer_y[buffer_index];
                    context_ptr->me_context_ptr->sb_src_stride = input_padded_picture_ptr->stride_y;

                    // Load the 1/4 decimated LCU from the 1/4 decimated input to the 1/4 intermediate LCU buffer
                    if (picture_control_set_ptr->enable_hme_level_1_flag) {

                        buffer_index = (quarter_decimated_picture_ptr->origin_y + (sb_origin_y >> 1)) * quarter_decimated_picture_ptr->stride_y + quarter_decimated_picture_ptr->origin_x + (sb_origin_x >> 1);

                        for (sb_row = 0; sb_row < (sb_height >> 1); sb_row++) {
                            EB_MEMCPY((&(context_ptr->me_context_ptr->quarter_sb_buffer[sb_row * context_ptr->me_context_ptr->quarter_sb_buffer_stride])), (&(quarter_decimated_picture_ptr->buffer_y[buffer_index + sb_row * quarter_decimated_picture_ptr->stride_y])), (sb_width >> 1) * sizeof(uint8_t));

                        }
                    }

                    // Load the 1/16 decimated LCU from the 1/16 decimated input to the 1/16 intermediate LCU buffer
                    if (picture_control_set_ptr->enable_hme_level_0_flag) {

                        buffer_index = (sixteenth_decimated_picture_ptr->origin_y + (sb_origin_y >> 2)) * sixteenth_decimated_picture_ptr->stride_y + sixteenth_decimated_picture_ptr->origin_x + (sb_origin_x >> 2);

                        {
                            uint8_t  *frame_ptr = &sixteenth_decimated_picture_ptr->buffer_y[buffer_index];
                            uint8_t  *local_ptr = context_ptr->me_context_ptr->sixteenth_sb_buffer;

                            for (sb_row = 0; sb_row < (sb_height >> 2); sb_row += 2) {
                                EB_MEMCPY(local_ptr, frame_ptr, (sb_width >> 2) * sizeof(uint8_t));
                                local_ptr += 16;
                                frame_ptr += sixteenth_decimated_picture_ptr->stride_y << 1;
                            }
                        }
                    }

                    motion_estimate_sb(
                        picture_control_set_ptr,
                        sb_index,
                        sb_origin_x,
                        sb_origin_y,
                        context_ptr->me_context_ptr,
                        input_picture_ptr);
                }
            }
        }

        // OIS + Similar Collocated Checks + Stationary Edge Over Time Check
        // LCU Loop
        for (ysb_index = y_sb_start_index; ysb_index < y_sb_end_index; ++ysb_index) {
            for (xsb_index = x_sb_start_index; xsb_index < x_sb_end_index; ++xsb_index) {

                sb_origin_x = xsb_index * MAX_SB_SIZE;
                sb_origin_y = ysb_index * MAX_SB_SIZE;
                sb_index = (uint16_t)(xsb_index + ysb_index * picture_width_in_sb);

                // Derive Similar Collocated Flag
                eb_vp9_derive_similar_collocated_flag(
                    picture_control_set_ptr,
                    sb_index);

                //Check conditions for stationary edge over time Part 1
                stationary_edge_over_update_over_time_sb_part1(
                    sequence_control_set_ptr,
                    picture_control_set_ptr,
                    sb_index);

                //Check conditions for stationary edge over time Part 2
                if (!picture_control_set_ptr->end_of_sequence_flag && sequence_control_set_ptr->look_ahead_distance != 0) {
                    stationary_edge_over_update_over_time_sb_part2(
                        sequence_control_set_ptr,
                        picture_control_set_ptr,
                        sb_index);
                }
            }
        }

        // ZZ SADs Computation
        // 1 lookahead frame is needed to get valid (0,0) SAD
        if (sequence_control_set_ptr->look_ahead_distance != 0) {
            // when DG is ON, the ZZ SADs are computed @ the PD process
            {
                // ZZ SAD
                if (picture_control_set_ptr->picture_number > 0) {

#if 0//ADAPTIVE_QP_INDEX_GEN
                    compute_zz_sad(
                        context_ptr,
                        sequence_control_set_ptr,
                        picture_control_set_ptr,
                        input_padded_picture_ptr,
                        x_sb_start_index,
                        x_sb_end_index,
                        y_sb_start_index,
                        y_sb_end_index);
#else
                    compute_zz_sad(
                        context_ptr,
                        sequence_control_set_ptr,
                        picture_control_set_ptr,
                        sixteenth_decimated_picture_ptr,
                        x_sb_start_index,
                        x_sb_end_index,
                        y_sb_start_index,
                        y_sb_end_index);
#endif

                }
            }
        }

        // Calculate the ME Distortion and OIS Historgrams
        eb_vp9_block_on_mutex(picture_control_set_ptr->rc_distortion_histogram_mutex);
        if (sequence_control_set_ptr->static_config.rate_control_mode){
            if (picture_control_set_ptr->slice_type != I_SLICE){
                uint16_t sad_interval_index;
                for (ysb_index = y_sb_start_index; ysb_index < y_sb_end_index; ++ysb_index) {
                    for (xsb_index = x_sb_start_index; xsb_index < x_sb_end_index; ++xsb_index) {

                        sb_origin_x = xsb_index * MAX_SB_SIZE;
                        sb_origin_y = ysb_index * MAX_SB_SIZE;

                        sb_index = (uint16_t)(xsb_index + ysb_index * picture_width_in_sb);

                        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

                        sb_width = sb_params->width;
                        sb_height = sb_params->height;

                        picture_control_set_ptr->inter_sad_interval_index[sb_index] = 0;
                        picture_control_set_ptr->intra_sad_interval_index[sb_index] = 0;

                        if (sb_params->is_complete_sb) {

                            sad_interval_index = (uint16_t)(picture_control_set_ptr->rcme_distortion[sb_index] >> (12 - SAD_PRECISION_INTERVAL));//change 12 to 2*log2(64)

                            sad_interval_index = (uint16_t)(sad_interval_index >> 2);
                            if (sad_interval_index > (NUMBER_OF_SAD_INTERVALS>>1) -1){
                                uint16_t sadIntervalIndexTemp = sad_interval_index - ((NUMBER_OF_SAD_INTERVALS >> 1) - 1);

                                sad_interval_index = ((NUMBER_OF_SAD_INTERVALS >> 1) - 1) + (sadIntervalIndexTemp >> 3);

                            }
                            if (sad_interval_index >= NUMBER_OF_SAD_INTERVALS - 1)
                                sad_interval_index = NUMBER_OF_SAD_INTERVALS - 1;

                            picture_control_set_ptr->inter_sad_interval_index[sb_index] = sad_interval_index;
                            picture_control_set_ptr->me_distortion_histogram[sad_interval_index] ++;

#if VP9_RC

                            intra_sad_interval_index = picture_control_set_ptr->variance[sb_index][PA_RASTER_SCAN_CU_INDEX_64x64] >> 4;
#else
                            uint32_t                       bestOisblock_index = 0;
                            intra_sad_interval_index = (uint32_t)
                                (((picture_control_set_ptr->oisCu32Cu16Results[sb_index]->sortedOisCandidate[1][bestOisblock_index].distortion +
                                picture_control_set_ptr->oisCu32Cu16Results[sb_index]->sortedOisCandidate[2][bestOisblock_index].distortion +
                                picture_control_set_ptr->oisCu32Cu16Results[sb_index]->sortedOisCandidate[3][bestOisblock_index].distortion +
                                picture_control_set_ptr->oisCu32Cu16Results[sb_index]->sortedOisCandidate[4][bestOisblock_index].distortion)) >> (12 - SAD_PRECISION_INTERVAL));//change 12 to 2*log2(64) ;
#endif
                            intra_sad_interval_index = (uint16_t)(intra_sad_interval_index >> 2);
                            if (intra_sad_interval_index > (NUMBER_OF_SAD_INTERVALS >> 1) - 1){
                                uint32_t sadIntervalIndexTemp = intra_sad_interval_index - ((NUMBER_OF_SAD_INTERVALS >> 1) - 1);

                                intra_sad_interval_index = ((NUMBER_OF_SAD_INTERVALS >> 1) - 1) + (sadIntervalIndexTemp >> 3);

                            }
                            if (intra_sad_interval_index >= NUMBER_OF_SAD_INTERVALS - 1)
                                intra_sad_interval_index = NUMBER_OF_SAD_INTERVALS - 1;

                            picture_control_set_ptr->intra_sad_interval_index[sb_index] = intra_sad_interval_index;
                            picture_control_set_ptr->ois_distortion_histogram[intra_sad_interval_index] ++;

                            ++picture_control_set_ptr->full_sb_count;
                        }

                    }
                }
            }
            else{
                for (ysb_index = y_sb_start_index; ysb_index < y_sb_end_index; ++ysb_index) {
                    for (xsb_index = x_sb_start_index; xsb_index < x_sb_end_index; ++xsb_index) {
                        sb_origin_x = xsb_index * MAX_SB_SIZE;
                        sb_origin_y = ysb_index * MAX_SB_SIZE;
                        // to use sb params here
                        sb_width = (sequence_control_set_ptr->luma_width - sb_origin_x) < MAX_SB_SIZE ? sequence_control_set_ptr->luma_width - sb_origin_x : MAX_SB_SIZE;
                        sb_height = (sequence_control_set_ptr->luma_height - sb_origin_y) < MAX_SB_SIZE ? sequence_control_set_ptr->luma_height - sb_origin_y : MAX_SB_SIZE;

                        sb_index = (uint16_t)(xsb_index + ysb_index * picture_width_in_sb);

                        picture_control_set_ptr->inter_sad_interval_index[sb_index] = 0;
                        picture_control_set_ptr->intra_sad_interval_index[sb_index] = 0;

                        if (sb_width == MAX_SB_SIZE && sb_height == MAX_SB_SIZE) {

#if VP9_RC

                            intra_sad_interval_index = picture_control_set_ptr->variance[sb_index][PA_RASTER_SCAN_CU_INDEX_64x64] >> 4;
#else
                            intra_sad_interval_index = (uint32_t)
                                (((picture_control_set_ptr->oisCu32Cu16Results[sb_index]->sortedOisCandidate[1][bestOisblock_index].distortion +
                                picture_control_set_ptr->oisCu32Cu16Results[sb_index]->sortedOisCandidate[2][bestOisblock_index].distortion +
                                picture_control_set_ptr->oisCu32Cu16Results[sb_index]->sortedOisCandidate[3][bestOisblock_index].distortion +
                                picture_control_set_ptr->oisCu32Cu16Results[sb_index]->sortedOisCandidate[4][bestOisblock_index].distortion)) >> (12 - SAD_PRECISION_INTERVAL));//change 12 to 2*log2(64) ;
#endif

                            intra_sad_interval_index = (uint16_t)(intra_sad_interval_index >> 2);
                            if (intra_sad_interval_index > (NUMBER_OF_SAD_INTERVALS >> 1) - 1){
                                uint32_t sadIntervalIndexTemp = intra_sad_interval_index - ((NUMBER_OF_SAD_INTERVALS >> 1) - 1);

                                intra_sad_interval_index = ((NUMBER_OF_SAD_INTERVALS >> 1) - 1) + (sadIntervalIndexTemp >> 3);

                            }
                            if (intra_sad_interval_index >= NUMBER_OF_SAD_INTERVALS - 1)
                                intra_sad_interval_index = NUMBER_OF_SAD_INTERVALS - 1;

                            picture_control_set_ptr->intra_sad_interval_index[sb_index] = intra_sad_interval_index;
                            picture_control_set_ptr->ois_distortion_histogram[intra_sad_interval_index] ++;
                            ++picture_control_set_ptr->full_sb_count;
                        }

                    }
                }
            }
        }
        eb_vp9_release_mutex(picture_control_set_ptr->rc_distortion_histogram_mutex);
        // Get Empty Results Object
        eb_vp9_get_empty_object(
            context_ptr->motion_estimation_results_output_fifo_ptr,
            &output_results_wrapper_ptr);

        output_results_ptr = (MotionEstimationResults*)output_results_wrapper_ptr->object_ptr;
        output_results_ptr->picture_control_set_wrapper_ptr = input_results_ptr->picture_control_set_wrapper_ptr;
        output_results_ptr->segment_index = segment_index;

        // Release the Input Results
        eb_vp9_release_object(input_results_wrapper_ptr);

        // Post the Full Results Object
        eb_vp9_post_full_object(output_results_wrapper_ptr);
    }
    return EB_NULL;
}
