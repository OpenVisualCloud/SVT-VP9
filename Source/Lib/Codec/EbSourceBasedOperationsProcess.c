/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbSourceBasedOperationsProcess.h"
#include "EbInitialRateControlResults.h"
#include "EbPictureDemuxResults.h"
#include "EbMotionEstimationContext.h"
#include "emmintrin.h"
/**************************************
* Macros
**************************************/

#define VARIANCE_PRECISION               16
#define PAN_SB_PERCENTAGE                75
#define LOW_AMPLITUDE_TH                 16

#define Y_MEAN_RANGE_03                  52
#define Y_MEAN_RANGE_02                  70
#define Y_MEAN_RANGE_01                 130
#define CB_MEAN_RANGE_02                115
#define CR_MEAN_RANGE_00                110
#define CR_MEAN_RANGE_02                135

#define DARK_FRM_TH                      45
#define CB_MEAN_RANGE_00                 80

#define SAD_DEVIATION_SB_TH_0            15
#define SAD_DEVIATION_SB_TH_1            20

#define MAX_DELTA_QP_SHAPE_TH             4
#define MIN_DELTA_QP_SHAPE_TH             1

#define MIN_BLACK_AREA_PERCENTAGE        20
#define LOW_MEAN_TH_0                    25

#define MIN_WHITE_AREA_PERCENTAGE         1
#define LOW_MEAN_TH_1                    40
#define HIGH_MEAN_TH                    210

/************************************************
* Initial Rate Control Context Constructor
************************************************/

EbErrorType eb_vp9_source_based_operations_context_ctor(
    SourceBasedOperationsContext **context_dbl_ptr,
    EbFifo                        *initial_rate_control_results_input_fifo_ptr,
    EbFifo                        *picture_demux_results_output_fifo_ptr)
{
    SourceBasedOperationsContext *context_ptr;

    EB_MALLOC(SourceBasedOperationsContext*, context_ptr, sizeof(SourceBasedOperationsContext), EB_N_PTR);
    *context_dbl_ptr = context_ptr;
    context_ptr->initial_rate_control_results_input_fifo_ptr = initial_rate_control_results_input_fifo_ptr;
    context_ptr->picture_demux_results_output_fifo_ptr = picture_demux_results_output_fifo_ptr;

    return EB_ErrorNone;
}

/***************************************************
* Derives BEA statistics and set activity flags
***************************************************/

void eb_vp9_derive_picture_activity_statistics(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr)
{
    uint64_t non_moving_index_sum = 0;
    uint32_t complete_sb_count = 0;
    uint32_t non_moving_sb_count = 0;
    uint32_t sb_index;

#if BEA
    uint64_t non_moving_index_min = ~0u;
    uint64_t non_moving_index_max = 0;
#endif

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

        if (sb_params->is_complete_sb) {
#if BEA
            non_moving_index_min = picture_control_set_ptr->non_moving_index_array[sb_index] < non_moving_index_min ?
                picture_control_set_ptr->non_moving_index_array[sb_index] :
                non_moving_index_min;

            non_moving_index_max = picture_control_set_ptr->non_moving_index_array[sb_index] > non_moving_index_max ?
                picture_control_set_ptr->non_moving_index_array[sb_index] :
                non_moving_index_max;
#endif
            if (picture_control_set_ptr->non_moving_index_array[sb_index] < NON_MOVING_SCORE_1) {
                non_moving_sb_count++;
            }
            non_moving_index_sum += picture_control_set_ptr->non_moving_index_array[sb_index];
            complete_sb_count++;
        }
    }

    if (complete_sb_count > 0)
        picture_control_set_ptr->non_moving_average_score = (uint16_t)(non_moving_index_sum / complete_sb_count);
#if BEA
    picture_control_set_ptr->non_moving_index_min_distance = (uint16_t)(ABS((int32_t)(picture_control_set_ptr->non_moving_average_score) - (int32_t)non_moving_index_min));
    picture_control_set_ptr->non_moving_index_max_distance = (uint16_t)(ABS((int32_t)(picture_control_set_ptr->non_moving_average_score) - (int32_t)non_moving_index_max));
#endif
    if (complete_sb_count > 0)
        picture_control_set_ptr->kf_zeromotion_pct = (non_moving_sb_count * 100) / complete_sb_count;

    return;
}

void grass_skin_sb(
    SourceBasedOperationsContext *context_ptr,
    SequenceControlSet           *sequence_control_set_ptr,
    PictureParentControlSet      *picture_control_set_ptr,
    uint32_t                      sb_index)
{

    uint32_t child_index;

    EB_BOOL sb_grass_flag = EB_FALSE;

    uint32_t grass_sb_inrange;
    uint32_t processed_cus;
    uint32_t  raster_scan_block_index;

    SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
    SbStat *sb_stat_ptr = &(picture_control_set_ptr->sb_stat_array[sb_index]);

    _mm_prefetch((const char*)sb_stat_ptr, _MM_HINT_T0);

    sb_grass_flag = EB_FALSE;
    grass_sb_inrange = 0;
    processed_cus = 0;

    for (raster_scan_block_index = PA_RASTER_SCAN_CU_INDEX_16x16_0; raster_scan_block_index <= PA_RASTER_SCAN_CU_INDEX_16x16_15; raster_scan_block_index++) {
        if (sb_params->pa_raster_scan_block_validity[raster_scan_block_index]) {
            const uint32_t md_scan_block_index = raster_scan_to_md_scan[raster_scan_block_index];
            const uint32_t raster_scan_parent_block_index = RASTER_SCAN_CU_PARENT_INDEX[raster_scan_block_index];
            const uint32_t mdScanParentblock_index = raster_scan_to_md_scan[raster_scan_parent_block_index];
            CuStat *cu_stat_ptr = &(sb_stat_ptr->cu_stat_array[md_scan_block_index]);

            const uint32_t perfect_condition = 7;
            const uint8_t y_mean = context_ptr->y_mean_ptr[raster_scan_block_index];
            const uint8_t cb_mean = context_ptr->cb_mean_ptr[raster_scan_block_index];
            const uint8_t cr_mean = context_ptr->cr_mean_ptr[raster_scan_block_index];
            uint32_t grass_condition = 0;
            uint32_t skin_condition = 0;

            uint32_t high_chroma_condition = 0;
            uint32_t high_luma_condition = 0;

            // GRASS
            grass_condition += (y_mean > Y_MEAN_RANGE_02 && y_mean < Y_MEAN_RANGE_01) ? 1 : 0;
            grass_condition += (cb_mean > CB_MEAN_RANGE_00 && cb_mean < CB_MEAN_RANGE_02) ? 2 : 0;
            grass_condition += (cr_mean > CR_MEAN_RANGE_00 && cr_mean < CR_MEAN_RANGE_02) ? 4 : 0;

            grass_sb_inrange += (grass_condition == perfect_condition) ? 1 : 0;
            processed_cus++;

            sb_grass_flag = grass_condition == perfect_condition ? EB_TRUE : sb_grass_flag;

            cu_stat_ptr->grass_area = (EB_BOOL)(grass_condition == perfect_condition);
            // SKIN
            skin_condition += (y_mean > Y_MEAN_RANGE_03 && y_mean < Y_MEAN_RANGE_01) ? 1 : 0;
            skin_condition += (cb_mean > 100 && cb_mean < 120) ? 2 : 0;
            skin_condition += (cr_mean > 135 && cr_mean < 160) ? 4 : 0;

            cu_stat_ptr->skin_area = (EB_BOOL)(skin_condition == perfect_condition);

            high_chroma_condition = (cr_mean >= 127 || cb_mean > 150) ? 1 : 0;
            high_luma_condition = (cr_mean >= 80 && y_mean > 180) ? 1 : 0;
            cu_stat_ptr->high_luma = (high_luma_condition == 1) ? EB_TRUE : EB_FALSE;
            cu_stat_ptr->high_chroma = (high_chroma_condition == 1) ? EB_TRUE : EB_FALSE;

            for (child_index = md_scan_block_index + 1; child_index < md_scan_block_index + 5; ++child_index){
                sb_stat_ptr->cu_stat_array[child_index].grass_area = cu_stat_ptr->grass_area;
                sb_stat_ptr->cu_stat_array[child_index].skin_area = cu_stat_ptr->skin_area;
                sb_stat_ptr->cu_stat_array[child_index].high_chroma = cu_stat_ptr->high_chroma;
                sb_stat_ptr->cu_stat_array[child_index].high_luma = cu_stat_ptr->high_luma;

            }

            sb_stat_ptr->cu_stat_array[mdScanParentblock_index].grass_area = cu_stat_ptr->grass_area ? EB_TRUE :
                sb_stat_ptr->cu_stat_array[mdScanParentblock_index].grass_area;
            sb_stat_ptr->cu_stat_array[0].grass_area = cu_stat_ptr->grass_area ? EB_TRUE :
                sb_stat_ptr->cu_stat_array[0].grass_area;
            sb_stat_ptr->cu_stat_array[mdScanParentblock_index].skin_area = cu_stat_ptr->skin_area ? EB_TRUE :
                sb_stat_ptr->cu_stat_array[mdScanParentblock_index].skin_area;
            sb_stat_ptr->cu_stat_array[0].skin_area = cu_stat_ptr->skin_area ? EB_TRUE :
                sb_stat_ptr->cu_stat_array[0].skin_area;
            sb_stat_ptr->cu_stat_array[mdScanParentblock_index].high_chroma = cu_stat_ptr->high_chroma ? EB_TRUE :
                sb_stat_ptr->cu_stat_array[mdScanParentblock_index].high_chroma;
            sb_stat_ptr->cu_stat_array[0].high_chroma = cu_stat_ptr->high_chroma ? EB_TRUE :
                sb_stat_ptr->cu_stat_array[0].high_chroma;

            sb_stat_ptr->cu_stat_array[mdScanParentblock_index].high_luma = cu_stat_ptr->high_luma ? EB_TRUE :
                sb_stat_ptr->cu_stat_array[mdScanParentblock_index].high_luma;
            sb_stat_ptr->cu_stat_array[0].high_luma = cu_stat_ptr->high_luma ? EB_TRUE :
                sb_stat_ptr->cu_stat_array[0].high_luma;

        }
    }

    context_ptr->picture_num_grass_sb += sb_grass_flag ? 1 : 0;
}

void grass_skin_picture(
    SourceBasedOperationsContext *context_ptr,
    PictureParentControlSet      *picture_control_set_ptr) {
    picture_control_set_ptr->grass_percentage_in_picture = (uint8_t)(context_ptr->picture_num_grass_sb * 100 / picture_control_set_ptr->sb_total_count);
}

/******************************************************
* Detect and mark LCU and 32x32 CUs which belong to an isolated non-homogeneous region surrounding a homogenous and flat region
******************************************************/
static void determine_isolated_non_homogeneous_region_in_picture(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr)
{
    uint32_t sb_index;
    uint32_t cuu_index;
    int32_t sb_hor, sb_ver, sb_ver_offset;
    uint32_t sb_total_count = picture_control_set_ptr->sb_total_count;
    uint32_t picture_width_in_sb = sequence_control_set_ptr->picture_width_in_sb;
    uint32_t picture_height_in_sb = sequence_control_set_ptr->picture_height_in_sb;

    for (sb_index = 0; sb_index < sb_total_count; ++sb_index) {

        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
        // Initialize
        picture_control_set_ptr->sb_isolated_non_homogeneous_area_array[sb_index] = EB_FALSE;
        if ((sb_params->horizontal_index > 0) && (sb_params->horizontal_index < picture_width_in_sb - 1) && (sb_params->vertical_index > 0) && (sb_params->vertical_index < picture_height_in_sb - 1)) {
            uint32_t count_of_med_variance_sb;
            count_of_med_variance_sb = 0;

            // top neighbors
            count_of_med_variance_sb += ((picture_control_set_ptr->variance[sb_index - picture_width_in_sb - 1][ME_TIER_ZERO_PU_64x64]) <= MEDIUM_SB_VARIANCE) ? 1 : 0;
            count_of_med_variance_sb += (picture_control_set_ptr->variance[sb_index - picture_width_in_sb][ME_TIER_ZERO_PU_64x64] <= MEDIUM_SB_VARIANCE) ? 1 : 0;
            count_of_med_variance_sb += (picture_control_set_ptr->variance[sb_index - picture_width_in_sb + 1][ME_TIER_ZERO_PU_64x64] <= MEDIUM_SB_VARIANCE && sequence_control_set_ptr->sb_params_array[sb_index - picture_width_in_sb + 1].is_complete_sb) ? 1 : 0;
            // bottom
            count_of_med_variance_sb += (picture_control_set_ptr->variance[sb_index + picture_width_in_sb - 1][ME_TIER_ZERO_PU_64x64] <= MEDIUM_SB_VARIANCE && sequence_control_set_ptr->sb_params_array[sb_index + picture_width_in_sb - 1].is_complete_sb) ? 1 : 0;
            count_of_med_variance_sb += (picture_control_set_ptr->variance[sb_index + picture_width_in_sb][ME_TIER_ZERO_PU_64x64] <= MEDIUM_SB_VARIANCE && sequence_control_set_ptr->sb_params_array[sb_index + picture_width_in_sb].is_complete_sb) ? 1 : 0;
            count_of_med_variance_sb += (picture_control_set_ptr->variance[sb_index + picture_width_in_sb + 1][ME_TIER_ZERO_PU_64x64] <= MEDIUM_SB_VARIANCE && sequence_control_set_ptr->sb_params_array[sb_index + picture_width_in_sb + 1].is_complete_sb) ? 1 : 0;
            // left right
            count_of_med_variance_sb += (picture_control_set_ptr->variance[sb_index + 1][ME_TIER_ZERO_PU_64x64] <= MEDIUM_SB_VARIANCE  && sequence_control_set_ptr->sb_params_array[sb_index + 1].is_complete_sb) ? 1 : 0;
            count_of_med_variance_sb += (picture_control_set_ptr->variance[sb_index - 1][ME_TIER_ZERO_PU_64x64] <= MEDIUM_SB_VARIANCE) ? 1 : 0;

            // At least two neighbors are flat
            if ((count_of_med_variance_sb > 2)|| count_of_med_variance_sb > 1)
            {
                // Search within an LCU if any of the 32x32 CUs is non-homogeneous
                uint32_t count32x32_nonhom_cus_in_sb = 0;
                for (cuu_index = 0; cuu_index < 4; cuu_index++)
                {
                    if (picture_control_set_ptr->var_of_var_32x32_based_sb_array[sb_index][cuu_index] > VAR_BASED_DETAIL_PRESERVATION_SELECTOR_THRSLHD)
                        count32x32_nonhom_cus_in_sb++;
                }
                // If atleast one is non-homogeneous, then check all its neighbors (top left, top, top right, left, right, btm left, btm, btm right)
                uint32_t count_of_homogeneous_neighbor_sbs = 0;
                if (count32x32_nonhom_cus_in_sb > 0) {
                    for (sb_ver = -1; sb_ver <= 1; sb_ver++) {
                        sb_ver_offset = sb_ver * (int32_t)picture_width_in_sb;
                        for (sb_hor = -1; sb_hor <= 1; sb_hor++) {
                            if (sb_ver != 0 && sb_hor != 0)
                                count_of_homogeneous_neighbor_sbs += (picture_control_set_ptr->sb_homogeneous_area_array[sb_index + sb_ver_offset + sb_hor] == EB_TRUE);

                        }
                    }
                }

                // To determine current lcu is isolated non-homogeneous, at least 2 neighbors must be homogeneous
                if (count_of_homogeneous_neighbor_sbs >= 2){
                    for (cuu_index = 0; cuu_index < 4; cuu_index++)
                    {
                        if (picture_control_set_ptr->var_of_var_32x32_based_sb_array[sb_index][cuu_index] > VAR_BASED_DETAIL_PRESERVATION_SELECTOR_THRSLHD)
                        {
                            picture_control_set_ptr->sb_isolated_non_homogeneous_area_array[sb_index] = EB_TRUE;
                        }
                    }
                }

            }
        }
    }
    return;
}

static void determine_more_potential_aura_areas(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr)
{
    uint16_t sb_index;
    int32_t sb_hor, sb_ver, sb_ver_offset;
    uint32_t count_of_edge_blocks = 0, count_of_non_edge_blocks = 0;

    uint32_t lightLumaValue = 150;

    uint16_t sb_total_count = picture_control_set_ptr->sb_total_count;

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {
        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

        // For all the internal LCUs
        if (!sb_params->is_edge_sb) {
            count_of_non_edge_blocks = 0;
            if (picture_control_set_ptr->edge_results_ptr[sb_index].edge_block_num
                && picture_control_set_ptr->y_mean[sb_index][ME_TIER_ZERO_PU_64x64] >= lightLumaValue) {

                for (sb_ver = -1; sb_ver <= 1; sb_ver++) {
                    sb_ver_offset = sb_ver * (int32_t)sequence_control_set_ptr->picture_width_in_sb;
                    for (sb_hor = -1; sb_hor <= 1; sb_hor++) {
                        count_of_non_edge_blocks += (!picture_control_set_ptr->edge_results_ptr[sb_index + sb_ver_offset + sb_hor].edge_block_num) &&
                            (picture_control_set_ptr->non_moving_index_array[sb_index + sb_ver_offset + sb_hor] < 30);
                    }
                }
            }

            if (count_of_non_edge_blocks > 1) {
                count_of_edge_blocks++;
            }
        }
    }

    // To check the percentage of potential aura in the picture.. If a large area is detected then this is not isolated
    picture_control_set_ptr->percentage_of_edge_in_light_background = (uint8_t)(count_of_edge_blocks * 100 / sb_total_count);

}

/***************************************************
* Detects the presence of dark area
***************************************************/
void eb_vp9_derive_high_dark_area_density_flag(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr) {

    uint32_t    region_in_picture_width_index;
    uint32_t    region_in_picture_height_index;
    uint32_t    luma_histogram_bin;
    uint32_t    black_samples_count = 0;
    uint32_t    black_area_percentage;
    // Loop over regions inside the picture
    for (region_in_picture_width_index = 0; region_in_picture_width_index < sequence_control_set_ptr->picture_analysis_number_of_regions_per_width; region_in_picture_width_index++){  // loop over horizontal regions
        for (region_in_picture_height_index = 0; region_in_picture_height_index < sequence_control_set_ptr->picture_analysis_number_of_regions_per_height; region_in_picture_height_index++){ // loop over vertical regions
            for (luma_histogram_bin = 0; luma_histogram_bin < LOW_MEAN_TH_0; luma_histogram_bin++){ // loop over the 1st LOW_MEAN_THLD bins
                black_samples_count += picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][0][luma_histogram_bin];
            }
        }
    }

    black_area_percentage = (black_samples_count * 100) / (sequence_control_set_ptr->luma_width * sequence_control_set_ptr->luma_height);
    picture_control_set_ptr->high_dark_area_density_flag = (EB_BOOL)(black_area_percentage >= MIN_BLACK_AREA_PERCENTAGE);

    black_samples_count = 0;
    uint32_t    white_samples_count = 0;
    uint32_t    white_area_percentage;
    // Loop over regions inside the picture
    for (region_in_picture_width_index = 0; region_in_picture_width_index < sequence_control_set_ptr->picture_analysis_number_of_regions_per_width; region_in_picture_width_index++){  // loop over horizontal regions
        for (region_in_picture_height_index = 0; region_in_picture_height_index < sequence_control_set_ptr->picture_analysis_number_of_regions_per_height; region_in_picture_height_index++){ // loop over vertical regions
            for (luma_histogram_bin = 0; luma_histogram_bin < LOW_MEAN_TH_1; luma_histogram_bin++){ // loop over the 1st LOW_MEAN_THLD bins
                black_samples_count += picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][0][luma_histogram_bin];
            }
            for (luma_histogram_bin = HIGH_MEAN_TH; luma_histogram_bin < HISTOGRAM_NUMBER_OF_BINS; luma_histogram_bin++){
                white_samples_count += picture_control_set_ptr->picture_histogram[region_in_picture_width_index][region_in_picture_height_index][0][luma_histogram_bin];
            }
        }
    }

    black_area_percentage = (black_samples_count * 100) / (sequence_control_set_ptr->luma_width * sequence_control_set_ptr->luma_height);
    white_area_percentage = (white_samples_count * 100) / (sequence_control_set_ptr->luma_width * sequence_control_set_ptr->luma_height);

    picture_control_set_ptr->black_area_percentage = (uint8_t) black_area_percentage;

    picture_control_set_ptr->high_dark_low_light_area_density_flag = (EB_BOOL)(black_area_percentage >= MIN_BLACK_AREA_PERCENTAGE) && (white_area_percentage >= MIN_WHITE_AREA_PERCENTAGE);
}
#define NORM_FACTOR    10
#define VAR_MIN        10
#define VAR_MAX        300
#define MIN_Y          70
#define MAX_Y          145
#define MID_CB         140
#define MID_CR         115
#define TH_CB          10
#define TH_CR          15

/******************************************************
* High  contrast classifier
******************************************************/
void temporal_high_contrast_classifier(
    SourceBasedOperationsContext *context_ptr,
    PictureParentControlSet      *picture_control_set_ptr,
    uint32_t                      sb_index)
{

    uint32_t blk_it;
    uint32_t nsad_table[] = { 10, 5, 5, 5, 5, 5 };
    uint32_t th_res = 0;
    uint32_t nsad;
    uint32_t me_dist = 0;

    if (picture_control_set_ptr->slice_type == B_SLICE){

            for (blk_it = 0; blk_it < 4; blk_it++) {

                nsad = ((uint32_t)picture_control_set_ptr->me_results[sb_index][1 + blk_it].distortion_direction[0].distortion) >> NORM_FACTOR;

                if (nsad >= nsad_table[picture_control_set_ptr->temporal_layer_index] + th_res)
                    me_dist++;
            }

    }
    context_ptr->high_dist = me_dist>0 ? EB_TRUE : EB_FALSE;
}

void spatial_high_contrast_classifier(
    SourceBasedOperationsContext *context_ptr,
    PictureParentControlSet      *picture_control_set_ptr,
    uint32_t                      sb_index)
{

    uint32_t blk_it;

    context_ptr->high_contrast_num = 0;

    //16x16 blocks
    for (blk_it = 0; blk_it < 16; blk_it++) {

        uint8_t y_mean = context_ptr->y_mean_ptr[5 + blk_it];
        uint8_t umean = context_ptr->cb_mean_ptr[5 + blk_it];
        uint8_t vmean = context_ptr->cr_mean_ptr[5 + blk_it];

        uint16_t var = picture_control_set_ptr->variance[sb_index][5 + blk_it];

        if (var>VAR_MIN && var<VAR_MAX          &&  //medium texture
            y_mean>MIN_Y && y_mean < MAX_Y        &&  //medium brightness(not too dark and not too bright)
            ABS((int64_t)umean - MID_CB) < TH_CB &&  //middle of the color plane
            ABS((int64_t)vmean - MID_CR) < TH_CR     //middle of the color plane
            )
        {
            context_ptr->high_contrast_num++;

        }

    }
}

/******************************************************
Input   : current SB value
Output  : populate to neighbor SBs
******************************************************/
void populate_from_current_sb_to_neighbor_sbs(
    PictureParentControlSet *picture_control_set_ptr,
    EB_BOOL                  input_to_populate,
    EB_BOOL                 *output_buffer,
    uint32_t                 sb_adrr,
    uint32_t                 sb_origin_x,
    uint32_t                 sb_origin_y)
{
    uint32_t picture_width_in_sbs = (picture_control_set_ptr->enhanced_picture_ptr->width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE;

    // Copy to left if present
    if (sb_origin_x != 0) {
        output_buffer[sb_adrr - 1] = input_to_populate;
    }

    // Copy to right if present
    if ((sb_origin_x + MAX_SB_SIZE) < picture_control_set_ptr->enhanced_picture_ptr->width) {
        output_buffer[sb_adrr + 1] = input_to_populate;
    }

    // Copy to top LCU if present
    if (sb_origin_y != 0) {
        output_buffer[sb_adrr - picture_width_in_sbs] = input_to_populate;
    }

    // Copy to bottom LCU if present
    if ((sb_origin_y + MAX_SB_SIZE) < picture_control_set_ptr->enhanced_picture_ptr->height) {
        output_buffer[sb_adrr + picture_width_in_sbs] = input_to_populate;
    }

    // Copy to top-left LCU if present
    if ((sb_origin_x >= MAX_SB_SIZE) && (sb_origin_y >= MAX_SB_SIZE)) {
        output_buffer[sb_adrr - picture_width_in_sbs - 1] = input_to_populate;
    }

    // Copy to top-right LCU if present
    if ((sb_origin_x < picture_control_set_ptr->enhanced_picture_ptr->width - MAX_SB_SIZE) && (sb_origin_y >= MAX_SB_SIZE)) {
        output_buffer[sb_adrr - picture_width_in_sbs + 1] = input_to_populate;
    }

    // Copy to bottom-left LCU if present
    if ((sb_origin_x >= MAX_SB_SIZE) && (sb_origin_y < picture_control_set_ptr->enhanced_picture_ptr->height - MAX_SB_SIZE)) {
        output_buffer[sb_adrr + picture_width_in_sbs - 1] = input_to_populate;
    }

    // Copy to bottom-right LCU if present
    if ((sb_origin_x < picture_control_set_ptr->enhanced_picture_ptr->width - MAX_SB_SIZE) && (sb_origin_y < picture_control_set_ptr->enhanced_picture_ptr->height - MAX_SB_SIZE)) {
        output_buffer[sb_adrr + picture_width_in_sbs + 1] = input_to_populate;
    }
}

/******************************************************
Input   : variance
Output  : true if current & neighbors are spatially complex
******************************************************/
EB_BOOL is_spatially_complex_area(

    PictureParentControlSet *picture_control_set_ptr,
    uint32_t                 sb_adrr,
    uint32_t                 sb_origin_x,
    uint32_t                 sb_origin_y)
{

    uint32_t  available_sbs_count      = 0;
    uint32_t  high_variance_sbs_count   = 0;
    uint32_t  picture_width_in_sbs      = (picture_control_set_ptr->enhanced_picture_ptr->width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE ;

    // Check the variance of the current LCU
    if ((picture_control_set_ptr->variance[sb_adrr][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_SB_VARIANCE_TH) {
        available_sbs_count++;
        high_variance_sbs_count++;
    }

    // Check the variance of left LCU if available
    if (sb_origin_x != 0) {
        available_sbs_count++;
        if ((picture_control_set_ptr->variance[sb_adrr - 1][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_SB_VARIANCE_TH) {
            high_variance_sbs_count++;
        }
    }

    // Check the variance of right LCU if available
    if ((sb_origin_x + MAX_SB_SIZE) < picture_control_set_ptr->enhanced_picture_ptr->width) {
        available_sbs_count++;
        if ((picture_control_set_ptr->variance[sb_adrr + 1][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_SB_VARIANCE_TH) {
            high_variance_sbs_count++;
        }

    }

    // Check the variance of top LCU if available
    if (sb_origin_y != 0) {
        available_sbs_count++;
        if ((picture_control_set_ptr->variance[sb_adrr - picture_width_in_sbs][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_SB_VARIANCE_TH) {
            high_variance_sbs_count++;
        }
    }

    // Check the variance of bottom LCU
    if ((sb_origin_y + MAX_SB_SIZE) < picture_control_set_ptr->enhanced_picture_ptr->height) {
        available_sbs_count++;
        if ((picture_control_set_ptr->variance[sb_adrr + picture_width_in_sbs][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_SB_VARIANCE_TH) {
            high_variance_sbs_count++;
        }

    }

    // Check the variance of top-left LCU
    if ((sb_origin_x >= MAX_SB_SIZE) && (sb_origin_y >= MAX_SB_SIZE)) {
        available_sbs_count++;
        if ((picture_control_set_ptr->variance[sb_adrr - picture_width_in_sbs - 1][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_SB_VARIANCE_TH) {
            high_variance_sbs_count++;
        }

    }

    // Check the variance of top-right LCU
    if ((sb_origin_x < picture_control_set_ptr->enhanced_picture_ptr->width - MAX_SB_SIZE) && (sb_origin_y >= MAX_SB_SIZE)) {
        available_sbs_count++;
        if ((picture_control_set_ptr->variance[sb_adrr - picture_width_in_sbs + 1][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_SB_VARIANCE_TH) {
            high_variance_sbs_count++;
        }
    }

    // Check the variance of bottom-left LCU
    if ((sb_origin_x >= MAX_SB_SIZE) && (sb_origin_y < picture_control_set_ptr->enhanced_picture_ptr->height - MAX_SB_SIZE)) {
        available_sbs_count++;
        if ((picture_control_set_ptr->variance[sb_adrr + picture_width_in_sbs - 1][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_SB_VARIANCE_TH) {
            high_variance_sbs_count++;
        }

    }

    // Check the variance of bottom-right LCU
    if ((sb_origin_x < picture_control_set_ptr->enhanced_picture_ptr->width - MAX_SB_SIZE) && (sb_origin_y < picture_control_set_ptr->enhanced_picture_ptr->height - MAX_SB_SIZE)) {
        available_sbs_count++;
        if ((picture_control_set_ptr->variance[sb_adrr + picture_width_in_sbs + 1][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_SB_VARIANCE_TH) {
            high_variance_sbs_count++;
        }
    }

    if (high_variance_sbs_count == available_sbs_count) {
        return EB_TRUE;
    }

    return EB_FALSE;

}

/******************************************************
Input   : activity, layer index
Output  : LCU complexity level
******************************************************/
void eb_vp9_derive_blockiness_present_flag(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr)
{
    uint32_t sb_index;

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

        SbParams         *sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];
        picture_control_set_ptr->complex_sb_array[sb_index] = SB_COMPLEXITY_STATUS_INVALID;

        // Spatially complex LCU within a spatially complex area
        if (is_spatially_complex_area(
            picture_control_set_ptr,
            sb_index,
            sb_params_ptr->origin_x,
            sb_params_ptr->origin_y)
            && picture_control_set_ptr->non_moving_index_array[sb_index]  != INVALID_NON_MOVING_SCORE
            && picture_control_set_ptr->non_moving_average_score          != INVALID_NON_MOVING_SCORE

            ) {

            // Active LCU within an active scene (added a check on 4K & non-BASE to restrict the action - could be generated for all resolutions/layers)
            if (picture_control_set_ptr->non_moving_index_array[sb_index] == SB_COMPLEXITY_NON_MOVING_INDEX_TH_0 && picture_control_set_ptr->non_moving_average_score >= SB_COMPLEXITY_NON_MOVING_INDEX_TH_1 && picture_control_set_ptr->temporal_layer_index > 0 && sequence_control_set_ptr->input_resolution == INPUT_SIZE_4K_RANGE) {
                picture_control_set_ptr->complex_sb_array[sb_index] = SB_COMPLEXITY_STATUS_2;
            }
            // Active LCU within a scene with a moderate acitivity (eg. active foregroud & static background)
            else if (picture_control_set_ptr->non_moving_index_array[sb_index] == SB_COMPLEXITY_NON_MOVING_INDEX_TH_0 && picture_control_set_ptr->non_moving_average_score >= SB_COMPLEXITY_NON_MOVING_INDEX_TH_2 && picture_control_set_ptr->non_moving_average_score < SB_COMPLEXITY_NON_MOVING_INDEX_TH_1) {
                picture_control_set_ptr->complex_sb_array[sb_index] = SB_COMPLEXITY_STATUS_1;
            }
            else {
                picture_control_set_ptr->complex_sb_array[sb_index] = SB_COMPLEXITY_STATUS_0;
            }
        }
        else {
            picture_control_set_ptr->complex_sb_array[sb_index] = SB_COMPLEXITY_STATUS_0;
        }

    }
}

void eb_vp9_derive_min_max_me_distortion(
    SequenceControlSet      *sequence_control_set_ptr,
    PictureParentControlSet *picture_control_set_ptr)
{
    uint32_t me_distortion;

    picture_control_set_ptr->min_me_distortion = ~0u;
    picture_control_set_ptr->max_me_distortion =   0;

    uint32_t sb_index;

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; sb_index++) {
        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
        if (sb_params->pa_raster_scan_block_validity[PA_RASTER_SCAN_CU_INDEX_64x64]) {
            me_distortion = picture_control_set_ptr->me_results[sb_index][PA_RASTER_SCAN_CU_INDEX_64x64].distortion_direction[0].distortion;
            picture_control_set_ptr->min_me_distortion = me_distortion < picture_control_set_ptr->min_me_distortion ? me_distortion : picture_control_set_ptr->min_me_distortion;
            picture_control_set_ptr->max_me_distortion = me_distortion > picture_control_set_ptr->max_me_distortion ? me_distortion : picture_control_set_ptr->max_me_distortion;
        }
    }
}

static void stationary_edge_over_update_over_time_sb(
    SequenceControlSet      *sequence_control_set_ptr,
    uint32_t                 total_checked_pictures,
    PictureParentControlSet *picture_control_set_ptr,
    uint32_t                 total_sb_count)
{

    uint32_t sb_index;

    const uint32_t slide_window_th = ((total_checked_pictures / 4) - 1);

    for (sb_index = 0; sb_index < total_sb_count; sb_index++) {

        SbParams *sb_params  = &sequence_control_set_ptr->sb_params_array[sb_index];
        SbStat   *sb_stat_ptr = &picture_control_set_ptr->sb_stat_array[sb_index];

        sb_stat_ptr->stationary_edge_over_time_flag = EB_FALSE;
        if (sb_params->potential_logo_sb && sb_params->is_complete_sb && sb_stat_ptr->check1_for_logo_stationary_edge_over_time_flag && sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag) {

            uint32_t raster_scan_block_index;
            uint32_t similar_edge_count_sb = 0;
            // CU Loop
            for (raster_scan_block_index = PA_RASTER_SCAN_CU_INDEX_16x16_0; raster_scan_block_index <= PA_RASTER_SCAN_CU_INDEX_16x16_15; raster_scan_block_index++) {
                similar_edge_count_sb += (sb_stat_ptr->cu_stat_array[raster_scan_block_index].similar_edge_count > slide_window_th) ? 1 : 0;
            }

            sb_stat_ptr->stationary_edge_over_time_flag = (similar_edge_count_sb >= 4) ? EB_TRUE : EB_FALSE;
        }

        sb_stat_ptr->pm_stationary_edge_over_time_flag = EB_FALSE;
        if (sb_params->potential_logo_sb && sb_params->is_complete_sb && sb_stat_ptr->pm_check1_for_logo_stationary_edge_over_time_flag && sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag) {

            uint32_t raster_scan_block_index;
            uint32_t similar_edge_count_sb = 0;
            // CU Loop
            for (raster_scan_block_index = PA_RASTER_SCAN_CU_INDEX_16x16_0; raster_scan_block_index <= PA_RASTER_SCAN_CU_INDEX_16x16_15; raster_scan_block_index++) {
                similar_edge_count_sb += (sb_stat_ptr->cu_stat_array[raster_scan_block_index].pm_similar_edge_count > slide_window_th) ? 1 : 0;
            }

            sb_stat_ptr->pm_stationary_edge_over_time_flag = (similar_edge_count_sb >= 4) ? EB_TRUE : EB_FALSE;
        }

    }
    {
        uint32_t sb_index;
        uint32_t sb_x, sb_y;
        uint32_t count_of_neighbors = 0;

        uint32_t count_of_neighbors_pm = 0;

        int32_t sb_hor, sb_ver, sb_ver_offset;
        int32_t sb_hor_s, sb_ver_s, sb_hor_e, sb_ver_e;
        uint32_t picture_width_in_sb = sequence_control_set_ptr->picture_width_in_sb;
        uint32_t picture_height_in_sb = sequence_control_set_ptr->picture_height_in_sb;

        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams     *sb_params  = &sequence_control_set_ptr->sb_params_array[sb_index];
            SbStat       *sb_stat_ptr = &picture_control_set_ptr->sb_stat_array[sb_index];

            sb_x = sb_params->horizontal_index;
            sb_y = sb_params->vertical_index;
            if (sb_params->potential_logo_sb && sb_params->is_complete_sb && sb_stat_ptr->check1_for_logo_stationary_edge_over_time_flag && sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag) {
                {
                    sb_hor_s = (sb_x > 0) ? -1 : 0;
                    sb_hor_e = (sb_x < picture_width_in_sb - 1) ? 1 : 0;
                    sb_ver_s = (sb_y > 0) ? -1 : 0;
                    sb_ver_e = (sb_y < picture_height_in_sb - 1) ? 1 : 0;
                    count_of_neighbors = 0;
                    for (sb_ver = sb_ver_s; sb_ver <= sb_ver_e; sb_ver++) {
                        sb_ver_offset = sb_ver * (int32_t)picture_width_in_sb;
                        for (sb_hor = sb_hor_s; sb_hor <= sb_hor_e; sb_hor++) {
                            count_of_neighbors += (picture_control_set_ptr->sb_stat_array[sb_index + sb_ver_offset + sb_hor].stationary_edge_over_time_flag == 1);

                        }
                    }
                    if (count_of_neighbors == 1) {
                        sb_stat_ptr->stationary_edge_over_time_flag = 0;
                    }
                }
            }

            if (sb_params->potential_logo_sb && sb_params->is_complete_sb && sb_stat_ptr->pm_check1_for_logo_stationary_edge_over_time_flag && sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag) {
                {
                    sb_hor_s = (sb_x > 0) ? -1 : 0;
                    sb_hor_e = (sb_x < picture_width_in_sb - 1) ? 1 : 0;
                    sb_ver_s = (sb_y > 0) ? -1 : 0;
                    sb_ver_e = (sb_y < picture_height_in_sb - 1) ? 1 : 0;
                    count_of_neighbors_pm = 0;
                    for (sb_ver = sb_ver_s; sb_ver <= sb_ver_e; sb_ver++) {
                        sb_ver_offset = sb_ver * (int32_t)picture_width_in_sb;
                        for (sb_hor = sb_hor_s; sb_hor <= sb_hor_e; sb_hor++) {
                            count_of_neighbors_pm += (picture_control_set_ptr->sb_stat_array[sb_index + sb_ver_offset + sb_hor].pm_stationary_edge_over_time_flag == 1);
                        }
                    }

                    if (count_of_neighbors_pm == 1) {
                        sb_stat_ptr->pm_stationary_edge_over_time_flag = 0;
                    }
                }
            }

        }
    }

    {

        uint32_t sb_index;
        uint32_t sb_x, sb_y;
        uint32_t count_of_neighbors = 0;
        int32_t  sb_hor, sb_ver, sb_ver_offset;
        int32_t  sb_hor_s, sb_ver_s, sb_hor_e, sb_ver_e;
        uint32_t picture_width_in_sb = sequence_control_set_ptr->picture_width_in_sb;
        uint32_t picture_height_in_sb = sequence_control_set_ptr->picture_height_in_sb;

        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams        *sb_params  = &sequence_control_set_ptr->sb_params_array[sb_index];
            SbStat       *sb_stat_ptr = &picture_control_set_ptr->sb_stat_array[sb_index];

            sb_x = sb_params->horizontal_index;
            sb_y = sb_params->vertical_index;

            {
                if (sb_stat_ptr->stationary_edge_over_time_flag == 0 && sb_params->potential_logo_sb && (sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag || !sb_params->is_complete_sb)) {
                    sb_hor_s = (sb_x > 0) ? -1 : 0;
                    sb_hor_e = (sb_x < picture_width_in_sb - 1) ? 1 : 0;
                    sb_ver_s = (sb_y > 0) ? -1 : 0;
                    sb_ver_e = (sb_y < picture_height_in_sb - 1) ? 1 : 0;
                    count_of_neighbors = 0;
                    for (sb_ver = sb_ver_s; sb_ver <= sb_ver_e; sb_ver++) {
                        sb_ver_offset = sb_ver * (int32_t)picture_width_in_sb;
                        for (sb_hor = sb_hor_s; sb_hor <= sb_hor_e; sb_hor++) {
                            count_of_neighbors += (picture_control_set_ptr->sb_stat_array[sb_index + sb_ver_offset + sb_hor].stationary_edge_over_time_flag == 1);

                        }
                    }
                    if (count_of_neighbors > 0) {
                        sb_stat_ptr->stationary_edge_over_time_flag = 2;
                    }

                }
            }
        }

        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams     *sb_params  = &sequence_control_set_ptr->sb_params_array[sb_index];
            SbStat       *sb_stat_ptr = &picture_control_set_ptr->sb_stat_array[sb_index];

            sb_x = sb_params->horizontal_index;
            sb_y = sb_params->vertical_index;

            {
                if (sb_stat_ptr->stationary_edge_over_time_flag == 0 && sb_params->potential_logo_sb && (sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag || !sb_params->is_complete_sb)) {

                    sb_hor_s = (sb_x > 0) ? -1 : 0;
                    sb_hor_e = (sb_x < picture_width_in_sb - 1) ? 1 : 0;
                    sb_ver_s = (sb_y > 0) ? -1 : 0;
                    sb_ver_e = (sb_y < picture_height_in_sb - 1) ? 1 : 0;
                    count_of_neighbors = 0;
                    for (sb_ver = sb_ver_s; sb_ver <= sb_ver_e; sb_ver++) {
                        sb_ver_offset = sb_ver * (int32_t)picture_width_in_sb;
                        for (sb_hor = sb_hor_s; sb_hor <= sb_hor_e; sb_hor++) {
                            count_of_neighbors += (picture_control_set_ptr->sb_stat_array[sb_index + sb_ver_offset + sb_hor].stationary_edge_over_time_flag == 2);

                        }
                    }
                    if (count_of_neighbors > 3) {
                        sb_stat_ptr->stationary_edge_over_time_flag = 3;
                    }

                }
            }
        }
    }

    {

        uint32_t sb_index;
        uint32_t sb_x, sb_y;
        uint32_t count_of_neighbors = 0;
        int32_t  sb_hor, sb_ver, sb_ver_offset;
        int32_t  sb_hor_s, sb_ver_s, sb_hor_e, sb_ver_e;
        uint32_t picture_width_in_sb = sequence_control_set_ptr->picture_width_in_sb;
        uint32_t picture_height_in_sb = sequence_control_set_ptr->picture_height_in_sb;

        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams     *sb_params  = &sequence_control_set_ptr->sb_params_array[sb_index];
            SbStat       *sb_stat_ptr = &picture_control_set_ptr->sb_stat_array[sb_index];

            sb_x = sb_params->horizontal_index;
            sb_y = sb_params->vertical_index;

            {
                if (sb_stat_ptr->pm_stationary_edge_over_time_flag == 0 && sb_params->potential_logo_sb && (sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag || !sb_params->is_complete_sb)) {
                    sb_hor_s = (sb_x > 0) ? -1 : 0;
                    sb_hor_e = (sb_x < picture_width_in_sb - 1) ? 1 : 0;
                    sb_ver_s = (sb_y > 0) ? -1 : 0;
                    sb_ver_e = (sb_y < picture_height_in_sb - 1) ? 1 : 0;
                    count_of_neighbors = 0;
                    for (sb_ver = sb_ver_s; sb_ver <= sb_ver_e; sb_ver++) {
                        sb_ver_offset = sb_ver * (int32_t)picture_width_in_sb;
                        for (sb_hor = sb_hor_s; sb_hor <= sb_hor_e; sb_hor++) {
                            count_of_neighbors += (picture_control_set_ptr->sb_stat_array[sb_index + sb_ver_offset + sb_hor].pm_stationary_edge_over_time_flag == 1);

                        }
                    }
                    if (count_of_neighbors > 0) {
                        sb_stat_ptr->pm_stationary_edge_over_time_flag = 2;
                    }

                }
            }
        }

        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams     *sb_params  = &sequence_control_set_ptr->sb_params_array[sb_index];
            SbStat       *sb_stat_ptr = &picture_control_set_ptr->sb_stat_array[sb_index];

            sb_x = sb_params->horizontal_index;
            sb_y = sb_params->vertical_index;

            {
                if (sb_stat_ptr->pm_stationary_edge_over_time_flag == 0 && sb_params->potential_logo_sb && (sb_stat_ptr->check2_for_logo_stationary_edge_over_time_flag || !sb_params->is_complete_sb)) {

                    sb_hor_s = (sb_x > 0) ? -1 : 0;
                    sb_hor_e = (sb_x < picture_width_in_sb - 1) ? 1 : 0;
                    sb_ver_s = (sb_y > 0) ? -1 : 0;
                    sb_ver_e = (sb_y < picture_height_in_sb - 1) ? 1 : 0;
                    count_of_neighbors = 0;
                    for (sb_ver = sb_ver_s; sb_ver <= sb_ver_e; sb_ver++) {
                        sb_ver_offset = sb_ver * (int32_t)picture_width_in_sb;
                        for (sb_hor = sb_hor_s; sb_hor <= sb_hor_e; sb_hor++) {
                            count_of_neighbors += (picture_control_set_ptr->sb_stat_array[sb_index + sb_ver_offset + sb_hor].pm_stationary_edge_over_time_flag == 2);

                        }
                    }
                    if (count_of_neighbors > 3) {
                        sb_stat_ptr->pm_stationary_edge_over_time_flag = 3;
                    }

                }
            }
        }
    }
}

/************************************************
 * Source Based Operations Kernel
 ************************************************/
void* eb_vp9_source_based_operations_kernel(void *input_ptr)
{
    SourceBasedOperationsContext *context_ptr = (SourceBasedOperationsContext*)input_ptr;
    PictureParentControlSet      *picture_control_set_ptr;
    SequenceControlSet           *sequence_control_set_ptr;
    EbObjectWrapper              *input_results_wrapper_ptr;
    InitialRateControlResults    *input_results_ptr;
    EbObjectWrapper              *output_results_wrapper_ptr;
    PictureDemuxResults          *output_results_ptr;

    for (;;) {

        // Get Input Full Object
        eb_vp9_get_full_object(
            context_ptr->initial_rate_control_results_input_fifo_ptr,
            &input_results_wrapper_ptr);

        input_results_ptr = (InitialRateControlResults*)input_results_wrapper_ptr->object_ptr;
        picture_control_set_ptr = (PictureParentControlSet  *)input_results_ptr->picture_control_set_wrapper_ptr->object_ptr;
        sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;

        picture_control_set_ptr->dark_background_light_foreground = EB_FALSE;
        context_ptr->picture_num_grass_sb = 0;
        uint32_t sb_total_count = picture_control_set_ptr->sb_total_count;
        uint32_t sb_index;

        /***********************************************LCU-based operations************************************************************/
        for (sb_index = 0; sb_index < sb_total_count; ++sb_index) {
            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
            picture_control_set_ptr->sb_cmplx_contrast_array[sb_index] = 0;
            EB_BOOL is_complete_sb = sb_params->is_complete_sb;
            uint8_t  *y_mean_ptr = picture_control_set_ptr->y_mean[sb_index];

            _mm_prefetch((const char*)y_mean_ptr, _MM_HINT_T0);

            uint8_t  *cr_mean_ptr = picture_control_set_ptr->cr_mean[sb_index];
            uint8_t  *cb_mean_ptr = picture_control_set_ptr->cb_mean[sb_index];

            _mm_prefetch((const char*)cr_mean_ptr, _MM_HINT_T0);
            _mm_prefetch((const char*)cb_mean_ptr, _MM_HINT_T0);

            context_ptr->y_mean_ptr = y_mean_ptr;
            context_ptr->cr_mean_ptr = cr_mean_ptr;
            context_ptr->cb_mean_ptr = cb_mean_ptr;

            // Grass & Skin detection
            grass_skin_sb(
                context_ptr,
                sequence_control_set_ptr,
                picture_control_set_ptr,
                sb_index);

            // Spatial high contrast classifier
            if (is_complete_sb) {
                spatial_high_contrast_classifier(
                    context_ptr,
                    picture_control_set_ptr,
                    sb_index);
            }

            // Temporal high contrast classifier
            if (is_complete_sb) {
                temporal_high_contrast_classifier(
                    context_ptr,
                    picture_control_set_ptr,
                    sb_index);

                if (context_ptr->high_contrast_num && context_ptr->high_dist) {
                    populate_from_current_sb_to_neighbor_sbs(
                        picture_control_set_ptr,
                        (context_ptr->high_contrast_num && context_ptr->high_dist),
                        picture_control_set_ptr->sb_cmplx_contrast_array,
                        sb_index,
                        sb_params->origin_x,
                        sb_params->origin_y);
                }
            }
        }

        /*********************************************Picture-based operations**********************************************************/
        // Dark density derivation (histograms not available when no SCD)
        eb_vp9_derive_high_dark_area_density_flag(
            sequence_control_set_ptr,
            picture_control_set_ptr);

        // Detect and mark LCU and 32x32 CUs which belong to an isolated non-homogeneous region surrounding a homogenous and flat region.
        determine_isolated_non_homogeneous_region_in_picture(
            sequence_control_set_ptr,
            picture_control_set_ptr);

        // Detect aura areas in lighter background when subject is moving similar to background
        determine_more_potential_aura_areas(
            sequence_control_set_ptr,
            picture_control_set_ptr);

        // Activity statistics derivation
        eb_vp9_derive_picture_activity_statistics(
            sequence_control_set_ptr,
            picture_control_set_ptr);

        // Derive blockinessPresentFlag
        eb_vp9_derive_blockiness_present_flag(
            sequence_control_set_ptr,
            picture_control_set_ptr);

        // Skin & Grass detection
        grass_skin_picture(
            context_ptr,
            picture_control_set_ptr);

        // Stationary edge over time (final stage)
        if (!picture_control_set_ptr->end_of_sequence_flag && sequence_control_set_ptr->look_ahead_distance != 0) {
            stationary_edge_over_update_over_time_sb(
                sequence_control_set_ptr,
                MIN(MIN(((picture_control_set_ptr->pred_struct_ptr->pred_struct_period << 1) + 1), picture_control_set_ptr->frames_in_sw), sequence_control_set_ptr->look_ahead_distance),
                picture_control_set_ptr,
                picture_control_set_ptr->sb_total_count);
        }

        // Derive Min/Max ME distortion
        if (picture_control_set_ptr->pic_depth_mode == PIC_SB_SWITCH_DEPTH_MODE) {
            eb_vp9_derive_min_max_me_distortion(
                sequence_control_set_ptr,
                picture_control_set_ptr);
        }

        // Get Empty Results Object
        eb_vp9_get_empty_object(
            context_ptr->picture_demux_results_output_fifo_ptr,
            &output_results_wrapper_ptr);

        output_results_ptr = (PictureDemuxResults*)output_results_wrapper_ptr->object_ptr;
        output_results_ptr->picture_control_set_wrapper_ptr = input_results_ptr->picture_control_set_wrapper_ptr;
        output_results_ptr->picture_type = EB_PIC_INPUT;

        // Release the Input Results
        eb_vp9_release_object(input_results_wrapper_ptr);

        // Post the Full Results Object
        eb_vp9_post_full_object(output_results_wrapper_ptr);

    }
    return EB_NULL;
}
