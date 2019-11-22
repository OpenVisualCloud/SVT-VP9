/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEntropyCoding_h
#define EbEntropyCoding_h

#include "EbDefinitions.h"
#include "EbEntropyCodingObject.h"
#include "EbCodingUnit.h"
#include "EbPictureBufferDesc.h"
#include "EbSequenceControlSet.h"
#include "EbPictureControlSet.h"

#include "EbModeDecision.h"
#include "EbIntraPrediction.h"
#include "EbBitstreamUnit.h"
#include "EbPacketizationProcess.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType eb_vp9_reset_bitstream(
    EbPtr bitstream_ptr);

#ifdef __cplusplus
}
#endif
#endif //EbEntropyCoding_h
