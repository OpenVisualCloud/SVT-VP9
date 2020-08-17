/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbSequenceControlSet.h"

/**************************************************************************************************
    General notes on how Sequence Control Sets (SCS) are used.

    SequenceControlSetInstance
        is the master copy that interacts with the API in real-time.  When a
        change happens, the changeFlag is signaled so that appropriate action can
        be taken.  There is one scsInstance per stream/encode instance.  The scsInstance
        owns the encodeContext

    encodeContext
        has context type variables (i.e. non-config) that keep track of global parameters.

    SequenceControlSets
        general SCSs are controled by a system resource manager.  They are kept completely
        separate from the instances.  In general there is one active SCS at a time.  When the
        changeFlag is signaled, the old active SCS is no longer used for new input pictures.
        A fresh copy of the scsInstance is made to a new SCS, which becomes the active SCS.  The
        old SCS will eventually be released back into the SCS pool when its current pictures are
        finished encoding.

    Motivations
        The whole reason for this structure is due to the nature of the pipeline.  We have to
        take great care not to have pipeline mismanagement.  Once an object enters use in the
        pipeline, it cannot be changed on the fly or you will have pipeline coherency problems.
 ***************************************************************************************************/
EbErrorType eb_vp9_sequence_control_set_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    EbSequenceControlSetInitData *scs_init_data = (EbSequenceControlSetInitData  *) object_init_data_ptr;
    uint32_t segment_index;
    EbErrorType return_error = EB_ErrorNone;
    SequenceControlSet *sequence_control_set_ptr;
    EB_MALLOC(SequenceControlSet *, sequence_control_set_ptr, sizeof(SequenceControlSet ), EB_N_PTR);

    *object_dbl_ptr = (EbPtr) sequence_control_set_ptr;

    sequence_control_set_ptr->static_config.qp                                  = 32;

    // Segments
    for(segment_index=0; segment_index < MAX_TEMPORAL_LAYERS; ++segment_index) {
        sequence_control_set_ptr->me_segment_column_count_array[segment_index] = 1;
        sequence_control_set_ptr->me_segment_row_count_array[segment_index]    = 1;
        sequence_control_set_ptr->enc_dec_segment_col_count_array[segment_index] = 1;
        sequence_control_set_ptr->enc_dec_segment_row_count_array[segment_index]  = 1;
    }

    // Encode Context
    if(scs_init_data != EB_NULL) {
        sequence_control_set_ptr->encode_context_ptr                             = scs_init_data->encode_context_ptr;
    }
    else {
        sequence_control_set_ptr->encode_context_ptr                             = (EncodeContext *)EB_NULL;
    }

    // Profile & ID
    sequence_control_set_ptr->level_idc                                          = 0;
    sequence_control_set_ptr->max_temporal_layers                                = 3;

    // Picture Dimensions
    sequence_control_set_ptr->luma_width                                         = 0;
    sequence_control_set_ptr->luma_height                                        = 0;
    sequence_control_set_ptr->chroma_width                                       = 0;
    sequence_control_set_ptr->chroma_height                                      = 0;
    sequence_control_set_ptr->frame_rate                                         = 0;
    sequence_control_set_ptr->encoder_bit_depth                                  = 8;

    // bit_depth
    sequence_control_set_ptr->input_bit_depth                                    = EB_8BIT;
    sequence_control_set_ptr->output_bitdepth                                    = EB_8BIT;

    // GOP Structure
    sequence_control_set_ptr->max_ref_count                                      = 1;
    sequence_control_set_ptr->intra_period                                       = 0;

    // Rate Control
    sequence_control_set_ptr->rate_control_mode                                  = 0;
    sequence_control_set_ptr->target_bit_rate                                    = 0x1000;
    sequence_control_set_ptr->available_bandwidth                                = 0x1000;

    // Quantization
    sequence_control_set_ptr->qp                                                 = 20;

    // Video Usability Info
    EB_MALLOC(AppVideoUsabilityInfo*, sequence_control_set_ptr->video_usability_info_ptr, sizeof(AppVideoUsabilityInfo), EB_N_PTR);

    // Initialize vui parameters
    return_error = eb_video_usability_info_ctor(
        sequence_control_set_ptr->video_usability_info_ptr);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Initialize picture timing SEI
    eb_picture_timeing_sei_ctor(
       &sequence_control_set_ptr->pic_timing_sei);

    // Initialize picture timing SEI
    eb_buffering_period_sei_ctor(
        &sequence_control_set_ptr->buffering_period);

        // Initialize picture timing SEI
    eb_recovery_point_sei_ctor(
        &sequence_control_set_ptr->recovery_point);

    // Initialize LCU params
    sb_params_ctor(
        sequence_control_set_ptr);

    // Buffers:
    sequence_control_set_ptr->input_output_buffer_fifo_init_count        =    100;
   sequence_control_set_ptr->output_recon_buffer_fifo_init_count        = sequence_control_set_ptr->input_output_buffer_fifo_init_count;

#if ADP_STATS_PER_LAYER
    uint8_t temporal_layer_index;
    for (temporal_layer_index = 0; temporal_layer_index < 4; temporal_layer_index++) {
        sequence_control_set_ptr->total_count[temporal_layer_index] = 0;

        sequence_control_set_ptr->fs_count[temporal_layer_index] = 0;
        sequence_control_set_ptr->f_bdp_count[temporal_layer_index] = 0;
        sequence_control_set_ptr->l_bdp_count[temporal_layer_index] = 0;
        sequence_control_set_ptr->f_mdc_count[temporal_layer_index] = 0;
        sequence_control_set_ptr->l_mdc_count[temporal_layer_index] = 0;
        sequence_control_set_ptr->avc_count[temporal_layer_index] = 0;
        sequence_control_set_ptr->pred_count[temporal_layer_index] = 0;
        sequence_control_set_ptr->pred1_nfl_count[temporal_layer_index] = 0;
    }
#endif

    return EB_ErrorNone;
}

/************************************************
 * Sequence Control Set Copy
 ************************************************/
EbErrorType eb_vp9_copy_sequence_control_set(
    SequenceControlSet *dst,
    SequenceControlSet *src)
{
    uint32_t  write_count = 0;
    uint32_t  segment_index = 0;

    dst->static_config               = src->static_config;                            write_count += sizeof(EbSvtVp9EncConfiguration);
    dst->encode_context_ptr          = src->encode_context_ptr;                       write_count += sizeof(EncodeContext*);
    dst->level_idc                   = src->level_idc;                                write_count += sizeof(uint32_t);
    dst->hierarchical_levels         = src->hierarchical_levels;                      write_count += sizeof(uint32_t);
    dst->look_ahead_distance         = src->look_ahead_distance;                      write_count += sizeof(uint32_t);
    dst->max_temporal_layers         = src->max_temporal_layers;                      write_count += sizeof(uint32_t);
    dst->max_input_luma_width        = src->max_input_luma_width;                     write_count += sizeof(uint32_t);
    dst->max_input_luma_height       = src->max_input_luma_height;                    write_count += sizeof(uint32_t);
    dst->max_input_chroma_height     = src->max_input_chroma_height;                  write_count += sizeof(uint32_t);
    dst->max_input_chroma_width      = src->max_input_chroma_width;                   write_count += sizeof(uint32_t);
    dst->max_input_pad_right         = src->max_input_pad_right;                      write_count += sizeof(uint32_t);
    dst->max_input_pad_bottom        = src->max_input_pad_bottom;                     write_count += sizeof(uint32_t);
    dst->luma_width                  = src->luma_width;                               write_count += sizeof(uint32_t);
    dst->luma_height                 = src->luma_height;                              write_count += sizeof(uint32_t);
    dst->chroma_width                = src->chroma_width;                             write_count += sizeof(uint32_t);
    dst->chroma_height               = src->chroma_height;                            write_count += sizeof(uint32_t);
    dst->pad_right                   = src->pad_right;                                write_count += sizeof(uint32_t);
    dst->pad_bottom                  = src->pad_bottom;                               write_count += sizeof(uint32_t);
    dst->cropping_right_offset       = src->cropping_right_offset;                    write_count += sizeof(int32_t);
    dst->cropping_bottom_offset      = src->cropping_bottom_offset;                   write_count += sizeof(int32_t);
    dst->frame_rate                  = src->frame_rate;                               write_count += sizeof(uint32_t);
    dst->input_bit_depth             = src->input_bit_depth;                          write_count += sizeof(EbBitDepth);
    dst->output_bitdepth             = src->output_bitdepth;                          write_count += sizeof(EbBitDepth);
    dst->pred_struct_ptr             = src->pred_struct_ptr;                          write_count += sizeof(PredictionStructure*);
    dst->intra_period                = src->intra_period;                             write_count += sizeof(int32_t);
    dst->max_ref_count               = src->max_ref_count;                            write_count += sizeof(uint32_t);
    dst->target_bit_rate             = src->target_bit_rate;                          write_count += sizeof(uint32_t);
    dst->available_bandwidth         = src->available_bandwidth;                      write_count += sizeof(uint32_t);
    dst->qp                          = src->qp;                                       write_count += sizeof(uint32_t);
    dst->enable_qp_scaling_flag      = src->enable_qp_scaling_flag;                   write_count += sizeof(EB_BOOL);
    dst->left_padding                = src->left_padding;                             write_count += sizeof(uint16_t);
    dst->right_padding               = src->right_padding;                            write_count += sizeof(uint16_t);
    dst->top_padding                 = src->top_padding;                              write_count += sizeof(uint16_t);
    dst->bot_padding                 = src->bot_padding;                              write_count += sizeof(uint16_t);
    dst->enable_denoise_flag         = src->enable_denoise_flag;                      write_count += sizeof(EB_BOOL);
    dst->max_enc_mode                = src->max_enc_mode;                             write_count += sizeof(uint8_t);

    // Segments
    for (segment_index = 0; segment_index < MAX_TEMPORAL_LAYERS; ++segment_index) {
        dst->me_segment_column_count_array[segment_index]   = src->me_segment_column_count_array[segment_index]; write_count += sizeof(uint32_t);
        dst->me_segment_row_count_array[segment_index]      = src->me_segment_row_count_array[segment_index]; write_count += sizeof(uint32_t);
        dst->enc_dec_segment_col_count_array[segment_index] = src->enc_dec_segment_col_count_array[segment_index]; write_count += sizeof(uint32_t);
        dst->enc_dec_segment_row_count_array[segment_index] = src->enc_dec_segment_row_count_array[segment_index]; write_count += sizeof(uint32_t);
    }

    EB_MEMCPY(
        &dst->buffering_period,
        &src->buffering_period,
        sizeof(AppBufferingPeriodSei));

    write_count += sizeof(AppBufferingPeriodSei);

    EB_MEMCPY(
        &dst->recovery_point,
        &src->recovery_point,
        sizeof(AppRecoveryPoint));

    write_count += sizeof(AppRecoveryPoint);

    EB_MEMCPY(
        &dst->pic_timing_sei,
        &src->pic_timing_sei,
        sizeof(AppPictureTimingSei));

    write_count += sizeof(AppPictureTimingSei);

    eb_video_usability_info_copy(
        dst->video_usability_info_ptr,
        src->video_usability_info_ptr);

    write_count += sizeof(AppVideoUsabilityInfo*);

    return EB_ErrorNone;
}

EbErrorType eb_vp9_sequence_control_set_instance_ctor(
    EbSequenceControlSetInstance **object_dbl_ptr)
{
    EbSequenceControlSetInitData scs_init_data;
    EbErrorType return_error = EB_ErrorNone;
    EB_MALLOC(EbSequenceControlSetInstance*, *object_dbl_ptr, sizeof(EbSequenceControlSetInstance), EB_N_PTR);

    return_error = eb_vp9_encode_context_ctor(
        (void **) &(*object_dbl_ptr)->encode_context_ptr,
        EB_NULL);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    scs_init_data.encode_context_ptr = (*object_dbl_ptr)->encode_context_ptr;

    return_error = eb_vp9_sequence_control_set_ctor(
        (void **) &(*object_dbl_ptr)->sequence_control_set_ptr,
        (void *) &scs_init_data);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    EB_CREATEMUTEX(EbHandle*, (*object_dbl_ptr)->config_mutex, sizeof(EbHandle), EB_MUTEX);

    return EB_ErrorNone;
}

extern EbErrorType sb_params_ctor(
    SequenceControlSet *sequence_control_set_ptr) {

    EbErrorType return_error = EB_ErrorNone;

    EB_MALLOC(SbParams*, sequence_control_set_ptr->sb_params_array, sizeof(SbParams) * ((MAX_PICTURE_WIDTH_SIZE + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE) * ((MAX_PICTURE_HEIGHT_SIZE + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE), EB_N_PTR);
    return return_error;
}

extern EbErrorType eb_vp9_sb_params_init(
    SequenceControlSet *sequence_control_set_ptr) {

    EbErrorType return_error = EB_ErrorNone;
    uint16_t    sb_index;
    uint16_t    raster_scan_block_index;
    uint16_t    md_scan_block_index;

    uint8_t    picturesb_width  = (uint8_t)((sequence_control_set_ptr->luma_width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE);
    uint8_t    picturesb_height = (uint8_t)((sequence_control_set_ptr->luma_height + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE);
    EB_MALLOC(SbParams*, sequence_control_set_ptr->sb_params_array, sizeof(SbParams) * picturesb_width * picturesb_height, EB_N_PTR);

    for (sb_index = 0; sb_index < picturesb_width * picturesb_height; ++sb_index) {
        sequence_control_set_ptr->sb_params_array[sb_index].horizontal_index = (uint8_t)(sb_index % picturesb_width);
        sequence_control_set_ptr->sb_params_array[sb_index].vertical_index   = (uint8_t)(sb_index / picturesb_width);
        sequence_control_set_ptr->sb_params_array[sb_index].origin_x         = sequence_control_set_ptr->sb_params_array[sb_index].horizontal_index * MAX_SB_SIZE;
        sequence_control_set_ptr->sb_params_array[sb_index].origin_y         = sequence_control_set_ptr->sb_params_array[sb_index].vertical_index * MAX_SB_SIZE;

        sequence_control_set_ptr->sb_params_array[sb_index].width = (uint8_t)(((uint32_t)(sequence_control_set_ptr->luma_width - sequence_control_set_ptr->sb_params_array[sb_index].origin_x)  < MAX_SB_SIZE) ?
            (uint8_t)(sequence_control_set_ptr->luma_width - sequence_control_set_ptr->sb_params_array[sb_index].origin_x ):
            (uint8_t)MAX_SB_SIZE);

        sequence_control_set_ptr->sb_params_array[sb_index].height = (uint8_t)(((uint32_t)(sequence_control_set_ptr->luma_height - sequence_control_set_ptr->sb_params_array[sb_index].origin_y) < MAX_SB_SIZE) ?
            (uint8_t)(sequence_control_set_ptr->luma_height - sequence_control_set_ptr->sb_params_array[sb_index].origin_y) :
            (uint8_t)MAX_SB_SIZE);

        sequence_control_set_ptr->sb_params_array[sb_index].is_complete_sb = (uint8_t)(((sequence_control_set_ptr->sb_params_array[sb_index].width == MAX_SB_SIZE) && (sequence_control_set_ptr->sb_params_array[sb_index].height == MAX_SB_SIZE)) ?
            1 :
            0);

        sequence_control_set_ptr->sb_params_array[sb_index].is_edge_sb = (sequence_control_set_ptr->sb_params_array[sb_index].origin_x < MAX_SB_SIZE) ||
            (sequence_control_set_ptr->sb_params_array[sb_index].origin_y < MAX_SB_SIZE) ||
            (sequence_control_set_ptr->sb_params_array[sb_index].origin_x > sequence_control_set_ptr->luma_width - MAX_SB_SIZE) ||
            (sequence_control_set_ptr->sb_params_array[sb_index].origin_y > sequence_control_set_ptr->luma_height - MAX_SB_SIZE) ? 1 : 0;

        uint8_t potential_logo_sb = 0;

        // 4K
        /*__ 14 __         __ 14__*/
        ///////////////////////////
        //         |          |         //
        //         8          8         //
        //___14_ |          |_14___//
        //                         //
        //                         //
        //-----------------------// |
        //                         // 8
        ///////////////////////////    |

        // 1080p/720P
        /*__ 7 __          __ 7__*/
        ///////////////////////////
        //         |          |         //
        //         4          4         //
        //___7__ |          |__7___//
        //                         //
        //                         //
        //-----------------------// |
        //                         // 4
        ///////////////////////////    |

        // 480P
        /*__ 3 __          __ 3__*/
        ///////////////////////////
        //         |          |         //
        //         2          2         //
        //___3__ |          |__3___//
        //                         //
        //                         //
        //-----------------------// |
        //                         // 2
        ///////////////////////////    |
        if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER){
            potential_logo_sb = (sequence_control_set_ptr->sb_params_array[sb_index].origin_x >= (sequence_control_set_ptr->luma_width - (3 * MAX_SB_SIZE))) && (sequence_control_set_ptr->sb_params_array[sb_index].origin_y < 2 * MAX_SB_SIZE) ? 1 : potential_logo_sb;
            potential_logo_sb = (sequence_control_set_ptr->sb_params_array[sb_index].origin_x <  ((3 * MAX_SB_SIZE))) && (sequence_control_set_ptr->sb_params_array[sb_index].origin_y < 2 * MAX_SB_SIZE) ? 1 : potential_logo_sb;
            potential_logo_sb = (sequence_control_set_ptr->sb_params_array[sb_index].origin_y >= (sequence_control_set_ptr->luma_height - (2 * MAX_SB_SIZE))) ? 1 : potential_logo_sb;

        }
        else
        if (sequence_control_set_ptr->input_resolution < INPUT_SIZE_4K_RANGE){
            potential_logo_sb = (sequence_control_set_ptr->sb_params_array[sb_index].origin_x >= (sequence_control_set_ptr->luma_width - (7 * MAX_SB_SIZE))) && (sequence_control_set_ptr->sb_params_array[sb_index].origin_y < 4 * MAX_SB_SIZE) ? 1 : potential_logo_sb;
            potential_logo_sb = (sequence_control_set_ptr->sb_params_array[sb_index].origin_x <  ((7 * MAX_SB_SIZE))) && (sequence_control_set_ptr->sb_params_array[sb_index].origin_y < 4 * MAX_SB_SIZE) ? 1 : potential_logo_sb;
            potential_logo_sb = (sequence_control_set_ptr->sb_params_array[sb_index].origin_y >= (sequence_control_set_ptr->luma_height - (4 * MAX_SB_SIZE))) ? 1 : potential_logo_sb;

        }
        else{

            potential_logo_sb = (sequence_control_set_ptr->sb_params_array[sb_index].origin_x >= (sequence_control_set_ptr->luma_width - (14 * MAX_SB_SIZE))) && (sequence_control_set_ptr->sb_params_array[sb_index].origin_y < 8 * MAX_SB_SIZE) ? 1 : potential_logo_sb;
            potential_logo_sb = (sequence_control_set_ptr->sb_params_array[sb_index].origin_x < ((14 * MAX_SB_SIZE))) && (sequence_control_set_ptr->sb_params_array[sb_index].origin_y < 8 * MAX_SB_SIZE) ? 1 : potential_logo_sb;
            potential_logo_sb = (sequence_control_set_ptr->sb_params_array[sb_index].origin_y >= (sequence_control_set_ptr->luma_height - (8 * MAX_SB_SIZE))) ? 1 : potential_logo_sb;

        }
        sequence_control_set_ptr->sb_params_array[sb_index].potential_logo_sb = potential_logo_sb;

        for (raster_scan_block_index = PA_RASTER_SCAN_CU_INDEX_64x64; raster_scan_block_index <= PA_RASTER_SCAN_CU_INDEX_8x8_63; raster_scan_block_index++) {
            sequence_control_set_ptr->sb_params_array[sb_index].pa_raster_scan_block_validity[raster_scan_block_index] = ((sequence_control_set_ptr->sb_params_array[sb_index].origin_x + pa_raster_scan_cu_x[raster_scan_block_index] + pa_raster_scan_cu_size[raster_scan_block_index] > sequence_control_set_ptr->luma_width ) || (sequence_control_set_ptr->sb_params_array[sb_index].origin_y + pa_raster_scan_cu_y[raster_scan_block_index] + pa_raster_scan_cu_size[raster_scan_block_index] > sequence_control_set_ptr->luma_height)) ?
                EB_FALSE :
                EB_TRUE;
        }

        for (md_scan_block_index = 0; md_scan_block_index < EP_BLOCK_MAX_COUNT; md_scan_block_index++) {
            const EpBlockStats * ep_block_stats_ptr = ep_get_block_stats(md_scan_block_index);
            sequence_control_set_ptr->sb_params_array[sb_index].ep_scan_block_validity[md_scan_block_index] = (((sequence_control_set_ptr->sb_params_array[sb_index].origin_x + ep_block_stats_ptr->origin_x + ep_block_stats_ptr->bwidth) > sequence_control_set_ptr->luma_width) || ((sequence_control_set_ptr->sb_params_array[sb_index].origin_y + ep_block_stats_ptr->origin_y + ep_block_stats_ptr->bheight) > sequence_control_set_ptr->luma_height)) ?
                EB_FALSE :
                EB_TRUE;

           sequence_control_set_ptr->sb_params_array[sb_index].ec_scan_block_valid_block[md_scan_block_index] = (uint16_t)~0;
        }

        // Find the valid block for each block (highest depth block which is valid and covered by this block). 
        // A valid block is a block that is within the boundaries of the frame.
        // Note: The unsigned int loop index logic will go from EP_BLOCK_MAX_COUNT-1 to and including 0.
        for (md_scan_block_index = EP_BLOCK_MAX_COUNT - 1; md_scan_block_index-- != 0; )
        {
            // Initialize the valid block
            sequence_control_set_ptr->sb_params_array[sb_index].ec_scan_block_valid_block[md_scan_block_index] = (uint16_t)~0;

            // If this block is valid, set it as the valid block.
            if (sequence_control_set_ptr->sb_params_array[sb_index].ep_scan_block_validity[md_scan_block_index] == EB_TRUE)
                sequence_control_set_ptr->sb_params_array[sb_index].ec_scan_block_valid_block[md_scan_block_index] = md_scan_block_index;
            else{
                // search the next lowest depth for valid blocks, but don't go lower than 8x8
                const EpBlockStats *ep_block_stats_ptr = ep_get_block_stats(md_scan_block_index);

                // Only search when block size is greater than 8x8
                if (ep_block_stats_ptr->bsize > 3)
                {
                    // Check all of the blocks at the lower depth and find the first one that is valid.
                    for (uint16_t search_valid_index = md_scan_block_index; search_valid_index < (md_scan_block_index + ep_inter_depth_offset); search_valid_index++) {
                        if (search_valid_index < EP_BLOCK_MAX_COUNT && sequence_control_set_ptr->sb_params_array[sb_index].ec_scan_block_valid_block[search_valid_index] != (uint16_t)~0) {
                            sequence_control_set_ptr->sb_params_array[sb_index].ec_scan_block_valid_block[md_scan_block_index] = sequence_control_set_ptr->sb_params_array[sb_index].ec_scan_block_valid_block[search_valid_index];
                            break;
                        }
                    }
                }
            }
        }
    }

    sequence_control_set_ptr->picture_width_in_sb = picturesb_width;
    sequence_control_set_ptr->picture_height_in_sb = picturesb_height;
    sequence_control_set_ptr->sb_total_count = picturesb_width * picturesb_height;

    return return_error;
}

extern EbErrorType eb_vp9_derive_input_resolution(
    SequenceControlSet *sequence_control_set_ptr,
    uint32_t            input_size)
{
    EbErrorType return_error = EB_ErrorNone;

    sequence_control_set_ptr->input_resolution = (input_size < INPUT_SIZE_1080i_TH) ?
        INPUT_SIZE_576p_RANGE_OR_LOWER :
        (input_size < INPUT_SIZE_1080p_TH) ?
            INPUT_SIZE_1080i_RANGE :
            (input_size < INPUT_SIZE_4K_TH) ?
                INPUT_SIZE_1080p_RANGE :
                INPUT_SIZE_4K_RANGE;

    return return_error;
}
