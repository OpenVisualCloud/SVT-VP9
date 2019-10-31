/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbSei.h"

void eb_video_usability_info_copy(
    AppVideoUsabilityInfo *dst_vui_ptr,
    AppVideoUsabilityInfo *src_vui_ptr)
{

    dst_vui_ptr->aspect_ratio_info_present_flag          = src_vui_ptr->aspect_ratio_info_present_flag;
    dst_vui_ptr->aspect_ratio_idc                        = src_vui_ptr->aspect_ratio_idc;
    dst_vui_ptr->sar_width                               = src_vui_ptr->sar_width;
    dst_vui_ptr->sar_height                              = src_vui_ptr->sar_height;

    dst_vui_ptr->overscan_info_present_flag              = src_vui_ptr->overscan_info_present_flag;
    dst_vui_ptr->overscan_approriate_flag                = src_vui_ptr->overscan_approriate_flag;
    dst_vui_ptr->video_signal_type_present_flag          = src_vui_ptr->video_signal_type_present_flag;

    dst_vui_ptr->video_format                            = src_vui_ptr->video_format;
    dst_vui_ptr->video_full_range_flag                   = src_vui_ptr->video_full_range_flag;

    dst_vui_ptr->color_description_present_flag          =  src_vui_ptr->color_description_present_flag;
    dst_vui_ptr->color_primaries                         = src_vui_ptr->color_primaries;
    dst_vui_ptr->transfer_characteristics                = src_vui_ptr->transfer_characteristics;
    dst_vui_ptr->matrix_coeffs                           = src_vui_ptr->matrix_coeffs;

    dst_vui_ptr->chroma_loc_info_present_flag            = src_vui_ptr->chroma_loc_info_present_flag;
    dst_vui_ptr->chroma_sample_loc_type_top_field        = src_vui_ptr->chroma_sample_loc_type_top_field;
    dst_vui_ptr->chroma_sample_loc_type_bottom_field     = src_vui_ptr->chroma_sample_loc_type_bottom_field;

    dst_vui_ptr->neutral_chroma_indication_flag          = src_vui_ptr->neutral_chroma_indication_flag;
    dst_vui_ptr->field_seq_flag                          = src_vui_ptr->field_seq_flag;
    dst_vui_ptr->frame_field_info_present_flag           = src_vui_ptr->frame_field_info_present_flag;

    dst_vui_ptr->default_display_window_flag             = src_vui_ptr->default_display_window_flag;
    dst_vui_ptr->default_display_win_left_offset         = src_vui_ptr->default_display_win_left_offset;
    dst_vui_ptr->default_display_win_right_offset        = src_vui_ptr->default_display_win_right_offset;
    dst_vui_ptr->default_display_win_top_offset          =  src_vui_ptr->default_display_win_top_offset;
    dst_vui_ptr->default_display_win_bottom_offset       = src_vui_ptr->default_display_win_bottom_offset;

    dst_vui_ptr->vui_timing_info_present_flag            = src_vui_ptr->vui_timing_info_present_flag;
    dst_vui_ptr->vui_num_units_in_tick                   = src_vui_ptr->vui_num_units_in_tick;
    dst_vui_ptr->vui_time_scale                          = src_vui_ptr->vui_time_scale;

    dst_vui_ptr->vui_poc_propotional_timing_flag         = src_vui_ptr->vui_poc_propotional_timing_flag;
    dst_vui_ptr->vui_num_ticks_poc_diff_one_minus1       = src_vui_ptr->vui_num_ticks_poc_diff_one_minus1;

    dst_vui_ptr->vui_hrd_parameters_present_flag         = src_vui_ptr->vui_hrd_parameters_present_flag;

    dst_vui_ptr->bitstream_restriction_flag              = src_vui_ptr->bitstream_restriction_flag;

    dst_vui_ptr->motion_vectors_over_pic_boundaries_flag = src_vui_ptr->motion_vectors_over_pic_boundaries_flag;
    dst_vui_ptr->restricted_ref_pic_lists_flag           = src_vui_ptr->restricted_ref_pic_lists_flag;

    dst_vui_ptr->min_spatial_segmentation_idc            = src_vui_ptr->min_spatial_segmentation_idc;
    dst_vui_ptr->max_bytes_per_pic_denom                 = src_vui_ptr->max_bytes_per_pic_denom;
    dst_vui_ptr->max_bits_per_min_cu_denom               = src_vui_ptr->max_bits_per_min_cu_denom;
    dst_vui_ptr->log2_max_mv_length_horizontal           = src_vui_ptr->log2_max_mv_length_horizontal;
    dst_vui_ptr->log2_max_mv_length_vertical             = src_vui_ptr->log2_max_mv_length_vertical;

    EB_MEMCPY(
        dst_vui_ptr->hrd_parameters_ptr,
        src_vui_ptr->hrd_parameters_ptr,
        sizeof(AppHrdParameters));

    return;
}

EbErrorType eb_video_usability_info_ctor(
    AppVideoUsabilityInfo *vui_ptr)
{
    AppHrdParameters* hrd_param_ptr;
    EB_MALLOC(AppHrdParameters*, vui_ptr->hrd_parameters_ptr, sizeof(AppHrdParameters), EB_N_PTR);

    hrd_param_ptr = vui_ptr->hrd_parameters_ptr;

    // Initialize vui variables

    vui_ptr->aspect_ratio_info_present_flag                = EB_TRUE;
    vui_ptr->aspect_ratio_idc                              = 0;
    vui_ptr->sar_width                                     = 0;
    vui_ptr->sar_height                                    = 0;

    vui_ptr->overscan_info_present_flag                    = EB_FALSE;
    vui_ptr->overscan_approriate_flag                      = EB_FALSE;
    vui_ptr->video_signal_type_present_flag                = EB_FALSE;

    vui_ptr->video_format                                  = 0;
    vui_ptr->video_full_range_flag                         = EB_FALSE;

    vui_ptr->color_description_present_flag                = EB_FALSE;
    vui_ptr->color_primaries                               = 0;
    vui_ptr->transfer_characteristics                      = 0;
    vui_ptr->matrix_coeffs                                 = 0;

    vui_ptr->chroma_loc_info_present_flag                  = EB_FALSE;
    vui_ptr->chroma_sample_loc_type_top_field              = 0;
    vui_ptr->chroma_sample_loc_type_bottom_field           = 0;

    vui_ptr->neutral_chroma_indication_flag                = EB_FALSE;
    vui_ptr->field_seq_flag                                = EB_FALSE;
    vui_ptr->frame_field_info_present_flag                 = EB_FALSE;//EB_TRUE;

    vui_ptr->default_display_window_flag                   = EB_TRUE;
    vui_ptr->default_display_win_left_offset               = 0;
    vui_ptr->default_display_win_right_offset              = 0;
    vui_ptr->default_display_win_top_offset                = 0;
    vui_ptr->default_display_win_bottom_offset             = 0;

    vui_ptr->vui_timing_info_present_flag                  = EB_FALSE;//EB_TRUE;
    vui_ptr->vui_num_units_in_tick                         = 0;
    vui_ptr->vui_time_scale                                = 0;

    vui_ptr->vui_poc_propotional_timing_flag               = EB_FALSE;
    vui_ptr->vui_num_ticks_poc_diff_one_minus1             = 0;

    vui_ptr->vui_hrd_parameters_present_flag               = EB_FALSE;//EB_TRUE;

    vui_ptr->bitstream_restriction_flag                    = EB_FALSE;

    vui_ptr->motion_vectors_over_pic_boundaries_flag       = EB_FALSE;
    vui_ptr->restricted_ref_pic_lists_flag                 = EB_FALSE;

    vui_ptr->min_spatial_segmentation_idc                  = 0;
    vui_ptr->max_bytes_per_pic_denom                       = 0;
    vui_ptr->max_bits_per_min_cu_denom                     = 0;
    vui_ptr->log2_max_mv_length_horizontal                 = 0;
    vui_ptr->log2_max_mv_length_vertical                   = 0;

    // Initialize HRD parameters
    hrd_param_ptr->nal_hrd_parameters_present_flag         = EB_FALSE;//EB_TRUE;
    hrd_param_ptr->vcl_hrd_parameters_present_flag         = EB_FALSE;
    hrd_param_ptr->sub_pic_cpb_params_present_flag         = EB_FALSE;//EB_TRUE;

    hrd_param_ptr->tick_divisor_minus2                     = 0;
    hrd_param_ptr->du_cpb_removal_delay_length_minus1      = 0;

    hrd_param_ptr->sub_pic_cpb_params_pic_timing_sei_flag  = EB_FALSE;//EB_TRUE;

    hrd_param_ptr->dpb_output_delay_du_length_minus1       = 0;

    hrd_param_ptr->bit_rate_scale                          = 0;
    hrd_param_ptr->cpb_size_scale                          = 0;
    hrd_param_ptr->du_cpb_size_scale                       = 0;

    hrd_param_ptr->initial_cpb_removal_delay_length_minus1 = 0;
    hrd_param_ptr->au_cpb_removal_delay_length_minus1      = 0;
    hrd_param_ptr->dpb_output_delay_length_minus1          = 0;

    EB_MEMSET(
        hrd_param_ptr->fixed_pic_rate_general_flag,
        EB_FALSE,
        sizeof(EB_BOOL)*MAX_TEMPORAL_LAYERS);

    EB_MEMSET(
        hrd_param_ptr->fixed_pic_rate_within_cvs_flag,
        EB_FALSE,
        sizeof(EB_BOOL)*MAX_TEMPORAL_LAYERS);

    EB_MEMSET(
        hrd_param_ptr->elemental_duration_tc_minus1,
        EB_FALSE,
        sizeof(uint32_t)*MAX_TEMPORAL_LAYERS);

    EB_MEMSET(
        hrd_param_ptr->low_delay_hrd_flag,
        EB_FALSE,
        sizeof(EB_BOOL)*MAX_TEMPORAL_LAYERS);

    EB_MEMSET(
        hrd_param_ptr->cpb_count_minus1,
        0,
        sizeof(uint32_t)*MAX_TEMPORAL_LAYERS);
    //hrd_param_ptr->cpb_count_minus1[0] = 2;

    EB_MEMSET(
        hrd_param_ptr->bit_rate_value_minus1,
        EB_FALSE,
        sizeof(uint32_t)*MAX_TEMPORAL_LAYERS*2*MAX_CPB_COUNT);

    EB_MEMSET(
        hrd_param_ptr->cpb_size_value_minus1,
        EB_FALSE,
        sizeof(uint32_t)*MAX_TEMPORAL_LAYERS*2*MAX_CPB_COUNT);

    EB_MEMSET(
        hrd_param_ptr->bit_rate_du_value_minus1,
        EB_FALSE,
        sizeof(uint32_t)*MAX_TEMPORAL_LAYERS*2*MAX_CPB_COUNT);

    EB_MEMSET(
        hrd_param_ptr->cpb_size_du_value_minus1,
        EB_FALSE,
        sizeof(uint32_t)*MAX_TEMPORAL_LAYERS*2*MAX_CPB_COUNT);

    EB_MEMSET(
        hrd_param_ptr->cbr_flag,
        EB_FALSE,
        sizeof(EB_BOOL)*MAX_TEMPORAL_LAYERS*2*MAX_CPB_COUNT);

    hrd_param_ptr->cpb_dpb_delays_present_flag = (EB_BOOL)((hrd_param_ptr->nal_hrd_parameters_present_flag || hrd_param_ptr->vcl_hrd_parameters_present_flag) && vui_ptr->vui_hrd_parameters_present_flag);

    return EB_ErrorNone;
}

void eb_picture_timeing_sei_ctor(
    AppPictureTimingSei *pic_timing_ptr)
{
    pic_timing_ptr->pic_struct                         = 0;
    pic_timing_ptr->source_scan_type                   = 0;
    pic_timing_ptr->duplicate_flag                     = EB_FALSE;
    pic_timing_ptr->au_cpb_removal_delay_minus1        = 0;
    pic_timing_ptr->pic_dpb_output_delay               = 0;
    pic_timing_ptr->pic_dpb_output_du_delay            = 0;
    pic_timing_ptr->num_decoding_units_minus1          = 0;
    pic_timing_ptr->du_common_cpb_removal_delay_flag   = EB_FALSE;
    pic_timing_ptr->du_common_cpb_removal_delay_minus1 = 0;
    pic_timing_ptr->num_nalus_in_du_minus1             = 0;

    EB_MEMSET(
        pic_timing_ptr->du_cpb_removal_delay_minus1,
        0,
        sizeof(uint32_t) * MAX_DECODING_UNIT_COUNT);

    return;
}

void eb_buffering_period_sei_ctor(
    AppBufferingPeriodSei *buffering_period_ptr)
{
    buffering_period_ptr->bp_seq_parameter_set_id           = 0;
    buffering_period_ptr->rap_cpb_params_present_flag       = EB_FALSE;
    buffering_period_ptr->concatenation_flag                = EB_FALSE;
    buffering_period_ptr->au_cpb_removal_delay_delta_minus1 = 0;
    buffering_period_ptr->cpb_delay_offset                  = 0;
    buffering_period_ptr->dpb_delay_offset                  = 0;

    EB_MEMSET(
        buffering_period_ptr->initial_cpb_removal_delay,
        0,
        sizeof(uint32_t) * 2 * MAX_CPB_COUNT);
    EB_MEMSET(
        buffering_period_ptr->initial_cpb_removal_delay_offset,
        0,
        sizeof(uint32_t) * 2 * MAX_CPB_COUNT);
    EB_MEMSET(
        buffering_period_ptr->initial_alt_cpb_removal_delay,
        0,
        sizeof(uint32_t) * 2 * MAX_CPB_COUNT);
    EB_MEMSET(
        buffering_period_ptr->initial_alt_cpb_removal_delay_offset,
        0,
        sizeof(uint32_t) * 2 * MAX_CPB_COUNT);

    return;
}

void eb_recovery_point_sei_ctor(
    AppRecoveryPoint *recovery_point_sei_ptr)
{
    recovery_point_sei_ptr->recovery_poc_cnt = 0;

    recovery_point_sei_ptr->exact_matching_flag = EB_FALSE;

    recovery_point_sei_ptr->broken_link_flag = EB_FALSE;

    return;
}

/**************************************************
 * get_uvlc_code_length
 **************************************************/
uint32_t get_uvlc_code_length(
    uint32_t code)
{
    uint32_t number_of_bits = 1;
    uint32_t temp_bits = ++code;

    while( 1 != temp_bits )
    {
        temp_bits >>= 1;
        number_of_bits += 2;
    }

    return number_of_bits;
}

/**************************************************
 * get_svlc_code_length
 **************************************************/
uint32_t get_svlc_code_length(
    int32_t code)
{
    uint32_t number_of_bits = 1;
    uint32_t temp_bits;
    uint32_t bits;

    bits = (code <= 0) ? -code<<1 : (code<<1)-1;;

    temp_bits = ++bits;

    while( 1 != temp_bits )
    {
        temp_bits >>= 1;
        number_of_bits += 2;
    }

    return number_of_bits;
}

uint32_t get_picture_timing_sei_length(
    AppPictureTimingSei   *pic_timing_sei_ptr,
    AppVideoUsabilityInfo *vui_ptr)
{
    uint32_t    sei_length = 0;
    uint32_t    decoding_unit_index;

    if(vui_ptr->frame_field_info_present_flag) {
        // pic_struct
        sei_length += 4;

        // source_scan_type
        sei_length += 2;

        // duplicate_flag
        sei_length += 1;
    }

    if(vui_ptr->hrd_parameters_ptr->cpb_dpb_delays_present_flag) {
        // au_cpb_removal_delay_minus1
        sei_length += vui_ptr->hrd_parameters_ptr->du_cpb_removal_delay_length_minus1 + 1;

        // pic_dpb_output_delay
        sei_length += vui_ptr->hrd_parameters_ptr->dpb_output_delay_length_minus1 + 1;

        if(vui_ptr->hrd_parameters_ptr->sub_pic_cpb_params_present_flag) {
            // pic_dpb_output_du_delay
            sei_length += vui_ptr->hrd_parameters_ptr->dpb_output_delay_du_length_minus1 + 1;
        }

        if(vui_ptr->hrd_parameters_ptr->sub_pic_cpb_params_present_flag && vui_ptr->hrd_parameters_ptr->sub_pic_cpb_params_pic_timing_sei_flag) {
            // num_decoding_units_minus1
            sei_length += get_uvlc_code_length(
                             pic_timing_sei_ptr->num_decoding_units_minus1);

            // du_common_cpb_removal_delay_flag
            sei_length += 1;

            if(pic_timing_sei_ptr->du_common_cpb_removal_delay_flag) {
                // du_common_cpb_removal_delay_minus1
                sei_length += vui_ptr->hrd_parameters_ptr->du_cpb_removal_delay_length_minus1 + 1;
            }

            for(decoding_unit_index = 0; decoding_unit_index <= pic_timing_sei_ptr->num_decoding_units_minus1; ++decoding_unit_index) {
                // num_nalus_in_du_minus1
                sei_length += get_uvlc_code_length(
                                 pic_timing_sei_ptr->num_nalus_in_du_minus1);

                if(!pic_timing_sei_ptr->du_common_cpb_removal_delay_flag && decoding_unit_index < pic_timing_sei_ptr->num_decoding_units_minus1) {
                    // du_cpb_removal_delay_minus1
                    sei_length += vui_ptr->hrd_parameters_ptr->du_cpb_removal_delay_length_minus1 + 1;
                }
            }
        }
    }

    sei_length = (sei_length + 7) >> 3;

    return sei_length;
}

uint32_t get_buf_period_sei_length(
    AppBufferingPeriodSei *buffering_period_ptr,
    AppVideoUsabilityInfo *vui_ptr)
{
    uint32_t       sei_length = 0;
    uint32_t       nal_vcl_index;
    uint32_t       cpb_index;

    // bp_seq_parameter_set_id
    sei_length += get_uvlc_code_length(
                     buffering_period_ptr->bp_seq_parameter_set_id);

    if(!vui_ptr->hrd_parameters_ptr->sub_pic_cpb_params_present_flag) {
        // rap_cpb_params_present_flag
        sei_length += 1;
    }

    // concatenation_flag
    sei_length += 1;

    // au_cpb_removal_delay_delta_minus1
    sei_length += vui_ptr->hrd_parameters_ptr->initial_cpb_removal_delay_length_minus1 + 1;

    if(buffering_period_ptr->rap_cpb_params_present_flag) {
        // cpb_delay_offset
        sei_length += vui_ptr->hrd_parameters_ptr->initial_cpb_removal_delay_length_minus1 + 1;

        // dpb_delay_offset
        sei_length += vui_ptr->hrd_parameters_ptr->dpb_output_delay_du_length_minus1 + 1;
    }

    for(nal_vcl_index = 0; nal_vcl_index < 2; ++nal_vcl_index) {
        if((nal_vcl_index == 0 && vui_ptr->hrd_parameters_ptr->nal_hrd_parameters_present_flag) ||
                (nal_vcl_index == 1 && vui_ptr->hrd_parameters_ptr->vcl_hrd_parameters_present_flag)) {

            for(cpb_index = 0; cpb_index < vui_ptr->hrd_parameters_ptr->cpb_count_minus1[0] + 1; ++cpb_index) {
                // initial_cpb_removal_delay
                sei_length += vui_ptr->hrd_parameters_ptr->initial_cpb_removal_delay_length_minus1 + 1;

                // initial_cpb_removal_delay_offset
                sei_length += vui_ptr->hrd_parameters_ptr->initial_cpb_removal_delay_length_minus1 + 1;

                if(vui_ptr->hrd_parameters_ptr->sub_pic_cpb_params_present_flag || buffering_period_ptr->rap_cpb_params_present_flag) {
                    // initial_alt_cpb_removal_delay
                    sei_length += vui_ptr->hrd_parameters_ptr->initial_cpb_removal_delay_length_minus1 + 1;

                    // initial_alt_cpb_removal_delay_offset
                    sei_length += vui_ptr->hrd_parameters_ptr->initial_cpb_removal_delay_length_minus1 + 1;
                }
            }
        }
    }

    sei_length = (sei_length + 7) >> 3;

    return sei_length;
}

uint32_t get_recovery_point_sei_length(
    AppRecoveryPoint *recovery_point_sei_ptr)
{
    uint32_t    sei_length = 0;

    // recovery_poc_cnt
    sei_length += get_svlc_code_length(
                     recovery_point_sei_ptr->recovery_poc_cnt);

    // exact_matching_flag
    sei_length += 1;

    // broken_link_flag
    sei_length += 1;

    sei_length = (sei_length + 7) >> 3;

    return sei_length;
}
