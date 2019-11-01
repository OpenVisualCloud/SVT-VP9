/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbRateDistortionCost_h
#define EbRateDistortionCost_h

/***************************************
 * Includes
 ***************************************/
#include "EbIntraPrediction.h"

#include "EbEncDecProcess.h"

#ifdef __cplusplus
extern "C" {
#endif

#if VP9_RD
extern void estimate_ref_frame_costs(
    const VP9_COMMON  *cm,
    const MACROBLOCKD *xd,
    int                segment_id,
    uint32_t          *ref_costs_single,
    uint32_t          *ref_costs_comp,
    vpx_prob          *comp_mode_p);
#endif

extern int64_t inter_fast_cost(
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

extern int64_t intra_fast_cost(
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

extern EbErrorType inter_full_cost(
    struct EncDecContext               *context_ptr,
    struct ModeDecisionCandidateBuffer *candidate_buffer_ptr,
    int                                *y_distortion,
    int                                *cb_distortion,
    int                                *cr_distortion,
    int                                *y_coeff_bits,
    int                                *cb_coeff_bits,
    int                                *cr_coeff_bits,
    PictureControlSet                  *picture_control_set_ptr);

extern EbErrorType intra_full_cost(
    struct EncDecContext               *context_ptr,
    struct ModeDecisionCandidateBuffer *candidate_buffer_ptr,
    int                                *y_distortion,
    int                                *cb_distortion,
    int                                *cr_distortion,
    int                                *y_coeff_bits,
    int                                *cb_coeff_bits,
    int                                *cr_coeff_bits,
    PictureControlSet                  *picture_control_set_ptr);

extern EbErrorType get_partition_cost(
    PictureControlSet *picture_control_set_ptr,
    EncDecContext     *context_ptr,
    BLOCK_SIZE         bsize,
    PARTITION_TYPE     partition_type,
    int                partition_context,
    uint64_t          *partition_cost);

extern int coeff_rate_estimate( //cost_coeffs
    struct EncDecContext *context_ptr,
    MACROBLOCK           *x,
    int16_t*              trans_coeff_buffer,
    uint16_t              eob,
    int                   plane,
    int                   block,
    TX_SIZE               tx_size,
    int                   pt, // coeff_ctx
    int                   use_fast_coef_costing);

#ifdef __cplusplus
}
#endif
#endif //EbRateDistortionCost_h
