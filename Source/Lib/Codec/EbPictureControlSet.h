/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureControlSet_h
#define EbPictureControlSet_h

#include <time.h>

#include "EbSvtVp9Enc.h"

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureBufferDesc.h"
#include "EbCodingUnit.h"
#include "EbEntropyCodingObject.h"
#include "EbDefinitions.h"
#include "EbPredictionStructure.h"
#include "EbNeighborArrays.h"
#include "EbEncDecSegments.h"
#include "EbRateControlTables.h"

#include "vp9_encoder.h"
#include "vp9_blockd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SEGMENT_ENTROPY_BUFFER_SIZE                         0x989680                               // Entropy Bitstream Buffer Size
#define PACKETIZATION_PROCESS_BUFFER_SIZE                   SEGMENT_ENTROPY_BUFFER_SIZE + 0x001000 // Entropy Bitstream + Header(s) Buffer Size.
#define HISTOGRAM_NUMBER_OF_BINS                            256
#define MAX_NUMBER_OF_REGIONS_IN_WIDTH                      4
#define MAX_NUMBER_OF_REGIONS_IN_HEIGHT                     4

#define MAX_REF_QP_NUM                                      81

#define SEGMENT_MAX_COUNT   64
#define SEGMENT_COMPLETION_MASK_SET(mask,                   index)        MULTI_LINE_MACRO_BEGIN (mask) |= (((uint64_t) 1) << (index)); MULTI_LINE_MACRO_END
#define SEGMENT_COMPLETION_MASK_TEST(mask,                  total_count)  ((mask) == ((((uint64_t) 1) << (total_count)) - 1))
#define SEGMENT_ROW_COMPLETION_TEST(mask,                   row_index, width) ((((mask) >> ((row_index) * (width))) & ((1ull << (width))-1)) == ((1ull << (width))-1))
#define SEGMENT_CONVERT_IDX_TO_XY(index,                    x, y, pic_width_in_sb) \
                                                            MULTI_LINE_MACRO_BEGIN \
                                                            (y) = (index) / (pic_width_in_sb); \
                                                            (x) = (index) - (y) * (pic_width_in_sb); \
                                                            MULTI_LINE_MACRO_END
#define SEGMENT_START_IDX(index,                            pic_size_in_sb, num_of_seg) (((index) * (pic_size_in_sb)) / (num_of_seg))
#define SEGMENT_END_IDX(index, pic_size_in_sb,              num_of_seg)   ((((index)+1) * (pic_size_in_sb)) / (num_of_seg))

#define NEIGHBOR_ARRAY_TOTAL_COUNT                          3

/**************************************
 * Segment-based Control Sets
 **************************************/

typedef struct PartBlockData
{
    uint16_t block_index;
    EB_BOOL  split_flag;

} PartBlockData;

typedef struct MdcSbData
{
    uint16_t      block_count;
    PartBlockData block_data_array[EP_BLOCK_MAX_COUNT];

} MdcSbData;

typedef struct BdpSbData
{
    uint16_t      block_count;
    PartBlockData block_data_array[EP_BLOCK_MAX_COUNT];

} BdpSbData;

/**************************************
 * MD Segment Control
 **************************************/
typedef struct MdSegmentCtrl
{
    uint32_t total_count;
    uint32_t column_count;
    uint32_t row_count;
    EB_BOOL  in_progress;
    uint32_t current_row_idx;

} MdSegmentCtrl;

/**************************************
 * Picture Control Set
 **************************************/
struct CodedTreeblock;
struct SbUnit;

typedef struct PictureControlSet
{
    EbObjectWrapper               *sequence_control_set_wrapper_ptr;
    EbPictureBufferDesc            *recon_picture_ptr;

    EbPictureBufferDesc            *recon_picture_16bit_ptr;

    EntropyCoder                   *entropy_coder_ptr;

    // Packetization (used to      encode SPS, PPS, etc)
    Bitstream                      *bitstream_ptr;

    // Reference Lists
    EbObjectWrapper               *ref_pic_ptr_array[MAX_NUM_OF_REF_PIC_LIST];

    uint8_t                         ref_pic_qp_array[MAX_NUM_OF_REF_PIC_LIST];
    EB_SLICE                        ref_slice_type_array[MAX_NUM_OF_REF_PIC_LIST];

    // GOP
    uint64_t                        picture_number;
    uint8_t                         temporal_layer_index;

    EncDecSegments                 *enc_dec_segment_ctrl;

    // Entropy Process Rows
    int8_t                          entropy_coding_current_available_row;
    EB_BOOL                         entropy_coding_row_array[MAX_SB_ROWS];
    int8_t                          entropy_coding_current_row;
    int8_t                          entropy_coding_row_count;
    EbHandle                        entropy_coding_mutex;
    EB_BOOL                         entropy_coding_in_progress;
    EB_BOOL                         entropy_coding_pic_done;

    // Mode Decision Config
    MdcSbData                      *mdc_sb_data_array;

    // Slice Type
    EB_SLICE                        slice_type;

    // Rate Control
    uint8_t                         picture_qp;

    int                             base_qindex; // Hsan to remove as present under cpi -> cm
    int                             bottom_index;
    int                             top_index;
    // LCU Array
    uint8_t                         sb_max_depth;
    uint16_t                        sb_total_count;
    SbUnit                        **sb_ptr_array;

    // Hsan: to excute - Derived   @ Mode Decision for the final partitioning & mode distribution, and use @ Encode Pass, Loop Filter & Entropy Coding ()derived
    ModeInfo                      **mode_info_array;

    // Mode Decision Neighbor      Arrays
    NeighborArrayUnit              *md_luma_recon_neighbor_array[NEIGHBOR_ARRAY_TOTAL_COUNT];
    NeighborArrayUnit              *md_cb_recon_neighbor_array[NEIGHBOR_ARRAY_TOTAL_COUNT];
    NeighborArrayUnit              *md_cr_recon_neighbor_array[NEIGHBOR_ARRAY_TOTAL_COUNT];
    PARTITION_CONTEXT              *md_above_seg_context[NEIGHBOR_ARRAY_TOTAL_COUNT];
    PARTITION_CONTEXT              *md_left_seg_context[NEIGHBOR_ARRAY_TOTAL_COUNT];
    ENTROPY_CONTEXT                *md_above_context[NEIGHBOR_ARRAY_TOTAL_COUNT];
    ENTROPY_CONTEXT                *md_left_context[NEIGHBOR_ARRAY_TOTAL_COUNT];

    // Encode Pass Neighbor        Arrays
    NeighborArrayUnit              *ep_luma_recon_neighbor_array;
    NeighborArrayUnit              *ep_cb_recon_neighbor_array;
    NeighborArrayUnit              *ep_cr_recon_neighbor_array;
    NeighborArrayUnit              *ep_luma_recon_neighbor_array_16bit;
    NeighborArrayUnit              *ep_cb_recon_neighbor_array_16bit;
    NeighborArrayUnit              *ep_cr_recon_neighbor_array_16bit;
    ENTROPY_CONTEXT                *ep_above_context;
    ENTROPY_CONTEXT                *ep_left_context;
    EB_BOOL                         is_low_delay;

    struct PictureParentControlSet *parent_pcs_ptr;//The parent of this PCS.
    EbObjectWrapper               *picture_parent_control_set_wrapper_ptr;

    EB_FRAME_CARACTERICTICS         scene_characteristic_id;
    EB_ENC_MODE                     enc_mode;

    EB_BOOL                         bdp_present_flag;
    EB_BOOL                         md_present_flag;
#if SEG_SUPPORT
    int                             segment_counts[MAX_SEGMENTS];
#endif
} PictureControlSet;

// To optimize based on the max input size
// To study speed-memory trade-offs
typedef struct SbParameters
{
    uint8_t   horizontal_index;
    uint8_t   vertical_index;
    uint16_t  origin_x;
    uint16_t  origin_y;
    uint8_t   width;
    uint8_t   height;
    uint8_t   is_complete_sb;
    EB_BOOL   pa_raster_scan_block_validity[PA_BLOCK_MAX_COUNT];
    EB_BOOL   ep_scan_block_validity[EP_BLOCK_MAX_COUNT];
    uint16_t  ec_scan_block_valid_block[EP_BLOCK_MAX_COUNT];
    uint8_t   potential_logo_sb;
    uint8_t   is_edge_sb;
} SbParams;

typedef struct CuStat
{
    EB_BOOL  grass_area;
    EB_BOOL  skin_area;

    EB_BOOL  high_chroma;
    EB_BOOL  high_luma;

    uint16_t edge_cu;
    uint16_t similar_edge_count;
    uint16_t pm_similar_edge_count;

} CuStat;

typedef struct SbStat
{
    CuStat  cu_stat_array[PA_BLOCK_MAX_COUNT];
    uint8_t stationary_edge_over_time_flag;

    uint8_t pm_stationary_edge_over_time_flag;
    uint8_t pm_check1_for_logo_stationary_edge_over_time_flag;

    uint8_t check1_for_logo_stationary_edge_over_time_flag;
    uint8_t check2_for_logo_stationary_edge_over_time_flag;
    uint8_t low_dist_logo;

} SbStat;

//CHKN
// Add the concept of PictureParentControlSet             which is a subset of the old picture_control_set.
// It actually holds only high                          level Pciture based control data:(GOP management,when to start a picture, when to release the PCS, ....).
// The regular picture_control_set(Child)               will be dedicated to store LCU based encoding results and information.
// Parent is created before                             the Child, and continue to live more. Child PCS only lives the exact time needed to encode the picture: from ME to EC/ALF.
typedef struct PictureParentControlSet
{
    VP9_COMP                                          *cpi; // Hsan - to remove unessary fields
    EbObjectWrapper                                   *sequence_control_set_wrapper_ptr;
    EbObjectWrapper                                   *input_picture_wrapper_ptr;
    EbObjectWrapper                                   *reference_picture_wrapper_ptr;
    EbObjectWrapper                                   *pareference_picture_wrapper_ptr;

    EB_BOOL                                            idr_flag;
    EB_BOOL                                            scene_change_flag;
    EB_BOOL                                            end_of_sequence_flag;

    uint8_t                                            picture_qp;
    uint64_t                                           picture_number;

    EbPictureBufferDesc                               *enhanced_picture_ptr;

    EB_PICNOISE_CLASS                                  pic_noise_class;

    EbBufferHeaderType                                *eb_input_ptr;

    EB_SLICE                                           slice_type;
    uint8_t                                            pred_struct_index;
    EB_BOOL                                            use_rps_in_sps;
    uint8_t                                            temporal_layer_index;
    uint64_t                                           decode_order;
    EB_BOOL                                            is_used_as_reference_flag;
    uint8_t                                            ref_list0_count;
    uint8_t                                            ref_list1_count;
    PredictionStructure                               *pred_struct_ptr;          // need to check
    struct PictureParentControlSet                    *ref_pa_pcs_array[MAX_NUM_OF_REF_PIC_LIST];
    EbObjectWrapper                                   *p_pcs_wrapper_ptr;
    // Rate Control
    uint64_t                                           pred_bits_ref_qp[MAX_REF_QP_NUM];
    uint64_t                                           target_bits_best_pred_qp;
    uint64_t                                           target_bits_rc;
    uint8_t                                            best_pred_qp;
    uint64_t                                           total_num_bits;
    uint8_t                                            first_frame_in_temporal_layer;
    uint8_t                                            first_non_intra_frame_in_temporal_layer;
    uint64_t                                           frames_in_interval[EB_MAX_TEMPORAL_LAYERS];
    uint64_t                                           bits_per_sw_per_layer[EB_MAX_TEMPORAL_LAYERS];
    uint64_t                                           sb_total_bits_per_gop;
    EB_BOOL                                            tables_updated;
    EB_BOOL                                            percentage_updated;
    uint32_t                                           target_bit_rate;
    uint32_t                                           frame_rate;
    uint16_t                                           sb_total_count;
    EB_BOOL                                            end_of_sequence_region;
    EB_BOOL                                            scene_change_in_gop;
    // used for Look ahead
    uint8_t                                            frames_in_sw;
    int8_t                                             historgram_life_count;

    EB_BOOL                                            qp_on_the_fly;

    uint8_t                                            calculated_qp;
    uint8_t                                            intra_selected_org_qp;
    uint64_t                                           sad_me;

    uint64_t                                           quantized_coeff_num_bits;

    uint64_t                                           last_idr_picture;

    uint64_t                                           start_time_seconds;
    uint64_t                                           start_timeu_seconds;
    uint32_t                                           luma_sse;
    uint32_t                                           cr_sse;
    uint32_t                                           cb_sse;

    // PA
    uint32_t                                           pre_assignment_buffer_count;

    EbObjectWrapper                                   *ref_pa_pic_ptr_array[MAX_NUM_OF_REF_PIC_LIST];
    uint64_t                                           ref_pic_poc_array[MAX_NUM_OF_REF_PIC_LIST];
    uint16_t                                         **variance;

    uint8_t                                          **y_mean;
    uint8_t                                          **cb_mean;
    uint8_t                                          **cr_mean;

    uint16_t                                           pic_avg_variance;

    // Histograms
    uint32_t                                       ****picture_histogram;

    uint64_t                                           average_intensity_per_region[MAX_NUMBER_OF_REGIONS_IN_WIDTH][MAX_NUMBER_OF_REGIONS_IN_HEIGHT][3];

    // Segments
    uint16_t                                           me_segments_total_count;
    uint8_t                                            me_segments_column_count;
    uint8_t                                            me_segments_row_count;
    uint64_t                                           me_segments_completion_mask;

    // Motion Estimation Results
    uint8_t                                            max_number_of_pus_per_sb;
    uint8_t                                            max_number_of_me_candidates_per_pu;

    MeCuResults                                      **me_results;
    uint32_t                                          *rcme_distortion;

    // Motion Estimation Distortion                     and OIS Historgram
    uint16_t                                           *me_distortion_histogram;
    uint16_t                                           *ois_distortion_histogram;

    uint32_t                                           *intra_sad_interval_index;
    uint32_t                                           *inter_sad_interval_index;

    EbHandle                                            rc_distortion_histogram_mutex;

    uint16_t                                            full_sb_count;

    // Dynamic GOP
    EB_PRED                                             pred_structure;
    uint8_t                                             hierarchical_levels;
    EB_BOOL                                             init_pred_struct_position_flag;
    int8_t                                              hierarchical_layers_diff;

    // Interlaced Video
    EB_PIC_STRUCT                                       pict_struct;

    // Average intensity
    uint8_t                                             average_intensity[3];

    EbObjectWrapper                                    *output_stream_wrapper_ptr;

    // Non moving index array
    uint8_t                                            *non_moving_index_array;
    int                                                 kf_zeromotion_pct;

    EB_BOOL                                             is_pan;
    EB_BOOL                                             is_tilt;

    EB_BOOL                                            *similar_colocated_sb_array;
    EB_BOOL                                            *similar_colocated_sb_array_all_layers;
    uint8_t                                            *sb_flat_noise_array;
    uint64_t                                           *sb_variance_of_variance_over_time;
    EB_BOOL                                            *is_sb_homogeneous_over_time;
    // 5L or 6L prediction error                        compared to 4L prediction structure
    // 5L: computed for base                            layer frames (16 -  8 on top of 16 - 0)
    // 6L: computed for base                            layer frames (32 - 24 on top of 32 - 0 & 16 - 8 on top of 16 - 0)
    uint8_t                                             pic_homogenous_over_time_sb_percentage;

    // To further optimize
    EdgeSbResults                                      *edge_results_ptr;                                // used by EncDecProcess()

    uint8_t                                            *sharp_edge_sb_flag;
#if 0 // Hsan:  PA detector to evaluate                     after adding OIS
    uint8_t                                             *failing_motion_sb_flag;                            // used by EncDecProcess() and ModeDecisionConfigurationProcess
    EB_BOOL                                             *uncovered_area_sb_flag;                            // used by EncDecProcess()
#endif
    EB_BOOL                                            *sb_homogeneous_area_array;                        // used by EncDecProcess()
    EB_BOOL                                             logo_pic_flag;                                    // used by EncDecProcess()
    uint64_t                                          **var_of_var_32x32_based_sb_array;                    // used by ModeDecisionConfigurationProcess()- the variance of 8x8 block variances for each 32x32 block
    EB_BOOL                                            *sb_cmplx_contrast_array;                        // used by EncDecProcess()

    uint16_t                                            non_moving_average_score;                        // used by ModeDecisionConfigurationProcess()
#if BEA
    int16_t                                             non_moving_index_min_distance;
    int16_t                                             non_moving_index_max_distance;
#endif
    EB_BOOL                                            *sb_isolated_non_homogeneous_area_array;            // used by ModeDecisionConfigurationProcess()
    uint8_t                                             grass_percentage_in_picture;
    uint8_t                                             percentage_of_edge_in_light_background;
    EB_BOOL                                             dark_background_light_foreground;
    EbObjectWrapper                                    *previous_picture_control_set_wrapper_ptr;
    SbStat                                             *sb_stat_array;
    uint8_t                                             very_low_var_pic_flag;
    EB_BOOL                                             high_dark_area_density_flag;                        // computed @ PictureAnalysisProcess() and used @ SourceBasedOperationsProcess()
    EB_BOOL                                             high_dark_low_light_area_density_flag;                // computed @ PictureAnalysisProcess() and used @ SourceBasedOperationsProcess()
    uint8_t                                             black_area_percentage;

    uint32_t                                            min_me_distortion;
    uint32_t                                            max_me_distortion;

    EB_SB_DEPTH_MODE                                   *sb_depth_mode_array;
    EB_SB_COMPLEXITY_STATUS                            *complex_sb_array;
    EB_BOOL                                             use_src_ref;
    EB_BOOL                                             qp_scaling_mode;
    RpsNode                                             ref_signal;
    uint8_t                                             show_existing_frame_index_array[4];

    // Encoder Mode
    EB_ENC_MODE                                         enc_mode;

    // Multi-modes signal(s)
    EbPictureDepthMode                                  pic_depth_mode;
    EB_BOOL                                             use_subpel_flag;
    EbCu8x8Mode                                         cu8x8_mode;
    EbCu16x16Mode                                       cu16x16_mode;
    EB_NOISE_DETECT_MODE                                noise_detection_method;
    uint8_t                                             noise_detection_th;
    EB_BOOL                                             enable_denoise_src_flag;
    EB_BOOL                                             enable_hme_flag;
    EB_BOOL                                             enable_hme_level_0_flag;
    EB_BOOL                                             enable_hme_level_1_flag;
    EB_BOOL                                             enable_hme_level_2_flag;

} PictureParentControlSet;

typedef struct PictureControlSetInitData
{
    uint16_t    picture_width;
    uint16_t    picture_height;
    uint16_t    left_padding;
    uint16_t    right_padding;
    uint16_t    top_padding;
    uint16_t    bot_padding;
    EbBitDepth  bit_depth;
    uint32_t    eb_vp9_max_depth;
    EB_BOOL     is16bit;
    uint16_t    enc_dec_segment_col;
    uint16_t    enc_dec_segment_row;

    EB_ENC_MODE enc_mode;

    uint8_t     speed_control;

    uint8_t     tune;

} PictureControlSetInitData;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType eb_vp9_picture_control_set_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr);

extern EbErrorType eb_vp9_picture_parent_control_set_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr);

#ifdef __cplusplus
}
#endif
#endif // EbPictureControlSet_h
