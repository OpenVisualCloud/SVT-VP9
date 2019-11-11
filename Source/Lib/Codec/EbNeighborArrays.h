/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbNeighborArrays_h
#define EbNeighborArrays_h

#include "EbDefinitions.h"
#include "EbSyntaxElements.h"

#ifdef __cplusplus
extern "C" {
#endif
// Neighbor Array Granulairity
#define SB_NEIGHBOR_ARRAY_GRANULARITY                  64
#define SAMPLE_NEIGHBOR_ARRAY_GRANULARITY               1

typedef enum NeighborArrayType
{
    NEIGHBOR_ARRAY_LEFT     = 0,
    NEIGHBOR_ARRAY_TOP      = 1,
    NEIGHBOR_ARRAY_TOPLEFT  = 2,
    NEIGHBOR_ARRAY_INVALID  = ~0
} NeighborArrayType;

#define NEIGHBOR_ARRAY_UNIT_LEFT_MASK                   (1 << NEIGHBOR_ARRAY_LEFT)
#define NEIGHBOR_ARRAY_UNIT_TOP_MASK                    (1 << NEIGHBOR_ARRAY_TOP)
#define NEIGHBOR_ARRAY_UNIT_TOPLEFT_MASK                (1 << NEIGHBOR_ARRAY_TOPLEFT)

#define NEIGHBOR_ARRAY_UNIT_FULL_MASK                   (NEIGHBOR_ARRAY_UNIT_LEFT_MASK | NEIGHBOR_ARRAY_UNIT_TOP_MASK | NEIGHBOR_ARRAY_UNIT_TOPLEFT_MASK)
#define NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK      (NEIGHBOR_ARRAY_UNIT_LEFT_MASK | NEIGHBOR_ARRAY_UNIT_TOP_MASK)

typedef struct NeighborArrayUnit
{
    uint8_t   *left_array;
    uint8_t   *top_array;
    uint8_t   *top_left_array;
    uint16_t   left_array_size;
    uint16_t   top_array_size;
    uint16_t   top_left_array_size;
    uint8_t    unit_size;
    uint8_t    granularity_normal;
    uint8_t    granularity_normal_log2;
    uint8_t    granularity_top_left;
    uint8_t    granularity_top_left_log2;

} NeighborArrayUnit;

extern EbErrorType eb_vp9_neighbor_array_unit_ctor(
    NeighborArrayUnit **na_unit_dbl_ptr,
    uint32_t            maxpicture_width,
    uint32_t            maxpicture_height,
    uint32_t            unit_size,
    uint32_t            granularity_normal,
    uint32_t            granularity_top_left,
    uint32_t            type_mask);

extern void eb_vp9_neighbor_array_unit_reset(NeighborArrayUnit *na_unit_ptr);

extern uint32_t get_neighbor_array_unit_left_index(
    NeighborArrayUnit *na_unit_ptr,
    uint32_t           loc_y);

extern uint32_t get_neighbor_array_unit_top_index(
    NeighborArrayUnit *na_unit_ptr,
    uint32_t           loc_x);

extern uint32_t eb_vp9_get_neighbor_array_unit_top_left_index(
    NeighborArrayUnit *na_unit_ptr,
    int32_t            loc_x,
    int32_t            loc_y);

extern void eb_vp9_neighbor_array_unit_sample_write(
    NeighborArrayUnit *na_unit_ptr,
    uint8_t           *src_ptr,
    uint32_t           stride,
    uint32_t           srcorigin_x,
    uint32_t           srcorigin_y,
    uint32_t           picorigin_x,
    uint32_t           picorigin_y,
    uint32_t           block_width,
    uint32_t           block_height,
    uint32_t           neighbor_array_type_mask);

extern void eb_vp9_neighbor_array_unit16bit_sample_write(
    NeighborArrayUnit *na_unit_ptr,
    uint16_t          *src_ptr,
    uint32_t           stride,
    uint32_t           srcorigin_x,
    uint32_t           srcorigin_y,
    uint32_t           picorigin_x,
    uint32_t           picorigin_y,
    uint32_t           block_width,
    uint32_t           block_height,
    uint32_t           neighbor_array_type_mask);

extern void eb_vp9_neighbor_array_unit_mode_write(
    NeighborArrayUnit *na_unit_ptr,
    uint8_t           *value,
    uint32_t           origin_x,
    uint32_t           origin_y,
    uint32_t           block_width,
    uint32_t           block_height,
    uint32_t           neighbor_array_type_mask);

extern void neighbor_array_unit_intra_write(
    NeighborArrayUnit *na_unit_ptr,
    uint8_t           *value,
    uint32_t           origin_x,
    uint32_t           origin_y,
    uint32_t           block_size);

extern void neighbor_array_unit_mode_type_write(
    NeighborArrayUnit *na_unit_ptr,
    uint8_t           *value,
    uint32_t           origin_x,
    uint32_t           origin_y,
    uint32_t           block_size);

extern void eb_vp9_neighbor_array_unit_mv_write(
    NeighborArrayUnit *na_unit_ptr,
    uint8_t           *value,
    uint32_t           origin_x,
    uint32_t           origin_y,
    uint32_t           block_size);
#ifdef __cplusplus
}
#endif
#endif //EbNeighborArrays_h
