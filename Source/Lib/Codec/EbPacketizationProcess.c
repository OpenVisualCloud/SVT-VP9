/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/*
 *  Copyright (c) 2013 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbPacketizationProcess.h"
#include "EbEntropyCodingResults.h"

#include "EbSystemResourceManager.h"
#include "EbSequenceControlSet.h"
#include "EbPictureControlSet.h"

#include "EbEntropyCoding.h"
#include "EbRateControlTasks.h"
#include "EbTime.h"
#include "stdint.h"

//struct vpx_write_bit_buffer *wb; // needs to be here

#include "vp9_bitstream.h"
#include "mem_ops.h"
#define RC_PRECISION                16
EbErrorType eb_vp9_packetization_context_ctor(
    PacketizationContext **context_dbl_ptr,
    EbFifo                *entropy_coding_input_fifo_ptr,
    EbFifo                *rate_control_tasks_output_fifo_ptr)
{
    PacketizationContext *context_ptr;
    EB_MALLOC(PacketizationContext*, context_ptr, sizeof(PacketizationContext), EB_N_PTR);
    *context_dbl_ptr = context_ptr;

    context_ptr->entropy_coding_input_fifo_ptr      = entropy_coding_input_fifo_ptr;
    context_ptr->rate_control_tasks_output_fifo_ptr  = rate_control_tasks_output_fifo_ptr;

    return EB_ErrorNone;
}

void ivf_write_frame_header(
    EbByte    write_byte_ptr,
    uint32_t *write_location,
    size_t    frame_size,
    uint64_t  pts,
    uint32_t *output_buffer_index)
{

    mem_put_le32(&write_byte_ptr[*write_location], (int)frame_size);
    *write_location = *write_location + 4;
    mem_put_le32(&write_byte_ptr[*write_location], (int)(pts & 0xFFFFFFFF));
    *write_location = *write_location + 4;
    mem_put_le32(&write_byte_ptr[*write_location], (int)(pts >> 32));
    *write_location = *write_location + 4;
    *output_buffer_index += 12;
}

#if  VP9_RC
void eb_vp9_update_rc_rate_tables(
    PictureControlSet  *picture_control_set_ptr,
    SequenceControlSet *sequence_control_set_ptr){

    EncodeContext* encode_context_ptr = (EncodeContext*)sequence_control_set_ptr->encode_context_ptr;

    uint32_t  intra_sad_interval_index;
    uint32_t  sad_interval_index;

    SbUnit   *sb_ptr;
    SbParams *sb_params_ptr;

    uint32_t  sb_index;
    int32_t   qp_index;

    // LCU Loop
    if (sequence_control_set_ptr->static_config.rate_control_mode > 0) {
        uint64_t  sadBits[NUMBER_OF_SAD_INTERVALS] = { 0 };
        uint32_t  count[NUMBER_OF_SAD_INTERVALS] = { 0 };

        encode_context_ptr->rate_control_tables_array_updated = EB_TRUE;

        for (sb_index = 0; sb_index < picture_control_set_ptr->sb_total_count; ++sb_index) {

            sb_ptr = picture_control_set_ptr->sb_ptr_array[sb_index];
            sb_params_ptr = &sequence_control_set_ptr->sb_params_array[sb_index];

            if (sb_params_ptr->is_complete_sb) {
                if (picture_control_set_ptr->slice_type == I_SLICE) {

                    intra_sad_interval_index = picture_control_set_ptr->parent_pcs_ptr->intra_sad_interval_index[sb_index];

                    sadBits[intra_sad_interval_index] += sb_ptr->sb_total_bits;
                    count[intra_sad_interval_index] ++;

                }
                else {
                    sad_interval_index = picture_control_set_ptr->parent_pcs_ptr->inter_sad_interval_index[sb_index];

                    sadBits[sad_interval_index] += sb_ptr->sb_total_bits;
                    count[sad_interval_index] ++;

                }
            }
        }
        {
            eb_vp9_block_on_mutex(encode_context_ptr->rate_table_update_mutex);

            uint64_t ref_qindex_dequant = (uint64_t)picture_control_set_ptr->parent_pcs_ptr->cpi->y_dequant[picture_control_set_ptr->base_qindex][1];
            uint64_t sad_bits_ref_dequant = 0;
            uint64_t weight = 0;
            {
                if (picture_control_set_ptr->slice_type == I_SLICE) {
                    if (sequence_control_set_ptr->input_resolution < INPUT_SIZE_4K_RANGE) {
                        for (sad_interval_index = 0; sad_interval_index < NUMBER_OF_INTRA_SAD_INTERVALS; sad_interval_index++) {

                            if (count[sad_interval_index] > 5) {
                                weight = 8;
                            }
                            else if (count[sad_interval_index] > 1) {
                                weight = 5;
                            }
                            else if (count[sad_interval_index] == 1) {
                                weight = 2;
                            }
                            if (count[sad_interval_index] > 0) {
                                sadBits[sad_interval_index] /= count[sad_interval_index];
                                sad_bits_ref_dequant = sadBits[sad_interval_index] * ref_qindex_dequant;
                                for (qp_index = sequence_control_set_ptr->static_config.min_qp_allowed; qp_index <= (int32_t)sequence_control_set_ptr->static_config.max_qp_allowed; qp_index++) {
                                    encode_context_ptr->rate_control_tables_array[qp_index].intra_sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index] =
                                        (EbBitNumber)(((weight * sad_bits_ref_dequant / picture_control_set_ptr->parent_pcs_ptr->cpi->y_dequant[eb_vp9_quantizer_to_qindex(qp_index)][1])
                                            + (10- weight) * (uint32_t)encode_context_ptr->rate_control_tables_array[qp_index].intra_sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index] + 5) / 10);

                                    encode_context_ptr->rate_control_tables_array[qp_index].intra_sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index] =
                                        MIN((uint16_t)encode_context_ptr->rate_control_tables_array[qp_index].intra_sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index], (uint16_t)((1 << 15) - 1));
                                }
                            }
                        }
                    }
                    else {
                        for (sad_interval_index = 0; sad_interval_index < NUMBER_OF_INTRA_SAD_INTERVALS; sad_interval_index++) {

                            if (count[sad_interval_index] > 10) {
                                weight = 8;
                            }
                            else if (count[sad_interval_index] > 5) {
                                weight = 5;
                            }
                            else if (count[sad_interval_index] >= 1) {
                                weight = 1;
                            }
                            if (count[sad_interval_index] > 0) {
                                sadBits[sad_interval_index] /= count[sad_interval_index];
                                sad_bits_ref_dequant = sadBits[sad_interval_index] * ref_qindex_dequant;
                                for (qp_index = sequence_control_set_ptr->static_config.min_qp_allowed; qp_index <= (int32_t)sequence_control_set_ptr->static_config.max_qp_allowed; qp_index++) {
                                    encode_context_ptr->rate_control_tables_array[qp_index].intra_sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index] =
                                        (EbBitNumber)(((weight * sad_bits_ref_dequant / picture_control_set_ptr->parent_pcs_ptr->cpi->y_dequant[eb_vp9_quantizer_to_qindex(qp_index)][1])
                                            + (10 - weight) * (uint32_t)encode_context_ptr->rate_control_tables_array[qp_index].intra_sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index] + 5) / 10);

                                    encode_context_ptr->rate_control_tables_array[qp_index].intra_sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index] =
                                        MIN((uint16_t)encode_context_ptr->rate_control_tables_array[qp_index].intra_sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index], (uint16_t)((1 << 15) - 1));
                                }

                            }
                        }
                    }

                }
                else {

                    if (sequence_control_set_ptr->input_resolution < INPUT_SIZE_4K_RANGE) {
                        for (sad_interval_index = 0; sad_interval_index < NUMBER_OF_SAD_INTERVALS; sad_interval_index++) {

                            if (count[sad_interval_index] > 5) {
                                weight = 8;
                            }
                            else if (count[sad_interval_index] > 1) {
                                weight = 5;
                            }
                            else if (count[sad_interval_index] == 1) {
                                weight = 1;
                            }

                            if (count[sad_interval_index] > 0) {
                                sadBits[sad_interval_index] /= count[sad_interval_index];
                                sad_bits_ref_dequant = sadBits[sad_interval_index] * ref_qindex_dequant;
                                for (qp_index = sequence_control_set_ptr->static_config.min_qp_allowed; qp_index <= (int32_t)sequence_control_set_ptr->static_config.max_qp_allowed; qp_index++) {
                                    encode_context_ptr->rate_control_tables_array[qp_index].sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index] =
                                        (EbBitNumber)(((weight * sad_bits_ref_dequant / picture_control_set_ptr->parent_pcs_ptr->cpi->y_dequant[eb_vp9_quantizer_to_qindex(qp_index)][1])
                                            + (10 - weight) * (uint32_t)encode_context_ptr->rate_control_tables_array[qp_index].sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index] + 5) / 10);
                                    encode_context_ptr->rate_control_tables_array[qp_index].sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index] =
                                        MIN((uint16_t)encode_context_ptr->rate_control_tables_array[qp_index].sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index], (uint16_t)((1 << 15) - 1));

                                }
                            }
                        }
                    }
                    else {
                        for (sad_interval_index = 0; sad_interval_index < NUMBER_OF_SAD_INTERVALS; sad_interval_index++) {
                            if (count[sad_interval_index] > 10) {
                                weight = 7;
                            }
                            else if (count[sad_interval_index] > 5) {
                                weight = 5;
                            }
                            else if (sad_interval_index > ((NUMBER_OF_SAD_INTERVALS >> 1) - 1) && count[sad_interval_index] > 1) {
                                weight = 1;
                            }

                            if (count[sad_interval_index] > 0) {
                                sadBits[sad_interval_index] /= count[sad_interval_index];
                                sad_bits_ref_dequant = sadBits[sad_interval_index] * ref_qindex_dequant;
                                for (qp_index = sequence_control_set_ptr->static_config.min_qp_allowed; qp_index <= (int32_t)sequence_control_set_ptr->static_config.max_qp_allowed; qp_index++) {
                                    encode_context_ptr->rate_control_tables_array[qp_index].sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index] =
                                        (EbBitNumber)(((weight * sad_bits_ref_dequant / picture_control_set_ptr->parent_pcs_ptr->cpi->y_dequant[eb_vp9_quantizer_to_qindex(qp_index)][1])
                                            + (10- weight) * (uint32_t)encode_context_ptr->rate_control_tables_array[qp_index].sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index] + 5) / 10);
                                    encode_context_ptr->rate_control_tables_array[qp_index].sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index] =
                                        MIN((uint16_t)encode_context_ptr->rate_control_tables_array[qp_index].sad_bits_array[picture_control_set_ptr->temporal_layer_index][sad_interval_index], (uint16_t)((1 << 15) - 1));

                                }
                            }
                        }
                    }

                }
            }
            //if (picture_control_set_ptr->temporalLayerIndex == 0) {
            //    qp_index = 40;
            //    SVT_LOG("UPDATE: %d\t%d\t%d\t%d\n", picture_control_set_ptr->pictureNumber, picture_control_set_ptr->pictureQp, ref_qindex_dequant, picture_control_set_ptr->ParentPcsPtr->cpi->y_dequant[eb_vp9_quantizer_to_qindex(qp_index)][1]);
            //
            //    for (sad_interval_index = 0; sad_interval_index < NUMBER_OF_INTRA_SAD_INTERVALS; sad_interval_index++) {

            //        SVT_LOG("%d\t%d\n", sad_interval_index, encodecontext_ptr->rateControlTablesArray[qp_index].sad_bits_array[picture_control_set_ptr->temporalLayerIndex][sad_interval_index]);
            //    }
            //}

            eb_vp9_release_mutex(encode_context_ptr->rate_table_update_mutex);
        }
    }
}
#endif

void* eb_vp9_packetization_kernel(void *input_ptr)
{
    // Context
    PacketizationContext      *context_ptr = (PacketizationContext*) input_ptr;

    PictureControlSet         *picture_control_set_ptr;

    // Config
    SequenceControlSet        *sequence_control_set_ptr;

    // Encoding Context
    EncodeContext             *encode_context_ptr;

    // Input
    EbObjectWrapper           *entropy_coding_results_wrapper_ptr;
    EntropyCodingResults      *entropy_coding_results_ptr;

    // Output
    EbObjectWrapper           *output_stream_wrapper_ptr;
    EbBufferHeaderType        *output_stream_ptr;
    EbObjectWrapper           *rate_control_tasks_wrapper_ptr;
    RateControlTasks          *rate_control_tasks_ptr;

    // Queue variables
    int32_t                    queue_entry_index;
    PacketizationReorderEntry *queue_entry_ptr;

    context_ptr->tot_shown_frames = 0;
    context_ptr->disp_order_continuity_count = 0;

    for(;;) {

        // Get EntropyCoding Results
        eb_vp9_get_full_object(
            context_ptr->entropy_coding_input_fifo_ptr,
            &entropy_coding_results_wrapper_ptr);
        entropy_coding_results_ptr = (EntropyCodingResults*) entropy_coding_results_wrapper_ptr->object_ptr;
        picture_control_set_ptr    = (PictureControlSet  *)    entropy_coding_results_ptr->picture_control_set_wrapper_ptr->object_ptr;
        sequence_control_set_ptr   = (SequenceControlSet *)   picture_control_set_ptr->sequence_control_set_wrapper_ptr->object_ptr;
        encode_context_ptr        = (EncodeContext*)        sequence_control_set_ptr->encode_context_ptr;

        //****************************************************
        // Input Entropy Results into Reordering Queue
        //****************************************************

        //get a new entry spot
        queue_entry_index = picture_control_set_ptr->parent_pcs_ptr->decode_order % PACKETIZATION_REORDER_QUEUE_MAX_DEPTH;
        queue_entry_ptr    = encode_context_ptr->packetization_reorder_queue[queue_entry_index];
        queue_entry_ptr->start_time_seconds = picture_control_set_ptr->parent_pcs_ptr->start_time_seconds;
        queue_entry_ptr->start_timeu_seconds = picture_control_set_ptr->parent_pcs_ptr->start_timeu_seconds;

        //TODO: buffer should be big enough to avoid a deadlock here. Add an assert that make the warning
        // Get Output Bitstream buffer
        output_stream_wrapper_ptr       = picture_control_set_ptr->parent_pcs_ptr->output_stream_wrapper_ptr;
        output_stream_ptr               = (EbBufferHeaderType*) output_stream_wrapper_ptr->object_ptr;
        output_stream_ptr               = (EbBufferHeaderType*)output_stream_wrapper_ptr->object_ptr;
        output_stream_ptr->flags      = 0;
        output_stream_ptr->flags      |= (encode_context_ptr->terminating_sequence_flag_received == EB_TRUE && picture_control_set_ptr->parent_pcs_ptr->decode_order == encode_context_ptr->terminating_picture_number) ? EB_BUFFERFLAG_EOS : 0;
        output_stream_ptr->n_filled_len = 0;
        output_stream_ptr->pts          = picture_control_set_ptr->parent_pcs_ptr->eb_input_ptr->pts;
        output_stream_ptr->dts          = picture_control_set_ptr->parent_pcs_ptr->decode_order - (uint64_t)(1 << sequence_control_set_ptr->hierarchical_levels) + 1;
        output_stream_ptr->pic_type     = picture_control_set_ptr->parent_pcs_ptr->is_used_as_reference_flag ?
            picture_control_set_ptr->parent_pcs_ptr->idr_flag ? EB_IDR_PICTURE :
            picture_control_set_ptr->slice_type : EB_NON_REF_PICTURE;
        output_stream_ptr->p_app_private = picture_control_set_ptr->parent_pcs_ptr->eb_input_ptr->p_app_private;

        // Get Empty Rate Control Input Tasks
        eb_vp9_get_empty_object(
            context_ptr->rate_control_tasks_output_fifo_ptr,
            &rate_control_tasks_wrapper_ptr);
        rate_control_tasks_ptr                                 = (RateControlTasks*) rate_control_tasks_wrapper_ptr->object_ptr;
        rate_control_tasks_ptr->picture_control_set_wrapper_ptr    = picture_control_set_ptr->picture_parent_control_set_wrapper_ptr;
        rate_control_tasks_ptr->task_type                       = RC_PACKETIZATION_FEEDBACK_RESULT;

        OutputBitstreamUnit *output_bitstream_ptr = (OutputBitstreamUnit*)picture_control_set_ptr->bitstream_ptr->output_bitstream_ptr;
        uint64_t size;

        size = 0;
        eb_vp9_reset_bitstream(
            picture_control_set_ptr->bitstream_ptr->output_bitstream_ptr);

        eb_vp9_pack_bitstream(
            picture_control_set_ptr,
            picture_control_set_ptr->parent_pcs_ptr->cpi,
            output_bitstream_ptr->buffer,
            &size,
            0,
            0);

        // Stream construction
        output_bitstream_ptr->written_bits_count = (uint32_t) size;
        output_bitstream_ptr->buffer = output_bitstream_ptr->buffer_begin + size;
        uint32_t write_location = 0;
        uint32_t read_location  = 0;
        EbByte read_byte_ptr;
        EbByte write_byte_ptr;
        uint32_t *output_buffer_index = (uint32_t*) &(output_stream_ptr->n_filled_len);
        uint32_t *output_buffer_size  = (uint32_t*) &(output_stream_ptr->n_alloc_len);
        read_byte_ptr  = (EbByte)output_bitstream_ptr->buffer_begin;
        write_byte_ptr = &output_stream_ptr->p_buffer[*output_buffer_index];

        // IVF data: 12-byte header
        // bytes 0 -  3 :   size of frame in bytes(not including the 12 - byte header)
        // bytes 4 - 11 :   64 - bit presentation timestamp
        //ivf_write_frame_header(write_byte_ptr, &write_location, size, picture_control_set_ptr->picture_number, output_buffer_index);

        // Frame Data: 12+-byte
        // AMIR to replace by memcopy
        while ((read_location < size)) {

            if ((*output_buffer_index) < (*output_buffer_size)) {
                write_byte_ptr[write_location++] = read_byte_ptr[read_location];
                *output_buffer_index += 1;
            }
            read_location++;
        }

        if (picture_control_set_ptr->parent_pcs_ptr->cpi->common.show_existing_frame) {
            output_stream_ptr->flags |= EB_BUFFERFLAG_SHOW_EXT;
            for (int show_existing_frame_index = 0; show_existing_frame_index < 4; show_existing_frame_index++) {

                size = 0;
                eb_vp9_reset_bitstream(
                    picture_control_set_ptr->bitstream_ptr->output_bitstream_ptr);

                eb_vp9_pack_bitstream(
                    picture_control_set_ptr,
                    picture_control_set_ptr->parent_pcs_ptr->cpi,
                    output_bitstream_ptr->buffer,
                    &size,
                    1,
                    show_existing_frame_index);

                // Stream construction
                output_bitstream_ptr->written_bits_count = (uint32_t)size;
                output_bitstream_ptr->buffer = output_bitstream_ptr->buffer_begin + size;
                uint32_t  write_location = 0;
                uint32_t  read_location = 0;
                EbByte read_byte_ptr;
                EbByte write_byte_ptr;
                uint32_t *output_buffer_index = (uint32_t*) &(output_stream_ptr->n_filled_len);
                uint32_t *output_buffer_size = (uint32_t*) &(output_stream_ptr->n_alloc_len);
                read_byte_ptr = (EbByte)output_bitstream_ptr->buffer_begin;
                write_byte_ptr = &output_stream_ptr->p_buffer[*output_buffer_index];

                // Frame Data: 12+-byte
#if COPY_STREAM
                EB_MEMCPY(write_byte_ptr+write_location, read_byte_ptr, size);
#else
                while ((read_location < size)) {

                    if ((*output_buffer_index) < (*output_buffer_size)) {
                        write_byte_ptr[write_location++] = read_byte_ptr[read_location];
                        *output_buffer_index += 1;
                    }
                    read_location++;
                }
#endif
            }

        }

        // Send the number of bytes per frame to RC
        picture_control_set_ptr->parent_pcs_ptr->total_num_bits = output_stream_ptr->n_filled_len << 3;
        queue_entry_ptr->actual_bits = picture_control_set_ptr->parent_pcs_ptr->total_num_bits;
        queue_entry_ptr->total_num_bits = picture_control_set_ptr->parent_pcs_ptr->total_num_bits;

#if  VP9_RC
        // update the rate tables used in RC based on the encoded bits of each sb
        eb_vp9_update_rc_rate_tables(
            picture_control_set_ptr,
            sequence_control_set_ptr);
#endif

        queue_entry_ptr->frame_type = picture_control_set_ptr->parent_pcs_ptr->cpi->common.frame_type;
        queue_entry_ptr->intra_only = picture_control_set_ptr->parent_pcs_ptr->cpi->common.intra_only;
        queue_entry_ptr->poc = picture_control_set_ptr->picture_number;
        EB_MEMCPY(&queue_entry_ptr->ref_signal, &picture_control_set_ptr->parent_pcs_ptr->ref_signal, sizeof(RpsNode));

        queue_entry_ptr->slice_type = picture_control_set_ptr->slice_type;
        queue_entry_ptr->ref_poc_list0 = picture_control_set_ptr->parent_pcs_ptr->ref_pic_poc_array[REF_LIST_0];
        queue_entry_ptr->ref_poc_list1 = picture_control_set_ptr->parent_pcs_ptr->ref_pic_poc_array[REF_LIST_1];

        queue_entry_ptr->show_frame = (uint8_t) picture_control_set_ptr->parent_pcs_ptr->cpi->common.show_frame;
        queue_entry_ptr->show_existing_frame = picture_control_set_ptr->parent_pcs_ptr->cpi->common.show_existing_frame;
        queue_entry_ptr->show_existing_frame_index_array[0] = picture_control_set_ptr->parent_pcs_ptr->show_existing_frame_index_array[0];
        queue_entry_ptr->show_existing_frame_index_array[1] = picture_control_set_ptr->parent_pcs_ptr->show_existing_frame_index_array[1];
        queue_entry_ptr->show_existing_frame_index_array[2] = picture_control_set_ptr->parent_pcs_ptr->show_existing_frame_index_array[2];
        queue_entry_ptr->show_existing_frame_index_array[3] = picture_control_set_ptr->parent_pcs_ptr->show_existing_frame_index_array[3];

        //Store the buffer in the Queue
        queue_entry_ptr->output_stream_wrapper_ptr = output_stream_wrapper_ptr;

        if (sequence_control_set_ptr->static_config.speed_control_flag) {
            // update speed control variables
            eb_vp9_block_on_mutex(encode_context_ptr->sc_buffer_mutex);
            encode_context_ptr->sc_frame_out++;
            eb_vp9_release_mutex(encode_context_ptr->sc_buffer_mutex);
        }

        // Post Rate Control Taks
        eb_vp9_post_full_object(rate_control_tasks_wrapper_ptr);

        //Release the Parent PCS then the Child PCS
        eb_vp9_release_object(entropy_coding_results_ptr->picture_control_set_wrapper_ptr);//Child

        // Release the Entropy Coding Result
        eb_vp9_release_object(entropy_coding_results_wrapper_ptr);

        //****************************************************
        // Process the head of the queue
        //****************************************************
        // Look at head of queue and see if any picture is ready to go
        queue_entry_ptr = encode_context_ptr->packetization_reorder_queue[encode_context_ptr->packetization_reorder_queue_head_index];

        while (queue_entry_ptr->output_stream_wrapper_ptr != EB_NULL) {

            output_stream_wrapper_ptr = queue_entry_ptr->output_stream_wrapper_ptr;
            output_stream_ptr = (EbBufferHeaderType*)output_stream_wrapper_ptr->object_ptr;

#if VP9_RC_PRINTS
           // SVT_LOG("%d\t%d%%\n",queue_entry_ptr->picture_number, encode_context_ptr->vbv_level*100/ sequence_control_set_ptr->static_config.vbv_buf_size);
#endif
#if ADP_STATS_PER_LAYER
            if (queue_entry_ptr->picture_number == sequence_control_set_ptr->static_config.frames_to_be_encoded - 1) {

                uint8_t layerIndex;
                SVT_LOG("\nFS\tAVC\tF_BDP\tL_BDP\tF_MDC\tL_MDC\tP_MDC\tP_MDC_1_NFL");
                for (layerIndex = 0; layerIndex < 4; layerIndex++) {
                    SVT_LOG("\n/***************************Layer %d Stats ********************************/\n", layerIndex);
                    if (sequence_control_set_ptr->total_count[layerIndex]) {
                        SVT_LOG("%d\t", ((sequence_control_set_ptr->fs_count[layerIndex] * 100) / sequence_control_set_ptr->total_count[layerIndex]));
                        SVT_LOG("%d\t", ((sequence_control_set_ptr->avc_count[layerIndex] * 100) / sequence_control_set_ptr->total_count[layerIndex]));
                        SVT_LOG("%d\t", ((sequence_control_set_ptr->f_bdp_count[layerIndex] * 100) / sequence_control_set_ptr->total_count[layerIndex]));
                        SVT_LOG("%d\t", ((sequence_control_set_ptr->l_bdp_count[layerIndex] * 100) / sequence_control_set_ptr->total_count[layerIndex]));
                        SVT_LOG("%d\t", ((sequence_control_set_ptr->f_mdc_count[layerIndex] * 100) / sequence_control_set_ptr->total_count[layerIndex]));
                        SVT_LOG("%d\t", ((sequence_control_set_ptr->l_mdc_count[layerIndex] * 100) / sequence_control_set_ptr->total_count[layerIndex]));
                        SVT_LOG("%d\t", ((sequence_control_set_ptr->pred_count[layerIndex] * 100) / sequence_control_set_ptr->total_count[layerIndex]));
                        SVT_LOG("%d\t", ((sequence_control_set_ptr->pred1_nfl_count[layerIndex] * 100) / sequence_control_set_ptr->total_count[layerIndex]));
                    }
                }
                SVT_LOG("\n");
            }
#endif

            {
                int32_t i;
#if !VP9_RC_PRINTS && PKT_PRINT
                uint8_t  showTab[] = { 'H', 'V' };
#endif
                //Countinuity count check of visible frames
                if (queue_entry_ptr->show_frame) {
                    if (context_ptr->disp_order_continuity_count == queue_entry_ptr->poc)
                        context_ptr->disp_order_continuity_count++;
                    else
                    {
                        SVT_LOG("SVT [ERROR]: disp_order_continuity_count Error1 POC:%i\n", (int)queue_entry_ptr->poc);
                        return EB_NULL;
                    }
                }

                if (queue_entry_ptr->show_existing_frame) {
                    if (context_ptr->disp_order_continuity_count == context_ptr->dpb_disp_order[queue_entry_ptr->show_existing_frame_index_array[0]])
                        context_ptr->disp_order_continuity_count += 4;
                    else
                    {
                        SVT_LOG("SVT [ERROR]: disp_order_continuity_count Error2 POC:%i\n", (int)queue_entry_ptr->poc);
                        return EB_NULL;
                    }
                }

                //update total number of shown frames
                if (queue_entry_ptr->show_frame)
                    context_ptr->tot_shown_frames++;
                if (queue_entry_ptr->show_existing_frame)
                    context_ptr->tot_shown_frames += 4;

                //implement the GOP here - Serial dec order
                if (queue_entry_ptr->frame_type == KEY_FRAME)
                {
                    //reset the DPB on a Key frame
                    for (i = 0; i < 8; i++)
                    {
                        context_ptr->dpb_dec_order[i] = queue_entry_ptr->picture_number;
                        context_ptr->dpb_disp_order[i] = queue_entry_ptr->poc;
                    }
#if !VP9_RC_PRINTS && PKT_PRINT
                    SVT_LOG("%i  %i  %c ****KEY***** %i frames\n", (int)queue_entry_ptr->picture_number, (int)queue_entry_ptr->poc, showTab[queue_entry_ptr->show_frame], (int)context_ptr->tot_shown_frames);
#endif
                }
                else
                {
#if !VP9_RC_PRINTS && PKT_PRINT
                    int LASTrefIdx = queue_entry_ptr->ref_signal.ref_dpb_index[0];
                    int BWDrefIdx = queue_entry_ptr->ref_signal.ref_dpb_index[2];
#endif
                    //Update the DPB
                    for (i = 0; i < 8; i++)
                    {
                        if ((queue_entry_ptr->ref_signal.refresh_frame_mask >> i) & 1)
                        {
                            context_ptr->dpb_dec_order[i] = queue_entry_ptr->picture_number;
                            context_ptr->dpb_disp_order[i] = queue_entry_ptr->poc;
                        }
                    }
#if 0
                    SVT_LOG("DPB:\t");
                    for (i = 0; i < 8; i++)
                    {
                       SVT_LOG("%i | ", (int)context_ptr->dpb_disp_order[i]);
                    }
                    SVT_LOG("\n");
#endif
#if !VP9_RC_PRINTS && PKT_PRINT
                    if (queue_entry_ptr->frame_type == INTER_FRAME && queue_entry_ptr->intra_only == EB_FALSE)
                    {
                        if (queue_entry_ptr->show_existing_frame)
                            SVT_LOG("%i (%i  %i)    %i  (%i  %i)   %c  showEx: %i %i %i %i  %i frames\n",
                            (int)queue_entry_ptr->picture_number, (int)context_ptr->dpb_dec_order[LASTrefIdx], (int)context_ptr->dpb_dec_order[BWDrefIdx],
                                (int)queue_entry_ptr->poc, (int)context_ptr->dpb_disp_order[LASTrefIdx], (int)context_ptr->dpb_disp_order[BWDrefIdx],
                                showTab[queue_entry_ptr->show_frame],
                                (int)context_ptr->dpb_disp_order[queue_entry_ptr->show_existing_frame_index_array[0]],
                                (int)context_ptr->dpb_disp_order[queue_entry_ptr->show_existing_frame_index_array[1]],
                                (int)context_ptr->dpb_disp_order[queue_entry_ptr->show_existing_frame_index_array[2]],
                                (int)context_ptr->dpb_disp_order[queue_entry_ptr->show_existing_frame_index_array[3]],
                                (int)context_ptr->tot_shown_frames);
                        else
                            SVT_LOG("%i (%i  %i)    %i  (%i  %i)   %c  %i frames\n",
                            (int)queue_entry_ptr->picture_number, (int)context_ptr->dpb_dec_order[LASTrefIdx], (int)context_ptr->dpb_dec_order[BWDrefIdx],
                                (int)queue_entry_ptr->poc, (int)context_ptr->dpb_disp_order[LASTrefIdx], (int)context_ptr->dpb_disp_order[BWDrefIdx],
                                showTab[queue_entry_ptr->show_frame], (int)context_ptr->tot_shown_frames);

                        if (queue_entry_ptr->ref_poc_list0 != context_ptr->dpb_disp_order[LASTrefIdx])
                        {
                            SVT_LOG("SVT [ERROR]: L0 MISMATCH POC:%i\n", (int)queue_entry_ptr->poc);
                            return EB_NULL;
                        }

                        if (sequence_control_set_ptr->hierarchical_levels == 3 && queue_entry_ptr->slice_type == B_SLICE && queue_entry_ptr->ref_poc_list1 != context_ptr->dpb_disp_order[BWDrefIdx])
                        {
                            SVT_LOG("SVT [ERROR]: L1 MISMATCH POC:%i\n", (int)queue_entry_ptr->poc);
                            return EB_NULL;
                        }

                    }
                    else
                    {
                        if (queue_entry_ptr->show_existing_frame)
                            SVT_LOG("%i  %i  %c   showEx: %i ----INTRA---- %i frames \n", (int)queue_entry_ptr->picture_number, (int)queue_entry_ptr->poc,
                                showTab[queue_entry_ptr->show_frame], (int)context_ptr->dpb_disp_order[queue_entry_ptr->show_existing_frame_index_array[0]], (int)context_ptr->tot_shown_frames);
                        else
                            SVT_LOG("%i  %i  %c   ----INTRA---- %i frames\n", (int)queue_entry_ptr->picture_number, (int)queue_entry_ptr->poc,
                            (int)showTab[queue_entry_ptr->show_frame], (int)context_ptr->tot_shown_frames);
                    }
#endif
                }

            }

            // Calculate frame latency in milli_seconds
            double latency = 0.0;
            uint64_t finish_time_seconds = 0;
            uint64_t finish_timeu_seconds = 0;
            svt_vp9_get_time(&finish_time_seconds, &finish_timeu_seconds);

            latency = svt_vp9_compute_overall_elapsed_time_ms(
                queue_entry_ptr->start_time_seconds,
                queue_entry_ptr->start_timeu_seconds, finish_time_seconds,
                finish_timeu_seconds);

            output_stream_ptr->n_tick_count = (uint32_t)latency;

            /* update VBV plan */
            if (encode_context_ptr->vbv_max_rate &&
                encode_context_ptr->vbv_buf_size) {
                int64_t buffer_fill_temp = (int64_t)(encode_context_ptr->buffer_fill);

                buffer_fill_temp -= queue_entry_ptr->actual_bits;
                buffer_fill_temp = MAX(buffer_fill_temp, 0);
                buffer_fill_temp = (int64_t)(buffer_fill_temp + (encode_context_ptr->vbv_max_rate * (1.0 / (sequence_control_set_ptr->frame_rate >> RC_PRECISION))));
                buffer_fill_temp = MIN(buffer_fill_temp, encode_context_ptr->vbv_buf_size);
                encode_context_ptr->buffer_fill = buffer_fill_temp;
                //printf("totalNumBits = %lld \t bufferFill = %lld \t pictureNumber = %lld \n", queue_entry_ptr->actual_bits, encode_context_ptr->bufferFill, picture_control_set_ptr->picture_number);
            }
            // Release the Bitstream wrapper object
            eb_vp9_post_full_object(output_stream_wrapper_ptr);
            // Reset the Reorder Queue Entry
            queue_entry_ptr->picture_number += PACKETIZATION_REORDER_QUEUE_MAX_DEPTH;
            queue_entry_ptr->output_stream_wrapper_ptr = (EbObjectWrapper *)EB_NULL;

            // Increment the Reorder Queue head Ptr
            encode_context_ptr->packetization_reorder_queue_head_index =
                (encode_context_ptr->packetization_reorder_queue_head_index == PACKETIZATION_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encode_context_ptr->packetization_reorder_queue_head_index + 1;

            queue_entry_ptr = encode_context_ptr->packetization_reorder_queue[encode_context_ptr->packetization_reorder_queue_head_index];
        }
    }
    return EB_NULL;
}
