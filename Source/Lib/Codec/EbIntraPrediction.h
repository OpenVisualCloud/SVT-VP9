/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbIntraPrediction_h
#define EbIntraPrediction_h

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbPictureBufferDesc.h"
#include "EbPictureControlSet.h"
#include "EbCodingUnit.h"
#include "EbPictureControlSet.h"
#include "EbModeDecision.h"
#include "EbNeighborArrays.h"
#include "EbMotionEstimationProcess.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void intra_prediction(
    struct EncDecContext *context_ptr,
    EbByte                pred_buffer,
    uint16_t              pred_stride,
    int                   plane);

extern void inter_prediction(
    struct EncDecContext *context_ptr,
    EbByte                pred_buffer,
    uint16_t              pred_stride,
    int                   plane);

#ifdef __cplusplus
}
#endif
#endif // EbIntraPrediction_h
