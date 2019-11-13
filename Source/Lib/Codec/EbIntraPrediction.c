/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <assert.h>
#include <stdint.h>

#include "EbIntraPrediction.h"
#include "EbDefinitions.h"

#include "vp9_reconintra.h"
#include "vp9_reconinter.h"

void intra_prediction(
    EncDecContext *context_ptr,
    EbByte         pred_buffer,
    uint16_t       pred_stride,
    int            plane)
{
    MACROBLOCKD *const xd = context_ptr->e_mbd;

#if 0
    const TX_SIZE tx_size = plane ? get_uv_tx_size(xd->mi[0], pd) : xd->mi[0]->tx_size;
#else
    const TX_SIZE tx_size = plane ? context_ptr->ep_block_stats_ptr->tx_size_uv : context_ptr->ep_block_stats_ptr->tx_size;
#endif

    PREDICTION_MODE mode;
    int block = context_ptr->bmi_index;

    if (tx_size == TX_4X4) {
        mode = plane == 0 ? get_y_mode(xd->mi[0], block) : xd->mi[0]->uv_mode;
    }
    else {
        mode = plane == 0 ? xd->mi[0]->mode : xd->mi[0]->uv_mode;
    }

    eb_vp9_predict_intra_block(
        context_ptr,
#if 0 // Hsan: reference samples generation done per block prior to fast loop @ generate_intra_reference_samples()
        xd,
#endif
        tx_size,
        mode,
        pred_buffer,
        pred_stride,
#if 0
        0,
        0,
#endif
        plane);
}

void inter_prediction(
    struct EncDecContext *context_ptr,
    EbByte                pred_buffer,
    uint16_t              pred_stride,
    int                   plane)
{
    MACROBLOCKD *const xd = context_ptr->e_mbd;

    const int mi_x =  context_ptr->block_origin_x;
    const int mi_y =  context_ptr->block_origin_y;

    const BLOCK_SIZE plane_bsize = plane ? context_ptr->ep_block_stats_ptr->bsize_uv: context_ptr->ep_block_stats_ptr->bsize;

    const int num_4x4_w = eb_vp9_num_4x4_blocks_wide_lookup[plane_bsize];
    const int num_4x4_h = eb_vp9_num_4x4_blocks_high_lookup[plane_bsize];
    const int bw = num_4x4_w << 2;
    const int bh = num_4x4_h << 2;

    if (xd->mi[0]->sb_type < BLOCK_8X8) {
        int i = 0, x, y;
        assert(xd->mi[0]->sb_type == BLOCK_8X8);
        for (y = 0; y < num_4x4_h; ++y)
            for (x = 0; x < num_4x4_w; ++x)
                build_inter_predictors(context_ptr, pred_buffer, pred_stride, xd, plane, i++, bw, bh, 4 * x, 4 * y, 4, 4, mi_x, mi_y);
    }
    else {
        build_inter_predictors(context_ptr, pred_buffer, pred_stride, xd, plane, 0, bw, bh, 0, 0, bw, bh, mi_x, mi_y);
    }
}
