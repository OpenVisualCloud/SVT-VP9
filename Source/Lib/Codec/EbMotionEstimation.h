/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMotionEstimation_h
#define EbMotionEstimation_h

#include "EbDefinitions.h"
#include "EbCodingUnit.h"

#include "EbMotionEstimationProcess.h"
#include "EbMotionEstimationContext.h"
#include "EbPictureBufferDesc.h"
#include "EbSequenceControlSet.h"
#include "EbReferenceObject.h"
#include "EbPictureDecisionProcess.h"
#ifdef __cplusplus
extern "C" {
#endif
extern EbErrorType motion_estimate_sb(
    PictureParentControlSet *picture_control_set_ptr,
    uint32_t                 sb_index,
    uint32_t                 sb_origin_x,
    uint32_t                 sb_origin_y,
    MeContext               *context_ptr,
    EbPictureBufferDesc     *input_ptr);

extern void eb_vp9_decimation_2d(
    uint8_t  *input_samples,
    uint32_t  input_stride,
    uint32_t  input_area_width,
    uint32_t  input_area_height,
    uint8_t  *decim_samples,
    uint32_t  decim_stride,
    uint32_t  decim_step);

extern void eb_vp9_get_mv(
    PictureParentControlSet *picture_control_set_ptr,
    uint32_t                 sb_index,
    int32_t                 *x_current_mv,
    int32_t                 *y_current_mv);

int8_t sort3_elements(uint32_t a, uint32_t b, uint32_t c);
#define a_b_c  0
#define a_c_b  1
#define b_a_c  2
#define b_c_a  3
#define c_a_b  4
#define c_b_a  5

#ifdef __cplusplus
}
#endif
#endif // EbMotionEstimation_h
