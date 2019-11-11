/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbMotionEstimationContext.h"

void motion_estimetion_pred_unit_ctor(
    MePredUnit *pu)
{

    pu->distortion = 0xFFFFFFFFull;

    pu->prediction_direction = UNI_PRED_LIST_0;

    pu->Mv[0]        =  0;

    pu->Mv[1]        =  0;

    return;
}

EbErrorType eb_vp9_me_context_ctor(
    MeContext **object_dbl_ptr)
{
    uint32_t list_index;
    uint32_t ref_pic_index;
    uint32_t pu_index;
    uint32_t mecandidate_index;

    EB_MALLOC(MeContext*, *object_dbl_ptr, sizeof(MeContext), EB_N_PTR);

    (*object_dbl_ptr)->x_mv_offset = 0;
    (*object_dbl_ptr)->y_mv_offset = 0;

    // Intermediate LCU-sized buffer to retain the input samples
    (*object_dbl_ptr)->sb_buffer_stride                = MAX_SB_SIZE;
    EB_ALLIGN_MALLOC(uint8_t *, (*object_dbl_ptr)->sb_buffer, sizeof(uint8_t) * MAX_SB_SIZE * (*object_dbl_ptr)->sb_buffer_stride, EB_A_PTR);

    (*object_dbl_ptr)->hme_sb_buffer_stride             = (MAX_SB_SIZE + HME_DECIM_FILTER_TAP - 1);
    EB_MALLOC(uint8_t *, (*object_dbl_ptr)->hme_sb_buffer, sizeof(uint8_t) * (MAX_SB_SIZE + HME_DECIM_FILTER_TAP - 1) * (*object_dbl_ptr)->hme_sb_buffer_stride, EB_N_PTR);

    (*object_dbl_ptr)->quarter_sb_buffer_stride         = (MAX_SB_SIZE >> 1);
    EB_MALLOC(uint8_t *, (*object_dbl_ptr)->quarter_sb_buffer, sizeof(uint8_t) * (MAX_SB_SIZE >> 1) * (*object_dbl_ptr)->quarter_sb_buffer_stride, EB_N_PTR);

    (*object_dbl_ptr)->sixteenth_sb_buffer_stride       = (MAX_SB_SIZE >> 2);
    EB_ALLIGN_MALLOC(uint8_t *, (*object_dbl_ptr)->sixteenth_sb_buffer, sizeof(uint8_t) * (MAX_SB_SIZE >> 2) * (*object_dbl_ptr)->sixteenth_sb_buffer_stride, EB_A_PTR);

    (*object_dbl_ptr)->interpolated_stride             = MAX_SEARCH_AREA_WIDTH;
    EB_MALLOC(uint8_t *, (*object_dbl_ptr)->hme_buffer, sizeof(uint8_t) * (*object_dbl_ptr)->interpolated_stride * MAX_SEARCH_AREA_HEIGHT, EB_N_PTR);

    (*object_dbl_ptr)->hme_buffer_stride                = MAX_SEARCH_AREA_WIDTH;

    EB_MEMSET((*object_dbl_ptr)->sb_buffer, 0 , sizeof(uint8_t) * MAX_SB_SIZE * (*object_dbl_ptr)->sb_buffer_stride);
    EB_MEMSET((*object_dbl_ptr)->hme_sb_buffer, 0 ,sizeof(uint8_t) * (MAX_SB_SIZE + HME_DECIM_FILTER_TAP - 1) * (*object_dbl_ptr)->hme_sb_buffer_stride);

    // 15 intermediate buffers to retain the interpolated reference samples

    //      0    1    2    3
    // 0    A    a    b    c
    // 1    d    e    f    g
    // 2    h    i    j    k
    // 3    n    p    q    r

    //                  _____________
    //                 |             |
    // --I samples --> |Interpolation|-- O samples -->
    //                 | ____________|

    // Before Interpolation: 2 x 3
    //   I   I
    //   I   I
    //   I   I

    // After 1-D Horizontal Interpolation: (2 + 1) x 3 - a, b, and c
    // O I O I O
    // O I O I O
    // O I O I O

    // After 1-D Vertical Interpolation: 2 x (3 + 1) - d, h, and n
    //   O   O
    //   I   I
    //   O   O
    //   I   I
    //   O   O
    //   I   I
    //   O   O

    // After 2-D (Horizontal/Vertical) Interpolation: (2 + 1) x (3 + 1) - e, f, g, i, j, k, n, p, q, and r
    // O   O   O
    //   I   I
    // O   O   O
    //   I   I
    // O   O   O
    //   I   I
    // O   O   O

    for( list_index = 0; list_index < MAX_NUM_OF_REF_PIC_LIST; list_index++) {

        for( ref_pic_index = 0; ref_pic_index < MAX_REF_IDX; ref_pic_index++) {

            EB_MALLOC(uint8_t *, (*object_dbl_ptr)->integer_buffer[list_index][ref_pic_index], sizeof(uint8_t) * (*object_dbl_ptr)->interpolated_stride * MAX_SEARCH_AREA_HEIGHT, EB_N_PTR);

            EB_MALLOC(uint8_t *, (*object_dbl_ptr)->posb_buffer[list_index][ref_pic_index], sizeof(uint8_t) * (*object_dbl_ptr)->interpolated_stride * MAX_SEARCH_AREA_HEIGHT, EB_N_PTR);

            EB_MALLOC(uint8_t *, (*object_dbl_ptr)->posh_buffer[list_index][ref_pic_index], sizeof(uint8_t) * (*object_dbl_ptr)->interpolated_stride * MAX_SEARCH_AREA_HEIGHT, EB_N_PTR);

            EB_MALLOC(uint8_t *, (*object_dbl_ptr)->posj_buffer[list_index][ref_pic_index], sizeof(uint8_t) * (*object_dbl_ptr)->interpolated_stride * MAX_SEARCH_AREA_HEIGHT, EB_N_PTR);

        }

    }

    EB_MALLOC(EbByte, (*object_dbl_ptr)->one_d_intermediate_results_buf0, sizeof(uint8_t)*MAX_SB_SIZE*MAX_SB_SIZE, EB_N_PTR);

    EB_MALLOC(EbByte, (*object_dbl_ptr)->one_d_intermediate_results_buf1, sizeof(uint8_t)*MAX_SB_SIZE*MAX_SB_SIZE, EB_N_PTR);

    for(pu_index= 0; pu_index < MAX_ME_PU_COUNT; pu_index++) {
        for( mecandidate_index = 0; mecandidate_index < MAX_ME_CANDIDATE_PER_PU; mecandidate_index++) {
            motion_estimetion_pred_unit_ctor(&((*object_dbl_ptr)->me_candidate[mecandidate_index]).pu[pu_index]);
        }
    }

    EB_MALLOC(uint8_t *, (*object_dbl_ptr)->avctemp_buffer, sizeof(uint8_t) * (*object_dbl_ptr)->interpolated_stride * MAX_SEARCH_AREA_HEIGHT, EB_N_PTR);

    EB_MALLOC(uint16_t *, (*object_dbl_ptr)->p_eight_pos_sad16x16, sizeof(uint16_t) * 8 * 16, EB_N_PTR);//16= 16 16x16 blocks in a LCU.       8=8search points

    return EB_ErrorNone;
}
