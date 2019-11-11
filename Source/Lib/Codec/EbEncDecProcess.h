/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEncDecProcess_h
#define EbEncDecProcess_h

#include "EbDefinitions.h"
#include "EbSyntaxElements.h"

#include "EbSystemResourceManager.h"
#include "EbPictureBufferDesc.h"
#include "EbModeDecision.h"

#include "EbEntropyCoding.h"
#include "EbTransQuantBuffers.h"
#include "EbReferenceObject.h"
#include "EbNeighborArrays.h"
#include "EbCodingUnit.h"

#include "vpx_convolve.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************
* Defines
**************************************/
#define MODE_DECISION_CANDIDATE_MAX_COUNT                      128

#define MAX_NFL                                                 11
#define MODE_DECISION_CANDIDATE_BUFFER_MAX_COUNT   (MAX_NFL+1) * 5
#define MAX_FULL_LOOP_CANIDATES_PER_DEPTH                  MAX_NFL

/**************************************
* Macros
**************************************/
#define GROUP_OF_4_8x8_BLOCKS(origin_x,origin_y) (((origin_x >> 3) & 0x1) && ((origin_y >> 3) & 0x1) ? EB_TRUE : EB_FALSE)
#define GROUP_OF_4_16x16_BLOCKS(origin_x,origin_y) (((((origin_x >> 3) & 0x2) == 0x2) && (((origin_y >> 3) & 0x2) == 0x2)) ? EB_TRUE : EB_FALSE)
#define GROUP_OF_4_32x32_BLOCKS(origin_x,origin_y) (((((origin_x >> 3) & 0x4) == 0x4) && (((origin_y >> 3) & 0x4) == 0x4)) ? EB_TRUE : EB_FALSE)

 /**************************************
 * Coding Loop Context
 **************************************/
typedef struct EncDecBlockUnit
{
    ModeInfo     mode_info;
    uint64_t     cost;

} EncDecBlockUnit;

/**************************************
* Enc Dec Context
**************************************/
typedef struct EncDecContext
{
    EbFifo                          *mode_decision_input_fifo_ptr;
    EbFifo                          *enc_dec_output_fifo_ptr;
    EbFifo                          *enc_dec_feedback_fifo_ptr;
    EbFifo                          *picture_demux_output_fifo_ptr;

    // Coding Unit Workspace
    EbPictureBufferDesc             *residual_buffer;
    EbPictureBufferDesc             *transform_buffer;
    EbPictureBufferDesc             *prediction_buffer;
    EbPictureBufferDesc             *input_samples;

    // uv_mode search scratch buffers
    EbPictureBufferDesc             *uv_mode_search_prediction_buffer;
    EbPictureBufferDesc             *uv_mode_search_residual_quant_coeff_buffer;
    EbPictureBufferDesc             *uv_mode_search_recon_coeff_buffer;
    EbPictureBufferDesc             *uv_mode_search_recon_buffer;
    ModeInfo                        *uv_mode_search_mode_info;
    uint16_t                         uv_mode_search_eob[MAX_MB_PLANE][4];
    PREDICTION_MODE                  best_uv_mode[TM_PRED + 1];

    // Context Variables
    CodingUnit                      *block_ptr;
    const EpBlockStats              *ep_block_stats_ptr;
    uint32_t                         ep_block_index;

    uint16_t                         block_origin_x;
    uint16_t                         block_origin_y;

    uint32_t                         sb_index;

    EB_BOOL                          is16bit;
    EbFifo                          *mode_decision_configuration_input_fifo_ptr;

    ModeDecisionCandidate          **fast_candidate_ptr_array;
    ModeDecisionCandidate           *fast_candidate_array;

    ModeDecisionCandidateBuffer    **candidate_buffer_ptr_array;

    // Transform and Quantization Buffers
    EbTransQuantBuffers             *trans_quant_buffers_ptr;

    struct EncDecContext            *enc_dec_context_ptr;

    uint64_t                        *fast_cost_array;
    uint64_t                        *full_cost_array;

    // Fast loop buffers
    uint8_t                          buffer_depth_index_start[MAX_LEVEL_COUNT];
    uint8_t                          buffer_depth_index_width[MAX_LEVEL_COUNT];

    SbUnit                          *sb_ptr;

    // Inter depth decision
    uint8_t                          best_candidate_index_array[MAX_FULL_LOOP_CANIDATES_PER_DEPTH];
    uint32_t                         full_recon_search_count;

    NeighborArrayUnit               *luma_recon_neighbor_array;
    NeighborArrayUnit               *cb_recon_neighbor_array;
    NeighborArrayUnit               *cr_recon_neighbor_array;

    PARTITION_CONTEXT               *above_seg_context;
    PARTITION_CONTEXT               *left_seg_context;
    ENTROPY_CONTEXT                 *above_context;
    ENTROPY_CONTEXT                 *left_context;

    EncDecBlockUnit                **enc_dec_local_block_array;

    EB_BOOL                          restrict_intra_global_motion;
    uint8_t                          use_subpel_flag;

    // Multi-modes signal(s)
    EB_BOOL                          spatial_sse_full_loop;
    EB_BOOL                          full_loop_escape;
    EB_BOOL                          single_fast_loop_flag;
    EB_BOOL                          nearest_injection;
    EB_BOOL                          near_injection;
    EB_BOOL                          zero_injection;
    EB_BOOL                          unipred3x3_injection;
    EB_BOOL                          bipred3x3_injection;
    uint8_t                          chroma_level;
    EB_BOOL                          intra_md_open_loop_flag;
    EB_BOOL                          limit_intra;

    uint8_t                          pf_md_level;
    uint8_t                          nfl_level;

    // Hsan: how to avoid intra_above_ref and intra_left_ref (lossless optimization toward faster generate_intra_reference_samples).
    uint8_t                          *intra_above_ref;
    uint8_t                          *intra_left_ref;

    uint8_t                           left_col[MAX_MB_PLANE][32];
    uint8_t                           above_data[MAX_MB_PLANE][80];
    const uint8_t                    *const_above_row[MAX_MB_PLANE];

    ModeInfo                        **mode_info_array;

    int                               mi_row;
    int                               mi_col;
    int                               mi_stride; // Hsan : to remove
    int                               bmi_index; // Hsan : to access bmi under mode info (apply minimum changes to WebM kernels)
    MACROBLOCKD                      *e_mbd;

    uint32_t                          input_origin_index;
    uint32_t                          input_chroma_origin_index;

    uint32_t                          block_origin_index;
    uint32_t                          block_chroma_origin_index;

    EbPictureBufferDesc              *ref_pic_list[2];
    EbPictureBufferDesc              *recon_buffer;

    convolve_fn_t                     predict[2][2][2];  // horiz, vert, avg
    struct scale_factors             *sf;
    int                               use_ref_mvs_flag[MAX_REF_FRAMES];

#if VP9_RD
    uint32_t                          ref_costs_single[MAX_REF_FRAMES];
    uint32_t                          ref_costs_comp[MAX_REF_FRAMES];
    vpx_prob                          comp_mode_p;
    ENTROPY_CONTEXT                   t_above[3][16];
    ENTROPY_CONTEXT                   t_left[3][16];
#endif

    EB_BOOL                           skip_eob_zero_mode_ep;
    EB_BOOL                           eob_zero_mode;

    // Multi-modes signal(s)
    EB_BOOL                           allow_enc_dec_mismatch;

    BdpSbData                         bdp_block_data;
    EbPictureBufferDesc              *bdp_pillar_scratch_recon_buffer;

    // Hsan: use macro definitions here
    int                               depth_part_stage; // 0: mdc_depth_part or bdp_depth_part @ nearest/near stage, 1: bdp_depth_part @ pillar stage, 2: bdp_depth_part @ refinement stage

#if SEG_SUPPORT
    int8_t                            segment_id;
#endif
    int                               RDMULT;
    int                               rd_mult_sad;

} EncDecContext  ;

/**************************************
* Extern Function Declarations
**************************************/
extern void eb_vp9_reset_mode_decision(
    EncDecContext     *context_ptr,
    PictureControlSet *picture_control_set_ptr,
    uint32_t           segment_index);

extern EbErrorType eb_vp9_enc_dec_context_ctor(
    EncDecContext **context_dbl_ptr,
    EbFifo         *mode_decision_configuration_input_fifo_ptr,
    EbFifo         *packetization_output_fifo_ptr,
    EbFifo         *feedback_fifo_ptr,
    EbFifo         *picture_demux_fifo_ptr);

extern void* eb_vp9_enc_dec_kernel(void *input_ptr);

#ifdef __cplusplus
}
#endif
#endif // EbEncDecProcess_h
