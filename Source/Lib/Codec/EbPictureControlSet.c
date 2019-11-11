/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <stdint.h>

#include "EbDefinitions.h"
#include "EbPictureControlSet.h"
#include "EbPictureBufferDesc.h"

#include "vp9_encoder.h"

EbErrorType eb_vp9_picture_control_set_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    PictureControlSet *object_ptr;
    PictureControlSetInitData *init_data_ptr  = (PictureControlSetInitData*) object_init_data_ptr;

    EbPictureBufferDescInitData input_picture_buffer_desc_init_data;
    EbPictureBufferDescInitData coeff_buffer_desc_init_data;

    // LCUs
    const uint16_t picturesb_width    = (uint16_t)((init_data_ptr->picture_width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE);
    const uint16_t picturesb_height   = (uint16_t)((init_data_ptr->picture_height + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE);
    uint16_t sb_index;
    uint16_t sb_origin_x;
    uint16_t sb_origin_y;
    EbErrorType return_error = EB_ErrorNone;

    EB_BOOL is16bit = init_data_ptr->is16bit;

    EB_MALLOC(PictureControlSet*, object_ptr, sizeof(PictureControlSet), EB_N_PTR);

    // Hsan to clean up the constructor
    const int aligned_width = ALIGN_POWER_OF_TWO(init_data_ptr->picture_width, MI_SIZE_LOG2);
    const int aligned_height = ALIGN_POWER_OF_TWO(init_data_ptr->picture_height, MI_SIZE_LOG2);

    int mi_cols = aligned_width >> MI_SIZE_LOG2;
    int mi_rows = aligned_height >> MI_SIZE_LOG2;

    EB_MALLOC(ModeInfo **, object_ptr->mode_info_array, sizeof(ModeInfo*) * mi_cols * mi_rows, EB_N_PTR);
    for (int mi_index = 0; mi_index < mi_cols * mi_rows; mi_index++) {
        EB_MALLOC(ModeInfo  *, object_ptr->mode_info_array[mi_index], sizeof(ModeInfo), EB_N_PTR);
    }

    // Init Picture Init data
    input_picture_buffer_desc_init_data.max_width                       = init_data_ptr->picture_width;
    input_picture_buffer_desc_init_data.max_height                      = init_data_ptr->picture_height;
    input_picture_buffer_desc_init_data.bit_depth                       = init_data_ptr->bit_depth;
    input_picture_buffer_desc_init_data.buffer_enable_mask              = PICTURE_BUFFER_DESC_FULL_MASK;
    input_picture_buffer_desc_init_data.left_padding                    = 0;
    input_picture_buffer_desc_init_data.right_padding                   = 0;
    input_picture_buffer_desc_init_data.top_padding                     = 0;
    input_picture_buffer_desc_init_data.bot_padding                     = 0;
    input_picture_buffer_desc_init_data.split_mode                      = EB_FALSE;

    coeff_buffer_desc_init_data.max_width                               = init_data_ptr->picture_width;
    coeff_buffer_desc_init_data.max_height                              = init_data_ptr->picture_height;
    coeff_buffer_desc_init_data.bit_depth                               = EB_16BIT;
    coeff_buffer_desc_init_data.buffer_enable_mask                      = PICTURE_BUFFER_DESC_FULL_MASK;
    coeff_buffer_desc_init_data.left_padding                            = 0;
    coeff_buffer_desc_init_data.right_padding                           = 0;
    coeff_buffer_desc_init_data.top_padding                             = 0;
    coeff_buffer_desc_init_data.bot_padding                             = 0;
    coeff_buffer_desc_init_data.split_mode                              = EB_FALSE;

   *object_dbl_ptr                                                      = (EbPtr) object_ptr;

    object_ptr->sequence_control_set_wrapper_ptr                        = (EbObjectWrapper*)EB_NULL;

    object_ptr->recon_picture_16bit_ptr                                 =  (EbPictureBufferDesc*)EB_NULL;
    object_ptr->recon_picture_ptr                                       =  (EbPictureBufferDesc*)EB_NULL;

    // Reconstructed Picture Buffer
    if(init_data_ptr->is16bit == EB_TRUE){
        return_error = eb_vp9_recon_picture_buffer_desc_ctor(
        (EbPtr*) &(object_ptr->recon_picture_16bit_ptr),
        (EbPtr) &coeff_buffer_desc_init_data);
    }
    else
    {

        return_error = eb_vp9_recon_picture_buffer_desc_ctor(
        (EbPtr*) &(object_ptr->recon_picture_ptr),
        (EbPtr) &input_picture_buffer_desc_init_data);
    }

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Entropy Coder
    return_error = eb_vp9_entropy_coder_ctor(
        &object_ptr->entropy_coder_ptr,
        SEGMENT_ENTROPY_BUFFER_SIZE);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    // Packetization process Bitstream
    return_error = eb_vp9_bitstream_ctor(
        &object_ptr->bitstream_ptr,
        PACKETIZATION_PROCESS_BUFFER_SIZE);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    // GOP
    object_ptr->picture_number        = 0;
    object_ptr->temporal_layer_index   = 0;

    // LCU Array
    object_ptr->sb_max_depth = (uint8_t) init_data_ptr->eb_vp9_max_depth;
    object_ptr->sb_total_count = picturesb_width * picturesb_height;
    EB_MALLOC(SbUnit**, object_ptr->sb_ptr_array, sizeof(SbUnit*) * object_ptr->sb_total_count, EB_N_PTR);

    sb_origin_x = 0;
    sb_origin_y = 0;

    for(sb_index=0; sb_index < object_ptr->sb_total_count; ++sb_index) {

        return_error = sb_unit_ctor(
            &(object_ptr->sb_ptr_array[sb_index]),
            init_data_ptr->picture_width,
            init_data_ptr->picture_height,
            (uint16_t)(sb_origin_x * MAX_SB_SIZE),
            (uint16_t)(sb_origin_y * MAX_SB_SIZE),
            (uint16_t)sb_index,
            object_ptr);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
        // Increment the Order in coding order (Raster Scan Order)
        sb_origin_y = (sb_origin_x == picturesb_width - 1) ? sb_origin_y + 1: sb_origin_y;
        sb_origin_x = (sb_origin_x == picturesb_width - 1) ? 0 : sb_origin_x + 1;
    }

    // Mode Decision Control config
    EB_MALLOC(MdcSbData*, object_ptr->mdc_sb_data_array, object_ptr->sb_total_count  * sizeof(MdcSbData), EB_N_PTR);
    // Mode Decision Neighbor Arrays
    uint8_t depth;
    for (depth = 0; depth < NEIGHBOR_ARRAY_TOTAL_COUNT; depth++) {

        return_error = eb_vp9_neighbor_array_unit_ctor(
            &object_ptr->md_luma_recon_neighbor_array[depth],
            MAX_PICTURE_WIDTH_SIZE,
            MAX_PICTURE_HEIGHT_SIZE,
            sizeof(uint8_t),
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

        return_error = eb_vp9_neighbor_array_unit_ctor(
            &object_ptr->md_cb_recon_neighbor_array[depth],
            MAX_PICTURE_WIDTH_SIZE >> 1,
            MAX_PICTURE_HEIGHT_SIZE >> 1,
            sizeof(uint8_t),
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

        return_error = eb_vp9_neighbor_array_unit_ctor(
            &object_ptr->md_cr_recon_neighbor_array[depth],
            MAX_PICTURE_WIDTH_SIZE >> 1,
            MAX_PICTURE_HEIGHT_SIZE >> 1,
            sizeof(uint8_t),
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    return_error = eb_vp9_neighbor_array_unit_ctor(
        &object_ptr->ep_luma_recon_neighbor_array,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(uint8_t),
        SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
        SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = eb_vp9_neighbor_array_unit_ctor(
        &object_ptr->ep_cb_recon_neighbor_array,
        MAX_PICTURE_WIDTH_SIZE >> 1,
        MAX_PICTURE_HEIGHT_SIZE >> 1,
        sizeof(uint8_t),
        SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
        SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = eb_vp9_neighbor_array_unit_ctor(
        &object_ptr->ep_cr_recon_neighbor_array,
        MAX_PICTURE_WIDTH_SIZE >> 1,
        MAX_PICTURE_HEIGHT_SIZE >> 1,
        sizeof(uint8_t),
        SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
        SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    if(is16bit){
        return_error = eb_vp9_neighbor_array_unit_ctor(
            &object_ptr->ep_luma_recon_neighbor_array_16bit,
            MAX_PICTURE_WIDTH_SIZE,
            MAX_PICTURE_HEIGHT_SIZE,
            sizeof(uint16_t),
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
        return_error = eb_vp9_neighbor_array_unit_ctor(
            &object_ptr->ep_cb_recon_neighbor_array_16bit,
            MAX_PICTURE_WIDTH_SIZE >> 1,
            MAX_PICTURE_HEIGHT_SIZE >> 1,
             sizeof(uint16_t),
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
        return_error = eb_vp9_neighbor_array_unit_ctor(
            &object_ptr->ep_cr_recon_neighbor_array_16bit,
            MAX_PICTURE_WIDTH_SIZE >> 1,
            MAX_PICTURE_HEIGHT_SIZE >> 1,
            sizeof(uint16_t),
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    else {
        object_ptr->ep_luma_recon_neighbor_array_16bit = 0;
        object_ptr->ep_cb_recon_neighbor_array_16bit = 0;
        object_ptr->ep_cr_recon_neighbor_array_16bit = 0;
    }

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

#if VP9_RD
    for (uint8_t depth = 0; depth < NEIGHBOR_ARRAY_TOTAL_COUNT; depth++) {
        EB_MALLOC(PARTITION_CONTEXT *, object_ptr->md_above_seg_context[depth], sizeof(PARTITION_CONTEXT) * mi_cols_aligned_to_sb(mi_cols), EB_N_PTR);
        EB_MALLOC(PARTITION_CONTEXT *, object_ptr->md_left_seg_context[depth], sizeof(PARTITION_CONTEXT) * mi_cols_aligned_to_sb(mi_rows), EB_N_PTR);
        EB_MALLOC(ENTROPY_CONTEXT *, object_ptr->md_above_context[depth], sizeof(ENTROPY_CONTEXT) * MAX_MB_PLANE * 2 * mi_cols_aligned_to_sb(mi_cols), EB_N_PTR);
        EB_MALLOC(ENTROPY_CONTEXT *, object_ptr->md_left_context[depth], sizeof(ENTROPY_CONTEXT) * MAX_MB_PLANE * 2 * mi_cols_aligned_to_sb(mi_rows), EB_N_PTR);
    }

    EB_MALLOC(ENTROPY_CONTEXT *, object_ptr->ep_above_context, sizeof(ENTROPY_CONTEXT) * MAX_MB_PLANE * 2 * mi_cols_aligned_to_sb(mi_cols), EB_N_PTR);
    EB_MALLOC(ENTROPY_CONTEXT *, object_ptr->ep_left_context, sizeof(ENTROPY_CONTEXT) * MAX_MB_PLANE * 2 * mi_cols_aligned_to_sb(mi_rows), EB_N_PTR);
#endif

    // Segments
    return_error = eb_vp9_enc_dec_segments_ctor(
        &object_ptr->enc_dec_segment_ctrl,
        init_data_ptr->enc_dec_segment_col,
        init_data_ptr->enc_dec_segment_row);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Entropy Rows
    EB_CREATEMUTEX(EbHandle, object_ptr->entropy_coding_mutex, sizeof(EbHandle), EB_MUTEX);

    return EB_ErrorNone;
}

EbErrorType eb_vp9_picture_parent_control_set_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    PictureParentControlSet *object_ptr;
    PictureControlSetInitData *init_data_ptr    = (PictureControlSetInitData*) object_init_data_ptr;

    EbErrorType return_error = EB_ErrorNone;
    const uint16_t picturesb_width    = (uint16_t)((init_data_ptr->picture_width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE);
    const uint16_t picturesb_height   = (uint16_t)((init_data_ptr->picture_height + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE);
    uint16_t sb_index;
    uint32_t region_in_picture_width_index;
    uint32_t region_in_picture_height_index;

    EB_MALLOC(PictureParentControlSet  *, object_ptr, sizeof(PictureParentControlSet  ), EB_N_PTR);

    EB_MALLOC(VP9_COMP *, object_ptr->cpi, sizeof(VP9_COMP), EB_N_PTR);

    object_ptr->cpi->common.mi_cols = init_data_ptr->picture_width  >> MI_SIZE_LOG2;
    object_ptr->cpi->common.mi_rows = init_data_ptr->picture_height >> MI_SIZE_LOG2;

    EB_MALLOC(FRAME_COUNTS *, object_ptr->cpi->td.counts, sizeof(FRAME_COUNTS), EB_N_PTR);

    EB_MALLOC(FRAME_CONTEXT*, object_ptr->cpi->common.fc, sizeof(FRAME_CONTEXT), EB_N_PTR);
    EB_MALLOC(FRAME_CONTEXT*, object_ptr->cpi->common.frame_contexts, sizeof(FRAME_CONTEXT), EB_N_PTR);

    EB_MALLOC(uint16_t   *, object_ptr->cpi->td.mb.plane[0].eobs  , sizeof(uint16_t  ) * (MAX_CU_SIZE << 2) * (MAX_CU_SIZE << 2), EB_N_PTR);
    EB_MALLOC(tran_low_t *, object_ptr->cpi->td.mb.plane[0].qcoeff, sizeof(tran_low_t) *  MAX_CU_SIZE * MAX_CU_SIZE, EB_N_PTR);

    EB_MALLOC(uint16_t   *, object_ptr->cpi->td.mb.plane[1].eobs  , sizeof(uint16_t), EB_N_PTR);
    EB_MALLOC(tran_low_t *, object_ptr->cpi->td.mb.plane[1].qcoeff, sizeof(tran_low_t) * MAX_CU_SIZE * MAX_CU_SIZE, EB_N_PTR);

    EB_MALLOC(uint16_t   *, object_ptr->cpi->td.mb.plane[2].eobs  , sizeof(uint16_t), EB_N_PTR);
    EB_MALLOC(tran_low_t *, object_ptr->cpi->td.mb.plane[2].qcoeff, sizeof(tran_low_t) * MAX_CU_SIZE * MAX_CU_SIZE, EB_N_PTR);

#if VP9_RD
    EB_MALLOC(int*, object_ptr->cpi->nmvcosts[0], sizeof(int)* MV_VALS, EB_N_PTR);
    EB_MALLOC(int*, object_ptr->cpi->nmvcosts[1], sizeof(int)* MV_VALS, EB_N_PTR);
    EB_MALLOC(int*, object_ptr->cpi->nmvcosts_hp[0], sizeof(int)* MV_VALS, EB_N_PTR);
    EB_MALLOC(int*, object_ptr->cpi->nmvcosts_hp[1], sizeof(int)* MV_VALS, EB_N_PTR);
    EB_MALLOC(int*, object_ptr->cpi->nmvsadcosts[0], sizeof(int)* MV_VALS, EB_N_PTR);
    EB_MALLOC(int*, object_ptr->cpi->nmvsadcosts[1], sizeof(int)* MV_VALS, EB_N_PTR);
    EB_MALLOC(int*, object_ptr->cpi->nmvsadcosts_hp[0], sizeof(int)* MV_VALS, EB_N_PTR);
    EB_MALLOC(int*, object_ptr->cpi->nmvsadcosts_hp[1], sizeof(int)* MV_VALS, EB_N_PTR);
#endif
#if SEG_SUPPORT
    EB_MALLOC(uint8_t*, object_ptr->cpi->segmentation_map, sizeof(uint8_t)* object_ptr->cpi->common.mi_cols* object_ptr->cpi->common.mi_rows, EB_N_PTR);
#endif
    EB_MALLOC(PARTITION_CONTEXT *, object_ptr->cpi->common.above_seg_context, sizeof(PARTITION_CONTEXT) * mi_cols_aligned_to_sb(object_ptr->cpi->common.mi_cols), EB_N_PTR);

    EB_MALLOC(ENTROPY_CONTEXT *, object_ptr->cpi->common.above_context, sizeof(ENTROPY_CONTEXT) * MAX_MB_PLANE * 2 * mi_cols_aligned_to_sb(object_ptr->cpi->common.mi_cols), EB_N_PTR);

    object_ptr->cpi->common.lf.lfm_stride = (object_ptr->cpi->common.mi_cols + (MI_BLOCK_SIZE - 1)) >> 3;

    EB_MALLOC(LOOP_FILTER_MASK *, object_ptr->cpi->common.lf.lfm, sizeof(LOOP_FILTER_MASK) * ((object_ptr->cpi->common.mi_rows + (MI_BLOCK_SIZE - 1)) >> 3) * object_ptr->cpi->common.lf.lfm_stride, EB_N_PTR);

    *object_dbl_ptr = (EbPtr)object_ptr;

    object_ptr->sequence_control_set_wrapper_ptr = (EbObjectWrapper *)EB_NULL;
    object_ptr->input_picture_wrapper_ptr        = (EbObjectWrapper *)EB_NULL;
    object_ptr->reference_picture_wrapper_ptr    = (EbObjectWrapper *)EB_NULL;

    // GOP
    object_ptr->pred_struct_index                = 0;
    object_ptr->picture_number                   = 0;
    object_ptr->idr_flag                         = EB_FALSE;
    object_ptr->temporal_layer_index             = 0;

    object_ptr->total_num_bits                   = 0;

    object_ptr->last_idr_picture                 = 0;

    object_ptr->sb_total_count                   = picturesb_width * picturesb_height;

    EB_MALLOC(uint16_t**, object_ptr->variance, sizeof(uint16_t*) * object_ptr->sb_total_count, EB_N_PTR);
    EB_MALLOC(uint8_t**, object_ptr->y_mean, sizeof(uint8_t*) * object_ptr->sb_total_count, EB_N_PTR);
    EB_MALLOC(uint8_t**, object_ptr->cb_mean, sizeof(uint8_t*) * object_ptr->sb_total_count, EB_N_PTR);
    EB_MALLOC(uint8_t**, object_ptr->cr_mean, sizeof(uint8_t*) * object_ptr->sb_total_count, EB_N_PTR);
    for (sb_index = 0; sb_index < object_ptr->sb_total_count; ++sb_index) {
        EB_MALLOC(uint16_t*, object_ptr->variance[sb_index], sizeof(uint16_t) * MAX_ME_PU_COUNT, EB_N_PTR);
        EB_MALLOC(uint8_t*, object_ptr->y_mean[sb_index], sizeof(uint8_t) * MAX_ME_PU_COUNT, EB_N_PTR);
        EB_MALLOC(uint8_t*, object_ptr->cb_mean[sb_index], sizeof(uint8_t) * 21, EB_N_PTR);
        EB_MALLOC(uint8_t*, object_ptr->cr_mean[sb_index], sizeof(uint8_t) * 21, EB_N_PTR);
    }
    // Histograms
    uint32_t video_component;

    EB_MALLOC(uint32_t****, object_ptr->picture_histogram, sizeof(uint32_t***) * MAX_NUMBER_OF_REGIONS_IN_WIDTH, EB_N_PTR);

    for (region_in_picture_width_index = 0; region_in_picture_width_index < MAX_NUMBER_OF_REGIONS_IN_WIDTH; region_in_picture_width_index++){  // loop over horizontal regions
        EB_MALLOC(uint32_t***, object_ptr->picture_histogram[region_in_picture_width_index], sizeof(uint32_t**) * MAX_NUMBER_OF_REGIONS_IN_HEIGHT, EB_N_PTR);
    }

    for (region_in_picture_width_index = 0; region_in_picture_width_index < MAX_NUMBER_OF_REGIONS_IN_WIDTH; region_in_picture_width_index++){  // loop over horizontal regions
        for (region_in_picture_height_index = 0; region_in_picture_height_index < MAX_NUMBER_OF_REGIONS_IN_HEIGHT; region_in_picture_height_index++){ // loop over vertical regions
            EB_MALLOC(uint32_t**, object_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index], sizeof(uint32_t*) * 3, EB_N_PTR);
        }
    }

    for (region_in_picture_width_index = 0; region_in_picture_width_index < MAX_NUMBER_OF_REGIONS_IN_WIDTH; region_in_picture_width_index++){  // loop over horizontal regions
        for (region_in_picture_height_index = 0; region_in_picture_height_index < MAX_NUMBER_OF_REGIONS_IN_HEIGHT; region_in_picture_height_index++){ // loop over vertical regions
            for (video_component = 0; video_component < 3; ++video_component) {
                EB_MALLOC(uint32_t*, object_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][video_component], sizeof(uint32_t) * HISTOGRAM_NUMBER_OF_BINS, EB_N_PTR);
            }
        }
    }
#if 0 // Hsan: will be used when adding ois
    uint32_t maxOisCand;
    uint32_t maxOisCand = MAX(MAX_OIS_0, MAX_OIS_2);

    EB_MALLOC(OisCu32Cu16Results_t**, object_ptr->oisCu32Cu16Results, sizeof(OisCu32Cu16Results_t*) * object_ptr->sb_total_count, EB_N_PTR);

    for (sb_index = 0; sb_index < object_ptr->sb_total_count; ++sb_index){

        EB_MALLOC(OisCu32Cu16Results_t*, object_ptr->oisCu32Cu16Results[sb_index], sizeof(OisCu32Cu16Results_t), EB_N_PTR);

        OisCandidate_t* contigousCand;
        EB_MALLOC(OisCandidate_t*, contigousCand, sizeof(OisCandidate_t) * maxOisCand * 21, EB_N_PTR);

        uint32_t cuIdx;
        for (cuIdx = 0; cuIdx < 21; ++cuIdx){
            object_ptr->oisCu32Cu16Results[sb_index]->sortedOisCandidate[cuIdx] = &contigousCand[cuIdx*maxOisCand];
        }
    }

    EB_MALLOC(OisCu8Results_t**, object_ptr->oisCu8Results, sizeof(OisCu8Results_t*) * object_ptr->sb_total_count, EB_N_PTR);
    for (sb_index = 0; sb_index < object_ptr->sb_total_count; ++sb_index){

        EB_MALLOC(OisCu8Results_t*, object_ptr->oisCu8Results[sb_index], sizeof(OisCu8Results_t), EB_N_PTR);

        OisCandidate_t* contigousCand;
        EB_MALLOC(OisCandidate_t*, contigousCand, sizeof(OisCandidate_t) * maxOisCand * 64, EB_N_PTR);

        uint32_t cuIdx;
        for (cuIdx = 0; cuIdx < 64; ++cuIdx){
            object_ptr->oisCu8Results[sb_index]->sortedOisCandidate[cuIdx] = &contigousCand[cuIdx*maxOisCand];
        }
    }
#endif

    // Motion Estimation Results
    object_ptr->max_number_of_pus_per_sb           =   SQUARE_PU_COUNT;
    object_ptr->max_number_of_me_candidates_per_pu =   3;

    EB_MALLOC(MeCuResults**, object_ptr->me_results, sizeof(MeCuResults*) * object_ptr->sb_total_count, EB_N_PTR);
    for (sb_index = 0; sb_index < object_ptr->sb_total_count; ++sb_index) {
        EB_MALLOC(MeCuResults*, object_ptr->me_results[sb_index], sizeof(MeCuResults) * 85, EB_N_PTR);
    }

    EB_MALLOC(uint32_t*, object_ptr->rcme_distortion, sizeof(uint32_t) * object_ptr->sb_total_count, EB_N_PTR);

    // ME and OIS Distortion Histograms
    EB_MALLOC(uint16_t*, object_ptr->me_distortion_histogram, sizeof(uint16_t) * NUMBER_OF_SAD_INTERVALS, EB_N_PTR);

    EB_MALLOC(uint16_t*, object_ptr->ois_distortion_histogram, sizeof(uint16_t) * NUMBER_OF_INTRA_SAD_INTERVALS, EB_N_PTR);

    EB_MALLOC(uint32_t*, object_ptr->intra_sad_interval_index, sizeof(uint32_t) * object_ptr->sb_total_count, EB_N_PTR);
    EB_MALLOC(uint32_t*, object_ptr->inter_sad_interval_index, sizeof(uint32_t) * object_ptr->sb_total_count, EB_N_PTR);

    // Non moving index array
    EB_MALLOC(uint8_t*, object_ptr->non_moving_index_array, sizeof(uint8_t) * object_ptr->sb_total_count, EB_N_PTR);

    // similar Colocated Lcu array
    EB_MALLOC(EB_BOOL*, object_ptr->similar_colocated_sb_array, sizeof(EB_BOOL) * object_ptr->sb_total_count, EB_N_PTR);

    // similar Colocated Lcu array
    EB_MALLOC(EB_BOOL*, object_ptr->similar_colocated_sb_array_all_layers, sizeof(EB_BOOL) * object_ptr->sb_total_count, EB_N_PTR);

    // LCU noise variance array
    EB_MALLOC(uint8_t*, object_ptr->sb_flat_noise_array, sizeof(uint8_t) * object_ptr->sb_total_count, EB_N_PTR);
    EB_MALLOC(uint64_t*, object_ptr->sb_variance_of_variance_over_time, sizeof(uint64_t) * object_ptr->sb_total_count, EB_N_PTR);
    EB_MALLOC(EB_BOOL*, object_ptr->is_sb_homogeneous_over_time, sizeof(EB_BOOL) * object_ptr->sb_total_count, EB_N_PTR);
    EB_MALLOC(EdgeSbResults*, object_ptr->edge_results_ptr, sizeof(EdgeSbResults) * object_ptr->sb_total_count, EB_N_PTR);
    EB_MALLOC(uint8_t*, object_ptr->sharp_edge_sb_flag, sizeof(EB_BOOL) * object_ptr->sb_total_count, EB_N_PTR);
    EB_MALLOC(EB_BOOL*, object_ptr->sb_homogeneous_area_array, sizeof(EB_BOOL) * object_ptr->sb_total_count, EB_N_PTR);

    EB_MALLOC(uint64_t**, object_ptr->var_of_var_32x32_based_sb_array, sizeof(uint64_t*) * object_ptr->sb_total_count, EB_N_PTR);
    for(sb_index = 0; sb_index < object_ptr->sb_total_count; ++sb_index) {
        EB_MALLOC(uint64_t*, object_ptr->var_of_var_32x32_based_sb_array[sb_index], sizeof(uint64_t) * 4, EB_N_PTR);
    }

    EB_MALLOC(EB_BOOL*, object_ptr->sb_isolated_non_homogeneous_area_array, sizeof(EB_BOOL) * object_ptr->sb_total_count, EB_N_PTR);

    EB_MALLOC(uint8_t*, object_ptr->sb_cmplx_contrast_array, sizeof(uint8_t) * object_ptr->sb_total_count, EB_N_PTR);

    EB_MALLOC(SbStat*, object_ptr->sb_stat_array, sizeof(SbStat) * object_ptr->sb_total_count, EB_N_PTR);

    EB_MALLOC(EB_SB_COMPLEXITY_STATUS*, object_ptr->complex_sb_array, sizeof(EB_SB_COMPLEXITY_STATUS) * object_ptr->sb_total_count, EB_N_PTR);

    EB_CREATEMUTEX(EbHandle, object_ptr->rc_distortion_histogram_mutex, sizeof(EbHandle), EB_MUTEX);

    EB_MALLOC(EB_SB_DEPTH_MODE*, object_ptr->sb_depth_mode_array, sizeof(EB_SB_DEPTH_MODE) * object_ptr->sb_total_count, EB_N_PTR);

    return return_error;
}
