/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbUtility.h"
#include <math.h>

#include "EbSequenceControlSet.h"
#include "EbPictureControlSet.h"

#include "EbModeDecisionConfigurationProcess.h"
#include "EbRateControlResults.h"
#include "EbEncDecTasks.h"
#include "EbReferenceObject.h"
#include "EbEncDecProcess.h"
#include "EbRateDistortionCost.h"
#include "stdint.h"

#if SEG_SUPPORT
#include "vp9_segmentation.h"
#endif

// Shooting states
#define UNDER_SHOOTING                        0
#define OVER_SHOOTING                         1
#define TBD_SHOOTING                          2

// Set a cost to each search method (could be modified)
// EB30 @ Revision 12879
#define PRED_OPEN_LOOP_1_NFL_COST    97 // PRED_OPEN_LOOP_1_NFL_COST is ~03% faster than PRED_OPEN_LOOP_COST
#define U_099                        99
#define PRED_OPEN_LOOP_COST         100 // Let's assume PRED_OPEN_LOOP_COST costs ~100 U
#define U_101                       101
#define U_102                       102
#define U_103                       103
#define U_104                       104
#define U_105                       105
#define LIGHT_OPEN_LOOP_COST        106 // L_MDC is ~06% slower than PRED_OPEN_LOOP_COST
#define U_107                       107
#define U_108                       108
#define U_109                       109
#define OPEN_LOOP_COST              110 // F_MDC is ~10% slower than PRED_OPEN_LOOP_COST
#define U_111                       111
#define U_112                       112
#define U_113                       113
#define U_114                       114
#define U_115                       115
#define U_116                       116
#define U_117                       117
#define U_119                       119
#define U_120                       120
#define U_121                       121
#define U_122                       122
#define LIGHT_AVC_COST              122
#define LIGHT_BDP_COST              123 // L_BDP is ~23% slower than PRED_OPEN_LOOP_COST
#define U_125                       125
#define U_127                       127
#define BDP_COST                    129 // F_BDP is ~29% slower than PRED_OPEN_LOOP_COST
#define U_130                       130
#define U_132                       132
#define U_133                       133
#define U_134                       134
#define AVC_COST                    138 // L_BDP is ~38% slower than PRED_OPEN_LOOP_COST
#define U_140                       140
#define U_145                       145
#define U_150                       150
#define U_152                       152
#define FULL_SEARCH_COST            155 // FS    is ~55% slower than PRED_OPEN_LOOP_COST

// ADP LCU score Manipulation
#define ADP_CLASS_SHIFT_DIST_0    50
#define ADP_CLASS_SHIFT_DIST_1    75

#define ADP_BLACK_AREA_PERCENTAGE 25
#define ADP_DARK_SB_TH           25

#define ADP_CLASS_NON_MOVING_INDEX_TH_0 10
#define ADP_CLASS_NON_MOVING_INDEX_TH_1 25
#define ADP_CLASS_NON_MOVING_INDEX_TH_2 30

#define ADP_CONFIG_NON_MOVING_INDEX_TH_0 15
#define ADP_CONFIG_NON_MOVING_INDEX_TH_1 30

static const uint8_t adp_luminosity_change_th_array[MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = { // [Highest Temporal Layer] [Temporal Layer Index]
    {  2 },
    {  2, 2 },
    {  3, 2, 2 },
    {  3, 3, 2, 2 },
    {  4, 3, 3, 2, 2 },
    {  4, 4, 3, 3, 2, 2 },
};

#define HIGH_HOMOGENOUS_PIC_TH               30

#define US_ADJ_STEP_TH0                      7
#define US_ADJ_STEP_TH1                      5
#define US_ADJ_STEP_TH2                      2
#define OS_ADJ_STEP_TH1                      2
#define OS_ADJ_STEP_TH2                      5
#define OS_ADJ_STEP_TH3                      7

#define VALID_SLOT_TH                        2

/******************************************************
 * Mode Decision Configuration Context Constructor
 ******************************************************/
EbErrorType eb_vp9_mode_decision_configuration_context_ctor(
    ModeDecisionConfigurationContext **context_dbl_ptr,
    EbFifo                            *rate_control_input_fifo_ptr,
    EbFifo                            *mode_decision_configuration_output_fifo_ptr,
    uint16_t                           sb_total_count)

{
    ModeDecisionConfigurationContext *context_ptr;

    EB_MALLOC(ModeDecisionConfigurationContext*, context_ptr, sizeof(ModeDecisionConfigurationContext), EB_N_PTR);

    *context_dbl_ptr = context_ptr;

    // Input/Output System Resource Manager FIFOs
    context_ptr->rate_control_input_fifo_ptr                = rate_control_input_fifo_ptr;
    context_ptr->mode_decision_configuration_output_fifo_ptr = mode_decision_configuration_output_fifo_ptr;

    // Budgeting
    EB_MALLOC(uint32_t*,context_ptr->sb_score_array, sizeof(uint32_t) * sb_total_count, EB_N_PTR);
    EB_MALLOC(uint8_t *,context_ptr->sb_cost_array , sizeof(uint8_t ) * sb_total_count, EB_N_PTR);

    // Open Loop Partitioning
    EB_MALLOC(ModeDecisionCandidate*, context_ptr->candidate_ptr, sizeof(ModeDecisionCandidate), EB_N_PTR);
    EB_MALLOC(ModeInfo*, context_ptr->candidate_ptr->mode_info, sizeof(ModeInfo), EB_N_PTR);

    EB_MALLOC(MACROBLOCKD*, context_ptr->e_mbd, sizeof(MACROBLOCKD), EB_N_PTR);

    EB_MALLOC(MbModeInfoExt*, context_ptr->mbmi_ext, sizeof(MbModeInfoExt), EB_N_PTR);
    EB_MEMSET(context_ptr->mbmi_ext, 0, sizeof(MbModeInfoExt)); // Hsan: reference MVs not generated @ MDC (i.e. always (0,0))

    return EB_ErrorNone;
}

/******************************************************
* Detect complex/non-flat/moving LCU in a non-complex area (used to refine MDC depth control)
******************************************************/
void complex_non_flat_moving_sb(
    SequenceControlSet *sequence_control_set_ptr,
    PictureControlSet  *picture_control_set_ptr,
    uint32_t            picture_width_in_sb) {

    SbUnit *sb_ptr;
    uint32_t sb_index;

    if (picture_control_set_ptr->parent_pcs_ptr->non_moving_average_score != INVALID_NON_MOVING_SCORE && picture_control_set_ptr->parent_pcs_ptr->non_moving_average_score >= 10 && picture_control_set_ptr->temporal_layer_index <= 2) {

        // Determine deltaQP and assign QP value for each leaf
        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

            sb_ptr = picture_control_set_ptr->sb_ptr_array[sb_index];

            EB_BOOL condition = EB_FALSE;

            if (!picture_control_set_ptr->parent_pcs_ptr->similar_colocated_sb_array[sb_index] &&
                sb_ptr->picture_control_set_ptr->parent_pcs_ptr->edge_results_ptr[sb_index].edge_block_num > 0) {
                condition = EB_TRUE;
            }

            if (condition){
                uint32_t  counter = 0;

                if (!sb_params->is_edge_sb){
                    // Top
                    if (picture_control_set_ptr->parent_pcs_ptr->edge_results_ptr[sb_index - picture_width_in_sb].edge_block_num == 0)
                        counter++;
                    // Bottom
                    if (picture_control_set_ptr->parent_pcs_ptr->edge_results_ptr[sb_index + picture_width_in_sb].edge_block_num == 0)
                        counter++;
                    // Left
                    if (picture_control_set_ptr->parent_pcs_ptr->edge_results_ptr[sb_index - 1].edge_block_num == 0)
                        counter++;
                    // right
                    if (picture_control_set_ptr->parent_pcs_ptr->edge_results_ptr[sb_index + 1].edge_block_num == 0)
                        counter++;
                }
            }
        }
    }
}

EB_AURA_STATUS aura_detection64x64(
    PictureControlSet *picture_control_set_ptr,
    uint8_t            picture_qp,
    uint32_t           sb_index
    )
{

    SequenceControlSet *sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
    int32_t             picture_width_in_sb = (sequence_control_set_ptr->luma_width + MAX_SB_SIZE_MINUS_1) >> MAX_LOG2_SB_SIZE;
    uint32_t            curr_dist;
    uint32_t            top_dist, top_l_dist, top_r_dist;
    uint32_t            local_avg_dist, dist_thresh0, dist_thresh1;
    int32_t             sb_offset;

    EB_AURA_STATUS      aura_class = AURA_STATUS_0;
    uint8_t             aura_class1 = 0;
    uint8_t             aura_class2 = 0;

    int32_t             x_mv0 = 0;
    int32_t             y_mv0 = 0;
    int32_t             x_mv1 = 0;
    int32_t             y_mv1 = 0;

    uint32_t            left_dist, right_dist;

    SbParams           *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

    dist_thresh0 = picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag ||sequence_control_set_ptr->input_resolution == INPUT_SIZE_4K_RANGE ? 15 : 14;
    dist_thresh1 = 23;

    if (picture_qp > 38){
        dist_thresh0 = dist_thresh0 << 2;
        dist_thresh1 = dist_thresh1 << 2;
    }

    if (!sb_params->is_edge_sb){

        uint32_t k;

        MeCuResults *me_pu_result = &picture_control_set_ptr->parent_pcs_ptr->me_results[sb_index][0];

        //Curr Block

        for (k = 0; k < me_pu_result->total_me_candidate_index; k++) {

            if (me_pu_result->distortion_direction[k].direction == UNI_PRED_LIST_0) {
                // Get reference list 0 / reference index 0 MV
                x_mv0 = me_pu_result->x_mv_l0;
                y_mv0 = me_pu_result->y_mv_l0;
            }
            if (me_pu_result->distortion_direction[k].direction == UNI_PRED_LIST_1) {
                // Get reference list  1 / reference index 0 MV
                x_mv1 = me_pu_result->x_mv_l1;
                y_mv1 = me_pu_result->y_mv_l1;
            }

        }
        curr_dist = me_pu_result->distortion_direction[0].distortion;

        if ((curr_dist > 64 * 64) &&
            // Only mark a block as aura when it is moving (MV amplitude higher than X; X is temporal layer dependent)
            (abs(x_mv0) > global_motion_threshold[picture_control_set_ptr->parent_pcs_ptr->hierarchical_levels][picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index] ||
            abs(y_mv0) > global_motion_threshold[picture_control_set_ptr->parent_pcs_ptr->hierarchical_levels][picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index] ||
            abs(x_mv1) > global_motion_threshold[picture_control_set_ptr->parent_pcs_ptr->hierarchical_levels][picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index] ||
            abs(y_mv1) > global_motion_threshold[picture_control_set_ptr->parent_pcs_ptr->hierarchical_levels][picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index]))
        {

            //Top Distortion
            sb_offset = -picture_width_in_sb;
            top_dist = picture_control_set_ptr->parent_pcs_ptr->me_results[sb_index + sb_offset]->distortion_direction[0].distortion;

            //TopLeft Distortion
            sb_offset = -picture_width_in_sb - 1;
            top_l_dist = picture_control_set_ptr->parent_pcs_ptr->me_results[sb_index + sb_offset]->distortion_direction[0].distortion;

            //TopRightDistortion
            sb_offset = -picture_width_in_sb + 1;
            top_r_dist = picture_control_set_ptr->parent_pcs_ptr->me_results[sb_index + sb_offset]->distortion_direction[0].distortion;

            top_r_dist = (sb_params->horizontal_index < (uint32_t)(picture_width_in_sb - 2)) ? top_r_dist : curr_dist;

            //left Distortion
            sb_offset = -1;
            left_dist = picture_control_set_ptr->parent_pcs_ptr->me_results[sb_index + sb_offset]->distortion_direction[0].distortion;

            //RightDistortion
            sb_offset = 1;
            right_dist = picture_control_set_ptr->parent_pcs_ptr->me_results[sb_index + sb_offset]->distortion_direction[0].distortion;

            right_dist = (sb_params->horizontal_index < (uint32_t)(picture_width_in_sb - 2)) ? top_r_dist : curr_dist;

            local_avg_dist = MIN(MIN(MIN(top_l_dist, MIN(top_r_dist, top_dist)), left_dist), right_dist);

            if (10 * curr_dist > dist_thresh0*local_avg_dist){
                if (10 * curr_dist > dist_thresh1*local_avg_dist)
                    aura_class2++;
                else
                    aura_class1++;
            }
        }

    }

    aura_class = (aura_class2 > 0 || aura_class1 > 0) ? AURA_STATUS_1 : AURA_STATUS_0;

    return   aura_class;

}

/******************************************************
* Aura detection
******************************************************/
void aura_detection(
    SequenceControlSet *sequence_control_set_ptr,
    PictureControlSet  *picture_control_set_ptr)
{
    uint32_t sb_index;

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
        SbUnit   *sb_ptr = picture_control_set_ptr->sb_ptr_array[sb_index];

        // Aura status intialization
        sb_ptr->aura_status = INVALID_AURA_STATUS;

        if (picture_control_set_ptr->slice_type == B_SLICE){
            if (!sb_params->is_edge_sb){
                sb_ptr->aura_status = aura_detection64x64(
                    picture_control_set_ptr,
                    (uint8_t)picture_control_set_ptr->picture_qp,
                    sb_ptr->sb_index);
            }
        }
    }
return;
}

EbErrorType eb_vp9_derive_default_segments(
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    // @ BASE MDC is not considered as similar to BDP_L in term of speed
    if (picture_control_set_ptr->temporal_layer_index == 0) {

        if (context_ptr->adp_depth_sensitive_picture_class && context_ptr->budget >= (uint32_t)(picture_control_set_ptr->parent_pcs_ptr->sb_total_count * LIGHT_BDP_COST)) {

            if (context_ptr->budget > (uint32_t) (picture_control_set_ptr->parent_pcs_ptr->sb_total_count * BDP_COST)) {
                context_ptr->number_of_segments = 2;
                context_ptr->score_th[0] = (int8_t)((1 * 100) / context_ptr->number_of_segments);
                context_ptr->interval_cost[0] = context_ptr->cost_depth_mode[SB_BDP_DEPTH_MODE - 1];
                context_ptr->interval_cost[1] = context_ptr->cost_depth_mode[SB_FULL84_DEPTH_MODE - 1];
            }
            else {
                context_ptr->number_of_segments = 2;
                context_ptr->score_th[0] = (int8_t)((1 * 100) / context_ptr->number_of_segments);
                context_ptr->interval_cost[0] = context_ptr->cost_depth_mode[SB_LIGHT_BDP_DEPTH_MODE - 1];
                context_ptr->interval_cost[1] = context_ptr->cost_depth_mode[SB_BDP_DEPTH_MODE - 1];
            }

        }
        else {
            if (context_ptr->budget > (uint32_t) (picture_control_set_ptr->parent_pcs_ptr->sb_total_count * BDP_COST)) {

                context_ptr->number_of_segments = 2;

                context_ptr->score_th[0] = (int8_t)((1 * 100) / context_ptr->number_of_segments);

                context_ptr->interval_cost[0] = context_ptr->cost_depth_mode[SB_BDP_DEPTH_MODE - 1];
                context_ptr->interval_cost[1] = context_ptr->cost_depth_mode[SB_FULL84_DEPTH_MODE - 1];
            }

            else if (context_ptr->budget > (uint32_t)(picture_control_set_ptr->parent_pcs_ptr->sb_total_count * OPEN_LOOP_COST)) {

                context_ptr->number_of_segments = 4;

                context_ptr->score_th[0] = (int8_t)((1 * 100) / context_ptr->number_of_segments);
                context_ptr->score_th[1] = (int8_t)((2 * 100) / context_ptr->number_of_segments);
                context_ptr->score_th[2] = (int8_t)((3 * 100) / context_ptr->number_of_segments);

                context_ptr->interval_cost[0] = context_ptr->cost_depth_mode[SB_PRED_OPEN_LOOP_DEPTH_MODE - 1];
                context_ptr->interval_cost[1] = context_ptr->cost_depth_mode[SB_LIGHT_OPEN_LOOP_DEPTH_MODE - 1];
                context_ptr->interval_cost[2] = context_ptr->cost_depth_mode[SB_LIGHT_BDP_DEPTH_MODE - 1];
                context_ptr->interval_cost[3] = context_ptr->cost_depth_mode[SB_BDP_DEPTH_MODE - 1];
            }
            else {
                context_ptr->number_of_segments = 5;

                context_ptr->score_th[0] = (int8_t)((1 * 100) / context_ptr->number_of_segments);
                context_ptr->score_th[1] = (int8_t)((2 * 100) / context_ptr->number_of_segments);
                context_ptr->score_th[2] = (int8_t)((3 * 100) / context_ptr->number_of_segments);
                context_ptr->score_th[3] = (int8_t)((4 * 100) / context_ptr->number_of_segments);

                context_ptr->interval_cost[0] = context_ptr->cost_depth_mode[SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE - 1];
                context_ptr->interval_cost[1] = context_ptr->cost_depth_mode[SB_PRED_OPEN_LOOP_DEPTH_MODE - 1];
                context_ptr->interval_cost[2] = context_ptr->cost_depth_mode[SB_LIGHT_OPEN_LOOP_DEPTH_MODE - 1];
                context_ptr->interval_cost[3] = context_ptr->cost_depth_mode[SB_LIGHT_BDP_DEPTH_MODE - 1];
                context_ptr->interval_cost[4] = context_ptr->cost_depth_mode[SB_BDP_DEPTH_MODE - 1];
            }

        }
    }
    else {

        if (context_ptr->budget > (uint32_t)(picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_120)) {

            context_ptr->number_of_segments = 6;

            context_ptr->score_th[0] = (int8_t)((1 * 100) / context_ptr->number_of_segments);
            context_ptr->score_th[1] = (int8_t)((2 * 100) / context_ptr->number_of_segments);
            context_ptr->score_th[2] = (int8_t)((3 * 100) / context_ptr->number_of_segments);
            context_ptr->score_th[3] = (int8_t)((4 * 100) / context_ptr->number_of_segments);
            context_ptr->score_th[4] = (int8_t)((5 * 100) / context_ptr->number_of_segments);

            context_ptr->interval_cost[0] = context_ptr->cost_depth_mode[SB_PRED_OPEN_LOOP_DEPTH_MODE - 1];
            context_ptr->interval_cost[1] = context_ptr->cost_depth_mode[SB_LIGHT_OPEN_LOOP_DEPTH_MODE - 1];
            context_ptr->interval_cost[2] = context_ptr->cost_depth_mode[SB_OPEN_LOOP_DEPTH_MODE - 1];
            context_ptr->interval_cost[3] = context_ptr->cost_depth_mode[SB_LIGHT_BDP_DEPTH_MODE - 1];
            context_ptr->interval_cost[4] = context_ptr->cost_depth_mode[SB_BDP_DEPTH_MODE - 1];
            context_ptr->interval_cost[5] = context_ptr->cost_depth_mode[SB_FULL85_DEPTH_MODE - 1];
        }
        else if (context_ptr->budget > (uint32_t)(picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_115)) {

                context_ptr->number_of_segments = 5;

                context_ptr->score_th[0] = (int8_t)((1 * 100) / context_ptr->number_of_segments);
                context_ptr->score_th[1] = (int8_t)((2 * 100) / context_ptr->number_of_segments);
                context_ptr->score_th[2] = (int8_t)((3 * 100) / context_ptr->number_of_segments);
                context_ptr->score_th[3] = (int8_t)((4 * 100) / context_ptr->number_of_segments);

                context_ptr->interval_cost[0] = context_ptr->cost_depth_mode[SB_PRED_OPEN_LOOP_DEPTH_MODE - 1];
                context_ptr->interval_cost[1] = context_ptr->cost_depth_mode[SB_LIGHT_OPEN_LOOP_DEPTH_MODE - 1];
                context_ptr->interval_cost[2] = context_ptr->cost_depth_mode[SB_OPEN_LOOP_DEPTH_MODE - 1];
                context_ptr->interval_cost[3] = context_ptr->cost_depth_mode[SB_LIGHT_BDP_DEPTH_MODE - 1];
                context_ptr->interval_cost[4] = context_ptr->cost_depth_mode[SB_BDP_DEPTH_MODE - 1];

        } else if (context_ptr->budget > (uint32_t)(picture_control_set_ptr->parent_pcs_ptr->sb_total_count * OPEN_LOOP_COST)) {

            context_ptr->number_of_segments = 4;

            context_ptr->score_th[0] = (int8_t)((1 * 100) / context_ptr->number_of_segments);
            context_ptr->score_th[1] = (int8_t)((2 * 100) / context_ptr->number_of_segments);
            context_ptr->score_th[2] = (int8_t)((3 * 100) / context_ptr->number_of_segments);

            context_ptr->interval_cost[0] = context_ptr->cost_depth_mode[SB_PRED_OPEN_LOOP_DEPTH_MODE - 1];
            context_ptr->interval_cost[1] = context_ptr->cost_depth_mode[SB_LIGHT_OPEN_LOOP_DEPTH_MODE - 1];
            context_ptr->interval_cost[2] = context_ptr->cost_depth_mode[SB_OPEN_LOOP_DEPTH_MODE - 1];
            context_ptr->interval_cost[3] = context_ptr->cost_depth_mode[SB_LIGHT_BDP_DEPTH_MODE - 1];

        } else {

            context_ptr->number_of_segments = 4;

            context_ptr->score_th[0] = (int8_t)((1 * 100) / context_ptr->number_of_segments);
            context_ptr->score_th[1] = (int8_t)((2 * 100) / context_ptr->number_of_segments);
            context_ptr->score_th[2] = (int8_t)((3 * 100) / context_ptr->number_of_segments);

            context_ptr->interval_cost[0] = context_ptr->cost_depth_mode[SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE - 1];
            context_ptr->interval_cost[1] = context_ptr->cost_depth_mode[SB_PRED_OPEN_LOOP_DEPTH_MODE - 1];
            context_ptr->interval_cost[2] = context_ptr->cost_depth_mode[SB_LIGHT_OPEN_LOOP_DEPTH_MODE - 1];
            context_ptr->interval_cost[3] = context_ptr->cost_depth_mode[SB_OPEN_LOOP_DEPTH_MODE - 1];
        }
    }

    return return_error;
}

/******************************************************
* Set the target budget
    Input   : cost per depth
    Output  : budget per picture
******************************************************/

void SetTargetBudgetSq(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{
    uint32_t budget;

    if (context_ptr->adp_level <= ENC_MODE_4) {
        if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080i_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = (picture_control_set_ptr->parent_pcs_ptr->is_pan || picture_control_set_ptr->parent_pcs_ptr->is_tilt) ?
                picture_control_set_ptr->sb_total_count * U_152 :
                picture_control_set_ptr->sb_total_count * U_150;
            else
                budget = (picture_control_set_ptr->parent_pcs_ptr->is_pan || picture_control_set_ptr->parent_pcs_ptr->is_tilt) ?
                picture_control_set_ptr->sb_total_count * U_152 :
                picture_control_set_ptr->sb_total_count * U_145;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080p_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * AVC_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_134;
        }
        else {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_132;
            else
                budget = picture_control_set_ptr->sb_total_count * U_121;

        }
    }
    else if (context_ptr->adp_level == ENC_MODE_5) {
        if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080i_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = (picture_control_set_ptr->parent_pcs_ptr->is_pan || picture_control_set_ptr->parent_pcs_ptr->is_tilt) ?
                picture_control_set_ptr->sb_total_count * U_152 :
                picture_control_set_ptr->sb_total_count * U_150;
            else
                budget = (picture_control_set_ptr->parent_pcs_ptr->is_pan || picture_control_set_ptr->parent_pcs_ptr->is_tilt) ?
                picture_control_set_ptr->sb_total_count * U_152 :
                picture_control_set_ptr->sb_total_count * U_145;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080p_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * AVC_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_134;

        }
        else {

            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_125;
            else
                budget = picture_control_set_ptr->sb_total_count * U_121;

        }

    }

    else if (context_ptr->adp_level == ENC_MODE_6) {
        if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_133;
            else
                budget = picture_control_set_ptr->sb_total_count * U_120;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080i_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * BDP_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_113;
            else
                budget = picture_control_set_ptr->sb_total_count * U_112;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080p_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_130;
            else
                budget = picture_control_set_ptr->sb_total_count * U_120;
        }
        else {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_125;
            else
                budget = picture_control_set_ptr->sb_total_count * U_115;

        }

    }
    else if (context_ptr->adp_level == ENC_MODE_7) { // Adam
        if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_133;
            else
                budget = picture_control_set_ptr->sb_total_count * U_120;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080i_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * BDP_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_113;
            else
                budget = picture_control_set_ptr->sb_total_count * U_112;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080p_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_130;
            else
                budget = picture_control_set_ptr->sb_total_count * U_120;
        }
        else {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_125;
            else
                budget = picture_control_set_ptr->sb_total_count * U_115;

        }
    }
    else if (context_ptr->adp_level == ENC_MODE_8) {
        if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_115;
            else
                budget = picture_control_set_ptr->sb_total_count * U_114;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080i_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * BDP_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_112;
            else
                budget = picture_control_set_ptr->sb_total_count * U_111;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080p_RANGE) {
            if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * BDP_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_116;
            else
                budget = picture_control_set_ptr->sb_total_count * OPEN_LOOP_COST;
        }
        else {
            if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * AVC_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * LIGHT_BDP_COST;
            else
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_111;
        }
    }
    else if (context_ptr->adp_level == ENC_MODE_9) {
        if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_576p_RANGE_OR_LOWER) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * BDP_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_112;
            else
                budget = picture_control_set_ptr->sb_total_count * U_111;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080i_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * U_127;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * OPEN_LOOP_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_101;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080p_RANGE) {
            if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * BDP_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_113;
            else
                budget = picture_control_set_ptr->sb_total_count * OPEN_LOOP_COST;
        }
        else {
            if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_127;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_114;
            else
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * OPEN_LOOP_COST;

        }
    }
    else if (context_ptr->adp_level == ENC_MODE_10) {
        if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080i_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * U_127;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * OPEN_LOOP_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_101;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080p_RANGE) {
            if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * BDP_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * OPEN_LOOP_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_104;
        }
        else {
            if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0)
                budget = (context_ptr->adp_depth_sensitive_picture_class) ?
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_127 :
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_125;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * OPEN_LOOP_COST;
            else
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * LIGHT_OPEN_LOOP_COST;

        }
    }
    else if (context_ptr->adp_level == ENC_MODE_11) {
        if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080i_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * U_127;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * OPEN_LOOP_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_101;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080p_RANGE) {

            if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * BDP_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * OPEN_LOOP_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_104;

        }
        else {
            if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0)
                budget = (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_2) ?
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_127 :
                (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_1) ?
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_125 :
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_121;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag)
                budget = (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_2) ?
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * OPEN_LOOP_COST :
                (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_1) ?
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_104 :
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_103;
            else
                budget = (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_2) ?
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_104 :
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_103;
        }
    }
    else {
        if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0)
            budget = (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_2) ?
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_127 :
            (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_1) ?
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_125 :
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_121;
        else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag)
            budget = (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_2) ?
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * OPEN_LOOP_COST :
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * PRED_OPEN_LOOP_COST ;
        else
            budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * PRED_OPEN_LOOP_COST;
    }
    context_ptr->budget = budget;
}

/******************************************************
* Set the target budget
Input   : cost per depth
Output  : budget per picture
******************************************************/

void eb_vp9_set_target_budget_oq(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{
    uint32_t budget;

    if (context_ptr->adp_level <= ENC_MODE_3) {
        if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080i_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = (picture_control_set_ptr->parent_pcs_ptr->is_pan || picture_control_set_ptr->parent_pcs_ptr->is_tilt) ?
                picture_control_set_ptr->sb_total_count * U_152 :
                picture_control_set_ptr->sb_total_count * U_150;
            else
                budget = (picture_control_set_ptr->parent_pcs_ptr->is_pan || picture_control_set_ptr->parent_pcs_ptr->is_tilt) ?
                picture_control_set_ptr->sb_total_count * U_152 :
                picture_control_set_ptr->sb_total_count * U_145;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080p_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * AVC_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_134;
        }
        else {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * BDP_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * OPEN_LOOP_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_109;
        }
    }
    else if (context_ptr->adp_level <= ENC_MODE_4) {
        if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080i_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = (picture_control_set_ptr->parent_pcs_ptr->is_pan || picture_control_set_ptr->parent_pcs_ptr->is_tilt) ?
                picture_control_set_ptr->sb_total_count * U_152 :
                picture_control_set_ptr->sb_total_count * U_150;
            else
                budget = (picture_control_set_ptr->parent_pcs_ptr->is_pan || picture_control_set_ptr->parent_pcs_ptr->is_tilt) ?
                picture_control_set_ptr->sb_total_count * U_152 :
                picture_control_set_ptr->sb_total_count * U_145;
        }
        else if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080p_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * AVC_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_134;
        }
        else {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * U_125;
            else
                budget = picture_control_set_ptr->sb_total_count * U_121;
        }
    }
    else if (context_ptr->adp_level <= ENC_MODE_5) {
        if (picture_control_set_ptr->temporal_layer_index == 0)
            budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
        else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
            budget = picture_control_set_ptr->sb_total_count * AVC_COST;
        else
            budget = picture_control_set_ptr->sb_total_count * U_109;
    }
    else if (context_ptr->adp_level <= ENC_MODE_6) {
        if (picture_control_set_ptr->temporal_layer_index == 0)
            budget = picture_control_set_ptr->sb_total_count * BDP_COST;
        else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
            budget = picture_control_set_ptr->sb_total_count * OPEN_LOOP_COST;
        else
            budget = picture_control_set_ptr->sb_total_count * U_109;
    }
    else if (context_ptr->adp_level <= ENC_MODE_7) {
        if (sequence_control_set_ptr->input_resolution == INPUT_SIZE_4K_RANGE) {
            if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0)
                budget = (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_2) ?
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_127 :
                (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_1) ?
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_125 :
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_121;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag)
                budget = (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_2) ?
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * OPEN_LOOP_COST :
                picture_control_set_ptr->parent_pcs_ptr->sb_total_count * PRED_OPEN_LOOP_COST ;
            else
                budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * PRED_OPEN_LOOP_COST;
        }
        else {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * BDP_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * OPEN_LOOP_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_109;
        }
    }
    else {
        if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0)
            budget = (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_2) ?
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_127 :
            (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_1) ?
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_125 :
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_121 ;
        else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag)
            budget = (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_2) ?
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * OPEN_LOOP_COST :
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * PRED_OPEN_LOOP_COST;
        else
            budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * PRED_OPEN_LOOP_COST;
    }

    context_ptr->budget = budget;
}

/******************************************************
* Set the target budget
Input   : cost per depth
Output  : budget per picture
******************************************************/

void set_target_budget_vmaf(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{
    uint32_t budget;

    if (context_ptr->adp_level <= ENC_MODE_3) {
        if (picture_control_set_ptr->temporal_layer_index == 0)
            budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
        else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
            budget = picture_control_set_ptr->sb_total_count * AVC_COST;
        else
            budget = picture_control_set_ptr->sb_total_count * U_134;
    }
    else if (context_ptr->adp_level <= ENC_MODE_5) {
        if (sequence_control_set_ptr->input_resolution == INPUT_SIZE_4K_RANGE) {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * BDP_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * OPEN_LOOP_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_109;
        }
        else {
            if (picture_control_set_ptr->temporal_layer_index == 0)
                budget = picture_control_set_ptr->sb_total_count * FULL_SEARCH_COST;
            else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
                budget = picture_control_set_ptr->sb_total_count * AVC_COST;
            else
                budget = picture_control_set_ptr->sb_total_count * U_134;
        }
    }
    else if (context_ptr->adp_level <= ENC_MODE_8) {
        if (picture_control_set_ptr->temporal_layer_index == 0)
            budget = picture_control_set_ptr->sb_total_count * BDP_COST;
        else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE)
            budget = picture_control_set_ptr->sb_total_count * OPEN_LOOP_COST;
        else
            budget = picture_control_set_ptr->sb_total_count * U_109;
    }
    else {
        if (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0)
            budget = (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_2) ?
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_127 :
            (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_1) ?
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_125 :
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * U_121;
        else if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag)
            budget = (context_ptr->adp_depth_sensitive_picture_class == DEPTH_SENSITIVE_PIC_CLASS_2) ?
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * OPEN_LOOP_COST :
            picture_control_set_ptr->parent_pcs_ptr->sb_total_count * PRED_OPEN_LOOP_COST ;
        else
            budget = picture_control_set_ptr->parent_pcs_ptr->sb_total_count * PRED_OPEN_LOOP_COST;
    }

    context_ptr->budget = budget;
}

/******************************************************
 * is_avc_partitioning_mode()
 * Returns TRUE for LCUs where only Depth2 & Depth3
 * (AVC Partitioning) are goind to be tested by MD
 * The LCU is marked if Sharpe Edge or Potential Aura/Grass
 * or B-Logo or S-Logo or Potential Blockiness Area
 * Input: Sharpe Edge, Potential Aura/Grass, B-Logo, S-Logo, Potential Blockiness Area signals
 * Output: TRUE if one of the above is TRUE
 ******************************************************/
EB_BOOL is_avc_partitioning_mode(
    SequenceControlSet *sequence_control_set_ptr,
    PictureControlSet  *picture_control_set_ptr,
    SbUnit             *sb_ptr)
{
    const uint32_t sb_index = sb_ptr->sb_index;
    SbParams      *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
    EB_SLICE       slice_type = picture_control_set_ptr->slice_type;
    uint8_t        edge_block_num = picture_control_set_ptr->parent_pcs_ptr->edge_results_ptr[sb_index].edge_block_num;
    SbStat        *sb_stat_ptr = &(picture_control_set_ptr->parent_pcs_ptr->sb_stat_array[sb_index]);
    uint8_t        stationary_edge_over_time_flag = sb_stat_ptr->stationary_edge_over_time_flag;
    uint8_t        aura_status = sb_ptr->aura_status;

    if (sequence_control_set_ptr->static_config.tune == TUNE_SQ) {
        // No refinment for sub-1080p
        if (sequence_control_set_ptr->input_resolution <= INPUT_SIZE_1080i_RANGE)
            return EB_FALSE;

        // Sharpe Edge
        if (picture_control_set_ptr->parent_pcs_ptr->high_dark_low_light_area_density_flag && picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index > 0 && picture_control_set_ptr->parent_pcs_ptr->sharp_edge_sb_flag[sb_index] && !picture_control_set_ptr->parent_pcs_ptr->similar_colocated_sb_array_all_layers[sb_index]) {
            return EB_TRUE;
        }

        // Potential Aura/Grass
        if (picture_control_set_ptr->scene_characteristic_id == EB_FRAME_CARAC_0) {
            if (picture_control_set_ptr->parent_pcs_ptr->grass_percentage_in_picture > 60 && aura_status == AURA_STATUS_1) {
                if (slice_type != I_SLICE && sb_params->is_complete_sb) {
                    return EB_TRUE;
                }
            }
        }

        // B-Logo
        if (picture_control_set_ptr->parent_pcs_ptr->logo_pic_flag && edge_block_num)
            return EB_TRUE;

        // S-Logo
        if (stationary_edge_over_time_flag > 0)
            return EB_TRUE;

        // Potential Blockiness Area
        if (picture_control_set_ptr->parent_pcs_ptr->complex_sb_array[sb_index] == SB_COMPLEXITY_STATUS_2)
            return EB_TRUE;
    }

    return EB_FALSE;
}

/******************************************************
* Load the cost of the different partitioning method into a local array and derive sensitive picture flag
    Input   : the offline derived cost per search method, detection signals
    Output  : valid cost_depth_mode and valid sensitivePicture
******************************************************/
void eb_vp9_configure_adp(
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{

    context_ptr->cost_depth_mode[SB_FULL85_DEPTH_MODE - 1]               = FULL_SEARCH_COST;
    context_ptr->cost_depth_mode[SB_FULL84_DEPTH_MODE - 1]               = FULL_SEARCH_COST;
    context_ptr->cost_depth_mode[SB_BDP_DEPTH_MODE - 1]                  = BDP_COST;
    context_ptr->cost_depth_mode[SB_LIGHT_BDP_DEPTH_MODE - 1]            = LIGHT_BDP_COST;
    context_ptr->cost_depth_mode[SB_OPEN_LOOP_DEPTH_MODE - 1]            = OPEN_LOOP_COST;
    context_ptr->cost_depth_mode[SB_LIGHT_OPEN_LOOP_DEPTH_MODE - 1]      = LIGHT_OPEN_LOOP_COST;
    context_ptr->cost_depth_mode[SB_AVC_DEPTH_MODE - 1]                  = AVC_COST;
    context_ptr->cost_depth_mode[SB_LIGHT_AVC_DEPTH_MODE - 1]            = LIGHT_AVC_COST;
    context_ptr->cost_depth_mode[SB_PRED_OPEN_LOOP_DEPTH_MODE - 1]       = PRED_OPEN_LOOP_COST;
    context_ptr->cost_depth_mode[SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE - 1] = PRED_OPEN_LOOP_1_NFL_COST;

    // Initialize the score based TH
    context_ptr->score_th[0] = ~0;
    context_ptr->score_th[1] = ~0;
    context_ptr->score_th[2] = ~0;
    context_ptr->score_th[3] = ~0;
    context_ptr->score_th[4] = ~0;
    context_ptr->score_th[5] = ~0;
    context_ptr->score_th[6] = ~0;

    // Initialize the predicted budget
    context_ptr->predicted_cost = (uint32_t) ~0;

    // Initialize the predicted budget
    context_ptr->predicted_cost = (uint32_t)~0;

    // Derive the sensitive picture flag
    context_ptr->adp_depth_sensitive_picture_class = DEPTH_SENSITIVE_PIC_CLASS_0;

    EB_BOOL luminosity_change = EB_FALSE;
    // Derived for REF P & B & kept false otherwise (for temporal distance equal to 1 luminosity changes are easier to handle)
    // Derived for P & B
    if (picture_control_set_ptr->slice_type != I_SLICE) {
        if (picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag) {
            EbReferenceObject    * ref_obj_l0, *ref_obj_l1;
            ref_obj_l0 = (EbReferenceObject  *)picture_control_set_ptr->ref_pic_ptr_array[REF_LIST_0]->object_ptr;
            ref_obj_l1 = (picture_control_set_ptr->parent_pcs_ptr->slice_type == B_SLICE) ? (EbReferenceObject  *)picture_control_set_ptr->ref_pic_ptr_array[REF_LIST_1]->object_ptr : (EbReferenceObject  *)EB_NULL;
            luminosity_change = ((ABS(picture_control_set_ptr->parent_pcs_ptr->average_intensity[0] - ref_obj_l0->average_intensity) >= adp_luminosity_change_th_array[picture_control_set_ptr->parent_pcs_ptr->hierarchical_levels][picture_control_set_ptr->temporal_layer_index]) || (ref_obj_l1 != EB_NULL && ABS(picture_control_set_ptr->parent_pcs_ptr->average_intensity[0] - ref_obj_l1->average_intensity) >= adp_luminosity_change_th_array[picture_control_set_ptr->parent_pcs_ptr->hierarchical_levels][picture_control_set_ptr->temporal_layer_index]));
        }
    }

    if (picture_control_set_ptr->parent_pcs_ptr->non_moving_average_score != INVALID_NON_MOVING_SCORE && picture_control_set_ptr->parent_pcs_ptr->non_moving_average_score < ADP_CONFIG_NON_MOVING_INDEX_TH_1) { // could not seen by the eye if very active
        if (picture_control_set_ptr->parent_pcs_ptr->pic_noise_class > PIC_NOISE_CLASS_3 || picture_control_set_ptr->parent_pcs_ptr->high_dark_low_light_area_density_flag ||luminosity_change) { // potential complex picture: luminosity Change (e.g. fade, light..)
            context_ptr->adp_depth_sensitive_picture_class = DEPTH_SENSITIVE_PIC_CLASS_2;
        }
        // potential complex picture: light foreground and dark background(e.g.flash, light..) or moderate activity and high variance (noise or a lot of edge)
        else if ( (picture_control_set_ptr->parent_pcs_ptr->non_moving_average_score >= ADP_CONFIG_NON_MOVING_INDEX_TH_0 && picture_control_set_ptr->parent_pcs_ptr->pic_noise_class == PIC_NOISE_CLASS_3)) {
            context_ptr->adp_depth_sensitive_picture_class = DEPTH_SENSITIVE_PIC_CLASS_1;
        }
    }

}

/******************************************************
* Assign a search method based on the allocated cost
    Input   : allocated budget per LCU
    Output  : search method per LCU
******************************************************/
void eb_vp9_derive_search_method(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{

    uint32_t sb_index;

    picture_control_set_ptr->bdp_present_flag = EB_FALSE;
    picture_control_set_ptr->md_present_flag  = EB_FALSE;

    for (sb_index = 0; sb_index < picture_control_set_ptr->parent_pcs_ptr->sb_total_count; sb_index++) {

        if (context_ptr->sb_cost_array[sb_index] == context_ptr->cost_depth_mode[SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE - 1]) {
            picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] = SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE;
            picture_control_set_ptr->md_present_flag = EB_TRUE;
        }
        else
        if (context_ptr->sb_cost_array[sb_index] == context_ptr->cost_depth_mode[SB_PRED_OPEN_LOOP_DEPTH_MODE - 1]) {
            picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] = SB_PRED_OPEN_LOOP_DEPTH_MODE;
            picture_control_set_ptr->md_present_flag  = EB_TRUE;
        } else if (context_ptr->sb_cost_array[sb_index] == context_ptr->cost_depth_mode[SB_LIGHT_OPEN_LOOP_DEPTH_MODE - 1]) {
            picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] = SB_LIGHT_OPEN_LOOP_DEPTH_MODE;
            picture_control_set_ptr->md_present_flag  = EB_TRUE;
        } else if (context_ptr->sb_cost_array[sb_index] == context_ptr->cost_depth_mode[SB_OPEN_LOOP_DEPTH_MODE - 1]) {
            picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] = SB_OPEN_LOOP_DEPTH_MODE;
            picture_control_set_ptr->md_present_flag  = EB_TRUE;
        } else if (context_ptr->sb_cost_array[sb_index] == context_ptr->cost_depth_mode[SB_LIGHT_BDP_DEPTH_MODE - 1]) {
            picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] = SB_LIGHT_BDP_DEPTH_MODE;
            picture_control_set_ptr->bdp_present_flag = EB_TRUE;
        } else if (context_ptr->sb_cost_array[sb_index] == context_ptr->cost_depth_mode[SB_BDP_DEPTH_MODE - 1]) {
            picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] = SB_BDP_DEPTH_MODE;
            picture_control_set_ptr->bdp_present_flag = EB_TRUE;
        } else if (context_ptr->sb_cost_array[sb_index] == context_ptr->cost_depth_mode[SB_AVC_DEPTH_MODE - 1]) {
            picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] = SB_AVC_DEPTH_MODE;
            picture_control_set_ptr->md_present_flag  = EB_TRUE;
        } else if (context_ptr->sb_cost_array[sb_index] == context_ptr->cost_depth_mode[SB_LIGHT_AVC_DEPTH_MODE - 1]) {
            picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] = SB_LIGHT_AVC_DEPTH_MODE;
            picture_control_set_ptr->md_present_flag = EB_TRUE;
#if SHUT_64x64_BASE_RESTRICTION
        } else if (picture_control_set_ptr->temporal_layer_index == 0 && (sequence_control_set_ptr->static_config.tune == TUNE_SQ || sequence_control_set_ptr->static_config.tune == TUNE_VMAF)) {
            picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] = SB_FULL84_DEPTH_MODE;
            picture_control_set_ptr->md_present_flag = EB_TRUE;
#else
        } else if (picture_control_set_ptr->temporal_layer_index == 0) {
            picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] = SB_FULL84_DEPTH_MODE;
            picture_control_set_ptr->md_present_flag  = EB_TRUE;
#endif
        } else {
            picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] = SB_FULL85_DEPTH_MODE;
            picture_control_set_ptr->md_present_flag  = EB_TRUE;
        }

    }

#if ADP_STATS_PER_LAYER
    SequenceControlSet *sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->parent_pcs_ptr->sequence_control_set_wrapper_ptr->object_ptr;

    for (sb_index = 0; sb_index < picture_control_set_ptr->parent_pcs_ptr->sb_total_count; sb_index++) {

        sequence_control_set_ptr->total_count[picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index] ++;

        if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_FULL85_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_FULL84_DEPTH_MODE) {
            sequence_control_set_ptr->fs_count[picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index]  ++;
        }
        else if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_BDP_DEPTH_MODE) {
            sequence_control_set_ptr->f_bdp_count[picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index]  ++;
        }
        else if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_LIGHT_BDP_DEPTH_MODE) {
            sequence_control_set_ptr->l_bdp_count[picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index]  ++;
        }
        else if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_OPEN_LOOP_DEPTH_MODE) {
            sequence_control_set_ptr->f_mdc_count[picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index]  ++;
        }
        else if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_LIGHT_OPEN_LOOP_DEPTH_MODE) {
            sequence_control_set_ptr->l_mdc_count[picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index]  ++;
        }
        else if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_AVC_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_LIGHT_AVC_DEPTH_MODE) {
            sequence_control_set_ptr->avc_count[picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index]  ++;
        }
        else if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_PRED_OPEN_LOOP_DEPTH_MODE) {
            sequence_control_set_ptr->pred_count[picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index]  ++;
        }
        else if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE) {
            sequence_control_set_ptr->pred1_nfl_count[picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index]  ++;
        }
        else
        {
            SVT_LOG("error");
        }
    }
#endif

}

/******************************************************
* Set LCU budget
    Input   : LCU score, detection signals, iteration
    Output  : predicted budget for the LCU
******************************************************/
void eb_vp9_set_sb_budget(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    SbUnit                           *sb_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{
    const uint32_t      sb_index = sb_ptr->sb_index;
    uint32_t      max_to_min_score, score_to_min;

    const EB_BOOL is_avc_partitioning_mode_flag = is_avc_partitioning_mode(sequence_control_set_ptr, picture_control_set_ptr, sb_ptr);

    if (context_ptr->adp_refinement_mode == 2 && is_avc_partitioning_mode_flag) {

        context_ptr->sb_cost_array[sb_index] = context_ptr->cost_depth_mode[SB_AVC_DEPTH_MODE - 1];
        context_ptr->predicted_cost += context_ptr->cost_depth_mode[SB_AVC_DEPTH_MODE - 1];

    }
    else if (context_ptr->adp_refinement_mode == 1 && is_avc_partitioning_mode_flag) {

        context_ptr->sb_cost_array[sb_index] = context_ptr->cost_depth_mode[SB_LIGHT_AVC_DEPTH_MODE - 1];
        context_ptr->predicted_cost += context_ptr->cost_depth_mode[SB_LIGHT_AVC_DEPTH_MODE - 1];

    }
    else {
        context_ptr->sb_score_array[sb_index] = CLIP3(context_ptr->sb_min_score, context_ptr->sb_max_score, context_ptr->sb_score_array[sb_index]);
        score_to_min = context_ptr->sb_score_array[sb_index] - context_ptr->sb_min_score;
        max_to_min_score = context_ptr->sb_max_score - context_ptr->sb_min_score;

        if ((score_to_min <= (max_to_min_score * context_ptr->score_th[0]) / 100 && context_ptr->score_th[0] != 0) || context_ptr->number_of_segments == 1 || context_ptr->score_th[1] == 100) {
            context_ptr->sb_cost_array[sb_index] = context_ptr->interval_cost[0];
            context_ptr->predicted_cost += context_ptr->interval_cost[0];
        }
        else if ((score_to_min <= (max_to_min_score * context_ptr->score_th[1]) / 100 && context_ptr->score_th[1] != 0) || context_ptr->number_of_segments == 2 || context_ptr->score_th[2] == 100) {
            context_ptr->sb_cost_array[sb_index] = context_ptr->interval_cost[1];
            context_ptr->predicted_cost += context_ptr->interval_cost[1];
        }
        else if ((score_to_min <= (max_to_min_score * context_ptr->score_th[2]) / 100 && context_ptr->score_th[2] != 0) || context_ptr->number_of_segments == 3 || context_ptr->score_th[3] == 100) {
            context_ptr->sb_cost_array[sb_index] = context_ptr->interval_cost[2];
            context_ptr->predicted_cost += context_ptr->interval_cost[2];
        }
        else if ((score_to_min <= (max_to_min_score * context_ptr->score_th[3]) / 100 && context_ptr->score_th[3] != 0) || context_ptr->number_of_segments == 4 || context_ptr->score_th[4] == 100) {
            context_ptr->sb_cost_array[sb_index] = context_ptr->interval_cost[3];
            context_ptr->predicted_cost += context_ptr->interval_cost[3];
        }
        else if ((score_to_min <= (max_to_min_score * context_ptr->score_th[4]) / 100 && context_ptr->score_th[4] != 0) || context_ptr->number_of_segments == 5 || context_ptr->score_th[5] == 100) {
            context_ptr->sb_cost_array[sb_index] = context_ptr->interval_cost[4];
            context_ptr->predicted_cost += context_ptr->interval_cost[4];
        }
        else if ((score_to_min <= (max_to_min_score * context_ptr->score_th[5]) / 100 && context_ptr->score_th[5] != 0) || context_ptr->number_of_segments == 6 || context_ptr->score_th[6] == 100) {
            context_ptr->sb_cost_array[sb_index] = context_ptr->interval_cost[5];
            context_ptr->predicted_cost += context_ptr->interval_cost[5];
        }
        else {
            context_ptr->sb_cost_array[sb_index] = context_ptr->interval_cost[6];
            context_ptr->predicted_cost += context_ptr->interval_cost[6];
        }
        // Switch to AVC mode if the LCU cost is higher than the AVC cost and the the LCU is marked + adjust the current picture cost accordingly
        if (is_avc_partitioning_mode_flag) {
            if (context_ptr->sb_cost_array[sb_index] > context_ptr->cost_depth_mode[SB_AVC_DEPTH_MODE - 1]) {
                context_ptr->predicted_cost -= (context_ptr->sb_cost_array[sb_index] - context_ptr->cost_depth_mode[SB_AVC_DEPTH_MODE - 1]);
                context_ptr->sb_cost_array[sb_index] = context_ptr->cost_depth_mode[SB_AVC_DEPTH_MODE - 1];
            }
            else if (context_ptr->sb_cost_array[sb_index] > context_ptr->cost_depth_mode[SB_LIGHT_AVC_DEPTH_MODE - 1] && picture_control_set_ptr->temporal_layer_index > 0) {
                context_ptr->predicted_cost -= (context_ptr->sb_cost_array[sb_index] - context_ptr->cost_depth_mode[SB_LIGHT_AVC_DEPTH_MODE - 1]);
                context_ptr->sb_cost_array[sb_index] = context_ptr->cost_depth_mode[SB_LIGHT_AVC_DEPTH_MODE - 1];
            }
        }
    }
}

/******************************************************
* Loop multiple times over the LCUs in order to derive the optimal budget per LCU
    Input   : budget per picture, ditortion, detection signals, iteration
    Output  : optimal budget for each LCU
******************************************************/
void  eb_vp9_derive_optimal_budget_per_sb(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{
    uint32_t sb_index;
    // Initialize the deviation between the picture predicted cost & the target budget to 100,
    uint32_t deviation_to_target    = 1000;

    // Set the adjustment step to 1 (could be increased for faster convergence),
    int8_t  adjustement_step      =  1;

    // Set the initial shooting state & the final shooting state to TBD
    uint32_t initial_shooting  = TBD_SHOOTING;
    uint32_t final_shooting    = TBD_SHOOTING;

    uint8_t max_adjustement_iteration   = 100;
    uint8_t adjustement_iteration      =   0;

    while (deviation_to_target != 0 && (initial_shooting == final_shooting) && adjustement_iteration <= max_adjustement_iteration) {

        if (context_ptr->predicted_cost < context_ptr->budget) {
            initial_shooting =    UNDER_SHOOTING;
        }
        else {
            initial_shooting =   OVER_SHOOTING;
        }

        // reset running cost
        context_ptr->predicted_cost = 0;

        for (sb_index = 0; sb_index < picture_control_set_ptr->parent_pcs_ptr->sb_total_count; sb_index++) {

            SbUnit* sb_ptr = picture_control_set_ptr->sb_ptr_array[sb_index];

            eb_vp9_set_sb_budget(
                sequence_control_set_ptr,
                picture_control_set_ptr,
                sb_ptr,
                context_ptr);
        }

        // Compute the deviation between the predicted budget & the target budget
        deviation_to_target = (ABS((int32_t)(context_ptr->predicted_cost - context_ptr->budget)) * 1000) / context_ptr->budget;
        // Derive shooting status
        if (context_ptr->predicted_cost < context_ptr->budget) {
            context_ptr->score_th[0] = MAX((context_ptr->score_th[0] - adjustement_step), 0);
            context_ptr->score_th[1] = MAX((context_ptr->score_th[1] - adjustement_step), 0);
            context_ptr->score_th[2] = MAX((context_ptr->score_th[2] - adjustement_step), 0);
            context_ptr->score_th[3] = MAX((context_ptr->score_th[3] - adjustement_step), 0);
            context_ptr->score_th[4] = MAX((context_ptr->score_th[4] - adjustement_step), 0);
            final_shooting = UNDER_SHOOTING;
        }
        else {
            context_ptr->score_th[0] = (context_ptr->score_th[0] == 0) ? 0 : MIN(context_ptr->score_th[0] + adjustement_step, 100);
            context_ptr->score_th[1] = (context_ptr->score_th[1] == 0) ? 0 : MIN(context_ptr->score_th[1] + adjustement_step, 100);
            context_ptr->score_th[2] = (context_ptr->score_th[2] == 0) ? 0 : MIN(context_ptr->score_th[2] + adjustement_step, 100);
            context_ptr->score_th[3] = (context_ptr->score_th[3] == 0) ? 0 : MIN(context_ptr->score_th[3] + adjustement_step, 100);
            context_ptr->score_th[4] = (context_ptr->score_th[4] == 0) ? 0 : MIN(context_ptr->score_th[4] + adjustement_step, 100);
            final_shooting = OVER_SHOOTING;
        }

        if (adjustement_iteration == 0)
            initial_shooting = final_shooting ;

        adjustement_iteration ++;
    }
}

/******************************************************
* Compute the refinment cost
    Input   : budget per picture, and the cost of the refinment
    Output  : the refinment flag
******************************************************/
void compute_refinement_cost(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{

    uint32_t  sb_index;
    uint32_t  avc_refinement_cost = 0;
    uint32_t  light_avc_refinement_cost = 0;

    for (sb_index = 0; sb_index < picture_control_set_ptr->parent_pcs_ptr->sb_total_count; sb_index++) {
        SbUnit* sb_ptr = picture_control_set_ptr->sb_ptr_array[sb_index];
        if (is_avc_partitioning_mode(sequence_control_set_ptr, picture_control_set_ptr, sb_ptr)) {
            avc_refinement_cost       += context_ptr->cost_depth_mode[SB_AVC_DEPTH_MODE - 1];
            light_avc_refinement_cost  += context_ptr->cost_depth_mode[SB_LIGHT_AVC_DEPTH_MODE - 1];

        }
        // assumes the fastest mode will be used otherwise
        else {
            avc_refinement_cost       += context_ptr->interval_cost[0];
            light_avc_refinement_cost  += context_ptr->interval_cost[0];
        }
    }

    // Shut the refinement if the refinement cost is higher than the allocated budget
    if (avc_refinement_cost <= context_ptr->budget && ((context_ptr->budget > (uint32_t)(LIGHT_BDP_COST * picture_control_set_ptr->parent_pcs_ptr->sb_total_count)) || picture_control_set_ptr->temporal_layer_index == 0 || (sequence_control_set_ptr->input_resolution < INPUT_SIZE_4K_RANGE && picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag == EB_TRUE))) {
        context_ptr->adp_refinement_mode = ADP_MODE_1;
    }
    else if (light_avc_refinement_cost <= context_ptr->budget && picture_control_set_ptr->temporal_layer_index > 0) {
        context_ptr->adp_refinement_mode = ADP_MODE_0;
    }
    else {
        context_ptr->adp_refinement_mode = ADP_REFINEMENT_OFF;
    }

}
/******************************************************
* Compute the score of each LCU
    Input   : distortion, detection signals
    Output  : LCU score
******************************************************/
void eb_vp9_derive_sb_score(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{
    uint32_t  sb_index;
    uint32_t  sb_score = 0;
    uint32_t  distortion;

    context_ptr->sb_min_score = ~0u;
    context_ptr->sb_max_score = 0u;

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; sb_index++) {

        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

        if (picture_control_set_ptr->slice_type == I_SLICE) {
            assert(0);
        }
        else {
            if (sb_params->pa_raster_scan_block_validity[PA_RASTER_SCAN_CU_INDEX_64x64] == EB_FALSE) {

                uint8_t cu8x8Index;
                uint8_t validCu8x8Count = 0;
                distortion = 0;
                for (cu8x8Index = PA_RASTER_SCAN_CU_INDEX_8x8_0; cu8x8Index <= PA_RASTER_SCAN_CU_INDEX_8x8_63; cu8x8Index++) {
                    if (sb_params->pa_raster_scan_block_validity[cu8x8Index]) {
                        distortion += picture_control_set_ptr->parent_pcs_ptr->me_results[sb_index][cu8x8Index].distortion_direction[0].distortion;
                        validCu8x8Count++;
                    }
                }
                if (validCu8x8Count > 0)
                    distortion = CLIP3(picture_control_set_ptr->parent_pcs_ptr->min_me_distortion, picture_control_set_ptr->parent_pcs_ptr->max_me_distortion, (distortion / validCu8x8Count) * 64);

                // Do not perform LCU score manipulation for incomplete LCUs as not valid signals
                sb_score = distortion;

            }
            else {
                distortion = picture_control_set_ptr->parent_pcs_ptr->me_results[sb_index][PA_RASTER_SCAN_CU_INDEX_64x64].distortion_direction[0].distortion;

                sb_score = distortion;

                if (sequence_control_set_ptr->static_config.tune == TUNE_SQ) {

                    if (
                        // LCU @ a picture boundary
                        sb_params->is_edge_sb
                        && picture_control_set_ptr->parent_pcs_ptr->non_moving_index_array[sb_index] != INVALID_NON_MOVING_SCORE
                        && picture_control_set_ptr->parent_pcs_ptr->non_moving_average_score != INVALID_NON_MOVING_SCORE
                        // Active LCU
                        && picture_control_set_ptr->parent_pcs_ptr->non_moving_index_array[sb_index] >= ADP_CLASS_NON_MOVING_INDEX_TH_0
                        // Active Picture or LCU belongs to the most active LCUs
                        && (picture_control_set_ptr->parent_pcs_ptr->non_moving_index_array[sb_index] >= picture_control_set_ptr->parent_pcs_ptr->non_moving_average_score || picture_control_set_ptr->parent_pcs_ptr->non_moving_average_score > ADP_CLASS_NON_MOVING_INDEX_TH_1)
                        // Off for sub-4K (causes instability as % of picture boundary LCUs is 2x higher for 1080p than for 4K (9% vs. 18% ) => might hurt the non-boundary LCUs)
                        && sequence_control_set_ptr->input_resolution == INPUT_SIZE_4K_RANGE) {

                        sb_score += (((picture_control_set_ptr->parent_pcs_ptr->max_me_distortion - sb_score) * ADP_CLASS_SHIFT_DIST_1) / 100);

                    }
                    else {

                        // Use LCU variance & activity
                        if (picture_control_set_ptr->parent_pcs_ptr->non_moving_index_array[sb_index] == ADP_CLASS_NON_MOVING_INDEX_TH_2 && picture_control_set_ptr->parent_pcs_ptr->variance[sb_index][PA_RASTER_SCAN_CU_INDEX_64x64] > IS_COMPLEX_SB_VARIANCE_TH && (sequence_control_set_ptr->static_config.frame_rate >> 16) > 30)

                            sb_score -= (((sb_score - picture_control_set_ptr->parent_pcs_ptr->min_me_distortion) * ADP_CLASS_SHIFT_DIST_0) / 100);
                        // Use LCU luminosity
                        if (sequence_control_set_ptr->input_resolution == INPUT_SIZE_4K_RANGE) {
                            // Shift to the left dark LCUs & shift to the right otherwise ONLY if a high dark area is present
                            if (picture_control_set_ptr->parent_pcs_ptr->black_area_percentage > ADP_BLACK_AREA_PERCENTAGE) {
                                if (picture_control_set_ptr->parent_pcs_ptr->y_mean[sb_index][PA_RASTER_SCAN_CU_INDEX_64x64] < ADP_DARK_SB_TH)
                                    sb_score -= (((sb_score - picture_control_set_ptr->parent_pcs_ptr->min_me_distortion) * ADP_CLASS_SHIFT_DIST_0) / 100);
                                else
                                    sb_score += (((picture_control_set_ptr->parent_pcs_ptr->max_me_distortion - sb_score) * ADP_CLASS_SHIFT_DIST_0) / 100);
                            }
                        }
                        else {
                            // Shift to the left dark LCUs & shift to the right otherwise
                            if (picture_control_set_ptr->parent_pcs_ptr->y_mean[sb_index][PA_RASTER_SCAN_CU_INDEX_64x64] < ADP_DARK_SB_TH)
                                sb_score -= (((sb_score - picture_control_set_ptr->parent_pcs_ptr->min_me_distortion) * ADP_CLASS_SHIFT_DIST_0) / 100);
                            else
                                sb_score += (((picture_control_set_ptr->parent_pcs_ptr->max_me_distortion - sb_score) * ADP_CLASS_SHIFT_DIST_0) / 100);
                        }

                    }
                }
            }
        }

        context_ptr->sb_score_array[sb_index] = sb_score;

        // Track MIN & MAX LCU scores
        context_ptr->sb_min_score = MIN(context_ptr->sb_score_array[sb_index], context_ptr->sb_min_score);
        context_ptr->sb_max_score = MAX(context_ptr->sb_score_array[sb_index], context_ptr->sb_max_score);
    }
}

/******************************************************
* BudgetingOutlierRemovalLcu
    Input   : LCU score histogram
    Output  : Adjusted min & max LCU score (to be used to clip the LCU score @ eb_vp9_set_sb_budget)
 Performs outlier removal by:
 1. dividing the total distance between the maximum sb_score and the minimum sb_score into NI intervals(NI = 10).
 For each sb_score interval,
 2. computing the number of LCUs NV with sb_score belonging to the subject sb_score interval.
 3. marking the subject sb_score interval as not valid if its NV represent less than validity threshold V_TH per - cent of the total number of the processed LCUs in the picture. (V_TH = 2)
 4. computing the distance MIN_D from 0 and the index of the first, in incremental way, sb_score interval marked as valid in the prior step.
 5. computing the distance MAX_D from NI and the index of the first, in decreasing way, sb_score interval marked as valid in the prior step.
 6. adjusting the minimum and maximum sb_score as follows :
    MIN_SCORE = MIN_SCORE + MIN_D * I_Value.
    MAX_SCORE = MAX_SCORE - MAX_D * I_Value.
******************************************************/

void perform_outlier_removal(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureParentControlSet          *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{

    uint32_t max_interval = 0;
    uint32_t sub_interval = 0;
    uint32_t sb_scoreHistogram[10] = { 0 };
    uint32_t sb_index;
    uint32_t sb_score;
    uint32_t processed_sb_count = 0;
    int32_t slot = 0;

    max_interval = context_ptr->sb_max_score - context_ptr->sb_min_score;
    // Consider 10 bins
    sub_interval = max_interval / 10;

    // Count # of LCUs at each bin
    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; sb_index++) {

        SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];

        if (sb_params->pa_raster_scan_block_validity[PA_RASTER_SCAN_CU_INDEX_64x64]) {

            processed_sb_count++;

            sb_score = context_ptr->sb_score_array[sb_index] + context_ptr->sb_min_score;
            if (sb_score < (sub_interval + context_ptr->sb_min_score)){
                sb_scoreHistogram[0]++;
            }
            else if (sb_score < ((2 * sub_interval) + context_ptr->sb_min_score)){
                sb_scoreHistogram[1]++;
            }
            else if (sb_score < ((3 * sub_interval) + context_ptr->sb_min_score)){
                sb_scoreHistogram[2]++;
            }
            else if (sb_score < ((4 * sub_interval) + context_ptr->sb_min_score)){
                sb_scoreHistogram[3]++;
            }
            else if (sb_score < ((5 * sub_interval) + context_ptr->sb_min_score)){
                sb_scoreHistogram[4]++;
            }
            else if (sb_score < ((6 * sub_interval) + context_ptr->sb_min_score)){
                sb_scoreHistogram[5]++;
            }
            else if (sb_score < ((7 * sub_interval) + context_ptr->sb_min_score)){
                sb_scoreHistogram[6]++;
            }
            else if (sb_score < ((8 * sub_interval) + context_ptr->sb_min_score)){
                sb_scoreHistogram[7]++;
            }
            else if (sb_score < ((9 * sub_interval) + context_ptr->sb_min_score)){
                sb_scoreHistogram[8]++;
            }
            else if (sb_score < ((10 * sub_interval) + context_ptr->sb_min_score)){
                sb_scoreHistogram[9]++;
            }
        }
    }

    // Zero-out the bin if percentage lower than VALID_SLOT_TH
    for (slot = 0; slot < 10; slot++){
        if (processed_sb_count > 0 && (sb_scoreHistogram[slot] * 100 / processed_sb_count) < VALID_SLOT_TH){
            sb_scoreHistogram[slot] = 0;
        }
    }

    // Ignore null bins
    for (slot = 0; slot < 10; slot++){
        if (sb_scoreHistogram[slot]){
            context_ptr->sb_min_score = context_ptr->sb_min_score+ (slot * sub_interval);
            break;
        }
    }

    for (slot = 9; slot >= 0; slot--){
        if (sb_scoreHistogram[slot]){
            context_ptr->sb_max_score = context_ptr->sb_max_score - ((9 - slot) * sub_interval);
            break;
        }
    }
}
/******************************************************
* Assign a search method for each LCU
    Input   : LCU score, detection signals
    Output  : search method for each LCU
******************************************************/
void eb_vp9_derive_sb_md_mode(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr) {

    // Configure ADP
    eb_vp9_configure_adp(
        picture_control_set_ptr,
        context_ptr);

    // Set the target budget
    if (sequence_control_set_ptr->static_config.tune == TUNE_SQ) {
        SetTargetBudgetSq(
            sequence_control_set_ptr,
            picture_control_set_ptr,
            context_ptr);
    }
    else if (sequence_control_set_ptr->static_config.tune == TUNE_VMAF) {
        set_target_budget_vmaf(
            sequence_control_set_ptr,
            picture_control_set_ptr,
            context_ptr);
    }
    else {
        eb_vp9_set_target_budget_oq(
            sequence_control_set_ptr,
            picture_control_set_ptr,
            context_ptr);
    }

    // Set the percentage based thresholds
    eb_vp9_derive_default_segments(
        picture_control_set_ptr,
        context_ptr);

    // Compute the cost of the refinements
    compute_refinement_cost(
        sequence_control_set_ptr,
        picture_control_set_ptr,
        context_ptr);

    // Derive LCU score
    eb_vp9_derive_sb_score(
        sequence_control_set_ptr,
        picture_control_set_ptr,
        context_ptr);

    // Remove the outliers
    perform_outlier_removal(
        sequence_control_set_ptr,
        picture_control_set_ptr->parent_pcs_ptr,
        context_ptr);

    // Perform Budgetting
    eb_vp9_derive_optimal_budget_per_sb(
        sequence_control_set_ptr,
        picture_control_set_ptr,
        context_ptr);

    // Set the search method using the LCU cost (mapping)
    eb_vp9_derive_search_method(
        sequence_control_set_ptr,
        picture_control_set_ptr,
        context_ptr);

}

/******************************************************
* Derive Mode Decision Config Settings for SQ
Input   : encoder mode and tune
Output  : EncDec Kernel signal(s)
******************************************************/
EbErrorType eb_vp9_signal_derivation_mode_decision_config_kernel_sq(
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{

    EbErrorType return_error = EB_ErrorNone;

    context_ptr->adp_level = picture_control_set_ptr->parent_pcs_ptr->enc_mode;

    return return_error;
}

/******************************************************
* Derive Mode Decision Config Settings for OQ
Input   : encoder mode and tune
Output  : EncDec Kernel signal(s)
******************************************************/
EbErrorType eb_vp9_signal_derivation_mode_decision_config_kernel_oq_vmaf(
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr)
{

    EbErrorType return_error = EB_ErrorNone;

    context_ptr->adp_level = picture_control_set_ptr->parent_pcs_ptr->enc_mode;

    return return_error;
}

/********************************************
 * Constants
 ********************************************/
#define ADD_CU_STOP_SPLIT             0   // Take into account & Stop Splitting
#define ADD_CU_CONTINUE_SPLIT         1   // Take into account & Continue Splitting
#define DO_NOT_ADD_CU_CONTINUE_SPLIT  2   // Do not take into account & Continue Splitting

#define DEPTH_64 0   // Depth corresponding to the CU size
#define DEPTH_32 1   // Depth corresponding to the CU size
#define DEPTH_16 2   // Depth corresponding to the CU size
#define DEPTH_8  3   // Depth corresponding to the CU size

static const uint8_t incremental_count[PA_BLOCK_MAX_COUNT] = {

    //64x64
    0,
    //32x32
    4, 4,
    4, 4,
    //16x16
    0, 0, 0, 0,
    0, 4, 0, 4,
    0, 0, 0, 0,
    0, 4, 0, 4,
    //8x8
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 4, 0, 0, 0, 4,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 4, 0, 0, 0, 4

};

/*******************************************
mdcSetDepth : set depth to be tested
*******************************************/
#define REFINEMENT_P        0x01
#define REFINEMENT_Pp1      0x02
#define REFINEMENT_Pp2      0x04
#define REFINEMENT_Pp3      0x08
#define REFINEMENT_Pm1      0x10
#define REFINEMENT_Pm2      0x20
#define REFINEMENT_Pm3      0x40

#define PA_DEPTH_ONE_STEP     21
#define PA_DEPTH_TWO_STEP      5
#define PA_DEPTH_THREE_STEP    1

void eb_vp9_MdcInterDepthDecision(
    ModeDecisionConfigurationContext *context_ptr,
    uint32_t                          origin_x,
    uint32_t                          origin_y,
    uint32_t                          end_depth,
    uint32_t                          block_index)
{

    uint32_t leftblock_index;
    uint32_t topblock_index;
    uint32_t topLeftblock_index;
    uint32_t depth_zero_candidate_block_index;
    uint32_t depth_one_candidate_block_index = block_index;
    uint32_t depth_two_candidate_block_index = block_index;
    uint64_t depth_n_cost = 0;
    uint64_t depth_n_plus_one_cost = 0;
    uint64_t depth_n_part_cost = 0;
    uint64_t depth_n_plus_one_part_cost = 0;

    MdcpLocalCodingUnit *local_cu_array = context_ptr->local_cu_array;

    /*** Stage 0: Inter depth decision: depth 2 vs depth 3 ***/
    // Walks to the last coded 8x8 block for merging
    uint8_t  group_of8x8_blocks_count = context_ptr->group_of8x8_blocks_count;
    uint8_t  group_of16x16_blocks_count = context_ptr->group_of16x16_blocks_count;
    if ((GROUP_OF_4_8x8_BLOCKS(origin_x, origin_y))) {

        group_of8x8_blocks_count++;

        // From the last coded cu index, get the indices of the left, top, and top left cus
        leftblock_index = block_index - PA_DEPTH_THREE_STEP;
        topblock_index = leftblock_index - PA_DEPTH_THREE_STEP;
        topLeftblock_index = topblock_index - PA_DEPTH_THREE_STEP;

        // From the top left index, get the index of the candidate pu for merging
        depth_two_candidate_block_index = topLeftblock_index - 1;

        // Compute depth N cost
#if 0 // Hsan: partition rate not helping @ open loop partitioning
        get_partition_cost(
            picture_control_set_ptr,
            BLOCK_16X16,
            PARTITION_NONE,
            (local_cu_array[depth_two_candidate_block_index]).partition_context,
            &depth_n_part_cost);
#endif
        depth_n_cost =
            (local_cu_array[depth_two_candidate_block_index]).early_cost + depth_n_part_cost;

        if (end_depth < 3) {

            (local_cu_array[depth_two_candidate_block_index]).early_split_flag = EB_FALSE;
            (local_cu_array[depth_two_candidate_block_index]).early_cost = depth_n_cost;
        }
        else {
            // Compute depth N+1 cost
#if 0 // Hsan: partition rate not helping @ open loop partitioning
            get_partition_cost(
                picture_control_set_ptr,
                BLOCK_16X16,
                PARTITION_SPLIT,
                (local_cu_array[depth_two_candidate_block_index]).partition_context,
                &depth_n_plus_one_part_cost);
#endif
            depth_n_plus_one_cost =
                (local_cu_array[block_index]).early_cost +
                (local_cu_array[leftblock_index]).early_cost +
                (local_cu_array[topblock_index]).early_cost +
                (local_cu_array[topLeftblock_index]).early_cost + depth_n_plus_one_part_cost;

            if (depth_n_cost <= depth_n_plus_one_cost) {

                // If the cost is low enough to warrant not spliting further:
                // 1. set the split flag of the candidate pu for merging to false
                // 2. update the last pu index
                (local_cu_array[depth_two_candidate_block_index]).early_split_flag = EB_FALSE;
                (local_cu_array[depth_two_candidate_block_index]).early_cost = depth_n_cost;

            }
            else {
                // If the cost is not low enough:
                // update the cost of the candidate pu for merging
                // this update is required for the next inter depth decision
                (&local_cu_array[depth_two_candidate_block_index])->early_cost = depth_n_plus_one_cost;
            }

        }
    }

    // Walks to the last coded 16x16 block for merging
    if (GROUP_OF_4_16x16_BLOCKS(pa_get_block_stats(depth_two_candidate_block_index)->origin_x, pa_get_block_stats(depth_two_candidate_block_index)->origin_y) &&
        (group_of8x8_blocks_count == 4)) {

        group_of8x8_blocks_count = 0;
        group_of16x16_blocks_count++;

        // From the last coded pu index, get the indices of the left, top, and top left pus
        leftblock_index = depth_two_candidate_block_index - PA_DEPTH_TWO_STEP;
        topblock_index = leftblock_index - PA_DEPTH_TWO_STEP;
        topLeftblock_index = topblock_index - PA_DEPTH_TWO_STEP;

        // From the top left index, get the index of the candidate pu for merging
        depth_one_candidate_block_index = topLeftblock_index - 1;

        if (pa_get_block_stats(depth_one_candidate_block_index)->depth == 1) {

            // Compute depth N cost
#if 0 // Hsan: partition rate not helping @ open loop partitioning
            get_partition_cost(
                picture_control_set_ptr,
                BLOCK_32X32,
                PARTITION_NONE,
                local_cu_array[depth_one_candidate_block_index].partition_context,
                &depth_n_part_cost);
#endif

            depth_n_cost =
                local_cu_array[depth_one_candidate_block_index].early_cost + depth_n_part_cost;
            if (end_depth < 2) {

                local_cu_array[depth_one_candidate_block_index].early_split_flag = EB_FALSE;
                local_cu_array[depth_one_candidate_block_index].early_cost = depth_n_cost;

            }
            else {

                // Compute depth N+1 cost
#if 0 // Hsan: partition rate not helping @ open loop partitioning
                get_partition_cost(
                    picture_control_set_ptr,
                    BLOCK_32X32,
                    PARTITION_SPLIT,
                    local_cu_array[depth_one_candidate_block_index].partition_context,
                    &depth_n_plus_one_part_cost);
#endif
                depth_n_plus_one_cost = local_cu_array[depth_two_candidate_block_index].early_cost +
                    local_cu_array[leftblock_index].early_cost +
                    local_cu_array[topblock_index].early_cost +
                    local_cu_array[topLeftblock_index].early_cost +
                    depth_n_plus_one_part_cost;

                // Inter depth comparison: depth 1 vs depth 2
                if (depth_n_cost <= depth_n_plus_one_cost) {
                    // If the cost is low enough to warrant not spliting further:
                    // 1. set the split flag of the candidate pu for merging to false
                    // 2. update the last pu index
                    local_cu_array[depth_one_candidate_block_index].early_split_flag = EB_FALSE;
                    local_cu_array[depth_one_candidate_block_index].early_cost = depth_n_cost;
                }
                else {
                    // If the cost is not low enough:
                    // update the cost of the candidate pu for merging
                    // this update is required for the next inter depth decision
                    local_cu_array[depth_one_candidate_block_index].early_cost = depth_n_plus_one_cost;
                }
            }
        }
    }

    // Stage 2: Inter depth decision: depth 0 vs depth 1

    // Walks to the last coded 32x32 block for merging
    // Stage 2 isn't performed in I slices since the abcense of 64x64 candidates
    if (GROUP_OF_4_32x32_BLOCKS(pa_get_block_stats(depth_one_candidate_block_index)->origin_x, pa_get_block_stats(depth_one_candidate_block_index)->origin_y) &&
        (group_of16x16_blocks_count == 4)) {

        group_of16x16_blocks_count = 0;

        // From the last coded pu index, get the indices of the left, top, and top left pus
        leftblock_index = depth_one_candidate_block_index - PA_DEPTH_ONE_STEP;
        topblock_index = leftblock_index - PA_DEPTH_ONE_STEP;
        topLeftblock_index = topblock_index - PA_DEPTH_ONE_STEP;

        // From the top left index, get the index of the candidate pu for merging
        depth_zero_candidate_block_index = topLeftblock_index - 1;

        if (pa_get_block_stats(depth_zero_candidate_block_index)->depth == 0) {

            // Compute depth N cost
#if 0 // Hsan: partition rate not helping @ open loop partitioning
            get_partition_cost(
                picture_control_set_ptr,
                BLOCK_64X64,
                PARTITION_NONE,
                (&local_cu_array[depth_zero_candidate_block_index])->partition_context,
                &depth_n_part_cost);
#endif
            depth_n_cost = (&local_cu_array[depth_zero_candidate_block_index])->early_cost + depth_n_part_cost;

            if (end_depth < 1) {
                (&local_cu_array[depth_zero_candidate_block_index])->early_split_flag = EB_FALSE;
            } else {
                // Compute depth N+1 cost
#if 0 // Hsan: partition rate not helping @ open loop partitioning
                get_partition_cost(
                    picture_control_set_ptr,
                    BLOCK_64X64,
                    PARTITION_SPLIT,
                    (&local_cu_array[depth_zero_candidate_block_index])->partition_context,
                    &depth_n_plus_one_part_cost);
#endif
                depth_n_plus_one_cost =
                    local_cu_array[depth_one_candidate_block_index].early_cost +
                    local_cu_array[leftblock_index].early_cost +
                    local_cu_array[topblock_index].early_cost +
                    local_cu_array[topLeftblock_index].early_cost +
                    depth_n_plus_one_part_cost;

                // Inter depth comparison: depth 0 vs depth 1
                if (depth_n_cost <= depth_n_plus_one_cost) {
                    // If the cost is low enough to warrant not spliting further:
                    // 1. set the split flag of the candidate pu for merging to false
                    // 2. update the last pu index
                    (&local_cu_array[depth_zero_candidate_block_index])->early_split_flag = EB_FALSE;
                }
            }
        }
    }

    context_ptr->group_of8x8_blocks_count = group_of8x8_blocks_count;
    context_ptr->group_of16x16_blocks_count = group_of16x16_blocks_count;
}

static void prediction_partition_loop(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr,
    SbUnit                           *sb_ptr,
    uint32_t                          start_depth,
    uint32_t                          end_depth)
{
    MACROBLOCKD *const xd = context_ptr->e_mbd;

    MdcpLocalCodingUnit *local_cu_array = context_ptr->local_cu_array;
    MdcpLocalCodingUnit *block_ptr;

    SbParams *sb_params = &sequence_control_set_ptr->sb_params_array[sb_ptr->sb_index];

    uint32_t block_index_in_rater_scan;
    uint32_t block_index;

    VP9_COMP   *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;

#if SEG_SUPPORT
    VP9_COMMON *const cm = &cpi->common;
    struct segmentation *const seg = &cm->seg;
    const int qindex = eb_vp9_get_qindex(seg, sb_ptr->segment_id, picture_control_set_ptr->base_qindex);
#else
    const int qindex = picture_control_set_ptr->base_qindex;
#endif
    int RDMULT = eb_vp9_compute_rd_mult(cpi, qindex);
    context_ptr->rd_mult_sad = (int)MAX(round(sqrtf((float)RDMULT / 128) * 128), 1);

    for (block_index = 0; block_index < PA_BLOCK_MAX_COUNT; ++block_index) {

        block_ptr = &local_cu_array[block_index];

#if 0 // Hsan: partition rate not helping @ open loop partitioning
        // Hsan: neighbor not generated @ open loop partitioning
        block_ptr->partition_context = 0;
#endif
        local_cu_array[block_index].slected_cu = EB_FALSE;
        local_cu_array[block_index].stop_split = EB_FALSE;

        block_index_in_rater_scan = MD_SCAN_TO_RASTER_SCAN[block_index];

        if (sb_params->pa_raster_scan_block_validity[block_index_in_rater_scan])
        {
            uint32_t depth;
            PaBlockStats *block_stats_ptr = pa_get_block_stats(block_index);

            depth = block_stats_ptr->depth;
            block_ptr->early_split_flag = (depth < end_depth) ? EB_TRUE : EB_FALSE;

            if (depth >= start_depth && depth <= end_depth) {

                if (picture_control_set_ptr->slice_type != I_SLICE) {

                    MeCuResults * me_pu_result = &picture_control_set_ptr->parent_pcs_ptr->me_results[sb_ptr->sb_index][block_index_in_rater_scan];

                    if (me_pu_result->distortion_direction[0].direction == BI_PRED) {
                        context_ptr->candidate_ptr->mode_info->ref_frame[0] = LAST_FRAME;
                        context_ptr->candidate_ptr->mode_info->ref_frame[1] = ALTREF_FRAME;
                    }
                    else if (me_pu_result->distortion_direction[0].direction == UNI_PRED_LIST_0) {
                        context_ptr->candidate_ptr->mode_info->ref_frame[0] = LAST_FRAME;
                        context_ptr->candidate_ptr->mode_info->ref_frame[1] = INTRA_FRAME;
                    }
                    else { // if (me_pu_result->distortion_direction[0].direction == UNI_PRED_LIST_1)
                        context_ptr->candidate_ptr->mode_info->ref_frame[0] = ALTREF_FRAME;
                        context_ptr->candidate_ptr->mode_info->ref_frame[1] = INTRA_FRAME;
                    }

                    // Hsan: what's the best mode for rate simulation
                    context_ptr->candidate_ptr->mode_info->mode = NEARMV;

                    if (context_ptr->candidate_ptr->mode_info->ref_frame[0] == ALTREF_FRAME) {
                        context_ptr->candidate_ptr->mode_info->mv[0].as_mv.col = me_pu_result->x_mv_l1 << 1;
                        context_ptr->candidate_ptr->mode_info->mv[0].as_mv.row = me_pu_result->y_mv_l1 << 1;
                        context_ptr->candidate_ptr->mode_info->mv[1].as_mv.col = me_pu_result->x_mv_l1 << 1;
                        context_ptr->candidate_ptr->mode_info->mv[1].as_mv.row = me_pu_result->y_mv_l1 << 1;
                    }
                    else {
                        context_ptr->candidate_ptr->mode_info->mv[0].as_mv.col = me_pu_result->x_mv_l0 << 1;
                        context_ptr->candidate_ptr->mode_info->mv[0].as_mv.row = me_pu_result->y_mv_l0 << 1;
                        context_ptr->candidate_ptr->mode_info->mv[1].as_mv.col = me_pu_result->x_mv_l1 << 1;
                        context_ptr->candidate_ptr->mode_info->mv[1].as_mv.row = me_pu_result->y_mv_l1 << 1;
                    }

                    // Set above_mi and left_mi
                    // Hsan: neighbor not generated @ open loop partitioning
                    context_ptr->e_mbd->above_mi = NULL;
                    context_ptr->e_mbd->left_mi  = NULL;

                    estimate_ref_frame_costs(
                        &picture_control_set_ptr->parent_pcs_ptr->cpi->common,
                        xd,
                        0,// segment_id
                        context_ptr->ref_costs_single,
                        context_ptr->ref_costs_comp,
                        &context_ptr->comp_mode_p);

                    block_ptr->early_cost = inter_fast_cost(
                        picture_control_set_ptr,
                        0,
                        context_ptr->rd_mult_sad,
                        context_ptr->mbmi_ext,
                        context_ptr->e_mbd,
                        context_ptr->ref_costs_single,
                        context_ptr->ref_costs_comp,
                        context_ptr->comp_mode_p,
                        context_ptr->candidate_ptr,
                        me_pu_result->distortion_direction[0].distortion,
                        0);

                }
                else {
                    assert(0);
                }

                if (end_depth == 2) {
                    context_ptr->group_of8x8_blocks_count = depth == 2 ? incremental_count[block_index_in_rater_scan] : 0;
                } else if (end_depth == 1) {
                    context_ptr->group_of16x16_blocks_count = depth == 1 ? incremental_count[block_index_in_rater_scan] : 0;
                }

                eb_vp9_MdcInterDepthDecision(
                    context_ptr,
                    block_stats_ptr->origin_x,
                    block_stats_ptr->origin_y,
                    end_depth,
                    block_index);
            }
            else {
                block_ptr->early_cost = ~0u;
            }

        }
    }
}

static const uint8_t parentblock_index[PA_BLOCK_MAX_COUNT] =
{
    0,
    0, 0, 0, 1, 2, 3, 5, 0, 1, 2, 3, 10, 0, 1, 2, 3, 15, 0, 1, 2, 3,
    21, 0, 0, 1, 2, 3, 5, 0, 1, 2, 3, 10, 0, 1, 2, 3, 15, 0, 1, 2, 3,
    42, 0, 0, 1, 2, 3, 5, 0, 1, 2, 3, 10, 0, 1, 2, 3, 15, 0, 1, 2, 3,
    36, 0, 0, 1, 2, 3, 5, 0, 1, 2, 3, 10, 0, 1, 2, 3, 15, 0, 1, 2, 3,
};

static EbErrorType mdc_refinement(
    MdcpLocalCodingUnit *local_cu_array,
    uint32_t              block_index,
    uint32_t              depth,
    uint8_t               refinement_level,
    uint8_t               lowest_level)
{
    EbErrorType return_error = EB_ErrorNone;

    if (refinement_level & REFINEMENT_P) {
        if (lowest_level == REFINEMENT_P) {
            local_cu_array[block_index].stop_split = EB_TRUE;
        }

    }
    else {
        local_cu_array[block_index].slected_cu = EB_FALSE;
    }

    if (refinement_level & REFINEMENT_Pp1) {

        if (depth < 3 && block_index < 81) {
            local_cu_array[block_index + 1].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + depth_offset[depth + 1]].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + 2 * depth_offset[depth + 1]].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + 3 * depth_offset[depth + 1]].slected_cu = EB_TRUE;
        }
        if (lowest_level == REFINEMENT_Pp1) {
            if (depth < 3 && block_index < 81) {
                local_cu_array[block_index + 1].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + depth_offset[depth + 1]].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + 2 * depth_offset[depth + 1]].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + 3 * depth_offset[depth + 1]].stop_split = EB_TRUE;
            }
        }
    }

    if (refinement_level & REFINEMENT_Pp2) {
        if (depth < 2 && block_index < 65) {
            local_cu_array[block_index + 1 + 1].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + 1 + depth_offset[depth + 2]].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + 1 + 2 * depth_offset[depth + 2]].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + 1 + 3 * depth_offset[depth + 2]].slected_cu = EB_TRUE;

            local_cu_array[block_index + 1 + depth_offset[depth + 1] + 1].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + depth_offset[depth + 1] + 1 + depth_offset[depth + 2]].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + depth_offset[depth + 1] + 1 + 2 * depth_offset[depth + 2]].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + depth_offset[depth + 1] + 1 + 3 * depth_offset[depth + 2]].slected_cu = EB_TRUE;

            local_cu_array[block_index + 1 + 2 * depth_offset[depth + 1] + 1].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + 2 * depth_offset[depth + 1] + 1 + depth_offset[depth + 2]].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + 2 * depth_offset[depth + 1] + 1 + 2 * depth_offset[depth + 2]].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + 2 * depth_offset[depth + 1] + 1 + 3 * depth_offset[depth + 2]].slected_cu = EB_TRUE;

            local_cu_array[block_index + 1 + 3 * depth_offset[depth + 1] + 1].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + 3 * depth_offset[depth + 1] + 1 + depth_offset[depth + 2]].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + 3 * depth_offset[depth + 1] + 1 + 2 * depth_offset[depth + 2]].slected_cu = EB_TRUE;
            local_cu_array[block_index + 1 + 3 * depth_offset[depth + 1] + 1 + 3 * depth_offset[depth + 2]].slected_cu = EB_TRUE;
        }
        if (lowest_level == REFINEMENT_Pp2) {
            if (depth < 2 && block_index < 65) {
                local_cu_array[block_index + 1 + 1].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + 1 + depth_offset[depth + 2]].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + 1 + 2 * depth_offset[depth + 2]].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + 1 + 3 * depth_offset[depth + 2]].stop_split = EB_TRUE;

                local_cu_array[block_index + 1 + depth_offset[depth + 1] + 1].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + depth_offset[depth + 1] + 1 + depth_offset[depth + 2]].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + depth_offset[depth + 1] + 1 + 2 * depth_offset[depth + 2]].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + depth_offset[depth + 1] + 1 + 3 * depth_offset[depth + 2]].stop_split = EB_TRUE;

                local_cu_array[block_index + 1 + 2 * depth_offset[depth + 1] + 1].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + 2 * depth_offset[depth + 1] + 1 + depth_offset[depth + 2]].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + 2 * depth_offset[depth + 1] + 1 + 2 * depth_offset[depth + 2]].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + 2 * depth_offset[depth + 1] + 1 + 3 * depth_offset[depth + 2]].stop_split = EB_TRUE;

                local_cu_array[block_index + 1 + 3 * depth_offset[depth + 1] + 1].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + 3 * depth_offset[depth + 1] + 1 + depth_offset[depth + 2]].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + 3 * depth_offset[depth + 1] + 1 + 2 * depth_offset[depth + 2]].stop_split = EB_TRUE;
                local_cu_array[block_index + 1 + 3 * depth_offset[depth + 1] + 1 + 3 * depth_offset[depth + 2]].stop_split = EB_TRUE;
            }
        }
    }

    if (refinement_level & REFINEMENT_Pp3) {
        uint8_t in_loop;
        uint8_t out_loop;
        uint8_t block_index = 2;
        if (depth == 0) {

            for (out_loop = 0; out_loop < 16; ++out_loop) {
                for (in_loop = 0; in_loop < 4; ++in_loop) {
                    local_cu_array[++block_index].slected_cu = EB_TRUE;

                }
                block_index += block_index == 21 ? 2 : block_index == 42 ? 2 : block_index == 63 ? 2 : 1;

            }
            if (lowest_level == REFINEMENT_Pp3) {
                block_index = 2;
                for (out_loop = 0; out_loop < 16; ++out_loop) {
                    for (in_loop = 0; in_loop < 4; ++in_loop) {
                        local_cu_array[++block_index].stop_split = EB_TRUE;
                    }
                    block_index += block_index == 21 ? 2 : block_index == 42 ? 2 : block_index == 63 ? 2 : 1;
                }
            }
        }

    }

    if (refinement_level & REFINEMENT_Pm1) {
        if (depth > 0) {
            local_cu_array[block_index - 1 - parentblock_index[block_index]].slected_cu = EB_TRUE;
        }
        if (lowest_level == REFINEMENT_Pm1) {
            if (depth > 0) {
                local_cu_array[block_index - 1 - parentblock_index[block_index]].stop_split = EB_TRUE;
            }
        }
    }

    if (refinement_level & REFINEMENT_Pm2) {
        if (depth == 2) {
            local_cu_array[0].slected_cu = EB_TRUE;
        }
        if (depth == 3) {
            local_cu_array[1].slected_cu = EB_TRUE;
            local_cu_array[22].slected_cu = EB_TRUE;
            local_cu_array[43].slected_cu = EB_TRUE;
            local_cu_array[64].slected_cu = EB_TRUE;
        }
        if (lowest_level == REFINEMENT_Pm2) {
            if (depth == 2) {
                local_cu_array[0].stop_split = EB_TRUE;
            }
            if (depth == 3) {
                local_cu_array[1].stop_split = EB_TRUE;
                local_cu_array[22].stop_split = EB_TRUE;
                local_cu_array[43].stop_split = EB_TRUE;
                local_cu_array[64].stop_split = EB_TRUE;
            }
        }
    }

    if (refinement_level & REFINEMENT_Pm3) {
        if (depth == 3) {
            local_cu_array[0].slected_cu = EB_TRUE;
        }
        if (lowest_level == REFINEMENT_Pm2) {
            if (depth == 3) {
                local_cu_array[0].stop_split = EB_TRUE;
            }
        }
    }

    return return_error;
}

static void refinement_prediction_loop(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    SbUnit                           *sb_ptr,
    uint32_t                          sb_index,
    ModeDecisionConfigurationContext *context_ptr)
{
    MdcpLocalCodingUnit  *local_cu_array = context_ptr->local_cu_array;
    SbParams             *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
    uint32_t              temporal_layer_index = picture_control_set_ptr->temporal_layer_index;
    uint32_t              block_index = 0;
    uint8_t               stationary_edge_over_time_flag = (&(picture_control_set_ptr->parent_pcs_ptr->sb_stat_array[sb_index]))->stationary_edge_over_time_flag;

    sb_ptr->pred64 = EB_FALSE;

    while (block_index < PA_BLOCK_MAX_COUNT)
    {
        if (sb_params->pa_raster_scan_block_validity[MD_SCAN_TO_RASTER_SCAN[block_index]] && (local_cu_array[block_index].early_split_flag == EB_FALSE))
        {
            local_cu_array[block_index].slected_cu = EB_TRUE;
            sb_ptr->pred64 = (block_index == 0) ? EB_TRUE : sb_ptr->pred64;

            PaBlockStats   *block_stats_ptr = pa_get_block_stats(block_index);
            uint32_t depth = block_stats_ptr->depth;

            uint8_t refinement_level;

            if (sb_ptr->picture_control_set_ptr->slice_type == I_SLICE) {

                {
                    uint8_t lowest_level = 0x00;

                    if (sequence_control_set_ptr->input_resolution == INPUT_SIZE_4K_RANGE)
                        refinement_level = ndp_refinement_control_islice[depth];
                    else
                        refinement_level = ndp_refinement_control_islice_sub4_k[depth];

                    if (depth <= 1 && stationary_edge_over_time_flag > 0) {
                        if (depth == 0)
                            refinement_level = Predp1 + Predp2 + Predp3;
                        else
                            refinement_level = Pred + Predp1 + Predp2;

                    }
                    lowest_level = (refinement_level & REFINEMENT_Pp3) ? REFINEMENT_Pp3 : (refinement_level & REFINEMENT_Pp2) ? REFINEMENT_Pp2 : (refinement_level & REFINEMENT_Pp1) ? REFINEMENT_Pp1 :
                        (refinement_level & REFINEMENT_P) ? REFINEMENT_P :
                        (refinement_level & REFINEMENT_Pm1) ? REFINEMENT_Pm1 : (refinement_level & REFINEMENT_Pm2) ? REFINEMENT_Pm2 : (refinement_level & REFINEMENT_Pm3) ? REFINEMENT_Pm3 : 0x00;

                    mdc_refinement(
                        &(*context_ptr->local_cu_array),
                        block_index,
                        depth,
                        refinement_level,
                        lowest_level);
                }
            }
            else {
                if (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_SB_SWITCH_DEPTH_MODE && (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_PRED_OPEN_LOOP_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE)) {
                    refinement_level = Pred;
                }
                else

                    if (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_OPEN_LOOP_DEPTH_MODE ||
                        (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_SB_SWITCH_DEPTH_MODE && picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_OPEN_LOOP_DEPTH_MODE))

                        refinement_level = ndp_refinement_control_nref[temporal_layer_index][depth];
                    else
                        refinement_level = ndp_refinement_control_fast[temporal_layer_index][depth];

                if (picture_control_set_ptr->parent_pcs_ptr->cu8x8_mode == CU_8x8_MODE_1) {
                    refinement_level = ((refinement_level & REFINEMENT_Pp1) && depth == 2) ? refinement_level - REFINEMENT_Pp1 :
                        ((refinement_level & REFINEMENT_Pp2) && depth == 1) ? refinement_level - REFINEMENT_Pp2 :
                        ((refinement_level & REFINEMENT_Pp3) && depth == 0) ? refinement_level - REFINEMENT_Pp3 : refinement_level;
                }

                uint8_t lowest_level = 0x00;

                lowest_level = (refinement_level & REFINEMENT_Pp3) ? REFINEMENT_Pp3 : (refinement_level & REFINEMENT_Pp2) ? REFINEMENT_Pp2 : (refinement_level & REFINEMENT_Pp1) ? REFINEMENT_Pp1 :
                    (refinement_level & REFINEMENT_P) ? REFINEMENT_P :
                    (refinement_level & REFINEMENT_Pm1) ? REFINEMENT_Pm1 : (refinement_level & REFINEMENT_Pm2) ? REFINEMENT_Pm2 : (refinement_level & REFINEMENT_Pm3) ? REFINEMENT_Pm3 : 0x00;

                mdc_refinement(
                    &(*context_ptr->local_cu_array),
                    block_index,
                    depth,
                    refinement_level,
                    lowest_level);
            }

            block_index += depth_offset[depth];

        }
        else {

            block_index++;
        }
    } // End while 1 CU Loop
}

void forward_cu_to_mode_decision(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    uint32_t                          sb_index,
    ModeDecisionConfigurationContext *context_ptr)
{

    uint8_t              block_index = 0;
    uint32_t             cu_class = DO_NOT_ADD_CU_CONTINUE_SPLIT;
    EB_BOOL              split_flag = EB_TRUE;
    MdcSbData           *results_ptr = &picture_control_set_ptr->mdc_sb_data_array[sb_index];
    SbParams            *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
    MdcpLocalCodingUnit *local_cu_array = context_ptr->local_cu_array;
    EB_SLICE             slice_type = picture_control_set_ptr->slice_type;

    results_ptr->block_count = 0;

    block_index = 0;

    while (block_index < PA_BLOCK_MAX_COUNT)
    {
        PaBlockStats *block_stats_ptr = pa_get_block_stats(block_index);

        split_flag = EB_TRUE;
        if (sb_params->pa_raster_scan_block_validity[MD_SCAN_TO_RASTER_SCAN[block_index]])
        {
            switch (block_stats_ptr->depth) {

            case 0:
            case 1:
            case 2:

                cu_class = DO_NOT_ADD_CU_CONTINUE_SPLIT;

                if (slice_type == I_SLICE) {
                    cu_class = local_cu_array[block_index].slected_cu == EB_TRUE ? ADD_CU_CONTINUE_SPLIT : cu_class;
                    cu_class = local_cu_array[block_index].stop_split == EB_TRUE ? ADD_CU_STOP_SPLIT : cu_class;
                }
                else {
                    cu_class = local_cu_array[block_index].slected_cu == EB_TRUE ? ADD_CU_CONTINUE_SPLIT : cu_class;
                    cu_class = local_cu_array[block_index].stop_split == EB_TRUE ? ADD_CU_STOP_SPLIT : cu_class;

                }

                // Take into account MAX_CU_SIZE
                cu_class = (block_stats_ptr->size > MAX_CU_SIZE || (slice_type == I_SLICE && block_stats_ptr->size > MAX_INTRA_SIZE)) ?
                    DO_NOT_ADD_CU_CONTINUE_SPLIT :
                    cu_class;

                // Take into account MIN_CU_SIZE
                cu_class = (block_stats_ptr->size == MIN_CU_SIZE || (slice_type == I_SLICE && block_stats_ptr->size == MIN_INTRA_SIZE)) ?
                    ADD_CU_STOP_SPLIT :
                    cu_class;

                switch (cu_class) {

                case ADD_CU_STOP_SPLIT:
                    // Stop
                    results_ptr->block_data_array[results_ptr->block_count].block_index = pa_to_ep_block_index[block_index];
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;

                    break;

                case ADD_CU_CONTINUE_SPLIT:
                    // Go Down + consider the current CU as candidate
                    results_ptr->block_data_array[results_ptr->block_count].block_index = pa_to_ep_block_index[block_index];
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;

                    break;

                case DO_NOT_ADD_CU_CONTINUE_SPLIT:
                    // Go Down + do not consider the current CU as candidate
                    split_flag = EB_TRUE;
                    break;
                }

                break;
            case 3:
                results_ptr->block_data_array[results_ptr->block_count].block_index = pa_to_ep_block_index[block_index];
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;

                break;

            default:
                results_ptr->block_data_array[results_ptr->block_count].block_index = pa_to_ep_block_index[block_index];
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;
            }
        }

        block_index += (split_flag == EB_TRUE) ? 1 : depth_offset[block_stats_ptr->depth];

    }
}

static EbErrorType early_mode_decision_sb(
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    ModeDecisionConfigurationContext *context_ptr,
    SbUnit                           *sb_ptr)
{

    EbErrorType    return_error = EB_ErrorNone;

    EB_SLICE       slice_type = picture_control_set_ptr->slice_type;

    // Hsan: to evaluate after freezing a 1st M6
#if SHUT_64x64_BASE_RESTRICTION
    uint32_t      start_depth = (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0 && (sequence_control_set_ptr->static_config.tune == TUNE_SQ || sequence_control_set_ptr->static_config.tune == TUNE_VMAF)) ? DEPTH_32 : DEPTH_64;
#else
    uint32_t      start_depth = (picture_control_set_ptr->parent_pcs_ptr->temporal_layer_index == 0) ? DEPTH_32 : DEPTH_64;
#endif
    uint32_t      end_depth = (slice_type == I_SLICE) ? DEPTH_8 : DEPTH_16;

    context_ptr->group_of8x8_blocks_count = 0;
    context_ptr->group_of16x16_blocks_count = 0;

    prediction_partition_loop(
        sequence_control_set_ptr,
        picture_control_set_ptr,
        context_ptr,
        sb_ptr,
        start_depth,
        end_depth);

    refinement_prediction_loop(
        sequence_control_set_ptr,
        picture_control_set_ptr,
        sb_ptr,
        sb_ptr->sb_index,
        context_ptr);

    forward_cu_to_mode_decision(
        sequence_control_set_ptr,
        picture_control_set_ptr,
        sb_ptr->sb_index,
        context_ptr);

    return return_error;

}

/******************************************************
* Predict the LCU partitionning
******************************************************/
void picture_depth_open_loop(
    ModeDecisionConfigurationContext *context_ptr,
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr) {

    for (int sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {
        early_mode_decision_sb(
            sequence_control_set_ptr,
            picture_control_set_ptr,
            context_ptr,
            picture_control_set_ptr->sb_ptr_array[sb_index]);
    }
}

void sb_depth_open_loop(
    ModeDecisionConfigurationContext *context_ptr,
    SequenceControlSet               *sequence_control_set_ptr,
    PictureControlSet                *picture_control_set_ptr,
    uint32_t                          sb_index) {

    early_mode_decision_sb(
        sequence_control_set_ptr,
        picture_control_set_ptr,
        context_ptr,
        picture_control_set_ptr->sb_ptr_array[sb_index]);
}

void sb_depth_85_block(
    SequenceControlSet *sequence_control_set_ptr,
    PictureControlSet  *picture_control_set_ptr,
    uint32_t            sb_index)
{

    EB_BOOL    split_flag;
    SbParams  *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
    MdcSbData *results_ptr = &picture_control_set_ptr->mdc_sb_data_array[sb_index];

    results_ptr->block_count = 0;
    uint16_t block_index = 0;

    while (block_index < EP_BLOCK_MAX_COUNT)
    {
        split_flag = EB_TRUE;

        const EpBlockStats *ep_block_stats_ptr = ep_get_block_stats(block_index);
        uint8_t depth = ep_block_stats_ptr->depth;
        if (sb_params->ep_scan_block_validity[block_index] && ep_block_stats_ptr->shape == PART_N)
        {
            switch (depth) {
#if INTRA_4x4_SB_DEPTH_84_85
            case 0:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;
            case 1:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;
            case 2:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;
            case 3:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;
            case 4:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;
                break;
#else
            case 0:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;

            case 1:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;

            case 2:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;

            case 3:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;
                break;
#endif
            }
        }

        block_index += (split_flag == EB_FALSE) ? sq_depth_offset[depth] : nsq_depth_offset[depth];
    }
}

void sb_depth_84_block(
    SequenceControlSet *sequence_control_set_ptr,
    PictureControlSet  *picture_control_set_ptr,
    uint32_t            sb_index)
{

    EB_BOOL split_flag;
    SbParams  *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
    MdcSbData *results_ptr = &picture_control_set_ptr->mdc_sb_data_array[sb_index];

    results_ptr->block_count = 0;
    uint16_t block_index = 0;

    while (block_index < EP_BLOCK_MAX_COUNT)
    {
        split_flag = EB_TRUE;

        const EpBlockStats *ep_block_stats_ptr = ep_get_block_stats(block_index);
        uint8_t depth = ep_block_stats_ptr->depth;
        if (sb_params->ep_scan_block_validity[block_index] && ep_block_stats_ptr->shape == PART_N)
        {
            switch (depth) {

#if INTRA_4x4_SB_DEPTH_84_85
            case 0:
                split_flag = EB_TRUE;
                break;
            case 1:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;
            case 2:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;
            case 3:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;
            case 4:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;
                break;
#else
            case 0:
                split_flag = EB_TRUE;
                break;

            case 1:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;

            case 2:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;

            case 3:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;
                break;
#endif
            }
        }

        block_index += (split_flag == EB_FALSE) ? sq_depth_offset[depth] : nsq_depth_offset[depth];
    }
}

void picture_depth_85_block(
    SequenceControlSet *sequence_control_set_ptr,
    PictureControlSet  *picture_control_set_ptr)
{

    uint32_t sb_index;
    EB_BOOL split_flag;

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

        SbParams  *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
        MdcSbData *results_ptr = &picture_control_set_ptr->mdc_sb_data_array[sb_index];

        results_ptr->block_count = 0;
        uint16_t block_index = 0;

        while (block_index < EP_BLOCK_MAX_COUNT)
        {
            split_flag = EB_TRUE;

            const EpBlockStats *ep_block_stats_ptr = ep_get_block_stats(block_index);
            uint8_t depth = ep_block_stats_ptr->depth;
            if (sb_params->ep_scan_block_validity[block_index] && ep_block_stats_ptr->shape == PART_N)
            {
                switch (depth) {
#if INTRA_4x4_SB_DEPTH_84_85
                case 0:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                    break;
                case 1:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                    break;
                case 2:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                    break;
                case 3:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                    break;
                case 4:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;
                    break;
#else
                case 0:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                    break;

                case 1:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                    break;

                case 2:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                    break;

                case 3:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;
                    break;
#endif
                }
            }

            block_index += (split_flag == EB_FALSE) ? sq_depth_offset[depth] : nsq_depth_offset[depth];
        }
    }

}

void picture_depth_84_block(
    SequenceControlSet *sequence_control_set_ptr,
    PictureControlSet  *picture_control_set_ptr)
{

    uint32_t sb_index;
    EB_BOOL split_flag;

    for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

        SbParams  *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
        MdcSbData *results_ptr = &picture_control_set_ptr->mdc_sb_data_array[sb_index];

        results_ptr->block_count = 0;
        uint16_t block_index = 0;

        while (block_index < EP_BLOCK_MAX_COUNT)
        {
            split_flag = EB_TRUE;

            const EpBlockStats *ep_block_stats_ptr = ep_get_block_stats(block_index);
            uint8_t depth = ep_block_stats_ptr->depth;
            if (sb_params->ep_scan_block_validity[block_index] && ep_block_stats_ptr->shape == PART_N)
            {
                switch (depth) {
#if INTRA_4x4_I_SLICE
                case 0:
                    split_flag = EB_TRUE;
                    break;
                case 1:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                    break;
                case 2:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                    break;
                case 3:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                    break;
                case 4:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;
                    break;
#else
                case 0:
                    split_flag = EB_TRUE;
                    break;
                case 1:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                    break;
                case 2:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                    break;
                case 3:
                    results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                    results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;
                    break;
#endif
                }
            }
            block_index += (split_flag == EB_FALSE) ? sq_depth_offset[depth] : nsq_depth_offset[depth];
        }
    }
}

void sb_depth_8x8_16x16_block(
    SequenceControlSet *sequence_control_set_ptr,
    PictureControlSet  *picture_control_set_ptr,
    uint32_t            sb_index)
{

    EB_BOOL split_flag;
    SbParams  *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
    MdcSbData *results_ptr = &picture_control_set_ptr->mdc_sb_data_array[sb_index];

    results_ptr->block_count = 0;
    uint16_t block_index = 0;

    while (block_index < EP_BLOCK_MAX_COUNT)
    {
        split_flag = EB_TRUE;

        const EpBlockStats *ep_block_stats_ptr = ep_get_block_stats(block_index);
        uint8_t depth = ep_block_stats_ptr->depth;
        if (sb_params->ep_scan_block_validity[block_index] && ep_block_stats_ptr->shape == PART_N)
        {
            switch (depth) {
            case 0:
                split_flag = EB_TRUE;
                break;
            case 1:
                split_flag = EB_TRUE;
                break;
            case 2:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_TRUE;
                break;
            case 3:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;
                break;
            }
        }

        block_index += (split_flag == EB_FALSE) ? sq_depth_offset[depth] : nsq_depth_offset[depth];
    }
}

void sb_depth_16x16_block(
    SequenceControlSet *sequence_control_set_ptr,
    PictureControlSet  *picture_control_set_ptr,
    uint32_t            sb_index)
{

    EB_BOOL split_flag;

    SbParams  *sb_params = &sequence_control_set_ptr->sb_params_array[sb_index];
    MdcSbData *results_ptr = &picture_control_set_ptr->mdc_sb_data_array[sb_index];

    results_ptr->block_count = 0;
    uint16_t block_index = 0;

    while (block_index < EP_BLOCK_MAX_COUNT)
    {
        split_flag = EB_TRUE;

        const EpBlockStats *ep_block_stats_ptr = ep_get_block_stats(block_index);
        uint8_t depth = ep_block_stats_ptr->depth;
        if (sb_params->ep_scan_block_validity[block_index] && ep_block_stats_ptr->shape == PART_N)
        {
            switch (depth) {
            case 0:
                split_flag = EB_TRUE;
                break;
            case 1:
                split_flag = EB_TRUE;
                break;
            case 2:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;
                break;
            case 3:
                results_ptr->block_data_array[results_ptr->block_count].block_index = block_index;
                results_ptr->block_data_array[results_ptr->block_count++].split_flag = split_flag = EB_FALSE;
                break;
            }
        }
        block_index += (split_flag == EB_FALSE) ? sq_depth_offset[depth] : nsq_depth_offset[depth];
    }
}

#if BEA
#define MAX_DELTA_QINDEX 80
#define DELTA_QINDEX_SEGMENTS 8
EbErrorType qpm_derive_bea(
    ModeDecisionConfigurationContext      *context_ptr,
    PictureControlSet                       *picture_control_set_ptr,
    SequenceControlSet                    *sequence_control_set_ptr)
{

    EbErrorType     return_error = EB_ErrorNone;
    SbUnit       *sb_ptr;
    int64_t           non_moving_index_distance;
    int32_t           non_moving_weight = MAX_DELTA_QINDEX;
    int                             non_moving_delta_qp;
    int non_moving_delta_qp_temp;
    if (picture_control_set_ptr->slice_type == 2)
        non_moving_weight = MAX_DELTA_QINDEX;
    else if (picture_control_set_ptr->temporal_layer_index == 0)
        non_moving_weight = MAX_DELTA_QINDEX / 2;
    else if (picture_control_set_ptr->temporal_layer_index == 1)
        non_moving_weight = MAX_DELTA_QINDEX / 4;
#if BEA_SCENE_CHANGE
    if (picture_control_set_ptr->parent_pcs_ptr->scene_change_flag)
        non_moving_weight = non_moving_weight / 2;
#endif

    //else
    //    non_moving_weight = 1;

    uint32_t distanceRatio = (uint32_t)~0;

    for (int sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {
        sb_ptr = picture_control_set_ptr->sb_ptr_array[sb_index];
        if (picture_control_set_ptr->parent_pcs_ptr->non_moving_index_min_distance != 0 && picture_control_set_ptr->parent_pcs_ptr->non_moving_index_max_distance != 0) {
            distanceRatio = (picture_control_set_ptr->parent_pcs_ptr->non_moving_index_max_distance > picture_control_set_ptr->parent_pcs_ptr->non_moving_index_min_distance) ? abs(picture_control_set_ptr->parent_pcs_ptr->non_moving_index_max_distance * 100 / picture_control_set_ptr->parent_pcs_ptr->non_moving_index_min_distance) : abs(picture_control_set_ptr->parent_pcs_ptr->non_moving_index_min_distance * 100 / picture_control_set_ptr->parent_pcs_ptr->non_moving_index_max_distance);
        }

        //if (distanceRatio >= BEA_DISTANSE_RATIO_T0) {
        //    non_moving_weight = 1;
        //}

        non_moving_index_distance = (int32_t)picture_control_set_ptr->parent_pcs_ptr->non_moving_index_array[sb_index] - (int32_t)picture_control_set_ptr->parent_pcs_ptr->non_moving_average_score;

        if (non_moving_index_distance < 0) {

            non_moving_delta_qp = (picture_control_set_ptr->parent_pcs_ptr->non_moving_index_min_distance != 0) ? (int8_t)((non_moving_weight * non_moving_index_distance) / picture_control_set_ptr->parent_pcs_ptr->non_moving_index_min_distance) : 0;
            non_moving_delta_qp_temp = (non_moving_delta_qp * (DELTA_QINDEX_SEGMENTS - 2) / non_moving_weight);
            non_moving_delta_qp = (non_moving_delta_qp * (DELTA_QINDEX_SEGMENTS - 2) / non_moving_weight)* non_moving_weight / (DELTA_QINDEX_SEGMENTS - 2);
            sb_ptr->segment_id = CLIP3(1, DELTA_QINDEX_SEGMENTS - 1, 1 - non_moving_delta_qp_temp);
            //sb_ptr->segment_id = CLIP3( 1, DELTA_QINDEX_SEGMENTS - 1, 1 +  (-non_moving_delta_qp * (DELTA_QINDEX_SEGMENTS - 2) / non_moving_weight));
        }
        else {
            non_moving_delta_qp = (picture_control_set_ptr->parent_pcs_ptr->non_moving_index_max_distance != 0) ? (int8_t)((non_moving_index_distance * 100) / (picture_control_set_ptr->parent_pcs_ptr->non_moving_index_max_distance)) : 0;
            non_moving_delta_qp = (non_moving_delta_qp + 50) / 100;
            sb_ptr->segment_id = CLIP3(0, 1, 1 + -non_moving_delta_qp);

            if (non_moving_delta_qp) {
                if (picture_control_set_ptr->slice_type == 2)
                    non_moving_delta_qp = 8;
                else if (picture_control_set_ptr->temporal_layer_index == 0)
                    non_moving_delta_qp = 4;
                else if (picture_control_set_ptr->temporal_layer_index == 1)
                    non_moving_delta_qp = 2;

            }

        }
        context_ptr->qindex_delta[sb_ptr->segment_id] = non_moving_delta_qp;
        picture_control_set_ptr->segment_counts[sb_ptr->segment_id]++;
    }
    return return_error;
}
#endif

/******************************************************
 * Mode Decision Configuration Kernel
 ******************************************************/
void* eb_vp9_mode_decision_configuration_kernel(void *input_ptr)
{
    // Context & SCS & PCS
    ModeDecisionConfigurationContext *context_ptr = (ModeDecisionConfigurationContext*) input_ptr;
    PictureControlSet                *picture_control_set_ptr;
    SequenceControlSet               *sequence_control_set_ptr;

    // Input
    EbObjectWrapper                  *rate_control_results_wrapper_ptr;
    RateControlResults               *rate_control_results_ptr;

    // Output
    EbObjectWrapper                  *enc_dec_tasks_wrapper_ptr;
    EncDecTasks                      *enc_dec_tasks_ptr;
    uint32_t                          picture_width_in_sb;

    for(;;) {

        // Get RateControl Results
        eb_vp9_get_full_object(
            context_ptr->rate_control_input_fifo_ptr,
            &rate_control_results_wrapper_ptr);

        rate_control_results_ptr = (RateControlResults*)rate_control_results_wrapper_ptr->object_ptr;
        picture_control_set_ptr = (PictureControlSet  *)rate_control_results_ptr->picture_control_set_wrapper_ptr->object_ptr;
        sequence_control_set_ptr = (SequenceControlSet *)picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;

        // Mode Decision Configuration Kernel Signal(s) derivation
        if (sequence_control_set_ptr->static_config.tune == TUNE_SQ) {
            eb_vp9_signal_derivation_mode_decision_config_kernel_sq(
                picture_control_set_ptr,
                context_ptr);
        }
        else {
            eb_vp9_signal_derivation_mode_decision_config_kernel_oq_vmaf(
                picture_control_set_ptr,
                context_ptr);
        }

#if VP9_RD
        // Initialize the rd cost
        // Hsan: should be done after QP generation (to clean up)
        cal_nmvjointsadcost(picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvjointsadcost);
        picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvcost[0] = &picture_control_set_ptr->parent_pcs_ptr->cpi->nmvcosts[0][MV_MAX];
        picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvcost[1] = &picture_control_set_ptr->parent_pcs_ptr->cpi->nmvcosts[1][MV_MAX];
        picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvsadcost[0] = &picture_control_set_ptr->parent_pcs_ptr->cpi->nmvsadcosts[0][MV_MAX];
        picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvsadcost[1] = &picture_control_set_ptr->parent_pcs_ptr->cpi->nmvsadcosts[1][MV_MAX];
        cal_nmvsadcosts(picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvsadcost);

        picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvcost_hp[0] = &picture_control_set_ptr->parent_pcs_ptr->cpi->nmvcosts_hp[0][MV_MAX];
        picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvcost_hp[1] = &picture_control_set_ptr->parent_pcs_ptr->cpi->nmvcosts_hp[1][MV_MAX];
        picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvsadcost_hp[0] = &picture_control_set_ptr->parent_pcs_ptr->cpi->nmvsadcosts_hp[0][MV_MAX];
        picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvsadcost_hp[1] = &picture_control_set_ptr->parent_pcs_ptr->cpi->nmvsadcosts_hp[1][MV_MAX];
        cal_nmvsadcosts_hp(picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvsadcost_hp);

        eb_vp9_initialize_rd_consts(picture_control_set_ptr->parent_pcs_ptr->cpi);

        if (picture_control_set_ptr->parent_pcs_ptr->cpi->common.allow_high_precision_mv) {
            picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.mvcost = picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvcost_hp;
            picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.mvsadcost = picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvsadcost_hp;
        }
        else {
            picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.mvcost = picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvcost;
            picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.mvsadcost = picture_control_set_ptr->parent_pcs_ptr->cpi->td.mb.nmvsadcost;
        }
#endif

#if SEG_SUPPORT
#if BEA
        for (int segment_index = 0; segment_index < DELTA_QINDEX_SEGMENTS; ++segment_index) {
            context_ptr->qindex_delta[segment_index] = 0;
        }
        VP9_COMP *cpi = picture_control_set_ptr->parent_pcs_ptr->cpi;
        VP9_COMMON *const cm = &cpi->common;
        struct segmentation *const seg = &cm->seg;

        eb_vp9_disable_segmentation(seg);
        eb_vp9_clearall_segfeatures(seg);

        if (sequence_control_set_ptr->static_config.rate_control_mode == 2 && picture_control_set_ptr->temporal_layer_index < 1 && picture_control_set_ptr->parent_pcs_ptr->non_moving_average_score > 5) {
            qpm_derive_bea(
                context_ptr,
                picture_control_set_ptr,
                sequence_control_set_ptr);

            eb_vp9_enable_segmentation(seg);
            // Select delta coding method.
            seg->abs_delta = SEGMENT_DELTADATA;

            // Use some of the segments for in frame Q adjustment.
            for (int segment = 0; segment < DELTA_QINDEX_SEGMENTS; ++segment) {
                int qindex_delta;

                qindex_delta = context_ptr->qindex_delta[segment];
                //if ((cm->base_qindex + qindex_delta) > 0) {
                if ((cm->base_qindex + qindex_delta) > 0 && picture_control_set_ptr->segment_counts[segment]) {
                    eb_vp9_enable_segfeature(seg, segment, SEG_LVL_ALT_Q);
                    eb_vp9_set_segdata(seg, segment, SEG_LVL_ALT_Q, qindex_delta);
                }
            }
        }
#endif
#endif
        picture_width_in_sb = (sequence_control_set_ptr->luma_width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE;

        context_ptr->qp = picture_control_set_ptr->picture_qp;

        picture_control_set_ptr->scene_characteristic_id = EB_FRAME_CARAC_0;

        EB_PICNOISE_CLASS pic_noise_classTH = (picture_control_set_ptr->parent_pcs_ptr->noise_detection_th == 0) ? PIC_NOISE_CLASS_1 : PIC_NOISE_CLASS_3;

        picture_control_set_ptr->scene_characteristic_id = (
            (!picture_control_set_ptr->parent_pcs_ptr->is_pan) &&
            (!picture_control_set_ptr->parent_pcs_ptr->is_tilt) &&
            (picture_control_set_ptr->parent_pcs_ptr->grass_percentage_in_picture > 0) &&
            (picture_control_set_ptr->parent_pcs_ptr->grass_percentage_in_picture <= 35) &&
            (picture_control_set_ptr->parent_pcs_ptr->pic_noise_class >= pic_noise_classTH) &&
            (picture_control_set_ptr->parent_pcs_ptr->pic_homogenous_over_time_sb_percentage < 50)) ? EB_FRAME_CARAC_1 : picture_control_set_ptr->scene_characteristic_id;

        picture_control_set_ptr->scene_characteristic_id = (
            (picture_control_set_ptr->parent_pcs_ptr->is_pan) &&
            (!picture_control_set_ptr->parent_pcs_ptr->is_tilt) &&
            (picture_control_set_ptr->parent_pcs_ptr->grass_percentage_in_picture > 35) &&
            (picture_control_set_ptr->parent_pcs_ptr->grass_percentage_in_picture <= 70) &&
            (picture_control_set_ptr->parent_pcs_ptr->pic_noise_class >= pic_noise_classTH) &&
            (picture_control_set_ptr->parent_pcs_ptr->pic_homogenous_over_time_sb_percentage < 50)) ? EB_FRAME_CARAC_2 : picture_control_set_ptr->scene_characteristic_id;

        // Aura Detection: uses the picture QP to derive aura thresholds, therefore it could not move to the open loop
        aura_detection(
            sequence_control_set_ptr,
            picture_control_set_ptr);

        // Detect complex/non-flat/moving LCU in a non-complex area (used to refine MDC depth control)
        complex_non_flat_moving_sb(
            sequence_control_set_ptr,
            picture_control_set_ptr,
            picture_width_in_sb);

        if (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_SB_SWITCH_DEPTH_MODE) {

            eb_vp9_derive_sb_md_mode(
                sequence_control_set_ptr,
                picture_control_set_ptr,
                context_ptr);

            for (int sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {
                if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_FULL85_DEPTH_MODE) {
                    sb_depth_85_block(
                        sequence_control_set_ptr,
                        picture_control_set_ptr,
                        sb_index);
                }
                else if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_FULL84_DEPTH_MODE) {
                    sb_depth_84_block(
                        sequence_control_set_ptr,
                        picture_control_set_ptr,
                        sb_index);
                }
                else if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_AVC_DEPTH_MODE) {
                    sb_depth_8x8_16x16_block(
                        sequence_control_set_ptr,
                        picture_control_set_ptr,
                        sb_index);
                }
                else if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_LIGHT_AVC_DEPTH_MODE) {
                    sb_depth_16x16_block(
                        sequence_control_set_ptr,
                        picture_control_set_ptr,
                        sb_index);
                }
                else if (picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_OPEN_LOOP_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_LIGHT_OPEN_LOOP_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_PRED_OPEN_LOOP_DEPTH_MODE || picture_control_set_ptr->parent_pcs_ptr->sb_depth_mode_array[sb_index] == SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE) {
                    sb_depth_open_loop(
                        context_ptr,
                        sequence_control_set_ptr,
                        picture_control_set_ptr,
                        sb_index);
                }
            }
        }
        else  if (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_FULL85_DEPTH_MODE) {
            picture_depth_85_block(
                sequence_control_set_ptr,
                picture_control_set_ptr);
        }
        else  if (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_FULL84_DEPTH_MODE) {
            picture_depth_84_block(
                sequence_control_set_ptr,
                picture_control_set_ptr);
        }
        else if (picture_control_set_ptr->parent_pcs_ptr->pic_depth_mode == PIC_OPEN_LOOP_DEPTH_MODE) {
            picture_depth_open_loop(
                context_ptr,
                sequence_control_set_ptr,
                picture_control_set_ptr);
        }

        // Post the results to the MD processes
        eb_vp9_get_empty_object(
            context_ptr->mode_decision_configuration_output_fifo_ptr,
            &enc_dec_tasks_wrapper_ptr);

        enc_dec_tasks_ptr = (EncDecTasks*) enc_dec_tasks_wrapper_ptr->object_ptr;
        enc_dec_tasks_ptr->picture_control_set_wrapper_ptr = rate_control_results_ptr->picture_control_set_wrapper_ptr;
        enc_dec_tasks_ptr->input_type = ENCDEC_TASKS_MDC_INPUT;

        // Post the Full Results Object
        eb_vp9_post_full_object(enc_dec_tasks_wrapper_ptr);

        // Release Rate Control Results
        eb_vp9_release_object(rate_control_results_wrapper_ptr);

    }

    return EB_NULL;
}
