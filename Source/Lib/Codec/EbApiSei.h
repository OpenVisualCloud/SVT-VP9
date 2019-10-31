/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBAPISEI_h
#define EBAPISEI_h

#include "EbDefinitions.h"

#define  MAX_TEMPORAL_LAYERS                        6
#define  MAX_DECODING_UNIT_COUNT                    64   // picture timing SEI

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// These are temporary definitions, should be changed in the future according to user's requirements
#define MAX_CPB_COUNT    4

// User defined structures for passing data from application to the library should be added here
typedef struct UnregistedUserData
{
    uint8_t   uuid_iso_iec11578[16];
    uint8_t  *user_data;
    uint32_t  user_data_size;
} UnregistedUserData;

typedef struct RegistedUserData
{
    uint8_t   *user_data;    // First byte is itu_t_t35_country_code.
                             // If itu_t_t35_country_code  ==  0xFF, second byte is itu_t_t35_country_code_extension_byte.
                             // the rest are the payloadByte
    uint32_t   user_data_size;

} RegistedUserData;

// SEI structures
typedef struct AppHrdParameters
{
    EB_BOOL     nal_hrd_parameters_present_flag;
    EB_BOOL     vcl_hrd_parameters_present_flag;
    EB_BOOL     sub_pic_cpb_params_present_flag;

    uint32_t    tick_divisor_minus2;
    uint32_t    du_cpb_removal_delay_length_minus1;

    EB_BOOL     sub_pic_cpb_params_pic_timing_sei_flag;

    uint32_t    dpb_output_delay_du_length_minus1;

    uint32_t    bit_rate_scale;
    uint32_t    cpb_size_scale;
    uint32_t    du_cpb_size_scale;

    uint32_t    initial_cpb_removal_delay_length_minus1;
    uint32_t    au_cpb_removal_delay_length_minus1;
    uint32_t    dpb_output_delay_length_minus1;

    EB_BOOL     fixed_pic_rate_general_flag[MAX_TEMPORAL_LAYERS];
    EB_BOOL     fixed_pic_rate_within_cvs_flag[MAX_TEMPORAL_LAYERS];

    uint32_t    elemental_duration_tc_minus1[MAX_TEMPORAL_LAYERS];

    EB_BOOL     low_delay_hrd_flag[MAX_TEMPORAL_LAYERS];

    uint32_t    cpb_count_minus1[MAX_TEMPORAL_LAYERS];

    uint32_t    bit_rate_value_minus1[MAX_TEMPORAL_LAYERS][2][MAX_CPB_COUNT];
    uint32_t    cpb_size_value_minus1[MAX_TEMPORAL_LAYERS][2][MAX_CPB_COUNT];
    uint32_t    bit_rate_du_value_minus1[MAX_TEMPORAL_LAYERS][2][MAX_CPB_COUNT];
    uint32_t    cpb_size_du_value_minus1[MAX_TEMPORAL_LAYERS][2][MAX_CPB_COUNT];

    EB_BOOL     cbr_flag[MAX_TEMPORAL_LAYERS][2][MAX_CPB_COUNT];

    EB_BOOL     cpb_dpb_delays_present_flag;

} AppHrdParameters;

typedef struct AppVideoUsabilityInfo
{
    EB_BOOL           aspect_ratio_info_present_flag;
    uint32_t          aspect_ratio_idc;
    uint32_t          sar_width;
    uint32_t          sar_height;

    EB_BOOL           overscan_info_present_flag;
    EB_BOOL           overscan_approriate_flag;
    EB_BOOL           video_signal_type_present_flag;

    uint32_t          video_format;
    EB_BOOL           video_full_range_flag;
    EB_BOOL           color_description_present_flag;

    uint32_t          color_primaries;
    uint32_t          transfer_characteristics;
    uint32_t          matrix_coeffs;

    EB_BOOL           chroma_loc_info_present_flag;
    uint32_t          chroma_sample_loc_type_top_field;
    uint32_t          chroma_sample_loc_type_bottom_field;

    EB_BOOL           neutral_chroma_indication_flag;
    EB_BOOL           field_seq_flag;
    EB_BOOL           frame_field_info_present_flag;

    EB_BOOL           default_display_window_flag;
    uint32_t          default_display_win_left_offset;
    uint32_t          default_display_win_right_offset;
    uint32_t          default_display_win_top_offset;
    uint32_t          default_display_win_bottom_offset;

    EB_BOOL           vui_timing_info_present_flag;
    uint32_t          vui_num_units_in_tick;
    uint32_t          vui_time_scale;

    EB_BOOL           vui_poc_propotional_timing_flag;
    uint32_t          vui_num_ticks_poc_diff_one_minus1;

    EB_BOOL           vui_hrd_parameters_present_flag;

    EB_BOOL           bitstream_restriction_flag;

    EB_BOOL           motion_vectors_over_pic_boundaries_flag;
    EB_BOOL           restricted_ref_pic_lists_flag;

    uint32_t          min_spatial_segmentation_idc;
    uint32_t          max_bytes_per_pic_denom;
    uint32_t          max_bits_per_min_cu_denom;
    uint32_t          log2_max_mv_length_horizontal;
    uint32_t          log2_max_mv_length_vertical;

    AppHrdParameters *hrd_parameters_ptr;

} AppVideoUsabilityInfo;

typedef struct AppPictureTimingSei
{
    uint32_t  pic_struct;
    uint32_t  source_scan_type;
    EB_BOOL   duplicate_flag;
    uint32_t  au_cpb_removal_delay_minus1;
    uint32_t  pic_dpb_output_delay;
    uint32_t  pic_dpb_output_du_delay;
    uint32_t  num_decoding_units_minus1;
    EB_BOOL   du_common_cpb_removal_delay_flag;
    uint32_t  du_common_cpb_removal_delay_minus1;
    uint32_t  num_nalus_in_du_minus1;
    uint32_t  du_cpb_removal_delay_minus1[MAX_DECODING_UNIT_COUNT];

} AppPictureTimingSei;

typedef struct AppBufferingPeriodSei
{
    uint32_t  bp_seq_parameter_set_id;
    EB_BOOL   rap_cpb_params_present_flag;
    EB_BOOL   concatenation_flag;
    uint32_t  au_cpb_removal_delay_delta_minus1;
    uint32_t  cpb_delay_offset;
    uint32_t  dpb_delay_offset;
    uint32_t  initial_cpb_removal_delay[2][MAX_CPB_COUNT];
    uint32_t  initial_cpb_removal_delay_offset[2][MAX_CPB_COUNT];
    uint32_t  initial_alt_cpb_removal_delay[2][MAX_CPB_COUNT];
    uint32_t  initial_alt_cpb_removal_delay_offset[2][MAX_CPB_COUNT];

} AppBufferingPeriodSei;

typedef struct AppRecoveryPoint
{
    uint32_t  recovery_poc_cnt;
    EB_BOOL   exact_matching_flag;
    EB_BOOL   broken_link_flag;

} AppRecoveryPoint;

//

// Signals that the default prediction structure and controls are to be
//   overwritten and manually controlled. Manual control should be active
//   for an entire encode, from beginning to termination.  Mixing of default
//   prediction structure control and override prediction structure control
//   is not supported.
//
// The ref_list0_count and ref_list1_count variables control the picture/slice type.
//   I_SLICE: ref_list0_count == 0 && ref_list1_count == 0
//   P_SLICE: ref_list0_count  > 0 && ref_list1_count == 0
//   B_SLICE: ref_list0_count  > 0 && ref_list1_count  > 0

typedef struct EbPredStructureCfg
{
    uint64_t  picture_number;                 // Corresponds to the display order of the picture
    uint64_t  decode_orderNumber;             // Corresponds to the decode order of the picture
    uint32_t  temporal_layer_index;           // Corresponds to the temporal layer index of the picture
    uint32_t  nal_unit_type;                  // Pictures NAL Unit Type
    uint32_t  ref_list0_count;                // A count of zero indicates the list is inactive
    uint32_t  ref_list1_count;                // A count of zero indicates the list is inactive
    EB_BOOL   is_referenced;                  // Indicates whether or not the picture is used as
                                              //   future reference.
                                              //   be saved for future reference.  This signalling must
                                              //   be done with respect to decode picture order. Must
                                              //   be conformant with the DPB rules.
} EbPredStructureCfg;

// EbPicturePlane defines the data formatting of a singple plane of picture data.
typedef struct EbPicturePlane
{
    // "start" is the starting position of the first
    //   valid pixel in the picture plane.
    uint8_t *start;

    // "stride" is the number of bytes required to increment
    //   an address by one line without changing the horizontal
    //   positioning.
    uint32_t stride;

} EbPicturePlane;

typedef struct EbEosDataDef
{
    EB_BOOL  data_valid;                    // Indicates if the data attached with the last frame in the bitstream is valid or not.
                                            //   If the last frame is valid, the data will be included in the bistream
                                            //   If the last frame is NOT valid, the frame will be coded as IDR in the encoder, but
                                            //   not included in the bitstream.
} EbEosDataDef;

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // EBAPISEI_h
