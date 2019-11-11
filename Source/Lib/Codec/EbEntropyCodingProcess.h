/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEntropyCodingProcess_h
#define EbEntropyCodingProcess_h

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
#include "vp9_blockd.h"

/**************************************
 * Entropy Coding Context
 **************************************/
typedef struct EntropyCodingContext
{
    EbFifo             *enc_dec_input_fifo_ptr;
    EbFifo             *entropy_coding_output_fifo_ptr;
    EbFifo             *rate_control_output_fifo_ptr;

    CodingUnit         *block_ptr;
    const EpBlockStats *ep_block_stats_ptr;

    uint32_t            block_width;
    uint32_t            block_height;
    uint32_t            block_origin_x;
    uint32_t            block_origin_y;

    EB_BOOL             is16bit;

    int                 mi_row;
    int                 mi_col;

    MACROBLOCKD        *e_mbd;

    TOKENEXTRA         *tok;
    TOKENEXTRA         *tok_start;
    TOKENEXTRA         *tok_end;

} EntropyCodingContext;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType eb_vp9_entropy_coding_context_ctor(
    EntropyCodingContext **context_dbl_ptr,
    EbFifo                *enc_dec_input_fifo_ptr,
    EbFifo                *packetization_output_fifo_ptr,
    EbFifo                *rate_control_output_fifo_ptr,
    EB_BOOL                is16bit);

extern void* eb_vp9_entropy_coding_kernel(void *input_ptr);

#endif // EbEntropyCodingProcess_h
