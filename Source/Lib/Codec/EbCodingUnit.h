/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbCodingUnit_h
#define EbCodingUnit_h

#include "EbDefinitions.h"
#include "EbSyntaxElements.h"
#include "EbMotionEstimationLcuResults.h"
#include "EbDefinitions.h"
#include "EbPictureBufferDesc.h"

#include "vp9_enums.h"
#include "vp9_blockd.h"
#include "vp9_block.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PictureControlSet;

#define MAX_BLOCK_COST 0xFFFFFFFFFFFFFFFFull
#define INVALID_FAST_CANDIDATE_INDEX    ~0

typedef struct CodingUnit
{
    uint8_t        split_flag;
    int            partition_context;
    uint16_t       eob[MAX_MB_PLANE][4];
    MbModeInfoExt *mbmi_ext;
    PARTITION_TYPE part;
    Part           shape;
    uint16_t       ep_block_index;

} CodingUnit;

typedef struct EdgeSbResults
{
    uint8_t  edge_block_num;
    uint8_t  isolated_high_intensity_sb;

} EdgeSbResults;

typedef struct SbUnit
{
    struct PictureControlSet *picture_control_set_ptr;
    CodingUnit              **coded_block_array_ptr;

    // Coding Units
    EB_AURA_STATUS            aura_status;

    unsigned                  pred64     : 2;
    unsigned                  sb_index   : 14; // supports up to 8k resolution
    unsigned                  origin_x   : 13; // supports up to 8k resolution 8191
    unsigned                  origin_y   : 13; // supports up to 8k resolution 8191

    //Bits only used for quantized coeffs
    uint32_t                  sb_total_bits;

    // Quantized Coefficients
#if VP9_PERFORM_EP
    int16_t                  *quantized_coeff_buffer[MAX_MB_PLANE];

    int                       quantized_coeff_buffer_block_offset[MAX_MB_PLANE];
#else
    EbPictureBufferDesc      *quantized_coeff;
#endif

#if BEA
    int8_t                    segment_id;
#endif
} SbUnit;

extern EbErrorType sb_unit_ctor(
    SbUnit                  **sb_unit_dbl_ptr,
    uint32_t                  picture_width,
    uint32_t                  picture_height,
    uint16_t                  sb_origin_x,
    uint16_t                  sb_origin_y,
    uint16_t                  sb_index,
    struct PictureControlSet *picture_control_set);

#ifdef __cplusplus
}
#endif
#endif // EbCodingUnit_h
