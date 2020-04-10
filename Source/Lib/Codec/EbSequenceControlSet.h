/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSequenceControlSet_h
#define EbSequenceControlSet_h

#include "EbDefinitions.h"
#include "EbThreads.h"
#include "EbSystemResourceManager.h"
#include "EbEncodeContext.h"
#include "EbPredictionStructure.h"
#include "EbSei.h"
#ifdef __cplusplus
extern "C" {
#endif
/************************************
 * Sequence Control Set
 ************************************/
typedef struct SequenceControlSet
{
    EbSvtVp9EncConfiguration   static_config;

    // Encoding Context
    EncodeContext             *encode_context_ptr;
    uint32_t                   level_idc;
    uint32_t                   max_temporal_layers;

    // Picture deminsions
    uint16_t                   max_input_luma_width;
    uint16_t                   max_input_luma_height;
    uint16_t                   max_input_chroma_width;
    uint16_t                   max_input_chroma_height;
    uint16_t                   max_input_pad_right;
    uint16_t                   max_input_pad_bottom;
    uint16_t                   luma_width;
    uint16_t                   luma_height;
    uint32_t                   chroma_width;
    uint32_t                   chroma_height;
    uint32_t                   pad_right;
    uint32_t                   pad_bottom;
    uint16_t                   left_padding;
    uint16_t                   top_padding;
    uint16_t                   right_padding;
    uint16_t                   bot_padding;

    uint32_t                   frame_rate;
    uint32_t                   encoder_bit_depth;

    // Cropping Definitions
    int32_t                    cropping_right_offset;
    int32_t                    cropping_bottom_offset;

    // bit_depth
    EbBitDepth                 input_bit_depth;
    EbBitDepth                 output_bitdepth;

    // Group of Pictures (GOP) Structure
    uint32_t                   max_ref_count;            // Maximum number of reference pictures, however each pred
                                                       //   entry can be less.
    PredictionStructure       *pred_struct_ptr;
    int32_t                    intra_period;      // The frequency of intra pictures
    uint32_t                   look_ahead_distance;

    // Rate Control
    uint32_t                   rate_control_mode;
    uint32_t                   target_bit_rate;
    uint32_t                   available_bandwidth;

    // Quantization
    uint32_t                   qp;
    EB_BOOL                    enable_qp_scaling_flag;

    // Video Usability Info
    AppVideoUsabilityInfo     *video_usability_info_ptr;

    // Picture timing sei
    AppPictureTimingSei        pic_timing_sei;

    // Buffering period
    AppBufferingPeriodSei      buffering_period;

    // Recovery point
    AppRecoveryPoint           recovery_point;

    // Picture Analysis
    uint32_t                   picture_analysis_number_of_regions_per_width;
    uint32_t                   picture_analysis_number_of_regions_per_height;

    // Segments
    uint32_t                   me_segment_column_count_array[MAX_TEMPORAL_LAYERS];
    uint32_t                   me_segment_row_count_array[MAX_TEMPORAL_LAYERS];
    uint32_t                   enc_dec_segment_col_count_array[MAX_TEMPORAL_LAYERS];
    uint32_t                   enc_dec_segment_row_count_array[MAX_TEMPORAL_LAYERS];

    // Buffers
    uint32_t                   picture_control_set_pool_init_count;
    uint32_t                   picture_control_set_pool_init_count_child;
    uint32_t                   pa_reference_picture_buffer_init_count;
    uint32_t                   reference_picture_buffer_init_count;
    uint32_t                   input_output_buffer_fifo_init_count;
    uint32_t                   output_recon_buffer_fifo_init_count;
    uint32_t                   resource_coordination_fifo_init_count;
    uint32_t                   picture_analysis_fifo_init_count;
    uint32_t                   picture_decision_fifo_init_count;
    uint32_t                   motion_estimation_fifo_init_count;
    uint32_t                   initial_rate_control_fifo_init_count;
    uint32_t                   picture_demux_fifo_init_count;
    uint32_t                   rate_control_tasks_fifo_init_count;
    uint32_t                   rate_control_fifo_init_count;
    uint32_t                   mode_decision_configuration_fifo_init_count;
    uint32_t                   enc_dec_fifo_init_count;
    uint32_t                   entropy_coding_fifo_init_count;
    uint32_t                   picture_analysis_process_init_count;
    uint32_t                   motion_estimation_process_init_count;
    uint32_t                   source_based_operations_process_init_count;
    uint32_t                   mode_decision_configuration_process_init_count;
    uint32_t                   enc_dec_process_init_count;
    uint32_t                   entropy_coding_process_init_count;
    uint32_t                   total_process_init_count;

    SbParams                  *sb_params_array;
    uint8_t                    picture_width_in_sb;
    uint8_t                    picture_height_in_sb;
    uint16_t                   sb_total_count;
    EB_INPUT_RESOLUTION        input_resolution;
    EB_SCD_MODE                scd_mode;

    EB_BOOL                    enable_denoise_flag;

    uint8_t                    max_enc_mode;
    uint8_t                    hierarchical_levels;

#if ADP_STATS_PER_LAYER
    uint64_t                    total_count[4];
    uint64_t                    fs_count[4];
    uint64_t                    f_bdp_count[4];
    uint64_t                    l_bdp_count[4];
    uint64_t                    f_mdc_count[4];
    uint64_t                    l_mdc_count[4];
    uint64_t                    avc_count[4];
    uint64_t                    pred_count[4];
    uint64_t                    pred1_nfl_count[4];
#endif

} SequenceControlSet;

typedef struct EbSequenceControlSetInitData {
    EncodeContext *encode_context_ptr;
} EbSequenceControlSetInitData;

typedef struct EbSequenceControlSetInstance
{
    EncodeContext              *encode_context_ptr;
    SequenceControlSet       *sequence_control_set_ptr;
    EbHandle                   config_mutex;

} EbSequenceControlSetInstance;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType eb_vp9_sequence_control_set_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr);

extern EbErrorType eb_vp9_copy_sequence_control_set(
    SequenceControlSet *dst,
    SequenceControlSet *src);

extern EbErrorType eb_vp9_sequence_control_set_instance_ctor(
    EbSequenceControlSetInstance **object_dbl_ptr);

extern EbErrorType sb_params_ctor(
    SequenceControlSet *sequence_control_set_ptr);

extern EbErrorType eb_vp9_sb_params_init(
    SequenceControlSet *sequence_control_set_ptr);

extern EbErrorType eb_vp9_derive_input_resolution(
    SequenceControlSet *sequence_control_set_ptr,
    uint32_t            input_size);

#ifdef __cplusplus
}
#endif
#endif // EbSequenceControlSet_h
