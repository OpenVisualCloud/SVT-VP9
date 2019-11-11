/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbReferenceObject_h
#define EbReferenceObject_h

#include "EbDefinitions.h"
#include "EbDefinitions.h"
#include "EbPictureControlSet.h"

typedef struct EbReferenceObject
{
    EbPictureBufferDesc *reference_picture;
    EbPictureBufferDesc *reference_picture16bit;
    EbPictureBufferDesc *ref_den_src_picture;

    uint64_t             ref_poc;

    uint8_t              qp;
    EB_SLICE             slice_type;

    uint32_t             non_moving_index_array[MAX_NUMBER_OF_TREEBLOCKS_PER_PICTURE];//array to hold non-moving blocks in reference frames

    uint8_t              tmp_layer_idx;
    EB_BOOL              is_scene_change;
    uint16_t             pic_avg_variance;
    uint8_t              average_intensity;

} EbReferenceObject;

typedef struct EbReferenceObjectDescInitData {
    EbPictureBufferDescInitData reference_picture_desc_init_data;
} EbReferenceObjectDescInitData;

typedef struct EbPaReferenceObject
{
    EbPictureBufferDesc      *input_padded_picture_ptr;
    EbPictureBufferDesc      *quarter_decimated_picture_ptr;
    EbPictureBufferDesc      *sixteenth_decimated_picture_ptr;
    uint16_t                  variance[MAX_NUMBER_OF_TREEBLOCKS_PER_PICTURE];
    uint8_t                   y_mean[MAX_NUMBER_OF_TREEBLOCKS_PER_PICTURE];
    EB_SLICE                  slice_type;
    uint32_t                  dependent_pictures_count; //number of pic using this reference frame
    PictureParentControlSet  *p_pcs_ptr;

} EbPaReferenceObject;

typedef struct EbPaReferenceObjectDescInitData
{
    EbPictureBufferDescInitData reference_picture_desc_init_data;
    EbPictureBufferDescInitData quarter_picture_desc_init_data;
    EbPictureBufferDescInitData sixteenth_picture_desc_init_data;
} EbPaReferenceObjectDescInitData;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType eb_vp9_reference_object_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr);

extern EbErrorType eb_vp9_pa_reference_object_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr);

#endif //EbReferenceObject_h
