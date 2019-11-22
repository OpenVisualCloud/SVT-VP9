/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbModeDecision_h
#define EbModeDecision_h

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbCodingUnit.h"
#include "EbSyntaxElements.h"
#include "EbPictureBufferDesc.h"
#include "EbPictureOperators.h"
#include "EbNeighborArrays.h"
#ifdef __cplusplus
extern "C" {
#endif

struct ModeDecisionCandidateBuffer;
struct ModeDecisionCandidate;
struct EncDecContext;

/**************************************
* Function Ptrs Definitions
**************************************/
typedef void(*EbPredictionFunc)(
    struct EncDecContext *context_ptr,
    EbByte                pred_buffer,
    uint16_t              pred_stride,
    int                   plane);

typedef int64_t(*EbFastCostFunc)(
    PictureControlSet            *picture_control_set_ptr,
    int                           has_uv,
    int                           rd_mult_sad,
    MbModeInfoExt                *mbmi_ext,
    MACROBLOCKD                  *e_mbd,
    uint32_t                     *ref_costs_single,
    uint32_t                     *ref_costs_comp,
    vpx_prob                      comp_mode_p,
    struct ModeDecisionCandidate *candidate_ptr,
    uint64_t                      luma_distortion,
    uint64_t                      chroma_distortion);

typedef EbErrorType (*EbFullCostFunc)(
    struct EncDecContext               *context_ptr,
    struct ModeDecisionCandidateBuffer *candidate_buffer_ptr,
    int                                *y_distortion,
    int                                *cb_distortion,
    int                                *cr_distortion,
    int                                *y_coeff_bits,
    int                                *cb_coeff_bits,
    int                                *cr_coeff_bits,
    PictureControlSet                  *picture_control_set_ptr);

/**************************************
* Mode Decision Candidate
**************************************/
typedef struct ModeDecisionCandidate
{
    uint32_t  me_distortion;
    EB_BOOL   distortion_ready;

    uint64_t  fast_luma_rate;
    uint64_t  fast_chroma_rate;

    ModeInfo *mode_info;

    uint16_t  eob[MAX_MB_PLANE][4];

} ModeDecisionCandidate;

/**************************************
* Mode Decision Candidate Buffer
**************************************/
typedef struct ModeDecisionCandidateBuffer{
    // Candidate Ptr
    ModeDecisionCandidate *candidate_ptr;

    // Video Buffers
    EbPictureBufferDesc   *prediction_ptr;
    EbPictureBufferDesc   *residual_quant_coeff_ptr;
    EbPictureBufferDesc   *recon_coeff_ptr;
    EbPictureBufferDesc   *recon_ptr;

    // Costs
    uint64_t              *fast_cost_ptr;
    uint64_t              *full_cost_ptr;

} ModeDecisionCandidateBuffer;

/**************************************
* Extern Function Declarations
**************************************/
EbErrorType eb_vp9_mode_decision_candidate_buffer_ctor(
    ModeDecisionCandidateBuffer **buffer_dbl_ptr,
    uint16_t                      sb_max_size,
    EbBitDepth                    max_bitdepth,
    uint64_t                     *fast_cost_ptr,
    uint64_t                     *full_cost_ptr);

EbErrorType prepare_fast_loop_candidates(
    PictureControlSet    *picture_control_set_ptr,
    struct EncDecContext *context_ptr,
    SbUnit               *sb_ptr,
    uint32_t             *buffer_total_count_ptr,
    uint32_t             *candidate_total_count_ptr);

uint8_t full_mode_decision(
    struct EncDecContext         *context_ptr,
    ModeDecisionCandidateBuffer **buffer_ptr_array,
    uint32_t                      candidate_total_count,
    uint8_t                      *best_candidate_index_array,
    uint16_t                      ep_8x8_block_index,
    int                           depth_part_stage);

EbErrorType pre_mode_decision(
    uint32_t                      buffer_total_count,
    ModeDecisionCandidateBuffer **buffer_ptr_array,
    uint32_t                     *full_candidate_total_count_ptr,
    uint8_t                      *best_candidate_index_array,
    EB_BOOL                       same_fast_full_candidate);

struct CodingLoopContext;
#ifdef __cplusplus
}
#endif
#endif // EbModeDecision_h
